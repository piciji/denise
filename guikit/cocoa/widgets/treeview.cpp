
@implementation TreeViewWrapper : NSObject
-(id) initWith:(GUIKIT::TreeViewItem*)tvItem {
    if(self = [super init]) {
        treeViewItem = tvItem;
    }
    return self;
}
@end

@implementation CocoaTreeViewContent : NSOutlineView

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

@implementation CocoaTreeViewCell : NSTextFieldCell

-(NSString*) stringValue {
    return [[self objectValue] objectForKey:@"text"];
}

- (void)drawWithExpansionFrame:(NSRect)cellFrame inView:(NSView *)view {
    
}

- (NSRect)expansionFrameWithFrame:(NSRect)cellFrame inView:(NSView *)view {
    // fix the extra tooltip
    return NSZeroRect;
}

-(void) drawWithFrame:(NSRect)frame inView:(NSView*)view {
    NSString* text = [[self objectValue] objectForKey:@"text"];
    NSImage* image = [[self objectValue] objectForKey:@"image"];
    unsigned textDisplacement = 2;
    
    if(image) {
        [[NSGraphicsContext currentContext] saveGraphicsState];
        
        NSRect targetRect = NSMakeRect(frame.origin.x + 2, frame.origin.y, frame.size.height, frame.size.height);
        NSRect sourceRect = NSMakeRect(0, 0, [image size].width, [image size].height);
        [image drawInRect:targetRect fromRect:sourceRect operation:NSCompositeSourceOver fraction:1.0 respectFlipped:YES hints:nil];
        
        [[NSGraphicsContext currentContext] restoreGraphicsState];
        textDisplacement = frame.size.height + 4;
    }
    
    NSRect textRect = NSMakeRect(
        frame.origin.x + textDisplacement, frame.origin.y,
        frame.size.width + textDisplacement, frame.size.height);
    
    NSColor* textColor = [self isHighlighted] ? [NSColor alternateSelectedControlTextColor] : [NSColor textColor];
    
    if(treeView->overrideForegroundColor()) {
        unsigned color = treeView->foregroundColor();
        textColor = GUIKIT::pHelper::getColor( color );
    }
    
    [text drawInRect:textRect withAttributes:@{ NSForegroundColorAttributeName:textColor, NSFontAttributeName:[self font] }];
}
@end

@implementation CocoaTreeView : NSScrollView

-(id) initWith:(GUIKIT::TreeView&)treeViewReference {
    if(self = [super initWithFrame:NSMakeRect(0, 0, 0, 0)]) {
        treeView = &treeViewReference;
        content = [[CocoaTreeViewContent alloc] initWithFrame:NSMakeRect(0, 0, 0, 0)];
    
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
        [content setHeaderView:nil];
        [content setColumnAutoresizingStyle:NSTableViewLastColumnOnlyAutoresizingStyle];
        
        font = nil;
        [self setFont:nil];
        [self reloadColumns];
    }
    return self;
}

-(void) dealloc {
    [content release];
    [font release];
    [super dealloc];
}

-(CocoaTreeViewContent*) content {
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
}

-(void) reloadColumns {
    while([[content tableColumns] count]) {
        [content removeTableColumn:[[content tableColumns] lastObject]];
    }

    NSTableColumn* tableColumn = [[NSTableColumn alloc] initWithIdentifier:[[NSNumber numberWithInteger:0] stringValue]];
    NSTableHeaderCell* headerCell = [[NSTableHeaderCell alloc] initTextCell:@""];
    CocoaTreeViewCell* dataCell = [[CocoaTreeViewCell alloc] initTextCell:@""];
    
    dataCell->treeView = treeView;
    [dataCell setEditable:NO];
        
    [tableColumn setResizingMask:NSTableColumnAutoresizingMask | NSTableColumnUserResizingMask];
    [tableColumn setHeaderCell:headerCell];
    [tableColumn setDataCell:dataCell];
    [content addTableColumn:tableColumn];
    [content setOutlineTableColumn: tableColumn];
}

- (id)outlineView:(NSOutlineView*)outlineView child:(NSInteger)index ofItem:(id)item {
    
    if (item == nil) {
        return treeView->state.items[index]->p.wrapper;
        
    } else {
        GUIKIT::TreeViewItem* treeViewItem = ((TreeViewWrapper*)item)->treeViewItem;
        
        return treeViewItem->state.items[index]->p.wrapper;
    }
}

