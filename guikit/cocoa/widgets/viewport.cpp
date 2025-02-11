@implementation CocoaViewport : NSView

-(id) initWith:(GUIKIT::Viewport&)viewportReference {
    if(self = [super initWithFrame:NSMakeRect(0, 0, 0, 0)]) {
        viewport = &viewportReference;
    }
    return self;
}

-(void) drawRect:(NSRect)rect {
    [[NSColor blackColor] setFill];
    NSRectFillUsingOperation(rect, NSCompositingOperationSourceOver);
}

-(BOOL) acceptsFirstResponder {
    return YES;
}

-(NSDragOperation) draggingEntered:(id<NSDraggingInfo>)sender {
    auto paths = GUIKIT::getDropPaths(sender);
    if (!paths.empty() && viewport->onDragEnter)
        viewport->onDragEnter(paths);
        
    return GUIKIT::DropPathsOperation(sender);
}

-(void) draggingEnded:(id<NSDraggingInfo>)sender {
    if(viewport->onDragLeave)
        viewport->onDragLeave();
}

-(BOOL) wantsPeriodicDraggingUpdates {
    return YES;
}

-(NSDragOperation) draggingUpdated:(id<NSDraggingInfo>)sender {
    auto mp = [sender draggingLocation];
    GUIKIT::Geometry geo = viewport->GUIKIT::Widget::state.geometry;
    
    if (viewport->onDragMove)
        viewport->onDragMove(floor(mp.x), geo.height - ceil(mp.y));
    
    return GUIKIT::DropPathsOperation(sender);
}

-(BOOL) performDragOperation:(id<NSDraggingInfo>)sender {
    auto paths = GUIKIT::getDropPaths(sender);
    if(paths.empty()) return NO;
    if(viewport->onDrop) viewport->onDrop(paths);
    return YES;
}

-(void) mouseDown:(NSEvent*)event {
    if(viewport->onMousePress) viewport->onMousePress(GUIKIT::Mouse::Button::Left);
}

-(void) mouseUp:(NSEvent*)event {
    if(viewport->onMouseRelease) viewport->onMouseRelease(GUIKIT::Mouse::Button::Left);
}

-(void) rightMouseDown:(NSEvent*)event {
    if(viewport->onMousePress) viewport->onMousePress(GUIKIT::Mouse::Button::Right);
}

-(void) rightMouseUp:(NSEvent*)event {
    if(viewport->onMouseRelease) viewport->onMouseRelease(GUIKIT::Mouse::Button::Right);
}

-(void) otherMouseDown:(NSEvent*)event {
    if(viewport->onMousePress) viewport->onMousePress(GUIKIT::Mouse::Button::Middle);
}

-(void) otherMouseUp:(NSEvent*)event {
    if(viewport->onMouseRelease) viewport->onMouseRelease(GUIKIT::Mouse::Button::Middle);
}

-(void) mouseMoved:(NSEvent*)event {
    NSPoint mouseLoc;
    if (GUIKIT::Application::isQuit)
        return;
    mouseLoc = [self convertPoint:[event locationInWindow] fromView:nil];
    GUIKIT::Geometry geo = viewport->GUIKIT::Widget::state.geometry;
    auto& _pos = viewport->state.mousePos;
    _pos.y = geo.height - ceil(mouseLoc.y);
    _pos.x = floor(mouseLoc.x);
    
    if(viewport->onMouseMove)
        viewport->onMouseMove(viewport->state.mousePos);
    
    auto _window = viewport->window();
    auto& _timer = viewport->p.cursorHideTimer;
    
    if (_window && !_window->fullScreen() && _timer.interval())
        _timer.setEnabled();
    
    if (_window && (_window->cursor == GUIKIT::Window::Cursor::Blank)) {
        [NSCursor unhide];
        _window->cursor = GUIKIT::Window::Cursor::Default;
    }
}

-(void) mouseExited:(NSEvent*)event {
    if(viewport->onMouseLeave) viewport->onMouseLeave();
    viewport->p.cursorHideTimer.setEnabled(false);
   // printf("exit");
  //  fflush(stdout);
    auto _window = viewport->window();
    if (_window && (_window->cursor == GUIKIT::Window::Cursor::Blank)) {
        [NSCursor unhide];
        _window->cursor = GUIKIT::Window::Cursor::Default;
    }
}

//-(void) mouseEntered:(NSEvent*)event {
  //  printf("enter");
  //  fflush(stdout);
  //  auto _window = viewport->window();
    //if (_window && (_window->cursor == GUIKIT::Window::Cursor::Blank)) {
      //  [NSCursor unhide];
        //_window->cursor = GUIKIT::Window::Cursor::Default;
    //}
//}

-(void) updateTrackingAreas {
    if(trackingArea != nil) {
        [self removeTrackingArea:trackingArea];
        [trackingArea release];
    }
    
    int opts = (NSTrackingMouseEnteredAndExited | NSTrackingMouseMoved | NSTrackingActiveAlways);
    trackingArea = [ [NSTrackingArea alloc] initWithRect:[self bounds] options:opts owner:self userInfo:nil];
    [self addTrackingArea:trackingArea];
}

-(void) resetCursorRects {
    [self discardCursorRects];

    if (!viewport->Sizable::state.window)
        return;
    
    if (viewport->Sizable::state.window->p.customCursor)
        [self addCursorRect: [self bounds] cursor: viewport->Sizable::state.window->p.customCursor];
    else {
        if (viewport->Sizable::state.window->cursor == GUIKIT::Window::Cursor::Pointer)
            [self addCursorRect: [self bounds] cursor: [NSCursor pointingHandCursor]];
    }
}

@end

namespace GUIKIT {

auto pViewport::init() -> void {
    @autoreleasepool {
        cocoaView = [[CocoaViewport alloc] initWith:viewport];
    }

    cursorHideTimer.onFinished = [this]() {
        cursorHideTimer.setEnabled(false);
        if (viewport.window() && (viewport.window()->cursor == Window::Cursor::Default) && !viewport.window()->fullScreen()) {
            [NSCursor hide];
            viewport.window()->cursor =  Window::Cursor::Blank;
        }
    };
}

auto pViewport::hideCursorByInactivity(unsigned delayMS) -> void {
    cursorHideTimer.setInterval(delayMS);
}

auto pViewport::setDroppable(bool droppable) -> void {
    @autoreleasepool {
        if(droppable) {
            [cocoaView registerForDraggedTypes:[NSArray arrayWithObject:NSFilenamesPboardType]];
        } else {
            [cocoaView unregisterDraggedTypes];
        }
    }
}

auto pViewport::handle(bool hintRecreation) -> uintptr_t {
    return (uintptr_t)cocoaView;
}

}
