
auto pTreeViewItem::parentTreeView() -> TreeView* {
    return treeViewItem.state.parentTreeView;
}

auto pTreeViewItem::append(TreeViewItem& item) -> void {
    if(!parentTreeView()) return;
    item.state.parentTreeView = parentTreeView();
    item.p.update(&treeViewItem);
}

auto pTreeViewItem::remove(TreeViewItem& item) -> void {
    if(!parentTreeView()) return;
    gtk_tree_store_remove(parentTreeView()->p.gtkTreeStore, &item.p.iter);
    invalidateParent();
}

auto pTreeViewItem::reset() -> void {
    if(!parentTreeView()) return;
    for(auto item : treeViewItem.state.items) {
        gtk_tree_store_remove(parentTreeView()->p.gtkTreeStore, &item->p.iter);
    }
    invalidateParent();
}

auto pTreeViewItem::invalidateParent() -> void {
    treeViewItem.state.parentTreeView = nullptr;
    for(auto item : treeViewItem.state.items) {
        item->p.invalidateParent();
    }
}

auto pTreeViewItem::setText(std::string text) -> void {
    if (!parentTreeView()) return;

    gtk_tree_store_set(parentTreeView()->p.gtkTreeStore, &iter, 1, treeViewItem.text().c_str(), -1);
}

auto pTreeViewItem::setSelected() -> void {
    if (!parentTreeView() ) return;

    gtk_tree_selection_select_iter(parentTreeView()->p.gtkTreeSelection, &iter);
}

auto pTreeViewItem::setExpanded(bool expanded) -> void {
    if (!parentTreeView()) return;

    GtkTreePath* path = gtk_tree_model_get_path(parentTreeView()->p.gtkTreeModel, &iter);
    
    if (expanded)
        gtk_tree_view_expand_row (parentTreeView()->p.gtkTreeView, path, false);
    else
        gtk_tree_view_collapse_row (parentTreeView()->p.gtkTreeView, path);
}

auto pTreeViewItem::addItem(TreeViewItem* parent) -> void {
    gtk_tree_store_append(parentTreeView()->p.gtkTreeStore, &iter, parent == nullptr ? NULL : &parent->p.iter );
    gtk_tree_store_set(parentTreeView()->p.gtkTreeStore, &iter, 1, treeViewItem.text().c_str(), -1);
    
    gdkimage = parentTreeView()->p.getImage(treeViewItem.state.image);
    gdkimageSelected = parentTreeView()->p.getImage(treeViewItem.state.imageSelected);
    gtk_tree_store_set(parentTreeView()->p.gtkTreeStore, &iter, 0, gdkimage, -1);
}

auto pTreeViewItem::update(TreeViewItem* parent) -> void {
    addItem( parent );
    
    for(auto& item : treeViewItem.state.items) {
        item->state.parentTreeView = parentTreeView();
        item->p.update( &treeViewItem );
    }
    setExpanded( treeViewItem.expanded() );
}

auto pTreeViewItem::setImage(Image& image) -> void {
    if (!parentTreeView()) return;
    
    gdkimage = parentTreeView()->p.getImage(&image);    
    gtk_tree_store_set(parentTreeView()->p.gtkTreeStore, &iter, 0, gdkimage, -1);
}

auto pTreeViewItem::setImageSelected(Image& image) -> void {
    if (!parentTreeView() ) return;
    
    gdkimageSelected = parentTreeView()->p.getImage(&image);
}

auto pTreeViewItem::showImage(bool selected) -> void {
    if (selected && gdkimage && gdkimageSelected) {
        gtk_tree_store_set(parentTreeView()->p.gtkTreeStore, &iter, 0, gdkimageSelected, -1);  
    } else {        
        gtk_tree_store_set(parentTreeView()->p.gtkTreeStore, &iter, 0, gdkimage, -1);           
    }    
}

auto pTreeViewItem::find( char* _path ) -> TreeViewItem* {
    if (!parentTreeView()) return nullptr;
            
    auto path = gtk_tree_model_get_path(parentTreeView()->p.gtkTreeModel, &iter);
    bool match = path && gtk_tree_path_compare(path, gtk_tree_path_new_from_string(_path)) == 0;
    gtk_tree_path_free (path);
    
    if(match) return &treeViewItem;    

    for(auto child : treeViewItem.state.items) {
        TreeViewItem* item = child->p.find(_path);
        if (item) return item;
    }
    return nullptr;
}

auto pTreeView::append(TreeViewItem& item) -> void {
    item.state.parentTreeView = &treeView;
    item.p.update( nullptr );
}

auto pTreeView::remove(TreeViewItem& item) -> void {
    gtk_tree_store_remove(gtkTreeStore, &item.p.iter);
    item.p.invalidateParent();
}

auto pTreeView::reset() -> void {
    gtk_tree_store_clear(gtkTreeStore);
    for(auto item : treeView.state.items) item->p.invalidateParent();
    clearImages();
}

