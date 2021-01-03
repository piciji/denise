
auto Layout::synchronizeLayout() -> void {
    setGeometry( state.geometry );
}

auto Layout::has(Sizable& sizable) -> Children* {
    for (auto& child : children)
        if (child.sizable == &sizable)
            return &child;
    
    return nullptr;
}

auto Layout::update(Sizable& sizable, unsigned spacing) -> void {
    Children* child = has(sizable);

    if (!child)
        return;

    child->spacing = spacing;
}

auto Layout::update(Sizable& sizable, Size size, unsigned spacing) -> void {
    
    Children* child = has(sizable);
    
    if (!child)
        return;
        
    child->size = size;
    child->spacing = spacing;
}

auto Layout::append(Sizable& sizable, Size size, unsigned spacing) -> void {
	
    if (has(sizable))
        return;
    
    children.push_back({&sizable, size, {0,0}, spacing, 0});
    sizable.setVisible( visible() );
    
    updateLayout();
    //if(window()) window()->synchronizeLayout();
}

auto Layout::append(Sizable& sizable) -> void {
    sizable.state.parent = this;
    if (frameWidget) ((Sizable*)frameWidget)->state.parent = this;
    sizable.state.window = Sizable::state.window;

    if(dynamic_cast<Layout*>(&sizable)) {
        Layout& layout = (Layout&)sizable;
        layout.updateLayout();
    }

    if(dynamic_cast<Widget*>(&sizable)) {
        Widget& widget = (Widget&)sizable;
        if(sizable.window()) sizable.window()->append(widget);
    }
}

auto Layout::updateLayout() -> void {
    if (window() && frameWidget) window()->append(*frameWidget);

    if (window() && dynamic_cast<TabFrameLayout*>(this)) {
        TabFrameLayout* topTab = TabFrameLayout::getTopMostParentTabFrame(dynamic_cast<TabFrameLayout*>(this));
        topTab->setVisible();
    }
    for(auto& child : children) append(*child.sizable);
}

auto Layout::remove(Sizable& sizable) -> bool {
    unsigned pos = 0;
    for(auto& child : children) {
        if(child.sizable == &sizable) {
            if(dynamic_cast<Layout*>(child.sizable)) {
                ((Layout*)child.sizable)->reset();
            }
            children.erase(children.begin() + pos);
            cut(sizable);
            //if(window()) window()->synchronizeLayout();
            return true;
        }
        pos++;
    }
    return false;
}

auto Layout::cut(Sizable& sizable) -> void {
    if(dynamic_cast<Widget*>(&sizable)) {
        Widget& widget = (Widget&)sizable;
        if(sizable.window()) sizable.window()->remove(widget);
    }
    if(dynamic_cast<Layout*>(&sizable)) {
        Layout& layout = (Layout&)sizable;
		if (layout.frameWidget) {
			if (layout.window()) layout.window()->remove(*layout.frameWidget);
			((Sizable*)layout.frameWidget)->state.parent = nullptr;
		}
    }

    sizable.state.parent = nullptr;
    sizable.state.window = nullptr;
}

auto Layout::reset() -> void {
    if (!window()) return;    

    for(auto& child : children) {
        if(dynamic_cast<Layout*>(child.sizable)) ((Layout*)child.sizable)->reset();
        if(dynamic_cast<Widget*>(child.sizable)) window()->remove((Widget&)*child.sizable);
    }
    if (frameWidget) window()->remove(*frameWidget);
}

auto Layout::setEnabled(bool enabled) -> void {
    Sizable::state.enabled = enabled;
    if (frameWidget) frameWidget->setEnabled(enabled);

    for(auto& child : children) {
        child.sizable->setEnabled(enabled);
    }
}

auto Layout::setVisible(bool visible) -> void {
    Sizable::state.visible = visible;
    if (frameWidget) frameWidget->setVisible(visible);

    for(auto& child : children) {
        child.sizable->setVisible(visible);
    }
}

auto Layout::setMargin(unsigned margin) -> void {
    state.margin = margin;
}

