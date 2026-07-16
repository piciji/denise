
@implementation CocoaCheckButton : NSButton

-(id) initWith:(GUIKIT::CheckButton&)checkButtonReference {
    if(self = [super initWithFrame:NSMakeRect(0, 0, 0, 0)]) {
        checkButton = &checkButtonReference;
        [self setTarget:self];
        [self setAction:@selector(activate:)];
        #define _NSBezelStyleFlexiblePush 2
        [self setBezelStyle:(NSBezelStyle)_NSBezelStyleFlexiblePush];
        [self setButtonType:NSButtonTypeOnOff];
    }
    return self;
}

-(IBAction) activate:(id)sender {
    checkButton->state.checked = [self state] != NSControlStateValueOff;
    if(checkButton->onToggle) checkButton->onToggle();
}
@end

namespace GUIKIT {
    
auto pCheckButton::minimumSize() -> Size {
    return getMinimumSize();
}

auto pCheckButton::setText(const std::string& text) -> void {
    @autoreleasepool {
        [(id)cocoaView setTitle:[NSString stringWithUTF8String:text.c_str()]];
    }
    calculatedMinimumSize.updated = false;
}

auto pCheckButton::setChecked(bool checked) -> void {
    @autoreleasepool {
        [(id)cocoaView setState:checked ? NSControlStateValueOn : NSControlStateValueOff];
    }
}
    
auto pCheckButton::init() -> void {
    @autoreleasepool {
        cocoaView = [[CocoaCheckButton alloc] initWith:checkButton];
        setChecked(checkButton.checked());
    }
}    
    
}
