
@implementation CocoaLineEdit : NSTextField

-(id) initWith:(GUIKIT::LineEdit&)lineEditReference {
    if(self = [super initWithFrame:NSMakeRect(0, 0, 0, 0)]) {
        lineEdit = &lineEditReference;
        
        [self setRefusesFirstResponder:YES];
        [self setDelegate:self];
        [self setTarget:self];
    }
    return self;
}

-(BOOL) textShouldBeginEditing:(NSText*)text {
    
    if (lineEdit->editable())
        return YES;
        
    return NO;
}

-(void)controlTextDidEndEditing:(NSNotification *)notification {
    if ( [[[notification userInfo] objectForKey:@"NSTextMovement"] intValue] == NSReturnTextMovement ) {
        if (lineEdit->onReturn)
            lineEdit->onReturn();
    }
}

-(void) controlTextDidChange:(NSNotification*)notification {
    NSTextField* textField = [notification object];
    NSString* value = [textField stringValue];
    
    if (lineEdit->maxLength() > 0) {
        if (lineEdit->maxLength() < [value length]) {
                [textField setStringValue:[value substringWithRange:NSMakeRange(0, lineEdit->maxLength() )]];
            return;
        }
    }
    
    if(lineEdit->onChange) lineEdit->onChange();
}

-(BOOL) becomeFirstResponder {
    BOOL result = [super becomeFirstResponder];
    if(lineEdit->onFocus) lineEdit->onFocus();
    return result;
}

-(NSDragOperation) draggingEntered:(id<NSDraggingInfo>)sender {
    return GUIKIT::DropPathsOperation(sender);
}

-(BOOL) performDragOperation:(id<NSDraggingInfo>)sender {
    auto paths = GUIKIT::getDropPaths(sender);
    if(paths.empty()) return NO;
    if(lineEdit->onDrop) lineEdit->onDrop(paths);
    return YES;
}

@end

namespace GUIKIT {

auto pLineEdit::minimumSize() -> Size {
    bool _updated = calculatedMinimumSize.updated;
    
    Size size = getMinimumSize();
    if (!_updated) {
        auto _size = pFont::size([(id)cocoaView font], widget.text());
        calculatedMinimumSize.minimumSize.width += _size.width;
        size.width = calculatedMinimumSize.minimumSize.width;
    }
    
    return {size.width + 10, size.height + 0};
}

auto pLineEdit::setEditable(bool editable) -> void {
    // unlike the other OS's a cocoa lineEdit can not get focus when
    // seted not editable.
    // fortunately there is an event which requests permission to begin editing.
    
    //@autoreleasepool{
       // [cocoaView setEditable : editable];
    //}
}

auto pLineEdit::setDroppable(bool droppable) -> void {
    @autoreleasepool {
        if(droppable) {
            [cocoaView registerForDraggedTypes:@[NSPasteboardTypeFileURL]];
        } else {
            [cocoaView unregisterDraggedTypes];
        }
    }
}

auto pLineEdit::setPlaceholder(const std::string& placeholder) -> void {
    @autoreleasepool {
        [(id)cocoaView setPlaceholderString: [NSString stringWithUTF8String : placeholder.c_str()]];
    }
}

auto pLineEdit::setAlign( LineEdit::Align align ) -> void {
    @autoreleasepool {
        [(id)cocoaView setAlignment: align == LineEdit::Align::Right ? NSTextAlignmentRight : NSTextAlignmentLeft ];
    }
}

auto pLineEdit::text() -> std::string {
    @autoreleasepool{
        return [[(id)cocoaView stringValue] UTF8String];
    }
}

auto pLineEdit::setText(const std::string& text) -> void {
    @autoreleasepool{
        [(id)cocoaView setStringValue : [NSString stringWithUTF8String : text.c_str()]];
    }
    calculatedMinimumSize.updated = false;
}

auto pLineEdit::init() -> void {
    @autoreleasepool{
        cocoaView = [[CocoaLineEdit alloc] initWith : lineEdit];
        setEditable(lineEdit.editable());
    }
}

}        
