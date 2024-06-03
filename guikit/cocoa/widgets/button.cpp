
@implementation CocoaButton : NSButton

-(id) initWith:(GUIKIT::Button&)buttonReference {
    if(self = [super initWithFrame:NSMakeRect(0, 0, 0, 0)]) {
        button = &buttonReference;
        [self setTarget:self];
        [self setAction:@selector(activate:)];
        if (!GUIKIT::hasMinimumVersion(14,0))
            [self setBezelStyle:NSRegularSquareBezelStyle];
    }
    return self;
}

-(IBAction) activate:(id)sender {
    if(button->onActivate) button->onActivate();
}
@end

namespace GUIKIT {
    
auto pButton::minimumSize() -> Size {
    Size size = getMinimumSize();
    return {size.width + 22, size.height + 6};
}
    
auto pButton::setGeometry(Geometry geometry) -> void {
    pWidget::setGeometry({
        geometry.x - 2, geometry.y - 2,
        geometry.width + 4, geometry.height + 4
    });
}
    
auto pButton::setText(const std::string& text) -> void {
    @autoreleasepool {
        [(id)cocoaView setTitle:[NSString stringWithUTF8String:text.c_str()]];
    }
    calculatedMinimumSize.updated = false;
}

auto pButton::init() -> void {
    @autoreleasepool {
        cocoaView = [[CocoaButton alloc] initWith:button];
    }
}
    
}        
