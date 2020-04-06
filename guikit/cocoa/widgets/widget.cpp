
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
            [cocoaView setFont:pFont::cocoaFont(font)];
        }
    }
    calculatedMinimumSize.updated = false;
}

inline auto pWidget::getMinimumSize() -> Size {
    if (calculatedMinimumSize.updated)
        return calculatedMinimumSize.minimumSize;        
    
    calculatedMinimumSize.minimumSize = pFont::size([cocoaView font], widget.text());

    calculatedMinimumSize.updated = true;
    
    return calculatedMinimumSize.minimumSize;
}

auto pWidget::setEnabled(bool enabled) -> void {
    @autoreleasepool {
        if([cocoaView respondsToSelector:@selector(setEnabled:)]) {
            [cocoaView setEnabled:enabled];
        }
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
