@implementation CocoaTabFrame : NSTabView

-(id) initWith:(GUIKIT::pTabFrame&)pTabFrameReference {
    if(self = [super initWithFrame:NSMakeRect(0, 0, 0, 0)]) {
        p = &pTabFrameReference;
        [self setDelegate:self];
    }
    return self;
}

-(void) tabView:(NSTabView*)tabView didSelectTabViewItem:(NSTabViewItem*)tabViewItem {
    p->tabFrame.state.selection = [tabView indexOfTabViewItem:tabViewItem];
    if(p->tabFrame.onChange) p->tabFrame.onChange();
}
@end

@implementation CocoaTabFrameItem : NSTabViewItem

-(id) initWith:(GUIKIT::pTabFrame&)pTabFrameReference {
    if(self = [super initWithIdentifier:nil]) {
        p = &pTabFrameReference;
        cocoaTabFrame = p->cocoaView;
    }
    return self;
}

-(NSSize) sizeOfLabel:(BOOL)shouldTruncateLabel {
    NSSize sizeOfLabel = [super sizeOfLabel:shouldTruncateLabel];
    signed selection = [cocoaTabFrame indexOfTabViewItem:self];
    if(selection < 0) return sizeOfLabel;
    if(p->cocoaImages.size() <= selection) return sizeOfLabel;
    
    NSImage* image = p->cocoaImages.at(selection);
    if(image) {
        unsigned iconSize = GUIKIT::pFont::size([cocoaTabFrame font], " ").height;
        sizeOfLabel.width += iconSize + 2;
    }
    return sizeOfLabel;
}

-(void) drawLabel:(BOOL)shouldTruncateLabel inRect:(NSRect)tabRect {
    signed selection = [cocoaTabFrame indexOfTabViewItem:self];
    if(selection >= 0) {
        if(p->cocoaImages.size() > selection) {
            NSImage* image = p->cocoaImages.at(selection);
            if(image) {
                unsigned iconSize = GUIKIT::pFont::size([cocoaTabFrame font], " ").height;
            
                [[NSGraphicsContext currentContext] saveGraphicsState];
                NSRect targetRect = NSMakeRect(tabRect.origin.x, tabRect.origin.y + 0, iconSize, iconSize);
                [image drawInRect:targetRect fromRect:NSZeroRect operation:NSCompositeSourceOver fraction:1.0 respectFlipped:YES hints:nil];
                [[NSGraphicsContext currentContext] restoreGraphicsState];
            
                tabRect.origin.x += iconSize + 2;
                tabRect.size.width -= iconSize + 2;
            }
        }
    }
    tabRect.origin.y += 0;
    [super drawLabel:shouldTruncateLabel inRect:tabRect];
}
@end

namespace GUIKIT {

pTabFrame::~pTabFrame() {
    @autoreleasepool {
        for(auto& cocoaImage : cocoaImages) [cocoaImage release];
    }
}

auto pTabFrame::minimumSize() -> Size {
    std::string text = tabFrame.text(0);
    Size size = pFont::size([cocoaView font], text);
    return {size.width + (borderSize() << 1) + 55, size.height + (borderSize() << 1) + 4 };
}
    
auto pTabFrame::setGeometry(Geometry geometry) -> void {
    pWidget::setGeometry({
        int(geometry.x - 7), int(geometry.y - 6),
        geometry.width + 14, geometry.height + 16
    });
}

auto pTabFrame::append(std::string text, Image* image) -> void {
    @autoreleasepool {
        CocoaTabFrameItem* item = [[CocoaTabFrameItem alloc] initWith:*this];
        [item setLabel:[NSString stringWithUTF8String:text.c_str()]];
        [cocoaView addTabViewItem:item];
        tabs.push_back(item);
        cocoaImages.push_back(nil);
    }
    if(image) setImage(cocoaImages.size() - 1, *image);
}

auto pTabFrame::remove(unsigned selection) -> void {
    @autoreleasepool {
        CocoaTabFrameItem* item = tabs[selection];
        [cocoaView removeTabViewItem:item];
        tabs.erase(tabs.begin() + selection);
        [cocoaImages.at(selection) release];
        cocoaImages.erase(cocoaImages.begin() + selection);
    }
}

auto pTabFrame::setImage(unsigned selection, Image& image) -> void {
    @autoreleasepool {
        [cocoaImages.at(selection) release];
        cocoaImages.at(selection) = NSMakeImage(image);
    }
}

auto pTabFrame::setText(unsigned selection, std::string text) -> void {
    @autoreleasepool {
        CocoaTabFrameItem* item = tabs[selection];
        [item setLabel:[NSString stringWithUTF8String:text.c_str()]];
    }
}

auto pTabFrame::setSelection(unsigned selection) -> void {
    @autoreleasepool {
        CocoaTabFrameItem* item = tabs[selection];
        [cocoaView selectTabViewItem:item];
    }
}

auto pTabFrame::init() -> void {
    @autoreleasepool {
        cocoaView = [[CocoaTabFrame alloc] initWith:*this];
    }
}

}
