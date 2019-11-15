
@implementation CocoaFrame : NSBox

-(id) initWith {
    if(self = [super initWithFrame:NSMakeRect(0, 0, 0, 0)]) {
        [self setTitle:@""];
    }
    return self;
}
@end

namespace GUIKIT {
    
auto pFrame::minimumSize() -> Size {
    Size size = pFont::size([cocoaView titleFont], widget.text());
    size.width += 4 + (borderSize() << 1);
    if (widget.text().empty()) size.height = 0;
    
    size.height += borderSize() << 1;
    return size;
}

auto pFrame::setGeometry(Geometry geometry) -> void {
    Size size = pFont::size([cocoaView titleFont], widget.text());
    bool empty = widget.text().empty();
  
    pWidget::setGeometry({
        int(geometry.x - 3), int(geometry.y - (empty ? size.height - 0 : 0)),
        geometry.width + 6, geometry.height + (empty ? size.height + 4 : 4)
    });
}

auto pFrame::setText(std::string text) -> void {
    @autoreleasepool {
        [cocoaView setTitle:[NSString stringWithUTF8String:text.c_str()]];
    }
}

auto pFrame::setFont(std::string font) -> void {
    @autoreleasepool {
        [cocoaView setTitleFont:pFont::cocoaFont(font)];
    }
}

auto pFrame::init() -> void {
    @autoreleasepool {
        cocoaView = [[CocoaFrame alloc] initWith];
    }
} 
    
}        