- (BOOL)outlineView:(NSOutlineView*)outlineView isItemExpandable:(id)item {
    if(item == nil) {
        return YES;
    }
    return ((TreeViewWrapper*)item)->treeViewItem->itemCount() > 0 ? YES : NO;
}

- (NSInteger)outlineView:(NSOutlineView *)outlineView numberOfChildrenOfItem:(id)item {
    if (item == nil) {
        return treeView->itemCount();
    }
    return ((TreeViewWrapper*)item)->treeViewItem->itemCount();
}

- (id)outlineView:(NSOutlineView *)outlineView objectValueForTableColumn:(NSTableColumn*)tableColumn
    byItem:(id)item {
    
    NSImage* image = ((TreeViewWrapper*)item)->treeViewItem->p.usensimage;
    NSString* text = [NSString stringWithUTF8String:((TreeViewWrapper*)item)->treeViewItem->text().c_str()];
        
    if(image) return @{ @"text":text, @"image":image };
    return @{ @"text":text };
}

- (void)outlineView:(NSOutlineView *)outlineView willDisplayOutlineCell:(id)cell forTableColumn:(NSTableColumn *)tableColumn item:(id)item {
    
    [cell setFont:[self font]];
}

- (BOOL)outlineView:(NSOutlineView *)outlineView shouldExpandItem:(id)item {
    
    GUIKIT::TreeViewItem* tvitem = ((TreeViewWrapper*)item)->treeViewItem;
    
    tvitem->state.expanded = true;
    
    tvitem->p.usensimage = tvitem->p.nsimageSelected != nil ? tvitem->p.nsimageSelected : ((tvitem->expanded() && tvitem->p.nsimageExpanded) ?tvitem->p.nsimageExpanded : tvitem->p.nsimage);

    if (tvitem->parentView()->onExpand) tvitem->parentView()->onExpand(tvitem);
    
    return YES;
}

- (BOOL)outlineView:(NSOutlineView *)outlineView shouldCollapseItem:(id)item {
    
    GUIKIT::TreeViewItem* tvitem = ((TreeViewWrapper*)item)->treeViewItem;
    
    tvitem->state.expanded = false;
    
    tvitem->p.usensimage = tvitem->p.nsimageSelected != nil ? tvitem->p.nsimageSelected : ((tvitem->expanded() && tvitem->p.nsimageExpanded) ?tvitem->p.nsimageExpanded : tvitem->p.nsimage);
    
    if (tvitem->parentView()->onCollapse) tvitem->parentView()->onCollapse(tvitem);
    
    return YES;
}

-(void) outlineViewSelectionDidChange:(NSNotification*)notification {
    NSOutlineView* outlineView = [notification object];
    id item = [outlineView itemAtRow:[outlineView selectedRow]];
    if (item == nil) return;
    
    [self setImage: item];
    
    treeView->state.selected = ((TreeViewWrapper*)item)->treeViewItem;
    if(treeView->onChange) treeView->onChange();
}

- (void) outlineViewSelectionIsChanging:(NSNotification *)notification {
    NSOutlineView* outlineView = [notification object];
    id item = [outlineView itemAtRow:[outlineView selectedRow]];
    if (item == nil) return;
    [self setImage: item];
}

- (void) setImage:(TreeViewWrapper *)item {
    if (treeView->state.selected) {
        treeView->state.selected->p.usensimage =
        (treeView->state.selected->expanded() && treeView->state.selected->p.nsimageExpanded) ? treeView->state.selected->p.nsimageExpanded :
        treeView->state.selected->p.nsimage;
    }
    
    GUIKIT::TreeViewItem* tvitem = ((TreeViewWrapper*)item)->treeViewItem;
    tvitem->p.usensimage = tvitem->p.nsimageSelected != nil ? tvitem->p.nsimageSelected : ((tvitem->expanded() && tvitem->p.nsimageExpanded) ? tvitem->p.nsimageExpanded : tvitem->p.nsimage);
}

-(IBAction) activate:(id)sender {
    auto selected = treeView->state.selected;
    if (selected) {
        if(selected->itemCount() > 0) {
            BOOL expanded = [content isItemExpanded: selected->p.wrapper];
            selected->setExpanded( !expanded );
        }
    }
    if(treeView->onActivate) treeView->onActivate();
}

-(IBAction) doubleAction:(id)sender {
    if([content clickedRow] >= 0) {
        [self activate:self];
    }
}

@end


namespace GUIKIT {
    
    auto pTreeViewItem::parentTreeView() -> TreeView* {
        return treeViewItem.state.parentTreeView;
    }
    
