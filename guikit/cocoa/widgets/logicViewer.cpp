
@implementation CocoaLogicViewer : NSView

-(id) initWith:(GUIKIT::LogicViewer&)logicViewerReference {
    if(self = [super initWithFrame:NSMakeRect(0, 0, 0, 0)]) {
        logicViewer = &logicViewerReference;
    }
    return self;
}

- (BOOL)isFlipped
{
    return YES;
}

- (void)drawRect:(NSRect)dirtyRect {
    [super drawRect:dirtyRect];
    logicViewer->p.redraw();
}

@end

@implementation CocoaLogicViewerScroll : NSScrollView

-(id) initWith:(GUIKIT::LogicViewer&)logicViewerReference {
    if(self = [super initWithFrame:NSMakeRect(0, 0, 0, 0)]) {
        logicViewer = &logicViewerReference;
        
        content = [[CocoaLogicViewer alloc] initWith:logicViewerReference];
        
     //   [content setAutoresizingMask:NSViewWidthSizable];
        
        [self setDocumentView:content];
        [self setBorderType:NSBezelBorder];
        [self setHasVerticalScroller:NO];
        [self setHasHorizontalScroller:YES];
        if (GUIKIT::hasMinimumVersion(10, 10)) {
        //    [self setAutomaticallyAdjustsContentInsets:NO];
          //  [self setContentInsets:NSEdgeInsetsMake(0, 0, 0, 0)];
        }
        
        [[self contentView] setPostsBoundsChangedNotifications:YES];
        [[NSNotificationCenter defaultCenter] addObserver:self
            selector:@selector(boundsDidChange:)
            name:NSViewBoundsDidChangeNotification object:[self contentView]];

      //  [content setTarget:self];
    }
    return self;
}

-(void) dealloc {
    [content release];
    [super dealloc];
}

-(CocoaLogicViewer*) content {
    return content;
}

-(void)boundsDidChange:(NSNotification*)notification {
    logicViewer->p.update();
}

-(void)scrollWheel:(NSEvent*)event {
    NSClipView* clipView = [self contentView];
    //CGFloat deltaX = event.scrollingDeltaX;
    CGFloat deltaY = event.scrollingDeltaY;
    NSPoint point = clipView.bounds.origin;

    CGFloat newX = point.x - (deltaY * 10.0);
    NSPoint newOrigin = NSMakePoint(newX, point.y);
    
    [clipView setBoundsOrigin:newOrigin];
    [self reflectScrolledClipView:clipView];
}
@end

#define DMA_SLOT_WIDTH 70u
#define SCROLL_STEPS 8
#define HI_LOGIC_WRITE  0xff6f61
#define HI_LOGIC_MNEMONIC 0x87cefa

