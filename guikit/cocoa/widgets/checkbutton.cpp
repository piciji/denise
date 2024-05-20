
@implementation CocoaCheckButton : NSButton

-(id) initWith:(GUIKIT::CheckButton&)checkButtonReference {
    if(self = [super initWithFrame:NSMakeRect(0, 0, 0, 0)]) {
        checkButton = &checkButtonReference;
        [self setTarget:self];
        [self setAction:@selector(activate:)];
        [self setBezelStyle:NSRegularSquareBezelStyle];
        [self setButtonType:NSOnOffButton];
    }
    return self;
}

-(IBAction) activate:(id)sender {
    checkButton->state.checked = [self state] != NSOffState;
    if(checkButton->onToggle) checkButton->onToggle();
}
@end

namespace GUIKIT {
    
auto pCheckButton::minimumSize() -> Size {
    Size size = getMinimumSize();
    return {size.width + 22, size.height + 6};
}

auto pCheckButton::setGeometry(Geometry geometry) -> void {
    pWidget::setGeometry({
        geometry.x - 2, geometry.y - 2,
        geometry.width + 4, geometry.height + 4
    });
}

auto pCheckButton::setText(const std::string& text) -> void {
    @autoreleasepool {
        [(id)cocoaView setTitle:[NSString stringWithUTF8String:text.c_str()]];
    }
    calculatedMinimumSize.updated = false;
}

auto pCheckButton::setChecked(bool checked) -> void {
    @autoreleasepool {
        [(id)cocoaView setState:checked ? NSOnState : NSOffState];
    }
}
    
auto pCheckButton::init() -> void {
    @autoreleasepool {
        cocoaView = [[CocoaCheckButton alloc] initWith:checkButton];
        setChecked(checkButton.checked());
    }
}    
    
}
