
@implementation CocoaStepButton : NSStepper

-(id) initWith:(GUIKIT::StepButton&)stepButtonReference {
    if(self = [super initWithFrame:NSMakeRect(0, 0, 0, 0)]) {
        stepButton = &stepButtonReference;
        
        [self setIncrement: 1];
        [self setValueWraps: YES];
        [self setAction:@selector(activate:)];
        [self setTarget:self];
    }
    return self;
}

-(IBAction) activate:(id)sender {
    stepButton->state.value = [sender integerValue];
    stepButton->Widget::state.text = std::to_string(stepButton->state.value);
    
    [stepButton->p.editView setStringValue: [NSString stringWithUTF8String : stepButton->Widget::state.text.c_str()] ];
    
    if(stepButton->onChange) stepButton->onChange();
}

@end


@implementation CocoaStepEdit : NSTextField

-(id) initWith:(GUIKIT::StepButton&)stepButtonReference {
    if(self = [super initWithFrame:NSMakeRect(0, 0, 0, 0)]) {
        stepButton = &stepButtonReference;
        
        [self setDelegate:self];
        [self setTarget:self];
    }
    return self;
}

-(void) controlTextDidChange:(NSNotification*)notification {
    NSTextField* textField = [notification object];
    NSString* strValue = [textField stringValue];
    
    std::string str = std::string([strValue UTF8String]);
    stepButton->Widget::state.text = str;
    
    int value;
    
    if (str.empty())
        value = 0;
    else {
        try {
            value = std::stoi( str );
        } catch(...) {
            return;
        }
    }
    stepButton->state.value = value;
    
    [stepButton->p.stepView setIntegerValue: value];
    
    if(stepButton->onChange) stepButton->onChange();
}


@end


namespace GUIKIT {
    
auto pStepButton::minimumSize() -> Size {
    Size size;
    
    if (calculatedMinimumSize.updated)
        size = calculatedMinimumSize.minimumSize;
    else {
        calculatedMinimumSize.minimumSize = pFont::size([editView font], widget.text());
    
        calculatedMinimumSize.updated = true;
    }
    
    return {size.width + 10 + 15, size.height + 6};
}

auto pStepButton::setFont(std::string font) -> void {
    @autoreleasepool {
        if([editView respondsToSelector:@selector(setFont:)]) {
            [editView setFont:pFont::cocoaFont(font)];
        }
    }
    calculatedMinimumSize.updated = false;
}

auto pStepButton::setValue( int16_t value ) -> void {
    @autoreleasepool {
        [stepView setIntegerValue: value];
        [editView setStringValue : [NSString stringWithUTF8String : stepButton.Widget::state.text.c_str()]];
    }
    calculatedMinimumSize.updated = false;
}

auto pStepButton::updateRange() -> void {
    @autoreleasepool {
        [stepView setMinValue: stepButton.state.minValue];
        [stepView setMaxValue: stepButton.state.maxValue];
    }
}
    
auto pStepButton::setGeometry(Geometry geometry) -> void {
    @autoreleasepool {
        [editView setFrame:NSMakeRect(0, 0, geometry.width - 15, geometry.height)];
        
        [stepView setFrame:NSMakeRect(geometry.width - 15, 0, 15, geometry.height)];
    }
    pWidget::setGeometry( geometry );
}

auto pStepButton::init() -> void {
    @autoreleasepool {
        cocoaView = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 0, 0)];

        stepView = [[CocoaStepButton alloc] initWith : stepButton];
        editView = [[CocoaStepEdit alloc] initWith : stepButton];
        
        formatter = [[IntegerFormatter alloc] initWith: false];
        [editView setFormatter: formatter];
        
        [cocoaView addSubview: editView];
        [cocoaView addSubview: stepView];
    }
}
 
pStepButton::~pStepButton() {
    @autoreleasepool {
        [stepView release];
        [editView release];
        [formatter release];
    }

}
    
}