namespace GUIKIT {

pLogicViewer::~pLogicViewer() {
    if (nsFont)
        [nsFont release];
}

auto pLogicViewer::init() -> void {
    @autoreleasepool {
        cocoaView = [[CocoaLogicViewerScroll alloc] initWith:logicViewer];
    }
    
    if([NSFont respondsToSelector:@selector(monospacedSystemFontOfSize:)]) {
        nsFont = [[NSFont monospacedSystemFontOfSize:11 weight:NSFontWeightRegular] retain];
    } else {
        nsFont = [[[NSFontManager sharedFontManager] fontWithFamily:@"Menlo" traits:0 weight:NSFontWeightRegular size:10] retain];
    }
    
    scrollTimer.onFinished = [this]() {
        scrollToActive();
    };
    scrollTimer.setInterval(20);
    scrollTimer.setData(0);
}

auto pLogicViewer::update() -> void {
    [[(id)cocoaView content] setNeedsDisplay:YES];
    unsigned maxSlots = logicViewer.state.logics.size();
    unsigned neededWidth = maxSlots * (DMA_SLOT_WIDTH + 1);
    const auto& geometry = logicViewer.geometry();
    
    unsigned height = geometry.height;
    
    CGFloat innerHeight = height;
    if ([(id)cocoaView hasHorizontalScroller]) {
        CGFloat scrollHeight = [NSScroller scrollerWidthForControlSize:NSControlSizeRegular scrollerStyle:[(id)cocoaView scrollerStyle]] + 2.0;
        
        if (scrollHeight < height)
            innerHeight = height - scrollHeight;
    }
    
    [[(id)cocoaView content] setFrameSize:NSMakeSize(neededWidth, innerHeight)];
}

auto pLogicViewer::setGeometry(Geometry geometry) -> void {
    if (logicViewer.window()) {
        [cocoaView removeFromSuperview];
        [[logicViewer.window()->p.cocoaWindow contentView] addSubview:cocoaView positioned:NSWindowBelow relativeTo:nil];
    }
    
    pWidget::setGeometry(geometry);
    update();
}

auto pLogicViewer::scrollToActive() -> void {
    NSClipView* clipView = [(id)cocoaView contentView];
    NSRect visibleRect = [clipView documentVisibleRect];
    int scrollPos = (int)visibleRect.origin.x;

    unsigned scrollSlot = 0;
    for (auto& logicState : logicViewer.state.logics) {
        if (!logicState.active)
            break;

        scrollSlot++;
    }

    unsigned targetPos = scrollSlot * (DMA_SLOT_WIDTH + 1);
    unsigned width = logicViewer.geometry().width >> 1;

    if (targetPos > width)
        targetPos -= width;
    else
        targetPos = 0;

    unsigned counter = scrollTimer.data();

    if ( ((counter + 1) >= SCROLL_STEPS) || (targetPos == scrollPos) ) {
        scrollPos = targetPos;
        scrollTimer.setEnabled( false );
        scrollTimer.setData(0);
    } else {
        if (targetPos < scrollPos)
            scrollPos -= (scrollPos - targetPos) / (SCROLL_STEPS - counter);
        else
            scrollPos += (targetPos - scrollPos) / (SCROLL_STEPS - counter);

        scrollTimer.setData(counter + 1);
        scrollTimer.setEnabled( true );
    }

    visibleRect.origin.x = scrollPos;
    [clipView setBoundsOrigin:visibleRect.origin];
    [(id)cocoaView reflectScrolledClipView:clipView];

    update();
}

auto pLogicViewer::redraw() -> void {
    unsigned maxSlots = logicViewer.state.logics.size();
    unsigned neededWidth = maxSlots * (DMA_SLOT_WIDTH + 1);
    const auto& geometry = logicViewer.geometry();

    unsigned width = geometry.width;
    unsigned height = geometry.height;
    
    NSClipView* clipView = [(id)cocoaView contentView];
    NSRect visibleRect = [clipView documentVisibleRect];
    CGFloat _scrollOffset = visibleRect.origin.x;

    unsigned offset = 0;
    unsigned scrollPos = (unsigned)_scrollOffset;

    if (neededWidth <= width)
        width = neededWidth;
    else {
        offset = neededWidth - width;

        if (offset >= scrollPos)
            offset = scrollPos;
    }

    unsigned firstSlot = offset / (DMA_SLOT_WIDTH + 1);
    offset = offset % (DMA_SLOT_WIDTH + 1);
    unsigned buildSlots = (width / (DMA_SLOT_WIDTH + 1)) + 2;

    if (firstSlot >= maxSlots)
        firstSlot = 0;

    unsigned endSlot = firstSlot + buildSlots;
    if (endSlot > maxSlots)
        endSlot = maxSlots;

    int _startX = firstSlot * (DMA_SLOT_WIDTH + 1);

    Geometry geo = {_startX, 0, DMA_SLOT_WIDTH, height};

    CGContextRef context = [[NSGraphicsContext currentContext] CGContext];

    @autoreleasepool {
        CGContextSetFillColorWithColor(context, [pHelper::RGBToNSColor(0x383838) CGColor]);
        CGContextFillRect(context, CGRectMake(_startX, 0, buildSlots * (DMA_SLOT_WIDTH + 1), height));
        
        for (unsigned i = firstSlot; i < endSlot; i++) {
            auto& logicState = logicViewer.state.logics[i];
            bool lastSlot = i == (maxSlots - 1);
            if (lastSlot)
                geo.width += 1;
            buildDmaSlot(context, logicState, geo, lastSlot );
            geo.x += DMA_SLOT_WIDTH + 1;
        }
    }
}

auto pLogicViewer::buildDmaSlot(CGContextRef context, LogicState& logicState, Geometry geo, bool lastSlot) -> void {
    int addrLength = logicViewer.addrAs24bit() ? 6 : 4;
    
    if (!lastSlot) {
        CGContextMoveToPoint(context, pg(geo.x + geo.width), pg(geo.y));
        CGContextAddLineToPoint(context, pg(geo.x + geo.width), pg(geo.y + geo.height));
        CGContextSetStrokeColorWithColor(context, [pHelper::RGBToNSColor(0x545454) CGColor]);
        CGContextSetLineWidth(context, 1.0);
        CGContextStrokePath(context);
    }
    
    NSColor* nsCol = pHelper::RGBToNSColor(logicState.active ? 0xe0e0e0 : 0x808080);
    
    geo.height = 20;
    drawText(geo, std::to_string(logicState.position), nsCol);
    geo.y += geo.height + 5;
    
    if (logicState.display != LogicState::Display::EmptyBlock) {
        CGContextSetLineWidth(context, 5.0);
        CGContextMoveToPoint(context, pg(geo.x), pg(geo.y));
        CGContextAddLineToPoint(context, pg(geo.x + geo.width), pg(geo.y));
        CGContextSetStrokeColorWithColor(context, [pHelper::RGBToNSColor(logicState.color) CGColor]);
        CGContextStrokePath(context);
    }
    
    CGContextSetStrokeColorWithColor(context, [nsCol CGColor]);
    CGContextSetLineWidth(context, 1.0);
    
    setBox(geo, (int)LogicState::Offset::Usage1);
    
    if (logicState.display == LogicState::Display::EmptyBlock) {
        drawText(geo, "-", nsCol);
        setBox(geo, (int)LogicState::Offset::Addr1);
        drawLine(context, geo, nsCol);
        setBox(geo, (int)LogicState::Offset::Data1);
        drawLine(context, geo, nsCol);
    } else {
        drawText(geo, logicState.usage, nsCol);
        setBox(geo, (int)LogicState::Offset::Addr1);
        
        std::string addr = logicState.symbolicAddr.empty() ? String::convertToHex(logicState.addr, addrLength) : logicState.symbolicAddr;
        drawRectRounded(context, geo, addr, nsCol, 5);
        setBox(geo, (int)LogicState::Offset::Data1);
        drawRectRounded(context, geo, String::convertToHex(logicState.data), nsCol, 10);
    }
    
    setBox(geo, (int)LogicState::Offset::Mnemonic);
    if (logicState.mnemonic) {
        std::string _mnemonic = logicState.mnemonic;
        String::toUpperCase( _mnemonic );
        if (logicState.hilight == LogicState::Hilight::Mnemonic) {
            drawText(geo, _mnemonic, pHelper::RGBToNSColor(HI_LOGIC_MNEMONIC));
        } else
            drawText(geo, _mnemonic, nsCol);
    } else
        drawText(geo, "", nsCol);
    
    setBox(geo, (int)LogicState::Offset::Usage2);
    
    if (logicState.display2 == LogicState::Display::EmptyBlock) {
        drawText(geo, "-", nsCol);
        setBox(geo, (int)LogicState::Offset::Addr2);
        drawLine(context, geo, nsCol);
        setBox(geo, (int)LogicState::Offset::Data2);
        drawLine(context, geo, nsCol);
    } else {
        drawText(geo, logicState.usage2, nsCol);
        setBox(geo, (int)LogicState::Offset::Addr2);
        drawRectRounded(context, geo, String::convertToHex(logicState.addr2, addrLength), nsCol, 5);
        setBox(geo, (int)LogicState::Offset::Data2);
        if (logicState.hilight == LogicState::Hilight::Write) {
            drawRectRounded(context, geo, String::convertToHex(logicState.data2), pHelper::RGBToNSColor(HI_LOGIC_WRITE), 10);
            CGContextSetStrokeColorWithColor(context, [nsCol CGColor]);
        } else
            drawRectRounded(context, geo, String::convertToHex(logicState.data2), nsCol, 10);
    }
    
    int i = 0;
    for (auto& watch : logicState.watches) {
        setBox(geo, (int)(LogicState::Offset::Watch1) + i++);
        drawRect(context, watch.first, geo, String::convertToHex(watch.second), nsCol, 10);
    }
}

auto pLogicViewer::drawLine(CGContextRef context, Geometry& geo, NSColor* nsCol) -> void {
    unsigned center = geo.y + geo.height / 2;
    CGContextMoveToPoint(context, pg(geo.x), pg(center));
    CGContextAddLineToPoint(context, pg(geo.x + geo.width), pg(center));
    CGContextStrokePath(context);
}

auto pLogicViewer::drawRect(CGContextRef context, LogicState::Display display, Geometry& geo, const std::string& text, NSColor* nsCol, unsigned padding) -> void {
    switch (display) {
        default:
        case LogicState::Display::EmptyBlock:
            drawLine(context, geo, nsCol);
            break;
        case LogicState::Display::SingleBlock:
            drawRectRounded(context, geo, text, nsCol, padding);
            break;
        case LogicState::Display::BeginBlock:
            drawRectLeftRounded(context, geo, text, nsCol, padding);
            break;
        case LogicState::Display::KeepBlock:
            drawRect(context, geo, text, nsCol);
            break;
        case LogicState::Display::EndBlock:
            drawRectRightRounded(context, geo, text, nsCol, padding);
            break;
    }
}

auto pLogicViewer::drawRect(CGContextRef context, Geometry& geo, const std::string& str, NSColor* nsCol) -> void {
    CGContextMoveToPoint(context, pg(geo.x), pg(geo.y));
    CGContextAddLineToPoint(context, pg(geo.x + geo.width), pg(geo.y));
    
    CGContextMoveToPoint(context, pg(geo.x), pg(geo.y + geo.height));
    CGContextAddLineToPoint(context, pg(geo.x + geo.width), pg(geo.y + geo.height));
    
    CGContextStrokePath(context);
    
    drawText(geo, str, nsCol);
}

auto pLogicViewer::drawRectRounded(CGContextRef context, Geometry& geo, const std::string& str, NSColor* nsCol, unsigned padding) -> void {
    unsigned center = geo.y + geo.height / 2;
    CGContextMoveToPoint(context, pg(geo.x), pg(center));
    CGContextAddLineToPoint(context, pg(geo.x + padding), pg(center));
    
    CGContextMoveToPoint(context, pg(geo.x + geo.width - padding), pg(center));
    CGContextAddLineToPoint(context, pg(geo.x + geo.width), pg(center));
    
    auto _geo = geo;
    _geo.x += padding;
    _geo.width -= padding * 2;
    setRoundedPath(context, _geo);

    CGContextStrokePath(context);
    
    drawText(geo, str, nsCol);
}

auto pLogicViewer::drawRectRightRounded(CGContextRef context, Geometry& geo, const std::string& str, NSColor* nsCol, unsigned padding) -> void {
    unsigned center = geo.y + geo.height / 2;
    CGContextMoveToPoint(context, pg(geo.x + geo.width - padding), pg(center));
    CGContextAddLineToPoint(context, pg(geo.x + geo.width), pg(center));
    
    auto _geo = geo;
    _geo.width -= padding;
    setRightRoundedPath(context, _geo);

    CGContextStrokePath(context);
    
    drawText(_geo, str, nsCol);
}

auto pLogicViewer::drawRectLeftRounded(CGContextRef context, Geometry& geo, const std::string& str, NSColor* nsCol, unsigned padding) -> void {
    unsigned center = geo.y + geo.height / 2;
    CGContextMoveToPoint(context, pg(geo.x), pg(center));
    CGContextAddLineToPoint(context, pg(geo.x + padding), pg(center));
    
    auto _geo = geo;
    _geo.x += padding;
    _geo.width -= padding;
    setLeftRoundedPath(context, _geo);

    CGContextStrokePath(context);
    
    drawText(_geo, str, nsCol);
}

auto pLogicViewer::setRightRoundedPath(CGContextRef context, Geometry& geo) -> void {
    constexpr CGFloat radius = 5;
    
    CGContextMoveToPoint(context, pg(geo.x), pg(geo.y));
    CGContextAddLineToPoint(context, pg(geo.x + geo.width - radius), pg(geo.y));
    CGContextAddArc(context, pg(geo.x + geo.width - radius), pg(geo.y + radius), radius, 3 * M_PI_2, 0, 0);
    CGContextAddLineToPoint(context, pg(geo.x + geo.width), pg(geo.y + geo.height - radius));
    CGContextAddArc(context, pg(geo.x + geo.width - radius), pg(geo.y + geo.height - radius), radius, 0, M_PI_2, 0);
    CGContextAddLineToPoint(context, pg(geo.x), pg(geo.y + geo.height));
}

auto pLogicViewer::setLeftRoundedPath(CGContextRef context, Geometry& geo) -> void {
    constexpr CGFloat radius = 5;

    CGContextMoveToPoint(context, pg(geo.x + radius), pg(geo.y));
    CGContextAddLineToPoint(context, pg(geo.x + geo.width), pg(geo.y));
    CGContextMoveToPoint(context, pg(geo.x + geo.width), pg(geo.y + geo.height));
    CGContextAddLineToPoint(context, pg(geo.x + radius), pg(geo.y + geo.height));
    CGContextAddArc(context, pg(geo.x + radius), pg(geo.y + geo.height - radius), radius, M_PI_2, M_PI, 0);
    CGContextAddLineToPoint(context, pg(geo.x), pg(geo.y + radius));
    CGContextAddArc(context, pg(geo.x + radius), pg(geo.y + radius), radius, M_PI, 3 * M_PI_2, 0);
}

auto pLogicViewer::setRoundedPath(CGContextRef context, Geometry& geo) -> void {
    constexpr CGFloat radius = 5;
    
    CGContextMoveToPoint(context, pg(geo.x + radius), pg(geo.y));
    CGContextAddArcToPoint(context, pg(geo.x + geo.width), pg(geo.y), pg(geo.x + geo.width), pg(geo.y + geo.height), radius);
    CGContextAddArcToPoint(context, pg(geo.x + geo.width),pg(geo.y + geo.height), pg(geo.x), pg(geo.y + geo.height), radius);
    CGContextAddArcToPoint(context, pg(geo.x), pg(geo.y + geo.height), pg(geo.x), pg(geo.y), radius);
    CGContextAddArcToPoint(context, pg(geo.x), pg(geo.y), pg(geo.x + geo.width), pg(geo.y), radius);
}

auto pLogicViewer::drawText(Geometry& geo, const std::string& str, NSColor* nsCol) -> void {
    NSString* nsStr = [NSString stringWithUTF8String:str.c_str()];
    if (!nsStr)
        return;
    
    NSDictionary* attrsDictionary = nullptr;
    
    if (nsFont)
        attrsDictionary = @{NSFontAttributeName: nsFont, NSForegroundColorAttributeName: nsCol};
    else
        attrsDictionary = @{NSForegroundColorAttributeName: nsCol};
    
    if (!attrsDictionary)
        return;
    
    NSAttributedString* attrString = [[NSAttributedString alloc] initWithString:nsStr attributes:attrsDictionary];
    
    int _x = geo.x;
    int _y = geo.y;
    
    if (attrString) {
        auto _s = [attrString size];
         
        if (_s.width < geo.width)
            _x += (geo.width - _s.width) / 2;
        
        if (_s.height < geo.height)
            _y += (geo.height - _s.height) / 2;
        
        [attrString drawAtPoint:CGPointMake(pg(_x), pg(_y))];
    }
}

auto pLogicViewer::setBox(Geometry& geo, int offset) -> void {
    auto o = logicViewer.state.offsets;
    unsigned y = o[offset];
    
    if (offset == 0 || offset == 3)
        y += 1;

    geo.y = y > 24 ? y - 24 : y;
}

inline auto pLogicViewer::pg(int val) -> CGFloat {
    return (CGFloat)val + 0.5;
}

}