auto pTreeView::create() -> void {
    destroy();
    
    gtkWidget = gtk_scrolled_window_new(0, 0);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(gtkWidget), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(gtkWidget), GTK_SHADOW_ETCHED_IN);

    gtkTreeStore = gtk_tree_store_new(2, GDK_TYPE_PIXBUF, G_TYPE_STRING);
    gtkTreeModel = GTK_TREE_MODEL(gtkTreeStore);

    subWidget = gtk_tree_view_new_with_model(gtkTreeModel);
    gtkTreeView = GTK_TREE_VIEW(subWidget);
    gtkTreeSelection = gtk_tree_view_get_selection(gtkTreeView);
    gtk_tree_view_set_headers_visible(gtkTreeView, false);
	gtk_tree_view_set_enable_tree_lines(gtkTreeView, true);
    gtk_container_add(GTK_CONTAINER(gtkWidget), subWidget);
    gtk_widget_show(subWidget);

    gtkTreeViewColumn = gtk_tree_view_column_new();

    gtkCellPixbuf = gtk_cell_renderer_pixbuf_new();
    gtk_tree_view_column_pack_start(gtkTreeViewColumn, gtkCellPixbuf, false);
    gtk_tree_view_column_set_attributes(gtkTreeViewColumn, gtkCellPixbuf, "pixbuf", 0, nullptr);

    gtkCellText = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(gtkTreeViewColumn, gtkCellText, true);
    gtk_tree_view_column_set_attributes(gtkTreeViewColumn, gtkCellText, "text", 1, nullptr);

    gtk_tree_view_append_column(gtkTreeView, gtkTreeViewColumn);
    gtk_tree_view_set_search_column(gtkTreeView, 1);

    g_signal_connect(G_OBJECT(subWidget), "row-activated", G_CALLBACK(pTreeView::onActivate), (gpointer)&treeView);
    g_signal_connect(G_OBJECT(gtkTreeSelection), "changed", G_CALLBACK(pTreeView::onChange), (gpointer)&treeView);
}

auto pTreeView::destroy() -> void {
    if(subWidget) gtk_widget_destroy(subWidget);
    pWidget::destroy();
}

auto pTreeView::init() -> void {
    create();
	update();
	if (treeView.selected()) treeView.selected()->p.setSelected();
}

auto pTreeView::update() -> void {
    for(auto& item : treeView.state.items) {
        item->state.parentTreeView = &treeView;
        item->p.update( nullptr );
    }
}

auto pTreeView::onActivate(GtkTreeView* treeView, GtkTreePath* gtkPath, GtkTreeViewColumn* column, TreeView* self) -> void {
    char* path = gtk_tree_path_to_string(gtkPath);
    
    for(auto item : self->state.items) {
        auto treeViewItem = item->p.find( path );
        
        if (treeViewItem) {
			auto selected = self->state.selected;
			if (selected) {
				if (selected->itemCount() > 0) {
					bool expanded = gtk_tree_view_row_expanded (treeView, gtkPath);
					selected->p.setExpanded( !expanded );
				}
			}
            if (self->onActivate) self->onActivate();
            break;
        }
    }
    g_free(path);
}

auto pTreeView::onChange(GtkTreeSelection* selection, TreeView* self) -> void {
    if(self->state.items.empty()) return;
    
    GtkTreeIter iter;
    if(!gtk_tree_selection_get_selected(selection, &self->p.gtkTreeModel, &iter)) return;
    
    char* path = gtk_tree_model_get_string_from_iter(self->p.gtkTreeModel, &iter);

    for(auto item : self->state.items) {
        auto changed = item->p.find( path );      

        if (changed && (changed != self->state.selected) ) {
            if(self->state.selected) self->state.selected->p.showImage(false);
            changed->p.showImage(true);
                    
            self->state.selected = changed;
            if(self->onChange) self->onChange();
            break;
        }
    }
    g_free(path);
}

auto pTreeView::getImage(Image* image) -> GdkPixbuf* {
    if (!image || image->empty()) return nullptr;
    
    for(unsigned z = 0; z < images.size(); z++) {
        if (image == images[z]) {
            return gdkImages[z];
        }
    }  
    
    unsigned size = pFont::size(pfont, " ").height;
    auto gdkimage = CreatePixbuf(*image, size > 2 ? size-2 : size);
    
    if (!gdkimage) return nullptr;
        
    images.push_back(image);
    gdkImages.push_back(gdkimage);
    return gdkimage;
}

auto pTreeView::clearImages() -> void {
    for(auto gdkimage : gdkImages) {
        g_object_unref( G_OBJECT( gdkimage ) );
    }
    gdkImages.clear();
    images.clear();
}

auto pTreeView::focused() -> bool {
    return gtk_widget_has_focus(subWidget);
}

auto pTreeView::setFocused() -> void {
    gtk_widget_grab_focus(subWidget);
}

auto pTreeView::setBackgroundColor(unsigned color) -> void {
    if( !subWidget || !widget.overrideBackgroundColor() )
        return;

	std::string _color = "rgb(" + std::to_string( (color >> 16) & 0xff ) + ", " + std::to_string( (color >> 8) & 0xff )
		+ ", " + std::to_string( color & 0xff ) + ")";
	
	pSystem::applyCss( subWidget, "treeview { background-color: " + _color + " }" );
}

auto pTreeView::setForegroundColor(unsigned color) -> void {
    if( !subWidget || !widget.overrideForegroundColor() )
        return;

	std::string _color = "rgb(" + std::to_string( (color >> 16) & 0xff ) + ", " + std::to_string( (color >> 8) & 0xff )
		+ ", " + std::to_string( color & 0xff ) + ")";
	
	pSystem::applyCss( subWidget, "treeview { color: " + _color + " }" );
}
