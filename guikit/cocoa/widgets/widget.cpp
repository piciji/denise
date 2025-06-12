
namespace GUIKIT {

pWidget::pWidget(Widget& widget) : widget(widget) {
    setFont(Font::system());
}

pWidget::~pWidget() {
    @autoreleasepool {
        [cocoaView release];
    }
}

auto pWidget::focused() -> bool {
    @autoreleasepool {
        return cocoaView && cocoaView == [[cocoaView window] firstResponder];
    }
}

auto pWidget::setFocused() -> void {
    @autoreleasepool {
        if(cocoaView) [[cocoaView window] makeFirstResponder:cocoaView];
    }
}

auto pWidget::setFont(std::string font) -> void {
    @autoreleasepool {
        if([cocoaView respondsToSelector:@selector(setFont:)]) {
            NSFont* nsfont = pFont::cocoaFont(font);

            if (nsfont != nil)
                [(id)cocoaView setFont:nsfont];
        }
    }
    calculatedMinimumSize.updated = false;
}

inline auto pWidget::getMinimumSize() -> Size {
    if (calculatedMinimumSize.updated)
        return calculatedMinimumSize.minimumSize;        
    
    calculatedMinimumSize.minimumSize = pFont::size([(id)cocoaView font], widget.text());

    calculatedMinimumSize.updated = true;
    
    return calculatedMinimumSize.minimumSize;
}

auto pWidget::setEnabled(bool enabled) -> void {
    @autoreleasepool {
        if([cocoaView respondsToSelector:@selector(setEnabled:)]) {
            [(id)cocoaView setEnabled:enabled];
        }
    }
}

auto pWidget::setEnabledThreaded(bool enabled) -> void {
    @autoreleasepool {
        bool _enabled = enabled;
        dispatch_group_t group = dispatch_group_create();
        dispatch_group_async(group, dispatch_get_main_queue(), ^{
            if([cocoaView respondsToSelector:@selector(setEnabled:)]) {
                [(id)cocoaView setEnabled:_enabled];
            }
        });
    }
}
    
auto pWidget::setVisible(bool visible) -> void {
    @autoreleasepool {
        if(cocoaView) [cocoaView setHidden:!visible];
    }
}

auto pWidget::setGeometry(Geometry geometry) -> void {
    if(!cocoaView) return;
    
    @autoreleasepool {
        CGFloat windowHeight = [[cocoaView superview] frame].size.height;
        [cocoaView setFrame:NSMakeRect(geometry.x, windowHeight - geometry.y - geometry.height, geometry.width, geometry.height)];
        [[cocoaView superview] setNeedsDisplay:YES];
    }
    if(widget.onSize) widget.onSize();
}

auto pWidget::setTooltip(std::string tooltip) -> void {
    @autoreleasepool {
        if(cocoaView) [cocoaView setToolTip:[NSString stringWithUTF8String:tooltip.c_str()]];
    }
}

auto pWidget::add() -> void {
    setTooltip( widget.tooltip() );
    setFont( widget.font() );
}
   
}
