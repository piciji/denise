
@implementation StatusImageView : NSImageView

-(id) initWith:(GUIKIT::StatusBar::Part*)partPtr {
    if(self = [super initWithFrame:NSMakeRect(0, 0, 0, 0)]) {
        part = partPtr;
        [self setTarget:self];
    }
    return self;
}
- (void)mouseDown:(NSEvent*)event {
    
    if (part && part->popupMenu) {
        GUIKIT::pApplication::observeMenu([part->popupMenu->p.cocoaBase cocoaMenu]);
        
        [NSMenu popUpContextMenu: [part->popupMenu->p.cocoaBase cocoaMenu] withEvent:event forView:NULL];
    }
    
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
    //    [cocoaView setHidden:YES];
        
        [cocoaView setWantsLayer:YES];

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
        
        window->p.positionBGView();
    }
}

auto pStatusBar::setText(const std::string& text) -> void {
    @autoreleasepool {
        if (!cocoaView)
            return;
        
        [usedWidgets[0]->p.cocoaView setStringValue:[NSString stringWithUTF8String:text.c_str()]];
    }
}

auto pStatusBar::setFont(const std::string& font) -> void {
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
    
auto pStatusBar::getWidth(const std::string& text) -> unsigned {
    if (text == "")
        return 0;
    
    @autoreleasepool {
        Label label;
        label.setText( text );
        label.setFont( statusBar.font().empty() ? Font::system() : statusBar.font() );
        
        return label.minimumSize().width + 7;
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

    for( auto separator : separators )
        [separator release];
    
    separators.clear();
    
    for( auto widget : usedWidgets )
        delete widget;

    usedWidgets.clear();
    
    auto& parts = statusBar.state.parts;

    auto window = statusBar.window();
    
    NSRect area = [window->p.cocoaWindow contentRectForFrameRect:[window->p.cocoaWindow frame]];
    
    Label* label = new Label;
    label->setText( statusBar.text().empty() ? " " : statusBar.text() );
    label->setFont( statusBar.font().empty() ? Font::system() : statusBar.font() );
    
    unsigned textHeight = label->minimumSize().height;
    
    if (parts.size() == 0) { // simple status view
        usedWidgets.push_back( label );

        [label->p.cocoaView setFrame:NSMakeRect(5, -2, area.size.width, textHeight)];

        [cocoaView addSubview: label->p.cocoaView];
        return;
    }
    
    delete label;
    
    unsigned countVisible = 0;

    for(unsigned i = 0; i < parts.size(); i++) {
        auto& part = parts[i];
        
        if (!part.visible)
            continue;
        
        countVisible++;
    }
    
    unsigned xPos = 5;
    NSView* view;
    
    unsigned i = 0;
    for (auto& part : parts) {
        
        if (!part.visible)
            continue;
        
        i++;
        part.position = usedWidgets.size();
        
        if (part.image) {
            Widget* widget = new Widget();
            
            NSImage* image = NSMakeImage( *part.image );
            
            widget->p.cocoaView = [[StatusImageView alloc] initWith:&part];
            
            view = widget->p.cocoaView;
            
            unsigned yPos = (textHeight - part.image->height) / 2;
            
            [view setFrame:NSMakeRect(xPos, yPos, part.image->width, part.image->height)];
            
            [view setImage: image];
            
            xPos += part.image->width + 4;
            
            usedWidgets.push_back( widget );

        } else if (part.sliderLength) {
            Slider* slider = new Slider(Slider::Orientation::HORIZONTAL);
            slider->setLength( part.sliderLength );
            slider->setPositon( part.sliderPosition );
            slider->onChange = part.onChange;
            view = slider->p.cocoaView;

            if (i == countVisible && (area.size.width > xPos) )
                [view setFrame:NSMakeRect(xPos, -2, area.size.width - xPos, getHeight())];
            else
                [view setFrame:NSMakeRect(xPos, -2, part.width, getHeight())];

            xPos += part.width;

            usedWidgets.push_back( slider );

        } else {
            Label* label = new Label;
            label->setText( part.text );
            label->setFont( statusBar.font().empty() ? Font::system() : statusBar.font() );
            label->p.part = &part;
            
            if (part.overrideForegroundColor != -1)
                label->setForegroundColor( part.overrideForegroundColor );

            label->setAlign( part.alignRight ? Label::Align::Right : Label::Align::Left );

            view = label->p.cocoaView;
                
            if (i == countVisible && (area.size.width > xPos) )
                [view setFrame:NSMakeRect(xPos, -2, area.size.width - xPos, textHeight)];
            else
                [view setFrame:NSMakeRect(xPos, -2, part.width, textHeight)];
            
            xPos += part.width;
            
            usedWidgets.push_back( label );
        }
        
        [view setToolTip:[NSString stringWithUTF8String:part.tooltip.c_str()]];
        
        [cocoaView addSubview: view ];
        
        if (part.appendSeparator && (usedWidgets.size() < countVisible) ) {
            NSBox* verticalSeparator = [[NSBox alloc] initWithFrame:NSMakeRect(xPos, 2, 1.0, textHeight - 5)];
            [verticalSeparator setBoxType:NSBoxSeparator];
            
            [cocoaView addSubview: verticalSeparator ];
            
            separators.push_back( verticalSeparator );
            
            xPos += 4;
        }
    }
}
}

auto pStatusBar::updatePart( StatusBar::Part& part ) -> void {

    if (part.position < 0) {
        statusBar.state.updatePending = true;
        return;
    }

    if (part.position >= usedWidgets.size())
        return;
    
    if (!statusBar.window() || !cocoaView)
        return;

    @autoreleasepool {
        Widget* widget = usedWidgets[ part.position ];
        
        if ( part.image ) {
            
            [[widget->p.cocoaView image] release];
            
            NSImage* image = NSMakeImage( *part.image );
            
            [widget->p.cocoaView setImage: image];

        } else if (part.sliderLength) {
            Slider* slider = (Slider*)widget;
            if (slider->position() != part.sliderPosition)
                slider->setPositon( part.sliderPosition );

        } else {
            Label* label = (Label*)widget;
            Label::Align align = part.alignRight ? Label::Align::Right : Label::Align::Left;

            if (label->align() != align)
                label->setAlign( align );

            label->setText( part.text );

            if (part.overrideForegroundColor != -1)
                label->setForegroundColor( part.overrideForegroundColor );
            else
                label->resetForegroundColor();
        }
        
        [widget->p.cocoaView setToolTip:[NSString stringWithUTF8String:part.tooltip.c_str()]];
    }
    
}

}