    auto pTreeViewItem::append(TreeViewItem& item) -> void {
        item.state.parentTreeView = parentTreeView();

        @autoreleasepool {
            if (parentTreeView()) {
                [[parentTreeView()->p.cocoaView content] reloadData];
                parentTreeView()->p.update();
            }
        }
    }
    
    auto pTreeViewItem::remove(TreeViewItem& item) -> void {
        item.p.invalidateParent();
        @autoreleasepool {
            if (parentTreeView()) [[parentTreeView()->p.cocoaView content] reloadData];
        }
    }
    
    auto pTreeViewItem::reset() -> void {
        for(auto item : treeViewItem.state.items) {
            item->p.invalidateParent();
        }
        treeViewItem.state.items.clear();
        @autoreleasepool {
            if (parentTreeView()) [[parentTreeView()->p.cocoaView content] reloadData];
        }
    }

    auto pTreeViewItem::invalidateParent() -> void {
        treeViewItem.state.parentTreeView = nullptr;
        for(auto item : treeViewItem.state.items) {
            item->p.invalidateParent();
        }
    }

    auto pTreeViewItem::setText(std::string text) -> void {
        @autoreleasepool {
            if (parentTreeView()) [[parentTreeView()->p.cocoaView content] reloadItem:wrapper];
        }
    }
    
    auto pTreeViewItem::setSelected() -> void {
        if (!parentTreeView()) return;
        
        @autoreleasepool {
            NSInteger itemIndex = [[parentTreeView()->p.cocoaView content] rowForItem:wrapper];
            if (itemIndex < 0) return;
            
            [[parentTreeView()->p.cocoaView content] selectRowIndexes:[NSIndexSet indexSetWithIndex:itemIndex] byExtendingSelection:NO];
        }
    }
    
    auto pTreeViewItem::setExpanded(bool expanded) -> void {
        if (!parentTreeView()) return;
        @autoreleasepool {
            if (expanded)
                [[parentTreeView()->p.cocoaView content] expandItem:wrapper];
            else
                [[parentTreeView()->p.cocoaView content] collapseItem:wrapper];
        }
    }
    
    auto pTreeViewItem::setImage(Image& image) -> void {
        @autoreleasepool {
            usensimage = nsimage = NSMakeImage(image);
            if (parentTreeView()) [[parentTreeView()->p.cocoaView content] reloadItem:wrapper];
        }
    }
    
    auto pTreeViewItem::setImageSelected(Image& image) -> void {
        @autoreleasepool {
            nsimageSelected = NSMakeImage(image);
        }
    }
    
    auto pTreeViewItem::setImageExpanded(Image& image) -> void {
        @autoreleasepool {
            nsimageExpanded = NSMakeImage(image);
        }
    }
    
    auto pTreeView::update() -> void {
        for(auto& item : treeView.state.items) {
            item->state.parentTreeView = &treeView;
            item->p.update( );
        }
    }
    
    auto pTreeViewItem::update() -> void {
        
        for(auto& item : treeViewItem.state.items) {
            item->state.parentTreeView = parentTreeView();
            item->p.update();
        }
        setExpanded( treeViewItem.expanded() );
    }
    
    auto pTreeViewItem::init() -> void {
        wrapper = [[TreeViewWrapper alloc] initWith: &treeViewItem];
    }
    
    auto pTreeView::init() -> void {
        @autoreleasepool {
            cocoaView = [[CocoaTreeView alloc] initWith:treeView];
        }
    }
    
    auto pTreeView::append(TreeViewItem& item) -> void {
        item.state.parentTreeView = &treeView;

        @autoreleasepool {
            [[cocoaView content] reloadData];
            update();
        }
    }
    
    auto pTreeView::remove(TreeViewItem& item) -> void {
        item.p.invalidateParent();
        @autoreleasepool {
            [[cocoaView content] reloadData];
        }
    }
    
    auto pTreeView::reset() -> void {
        for(auto item : treeView.state.items) {
            item->p.invalidateParent();
        }
        treeView.state.items.clear();
        @autoreleasepool {
            [[cocoaView content] reloadData];
        }
    }
    
    auto pTreeView::setBackgroundColor(unsigned color) -> void {
        
        NSColor* bg = pHelper::getColor( color );
        
        @autoreleasepool {
            if (cocoaView)
                [[cocoaView content] setBackgroundColor: bg];
        }
    }

    
    pTreeViewItem::~pTreeViewItem() {
        @autoreleasepool {
            [nsimage release];
            [nsimageSelected release];
            [nsimageExpanded release];
        }
    }
    
}
