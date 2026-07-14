
@implementation CocoaRadioBox : NSButton

-(id) initWith:(GUIKIT::RadioBox&)radioBoxReference {
    if(self = [super initWithFrame:NSMakeRect(0, 0, 0, 0)]) {
        radioBox = &radioBoxReference;
        
        [self setTarget:self];
        [self setAction:@selector(activate:)];
        [self setButtonType:NSButtonTypeRadio];
    }
    return self;
}

-(IBAction) activate:(id)sender {
    if (radioBox->readonly()) {
        for(auto& item : radioBox->state.group) {
            if (item->checked()) {
                item->setChecked();
                break;
            }
        }
        return;
    }
    radioBox->setChecked();
    if(radioBox->onActivate) radioBox->onActivate();
}
@end

namespace GUIKIT {
        
auto pRadioBox::minimumSize() -> Size {
    if (calculatedMinimumSize.updated)
        return calculatedMinimumSize.minimumSize; 
        
    NSSize _size = [inner fittingSize];
    
    calculatedMinimumSize.updated = true;   
    calculatedMinimumSize.minimumSize = {(unsigned)_size.width, (unsigned)_size.height};
    
    return calculatedMinimumSize.minimumSize;
}
    
auto pRadioBox::setGeometry(Geometry geometry) -> void {
    pWidget::setGeometry({
        geometry.x, geometry.y,
        geometry.width, geometry.height
    });
    
    @autoreleasepool {
        [inner setFrame:NSMakeRect(0, 0, geometry.width, geometry.height)];
        [[inner superview] setNeedsDisplay:YES];
    }
}

auto pRadioBox::setChecked() -> void {
    @autoreleasepool {
        for(auto& item : radioBox.state.group) {
            auto state = (item == &radioBox) ? NSControlStateValueOn : NSControlStateValueOff;
            [(id)item->p.inner setState:state];
        }
    }
}

auto pRadioBox::setForegroundColor(unsigned color) -> void {
    setAttributedText();
}
    
auto pRadioBox::setText(const std::string& text) -> void {
    @autoreleasepool {
        if (radioBox.overrideForegroundColor())
            setAttributedText();
        else
            [(id)inner setTitle:[NSString stringWithUTF8String:text.c_str()]];
    }
    calculatedMinimumSize.updated = false;
}

auto pRadioBox::setAttributedText() -> void {
    NSMutableAttributedString* attrTitle =
        [[NSMutableAttributedString alloc] initWithString:[NSString stringWithUTF8String:widget.text().c_str()]];
    
    NSUInteger len = [attrTitle length];
    NSRange range = NSMakeRange(0, len);
    [attrTitle addAttribute:NSForegroundColorAttributeName value:getTextColor() range:range];
    [attrTitle fixAttributesInRange:range];
    [(id)inner setAttributedTitle:attrTitle];
}

auto pRadioBox::init() -> void {
    @autoreleasepool {
		// damn, cocoa expects that all radios of a group belongs to a separate view
		// our concept means to add all widgets to the same view and control the geometry ourselves
		// but that would result all radios are in the same group and cocoa makes sure only one
		// radio is active at a time
		// doing it right means an additional complexity for layouting
		// we do some dirty trick, so don't tell it anyone, because I am ashamed
		// we put each radio in a box, which gets the same geometry like the radio itself
		// from a cocoa point of view each radio group consists of one element
		// we control which elements belong to the same group, not cocoa
        cocoaView = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 0, 0)];

        inner = [[CocoaRadioBox alloc] initWith:radioBox];
        
        [cocoaView addSubview: inner positioned:NSWindowAbove relativeTo:nil];
    }
}
    
auto pRadioBox::focused() -> bool {
	@autoreleasepool {
		return inner && inner == [[inner window] firstResponder];
	}
}

auto pRadioBox::setFocused() -> void {
	@autoreleasepool {
		if(cocoaView) [[inner window] makeFirstResponder:inner];
	}
}

auto pRadioBox::setFont(std::string font) -> void {
	@autoreleasepool {
		if([inner respondsToSelector:@selector(setFont:)]) {
			[(id)inner setFont:pFont::cocoaFont(font)];
		}
	}
    calculatedMinimumSize.updated = false;
}

auto pRadioBox::setEnabled(bool enabled) -> void {
	@autoreleasepool {
		if([inner respondsToSelector:@selector(setEnabled:)]) {
			[(id)inner setEnabled:enabled];
		}
	}
}

auto pRadioBox::setTooltip(std::string tooltip) -> void {
	@autoreleasepool {
		if(cocoaView) [inner setToolTip:[NSString stringWithUTF8String:tooltip.c_str()]];
	}
}

}        
