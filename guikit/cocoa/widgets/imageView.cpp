

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
    
    if(isDown) {
        switch([event buttonNumber]) {
            case 0:
                if(imageView->onClick)
                    imageView->onClick();
                    
                if (!imageView->state.uri.empty()) {
                    [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString: [NSString stringWithUTF8String:imageView->state.uri.c_str()]]];
                }
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
        @autoreleasepool {
            if (surface) {
                [(id)cocoaView setImage:nil];
                [surface release];
                surface = nil;
            }
            
            if (imageView.image() && !imageView.image()->empty()) {
                surface = NSMakeImage(*imageView.state.image);
                [(id)cocoaView setImage : surface];
            }
        }
    }
    
}
