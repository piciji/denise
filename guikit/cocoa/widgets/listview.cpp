
@implementation CocoaListViewContent : NSTableView

-(void) keyDown:(NSEvent*)event {
    auto character = [[event characters] characterAtIndex:0];
    if(character == NSEnterCharacter || character == NSCarriageReturnCharacter) {
        if([self selectedRow] >= 0) {
            [[self delegate] activate:self];
            return;
        }
    }
    [super keyDown:event];
}
@end

@implementation CocoaListViewCell : NSTextFieldCell

//used by type-ahead
-(NSString*) stringValue {
    return [[self objectValue] objectForKey:@"text"];
}

-(void) drawWithFrame:(NSRect)frame inView:(NSView*)view {
    NSString* text = [[self objectValue] objectForKey:@"text"];
    NSImage* image = [[self objectValue] objectForKey:@"image"];
    unsigned textDisplacement = 0;
    
    if(image) {
        [[NSGraphicsContext currentContext] saveGraphicsState];
        
        NSRect targetRect = NSMakeRect(frame.origin.x, frame.origin.y, frame.size.height, frame.size.height);
        NSRect sourceRect = NSMakeRect(0, 0, [image size].width, [image size].height);
        [image drawInRect:targetRect fromRect:sourceRect operation:NSCompositeSourceOver fraction:1.0 respectFlipped:YES hints:nil];
        
        [[NSGraphicsContext currentContext] restoreGraphicsState];
        textDisplacement = frame.size.height + 2;
    }
    
    NSRect textRect = NSMakeRect(
        frame.origin.x + textDisplacement, frame.origin.y,
        frame.size.width - textDisplacement, frame.size.height);
    
    NSColor* textColor = [self isHighlighted] ? [NSColor alternateSelectedControlTextColor] : [NSColor textColor];
    
    if(listView->overrideForegroundColor()) {
        unsigned color = listView->foregroundColor();
        textColor = [NSColor
                     colorWithCalibratedRed:((color>>16) & 0xff) / 255.0
                     green:((color>>8) & 0xff) / 255.0
                     blue:(color & 0xff) / 255.0
                     alpha: 1.0];
    }
    
    [text drawInRect:textRect withAttributes:@{ NSForegroundColorAttributeName:textColor, NSFontAttributeName:[self font] }];
}
@end

@implementation CocoaListView : NSScrollView

-(id) initWith:(GUIKIT::ListView&)listViewReference {
    if(self = [super initWithFrame:NSMakeRect(0, 0, 0, 0)]) {
        listView = &listViewReference;
        content = [[CocoaListViewContent alloc] initWithFrame:NSMakeRect(0, 0, 0, 0)];
        
        [self setDocumentView:content];
        [self setBorderType:NSBezelBorder];
        [self setHasVerticalScroller:YES];
        
        [content setDataSource:self];
        [content setDelegate:self];
        [content setTarget:self];
        [content setDoubleAction:@selector(doubleAction:)];
        
        [content setAllowsColumnReordering:NO];
        [content setAllowsColumnResizing:YES];
        [content setAllowsColumnSelection:NO];
        [content setAllowsEmptySelection:YES];
        [content setAllowsMultipleSelection:NO];
        [content setColumnAutoresizingStyle:NSTableViewLastColumnOnlyAutoresizingStyle];
        
        font = nil;
        [self setFont:nil];
    }
    return self;
}

-(void) dealloc {
    [content release];
    [font release];
    [super dealloc];
}

-(CocoaListViewContent*) content {
    return content;
}

-(NSFont*) font {
    return font;
}

-(void) setFont:(NSFont*)fontPointer {
    if(!fontPointer) fontPointer = [NSFont systemFontOfSize:12];
    [fontPointer retain];
    if(font) [font release];
    font = fontPointer;
    
    unsigned fontHeight = GUIKIT::pFont::size(font, " ").height;
    [content setFont:font];
    [content setRowHeight:fontHeight];
    [self reloadColumns];
}

-(void) reloadColumns {
    while([[content tableColumns] count]) {
        [content removeTableColumn:[[content tableColumns] lastObject]];
    }
    
    auto headers = listView->state.header;
    if(headers.size() == 0) headers.push_back("");
    [content setUsesAlternatingRowBackgroundColors:headers.size() >= 2];
    
    for(unsigned column = 0; column < headers.size(); column++) {
        NSTableColumn* tableColumn = [[NSTableColumn alloc] initWithIdentifier:[[NSNumber numberWithInteger:column] stringValue]];
        NSTableHeaderCell* headerCell = [[NSTableHeaderCell alloc] initTextCell:[NSString stringWithUTF8String:headers.at(column).c_str()]];
        CocoaListViewCell* dataCell = [[CocoaListViewCell alloc] initTextCell:@""];
        dataCell->listView = listView;

        [dataCell setEditable:NO];

        [tableColumn setResizingMask:NSTableColumnAutoresizingMask | NSTableColumnUserResizingMask];
        [tableColumn setHeaderCell:headerCell];
        [tableColumn setDataCell:dataCell];
        [content addTableColumn:tableColumn];
    }
}

-(NSInteger) numberOfRowsInTableView:(NSTableView*)table {
    return listView->rowCount();
}

-(id) tableView:(NSTableView*)table objectValueForTableColumn:(NSTableColumn*)tableColumn row:(NSInteger)row {
    NSInteger column = [[tableColumn identifier] integerValue];
    
    NSString* text = [NSString stringWithUTF8String:listView->text(row, column).c_str()];    
    NSImage* image = listView->p.images.at(row).at(column);
    
    if(image) return @{ @"text":text, @"image":image };
    return @{ @"text":text };
}

