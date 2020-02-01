
@implementation CocoaLabel : NSTextField

-(id) initWith:(GUIKIT::Label&)labelReference {
    if(self = [super initWithFrame:NSMakeRect(0, 0, 0, 0)]) {
        label = &labelReference;
        
        [self setAlignment:NSLeftTextAlignment];
        [self setBordered:NO];
        [self setDrawsBackground:NO];
        [self setEditable:NO];
    }
    return self;
}
@end

namespace GUIKIT {
    
auto pLabel::minimumSize() -> Size {
    Size size = pFont::size([cocoaView font], widget.text());
    return {size.width + 1, size.height + 4};
}

auto pLabel::setAlign( Label::Align align ) -> void {
    if (align == Label::Align::Left)
        [cocoaView setAlignment:NSLeftTextAlignment];
    else
        [cocoaView setAlignment:NSRightTextAlignment];
}
    
auto pLabel::setGeometry(Geometry geometry) -> void {
    unsigned height = pFont::size([cocoaView font], " ").height;
    unsigned widgetHeight = geometry.height + 4;
    auto offset = geometry;
    
    if(widgetHeight > height) {
       unsigned diff = widgetHeight - height;
       offset.y += diff >> 1;
       offset.height -= diff >> 1;
    }
    
    pWidget::setGeometry({
        offset.x - 2, offset.y - 2,
        offset.width + 4, offset.height + 4
    });
}
    
auto pLabel::setText(std::string text) -> void {
    @autoreleasepool {
        [cocoaView setStringValue:[NSString stringWithUTF8String:text.c_str()]];
    }
}
    
auto pLabel::setEnabled(bool enabled) -> void {
    
    NSColor* textColor = [NSColor textColor];
    
    if(label.overrideForegroundColor()) {
        unsigned color = label.foregroundColor();
        textColor = [NSColor
                     colorWithSRGBRed:((color>>16) & 0xff) / 255.0
                     green:((color>>8) & 0xff) / 255.0
                     blue:(color & 0xff) / 255.0
                     alpha: 1.0];
    }
    
    [cocoaView setTextColor: enabled ? textColor : [NSColor grayColor]];
    pWidget::setEnabled(enabled);
}
    
auto pLabel::setForegroundColor(unsigned color) -> void {
    setEnabled( label.enabled() );
}


auto pLabel::init() -> void {
    cocoaView = [[CocoaLabel alloc] initWith:label];
}   
    
}       