auto Layout::setAlignment(double alignment) -> void {
    state.alignment = std::max(0.0, std::min(1.0, alignment));
}

auto Layout::addDisplacement(Geometry& geometry, unsigned offset) -> void {
    geometry.x      += offset;
    geometry.y      += offset;
    geometry.width  -= offset * 2;
    geometry.height -= offset * 2;
}

Layout::~Layout() {
    while(children.size()) remove(*children.at(0).sizable);
}

auto Layout::getFrameInnerGeometry(Geometry geometry) -> Geometry {
    unsigned borderSize = frameWidget->p.borderSize();
    Size minimumSize = frameWidget->p.minimumSize();

    geometry.x += borderSize, geometry.width -= borderSize << 1;
    geometry.y += minimumSize.height - borderSize;
    geometry.height -= minimumSize.height;
    return geometry;
}

auto Layout::addFrameSize(Size min) -> Size {
    unsigned borderSize = frameWidget->p.borderSize();
    Size minimumSize = frameWidget->p.minimumSize();

    min.width += borderSize << 1;
    min.width = std::max(min.width, minimumSize.width);
    min.height += minimumSize.height;
    return min;
}

auto FixedLayout::append(Widget& widget, Geometry geometry) -> void {
    for(auto& child : children) if(child.sizable == &widget) return;
    children.push_back({&widget, geometry.size(), geometry.position(), 0, 0});
    updateLayout();
    if(window()) window()->synchronizeLayout();
}

auto FixedLayout::setGeometry(Geometry geometry) -> void {   
    state.geometry = geometry;
    auto children = this->children;
    for(auto& child : children) {

        child.position.x = std::max(0, child.position.x);
        child.position.y = std::max(0, child.position.y);
        child.size.width = child.size.width == Size::Minimum ? child.sizable->minimumSize().width : child.size.width;
        child.size.height = child.size.height == Size::Minimum ? child.sizable->minimumSize().height : child.size.height;

        if (child.size.width >= geometry.width) {
            child.position.x = 0;
            child.size.width = geometry.width;
        } else if ((child.position.x + child.size.width) > geometry.width) {
            child.position.x -= (child.position.x + child.size.width) - geometry.width;
        }
        if (child.size.height >= geometry.height) {
            child.position.y = 0;
            child.size.height = geometry.height;
        } else if ((child.position.y + child.size.height) > geometry.height) {
            child.position.y -= (child.position.y + child.size.height) - geometry.height;
        }

        int x = child.position.x + geometry.x;
        int y = child.position.y + geometry.y;

        child.sizable->setGeometry({x, y, child.size.width, child.size.height});
    }
}

auto HorizontalLayout::minimumSize() -> Size {
    Size min;

    for(auto& child : children) {
        min.width += child.spacing;
        if(child.size.width == Size::Minimum || child.size.width == Size::Maximum) {
            min.width += child.sizable->minimumSize().width;
            continue;
        }
        min.width += child.size.width;
    }

    for(auto& child : children) {
        if(child.size.height == Size::Minimum || child.size.height == Size::Maximum) {
            min.height = std::max(min.height, child.sizable->minimumSize().height);
            continue;
        }
        min.height = std::max(min.height, child.size.height);
    }

    if(frameWidget) min = addFrameSize(min);
    min.width += (state.margin + state.padding) * 2;
    min.height += (state.margin + state.padding) * 2;

    return min;
}

