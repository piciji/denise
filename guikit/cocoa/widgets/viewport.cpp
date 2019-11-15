@implementation CocoaViewport : NSView

-(id) initWith:(GUIKIT::Viewport&)viewportReference {
    if(self = [super initWithFrame:NSMakeRect(0, 0, 0, 0)]) {
        viewport = &viewportReference;
    }
    return self;
}

-(void) drawRect:(NSRect)rect {
    [[NSColor blackColor] setFill];
    NSRectFillUsingOperation(rect, NSCompositeSourceOver);
}

-(BOOL) acceptsFirstResponder {
    return YES;
}

-(NSDragOperation) draggingEntered:(id<NSDraggingInfo>)sender {
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
    mouseLoc = [self convertPoint:[event locationInWindow] fromView:nil];
    GUIKIT::Geometry geo = viewport->GUIKIT::Widget::state.geometry;
    viewport->state.mousePos.y = geo.height - ceil(mouseLoc.y);
    viewport->state.mousePos.x = floor(mouseLoc.x);
    
    if(viewport->onMouseMove) viewport->onMouseMove({viewport->state.mousePos.x, viewport->state.mousePos.y});
}

-(void) mouseExited:(NSEvent*)event {
    if(viewport->onMouseLeave) viewport->onMouseLeave();
}

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

    if (!viewport->Sizable::state.window || !viewport->Sizable::state.window->p.customCursor)
        return;
    
    [self addCursorRect: [self bounds] cursor: viewport->Sizable::state.window->p.customCursor];
}

@end

namespace GUIKIT {

auto pViewport::init() -> void {
    @autoreleasepool {
        cocoaView = [[CocoaViewport alloc] initWith:viewport];
    }
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

auto pViewport::handle() -> uintptr_t {
    return (uintptr_t)cocoaView;
}

}
