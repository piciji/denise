
@implementation CocoaLabel : NSTextField

-(id) initWith:(GUIKIT::Label&)labelReference {
    if(self = [super initWithFrame:NSMakeRect(0, 0, 0, 0)]) {
        label = &labelReference;
        
        [self setAlignment:NSTextAlignmentLeft];
        [self setBordered:NO];
        [self setDrawsBackground:NO];
        [self setEditable:NO];
    }
    return self;
}
- (void)mouseDown:(NSEvent*)event {
    if (label->p.part && label->p.part->onClick)
        label->p.part->onClick();
            
    if (label->p.part && label->p.part->popupMenu) {
        GUIKIT::pApplication::observeMenu([(id)label->p.part->popupMenu->p.cocoaBase cocoaMenu]);
        
        [NSMenu popUpContextMenu: [(id)label->p.part->popupMenu->p.cocoaBase cocoaMenu] withEvent:event forView:self];
    }
}

-(void) resetCursorRects {
    if (label->p.part && (label->p.part->onClick || label->p.part->popupMenu) ) {
        [self discardCursorRects];
        
        [self addCursorRect: [self bounds] cursor: [NSCursor pointingHandCursor]];
    }
}

@end

namespace GUIKIT {

auto pLabel::minimumSize() -> Size {
    Size size = getMinimumSize();
    
    if ([[(id)cocoaView font] isFixedPitch])
        return {size.width + 1, size.height};

    return {size.width + 1, size.height + 4};
}

auto pLabel::setAlign( Label::Align align ) -> void {
    if (align == Label::Align::Left)
        [(id)cocoaView setAlignment:NSTextAlignmentLeft];
    else
        [(id)cocoaView setAlignment:NSTextAlignmentRight];
}
    
auto pLabel::setGeometry(Geometry geometry) -> void {
    unsigned height = getMinimumSize().height;
    unsigned widgetHeight = geometry.height + 4;
    auto offset = geometry;
    
    if(widgetHeight > height) {
        unsigned diff = widgetHeight - height;
        offset.y += diff >> 1;
        offset.height -= diff >> 1;
    }
    
    pWidget::setGeometry({
        offset.x - 2, offset.y - 3,
        offset.width + 4, offset.height + 6
    });
}
    
auto pLabel::setText(const std::string& text) -> void {
    @autoreleasepool {
        if([NSThread isMainThread])
            [(id)cocoaView setStringValue:[NSString stringWithUTF8String:text.c_str()]];
        else {
            std::string _text = text;
            dispatch_group_t group = dispatch_group_create();
            dispatch_group_async(group, dispatch_get_main_queue(), ^{
                [(id)cocoaView setStringValue:[NSString stringWithUTF8String:_text.c_str()]];
            });
            
         //   dispatch_group_notify(group, dispatch_get_main_queue(), ^{
                
           // });
        }
    }
    calculatedMinimumSize.updated = false;
}
    
auto pLabel::setEnabled(bool enabled) -> void {
    
    NSColor* textColor = [NSColor textColor];
    
    if(label.overrideForegroundColor()) {
        unsigned color = label.foregroundColor();
        textColor = pHelper::getColor( color );
    }
    
    [(id)cocoaView setTextColor: enabled ? textColor : [NSColor grayColor]];
    pWidget::setEnabled(enabled);
}
    
auto pLabel::setForegroundColor(unsigned color) -> void {
    setEnabled( label.enabled() );
}


auto pLabel::init() -> void {
    cocoaView = [[CocoaLabel alloc] initWith:label];
}   
    
}       