auto HorizontalLayout::setGeometry(Geometry containerGeometry) -> void {
    state.geometry = containerGeometry;
    auto children = this->children;
    for(auto& child : children) {
        if(child.size.width  == Size::Minimum) child.size.width  = child.sizable->minimumSize().width;
        if(child.size.height == Size::Minimum) child.size.height = child.sizable->minimumSize().height;
    }

    Geometry geometry = containerGeometry;
    addDisplacement(geometry, state.margin);

    if (frameWidget) {
        frameWidget->setGeometry(geometry);
        geometry = getFrameInnerGeometry(geometry);
        addDisplacement(geometry, state.padding);
    }

    unsigned minimumWidth = 0, maximumWidthCounter = 0;
    for(auto& child : children) {
        if(child.size.width == Size::Maximum) maximumWidthCounter++;
        if(child.size.width != Size::Maximum) minimumWidth += child.size.width;
        minimumWidth += child.spacing;
    }

    for(auto& child : children) {
        if(child.size.width  == Size::Maximum) child.size.width  = (geometry.width - minimumWidth) / maximumWidthCounter;
        if(child.size.height == Size::Maximum) child.size.height = geometry.height;
    }

    unsigned maximumHeight = 0;
    for(auto& child : children) maximumHeight = std::max(maximumHeight, child.size.height);

    for(auto& child : children) {
        int pivot = (maximumHeight - child.size.height) * state.alignment;
        Geometry childGeometry = {geometry.x, geometry.y + pivot, child.size.width, child.size.height};
        if((signed)childGeometry.width < 1) childGeometry.width = 1;
        if((signed)childGeometry.height < 1) childGeometry.height = 1;
        child.sizable->setGeometry(childGeometry);

        geometry.x += child.size.width + child.spacing;
        geometry.width -= child.size.width + child.spacing;
    }
}

auto VerticalLayout::minimumSize() -> Size {
    Size min;

    for(auto& child : children) {
        if(child.size.width == Size::Minimum || child.size.width == Size::Maximum) {
            min.width = std::max(min.width, child.sizable->minimumSize().width);
            continue;
        }
        min.width = std::max(min.width, child.size.width);
    }

    for(auto& child : children) {
        min.height += child.spacing;
        if(child.size.height == Size::Minimum || child.size.height == Size::Maximum) {
            min.height += child.sizable->minimumSize().height;
            continue;
        }
        min.height += child.size.height;
    }

    if(frameWidget) min = addFrameSize(min);
    min.width += (state.margin + state.padding) * 2;
    min.height += (state.margin + state.padding) * 2;

    return min;
}

auto VerticalLayout::setGeometry(Geometry containerGeometry) -> void {
    state.geometry = containerGeometry;
    auto children = this->children;
    for(auto& child : children) {
        if(child.size.width  == Size::Minimum) child.size.width  = child.sizable->minimumSize().width;
        if(child.size.height == Size::Minimum) child.size.height = child.sizable->minimumSize().height;
    }

    Geometry geometry = containerGeometry;
    addDisplacement(geometry, state.margin);

    if (frameWidget) {
        frameWidget->setGeometry(geometry);
        geometry = getFrameInnerGeometry(geometry);
        addDisplacement(geometry, state.padding);
    }

    unsigned minimumHeight = 0, maximumHeightCounter = 0;
    for(auto& child : children) {
        if(child.size.height == Size::Maximum) maximumHeightCounter++;
        if(child.size.height != Size::Maximum) minimumHeight += child.size.height;
        minimumHeight += child.spacing;
    }

    for(auto& child : children) {
        if(child.size.width  == Size::Maximum) child.size.width  = geometry.width;
        if(child.size.height == Size::Maximum) child.size.height = (geometry.height - minimumHeight) / maximumHeightCounter;
    }

    unsigned maximumWidth = 0;
    for(auto& child : children) maximumWidth = std::max(maximumWidth, child.size.width);

    for(auto& child : children) {
        int pivot = (maximumWidth - child.size.width) * state.alignment;
        Geometry childGeometry = {geometry.x + pivot, geometry.y, child.size.width, child.size.height};
        if((signed)childGeometry.width < 1) childGeometry.width = 1;
        if((signed)childGeometry.height < 1) childGeometry.height = 1;
        child.sizable->setGeometry(childGeometry);

        geometry.y += child.size.height + child.spacing;
        geometry.height -= child.size.height + child.spacing;
    }
}

FramedHorizontalLayout::Frame::Frame() : Widget(*new pFrame(*this)), p((pFrame&)Widget::p) {
    state.isContainer = true;
    p.init();
}

auto FramedHorizontalLayout::setPadding(unsigned padding) -> void {
	state.padding = pWidget::getScaledDim( padding );
}

