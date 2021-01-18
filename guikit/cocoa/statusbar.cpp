
@implementation StatusImageView : NSImageView

-(id) initWith:(GUIKIT::StatusBar::Part*)partPtr {
    if(self = [super initWithFrame:NSMakeRect(0, 0, 0, 0)]) {
        part = partPtr;
        [self setTarget:self];
    }
    return self;
}
- (void)mouseDown:(NSEvent*)event {
    
    if (part && part->popupMenu)
        [NSMenu popUpContextMenu: [part->popupMenu->p.cocoaBase cocoaMenu] withEvent:event forView:NULL];
    
    if (part && part->onClick)
        part->onClick();
}

-(void) resetCursorRects {
    if (part && (part->onClick || part->popupMenu) ) {
        [self discardCursorRects];
        
        [self addCursorRect: [self bounds] cursor: [NSCursor pointingHandCursor]];
    }
}

@end

namespace GUIKIT {
    
pStatusBar::pStatusBar(StatusBar& statusBar) : statusBar(statusBar) {
    cocoaView = nullptr;
}

pStatusBar::~pStatusBar() {
    
    destroy();
}

auto pStatusBar::destroy() -> void {
    @autoreleasepool {
        [cocoaView release];
        cocoaView = nil;
    }
}

auto pStatusBar::create() -> void {
    
    destroy();
    
    @autoreleasepool {
        cocoaView = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 0, 0)];
        [cocoaView setHidden:YES];
        
        [cocoaView setBackgroundColor: [NSColor textBackgroundColor]];

        [[statusBar.window()->p.cocoaWindow contentView] addSubview:cocoaView positioned:NSWindowBelow relativeTo:nil];
    }
    update();
}

auto pStatusBar::getHeight() -> unsigned {
    
    auto window = statusBar.window();
    if (!window || !cocoaView)
        return 0;

    if(!window->statusVisible())
        return 0;
    
    @autoreleasepool {
        NSArray* subviews = [cocoaView subviews];
        
        for (NSView* view in subviews) {
            
            if([view respondsToSelector:@selector(setFont:)]) {
                NSFont* font = [view font];
                return pFont::size(font, " ").height + 3;
            }
        }
        return 0;
    }
}

auto pStatusBar::reposition() -> void {
    
    @autoreleasepool {
        auto window = statusBar.window();
        if (!window)
            return;
        
        NSRect area = [window->p.cocoaWindow contentRectForFrameRect:[window->p.cocoaWindow frame]];
        [cocoaView setFrame:NSMakeRect(0, 0, area.size.width, getHeight())];
        [[window->p.cocoaWindow contentView] setNeedsDisplay:YES];
        
        update();
    }
}

auto pStatusBar::setText(std::string text) -> void {
    @autoreleasepool {
        if (!cocoaView)
            return;
        
        [usedWidgets[0] setStringValue:[NSString stringWithUTF8String:text.c_str()]];
    }
}

auto pStatusBar::setFont(std::string font) -> void {
    @autoreleasepool {
        if (!cocoaView)
            return;

        NSArray* subviews = [cocoaView subviews];
        
        for (NSView* view in subviews) {
            
            if([view respondsToSelector:@selector(setFont:)])
                [view setFont:pFont::cocoaFont(font)];
        }
    }
    
    reposition();
}
    
auto pStatusBar::getWidth(std::string text) -> unsigned {
    if (text == "")
        return 0;
    
    @autoreleasepool {
        Label label;
        label.setText( text );
        label.setFont( statusBar.font().empty() ? Font::system() : statusBar.font() );
        
        return label.minimumSize().width + 5;
    }
}

auto pStatusBar::setVisible(bool visible) -> void {
    
    if (!statusBar.window() || !cocoaView)
        return;
    
    [cocoaView setHidden:!visible];
}