-(BOOL) tableView:(NSTableView*)table shouldShowCellExpansionForTableColumn:(NSTableColumn*)tableColumn row:(NSInteger)row {
    return NO;
}

-(void) tableView:(NSTableView*)tableView willDisplayCell:(id)cell forTableColumn:(NSTableColumn*)tableColumn row:(NSInteger)row {
    [cell setFont:[self font]];
}

-(void) tableViewSelectionDidChange:(NSNotification*)notification {
    unsigned selectedRow = [content selectedRow];
    if (listView->rowCount() <= selectedRow) return;
    listView->state.selected = true;
    listView->state.selection = selectedRow;
    if(listView->onChange) listView->onChange();
}

-(IBAction) activate:(id)sender {
    if(listView->onActivate) listView->onActivate();
}

-(IBAction) doubleAction:(id)sender {
    if([content clickedRow] >= 0) {
        [self activate:self];
    }
}

@end

namespace GUIKIT {

auto pListView::autoSizeColumns() -> void {
    @autoreleasepool {
        unsigned height = [[cocoaView content] rowHeight];
        for(unsigned column = 0; column < listView.columnCount(); column++) {
            NSTableColumn* tableColumn = [[cocoaView content] tableColumnWithIdentifier:[[NSNumber numberWithInteger:column] stringValue]];
            unsigned minimumWidth = pFont::size([[tableColumn headerCell] font], listView.state.header.at(column)).width + 4;
            for(unsigned row = 0; row < listView.rowCount(); row++) {
                unsigned width = pFont::size([cocoaView font], listView.text(row, column)).width + 4;
                GUIKIT::Image* img = listView.state.images.at(row).at(column);

                if(img && !img->empty()) width += height + 2;
                if(width > minimumWidth) minimumWidth = width;
            }
            [tableColumn setWidth:minimumWidth];
        }
        [[cocoaView content] sizeLastColumnToFit];
    }
}

auto pListView::append(const std::vector<std::string>& list) -> void {
    @autoreleasepool {
        [[cocoaView content] reloadData];
    }
    std::vector<NSImage*> image;
    for (unsigned i = 0; i < list.size(); i++) image.push_back(nil);
    images.push_back(image);
    autoSizeColumns();
}

auto pListView::remove(unsigned selection) -> void {
    @autoreleasepool {
        [[cocoaView content] reloadData];
    }
    releaseRowImages(selection);
    autoSizeColumns();
}

auto pListView::reset() -> void {
    releaseAllImages();
    @autoreleasepool {
        [[cocoaView content] reloadData];
    }
}

auto pListView::setHeaderText(std::vector<std::string> list) -> void {
    @autoreleasepool {
        [cocoaView reloadColumns];
    }
    autoSizeColumns();
}

auto pListView::setHeaderVisible(bool visible) -> void {
    @autoreleasepool {
        if(visible) {
            [[cocoaView content] setHeaderView:[[[NSTableHeaderView alloc] init] autorelease]];
        } else {
            [[cocoaView content] setHeaderView:nil];
        }
    }
}

auto pListView::setSelected(bool selected) -> void {
    @autoreleasepool {
        if(!selected) {
            [[cocoaView content] deselectAll:nil];
        } else {
            setSelection(listView.selection());
        }
    }
}

auto pListView::setSelection(unsigned selection) -> void {
    @autoreleasepool {
        [[cocoaView content] selectRowIndexes:[NSIndexSet indexSetWithIndexesInRange:NSMakeRange(selection, 1)] byExtendingSelection:NO];
        
        [[cocoaView content] scrollRowToVisible: selection];
    }
}

auto pListView::setText(unsigned selection, unsigned position, const std::string& text) -> void {
    @autoreleasepool {
        [[cocoaView content] reloadDataForRowIndexes:[NSIndexSet indexSetWithIndex:selection]
                                       columnIndexes:[NSIndexSet indexSetWithIndex:position]];
    }
    autoSizeColumns();
}

auto pListView::init() -> void {
    @autoreleasepool {
        cocoaView = [[CocoaListView alloc] initWith:listView];
        setHeaderVisible(listView.headerVisible());
    }
}

auto pListView::setImage(unsigned selection, unsigned position, Image& image) -> void {
    @autoreleasepool {
        [images.at(selection).at(position) release];
        images.at(selection).at(position) = NSMakeImage(image);
        
        [[cocoaView content] reloadDataForRowIndexes:[NSIndexSet indexSetWithIndex:selection]
                             columnIndexes:[NSIndexSet indexSetWithIndex:position]];
    }
    autoSizeColumns();
}
    
auto pListView::setEnabled(bool enabled) -> void {
    @autoreleasepool {
        if([[cocoaView content] respondsToSelector:@selector(setEnabled:)]) {
            [[cocoaView content] setEnabled:enabled];
        }
    }
}
    
auto pListView::setGeometry(Geometry geometry) -> void {
    pWidget::setGeometry(geometry);
    autoSizeColumns();
}
    
auto pListView::releaseAllImages() -> void {
    for(unsigned i = 0; i < images.size(); i++) releaseRowImages(i);
    images.clear();
}
    
auto pListView::releaseRowImages(unsigned selection) -> void {
    auto imgList = images.at(selection);
    @autoreleasepool {
        for(auto& image : imgList) {
            [image release];
        }
    }
    images.erase(images.begin() + selection);
}
 
auto pListView::setBackgroundColor(unsigned color) -> void {
    
    NSColor* bg = [NSColor
        colorWithCalibratedRed:((color>>16) & 0xff) / 255.0
        green:((color>>8) & 0xff) / 255.0
        blue:(color & 0xff) / 255.0
        alpha: 1.0];
    
    @autoreleasepool {
        if (cocoaView)
            [[cocoaView content] setBackgroundColor: bg];
    }
}
    
}