auto FramedHorizontalLayout::setForegroundColor(unsigned color) -> void {
    frameWidget->p.widget.setForegroundColor( color );
}

FramedHorizontalLayout::FramedHorizontalLayout() { frameWidget = new Frame; }

FramedVerticalLayout::Frame::Frame() : Widget(*new pFrame(*this)), p((pFrame&)Widget::p) {
    state.isContainer = true;
    p.init();
}

auto FramedVerticalLayout::setPadding(unsigned padding) -> void {
	state.padding = pWidget::getScaledDim( padding );
}

auto FramedVerticalLayout::setForegroundColor(unsigned color) -> void {
    frameWidget->p.widget.setForegroundColor( color );
}

FramedVerticalLayout::FramedVerticalLayout() { frameWidget = new Frame; }

auto FramedHorizontalLayout::setFont(const std::string& font) -> void { frameWidget->setFont(font); }
auto FramedVerticalLayout::setFont(const std::string& font) -> void { frameWidget->setFont(font); }
auto FramedHorizontalLayout::setText(const std::string& text) -> void { frameWidget->setText(text); }
auto FramedVerticalLayout::setText(const std::string& text) -> void { frameWidget->setText(text); }

//tab frame layout
auto TabFrameLayout::getParentTabFrame(Sizable* sizable) -> TabFrameLayout* {
    //if (dynamic_cast<TabFrame*>(sizable)) return nullptr;

	if ( dynamic_cast<Widget*>(sizable) && ((Widget*)sizable)->isContainer( ) )
		sizable = sizable->parent();

    while(sizable) {
        if(sizable->parent() && dynamic_cast<TabFrameLayout*>(sizable->parent())) {
            return (TabFrameLayout*)sizable->parent();
        }
        sizable = sizable->state.parent;
    }
    return nullptr;
}

auto TabFrameLayout::getTopMostParentTabFrame(TabFrameLayout* tab) -> TabFrameLayout* {
    while( TabFrameLayout* topTabTemp = TabFrameLayout::getParentTabFrame(tab) ) { tab = topTabTemp; }
    return tab;
}

auto Layout::getParentWidget( Widget* widget, int& selection) -> Widget* {
    
    Sizable* sizable = (Sizable*)widget;
    
    if(widget->isContainer())
        sizable = sizable->state.parent;
    
    while(sizable) {
        
        if (sizable->parent()) {
            
            if (dynamic_cast<TabFrameLayout*>(sizable->parent())) {
                auto tabLayout = dynamic_cast<TabFrameLayout*>(sizable->parent());

                for (auto& child : tabLayout->children) {
                    if (child.sizable == sizable) {
                        selection = child.selection;
                        break;
                    }
                }
                return tabLayout->frameWidget;
                
            } else if (dynamic_cast<FramedVerticalLayout*>(sizable->parent())) {
                auto layout = dynamic_cast<FramedVerticalLayout*>(sizable->parent());
                return layout->frameWidget;
                
            } else if (dynamic_cast<FramedHorizontalLayout*>(sizable->parent())) {
                auto layout = dynamic_cast<FramedHorizontalLayout*>(sizable->parent());
                return layout->frameWidget;
            }
        }
        
        sizable = sizable->state.parent;
    }
    
    return nullptr;
}

TabFrameLayout::TabFrame::TabFrame() : Widget(*new pTabFrame(*this)), p((pTabFrame&)Widget::p) {
    Widget::state.isContainer = true;
    p.init();
}

TabFrameLayout::TabFrameLayout() {
    frameWidget = new TabFrame;
    getTabFrame()->onChange = [this]() {
        TabFrameLayout* topTab = getTopMostParentTabFrame(this);
        topTab->setVisible();
        if(onChange) onChange();
    };
}

auto TabFrameLayout::setLayout(unsigned selection, Layout& layout, Size size) -> void {
    bool found = false;
    for(auto& child : children) {
        if(selection == child.selection) {
            child.sizable = &layout;
            found = true;
            break;
        }
    }
    if(!found) children.push_back({&layout, size, {0,0}, 0, selection});
    updateLayout();
    //if(window()) window()->synchronizeLayout();
}

