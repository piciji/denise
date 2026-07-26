
@implementation CocoaLineEdit : NSTextField

-(id) initWith:(GUIKIT::LineEdit&)lineEditReference {
    if(self = [super initWithFrame:NSMakeRect(0, 0, 0, 0)]) {
        lineEdit = &lineEditReference;
        
        [self setRefusesFirstResponder:YES];
        [self setDelegate:self];
        [self setTarget:self];
        
        NSTextFieldCell* cell = (NSTextFieldCell*)self.cell;
        cell.wraps = NO;
        cell.scrollable = YES;
        cell.lineBreakMode = NSLineBreakByTruncatingTail;
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
    if (calculatedMinimumSize.updated)
        return calculatedMinimumSize.minimumSize;
    
    NSTextFieldCell* cell = [(NSTextField*)cocoaView cell];
    
    if (!cell || cell.cellSize.width <= 0.0) {
        Size size = getMinimumSize();
        calculatedMinimumSize.minimumSize.width = pFont::size([(id)cocoaView font], widget.text()).width + 14;
        size.width = calculatedMinimumSize.minimumSize.width;
        return {size.width, size.height};
    }
        
    calculatedMinimumSize.updated = true;
    calculatedMinimumSize.minimumSize = {(unsigned)cell.cellSize.width, (unsigned)cell.cellSize.height};
    
    if (!@available(macOS 10.14, *)) {
        if ([[(id)cocoaView font] isFixedPitch])
            calculatedMinimumSize.minimumSize.height += 1;
    }
    
    return calculatedMinimumSize.minimumSize;
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
    
auto pLineEdit::setForegroundColor(unsigned color) -> void {
    [(id)cocoaView setTextColor: getTextColor()];
}

}        
