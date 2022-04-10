
@implementation CocoaListViewContent : NSTableView

-(id) initWith:(GUIKIT::ListView&)listViewReference {
    if(self = [super initWithFrame:NSMakeRect(0, 0, 0, 0)]) {
        listView = &listViewReference;
        [self setRowSizeStyle: NSTableViewRowSizeStyleCustom];
    }
    
    return self;
}

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

-(void) updateTrackingAreas {
    if(!listView->p.useCustomTooltip)
        return;
    
    if(trackingArea != nil) {
        [self removeTrackingArea:trackingArea];
        [trackingArea release];
    }
    
    int opts = (NSTrackingMouseEnteredAndExited | NSTrackingMouseMoved | NSTrackingActiveAlways);
    trackingArea = [ [NSTrackingArea alloc] initWithRect:[self bounds] options:opts owner:self userInfo:nil];
    [self addTrackingArea:trackingArea];
}

-(void) mouseEntered:(NSEvent*)event {
    listView->p.mouseIsOver = true;
}

-(void) mouseExited:(NSEvent*)event {
    listView->p.mouseIsOver = false;
    if (!listView->p.useCustomTooltip)
        return;
    
    if (!listView->p.tooltip)
        listView->p.createCustomTooltip();
        
    [listView->p.tooltip orderOut:nil];
}

-(void) mouseMoved:(NSEvent*)event {
    
    if (!listView->p.mouseIsOver || !listView->p.useCustomTooltip)
        return;

    auto& toolTips = listView->state.rowTooltips;
    if (!toolTips.size())
        return;

    auto mouseOverRow = [self rowAtPoint:[self convertPoint:[event locationInWindow] fromView:nil]];
    
    if (!listView->p.tooltip)
        listView->p.createCustomTooltip();

    if (mouseOverRow < 0 || mouseOverRow >= toolTips.size()) {
        [listView->p.tooltip orderOut:nil];
        return;
    }
    
    if (toolTips[mouseOverRow].empty()) {
        [listView->p.tooltip orderOut:nil];
        return;
    }
    NSString* text = [NSString stringWithUTF8String:toolTips[mouseOverRow].c_str()];
    
    [listView->p.tooltip setTooltip:text];
    [listView->p.tooltip orderFront:nil];
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
        frame.origin.x + textDisplacement, frame.origin.y + listView->p.fontAdjust.yOffset,
        frame.size.width - textDisplacement, frame.size.height + listView->p.fontAdjust.height);
    
    NSColor* textColor;
    
    if ([self isHighlighted]) {
        if (listView->overrideSelectionColor())
            textColor = GUIKIT::pHelper::getColor( listView->selectionForegroundColor());
        else
            textColor = [NSColor alternateSelectedControlTextColor];
    } else {
        if(listView->overrideForegroundColor()) {
            textColor = GUIKIT::pHelper::getColor( listView->foregroundColor() );
        } else
            textColor = [NSColor textColor];
    }
    
    if ([self isHighlighted] && listView->overrideSelectionColor()) {
        NSColor* hicol = GUIKIT::pHelper::getColor( listView->selectionBackgroundColor() );
        
        [hicol set];
        if (listView->columnCount() > 1) {
            NSSize spacing = [[listView->p.cocoaView content] intercellSpacing];
            frame.size.width += spacing.width;
        } else
            frame.size.width = listView->geometry().width;
            
        NSRectFill(frame);
    }
    
    [text drawInRect:textRect withAttributes:@{ NSForegroundColorAttributeName:textColor, NSFontAttributeName:[self font] }];
}
@end

@implementation CocoaListView : NSScrollView