auto TabFrameLayout::remove(unsigned selection) -> void {
    if(selection < getTabFrame()->tabs()) {
        getTabFrame()->state.header.erase(getTabFrame()->state.header.begin() + selection);
        getTabFrame()->state.images.erase(getTabFrame()->state.images.begin() + selection);
        getTabFrame()->p.remove(selection);
    }

    for(auto& child : children) {
        if(selection == child.selection) {
            Layout::remove(*child.sizable);
            break;
        }
    }
}

auto TabFrameLayout::minimumSize() -> Size {
    Size min;

    for(auto& child : children) {
        min.width  = std::max(min.width,  child.sizable->minimumSize().width);
        min.height = std::max(min.height, child.sizable->minimumSize().height);
    }

    min = addFrameSize(min);

    min.width += (state.margin + state.padding) * 2;
    min.height += (state.margin + state.padding) * 2;

    return min;
}

auto TabFrameLayout::setGeometry(Geometry containerGeometry) -> void {
    state.geometry = containerGeometry;
    Geometry geometry = containerGeometry;

    addDisplacement(geometry, state.margin);
    frameWidget->setGeometry(geometry);
    geometry = getFrameInnerGeometry(geometry);
    addDisplacement(geometry, state.padding);

    auto children = this->children;
    for(auto& child : children) {
        if(child.size.width  == Size::Minimum) child.size.width  = child.sizable->minimumSize().width;
        if(child.size.height == Size::Minimum) child.size.height = child.sizable->minimumSize().height;

        child.size.width = std::min(child.size.width, geometry.width);
        child.size.height = std::min(child.size.height, geometry.height);

        Geometry childGeometry = {geometry.x, geometry.y, child.size.width, child.size.height};
        child.sizable->setGeometry(childGeometry);
    }
}

auto TabFrameLayout::setVisible(bool visible) -> void {
    Sizable::state.visible = visible;
    getTabFrame()->setVisible(visible);

    std::vector<Children*> temp;
    
    for(auto& child : children) {
        bool v = visible && (getTabFrame()->state.selection == child.selection);
        if (!v)
            child.sizable->setVisible(false);
        else
            temp.push_back( &child );
    }
    
    for(auto child : temp)
        child->sizable->setVisible(true);
}

auto TabFrameLayout::appendHeader(std::string text, Image* image) -> void {
    getTabFrame()->state.header.push_back(text);
    getTabFrame()->state.images.push_back(image);
    getTabFrame()->p.append(text, image);
}

auto TabFrameLayout::setHeader(unsigned selection, std::string text) -> void {
    if(selection >= getTabFrame()->tabs()) return;
    getTabFrame()->state.header.at(selection) = text;
    getTabFrame()->p.setText(selection, text);
}

auto TabFrameLayout::header(unsigned selection) const -> std::string {
    return getTabFrame()->text(selection);
}

auto TabFrameLayout::setSelection(unsigned selection) -> void {
    getTabFrame()->state.selection = selection;
    getTabFrame()->p.setSelection(selection);
    TabFrameLayout* topTab = getTopMostParentTabFrame(this);
    topTab->setVisible();
}

auto TabFrameLayout::selection() const -> unsigned { return getTabFrame()->selection(); }

auto TabFrameLayout::setFont(const std::string& font) -> void { getTabFrame()->setFont(font); }

auto TabFrameLayout::setImage(unsigned selection, Image& image) -> void {
    if(selection >= getTabFrame()->tabs()) return;
    getTabFrame()->state.images.at(selection) = &image;
    getTabFrame()->p.setImage(selection, image);
}

// switch layout

auto SwitchLayout::setLayout(unsigned selection, Layout& layout, Size size) -> void {
    bool found = false;
    for(auto& child : children) {
        if(selection == child.selection) {
            child.sizable = &layout;
            found = true;
            break;
        }
    }
    if(!found) children.push_back({&layout, size, {0,0}, 0, selection});
    updateLayout();
   // if(window()) window()->synchronizeLayout();
}

