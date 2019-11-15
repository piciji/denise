
@implementation CocoaSquareCanvas : NSImageView

-(id) initWith:(GUIKIT::SquareCanvas&)squareCanvasReference {
    if(self = [super initWithFrame:NSMakeRect(0, 0, 0, 0)]) {
        squareCanvas = &squareCanvasReference;
        [self setEditable:NO]; 
    
        NSTrackingArea* area = [[[NSTrackingArea alloc] initWithRect:[self bounds]
          options: NSTrackingMouseEnteredAndExited | NSTrackingMouseMoved | NSTrackingActiveInKeyWindow | NSTrackingInVisibleRect
          owner:self userInfo:nil
        ] autorelease];
        [self addTrackingArea:area];
    }
    return self;
}

-(void) mouseButton:(NSEvent*)event down:(BOOL)isDown {
    
    if(isDown) {
        switch([event buttonNumber]) {
            case 0: return squareCanvas->onMousePress(GUIKIT::Mouse::Button::Left);
            case 1: return squareCanvas->onMousePress(GUIKIT::Mouse::Button::Middle);
            case 2: return squareCanvas->onMousePress(GUIKIT::Mouse::Button::Right);
        }
    } else {
        switch([event buttonNumber]) {
            case 0: return squareCanvas->onMouseRelease(GUIKIT::Mouse::Button::Left);
            case 1: return squareCanvas->onMouseRelease(GUIKIT::Mouse::Button::Middle);
            case 2: return squareCanvas->onMouseRelease(GUIKIT::Mouse::Button::Right);
        }
    }
}

-(void) mouseDown:(NSEvent*)event {
    [self mouseButton:event down:YES];
}

-(void) mouseUp:(NSEvent*)event {
    [self mouseButton:event down:NO];
}

-(void) rightMouseDown:(NSEvent*)event {
    [self mouseButton:event down:YES];
}

-(void) rightMouseUp:(NSEvent*)event {
    [self mouseButton:event down:NO];
}

-(void) otherMouseDown:(NSEvent*)event {
    [self mouseButton:event down:YES];
}

-(void) otherMouseUp:(NSEvent*)event {
    [self mouseButton:event down:NO];
}


@end
        
namespace GUIKIT {           

auto pSquareCanvas::setGeometry(Geometry geometry) -> void {

    redraw();
    pWidget::setGeometry(geometry);
}

auto pSquareCanvas::setBackgroundColor(unsigned color) -> void {
    redraw();
}

auto pSquareCanvas::setBorderColor(unsigned borderSize, unsigned borderColor) -> void {
    redraw();
}    
    
auto pSquareCanvas::init() -> void {
    @autoreleasepool {
        cocoaView = [[CocoaSquareCanvas alloc] initWith:squareCanvas];
    }
}

auto pSquareCanvas::redraw() -> void {
    unsigned width = squareCanvas.Widget::state.geometry.width;
    unsigned height = squareCanvas.Widget::state.geometry.height;
    unsigned color = squareCanvas.Widget::state.backgroundColor;
    unsigned borderColor = squareCanvas.state.borderColor;
    unsigned borderSize = squareCanvas.state.borderSize;
    
    if (width == 0 || height == 0)
        return;
    
    uint8_t r = (color >> 16) & 0xff;
    uint8_t g = (color >> 8) & 0xff;
    uint8_t b = (color >> 0) & 0xff;

    uint8_t bR = (borderColor >> 16) & 0xff;
    uint8_t bG = (borderColor >> 8) & 0xff;
    uint8_t bB = (borderColor >> 0) & 0xff;

    @autoreleasepool {
        if (surface) {
            [cocoaView setImage:nil];
            [surface release];
            [bitmap release];
        }
        surface = [[NSImage alloc] initWithSize : NSMakeSize(width, height)];
        NSBitmapImageRep* bitmap = [[NSBitmapImageRep alloc]
                initWithBitmapDataPlanes: nil
                pixelsWide: width
                pixelsHigh: height
                bitsPerSample: 8
                samplesPerPixel: 4
                hasAlpha: YES
                isPlanar: NO
                colorSpaceName: NSCalibratedRGBColorSpace
                bitmapFormat: NSAlphaNonpremultipliedBitmapFormat
                bytesPerRow: (width * 4)
                bitsPerPixel: 32
                ];
        
        [surface addRepresentation : bitmap];
        
        [cocoaView setImage : surface];

        auto target = (uint32_t*)[bitmap bitmapData];
    
        for (unsigned y = 0; y < height; y++) {
            bool borderYPixel = 0;
            if (y < borderSize)
                borderYPixel = 1;
            else if (y >= (height - borderSize))
                borderYPixel = 1;

            for (unsigned x = 0; x < width; x++) {
                bool borderXPixel = 0;

                if (!borderYPixel) {
                    if (x < borderSize)
                        borderXPixel = 1;
                    else if (x >= (width - borderSize))
                        borderXPixel = 1;
                }

                if (borderYPixel || borderXPixel)            
                    *target++ = 0xff << 24 | bB << 16 | bG << 8 | bR;
                else
                    *target++ = 0xff << 24 | b << 16 | g << 8 | r;
            }
        }
    }
}

}        
