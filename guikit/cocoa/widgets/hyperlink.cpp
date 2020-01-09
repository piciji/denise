
@implementation CocoaHyperlink : NSTextField

-(id) initWith:(GUIKIT::Hyperlink&)hyperlinkReference {
    if(self = [super initWithFrame:NSMakeRect(0, 0, 0, 0)]) {
        hyperlink = &hyperlinkReference;
        
        [self setAlignment:NSLeftTextAlignment];
        [self setBordered:NO];
        [self setDrawsBackground:NO];
        [self setEditable:NO];
        [self setAllowsEditingTextAttributes: YES];
        [self setSelectable: YES];
    }
    return self;
}
- (void)resetCursorRects {
    [self addCursorRect:[self bounds] cursor:[NSCursor pointingHandCursor]];
}

@end

namespace GUIKIT {
    
auto pHyperlink::minimumSize() -> Size {
    Size size = pFont::size([cocoaView font], widget.text());
    return {size.width + 1, size.height + 4};
}
    
auto pHyperlink::setGeometry(Geometry geometry) -> void {
    unsigned height = pFont::size([cocoaView font], " ").height;
    unsigned widgetHeight = geometry.height + 4;
    auto offset = geometry;
    
    if(widgetHeight > height) {
       unsigned diff = widgetHeight - height;
       offset.y += diff >> 1;
       offset.height -= diff >> 1;
    }
    
    pWidget::setGeometry({
        offset.x - 2, offset.y - 2,
        offset.width + 4, offset.height + 4
    });
}
    
auto pHyperlink::setText(std::string text) -> void {
    @autoreleasepool {
        [cocoaView setStringValue:[NSString stringWithUTF8String:text.c_str()]];
    }
}

auto pHyperlink::setUri( std::string uri, std::string wrap ) -> void {
	
	updateLink();
}

auto pHyperlink::updateLink() -> void {
	std::string text = hyperlink.text();
	std::string uri = hyperlink.uri();
	std::string wrap = hyperlink.wrap();
    
	if (wrap.empty())
		wrap = uri;
        
    if (text.empty()) {
        hyperlink.setText( wrap );
    }
    
    @autoreleasepool {
        NSURL* url = [NSURL URLWithString:[NSString stringWithUTF8String:uri.c_str()]];

        NSAttributedString* attrString = [cocoaView attributedStringValue];

        NSMutableAttributedString* attr = [[NSMutableAttributedString alloc] initWithAttributedString:attrString];
        
        NSRange range = NSMakeRange(0, [attr length]);
        
        std::size_t found = text.find( wrap );
        
        if (found != std::string::npos) {
            NSTextField* wrapField = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, 0, 0) ];
            [wrapField setStringValue:[NSString stringWithUTF8String:wrap.c_str()]];

            NSAttributedString* attrStringWrap = [wrapField attributedStringValue];
            
            NSMutableAttributedString* attrWrap = [[NSMutableAttributedString alloc] initWithAttributedString:attrStringWrap];
            
            range = NSMakeRange(found, [attrWrap length]);
        }

        [attr addAttribute:NSLinkAttributeName value:url range:range];
        [attr addAttribute:NSForegroundColorAttributeName value:[NSColor blueColor] range:range ];
        [attr addAttribute:NSUnderlineStyleAttributeName value:[NSNumber numberWithInt:NSUnderlineStyleSingle] range:range];

        [cocoaView setAttributedStringValue:attr];
    }
}
    
auto pHyperlink::setEnabled(bool enabled) -> void {
    
    NSColor* textColor = [NSColor blackColor];
    
    if(hyperlink.overrideForegroundColor()) {
        unsigned color = hyperlink.foregroundColor();
        textColor = [NSColor
                     colorWithCalibratedRed:((color>>16) & 0xff) / 255.0
                     green:((color>>8) & 0xff) / 255.0
                     blue:(color & 0xff) / 255.0
                     alpha: 1.0];
    }
    
    [cocoaView setTextColor: enabled ? textColor : [NSColor grayColor]];
    pWidget::setEnabled(enabled);
}
    
auto pHyperlink::setForegroundColor(unsigned color) -> void {
    setEnabled( hyperlink.enabled() );
}


auto pHyperlink::init() -> void {
    cocoaView = [[CocoaHyperlink alloc] initWith:hyperlink];
}   
    
}       

