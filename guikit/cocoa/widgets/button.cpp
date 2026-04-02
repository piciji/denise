
@implementation CocoaButton : NSButton

-(id) initWith:(GUIKIT::Button&)buttonReference {
    if(self = [super initWithFrame:NSMakeRect(0, 0, 0, 0)]) {
        button = &buttonReference;
        [self setTarget:self];
        [self setAction:@selector(activate:)];
        // dirty workaround to build on 10.13 SDK, NSBezelStyleFlexiblePush needs at least 10.15
        #define _NSBezelStyleFlexiblePush 2
        [self setBezelStyle:(NSBezelStyle)_NSBezelStyleFlexiblePush];
        
    }
    return self;
}

-(IBAction) activate:(id)sender {
    if(button->onActivate) button->onActivate();
}

- (void)mouseDown:(NSEvent*)event {
    
    if (button->onMenu) {
        auto* menu = button->onMenu();
        
        if (menu) {
            GUIKIT::pApplication::observeMenu([(id)menu->p.cocoaBase cocoaMenu]);
            
            [NSMenu popUpContextMenu: [(id)menu->p.cocoaBase cocoaMenu] withEvent:event forView:self];
            
            return;
        }
    }
    [super mouseDown:event];
}
@end

namespace GUIKIT {
    
auto pButton::minimumSize() -> Size {
    Size size = getMinimumSize();

    if (!button.image())
        return {size.width + 22, size.height + 6};

    if (button.text().empty())
        return {button.image()->width + 22, button.image()->height + 6};

    return {size.width + button.image()->width + 22, size.height + 6};
}
    
auto pButton::setGeometry(Geometry geometry) -> void {
    int adjust = 2;
      if (!button.image())
        adjust = 6;
    
    pWidget::setGeometry({
        geometry.x - adjust, geometry.y - 2,
        geometry.width + (adjust * 2), geometry.height + 4
    });
}
    
auto pButton::setImage(Image* image) -> void {
    @autoreleasepool {
        if (!image) {
            [(id)cocoaView setImage:nil];
            return;
        }
        
        [(id)cocoaView setImage:NSMakeImage(*image)];
        
        if (widget.text().empty())
            [(id)cocoaView setImagePosition:NSImageOnly];
        else
            [(id)cocoaView setImagePosition:NSImageLeft];
    }
}

auto pButton::setText(const std::string& text) -> void {
    @autoreleasepool {
        [(id)cocoaView setTitle:[NSString stringWithUTF8String:text.c_str()]];
        
        if (button.image())
            [(id)cocoaView setImagePosition:NSImageLeft];
            
    }
    calculatedMinimumSize.updated = false;
}

auto pButton::init() -> void {
    @autoreleasepool {
        cocoaView = [[CocoaButton alloc] initWith:button];
    }
}
    
}        
