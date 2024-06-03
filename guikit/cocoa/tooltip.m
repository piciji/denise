
@implementation TooltipWindow

+(TooltipWindow*) getInstance {
    static TooltipWindow* singleton = nil;

    if (singleton == nil) {
        singleton = [[TooltipWindow alloc] initWithContentRect:NSMakeRect(0, 0, 0, 0) styleMask:NSBorderlessWindowMask backing:NSBackingStoreBuffered defer:NO];
    }
    return singleton;
}

-(id) initWithContentRect:(NSRect)contentRect styleMask:(unsigned long)styleMask backing:(NSBackingStoreType)backing defer:(BOOL)defer {

    [super initWithContentRect:contentRect styleMask:styleMask backing:backing defer:defer];

   // [self setColorSpace: [NSColorSpace sRGBColorSpace]];
    [self setLevel:NSPopUpMenuWindowLevel];
    [self setHasShadow:YES];
    
    textField = [[NSTextField alloc] initWithFrame:contentRect];
    [textField setEditable:NO];
    [textField setBordered:NO];
    [textField setBezeled:NO];
    [textField setDrawsBackground:NO];

    [[self contentView] addSubview:textField];
    [self setBackgroundColor:[NSColor colorWithSRGBRed:1. green:1. blue:.88 alpha:1.]];

    timer.onFinished = [self]() {

        if([self isVisible] == YES) {
            timer.setEnabled(false);
            //[super orderOut:nil];
            [self fadeWindowOut];

        } else {
           // [super orderFront:nil];
            [self fadeWindowIn];

            timer.setEnabled();
            timer.setInterval(3000);
        }
    };

    needUpdate = YES;
    dismissLock = NO;
    
    return self;
}

-(NSString*) tooltip {
    return [textField stringValue];
}

-(void) setTooltip:(NSString *)toolTip {
    [textField setStringValue:toolTip];
    needUpdate = YES;
}

-(void)orderOut:sender {
    timer.setEnabled(false);
   // [super orderOut:sender];
    if (dismissLock == YES)
        return;
    dismissLock = YES;
    [self fadeWindowOut];
}

-(void)orderFront:sender {
    dismissLock = NO;
    if (needUpdate == YES) {
        NSSize messageSize = NSZeroSize;
        NSRect windowFrame = [self frame];

        messageSize = [[NSScreen mainScreen] visibleFrame].size;

        NSDictionary* fontAttributes = [NSDictionary dictionaryWithObjectsAndKeys:[textField font], NSFontAttributeName, nil];
        NSSize tipSize = [[textField stringValue] sizeWithAttributes:fontAttributes];

        tipSize.width += 4;
        tipSize.height += 2;

        [textField setFrame:NSMakeRect(3, 3, tipSize.width, tipSize.height)];

        tipSize.width += 6;
        tipSize.height += 6;
        
        windowFrame.origin = [NSEvent mouseLocation];
        windowFrame.origin.x += 10;
        windowFrame.origin.y += 10;
        windowFrame.size = tipSize;
        [self setFrame:windowFrame display:YES];

        needUpdate = NO;
    }

    bool _visible = [self isVisible] == YES;

    if (!_visible || ([super alphaValue] != 1.0) )
        timer.setInterval(700);
    else
        timer.setInterval(3000);

    timer.setEnabled();
}

- (void)fadeWindowIn {
    bool _visible = [self isVisible] == YES;

    if (!_visible) {
        [super setAlphaValue: 0.0f];
        [super makeKeyAndOrderFront: nil];
    }

    [NSAnimationContext runAnimationGroup: ^(NSAnimationContext* context) {
        [context setDuration: 0.5f];
        [[super animator] setAlphaValue: 1.0f];
    } completionHandler: nil];
}

- (void)fadeWindowOut {
    bool _visible = [self isVisible] == YES;

    if (!_visible)
        return;
    
    [NSAnimationContext runAnimationGroup: ^(NSAnimationContext* context) {
        [context setDuration: 0.5f];
        [[super animator] setAlphaValue: 0.0f];
    } completionHandler: ^ {
        [super orderOut: nil];
    } ];
}

-(void) setFont:(NSFont*)fontPointer {
    [textField setFont: fontPointer];
}

-(void) setBackgroundColor:(NSColor*)_color {
    NSView* _view = [self contentView];
    [_view setWantsLayer:YES];
    [_view.layer setBackgroundColor: _color.CGColor];
}

-(void) setTextColor:(NSColor*)_color {
    [textField setTextColor: _color ];
}

@end
