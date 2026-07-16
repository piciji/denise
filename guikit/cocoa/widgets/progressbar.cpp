
@implementation CocoaProgressBar : NSProgressIndicator

-(id) initWith:(GUIKIT::ProgressBar&)progressBarReference {
    if(self = [super initWithFrame:NSMakeRect(0, 0, 0, 0)]) {
        progressBar = &progressBarReference;
        
        [self setIndeterminate:NO];
        [self setMinValue:0.0];
        [self setMaxValue:100.0];
    }
    return self;
}
@end

namespace GUIKIT {
    
auto pProgressBar::minimumSize() -> Size {
    auto _size = getMinimumSize();
    return {0, _size.height};
}

auto pProgressBar::init() -> void {
    @autoreleasepool {
        cocoaView = [[CocoaProgressBar alloc] initWith:progressBar];
        setPosition(progressBar.position());
    }
}

auto pProgressBar::setPositionThreaded(unsigned position) -> void {
    @autoreleasepool {
        unsigned _position = position;
        dispatch_group_t group = dispatch_group_create();
        dispatch_group_async(group, dispatch_get_main_queue(), ^{
            [NSAnimationContext beginGrouping];
            [[NSAnimationContext currentContext] setDuration:0.0];
            [(id)cocoaView setDoubleValue : (double)_position ];
            [cocoaView setNeedsDisplay:YES];
            [cocoaView displayIfNeeded];
            [NSAnimationContext endGrouping];
        });
    }
}

auto pProgressBar::setPosition(unsigned position) -> void {
    @autoreleasepool {
        [NSAnimationContext beginGrouping];
        [[NSAnimationContext currentContext] setDuration:0.0];
        [(id)cocoaView setDoubleValue : (double)position ];
        [cocoaView displayIfNeeded];
        [NSAnimationContext endGrouping];
    }
}
    
}        
