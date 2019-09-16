
@implementation CocoaRadioBox : NSButton

-(id) initWith:(GUIKIT::RadioBox&)radioBoxReference {
    if(self = [super initWithFrame:NSMakeRect(0, 0, 0, 0)]) {
        radioBox = &radioBoxReference;
        
        [self setTarget:self];
        [self setAction:@selector(activate:)];
        [self setButtonType:NSRadioButton];
    }
    return self;
}

-(IBAction) activate:(id)sender {
    radioBox->setChecked();
    if(radioBox->onActivate) radioBox->onActivate();
}
@end

namespace GUIKIT {
        
auto pRadioBox::minimumSize() -> Size {
    Size size = pFont::size([inner font], widget.text());
    return {size.width + 22, size.height + 1};
}
    
auto pRadioBox::setGeometry(Geometry geometry) -> void {
    pWidget::setGeometry({
        geometry.x - 1, geometry.y + 1,
        geometry.width + 2, geometry.height
    });
    
    @autoreleasepool {
        [inner setFrame:NSMakeRect(0, 0, geometry.width, geometry.height)];
        [[inner superview] setNeedsDisplay:YES];
    }
}

auto pRadioBox::setChecked() -> void {
    @autoreleasepool {
        for(auto& item : radioBox.state.group) {
            auto state = (item == &radioBox) ? NSOnState : NSOffState;
            [item->p.inner setState:state];
        }
    }
}
    
auto pRadioBox::setText(std::string text) -> void {
    @autoreleasepool {
        [inner setTitle:[NSString stringWithUTF8String:text.c_str()]];
    }
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
		// we control which elements belongs to the same group, not cocoa
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
			[inner setFont:pFont::cocoaFont(font)];
		}
	}
}

auto pRadioBox::setEnabled(bool enabled) -> void {
	@autoreleasepool {
		if([inner respondsToSelector:@selector(setEnabled:)]) {
			[inner setEnabled:enabled];
		}
	}
}

auto pRadioBox::setTooltip(std::string tooltip) -> void {
	@autoreleasepool {
		if(cocoaView) [inner setToolTip:[NSString stringWithUTF8String:tooltip.c_str()]];
	}
}

}        
