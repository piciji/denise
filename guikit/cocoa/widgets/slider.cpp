
@implementation CocoaSliderCell : NSSliderCell

-(id) initWith:(GUIKIT::Slider&)sliderReference {
    if(self = [super init]) {
        slider = &sliderReference;
        
        [self setTarget:self];
        [self setAction:@selector(activate:)];
        [self setMinValue:0];
    
    }
    return self;
}

-(void)stopTracking:(NSPoint)lastPoint at:(NSPoint)stopPoint inView:(NSView *)controlView mouseIsUp:(BOOL)flag {
    
    if (flag == YES) {
        using GUIKIT::pApplication;
        pApplication::setAppTimer();
        
        [[NSRunLoop currentRunLoop] addTimer:pApplication::appTimer forMode:NSDefaultRunLoopMode];

        slider->state.position = [self doubleValue];
        if(slider->onChange) slider->onChange();
    }
    
    [super stopTracking:lastPoint at:stopPoint inView:controlView mouseIsUp:flag];
}

-(void)startTrackingAt:(NSPoint)startPoint inView:(NSView*)controlView {
    
    using GUIKIT::pApplication;
    pApplication::setAppTimer();
    
    [[NSRunLoop currentRunLoop] addTimer:pApplication::appTimer forMode:NSRunLoopCommonModes];
    
    [super startTrackingAt:startPoint inView:controlView];
}

-(IBAction) activate:(id)sender {
    slider->state.position = [self doubleValue];
    if(slider->onChange) slider->onChange();
}
@end

@implementation CocoaVerticalSlider : NSSlider

-(id) initWith:(GUIKIT::Slider&)sliderReference {
    if(self = [super initWithFrame:NSMakeRect(0, 0, 0, 1)]) {
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
    slider->state.position = [self doubleValue];
    if(slider->onChange) slider->onChange();
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
    slider->state.position = [self doubleValue];
    if(slider->onChange) slider->onChange();
}
@end

namespace GUIKIT {
    
auto pSlider::minimumSize() -> Size {
    unsigned thickness = (unsigned)[cocoaView knobThickness];
    if (!GUIKIT::hasMinimumVersion(10, 11))
        thickness += 2; //wtf
    
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
        [cocoaView setMaxValue:length - 1];
    }
}

auto pSlider::setPosition(unsigned position) -> void {
    @autoreleasepool {
        [cocoaView setDoubleValue:position];
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
