
#include "main.h"

#include "tools.cpp"
#include "menu.cpp"
#include "browserWindow.cpp"
#include "messageWindow.cpp"

#include "widgets/widget.cpp"   
#include "widgets/button.cpp"   
#include "widgets/lineedit.cpp"
#include "widgets/label.cpp"
#include "widgets/checkbutton.cpp"
#include "widgets/checkbox.cpp"
#include "widgets/combobutton.cpp"
#include "widgets/slider.cpp"
#include "widgets/radiobox.cpp"
#include "widgets/progressbar.cpp"
#include "widgets/frame.cpp"
#include "widgets/tabframe.cpp"
#include "widgets/viewport.cpp"
#include "widgets/listview.cpp"
#include "widgets/treeview.cpp"
#include "widgets/squareCanvas.cpp"
#include "widgets/hyperlink.cpp"

@implementation CocoaDelegate : NSObject

-(NSApplicationTerminateReply) applicationShouldTerminate:(NSApplication*)sender {
    using GUIKIT::Application;
    if(Application::Cocoa::onQuit) Application::Cocoa::onQuit();
    else Application::quit();
    return NSTerminateCancel;
}

-(BOOL)application:(NSApplication*)sender openFile:(NSString*)filename {
    using GUIKIT::Application;
    if(!Application::Cocoa::onOpenFile)
        return NO;
    
    Application::Cocoa::onOpenFile(std::string([filename UTF8String]));
    return YES;
}

-(BOOL) applicationShouldHandleReopen:(NSApplication*)application hasVisibleWindows:(BOOL)flag {
    
    using GUIKIT::Application;
    if (Application::Cocoa::onDock) {
        Application::Cocoa::onDock();
        return YES;
    }
    return NO;
}

-(void) run:(NSTimer*)timer {
    using GUIKIT::Application;
    if(!Application::isQuit) Application::loop();    
}

@end

@implementation CocoaWindow : NSWindow

-(id) initWith:(GUIKIT::Window&)windowReference {
    window = &windowReference;

    NSUInteger style = NSTitledWindowMask | NSClosableWindowMask | NSMiniaturizableWindowMask;
    if(window->resizable()) style |= NSResizableWindowMask;

    GUIKIT::Geometry geo = window->state.geometry;

    if(self = [super initWithContentRect:NSMakeRect(geo.x, geo.y, geo.width, geo.height) styleMask:style backing:NSBackingStoreBuffered defer:YES]) {
        [self setDelegate:self];
        [self setReleasedWhenClosed:NO];
        [self setTitle:@""];
        [self setColorSpace: [NSColorSpace deviceRGBColorSpace]];

        NSBundle* bundle = [NSBundle mainBundle];
        NSDictionary* dictionary = [bundle infoDictionary];
        NSString* applicationName = [dictionary objectForKey:@"CFBundleDisplayName"];

        menuBar = [[NSMenu alloc] init];
        menuBarContext = [[NSMenu alloc] init];
        NSMenuItem* item;

        NSMenu* appMenu = [[NSMenu alloc] init];
        item = [[[NSMenuItem alloc] initWithTitle:applicationName action:nil keyEquivalent:@""] autorelease]; //app menu
        [item setSubmenu:appMenu];
        [menuBar addItem:item];

        item = [[[NSMenuItem alloc] initWithTitle:[NSString stringWithFormat:@"About %@", applicationName] action:@selector(menuAbout) keyEquivalent:@""] autorelease];
        [item setTarget:self];
        [appMenu addItem:item];
        [appMenu addItem:[NSMenuItem separatorItem]];

        item = [[[NSMenuItem alloc] initWithTitle:@"Preferences" action:@selector(menuPreferences) keyEquivalent:@","] autorelease];
        [item setTarget:self];
        [appMenu addItem:item];
		
		item = [[[NSMenuItem alloc] initWithTitle:@"Custom1" action:@selector(menuCustom1) keyEquivalent:@""] autorelease];
        [item setTarget:self];
        [appMenu addItem:item];
		
        [appMenu addItem:[NSMenuItem separatorItem]];

        item = [[[NSMenuItem alloc] initWithTitle:[NSString stringWithFormat:@"Hide %@", applicationName] action:@selector(hide:) keyEquivalent:@"h"] autorelease];
        [item setTarget:NSApp];
        [appMenu addItem:item];

        item = [[[NSMenuItem alloc] initWithTitle:@"Hide Others" action:@selector(hideOtherApplications:) keyEquivalent:@"h"] autorelease];
        [item setTarget:NSApp];
        [appMenu addItem:item];

        [item setKeyEquivalentModifierMask: NSAlternateKeyMask | NSCommandKeyMask];

        item = [[[NSMenuItem alloc] initWithTitle:@"Show All" action:@selector(unhideAllApplications:) keyEquivalent:@""] autorelease];
        [item setTarget:NSApp];
        [appMenu addItem:item];
        [appMenu addItem:[NSMenuItem separatorItem]];

        item = [[[NSMenuItem alloc] initWithTitle:[NSString stringWithFormat:@"Quit %@", applicationName] action:@selector(menuQuit) keyEquivalent:@"q"] autorelease];
        [item setTarget:self];
        [appMenu addItem:item];

        statusBar = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, 0, 0)];
        [statusBar setAlignment:NSLeftTextAlignment];
        [statusBar setBordered:NO];
        [statusBar setBezeled:NO];
        //[statusBar setBezelStyle:NSTextFieldSquareBezel];
        [statusBar setEditable:NO];
        [statusBar setHidden:YES];

        [[self contentView] addSubview:statusBar positioned:NSWindowBelow relativeTo:nil];
    }
    return self;
}

