
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
    
    if (calculatedMinimumSize.updated)
        return calculatedMinimumSize.minimumSize; 
    
    Size size = pFont::size([(id)cocoaView titleFont], widget.text());
    size.width += 4 + (borderSize() << 1);
    if (widget.text().empty()) size.height = 0;
    
    size.height += borderSize() << 1;    
    
    calculatedMinimumSize.updated = true;   
    calculatedMinimumSize.minimumSize = size;
    
    return calculatedMinimumSize.minimumSize;
}

auto pFrame::setGeometry(Geometry geometry) -> void {
    Size size;
    bool empty = widget.text().empty();
    if (empty) {
        size = pFont::size([(id)cocoaView titleFont], widget.text());
    }
  
    pWidget::setGeometry({
        int(geometry.x), int(geometry.y - (empty ? size.height : borderSize())),
        geometry.width, geometry.height + (empty ? size.height : (borderSize() << 1))
    });
}

auto pFrame::setText(const std::string& text) -> void {
    @autoreleasepool {
        [(id)cocoaView setTitle:[NSString stringWithUTF8String:text.c_str()]];
    }
    calculatedMinimumSize.updated = false;
}

auto pFrame::setFont(std::string font) -> void {
    @autoreleasepool {
        [(id)cocoaView setTitleFont:pFont::cocoaFont(font)];
    }
    calculatedMinimumSize.updated = false;
}

auto pFrame::init() -> void {
    @autoreleasepool {
        cocoaView = [[CocoaFrame alloc] initWith];
    }
} 
    
}        
