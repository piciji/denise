
@implementation CocoaCheckBox : NSButton

-(id) initWith:(GUIKIT::CheckBox&)checkBoxReference {
    if(self = [super initWithFrame:NSMakeRect(0, 0, 0, 0)]) {
        checkBox = &checkBoxReference;
        
        [self setTarget:self];
        [self setAction:@selector(activate:)];
        [self setButtonType:NSButtonTypeSwitch];
    }
    return self;
}

-(IBAction) activate:(id)sender {
    if (checkBox->readonly()) {
        [(id)self setState:checkBox->state.checked ? NSControlStateValueOn : NSControlStateValueOff];
        return;
    }
    checkBox->state.checked = [self state] != NSControlStateValueOff;
    if(checkBox->onToggle) checkBox->onToggle( checkBox->state.checked );
}
@end

namespace GUIKIT {
    
auto pCheckBox::minimumSize() -> Size {
    Size size = getMinimumSize();
    return {size.width + 20, size.height};
}
    
auto pCheckBox::setGeometry(Geometry geometry) -> void {
    pWidget::setGeometry({
        geometry.x - 2, geometry.y,
        geometry.width + 4, geometry.height
    });
}

auto pCheckBox::setChecked(bool checked) -> void {
    @autoreleasepool {
        [(id)cocoaView setState:checked ? NSControlStateValueOn : NSControlStateValueOff];
    }
}

auto pCheckBox::setText(const std::string& text) -> void {
    @autoreleasepool {
        if (checkBox.overrideForegroundColor())
            setAttributedText();
        else
            [(id)cocoaView setTitle:[NSString stringWithUTF8String:text.c_str()]];
    }
    calculatedMinimumSize.updated = false;
}

auto pCheckBox::setForegroundColor(unsigned color) -> void {
    setAttributedText();
}

auto pCheckBox::init() -> void {
    @autoreleasepool {
        cocoaView = [[CocoaCheckBox alloc] initWith:checkBox];
        setChecked(checkBox.checked());
    }
}

    
}        
