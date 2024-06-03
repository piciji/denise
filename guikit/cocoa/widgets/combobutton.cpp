
@implementation CocoaComboButton : NSPopUpButton

-(id) initWith:(GUIKIT::ComboButton&)comboButtonReference {
    if(self = [super initWithFrame:NSMakeRect(0, 0, 0, 0) pullsDown:NO]) {
        comboButton = &comboButtonReference;
        [self setTarget:self];
        if (GUIKIT::isBigSur()) {
            [[self cell] setBezelStyle: NSBezelStyleTexturedRounded];
            [[self cell] setArrowPosition: NSPopUpArrowAtBottom];
        }
        [self setAction:@selector(activate:)];
    }
    return self;
}

-(IBAction) activate:(id)sender {
    comboButton->state.selection = [self indexOfSelectedItem];
    if(comboButton->onChange) comboButton->onChange();
}
@end

namespace GUIKIT {
    
auto pComboButton::append(std::string text) -> void {
    @autoreleasepool {
        [(id)cocoaView addItemWithTitle:[NSString stringWithUTF8String:text.c_str()]];
    }
    calculatedMinimumSize.updated = false;
}

auto pComboButton::minimumSize() -> Size {
    if (calculatedMinimumSize.updated)
        return calculatedMinimumSize.minimumSize; 
        
    unsigned maximumWidth = 0;
    for(auto& text : comboButton.state.rows)
        maximumWidth = std::max(maximumWidth, pFont::size([(id)cocoaView font], text).width);
    
    Size size = pFont::size([(id)cocoaView font], " ");
    
    calculatedMinimumSize.updated = true;   
    calculatedMinimumSize.minimumSize = {maximumWidth + 36, size.height + 6};
    
    return calculatedMinimumSize.minimumSize;
}
    
auto pComboButton::setGeometry(Geometry geometry) -> void {
    pWidget::setGeometry({
        geometry.x - 2, geometry.y + (GUIKIT::hasMinimumVersion(10, 15) ? 1 : 0),
        geometry.width + 4, geometry.height
    });
}

auto pComboButton::remove(unsigned selection) -> void {
    @autoreleasepool {
        [(id)cocoaView removeItemAtIndex:selection];
    }
}

auto pComboButton::reset() -> void {
    @autoreleasepool {
        [(id)cocoaView removeAllItems];
    }
}

auto pComboButton::setSelection(unsigned selection) -> void {
    @autoreleasepool {
        [(id)cocoaView selectItemAtIndex:selection];
    }
}

auto pComboButton::setText(unsigned selection, const std::string& text) -> void {
    @autoreleasepool {
        [[(id)cocoaView itemAtIndex:selection] setTitle:[NSString stringWithUTF8String:text.c_str()]];
    }
    calculatedMinimumSize.updated = false;
}

auto pComboButton::init() -> void {
    @autoreleasepool {
        cocoaView = [[CocoaComboButton alloc] initWith:comboButton];
    }
}
    
}
