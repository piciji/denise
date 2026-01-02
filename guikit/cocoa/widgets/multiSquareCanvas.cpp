
@implementation CocoaMultiSquareCanvas : NSImageView

-(id) initWith:(GUIKIT::MultiSquareCanvas&)multiSquareCanvasReference {
    if(self = [super initWithFrame:NSMakeRect(0, 0, 0, 0)]) {
        multiSquareCanvas = &multiSquareCanvasReference;
        [self setEditable:NO];
    }
    return self;
}

- (BOOL)isFlipped
{
    return YES;
}

@end

@implementation CocoaMultiSquareScroll : NSScrollView

-(id) initWith:(GUIKIT::MultiSquareCanvas&)multiSquareCanvasReference {
    if(self = [super initWithFrame:NSMakeRect(0, 0, 0, 0)]) {
        multiSquareCanvas = &multiSquareCanvasReference;
        
        content = [[CocoaMultiSquareCanvas alloc] initWith:multiSquareCanvasReference];
        
     //   [content setAutoresizingMask:NSViewWidthSizable];
        
        [self setDocumentView:content];
        [self setBorderType:NSBezelBorder];
        [self setHasVerticalScroller:YES];
        [self setHasHorizontalScroller:YES];
        if (GUIKIT::hasMinimumVersion(10, 10)) {
       //     [self setAutomaticallyAdjustsContentInsets:NO];
         //   [self setContentInsets:NSEdgeInsetsMake(2, 2, 2, 2)];
        }

      //  [content setTarget:self];
    }
    return self;
}

-(void) dealloc {
    [content release];
    [super dealloc];
}

-(CocoaMultiSquareCanvas*) content {
    return content;
}
@end

namespace GUIKIT {

auto pMultiSquareCanvas::setGeometry(Geometry geometry) -> void {
    update();
    pWidget::setGeometry(geometry);
}

auto pMultiSquareCanvas::setPadding(unsigned padding) -> void {
    update();
}

auto pMultiSquareCanvas::update() -> void {
    buildDrawArea();
    redraw();
}

pMultiSquareCanvas::~pMultiSquareCanvas() {
    delete[] drawArea;
}

auto pMultiSquareCanvas::init() -> void {
    @autoreleasepool {
        cocoaView = [[CocoaMultiSquareScroll alloc] initWith:multiSquareCanvas];
    }
}

auto pMultiSquareCanvas::redraw() -> void {
    unsigned padding = multiSquareCanvas.padding();
    unsigned cols = multiSquareCanvas.cols();
    unsigned rows = multiSquareCanvas.rows();
    unsigned squareSize = multiSquareCanvas.squareSize();
    unsigned width = cols * (squareSize + padding);
    unsigned height = rows * (squareSize + padding);
    
    [[(id)cocoaView content] setFrameSize:NSMakeSize(width, height)];

    
    @autoreleasepool {
        if (surface) {
            [[(id)cocoaView content] setImage:nil];
            [surface release];
            [bitmap release];
            surface = nil;
        }
        
        if (width == 0 || height == 0)
            return;
        
        if (drawArea) {
            surface = [[NSImage alloc] initWithSize : NSMakeSize(width, height)];
            bitmap = [[NSBitmapImageRep alloc]
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
            
            [[(id)cocoaView content] setImage : surface];
            
            auto buffer = (uint8_t*)[bitmap bitmapData];
            
            std::memcpy(buffer, (uint8_t*)(drawArea), width * height * 4);
        }
    }
}
#define DARK_BG_COL         (0x20 << 16 | 0x20 << 8 | 0x20)
#define DARK_BG_SOFTER_COL  (0x38 << 16 | 0x38 << 8 | 0x38)

auto pMultiSquareCanvas::buildDrawArea() -> void {
    unsigned* dots = multiSquareCanvas.getDotPtr();
    delete[] drawArea;
    drawArea = nullptr;

    if (!dots)
        return;

    unsigned cols = multiSquareCanvas.cols();
    unsigned rows = multiSquareCanvas.rows();
    unsigned squareSize = multiSquareCanvas.squareSize();
    unsigned padding = multiSquareCanvas.padding();

    unsigned width = cols * (squareSize + padding);
    unsigned height = rows * (squareSize + padding);

    drawArea = new unsigned[width * height];

    unsigned yPos = 0;
    unsigned* target;

    for (unsigned r = 0; r < rows; r++) {
        unsigned xPos = 0;

        for (unsigned c = 0; c < cols; c++) {
            unsigned color = *dots++;
            if (!color)
                color = DARK_BG_SOFTER_COL | (0xff << 24);
            else {
                uint8_t r = (color >> 16) & 0xff;
                uint8_t b = color & 0xff;
                color &= 0xff00ff00;
                color |= b << 16 | r;
            }

            for (unsigned y = 0; y < squareSize; y++) {
                target = drawArea + (yPos + y) * width + xPos;

                for (unsigned x = 0; x < padding; x++) {
                    *target++ = DARK_BG_COL | (0xff << 24);
                }

                for (unsigned x = 0; x < squareSize; x++) {
                    *target++ = color;
                }
            }

            xPos += squareSize + padding;
        }

        yPos += squareSize;

        for (unsigned y = 0; y < padding; y++) {
            target = drawArea + (yPos + y) * width;

            for (unsigned x = 0; x < width; x++) {
                *target++ = DARK_BG_COL | (0xff << 24);
            }
        }

        yPos += padding;
    }
}

}
