
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
            [(CocoaListView*)[self delegate] activate:self];
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

-(void) mouseDown:(NSEvent *)event {
    [super mouseDown:event];
    
    if (listView->onClick) {
        NSPoint point = [self convertPoint:[event locationInWindow] fromView:nil];
        
        auto _row = [self rowAtPoint:point];
        auto _col = [self columnAtPoint:point];
        listView->onClick(_row, _col);
    }
}

-(void) rightMouseDown:(NSEvent *)event {
    [super rightMouseDown:event];
    
    if (listView->onContext) {
        NSPoint pointInWindow = event.locationInWindow;
        NSPoint point = [self convertPoint:pointInWindow fromView:nil];
        auto _row = [self rowAtPoint:point];
        auto _col = [self columnAtPoint:point];
        
        NSPoint screenPoint = [NSEvent mouseLocation];
        NSScreen* screen = [NSScreen mainScreen];
        CGFloat flippedY = screen.frame.size.height - screenPoint.y;
        
        listView->onContext(_row, _col, { (int)screenPoint.x, (int)flippedY });
    }
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
    NSNumber* row = [[self objectValue] objectForKey:@"row"];
    NSNumber* col = [[self objectValue] objectForKey:@"col"];
    
    int _row = [row intValue];
    int _col = [col intValue];
        
    unsigned textDisplacement = 0;
    
    if(image) {
        [[NSGraphicsContext currentContext] saveGraphicsState];
        
        NSRect targetRect = NSMakeRect(frame.origin.x, frame.origin.y, frame.size.height, frame.size.height);
        NSRect sourceRect = NSMakeRect(0, 0, [image size].width, [image size].height);
        [image drawInRect:targetRect fromRect:sourceRect operation:NSCompositingOperationSourceOver fraction:1.0 respectFlipped:YES hints:nil];
        
        [[NSGraphicsContext currentContext] restoreGraphicsState];
        textDisplacement = frame.size.height + 2;
    } else {
        auto& aligns = listView->state.aligns;
        if (_col < aligns.size()) {
            switch(aligns[_col]) {
                case GUIKIT::ListView::Align::Center: {
                    unsigned textWidth = GUIKIT::pFont::size([self font], listView->text(_row, _col)).width;
                    if (frame.size.width > textWidth)
                        textDisplacement = (frame.size.width - textWidth) / 2;
                } break;
                case GUIKIT::ListView::Align::Right: {
                    unsigned textWidth = GUIKIT::pFont::size([self font], listView->text(_row, _col)).width;
                    if (frame.size.width > textWidth)
                        textDisplacement = (frame.size.width - textWidth);
                } break;
                default: break;
            }
        }
    }
    
    NSRect textRect = NSMakeRect(
        frame.origin.x + textDisplacement, frame.origin.y + listView->p.fontAdjust.yOffset,
        frame.size.width - textDisplacement, frame.size.height + listView->p.fontAdjust.height);
    
    NSColor* textColor;
    
    if ([self isHighlighted]) {
        if (listView->overrideSelectionColor())
            textColor = GUIKIT::pHelper::RGBToNSColor( listView->selectionForegroundColor());
        else
            textColor = [NSColor alternateSelectedControlTextColor];
    } else {
        std::optional<unsigned> rowColor = listView->rowForegroundColor( _row, _col );
        
        if (rowColor.has_value()) {
            textColor = GUIKIT::pHelper::RGBToNSColor( rowColor.value() );
        
        } else if(listView->overrideForegroundColor()) {
            textColor = GUIKIT::pHelper::RGBToNSColor( listView->foregroundColor() );
        } else
            textColor = [NSColor textColor];
    }
    
    if ([self isHighlighted] && listView->overrideSelectionColor()) {
        NSColor* hicol = GUIKIT::pHelper::RGBToNSColor( listView->selectionBackgroundColor() );
        
        [hicol set];
        if (listView->columnCount() > 1) {
            NSSize spacing = [[(id)listView->p.cocoaView content] intercellSpacing];
            frame.size.width += spacing.width;
        } else
            frame.size.width = listView->geometry().width;
            
        frame.origin.x -= 5;
        NSRectFill(frame);
    } else if (![self isHighlighted]) {
        std::optional<unsigned> rowColor = listView->rowBackgroundColor( _row );
        if (rowColor.has_value()) {
            NSColor* frcol = GUIKIT::pHelper::RGBToNSColor( rowColor.value() );
            [frcol set];
            NSRectFill(frame);
        }
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
        [self setAutohidesScrollers:YES];
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
        
        _font = nil;
        [self setFont:nil];
    }
    return self;
}

-(void) dealloc {
    [content release];
    [_font release];
    [super dealloc];
}

-(CocoaListViewContent*) content {
    return content;
}

-(NSFont*) font {
    return _font;
}

