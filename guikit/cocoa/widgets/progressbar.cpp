
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
    return {0, 10};
}

auto pProgressBar::init() -> void {
    @autoreleasepool {
        cocoaView = [[CocoaProgressBar alloc] initWith:progressBar];
        setPosition(progressBar.position());
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
