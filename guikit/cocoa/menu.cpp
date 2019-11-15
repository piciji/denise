
@implementation CocoaMenu : NSMenuItem

-(id) initWith {
    if(self = [super initWithTitle:@"" action:nil keyEquivalent:@""]) {        
        cocoaMenu = [[NSMenu alloc] initWithTitle:@""];
        [cocoaMenu setAutoenablesItems:NO];
        [self setSubmenu:cocoaMenu];
    }
    return self;
}
-(NSMenu*) cocoaMenu { return cocoaMenu; }
@end

@implementation CocoaMenuItem : NSMenuItem

-(id) initWith:(GUIKIT::MenuItem&)menuItemReference {
    if(self = [super initWithTitle:@"" action:@selector(activate) keyEquivalent:@""]) {
        menuItem = &menuItemReference;
        [self setTarget:self];
    }
    return self;
}
-(void) activate {
    if(menuItem->onActivate) menuItem->onActivate();
}
@end

@implementation CocoaMenuCheckItem : NSMenuItem

-(id) initWith:(GUIKIT::MenuCheckItem&)menuCheckItemReference {
    if(self = [super initWithTitle:@"" action:@selector(activate) keyEquivalent:@""]) {
        menuCheckItem = &menuCheckItemReference;
        [self setTarget:self];
    }
    return self;
}
-(void) activate {
    menuCheckItem->state.checked = !menuCheckItem->checked();
    auto state = menuCheckItem->checked() ? NSOnState : NSOffState;
    [self setState:state];
    if(menuCheckItem->onToggle) menuCheckItem->onToggle();
}
@end

@implementation CocoaMenuRadioItem : NSMenuItem

-(id) initWith:(GUIKIT::MenuRadioItem&)menuRadioItemReference {
    if(self = [super initWithTitle:@"" action:@selector(activate) keyEquivalent:@""]) {
        menuRadioItem = &menuRadioItemReference;
        
        [self setTarget:self];
        [self setOnStateImage:[NSImage imageNamed:@"NSMenuRadio"]];
    }
    return self;
}
-(void) activate {
    menuRadioItem->setChecked();
    if(menuRadioItem->onActivate) menuRadioItem->onActivate();
}
@end

namespace GUIKIT {
    
//base
pMenuBase::~pMenuBase() {
    @autoreleasepool {
        [cocoaBase release];
    }
}

auto pMenuBase::setEnabled(bool enabled) -> void {
    @autoreleasepool {
        [cocoaBase setEnabled:enabled];
    }
}

auto pMenuBase::setVisible(bool visible) -> void {
    @autoreleasepool {
        [cocoaBase setHidden:!visible];
    }
}

auto pMenuBase::setText(const std::string& text) -> void {
    @autoreleasepool {
        [cocoaBase setTitle:[NSString stringWithUTF8String:text.c_str()]];
    }
}

auto pMenuBase::setIcon(Image& icon) -> void {
    
    @autoreleasepool {
        if (icon.empty()) {
            if (dynamic_cast<pMenuCheckItem*>(this)) [cocoaBase setOnStateImage:[NSImage imageNamed:@"NSMenuCheckmark"]];
            else if (dynamic_cast<pMenuRadioItem*>(this)) [cocoaBase setOnStateImage:[NSImage imageNamed:@"NSMenuRadio"]];
            else [cocoaBase setImage:nil];
        } else {
            if (dynamic_cast<pMenuCheckItem*>(this) || dynamic_cast<pMenuRadioItem*>(this)) [cocoaBase setOnStateImage:NSMakeImage(icon, 15, 15)];
            else [cocoaBase setImage:NSMakeImage(icon, 15, 15)];
        }
    }
}

//menu
pMenu::pMenu(Menu& menu) : pMenuBase(menu), menu(menu) { }
pMenu::~pMenu() {
    @autoreleasepool {
        [[cocoaBase cocoaMenu] release];
    }
}
    
auto pMenu::setIcon(Image& icon) -> void {
    
    if (menuBase.state.parentWindow && menuBase.state.parentWindow->p.disableIconsInTopMenu) {
        if ( menuBase.state.parentWindow->isApended(menu) ) {
            [cocoaBase setImage:nil];
            return;
        }
    }
    
    pMenuBase::setIcon( icon );
}

auto pMenu::init() -> void {
    @autoreleasepool {
        cocoaBase = [[CocoaMenu alloc] initWith];
    }
}
    
auto pMenu::setText(const std::string& text) -> void {
    @autoreleasepool {
        [[cocoaBase cocoaMenu]setTitle:[NSString stringWithUTF8String:text.c_str()]];
    }
    pMenuBase::setText(text);
}

auto pMenu::append(MenuBase& item) -> void {
    @autoreleasepool {
        [[cocoaBase cocoaMenu] addItem:item.p.cocoaBase];
    }
}

auto pMenu::remove(MenuBase& item) -> void {
    @autoreleasepool {
        [[cocoaBase cocoaMenu] removeItem:item.p.cocoaBase];
    }
}

//item
pMenuItem::pMenuItem(MenuItem& menuItem) : pMenuBase(menuItem), menuItem(menuItem) {}

auto pMenuItem::init() -> void {
    @autoreleasepool {
        cocoaBase = [[CocoaMenuItem alloc] initWith:menuItem];
    }
}
    
//check item
pMenuCheckItem::pMenuCheckItem(MenuCheckItem& menuCheckItem) : pMenuBase(menuCheckItem), menuCheckItem(menuCheckItem) { }

auto pMenuCheckItem::init() -> void {
    @autoreleasepool {
        cocoaBase = [[CocoaMenuCheckItem alloc] initWith:menuCheckItem];
    }
}

auto pMenuCheckItem::setChecked(bool checked) -> void {
    @autoreleasepool {
        auto state = checked ? NSOnState : NSOffState;
        [cocoaBase setState:state];
    }
}

//radio item
pMenuRadioItem::pMenuRadioItem(MenuRadioItem& menuRadioItem) : pMenuBase(menuRadioItem), menuRadioItem(menuRadioItem) { }

auto pMenuRadioItem::init() -> void {
    @autoreleasepool {
        cocoaBase = [[CocoaMenuRadioItem alloc] initWith:menuRadioItem];
    }
}

auto pMenuRadioItem::setChecked() -> void {
    @autoreleasepool {
        for(auto& item : menuRadioItem.group) {
            auto state = (item == &menuRadioItem) ? NSOnState : NSOffState;
            [item->p.cocoaBase setState:state];
        }
    }
}

//separator
pMenuSeparator::pMenuSeparator(MenuSeparator& menuSeparator) : pMenuBase(menuSeparator), menuSeparator(menuSeparator) { }

auto pMenuSeparator::init() -> void {
    @autoreleasepool {
        cocoaBase = [[NSMenuItem separatorItem] retain];
    }
}
    
}
