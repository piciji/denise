
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

-(NSDragOperation) draggingEntered:(id<NSDraggingInfo>)sender {
    return GUIKIT::DropPathsOperation(sender);
}

-(BOOL) performDragOperation:(id<NSDraggingInfo>)sender {
    auto paths = GUIKIT::getDropPaths(sender);
    if(paths.empty()) return NO;
    if(comboButton->onDrop) comboButton->onDrop(paths);
    return YES;
}

@end

namespace GUIKIT {
    
auto pComboButton::append(std::string text, const std::string& font) -> void {
    calculatedMinimumSize.updated = false;
    @autoreleasepool {
        int tries = 0;
        int expectedCount = comboButton.rows();
        std::string altText = text;
        
        NSString* nsStr = [NSString stringWithUTF8String:text.c_str()];
        if (nsStr)
            [(id)cocoaView addItemWithTitle:nsStr];
        
        int realCount = [(id)cocoaView numberOfItems]; //expects unique names
        
        while (realCount < expectedCount) {
            tries++;
            if(tries > 5)
                return;
                
            altText = text + "_" + std::to_string(tries);
            nsStr = [NSString stringWithUTF8String:altText.c_str()];
            if (nsStr)
                [(id)cocoaView addItemWithTitle:nsStr];
            realCount = [(id)cocoaView numberOfItems];
        }
        
        if (!font.empty()) {
            NSFont* nsfont = pFont::cocoaFont(font);
            
            if (nsfont != nil) {
                NSDictionary* attrsDictionary = [NSDictionary dictionaryWithObject:nsfont forKey:NSFontAttributeName];
                NSAttributedString* attrString = [[NSAttributedString alloc] initWithString:nsStr attributes:attrsDictionary];
                
                [[(id)cocoaView itemAtIndex:expectedCount-1] setAttributedTitle:attrString];
            }
        }
    }
}

auto pComboButton::minimumSize() -> Size {
    return getMinimumSize();
}
    
auto pComboButton::setGeometry(Geometry geometry) -> void {
    if(@available(macOS 26.0, *));
    else
        geometry.y += 1;
    
    pWidget::setGeometry({
        geometry.x, geometry.y,
        geometry.width, geometry.height
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
        [(id)cocoaView selectItemAtIndex:selection == ~0 ? -1 : selection];
    }
}

auto pComboButton::setText(unsigned selection, const std::string& text) -> void {
    @autoreleasepool {
        [[(id)cocoaView itemAtIndex:selection] setTitle:[NSString stringWithUTF8String:text.c_str()]];
    }
    calculatedMinimumSize.updated = false;
}

auto pComboButton::setDroppable(bool droppable) -> void {
    @autoreleasepool {
        if(droppable) {
            [cocoaView registerForDraggedTypes:@[NSPasteboardTypeFileURL]];
        } else {
            [cocoaView unregisterDraggedTypes];
        }
    }
}

auto pComboButton::init() -> void {
    @autoreleasepool {
        cocoaView = [[CocoaComboButton alloc] initWith:comboButton];
    }
}
    
}