auto pStatusBar::update() -> void {
    
    if (!statusBar.window() || !cocoaView)
        return;
    
@autoreleasepool {
    while([[cocoaView subviews] count] > 0) {
        
        NSView* view = [[cocoaView subviews] objectAtIndex:0];
        
        if([view respondsToSelector:@selector(setImage:)])
            [[view image] release];
        
        [view removeFromSuperview];
    }
    
    for( auto view : usedWidgets )
        [view release];

    usedWidgets.clear();
    
    auto& parts = statusBar.state.parts;

    auto window = statusBar.window();
    
    NSRect area = [window->p.cocoaWindow contentRectForFrameRect:[window->p.cocoaWindow frame]];
    
    Label* label = new Label;
    label->setText( statusBar.text().empty() ? " " : statusBar.text() );
    label->setFont( statusBar.font().empty() ? Font::system() : statusBar.font() );
    
    unsigned textHeight = label->minimumSize().height;
    
    if (parts.size() == 0) { // simple status view
        usedWidgets.push_back( label->p.cocoaView );

        [label->p.cocoaView setFrame:NSMakeRect(5, -2, area.size.width, textHeight)];

        [cocoaView addSubview: label->p.cocoaView];
        return;
    }
    
    delete label;
    
    unsigned width = area.size.width;
    width -= 8;
    
    for (int i = parts.size() - 1; i >= 0; i-- ) {
        
        auto& part = parts[i];
        
        if (!part.visible)
            continue;
        
        if (part.image)
            width -= part.image->width + 3;
        else
            width -= part.width;
    }
    
    unsigned xPos = 0;
    NSView* view;
    
    for (auto& part : parts) {
        
        if (!part.visible)
            continue;
        
        part.position = usedWidgets.size();
        
        if (part.image) {
            
            NSImage* image = NSMakeImage( *part.image );
            
            view = [[StatusImageView alloc] initWith:&part];
            
            unsigned yPos = (textHeight - part.image->height) / 2;
            
            [view setFrame:NSMakeRect(xPos, yPos, part.image->width, part.image->height)];
            
            [view setImage: image];
            
            xPos += part.image->width + 3;
            
        } else {
            Label* label = new Label;
            label->setText( part.text );
            label->setFont( statusBar.font().empty() ? Font::system() : statusBar.font() );
            label->p.part = &part;
            
            if (part.overrideForegroundColor != -1)
                label->setForegroundColor( part.overrideForegroundColor );

            label->setAlign( part.alignRight ? Label::Align::Right : Label::Align::Left );
                
            if (xPos == 0)
                width += part.width;
            else
                width = part.width;
                
            view = label->p.cocoaView;
                
            [view setFrame:NSMakeRect(xPos, -2, width, textHeight)];
            
            xPos += width;
        }
        
        usedWidgets.push_back( view );
        
        [view setToolTip:[NSString stringWithUTF8String:part.tooltip.c_str()]];
        
        [cocoaView addSubview: view ];
    }
}
}

auto pStatusBar::updatePart( StatusBar::Part& part ) -> void {
    
    if (part.position >= usedWidgets.size())
        return;
    
    if (!statusBar.window() || !cocoaView)
        return;

    @autoreleasepool {
        NSView* view = usedWidgets[ part.position ];
        
        if ( part.image ) {
            
            [[view image] release];
            
            NSImage* image = NSMakeImage( *part.image );
            
            [view setImage: image];

        } else {

            if (part.alignRight)
                [view setAlignment:NSRightTextAlignment];
            else
                [view setAlignment:NSLeftTextAlignment];

            [view setStringValue:[NSString stringWithUTF8String:part.text.c_str()]];
            
            NSColor* textColor = [NSColor textColor];
            
            if (part.overrideForegroundColor != -1)
                textColor = pHelper::getColor( part.overrideForegroundColor );

            [view setTextColor: textColor];
        }
        
        [view setToolTip:[NSString stringWithUTF8String:part.tooltip.c_str()]];
    }
    
}

}