-(void)sendEvent:(NSEvent*)event {
    if([event type] == NSRightMouseDown) {
        if (window->onContext) {
            if (window->onContext() && window->state.menus.size() > 0) {
              //  [[menuBar itemAtIndex:0] setHidden: TRUE];
                [NSMenu popUpContextMenu:menuBarContext withEvent:event forView:NULL];
                //[[menuBar itemAtIndex:0] setHidden: FALSE];
                [self resetCursorRects];
            }
        }
    }
    [super sendEvent:event];
}

-(void) keyDown:(NSEvent*)event {
    // that isn't useless, without it a system sound is triggered by each key down event
}

-(BOOL) canBecomeKeyWindow {
    return YES;
}

-(BOOL) canBecomeMainWindow {
    return YES;
}

-(void) windowDidBecomeMain:(NSNotification*)notification {
    if (window->state.menus.size() > 0)
        [NSApp setMainMenu:menuBar];
}

-(void) windowDidMove:(NSNotification*)notification {
    window->p.moveEvent();
}

-(void) windowDidResize:(NSNotification*)notification {
    window->p.sizeEvent();
}

-(void) windowWillEnterFullScreen:(NSNotification*)notification {
    window->state.fullScreen = true;
    window->p.fullScreenToggleDelay = true;
}

-(void) windowDidEnterFullScreen:(NSNotification*)notification {
    window->p.fullScreenToggleDelay = false;
}

-(void) windowWillExitFullScreen:(NSNotification*)notification {
    window->state.fullScreen = false;
    window->p.fullScreenToggleDelay = true;
}

-(void) windowDidExitFullScreen:(NSNotification*)notification {    
    window->p.setGeometry( window->state.geometry );
    if(window->onSize) window->onSize();
    window->p.fullScreenToggleDelay = false;
}

-(BOOL) windowShouldClose:(id)sender {
    if(window->onClose) window->onClose();
    else window->setVisible(false);
    return NO;
}

-(NSDragOperation) draggingEntered:(id<NSDraggingInfo>)sender {
    return GUIKIT::DropPathsOperation(sender);
}

-(BOOL) performDragOperation:(id<NSDraggingInfo>)sender {
    auto paths = GUIKIT::getDropPaths(sender);
    if(paths.empty()) return NO;
    if(window->onDrop) window->onDrop(paths);
    return YES;
}

-(NSMenu*) menuBar {
    return menuBar;
}

-(NSMenu*) menuBarContext {
    return menuBarContext;
}

-(void) menuAbout {
    using GUIKIT::Application;
    if(Application::Cocoa::onAbout) Application::Cocoa::onAbout();
}

