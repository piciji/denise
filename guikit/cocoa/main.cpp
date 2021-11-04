
#include "main.h"

#include "tools.cpp"
#include "menu.cpp"
#include "browserWindow.cpp"
#include "messageWindow.cpp"
#include "statusbar.cpp"
#include "tooltip.m"
#include "display.cpp"

#include "widgets/widget.cpp"   
#include "widgets/button.cpp"
#include "widgets/stepbutton.cpp"
#include "widgets/lineedit.cpp"
#include "widgets/multilineedit.cpp"
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
#include "widgets/imageView.cpp"

//#import <Foundation/Foundation.h>
#include <Availability.h>
#if(__MAC_OS_X_VERSION_MAX_ALLOWED >= 101500)
    #import <IOKit/hidsystem/IOHIDLib.h>
#endif


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

- (void)beganTracking:(NSNotification*)notification {

    using GUIKIT::pApplication;
    pApplication::setAppTimer();
    
    [[NSRunLoop currentRunLoop] addTimer:pApplication::appTimer forMode:NSEventTrackingRunLoopMode];
}

- (void)endTracking:(NSNotification*)notification {
    
    using GUIKIT::pApplication;
    pApplication::setAppTimer();
    
    [[NSRunLoop currentRunLoop] addTimer:pApplication::appTimer forMode:NSDefaultRunLoopMode];
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
        [self setColorSpace: [NSColorSpace sRGBColorSpace]];

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
        
        [self setAutorecalculatesKeyViewLoop: YES];
    }
    
    if(GUIKIT::Application::loop) {
        GUIKIT::pApplication::oberserveMenu( menuBarContext );
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

-(void)windowDidMiniaturize:(NSNotification*)notification {
    if (window->onMinimize)
        window->onMinimize();
}

-(void)windowDidDeminiaturize:(NSNotification*)notification {
    if (window->onUnminimize)
        window->onUnminimize();
}

-(void) windowWillEnterFullScreen:(NSNotification*)notification {
    window->state.fullScreen = true;
    window->p.fullScreenToggleDelay = true;
}

-(void) windowDidEnterFullScreen:(NSNotification*)notification {
    window->p.fullScreenToggleDelay = false;
    if (!window->menuVisible())
        window->setMenuVisible(true);
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

-(void) resetCursorRects {
    // delegate command to included views (viewport widget)
    // couldn't find out how to set custom cursor for NSWindow
    
    [super resetCursorRects];
}

@end

namespace GUIKIT {

CocoaDelegate* pApplication::cocoaDelegate = nullptr;
NSTimer* pApplication::appTimer = nullptr;

auto pApplication::run() -> void {
    if(Application::loop) {
        setAppTimer();
        oberserveMenu( [NSApp mainMenu] );
    }

    @autoreleasepool {
        [NSApp run];
    }
}

auto pApplication::oberserveMenu(NSMenu* menu) -> void {
    NSNotificationCenter* notificationCenter = [NSNotificationCenter defaultCenter];
    
    [notificationCenter addObserver:cocoaDelegate selector:@selector(beganTracking:) name:NSMenuDidBeginTrackingNotification object:menu];
    
    [notificationCenter addObserver:cocoaDelegate selector:@selector(endTracking:) name:NSMenuDidEndTrackingNotification object:menu];
}
    
auto pApplication::setAppTimer() -> void {
    
    if (appTimer)
        [appTimer invalidate];
    
    appTimer = [NSTimer scheduledTimerWithTimeInterval:0.0 target:cocoaDelegate selector:@selector(run:) userInfo:nil repeats:YES];
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

auto pApplication::requestClipboardText() -> void {
    std::string text;
    
    @autoreleasepool {
        NSPasteboard* pasteBoard = [NSPasteboard generalPasteboard];

        NSString* str = [pasteBoard stringForType:NSPasteboardTypeString];
        
        if (!str)
            return;
        
        text = [str UTF8String];
    }
    if (Application::onClipboardRequest)
        Application::onClipboardRequest( text );
}

auto pApplication::setClipboardText( std::string text ) -> void {
    @autoreleasepool {
        NSPasteboard* pasteBoard = [NSPasteboard generalPasteboard];
        [pasteBoard declareTypes:[NSArray arrayWithObject:NSPasteboardTypeString] owner:nil];

        [pasteBoard clearContents];
        
        [pasteBoard setString:[NSString stringWithUTF8String:text.c_str()] forType:NSPasteboardTypeString];
    }
}
    
auto pApplication::promptKeyboardAccess() -> void {
    #if(__MAC_OS_X_VERSION_MAX_ALLOWED >= 101500)
    //if (@available(macOS 10.15, *)) {
        static const IOHIDRequestType accessType = kIOHIDRequestTypeListenEvent;
        
        bool allowed = kIOHIDAccessTypeGranted == IOHIDCheckAccess(accessType);
        
       // if (!allowed) {
         //   dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0), ^{
                IOHIDRequestAccess(accessType);
           // });
       // }
    //}
    #endif
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
        //[cocoaWindow makeKeyAndOrderFront:nil];
        [cocoaWindow orderFrontRegardless];
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

    auto pWindow::setTitle(std::string text) -> void {
    @autoreleasepool {
        [cocoaWindow setTitle:[NSString stringWithUTF8String:text.c_str()]];
    }
}

auto pWindow::setStatusVisible(bool visible) -> void {
    
    @autoreleasepool {
        if (!window.statusBar())
            return;

        window.statusBar()->p.setVisible( visible );

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
        NSView* _view = [cocoaWindow contentView];
        
        [_view setWantsLayer:YES];
        
        [_view.layer setBackgroundColor:pHelper::getColor(color).CGColor
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
        unsigned statusHeight = 0;
        if (window.statusBar())
            statusHeight = window.statusBar()->p.getHeight();
        
        [cocoaWindow
             setFrame:[cocoaWindow
                    frameRectForContentRect:NSMakeRect(
                        geometry.x, pSystem::getDesktopSize().height - statusHeight - geometry.height - geometry.y,
                        geometry.width, geometry.height + statusHeight )
                       ]
        display:YES];

        if(window.state.layout) {
            window.state.layout->resetSynchronisation();
            Geometry layoutGeometry = this->geometry();
            layoutGeometry.x = layoutGeometry.y = 0;
            window.state.layout->setGeometry(layoutGeometry);
        }

        if (window.statusBar())
            window.statusBar()->p.reposition();
    }
    locked = false;
}

auto pWindow::geometry() -> Geometry {
    @autoreleasepool {
        unsigned statusHeight = 0;
        if (window.statusBar())
            statusHeight = window.statusBar()->p.getHeight();

        NSRect area = [cocoaWindow contentRectForFrameRect:[cocoaWindow frame]];

        unsigned height = area.size.height - statusHeight;
        int y = pSystem::getDesktopSize().height - area.origin.y - area.size.height;

        return {(int)area.origin.x, y, (unsigned)area.size.width, height};
    }
}

auto pWindow::setFullScreen(bool fullScreen) -> void {
    if (!window.resizable()) return;
    fullScreenToggleDelay = true;
    @autoreleasepool {
        if(fullScreen) {
            if (window.fullscreenSetting.inUse)
                pMonitor::setSetting( window.fullscreenSetting.displayId, window.fullscreenSetting.settingId );
            
            [NSApp setPresentationOptions:NSApplicationPresentationFullScreen];
            [cocoaWindow setCollectionBehavior:NSWindowCollectionBehaviorFullScreenPrimary];
            [cocoaWindow toggleFullScreen:nil];

        } else {
            [NSApp setPresentationOptions:NSApplicationPresentationDefault];
            [cocoaWindow setCollectionBehavior:NSWindowCollectionBehaviorDefault];
            locked = true;
            [cocoaWindow toggleFullScreen:nil];
            locked = false;
            pMonitor::resetSetting( );
        }
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
        window.state.layout->resetSynchronisation();
        Geometry layoutGeometry = this->geometry();
        layoutGeometry.x = layoutGeometry.y = 0;
        window.state.layout->setGeometry(layoutGeometry);
    }

    if (window.statusBar())
        window.statusBar()->p.reposition();

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

    if (window.statusBar())
        window.statusBar()->p.reposition();
}

auto pWindow::remove(Layout& layout) -> void {
    @autoreleasepool {
        [[cocoaWindow contentView] setNeedsDisplay:YES];
    }
}

auto pWindow::append(StatusBar& statusBar) -> void {
    statusBar.p.create();
}

auto pWindow::remove(StatusBar& statusBar) -> void {
    statusBar.p.destroy();
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
    
auto pWindow::setPointerCursor() -> void {
    @autoreleasepool {
        
        if (customCursor)
            [customCursor release];
        
        customCursor = nullptr;
        
        //[[NSCursor pointingHandCursor] set];
        
        [cocoaWindow resetCursorRects];
    }
}
 
auto pWindow::minimized() -> bool {
    return [cocoaWindow isMiniaturized];
}
    
auto pWindow::restore() -> void {
    
    if (minimized())
        [cocoaWindow deminiaturize:nil];
}
   
auto pWindow::setForeground() -> void {
    setFocused();
}
    
}