-(void) setFont:(NSFont*)fontPointer {
    
    listView->p.fontAdjust.rowHeight = 0;
    listView->p.fontAdjust.yOffset = 0;
    listView->p.fontAdjust.height = 0;
    
    if (listView->spacing() == 0) {
        // this is a hack to completly remove row spacing
        unsigned fontSize = GUIKIT::pFont::getSizeFromString( listView->font() );
        
        if (__MAC_OS_X_VERSION_MIN_REQUIRED <= MAC_OS_X_VERSION_10_13 ) { // High Sierra
            if (fontSize == 6 || fontSize == 7 || fontSize == 11) {
                listView->p.fontAdjust.rowHeight = -3;
                listView->p.fontAdjust.yOffset = -2;
                listView->p.fontAdjust.height = 2;
                
            } else if ( fontSize == 8 || fontSize == 9 || fontSize == 12 || fontSize == 13 || fontSize == 14 || fontSize == 16 ) {
                listView->p.fontAdjust.rowHeight = -5;
                listView->p.fontAdjust.yOffset = -4;
                listView->p.fontAdjust.height = 4;
                
            } else if ( fontSize == 10 ) {
                listView->p.fontAdjust.rowHeight = -4;
                listView->p.fontAdjust.yOffset = -3;
                listView->p.fontAdjust.height = 3;
            } else if ( fontSize == 15 ) {
                listView->p.fontAdjust.rowHeight = -6;
                listView->p.fontAdjust.yOffset = -5;
                listView->p.fontAdjust.height = 5;
            }
        }
    }

    if(!fontPointer)
        fontPointer = [NSFont systemFontOfSize:18];
    [fontPointer retain];
    if(_font) [_font release];
    _font = fontPointer;
    
    unsigned fontHeight = GUIKIT::pFont::size(_font, "O").height;
    [content setFont:_font];
    [content setRowHeight:fontHeight + listView->p.fontAdjust.rowHeight ];
    
    int _spacing = listView->spacing();
    
    if (_spacing == 0) {
        [content setIntercellSpacing:NSMakeSize(0.0, 0.0)];
    } else if (_spacing > 0)
        [content setIntercellSpacing:NSMakeSize((CGFloat)_spacing, 3.0)];
    else
        [content setIntercellSpacing:NSMakeSize(8.0, 3.0)];
    
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
        if (listView->state.centerHeader)
            [headerCell setAlignment:NSTextAlignmentCenter];
        
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
    NSNumber* _row = [NSNumber numberWithInt:row];
    NSNumber* _col = [NSNumber numberWithInt:column];
    
    if(image) return @{ @"text":text, @"image":image, @"row":_row, @"col":_col };
    return @{ @"text":text, @"row":_row, @"col":_col };
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
    listView->state.column = 0;
    if([content clickedRow] >= 0) {
        if([content clickedColumn] >= 0) {
            listView->state.column = [content clickedColumn];
        }

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

    if ( listView->p.useCustomTooltip || !toolTips.size())
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
    int spacing = listView.spacing();
    if (spacing == -1)
        spacing = 10;
    
    @autoreleasepool {
        unsigned height = [[(id)cocoaView content] rowHeight];
        for(unsigned column = 0; column < listView.columnCount(); column++) {
            NSTableColumn* tableColumn = [[(id)cocoaView content] tableColumnWithIdentifier:[[NSNumber numberWithInteger:column] stringValue]];
            
            auto _text = listView.state.header[column];
            
            unsigned minimumWidth = _text.empty() ? 0 : (pFont::size([[tableColumn headerCell] font], _text).width + spacing);
            
            for(unsigned row = 0; row < listView.rowCount(); row++) {
                auto _text = listView.text(row, column);
                unsigned width = _text.empty() ? 0 : (pFont::size([(id)cocoaView font], _text).width + spacing);
                GUIKIT::Image* img = listView.state.images[row][column];
                if(img) {
                    width += height + 2;
                }
                if(width > minimumWidth) minimumWidth = width;
            }
            [tableColumn setWidth:minimumWidth];
        }
        // would disable horizantal scrollbar of nsscrollview
     //   [[cocoaView content] sizeLastColumnToFit];
    }
}

auto pListView::append(const std::vector<std::string>& list, bool preventColumnResizing) -> void {
    @autoreleasepool {

        [[(id)cocoaView content] reloadData];
    }
    std::vector<NSImage*> image;
    for (unsigned i = 0; i < list.size(); i++) image.push_back(nil);
    images.push_back(image);
    if (!preventColumnResizing)
        autoSizeColumns();
}

auto pListView::remove(unsigned selection) -> void {
    @autoreleasepool {
        [[(id)cocoaView content] reloadData];
    }
    releaseRowImages(selection);
    autoSizeColumns();
}

auto pListView::reset() -> void {
    releaseAllImages();
    @autoreleasepool {
        [[(id)cocoaView content] reloadData];
    }
}

auto pListView::setHeaderText(std::vector<std::string> list) -> void {
    @autoreleasepool {
        [(id)cocoaView reloadColumns];
    }
    autoSizeColumns();
}

auto pListView::setHeaderVisible(bool visible) -> void {
    @autoreleasepool {
        if(visible) {
            [[(id)cocoaView content] setHeaderView:[[[NSTableHeaderView alloc] init] autorelease]];
        } else {
            [[(id)cocoaView content] setHeaderView:nil];
        }
    }
}

auto pListView::setSelected(bool selected) -> void {
    @autoreleasepool {
        if(!selected) {
            [[(id)cocoaView content] deselectAll:nil];
        } else {
            setSelection(listView.selection());
        }
    }
}

auto pListView::setSelection(unsigned selection) -> void {
    @autoreleasepool {
        [[(id)cocoaView content] selectRowIndexes:[NSIndexSet indexSetWithIndexesInRange:NSMakeRange(selection, 1)] byExtendingSelection:NO];
        
        [[(id)cocoaView content] scrollRowToVisible: selection];
    }
}

auto pListView::setText(unsigned selection, unsigned position, const std::string& text, bool preventColumnResizing) -> void {
    @autoreleasepool {
        [[(id)cocoaView content] reloadDataForRowIndexes:[NSIndexSet indexSetWithIndex:selection]
                                       columnIndexes:[NSIndexSet indexSetWithIndex:position]];
    }
    if (!preventColumnResizing)
        autoSizeColumns();
}

auto pListView::init() -> void {
    @autoreleasepool {
        cocoaView = [[CocoaListView alloc] initWith:listView];
        setHeaderVisible(listView.headerVisible());
    }
}

auto pListView::setImage(unsigned selection, unsigned position, Image& image, bool preventColumnResizing) -> void {
    @autoreleasepool {
        [images.at(selection).at(position) release];
        images.at(selection).at(position) = NSMakeImage(image);
        
        [[(id)cocoaView content] reloadDataForRowIndexes:[NSIndexSet indexSetWithIndex:selection]
                             columnIndexes:[NSIndexSet indexSetWithIndex:position]];
    }
    if (!preventColumnResizing)
        autoSizeColumns();
}
    
auto pListView::setEnabled(bool enabled) -> void {
    @autoreleasepool {
        if([[(id)cocoaView content] respondsToSelector:@selector(setEnabled:)]) {
            [[(id)cocoaView content] setEnabled:enabled];
        }
    }
}
    
auto pListView::setGeometry(Geometry geometry) -> void {
    pWidget::setGeometry(geometry);
    autoSizeColumns();
    if (useCustomTooltip)
        [[(id)cocoaView content] updateTrackingAreas];
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
    
    NSColor* bg = pHelper::RGBToNSColor( color );
    
    @autoreleasepool {
        if (cocoaView) {
            [(id)cocoaView setBackgroundColor: bg];
            [[(id)cocoaView content] setBackgroundColor: bg];
        }
    }
    updateTooltipUsage();
}
    
auto pListView::setForegroundColor(unsigned color) -> void {
    updateTooltipUsage();
}

auto pListView::setFont(std::string font) -> void {
    if (listView.spacing() != 0 && GUIKIT::hasMinimumVersion(10, 10))
        [(id)cocoaView setContentInsets:NSEdgeInsetsMake(0, 2, 0, 2)];
    
    updateTooltipUsage();
    pWidget::setFont(font);
    setGeometry( listView.geometry() );
}
    
auto pListView::createCustomTooltip() -> void {
    @autoreleasepool {
        
        tooltip = [[TooltipWindow alloc] initWithContentRect:NSMakeRect(0, 0, 0, 0) styleMask:NSBorderlessWindowMask backing:NSBackingStoreBuffered defer:NO];
        
        if (cocoaView)
            [tooltip setFont: [(id)cocoaView font] ];
        
        if (listView.state.colorRowTooltips) {
            
            if (listView.overrideBackgroundColor()) {
                [tooltip setBackgroundColor: pHelper::RGBToNSColor(listView.backgroundColor()) ];
            }
            if (listView.overrideForegroundColor()) {
                [tooltip setTextColor: pHelper::RGBToNSColor(listView.foregroundColor()) ];
            }
        }
    }
}
    
auto pListView::updateTooltipUsage() -> void {
    useCustomTooltip = false;
    
    if (listView.state.colorRowTooltips && (listView.overrideBackgroundColor() || listView.overrideForegroundColor()) )
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

auto pListView::updateRowColors() -> void {
    @autoreleasepool {
        [[(id)cocoaView content] reloadData];
    }
}

auto pListView::updateRowForegroundColors() -> void {
    @autoreleasepool {
        [[(id)cocoaView content] reloadData];
    }
}

auto pListView::getFirstVisibleRow() -> unsigned {
    NSRect rect = [[(id)cocoaView content] visibleRect];
    NSRange rows = [[(id)cocoaView content] rowsInRect:rect];
    return rows.location > 2 ? rows.location + 2 : 0;
}

pListView::~pListView() {
    releaseAllImages();
    
    @autoreleasepool {
        if (tooltip)
            [tooltip release];
    }
}
    
}