-(void) menuPreferences {
    using GUIKIT::Application;
    if(Application::Cocoa::onPreferences) Application::Cocoa::onPreferences();
}

-(void) menuCustom1 {
    using GUIKIT::Application;
    if(Application::Cocoa::onCustom1) Application::Cocoa::onCustom1();
}

-(void) menuQuit {
    using GUIKIT::Application;
    if(Application::Cocoa::onQuit) Application::Cocoa::onQuit();
}

-(NSTextField*) statusBar {
    return statusBar;
}

-(void) resetCursorRects {
    // delegate command to included views (viewport widget)
    // couldn't find out how to set custom cursor for NSWindow
    
    [super resetCursorRects];
}

@end

namespace GUIKIT {

CocoaDelegate* pApplication::cocoaDelegate = nullptr;
    NSTimer* appTimer = nullptr;

auto pApplication::run() -> void {
    if(Application::loop) {
        appTimer = [NSTimer scheduledTimerWithTimeInterval:0.0 target:cocoaDelegate selector:@selector(run:) userInfo:nil repeats:YES];
        
        // prevent blocking while menu popups
        [[NSRunLoop currentRunLoop] addTimer:appTimer forMode:NSEventTrackingRunLoopMode];
    }

    @autoreleasepool {
        [NSApp run];
    }
}

auto pApplication::processEvents() -> void {
    @autoreleasepool {
        while(!Application::isQuit) {
            NSEvent* event = [NSApp nextEventMatchingMask:NSAnyEventMask untilDate:[NSDate distantPast] inMode:NSDefaultRunLoopMode dequeue:YES];
            if(event == nil) break;
            [event retain];
            [NSApp sendEvent:event];
            [event release];
        }
    }
}

auto pApplication::quit() -> void {
    @autoreleasepool {
        [appTimer invalidate];
        [NSApp stop:nil];
        NSEvent* event = [NSEvent otherEventWithType:NSApplicationDefined location:NSMakePoint(0, 0) modifierFlags:0 timestamp:0.0 windowNumber:0 context:nil subtype:0 data1:0 data2:0];
        [NSApp postEvent:event atStart:true];
    }
}

auto pApplication::initialize() -> void {
    @autoreleasepool {
        [NSApplication sharedApplication];
        cocoaDelegate = [[CocoaDelegate alloc] init];
        [NSApp setDelegate:cocoaDelegate];
    }
}

//window
pWindow::pWindow(Window& window) : window(window) {
    @autoreleasepool {
        cocoaWindow = [[CocoaWindow alloc] initWith:window];
        
        static bool once = true;
        
        if (once) {
            once = false;
            [NSApp setMainMenu:[cocoaWindow menuBar]];
        }
    }
}

pWindow::~pWindow() {
    @autoreleasepool {
        [cocoaWindow release];
    }
}

auto pWindow::handle() -> uintptr_t {
    return (uintptr_t)cocoaWindow;
}

auto pWindow::setTitleForAppMenuItem(Window::Cocoa::AppMenuItem appMenuItem, std::string title) -> void {
    [[[[[cocoaWindow menuBar] itemAtIndex:0] submenu] itemAtIndex:appMenuItem] setTitle:[NSString stringWithUTF8String:title.c_str()]];
}
    
auto pWindow::setHiddenForAppMenuItem(Window::Cocoa::AppMenuItem appMenuItem, bool state) -> void {
    [[[[[cocoaWindow menuBar] itemAtIndex:0] submenu] itemAtIndex:appMenuItem] setHidden: state];
}

auto pWindow::setDroppable(bool droppable) -> void {
    @autoreleasepool {
        if(droppable) {
            [cocoaWindow registerForDraggedTypes:[NSArray arrayWithObject:NSFilenamesPboardType]];
        } else {
            [cocoaWindow unregisterDraggedTypes];
        }
    }
}

auto pWindow::setFocused() -> void {
    @autoreleasepool {
        [cocoaWindow makeKeyAndOrderFront:nil];
    }
}

auto pWindow::setVisible(bool visible) -> void {
    @autoreleasepool {
        if(visible) {
            try {
                [cocoaWindow makeKeyAndOrderFront:nil];
                if(!keepMenuVisibility) setMenuVisible(window.menuVisible());
            } catch(...) {
                window.setGeometry({100,100,400,300});
                [cocoaWindow makeKeyAndOrderFront:nil];
                if(!keepMenuVisibility) setMenuVisible(window.menuVisible());
            }
        }
        else [cocoaWindow orderOut:nil];
    }
}
    
auto pWindow::keepMenuVisibilityOnDisplay(bool state) -> void {
    keepMenuVisibility = state;
}


auto pWindow::setResizable(bool resizable) -> void {
    @autoreleasepool {
        NSUInteger style = NSTitledWindowMask | NSClosableWindowMask | NSMiniaturizableWindowMask;
        if(resizable) style |= NSResizableWindowMask;
        [cocoaWindow setStyleMask:style];
    }
}

auto pWindow::setStatusFont(std::string font) -> void {
    @autoreleasepool {
        [[cocoaWindow statusBar] setFont:pFont::cocoaFont(font)];
    }
    statusBarReposition();
}

auto pWindow::setTitle(std::string text) -> void {
    @autoreleasepool {
        [cocoaWindow setTitle:[NSString stringWithUTF8String:text.c_str()]];
    }
}

auto pWindow::setStatusText(std::string text) -> void {
    @autoreleasepool {
        [[cocoaWindow statusBar] setStringValue:[NSString stringWithUTF8String:text.c_str()]];
    }
}

auto pWindow::setStatusVisible(bool visible) -> void {
    @autoreleasepool {
        [[cocoaWindow statusBar] setHidden:!visible];
        setGeometry( !window.fullScreen() ? window.state.geometry : geometry());
    }
}
    
auto pWindow::setMenuVisible(bool visible) -> void {
    @autoreleasepool {
        [NSMenu setMenuBarVisible:visible];
    }
}

auto pWindow::setBackgroundColor(unsigned color) -> void {
    @autoreleasepool {
        [cocoaWindow setBackgroundColor:[NSColor
            colorWithDeviceRed:((color>>16) & 0xff) / 255.0
            green:((color>>8) & 0xff) / 255.0
            blue:(color & 0xff) / 255.0
            alpha: 0.0]
         ];
    }
}

auto pWindow::focused() -> bool {
    @autoreleasepool {
        return [cocoaWindow isMainWindow] == YES;
    }
}

auto pWindow::setGeometry(Geometry geometry) -> void {
    locked = true;

    @autoreleasepool {
        [cocoaWindow
             setFrame:[cocoaWindow
                    frameRectForContentRect:NSMakeRect(
                        geometry.x, pSystem::getDesktopSize().height - statusBarHeight() - geometry.height - geometry.y,
                        geometry.width, geometry.height + statusBarHeight() )
                       ]
        display:YES];

        if(window.state.layout) {
            Geometry layoutGeometry = this->geometry();
            layoutGeometry.x = layoutGeometry.y = 0;
            window.state.layout->setGeometry(layoutGeometry);
        }

        statusBarReposition();
    }
    locked = false;
}

auto pWindow::geometry() -> Geometry {
    @autoreleasepool {
        NSRect area = [cocoaWindow contentRectForFrameRect:[cocoaWindow frame]];

        unsigned height = area.size.height - statusBarHeight();
        int y = pSystem::getDesktopSize().height - area.origin.y - area.size.height;

        return {(int)area.origin.x, y, (unsigned)area.size.width, height};
    }
}

auto pWindow::setFullScreen(bool fullScreen) -> void {
    if (!window.resizable()) return;
    fullScreenToggleDelay = true;
    @autoreleasepool {
        if(fullScreen) {
            [NSApp setPresentationOptions:NSApplicationPresentationFullScreen];
            [cocoaWindow setCollectionBehavior:NSWindowCollectionBehaviorFullScreenPrimary];
            [cocoaWindow toggleFullScreen:nil];

        } else {
            [NSApp setPresentationOptions:NSApplicationPresentationDefault];
            [cocoaWindow setCollectionBehavior:NSWindowCollectionBehaviorDefault];
            locked = true;
            [cocoaWindow toggleFullScreen:nil];
            locked = false;
        }
    }
}

auto pWindow::statusBarHeight() -> unsigned {
    if(!window.statusVisible()) return 0;
    NSFont* font = [[cocoaWindow statusBar] font];
    return pFont::size(font, " ").height + 2;
}

auto pWindow::statusBarReposition() -> void {
    @autoreleasepool {
        NSRect area = [cocoaWindow contentRectForFrameRect:[cocoaWindow frame]];
        [[cocoaWindow statusBar] setFrame:NSMakeRect(0, 0, area.size.width, statusBarHeight())];
        [[cocoaWindow contentView] setNeedsDisplay:YES];
    }
}

auto pWindow::moveEvent() -> void {
    if(!locked && !window.fullScreen() && !fullScreenToggleDelay && window.visible()) {
        Geometry geometry = this->geometry();
        window.state.geometry.x = geometry.x;
        window.state.geometry.y = geometry.y;
    }

    if(!locked && window.onMove) window.onMove();
}

auto pWindow::sizeEvent() -> void {
    if(!locked && !window.fullScreen() && !fullScreenToggleDelay && window.visible()) {
        Geometry geometry = this->geometry();
        window.state.geometry.width = geometry.width;
        window.state.geometry.height = geometry.height;
    }

    if(window.state.layout) {
        Geometry layoutGeometry = this->geometry();
        layoutGeometry.x = layoutGeometry.y = 0;
        window.state.layout->setGeometry(layoutGeometry);
    }

    statusBarReposition();

    if(!locked && window.onSize) window.onSize();
}

auto pWindow::append(Menu& menu) -> void {    
    
    @autoreleasepool {
        if (disableIconsInTopMenu)
            [menu.p.cocoaBase setImage:nil];
        
        [[cocoaWindow menuBar] addItem:menu.p.cocoaBase];
        [[cocoaWindow menuBarContext] addItem:menu.p.cocoaBaseContext];
    }
}

auto pWindow::remove(Menu& menu) -> void {
    @autoreleasepool {
        [[cocoaWindow menuBar] removeItem:menu.p.cocoaBase];
        [[cocoaWindow menuBarContext] removeItem:menu.p.cocoaBaseContext];
    }
}

auto pWindow::append(Widget& widget) -> void {
    @autoreleasepool {
        [widget.p.cocoaView removeFromSuperview];
        [[cocoaWindow contentView] addSubview:widget.p.cocoaView positioned:NSWindowAbove relativeTo:nil];
        widget.p.add();
        [[cocoaWindow contentView] setNeedsDisplay:YES];
    }
}

auto pWindow::remove(Widget& widget) -> void {
    @autoreleasepool {
        [widget.p.cocoaView removeFromSuperview];
        [[cocoaWindow contentView] setNeedsDisplay:YES];
    }
}

auto pWindow::append(Layout& layout) -> void {
    Geometry geometry = window.state.geometry;
    geometry.x = geometry.y = 0;
    layout.setGeometry(geometry);

    statusBarReposition();
}

auto pWindow::remove(Layout& layout) -> void {
    @autoreleasepool {
        [[cocoaWindow contentView] setNeedsDisplay:YES];
    }
}

auto pWindow::addCustomFont(CustomFont* customFont) -> bool {
    return pFont::add( customFont );
}

auto pWindow::changeCursor( Image& image, unsigned hotSpotX, unsigned hotSpotY ) -> void {

    if (image.empty()) {

        setDefaultCursor();

        return;
    } 
    
    @autoreleasepool {
    
        if (customCursor)
            [customCursor release];

        auto nsImage = NSMakeImage( image );

        customCursor = [[NSCursor alloc] initWithImage:nsImage hotSpot:NSMakePoint( hotSpotX, hotSpotY)];
        
        [nsImage release];
        
        [cocoaWindow resetCursorRects];
    }
}

auto pWindow::setDefaultCursor() -> void {
        
    @autoreleasepool {
        
        if (customCursor)
            [customCursor release];
        
        customCursor = nullptr;
        
        [cocoaWindow resetCursorRects];
    }
}
    
}
