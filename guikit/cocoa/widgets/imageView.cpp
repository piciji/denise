

@implementation CocoaImageView : NSImageView

-(id) initWith:(GUIKIT::ImageView&)imageViewReference {
    if(self = [super initWithFrame:NSMakeRect(0, 0, 0, 0)]) {
        imageView = &imageViewReference;
        [self setEditable:NO];
    }
    return self;
}

- (void)resetCursorRects {
    [self addCursorRect:[self bounds] cursor:[NSCursor pointingHandCursor]];
}

-(void) mouseButton:(NSEvent*)event down:(BOOL)isDown {
    
    if(isDown && !imageView->state.uri.empty()) {
        switch([event buttonNumber]) {
            case 0:
                [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString: [NSString stringWithUTF8String:imageView->state.uri.c_str()]]];
                break;
        }
    }
}

-(void) mouseDown:(NSEvent*)event {
    [self mouseButton:event down:YES];
}

-(void) mouseUp:(NSEvent*)event {
    [self mouseButton:event down:NO];
}

@end

namespace GUIKIT {
    
    auto pImageView::minimumSize() -> Size {
        if (!imageView.state.image)
            return {0u, 0u};
        
        auto image = imageView.state.image;
        
        return {image->width, image->height};
    }
    
    auto pImageView::setGeometry(Geometry geometry) -> void {
        
       // redraw();
        pWidget::setGeometry(geometry);
    }
    
    
    auto pImageView::setImage(Image* image) -> void {
        redraw();
    }
    
    auto pImageView::init() -> void {
        @autoreleasepool {
            cocoaView = [[CocoaImageView alloc] initWith:imageView];
        }
    }
    
    auto pImageView::redraw() -> void {
        unsigned width = imageView.state.image->width;
        unsigned height = imageView.state.image->height;

        if (width == 0 || height == 0)
            return;
        
        @autoreleasepool {
            if (surface) {
                [cocoaView setImage:nil];
                [surface release];
            }
            
            surface = NSMakeImage(*imageView.state.image);
            
            [cocoaView setImage : surface];
        }
    }
    
}
