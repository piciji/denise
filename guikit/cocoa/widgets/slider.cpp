
@implementation CocoaSliderCell : NSSliderCell

-(id) initWith:(GUIKIT::Slider&)sliderReference {
    if(self = [super init]) {
        slider = &sliderReference;
        
        [self setTarget:self];
        [self setAction:@selector(activate:)];
        [self setMinValue:0];
        
        if (GUIKIT::hasMinimumVersion(11, 0))
            [self setControlSize:NSControlSizeSmall];
    
    }
    return self;
}

-(void)stopTracking:(NSPoint)lastPoint at:(NSPoint)stopPoint inView:(NSView *)controlView mouseIsUp:(BOOL)flag {
    
    if (flag == YES) {
        using GUIKIT::pApplication;
        pApplication::setAppTimer();
        
        [[NSRunLoop currentRunLoop] addTimer:pApplication::appTimer forMode:NSDefaultRunLoopMode];

        unsigned pos = [self doubleValue];
        slider->state.position = pos;
        if(slider->onChange) slider->onChange(pos);
    }
    
    [super stopTracking:lastPoint at:stopPoint inView:controlView mouseIsUp:flag];
}

-(BOOL)startTrackingAt:(NSPoint)startPoint inView:(NSView*)controlView {
    
    using GUIKIT::pApplication;
    pApplication::setAppTimer();
    
    [[NSRunLoop currentRunLoop] addTimer:pApplication::appTimer forMode:NSRunLoopCommonModes];
    
    return [super startTrackingAt:startPoint inView:controlView];
}

-(IBAction) activate:(id)sender {
    unsigned pos = [self doubleValue];
    slider->state.position = pos;
    if(slider->onChange) slider->onChange(pos);
}
@end

@implementation CocoaVerticalSlider : NSSlider

-(id) initWith:(GUIKIT::Slider&)sliderReference {
    if(self = [super initWithFrame:NSMakeRect(0, 0, 0, 1)]) {
        slider = &sliderReference;
        
        [self setTarget:self];
        [self setAction:@selector(activate:)];
        [self setMinValue:0];
        [self setVertical:1];
        
        CocoaSliderCell* notifySliderCell = [[[CocoaSliderCell alloc] initWith: *slider] autorelease];
        [self setCell:notifySliderCell];
    }
    return self;
}

-(IBAction) activate:(id)sender {
    unsigned pos = [self doubleValue];
    slider->state.position = pos;
    if(slider->onChange) slider->onChange(pos);
}
@end

@implementation CocoaHorizontalSlider : NSSlider

-(id) initWith:(GUIKIT::Slider&)sliderReference {
    if(self = [super initWithFrame:NSMakeRect(0, 0, 1, 0)]) {
        slider = &sliderReference;
        
        [self setTarget:self];
        [self setAction:@selector(activate:)];
        [self setMinValue:0];
        
        CocoaSliderCell* notifySliderCell = [[[CocoaSliderCell alloc] initWith: *slider] autorelease];
        [self setCell:notifySliderCell];
    }
    return self;
}

-(IBAction) activate:(id)sender {
    unsigned pos = [self doubleValue];
    slider->state.position = pos;
    if(slider->onChange) slider->onChange(pos);
}
@end

namespace GUIKIT {
    
auto pSlider::minimumSize() -> Size {
    unsigned thickness = 18;
    if (GUIKIT::hasMinimumVersion(10, 10)) {
        // don't access knob thickness in Mavericks or slider will be always vertical... wtf
        thickness = (unsigned)[(id)cocoaView knobThickness];
    }
    
    if (slider.orientation == Slider::Orientation::VERTICAL)
        return {thickness, thickness};
        
    return {thickness, thickness};
}
    
auto pSlider::setGeometry(Geometry geometry) -> void {
    if (slider.orientation == Slider::Orientation::VERTICAL) {
        pWidget::setGeometry({
            geometry.x, geometry.y - 2,
            geometry.width, geometry.height + 4
        });
    } else {
        pWidget::setGeometry({
            geometry.x - 2, geometry.y,
            geometry.width + 4, geometry.height
        });
    }
}

auto pSlider::setLength(unsigned length) -> void {
    @autoreleasepool {
        [(id)cocoaView setMaxValue:length - 1];
    }
}

auto pSlider::setPosition(unsigned position) -> void {
    @autoreleasepool {
        [(id)cocoaView setDoubleValue:position];
    }
}

auto pSlider::init() -> void {
    @autoreleasepool {
        if (slider.orientation == Slider::Orientation::VERTICAL) {
            cocoaView = [[CocoaVerticalSlider alloc] initWith:slider];
        } else {
            cocoaView = [[CocoaHorizontalSlider alloc] initWith:slider];
        }
        
        setLength(slider.length());
        setPosition(slider.position());
    }
}
    
}        
