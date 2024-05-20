

@implementation CocoaMultilineEdit : NSScrollView

-(id) initWith:(GUIKIT::MultilineEdit&)multilineEditReference {
    if(self = [super initWithFrame:NSMakeRect(0, 0, 0, 0)]) {
        multilineEdit = &multilineEditReference;
        
        content = [(CocoaTextView*)[CocoaTextView alloc] initWith: multilineEditReference];
        
       // [self setDelegate:self];
        [self setDocumentView:content];
        [self configure];
    }
    return self;
}

-(CocoaTextView*) content {
    return content;
}

-(void) configure {
    [content setMinSize:NSMakeSize(0,0)];
    //[content setMaxSize:NSMakeSize:(FLT_MAX, FLT_MAX)];
    
    [[content textContainer] setContainerSize:NSMakeSize(FLT_MAX, FLT_MAX)];
    [[content textContainer] setWidthTracksTextView:YES];
    
    [content setHorizontallyResizable:YES];
    [content setVerticallyResizable:YES];
    [content setAutoresizingMask:NSViewNotSizable];
    
    [self setHasVerticalScroller:YES];
    [self setHasHorizontalScroller:YES];
}

/*-(BOOL) textDidBeginEditing:(NSText*)text {
    
    if (multilineEdit->editable())
        return YES;
    
    return NO;
}*/


@end

@implementation CocoaTextView : NSTextView

-(id) initWith:(GUIKIT::MultilineEdit&)multilineEditReference {
    if(self = [super initWithFrame:NSMakeRect(0, 0, 0, 0)]) {
        multilineEdit = &multilineEditReference;
        
        NSMutableParagraphStyle* myStyle = [[NSMutableParagraphStyle alloc] init];
        [myStyle setLineSpacing:5.0];
        [self setDefaultParagraphStyle:myStyle];
        
        [self setDelegate:self];
        [self setRichText:NO];
    }
    return self;
}

-(void) textDidChange:(NSNotification*)notification {
    
    if(multilineEdit->onChange) multilineEdit->onChange();
}

-(BOOL) becomeFirstResponder {
    BOOL result = [multilineEdit->window()->p.cocoaWindow becomeFirstResponder];
    if(multilineEdit->onFocus) multilineEdit->onFocus();
    return result;
}

@end

namespace GUIKIT {
    
auto pMultilineEdit::setEditable(bool editable) -> void {
    
    @autoreleasepool{
        [[(id)cocoaView content] setEditable : editable];
    }
}

auto pMultilineEdit::text() -> std::string {
    @autoreleasepool{
        return [[[(id)cocoaView content] stringValue] UTF8String];
    }
}

auto pMultilineEdit::setText(const std::string& text) -> void {
    @autoreleasepool{
        [[(id)cocoaView content] setString: [NSString stringWithUTF8String: text.c_str()]];
        [(id)cocoaView configure];
    }
    calculatedMinimumSize.updated = false;
}

auto pMultilineEdit::init() -> void {
    @autoreleasepool {
        cocoaView = [[CocoaMultilineEdit alloc] initWith : multilineEdit];
        setEditable(multilineEdit.editable());
    }
}

auto pMultilineEdit::setForegroundColor(unsigned color) -> void {
    @autoreleasepool {
        NSColor* textColor = [NSColor textColor];
        
        if(multilineEdit.overrideForegroundColor()) {
            unsigned color = multilineEdit.foregroundColor();
            textColor = pHelper::getColor( color );
        }
        
        [[(id)cocoaView content] setTextColor: textColor];
    }
}
    
auto pMultilineEdit::setGeometry(Geometry geometry) -> void {
    
    pWidget::setGeometry(geometry);
    [(id)cocoaView configure];
}
    
auto pMultilineEdit::setFont(std::string font) -> void {
    @autoreleasepool {
        NSFont* nsfont = pFont::cocoaFont(font);
        if (nsfont != nil)
            [[(id)cocoaView content] setFont:nsfont];
    }
    calculatedMinimumSize.updated = false;
}
    
}
