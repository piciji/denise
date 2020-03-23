
#include <cstring>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <sstream>

#include "api.h"

#ifdef GUIKIT_WINAPI
    #include "winapi/main.cpp"
#elif GUIKIT_GTK
    #include "gtk/main.cpp"
#elif GUIKIT_COCOA
    #include "cocoa/main.cpp"
#else
    #error no GUI KIT found
#endif

namespace GUIKIT {

#include "tools/file/file.cpp"
#include "tools/setting.cpp"
#include "tools/translate.cpp"
#include "tools/image.cpp"
#include "tools/string.cpp"
#include "tools/utf8.cpp"
#include "tools/layout.cpp"

std::vector<Base*> Base::objects;

auto Base::find(unsigned id) -> Base* {
    id -= 100;
    if (objects.size() <= id )
        return nullptr;
    
    return objects[id];
}

Base::Base() {
    id = objects.size() + 100;
    objects.push_back(this);
    Application::initialize();
}

//application
bool Application::isQuit = false;
int Application::exitCode = 0;
bool Application::dummy = false;
std::string Application::name = "";
std::function<void ()> Application::loop = nullptr;
std::function<void ()> Application::Cocoa::onAbout;
std::function<void ()> Application::Cocoa::onPreferences;
std::function<void ()> Application::Cocoa::onCustom1;
std::function<void ()> Application::Cocoa::onQuit;
std::function<void ()> Application::Cocoa::onDock;
std::function<void (std::string fileName)> Application::Cocoa::onOpenFile;

typedef Application _A;

auto Application::isCocoa() -> bool {
#ifdef GUIKIT_COCOA
    return true;
#else
    return false;
#endif
}

auto Application::isGtk() -> bool {
#ifdef GUIKIT_GTK
    return true;
#else
    return false;
#endif    
}

auto Application::isWinApi() -> bool {
#ifdef GUIKIT_WINAPI
    return true;
#else
    return false;
#endif    
}

auto Application::run() -> void {
    pApplication::run();
}

auto Application::processEvents() -> void {
    pApplication::processEvents();
}

auto Application::quit() -> void {
    isQuit = true;
    pApplication::quit();
    MenuSeparator::cleanInstances();
}

auto Application::initialize() -> void {
    static bool initialized = false;

    if(!initialized) {
        initialized = true;
        pApplication::initialize();
    }
}

//window
std::vector<CustomFont*> Window::customFonts;

Window::Window() : p(*new pWindow(*this)), Base(), cocoa(*this), winapi(*this) {
    state.widgetFont = Font::system();
}

Window::~Window() { 
    delete &p;
    delete focusTimer;
}

auto Window::append(Menu& menu) -> void {
    if (_A::dummy) return;
    state.menus.push_back(&menu);
    menu.state.parentWindow = this;
    p.append(menu);
}

auto Window::remove(Menu& menu) -> void {
    if (_A::dummy) return;
    if (Vector::eraseVectorElement<Menu*>(state.menus, &menu)) {
        p.remove(menu);
        menu.state.parentWindow = nullptr;
    }
}

auto Window::isApended(Menu& menu) -> bool {
    
    for( auto apendedMenu : state.menus ) {        
        if (apendedMenu == &menu)
            return true;
    }
    return false;
}

auto Window::append(Widget& widget) -> void {
    if (_A::dummy) return;
    widget.Sizable::state.window = this;
    p.append(widget);
}

auto Window::remove(Widget& widget) -> void {
    if (_A::dummy) return;
    p.remove(widget);
    widget.Sizable::state.window = nullptr;
}

auto Window::append(Layout& layout) -> void {
    if (_A::dummy) return;
    if (state.layout) remove(layout);
    state.layout = &layout;
    layout.Sizable::state.parent = nullptr;
    layout.Sizable::state.window = this;
    layout.synchronizeLayout();
    p.append(layout);
}

auto Window::remove(Layout& layout) -> void {
    if (_A::dummy) return;
    state.layout = nullptr;
    p.remove(layout);
    layout.reset();
    layout.Sizable::state.window = nullptr;
}

auto Window::addCustomFont( CustomFont* customFont ) -> bool {	
    if (_A::dummy) return false;
	customFonts.push_back( customFont );	
	return pWindow::addCustomFont( customFont );
}

auto Window::setWidgetFont(const std::string& font) -> void {
    state.widgetFont = font;
}

auto Window::setDroppable(bool droppable) -> void {
    if (_A::dummy) return;
    state.droppable = droppable;
    p.setDroppable(droppable);
}

auto Window::setBackgroundColor(unsigned color) -> void {
    if (_A::dummy) return;
    p.setBackgroundColor(color);
}

auto Window::setVisible(bool visible) -> void {
    if (_A::dummy) return;
    state.visible = visible;
    synchronizeLayout();
    p.setVisible(visible);
}

auto Window::setFocused() -> void {
    if (_A::dummy) return;
    p.setFocused();
}

auto Window::setFocused(unsigned delay) -> void {
    if (_A::dummy) return;
    if (!focusTimer)
        focusTimer = new Timer;
    
	focusTimer->setInterval(100);
	
	focusTimer->onFinished = [this]() {
		focusTimer->setEnabled(false);		
		p.setFocused();        
	};
	focusTimer->setEnabled();    
}

auto Window::setStatusFont(const std::string& font) -> void {
    if (_A::dummy) return;
    p.setStatusFont(font);
}

auto Window::setTitle(const std::string& text) -> void {
    if (_A::dummy) return;
    state.title = text;
    p.setTitle(text);
}

auto Window::setStatusText(const std::string& text) -> void {
    if (_A::dummy) return;
    state.statusText = text;
    p.setStatusText(text);
}

auto Window::setStatusVisible(bool visible) -> void {
    if (_A::dummy) return;
    state.statusVisible = visible;
    p.setStatusVisible(visible);
}

auto Window::setMenuVisible(bool visible) -> void {
    if (_A::dummy) return;
    if (Application::isCocoa() && !visible && fullScreen())
        // macOS hides the menu in fullscreen.
        // moving mouse to upper screen border make it visible
        // so don't prevent this behaviour 
        return;
    state.menuVisible = visible;
    p.setMenuVisible(visible);
}
    
auto Window::setFullScreen(bool fullScreen) -> void {
    if (_A::dummy) return;
    if (p.fullScreenToggleDelayed()) {
        return;
    }
    state.fullScreen = fullScreen;
    p.setFullScreen(fullScreen);
}

auto Window::setResizable(bool resizable) -> void {
    if (_A::dummy) return;
    state.resizable = resizable;
    p.setResizable(resizable);
}

auto Window::setGeometry(Geometry geometry) -> void {
    if (_A::dummy) return;
    geometry.width = std::min(geometry.width, pSystem::getDesktopSize().width);
    geometry.height = std::min(geometry.height, pSystem::getDesktopSize().height);
    state.geometry = geometry;
    p.setGeometry(geometry);
}

auto Window::isOffscreen() -> bool { 
    if (_A::dummy) return false;
    return p.isOffscreen();
}

auto Window::synchronizeLayout() -> void {
    if (_A::dummy) return;
    if(visible() && !Application::isQuit) p.setGeometry(geometry());
}

auto Window::focused() -> bool {
    if (_A::dummy) return true;
    return p.focused();
}

auto Window::geometry() -> Geometry {
    if (_A::dummy) return {0,0,0,0};
    return p.geometry();
}

auto Window::changeCursor( Image& image, unsigned hotSpotX, unsigned hotSpotY ) -> void {
    if (_A::dummy) return;
    if (state.cursorImage == &image)
        return;
    
    state.cursorImage = &image;
    
    cursor = image.empty() ? Cursor::Default : Cursor::Image;
    
    p.changeCursor( image, hotSpotX, hotSpotY );
}

auto Window::setDefaultCursor( ) -> void {
    if (_A::dummy) return;
   // if (!state.cursorImage)
     //   return;
    
    if (cursor == Cursor::Default)
        return;
    
    cursor = Cursor::Default;
    
    state.cursorImage = nullptr;
    
    p.setDefaultCursor();
}

auto Window::setPointerCursor( ) -> void {
    if (_A::dummy) return;
    
    if (cursor == Cursor::Pointer)
        return;
    
    cursor = Cursor::Pointer;
    
    state.cursorImage = nullptr;
    
    p.setPointerCursor();
}

auto Window::handle() -> uintptr_t {
    if (_A::dummy) return 0;
    return p.handle();
}

auto Window::Cocoa::setTitleForAppMenuItem(AppMenuItem appMenuItem, std::string title) -> void {
    if (_A::dummy) return;
#if GUIKIT_COCOA
    window.p.setTitleForAppMenuItem(appMenuItem, title);
#endif
}

auto Window::Cocoa::setHiddenForAppMenuItem(AppMenuItem appMenuItem, bool state) -> void {
    if (_A::dummy) return;
#if GUIKIT_COCOA
    window.p.setHiddenForAppMenuItem(appMenuItem, state);
#endif
}
    
auto Window::Cocoa::keepMenuVisibilityOnDisplay(bool state) -> void {
    if (_A::dummy) return;
#if GUIKIT_COCOA
    window.p.keepMenuVisibilityOnDisplay( state );
#endif
}

auto Window::Cocoa::setDisableIconsInTopMenu(bool state) -> void {
    if (_A::dummy) return;
#if GUIKIT_COCOA
    window.p.disableIconsInTopMenu = state;
#endif
}
    
auto Window::Winapi::disableBackgroundRedrawDuringResize(bool state) -> void {
    if (_A::dummy) return;
    #ifdef GUIKIT_WINAPI
        window.p.bgUpdateState = state ? 1 : 0;
    #endif
}

//widgets
auto Widget::focused() -> bool {
    if (_A::dummy) return false;
    return p.focused();
}

auto Widget::setEnabled(bool enabled) -> void {
    if (_A::dummy) return;
    Sizable::state.enabled = enabled;
    p.setEnabled(enabled);
}

auto Widget::setFocused() -> void {
    if (_A::dummy) return;
    return p.setFocused();
}

auto Widget::setFont(const std::string& font) -> void {
    if (_A::dummy) return;
    state.font = font;
    p.setFont(font);
}

auto Widget::font() -> std::string {
    return !state.font.empty() ? state.font :
    ( window() ? window()->widgetFont() : Font::system() );
}

auto Widget::setGeometry(Geometry geometry) -> void {
    if (_A::dummy) return;
    state.geometry = geometry;
    p.setGeometry(geometry);
}

auto Widget::setVisible(bool visible) -> void {
    if (_A::dummy) return;
    Sizable::state.visible = visible;
    p.setVisible(visible);
}

auto Widget::minimumSize() -> Size {
    if (_A::dummy) return {0,0};
    return p.minimumSize();
}

auto Widget::setText(const std::string& text) -> void {
    if (_A::dummy) return;
    state.text = text;
    p.setText(text);
}

auto Widget::setTooltip(const std::string& tooltip) -> void {
    if (_A::dummy) return;
    state.tooltip = tooltip;
    p.setTooltip(tooltip);
}

auto Widget::setBackgroundColor(unsigned color) -> void {
    if (_A::dummy) return;
    state.overrideBackgroundColor = true;
    state.backgroundColor = color;
    p.setBackgroundColor(color);
}

auto Widget::setForegroundColor(unsigned color) -> void {
    if (_A::dummy) return;
    state.overrideForegroundColor = true;
    state.foregroundColor = color;
    p.setForegroundColor(color);
}

Widget::Widget() : p(*new pWidget(*this)), Sizable() { }
Widget::Widget(pWidget& p) : p(p), Sizable() { }
Widget::~Widget() { delete &p; }

auto LineEdit::setEditable(bool editable) -> void {
    if (_A::dummy) return;
    state.editable = editable;
    p.setEditable(editable);
}

auto LineEdit::setDroppable(bool droppable) -> void {
    if (_A::dummy) return;
    state.droppable = droppable;
    p.setDroppable(droppable);
}

auto LineEdit::text() -> std::string { 
    if (_A::dummy) return "";
    return p.text();
}

auto LineEdit::value() -> int {
    if (_A::dummy) return 0;
	try {
        return std::stoi( p.text() );
    } catch( ... ) {
		return 0;
	}
}

auto LineEdit::setValue(int value) -> void {
	setText( std::to_string( value ) );
}

auto LineEdit::setMaxLength( unsigned maxLength ) -> void {
    if (_A::dummy) return;
    state.maxLength = maxLength;
    p.setMaxLength( maxLength );
}

LineEdit::LineEdit() : Widget(*new pLineEdit(*this)), p((pLineEdit&)Widget::p) { if (!_A::dummy) p.init(); }

auto Label::setAlign( Align align ) -> void {
    if (_A::dummy) return;
    state.align = align;
    p.setAlign( align );
}

Label::Label() : Widget(*new pLabel(*this)), p((pLabel&)Widget::p) { if (!_A::dummy) p.init(); }

auto Hyperlink::setUri( std::string uri, std::string wrap ) -> void {
    if (_A::dummy) return;
	state.uri = uri;
	state.wrap = wrap;
	
	p.setUri( uri, wrap );
}

Hyperlink::Hyperlink() : Widget(*new pHyperlink(*this)), p((pHyperlink&)Widget::p) { if (!_A::dummy) p.init(); }

auto SquareCanvas::setBorderColor(unsigned borderSize, unsigned borderColor) -> void {
    if (_A::dummy) return;
    state.borderSize = borderSize;
    state.borderColor = borderColor;
    p.setBorderColor( borderSize, borderColor );
}

SquareCanvas::SquareCanvas() : Widget(*new pSquareCanvas(*this)), p((pSquareCanvas&)Widget::p) { if (!_A::dummy) p.init(); }

Button::Button() : Widget(*new pButton(*this)), p((pButton&)Widget::p) { if (!_A::dummy) p.init(); }

auto CheckButton::setChecked(bool checked) -> void {
    if (_A::dummy) return;
    state.checked = checked;
    p.setChecked(checked);
}

CheckButton::CheckButton() : Widget(*new pCheckButton(*this)), p((pCheckButton&)Widget::p) { if (!_A::dummy) p.init(); }

auto CheckBox::setChecked(bool checked) -> void {
    if (_A::dummy) return;
    state.checked = checked;
    p.setChecked(checked);
}

auto CheckBox::toggle() -> void {
    if (_A::dummy) return;
    state.checked ^= 1;
    p.setChecked(state.checked);
    if(onToggle) onToggle();
}

CheckBox::CheckBox() : Widget(*new pCheckBox(*this)), p((pCheckBox&)Widget::p) { if (!_A::dummy) p.init(); }

auto ComboButton::append(const std::string& text, int userData) -> void {
    if (_A::dummy) return;
    state.rows.push_back(text);
    state.userData.push_back(userData);
    p.append(text);
}

auto ComboButton::remove(unsigned selection) -> void {
    if (_A::dummy) return;
    if(selection >= state.rows.size()) return;
    state.rows.erase(state.rows.begin() + selection);
    state.userData.erase(state.userData.begin() + selection);
    p.remove(selection);
}

auto ComboButton::reset() -> void {
    if (_A::dummy) return;
    state.selection = 0;
    state.rows.clear();
    state.userData.clear();
    p.reset();
}

auto ComboButton::setSelection(unsigned selection) -> void {
    if (_A::dummy) return;
    if(selection >= state.rows.size()) return;
    state.selection = selection;
    p.setSelection(selection);
}

auto ComboButton::setText(unsigned selection, const std::string& text) -> void {
    if (_A::dummy) return;
    if(selection >= state.rows.size()) return;
    state.rows.at(selection) = text;
    p.setText(selection, text);
}

auto ComboButton::text(unsigned selection) const -> std::string {
    if(selection >= state.rows.size()) return "";
    return state.rows[selection];
}

auto ComboButton::setUserData(unsigned selection, int userData) -> void {
    if(selection >= state.userData.size()) return;
    state.userData[selection] = userData;
}

auto ComboButton::userData(unsigned selection) const -> int {
    if(selection >= state.userData.size()) return 0;
    return state.userData[selection];
}

ComboButton::ComboButton() : Widget(*new pComboButton(*this)), p((pComboButton&)Widget::p) { if (!_A::dummy) p.init(); }

auto Slider::setLength(unsigned length) -> void {
    if (_A::dummy) return;
    state.length = length;
    p.setLength(length);
}

auto Slider::setPosition(unsigned position) -> void {
    if (_A::dummy) return;
    state.position = position;
    p.setPosition(position);
}

Slider::Slider(Orientation orientation) : orientation(orientation), Widget(*new pSlider(*this)), p((pSlider&)Widget::p) { if (!_A::dummy) p.init(); }

auto RadioBox::setGroup(std::vector<RadioBox*> group) -> void {
    if (_A::dummy) return;
    for(auto& item : group) item->p.setGroup( item->state.group = group );
    if (group.size()) group.at(0)->setChecked();
}

auto RadioBox::setChecked() -> void {
    if (_A::dummy) return;
    for(auto& item : state.group) item->state.checked = false;
    state.checked = true;
    p.setChecked();
}

auto RadioBox::activate() -> void {
    if (_A::dummy) return;
    setChecked();
    if(onActivate) onActivate();
}

RadioBox::RadioBox() : Widget(*new pRadioBox(*this)), p((pRadioBox&)Widget::p) { if (!_A::dummy) p.init(); }

auto ProgressBar::setPosition(unsigned position) -> void {
    if (_A::dummy) return;
    state.position = position;
    p.setPosition(position);
}

ProgressBar::ProgressBar() : Widget(*new pProgressBar(*this)), p((pProgressBar&)Widget::p) { if (!_A::dummy) p.init(); }

auto ListView::append(const std::vector<std::string>& row) -> void {
    if (_A::dummy) return;
    state.rows.push_back(row);
    std::vector<Image*> images;
    for (unsigned i = 0; i < row.size(); i++) images.push_back(nullptr);
    state.images.push_back(images);
    p.append(row);
}

auto ListView::remove(unsigned selection) -> void {
    if (_A::dummy) return;
    if(selection >= state.rows.size()) return;
    state.rows.erase(state.rows.begin() + selection);
    state.images.erase(state.images.begin() + selection);
    p.remove(selection);
}

auto ListView::reset() -> void {
    if (_A::dummy) return;
    state.selected = false;
    state.selection = 0;
    state.rows.clear();
    state.images.clear();
    p.reset();
}

auto ListView::setSelection(unsigned selection) -> void {
    if (_A::dummy) return;
    if(selection >= state.rows.size()) return;
    state.selected = true;
    state.selection = selection;
    p.setSelection(selection);
}

auto ListView::setSelected(bool selected) -> void {
    if (_A::dummy) return;
    state.selected = selected;
    p.setSelected(selected);
}

auto ListView::setHeaderVisible(bool visible) -> void {
    if (_A::dummy) return;
    state.headerVisible = visible;
    p.setHeaderVisible(visible);
}

auto ListView::setHeaderText(const std::vector<std::string>& text) -> void {
    if (_A::dummy) return;
    state.header = text;
    p.setHeaderText(text);
}

auto ListView::setText(unsigned selection, const std::vector<std::string>& text) -> void {
    if(selection >= state.rows.size()) return;
    for(unsigned position = 0; position < text.size(); position++) {
        setText(selection, position, text.at(position));
    }
}

auto ListView::setText(unsigned selection, unsigned position, const std::string& text) -> void {
    if (_A::dummy) return;
    if(selection >= state.rows.size()) return;
    std::vector<std::string>& row = state.rows.at(selection);
    if(position >= row.size()) return;
    row.at(position) = text;
    p.setText(selection, position, text);
}

auto ListView::setImage(unsigned selection, unsigned position, Image& image) -> void {
    if (_A::dummy) return;
    if(selection >= state.images.size()) return;
    std::vector<Image*>& row = state.images.at(selection);
    if(position >= row.size()) return;
    row.at(position) = &image;
    p.setImage(selection, position, image);
}

auto ListView::getImage( unsigned selection, unsigned position ) -> Image* {
    if(selection >= state.images.size()) return nullptr;
    std::vector<Image*>& row = state.images[selection];
    if(position >= row.size()) return nullptr;
    return row[position];
}

auto ListView::countImages() -> unsigned {
    unsigned count = 0;
    for (auto& row : state.images)
        for (auto& img : row)
            if (img && !img->empty()) count++;
    return count;
}

auto ListView::text(unsigned selection, unsigned position) -> std::string {
    if(selection >= state.rows.size()) return "";
    std::vector<std::string>& row = state.rows.at(selection);
    if(position >= row.size()) return "";
    return row.at(position);
}

ListView::ListView() : Widget(*new pListView(*this)), p((pListView&)Widget::p) { if (!_A::dummy) p.init(); }

auto TreeViewItem::append(TreeViewItem& item) -> void {
    if (_A::dummy) return;
    state.items.push_back(&item);
    p.append(item);
}

auto TreeViewItem::remove(TreeViewItem& item) -> void {
    if (_A::dummy) return;
    if (Vector::eraseVectorElement<TreeViewItem*>(state.items, &item)) {
        p.remove(item);
    }
}

auto TreeViewItem::reset() -> void {
    if (_A::dummy) return;
    p.reset();
    state.items.clear();
}

auto TreeViewItem::setText(const std::string& text) -> void {
    if (_A::dummy) return;
    state.text = text;
    p.setText(text);
}

auto TreeViewItem::setSelected() -> void {
    if (_A::dummy) return;
    if (state.parentTreeView) state.parentTreeView->state.selected = this;
    p.setSelected();
}

auto TreeViewItem::selected() -> bool {
    if (state.parentTreeView) return state.parentTreeView->selected() == this;
    return false;
}

auto TreeViewItem::setExpanded(bool expanded) -> void {
    if (_A::dummy) return;
    state.expanded = expanded;
    p.setExpanded(expanded);
}

auto TreeViewItem::setUserData(uintptr_t userData) -> void {
    state.userData = userData;
}

auto TreeViewItem::setImage(Image& image) -> void {
    if (_A::dummy) return;
    state.image = &image;
    p.setImage(image);
}

auto TreeViewItem::setImageSelected(Image& image) -> void {
    if (_A::dummy) return;
    state.imageSelected = &image;
    p.setImageSelected(image);
}

TreeViewItem::TreeViewItem() : p(*new pTreeViewItem(*this)) { if (!_A::dummy) p.init(); }
TreeViewItem::~TreeViewItem() { delete &p; }

auto TreeView::append(TreeViewItem& item) -> void {
    if (_A::dummy) return;
    state.items.push_back(&item);
    p.append(item);
}

auto TreeView::remove(TreeViewItem& item) -> void {
    if (_A::dummy) return;
    if (Vector::eraseVectorElement<TreeViewItem*>(state.items, &item)) {
        p.remove(item);
    }
}

auto TreeView::reset() -> void {
    if (_A::dummy) return;
    state.selected = nullptr;
    p.reset();
    state.items.clear();
}

TreeView::TreeView() : Widget(*new pTreeView(*this)), p((pTreeView&)Widget::p) { if (!_A::dummy) p.init(); }

auto Viewport::handle() -> uintptr_t {
    if (_A::dummy) return 0;
    return p.handle();
}

auto Viewport::setDroppable(bool droppable) -> void {
    if (_A::dummy) return;
    state.droppable = droppable;
    p.setDroppable(droppable);
}

auto Viewport::getMousePosition() -> Position& {
    
    return state.mousePos;
}

Viewport::Viewport() : Widget(*new pViewport(*this)), p((pViewport&)Widget::p) { if (!_A::dummy) p.init(); }

//menu
MenuBase::MenuBase(pMenuBase& p) : p(p), Base() {}
MenuBase::~MenuBase() {
    if(state.parentMenu) state.parentMenu->remove(*this);
    delete &p;
}

auto MenuBase::setEnabled(bool enabled) -> void {
    if (_A::dummy) return;
    state.enabled = enabled;
    p.setEnabled(enabled);
}

auto MenuBase::setVisible(bool visible) -> void {
    if (_A::dummy) return;
    state.visible = visible;
    p.setVisible(visible);
}

auto MenuBase::setText(const std::string& text) -> void {
    if (_A::dummy) return;
    state.text = text;
    p.setText(text);
}

auto MenuBase::setIcon(Image& icon) -> void {
    if (_A::dummy) return;
    if (state.icon == &icon)
        return;
    
    state.icon = &icon;
    p.setIcon(icon);
}

Menu::Menu() : MenuBase(*new pMenu(*this)), p((pMenu&)MenuBase::p) { if (!_A::dummy) p.init(); }
Menu::~Menu() {
    if(!state.parentMenu && state.parentWindow) {
        state.parentWindow->remove(*this);
    }
}
    
auto Menu::append(MenuBase& item) -> void {
    if (_A::dummy) return;
    childs.push_back( &item );
    item.state.parentMenu = this;
    p.append( item );
}

auto Menu::remove(MenuBase& item) -> void {
    if (_A::dummy) return;
    if (Vector::eraseVectorElement<MenuBase*>(childs, &item)) {
        item.state.parentMenu = nullptr;
        p.remove(item);
    }
}

auto Menu::reset() -> void {
    auto childs = this->childs;
    for( auto child : childs ) {
        remove(*child);
    }
}

MenuItem::MenuItem() : MenuBase(*new pMenuItem(*this)), p((pMenuItem&)MenuBase::p) { if (!_A::dummy) p.init(); }

MenuSeparator::MenuSeparator() : MenuBase(*new pMenuSeparator(*this)), p((pMenuSeparator&)MenuBase::p) { if (!_A::dummy) p.init(); }

std::vector<MenuSeparator*> MenuSeparator::instances;

auto MenuSeparator::getInstance() -> MenuSeparator* {
    MenuSeparator* instance = new MenuSeparator;
    instances.push_back( instance );
    return instance;
}

auto MenuSeparator::cleanInstances() -> void {
    for(auto& instance : instances) delete instance;
}

MenuCheckItem::MenuCheckItem() : MenuBase(*new pMenuCheckItem(*this)), p((pMenuCheckItem&)MenuBase::p) { if (!_A::dummy) p.init(); }

auto MenuCheckItem::setChecked(bool checked) -> void {
    if (_A::dummy) return;
    state.checked = checked;
    p.setChecked(checked);
}

auto MenuCheckItem::toggle() -> void {
    if (_A::dummy) return;
    state.checked ^= 1;
    p.setChecked(state.checked);
    if(onToggle) onToggle();
}

MenuRadioItem::MenuRadioItem() : MenuBase(*new pMenuRadioItem(*this)), p((pMenuRadioItem&)MenuBase::p) { if (!_A::dummy) p.init(); }

MenuRadioItem::~MenuRadioItem() { group.clear(); }

auto MenuRadioItem::setChecked() -> void {
    if (_A::dummy) return;
    for(auto& item : group) item->state.checked = false;
    state.checked = true;
    p.setChecked();
}

auto MenuRadioItem::setGroup(std::vector<MenuRadioItem*> group) -> void {
    if (_A::dummy) return;
    for(auto& item : group) item->p.setGroup( item->group = group );
    if (group.size()) group.at(0)->setChecked();
}

//timer
Timer::Timer() : p(*new pTimer(*this)), Base() { }
Timer::~Timer() { delete &p; }

auto Timer::setEnabled(bool enabled) -> void {
    state.enabled = enabled;
    p.setEnabled(enabled);
}

auto Timer::setInterval(unsigned intervalInMs) -> void {
    state.interval = intervalInMs;
    p.setInterval(intervalInMs);
}
//browserWindow
std::function<void ()> BrowserWindow::onCall = nullptr;

BrowserWindow::BrowserWindow() : p(*new pBrowserWindow(*this)) { }
BrowserWindow::~BrowserWindow() { delete &p; }

auto BrowserWindow::directory() -> std::string {
    if (onCall) onCall();
    return p.directory();
}

auto BrowserWindow::open() -> std::string {
	if (onCall) onCall();
    return p.file(false);
}

auto BrowserWindow::save() -> std::string {
	if (onCall) onCall();
    return p.file(true);
}

auto BrowserWindow::close() -> void {
    p.close();
}

auto BrowserWindow::setFilters(std::vector<std::string> filters) -> BrowserWindow& {
    state.filters = filters;
    return *this;
}

auto BrowserWindow::setWindow(Window& window) -> BrowserWindow& {
    state.window = &window;
    return *this;
}

auto BrowserWindow::setPath(const std::string& path) -> BrowserWindow& {
    state.path = path;
    return *this;
}

auto BrowserWindow::setTitle(const std::string& title) -> BrowserWindow& {
    state.title = title;
    return *this;
}

auto BrowserWindow::setOnChangeCallback( std::function<void (std::string filePath)> onSelectionChange ) -> BrowserWindow& {
    state.onSelectionChange = onSelectionChange;
    return *this;
}

auto BrowserWindow::addCustomButton( std::string text, std::function<bool (std::string filePath)> onClick ) -> BrowserWindow& {    
    state.buttons.push_back({text, onClick});            
    return *this;
}

auto BrowserWindow::transformFilter( std::string description, const std::string& suffix ) -> std::string {
	
	std::vector<std::string> _suffix;
	_suffix.push_back( suffix );
	return transformFilter( description, _suffix );
}

auto BrowserWindow::transformFilter( std::string description, const std::vector<std::string>& suffix ) -> std::string {
	std::string out = description + " (";
	auto i = 0;
	
	for(auto& part : suffix) {		
		out += "*." + part;
		
		if (++i < suffix.size())
			out += ", ";
	}	
	return out + ")";
}

//messageWindow
auto MessageWindow::error(MessageWindow::Buttons buttons) -> MessageWindow::Response {
    state.buttons = buttons;
    return pMessageWindow::error(state);
}

auto MessageWindow::information(MessageWindow::Buttons buttons) -> MessageWindow::Response {
    state.buttons = buttons;
    return pMessageWindow::information(state);
}

auto MessageWindow::warning(MessageWindow::Buttons buttons) -> MessageWindow::Response {
    state.buttons = buttons;
    return pMessageWindow::warning(state);
}

auto MessageWindow::question(MessageWindow::Buttons buttons) -> MessageWindow::Response {
    state.buttons = buttons;
    return pMessageWindow::question(state);
}

auto MessageWindow::setWindow(Window& window) -> MessageWindow& {
    state.window = &window;
    return *this;
}

auto MessageWindow::setText(const std::string& text) -> MessageWindow& {
    state.text = text;
    return *this;
}

auto MessageWindow::setTitle(const std::string& title) -> MessageWindow& {
    state.title = title;
    return *this;
}

MessageWindow::Trans MessageWindow::trans = {"ok", "yes", "no", "cancel"};

auto MessageWindow::translateOk(const std::string& str) -> void {
    trans.ok = str;
}

auto MessageWindow::translateNo(const std::string& str) -> void {
    trans.no = str;
}

auto MessageWindow::translateYes(const std::string& str) -> void {
    trans.yes = str;
}

auto MessageWindow::translateCancel(const std::string& str) -> void {
    trans.cancel = str;
}
//font
auto Font::system(unsigned size, const std::string& style) -> std::string {
    return pFont::system(size, style);
}

auto Font::system(const std::string& style) -> std::string {
    return pFont::system(0, style);
}

auto Font::size(const std::string& font, const std::string& text) -> Size {
    return pFont::size(font, text);
}
//system
auto System::getUserDataFolder(std::string appIdent) -> std::string {
    std::string out = pSystem::getUserDataFolder();
    //fallback is application folder
    if(out.length() == 0) return "./";
    if (!appIdent.empty())
        out += appIdent + "/";
    return out;
}

auto System::getResourceFolder(std::string appIdent) -> std::string {
    std::string out = pSystem::getResourceFolder(appIdent);
    return File::beautifyPath(out);
}

auto System::getWorkingDirectory() -> std::string {
    std::string out = pSystem::getWorkingDirectory();
    return File::beautifyPath(out);
}

auto System::getDesktopSize() -> Size {
    return pSystem::getDesktopSize();
}

auto System::sleep(unsigned milliSeconds) -> void {
    pSystem::sleep( milliSeconds );
}

auto System::isOffscreen( Geometry geometry ) -> bool {
    return pSystem::isOffscreen( geometry );
}

auto System::getOSLang() -> Language {
    return pSystem::getOSLang();
}

auto System::printToCmd( std::string str ) -> void {
    pSystem::printToCmd( str );
}

}