-(id) initWith:(GUIKIT::ListView&)listViewReference {
    if(self = [super initWithFrame:NSMakeRect(0, 0, 0, 0)]) {
        listView = &listViewReference;
        
        content = [[CocoaListViewContent alloc] initWith:listViewReference];
        
        [self setDocumentView:content];
        [self setBorderType:NSBezelBorder];
        [self setHasVerticalScroller:YES];
        [self setHasHorizontalScroller:YES];
        if (GUIKIT::hasMinimumVersion(10, 10)) {
            [self setAutomaticallyAdjustsContentInsets:NO];
            [self setContentInsets:NSEdgeInsetsMake(2, 2, 2, 2)];
        }

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
    
    listView->p.fontAdjust.rowHeight = 0;
    listView->p.fontAdjust.yOffset = -1;
    listView->p.fontAdjust.height = 0;
    
    if (listView->specialFont()) {
        // this is a hack to completly remove row spacing
        unsigned fontSize = GUIKIT::pFont::getSizeFromString( listView->font() );
        
        if (__MAC_OS_X_VERSION_MIN_REQUIRED > MAC_OS_X_VERSION_10_13) { // build for Mojave and above
            listView->p.fontAdjust.rowHeight = 0;
            listView->p.fontAdjust.yOffset = 0;
            listView->p.fontAdjust.height = 0;
            
            if (fontSize == 8 || fontSize == 9) {
                listView->p.fontAdjust.rowHeight = -1;
                listView->p.fontAdjust.yOffset = 0;
                listView->p.fontAdjust.height = 1;
            } else if (fontSize == 10) {
                listView->p.fontAdjust.rowHeight = 0;
                listView->p.fontAdjust.yOffset = 1;
                listView->p.fontAdjust.height = 0;
            } else if (fontSize == 11) {
                listView->p.fontAdjust.rowHeight = 0;
                listView->p.fontAdjust.yOffset = 1;
                listView->p.fontAdjust.height = 1;
            } else if (fontSize == 13 || fontSize == 14) {
                listView->p.fontAdjust.rowHeight = 0;
                listView->p.fontAdjust.yOffset = 1;
                listView->p.fontAdjust.height = 0;
            }
        } else {
            if (fontSize == 6 || fontSize == 7 || fontSize == 11) {
                listView->p.fontAdjust.rowHeight = -3;
                listView->p.fontAdjust.yOffset = -2;
                listView->p.fontAdjust.height = 2;
                
            } else if ( fontSize == 8 || fontSize == 9 || fontSize == 12 || fontSize == 13 || fontSize == 14 ) {
                listView->p.fontAdjust.rowHeight = -5;
                listView->p.fontAdjust.yOffset = -4;
                listView->p.fontAdjust.height = 4;
                
            } else if ( fontSize == 10 ) {
                listView->p.fontAdjust.rowHeight = -4;
                listView->p.fontAdjust.yOffset = -3;
                listView->p.fontAdjust.height = 3;
            }
        }
    }

    if(!fontPointer)
        fontPointer = [NSFont systemFontOfSize:18];
    [fontPointer retain];
    if(font) [font release];
    font = fontPointer;
    
    unsigned fontHeight = GUIKIT::pFont::size(font, "O").height;
    [content setFont:font];
    [content setRowHeight:fontHeight + listView->p.fontAdjust.rowHeight ];
    
    if (listView->specialFont()) {
        [content setIntercellSpacing:NSMakeSize(0.0, 0.0)];
    }
    
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

- (NSString*) tableView:(NSTableView *)tableView
toolTipForCell:(NSCell*)cell
rect:(NSRectPointer)rect
tableColumn:(NSTableColumn*)tableColumn
row:(NSInteger)row
mouseLocation:(NSPoint)mouseLocation {
    auto& toolTips = listView->state.rowTooltips;

    if (listView->p.useCustomTooltip || !toolTips.size())
        return nil;

    if (row >= toolTips.size())
        return nil;
    
    if (toolTips[row].empty())
        return nil;
    
    NSString* text = [NSString stringWithUTF8String:toolTips[row].c_str()];
    
    return text;
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
        // would disable horizantal scrollbar of nsscrollview
     //   [[cocoaView content] sizeLastColumnToFit];
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
    if (useCustomTooltip)
        [[cocoaView content] updateTrackingAreas];
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
    
    NSColor* bg = pHelper::getColor( color );
    
    @autoreleasepool {
        if (cocoaView) {
            [cocoaView setBackgroundColor: bg];
            [[cocoaView content] setBackgroundColor: bg];
        }
    }
    updateTooltipUsage();
}
    
auto pListView::setForegroundColor(unsigned color) -> void {
    updateTooltipUsage();
}

auto pListView::setFont(std::string font) -> void {
    if (!listView.specialFont() && GUIKIT::hasMinimumVersion(10, 10))
        [cocoaView setContentInsets:NSEdgeInsetsMake(0, 2, 0, 2)];
    
    updateTooltipUsage();
    pWidget::setFont(font);
    setGeometry( listView.geometry() );
}
    
auto pListView::createCustomTooltip() -> void {
    @autoreleasepool {
        
        tooltip = [[TooltipWindow alloc] initWithContentRect:NSMakeRect(0, 0, 0, 0) styleMask:NSBorderlessWindowMask backing:NSBackingStoreBuffered defer:NO];
        
        if (cocoaView)
            [tooltip setFont: [cocoaView font] ];
        
        if (listView.state.colorRowTooltips) {
            
            if (listView.Widget::state.overrideBackgroundColor) {
                [tooltip setBackgroundColor: pHelper::getColor(listView.Widget::state.backgroundColor) ];
            }
            if (listView.Widget::state.overrideForegroundColor) {
                [tooltip setTextColor: pHelper::getColor(listView.Widget::state.foregroundColor) ];
            }
        }
    }
}
    
auto pListView::updateTooltipUsage() -> void {
    useCustomTooltip = false;
    
    if (listView.state.colorRowTooltips && (listView.Widget::state.overrideBackgroundColor || listView.Widget::state.overrideForegroundColor) )
        useCustomTooltip = true;
    else if ( listView.font() != Font::system() )
        useCustomTooltip = true;

    if (tooltip) {
        [tooltip release];
        tooltip = nullptr;
    }
}
    
auto pListView::colorRowTooltips( bool colorTip ) -> void {
    updateTooltipUsage();
}

pListView::~pListView() {
    releaseAllImages();
    
    @autoreleasepool {
        if (tooltip)
            [tooltip release];
    }
}
    
}