auto SwitchLayout::remove(unsigned selection) -> void {

    for(auto& child : children) {
        if(selection == child.selection) {
            Layout::remove(*child.sizable);
            break;
        }
    }
}

auto SwitchLayout::setSelection(unsigned selection) -> void {
    state.selection = selection;    
    Layout* top = Layout::getTopMostTabOrSwitchLayout(this);        
    top->setVisible();
}

auto SwitchLayout::selection() const -> unsigned { return state.selection; }

auto SwitchLayout::setVisible(bool visible) -> void {
    Sizable::state.visible = visible;
    
    std::vector<Children*> temp;
    
    for(auto& child : children) {
        bool v = visible && (state.selection == child.selection);
        if (!v)
            child.sizable->setVisible(false);
        else
            temp.push_back( &child );
    }
    
    for(auto child : temp)
        child->sizable->setVisible(true);
}

auto SwitchLayout::setGeometry(Geometry containerGeometry) -> void {
    Layout::state.geometry = containerGeometry;
    Geometry geometry = containerGeometry;

    addDisplacement(geometry, Layout::state.margin);    

    auto children = this->children;
    for(auto& child : children) {
        if(child.size.width  == Size::Minimum) child.size.width  = child.sizable->minimumSize().width;
        if(child.size.height == Size::Minimum) child.size.height = child.sizable->minimumSize().height;

        child.size.width = std::min(child.size.width, geometry.width);
        child.size.height = std::min(child.size.height, geometry.height);

        Geometry childGeometry = {geometry.x, geometry.y, child.size.width, child.size.height};
        child.sizable->setGeometry(childGeometry);
    }
}

auto SwitchLayout::minimumSize() -> Size {
    Size min;

    for(auto& child : children) {
        min.width  = std::max(min.width,  child.sizable->minimumSize().width);
        min.height = std::max(min.height, child.sizable->minimumSize().height);
    }

    min.width += Layout::state.margin * 2;
    min.height += Layout::state.margin * 2;

    return min;
}



auto Layout::getParentTabOrSwitchLayout(Sizable* sizable) -> Layout* {
    if (dynamic_cast<TabFrameLayout::TabFrame*>(sizable)) return nullptr;

    while(sizable) {
        if(sizable->parent() && dynamic_cast<TabFrameLayout*>(sizable->parent()))
            return (Layout*)sizable->parent();
        
        if(sizable->parent() && dynamic_cast<SwitchLayout*>(sizable->parent()))
            return (Layout*)sizable->parent();       
        
        sizable = sizable->state.parent;
    }
    
    return nullptr;
}

auto Layout::getTopMostTabOrSwitchLayout(Layout* layout) -> Layout* {
    
    while( Layout* topLayout = Layout::getParentTabOrSwitchLayout(layout) )
        layout = topLayout;   
    
    return layout;
}

auto Layout::getAllChildWidgets() -> std::vector<Widget*> {
    
    std::vector<Widget*> widgets;
    
    for( auto& child : children ) {
        
        if (dynamic_cast<Widget*>(child.sizable)) {
            
            widgets.push_back( (Widget*)(child.sizable) );
            
        } else if (dynamic_cast<Layout*>(child.sizable)) {
            
            auto in = ((Layout*)(child.sizable))->getAllChildWidgets();
            
            widgets = Vector::concat( widgets, in );
        }
    }
    
    return widgets;
}

auto HorizontalLayout::alignChildrenVertically( std::vector<HorizontalLayout*> layouts ) -> void {
    
    std::vector<unsigned> neededWidths;
    
    for (auto layout : layouts) {
        
        unsigned i = 0;
        for (auto& child : layout->children) {
            if (i >= neededWidths.size())
                neededWidths.push_back(0);
            
            neededWidths[i] = std::max(neededWidths[i], child.sizable->minimumSize().width);
            
            i++;
        }                
    }

    for (auto layout : layouts) {

        unsigned i = 0;
        
        for (auto& child : layout->children) {
            child.size.width = neededWidths[i];
            i++;
        }
    }
}