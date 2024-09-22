
#include <cstring>

#ifdef _MSC_VER
#include <io.h>
#include "winapi/dirent.h"
#else
#include <dirent.h>
#include <unistd.h>
#endif

#include <sys/stat.h>
#include <ctime>
#include <cmath>
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
std::string Application::name;
std::string Application::vendor;
std::function<void ()> Application::loop = nullptr;
std::function<void (std::string text)> Application::onClipboardRequest = nullptr;
std::function<void ()> Application::onDisplayChange = nullptr;
std::function<void ()> Application::onQuitRequest = nullptr;

std::function<void ()> Application::Cocoa::onAbout;
std::function<void ()> Application::Cocoa::onPreferences;
std::function<void ()> Application::Cocoa::onCustom1;
std::function<void ()> Application::Cocoa::onQuit;
std::function<void ()> Application::Cocoa::onDock;
std::function<void (std::string fileName)> Application::Cocoa::onOpenFile;

std::function<void (float rate)> Monitor::onFullscreenRefreshChange = nullptr;

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

auto Application::requestClipboardText() -> void {
    pApplication::requestClipboardText();
}

auto Application::closeOtherInstances() -> void {
    pInterProcess::closeOtherInstances();
}

auto Application::setClipboardText( std::string text ) -> void {
    pApplication::setClipboardText( text );
}
#ifdef _WIN32
auto Application::getUtf8CmdLine(std::vector<std::string>& out) -> bool {
    return pApplication::getUtf8CmdLine(out);
}
#endif
//window
std::vector<CustomFont*> Window::customFonts;

Window::Window(Hints hints) : p(*new pWindow(*this, hints)), Base(), cocoa(*this), winapi(*this) {
    state.widgetFont = Font::system();
    this->hints = hints;
}

Window::~Window() { 
    delete &p;
    delete focusTimer;
}

auto Window::append(Menu& menu) -> void {
    state.menus.push_back(&menu);
    menu.MenuBase::state.parentWindow = this;
    p.append(menu);
}

auto Window::remove(Menu& menu) -> void {
    if (Vector::eraseVectorElement<Menu*>(state.menus, &menu)) {
        p.remove(menu);
        menu.MenuBase::state.parentWindow = nullptr;
    }
}

auto Window::isApended(Menu& menu) -> bool {
    
    for( auto apendedMenu : state.menus ) {        
        if (apendedMenu == &menu)
            return true;
    }
    return false;
}

auto Window::hasAppended(Widget& widget) -> bool {
    return widget.Sizable::state.window == this;
}

auto Window::append(Widget& widget) -> void {
    widget.Sizable::state.window = this;
    p.append(widget);
}

auto Window::remove(Widget& widget) -> void {
    p.remove(widget);
    widget.Sizable::state.window = nullptr;
}

auto Window::append(StatusBar& statusBar) -> void {
    state.statusBar = &statusBar;
    statusBar.state.window = this;
    p.append(statusBar);
}

auto Window::remove(StatusBar& statusBar) -> void {
    state.statusBar = nullptr;
    statusBar.state.window = nullptr;
    p.remove(statusBar);
}

auto Window::append(Layout& layout) -> void {
    if (state.layout) remove(layout);
    state.layout = &layout;
    layout.Sizable::state.parent = nullptr;
    layout.Sizable::state.window = this;
    layout.updateLayout();
    p.append(layout);
}

auto Window::remove(Layout& layout) -> void {
    state.layout = nullptr;
    p.remove(layout);
    layout.reset();
    layout.Sizable::state.window = nullptr;
}

auto Window::addCustomFont( CustomFont* customFont ) -> bool {	

	bool ok = pWindow::addCustomFont( customFont );
	if (ok)
        customFonts.push_back( customFont );

	return ok;
}

auto Window::getCustomFont(void* refPtr) -> CustomFont* {
    for(auto cF : customFonts) {
        if (cF->refPtr == refPtr)
            return cF;
    }
    return nullptr;
}

auto Window::setWidgetFont(const std::string& font) -> void {
    state.widgetFont = font;
}

auto Window::setDroppable(bool droppable) -> void {
    state.droppable = droppable;
    p.setDroppable(droppable);
}

auto Window::setBackgroundColor(unsigned color) -> void {
    p.setBackgroundColor(color);
}

auto Window::setVisible(bool visible) -> bool {
    state.visible = visible;
    synchronizeLayout();
    return p.setVisible(visible);
}

auto Window::restore() -> void {
    p.restore();
}

auto Window::setForeground() -> void {
    p.setForeground();
}

auto Window::setFocused() -> void {
    p.setFocused();
}

auto Window::setFocused(unsigned delay) -> void {
    if (!focusTimer)
        focusTimer = new Timer;
    
	focusTimer->setInterval(delay);
	
	focusTimer->onFinished = [this]() {
		focusTimer->setEnabled(false);		
		p.setFocused();        
	};
	focusTimer->setEnabled();    
}

auto Window::setTitle(const std::string& text) -> void {
    state.title = text;
    p.setTitle(text);
}

auto Window::setStatusVisible(bool visible) -> void {
    state.statusVisible = visible;
    p.setStatusVisible(visible);
}

auto Window::setMenuVisible(bool visible) -> void {
    if (Application::isCocoa() && !visible && fullScreen())
        // macOS hides the menu in fullscreen.
        // moving mouse to upper screen border make it visible
        // so don't prevent this behaviour 
        return;
    state.menuVisible = visible;
    p.setMenuVisible(visible);
}
    
auto Window::setFullScreen(bool fullScreen) -> void {
    if (p.fullScreenToggleDelayed()) {
        return;
    }
    state.fullScreen = fullScreen;
    p.setFullScreen(fullScreen);
}

auto Window::setFullscreenSetting( bool inUse, unsigned displayId, unsigned settingId ) -> void {

//    if (fullScreen()) {
//        if (!inUse && fullscreenSetting.inUse)
//            p.updateFullScreen(false);
//        else if ( (inUse && !fullscreenSetting.inUse) || (inUse && ((displayId != fullscreenSetting.displayId) || (settingId != fullscreenSetting.settingId))))
//            p.updateFullScreen(true, displayId, settingId);
//    }

    fullscreenSetting.inUse = inUse;
    fullscreenSetting.displayId = displayId;
    fullscreenSetting.settingId = settingId;
}

auto Window::getCustomFullscreenRefreshRate() -> float {

    if (!fullscreenSetting.inUse)
        return 0.0;

    return pMonitor::getRefreshRate(fullscreenSetting.displayId, fullscreenSetting.settingId);
}

auto Window::setResizable(bool resizable) -> void {
    if (state.resizable == resizable)
        return;
    state.resizable = resizable;
    p.setResizable(resizable);
}

auto Window::setGeometry(Geometry geometry) -> void {
    auto _size = pSystem::getDesktopSize();
    geometry.width = std::min<unsigned>(geometry.width, _size.width);
    geometry.height = std::min<unsigned>(geometry.height, _size.height);
    state.geometry = geometry;
    p.setGeometry(geometry);
}

auto Window::isOffscreen() -> bool { 
    return p.isOffscreen();
}

auto Window::synchronizeLayout() -> void {
    if(visible() && !Application::isQuit) p.setGeometry(geometry());
}

auto Window::focused() -> bool {
    return p.focused();
}

auto Window::minimized() -> bool {
    return p.minimized();
}

auto Window::geometry() -> Geometry {
    return p.geometry();
}

auto Window::setAspectRatio(Size ratio) -> void {
    state.aspectRatio = ratio;
    p.applyAspectRatio();
}

auto Window::applyMaximizeCorrection(Geometry& geo) -> void {
    p.applyMaximizeCorrection(geo);
}

auto Window::changeCursor( Image& image, unsigned hotSpotX, unsigned hotSpotY ) -> void {
    if (state.cursorImage == &image)
        return;
    
    state.cursorImage = &image;
    
    cursor = image.empty() ? Cursor::Default : Cursor::Image;
    
    p.changeCursor( image, hotSpotX, hotSpotY );
}

auto Window::setDefaultCursor( ) -> void {
   // if (!state.cursorImage)
     //   return;
    
    if (cursor == Cursor::Default)
        return;
    
    cursor = Cursor::Default;
    
    state.cursorImage = nullptr;
    
    p.setDefaultCursor();
}

auto Window::setPointerCursor( ) -> void {    
    if (cursor == Cursor::Pointer)
        return;
    
    cursor = Cursor::Pointer;
    
    state.cursorImage = nullptr;
    
    p.setPointerCursor();
}
    
auto Window::handle() -> uintptr_t {
    return p.handle();
}

auto Window::Cocoa::setTitleForAppMenuItem(AppMenuItem appMenuItem, const std::string& title) -> void {
#if GUIKIT_COCOA
    window.p.setTitleForAppMenuItem(appMenuItem, title);
#endif
}

auto Window::Cocoa::setHiddenForAppMenuItem(AppMenuItem appMenuItem, bool state) -> void {
#if GUIKIT_COCOA
    window.p.setHiddenForAppMenuItem(appMenuItem, state);
#endif
}
    
auto Window::Cocoa::keepMenuVisibilityOnDisplay(bool state) -> void {
#if GUIKIT_COCOA
    window.p.keepMenuVisibilityOnDisplay( state );
#endif
}

auto Window::Cocoa::setDisableIconsInTopMenu(bool state) -> void {
#if GUIKIT_COCOA
    window.p.disableIconsInTopMenu = state;
#endif
}

auto Window::getScrollbarWidth() -> unsigned {
    return p.getScrollbarWidth();
}

auto Window::canGrabInputFocusAfterDnDFromOtherApp() -> bool {
#ifdef GUIKIT_GTK
    return pApplication::canGrabInputFocusAfterDnDFromOtherApp();
#else
    return true;
#endif

}

//statusbar
StatusBar::StatusBar() : p(*new pStatusBar(*this)), Base() {}

StatusBar::~StatusBar() { delete &p; }

auto StatusBar::setFont(const std::string& font) -> void {
    state.font = font;
    p.setFont(font);    
}

auto StatusBar::setText(const std::string& text) -> void {
    state.text = text;
    p.setText(text);
}

auto StatusBar::append(unsigned id, const std::string& text, std::function<void ()> onClick, Menu* popupMenu, int pos) -> void {
	Part part;
	part.id = id;
    part.width = p.getWidth( text );
	part.text = text;
	part.image = nullptr;
	part.popupMenu = popupMenu;
	part.onClick = onClick;
    part.sliderLength = 0;
	part.overrideForegroundColor = -1;
	part.visible = false;
	
	if ( (pos == -1) || (pos >= state.parts.size()) )
		state.parts.push_back(part);
	else
		Vector::insert<Part>(state.parts, part, pos);

	state.updatePending = true;
}

auto StatusBar::append(unsigned id, Image* image, std::function<void ()> onClick, Menu* popupMenu, int pos) -> void {	
	Part part;
	part.id = id;
	part.width = image->width;
	part.text = "";
	part.image = image;
	part.popupMenu = popupMenu;
	part.onClick = onClick;
    part.sliderLength = 0;
	part.visible = false;
	
	if ( (pos == -1) || (pos >= state.parts.size()) )
		state.parts.push_back(part);
	else
		Vector::insert<Part>(state.parts, part, pos);

	state.updatePending = true;
}

auto StatusBar::append(unsigned id, unsigned sliderLength, unsigned width, std::function<void (unsigned position)> onChange, Menu* popupMenu, int pos) -> void {
    Part part;
    part.id = id;
    part.width = width;
    part.text = "";
    part.image = nullptr;
    part.popupMenu = popupMenu;
    part.onChange = onChange;
    part.visible = false;
    part.sliderLength = sliderLength;

    if ( (pos == -1) || (pos >= state.parts.size()) )
        state.parts.push_back(part);
    else
        Vector::insert<Part>(state.parts, part, pos);

    state.updatePending = true;
}
    
auto StatusBar::removePart( unsigned id ) -> void {    
    unsigned pos = 0;
    for(auto& part : state.parts) {
        if (part.id == id) {
            Vector::eraseVectorPos( state.parts, pos );
            break;
        }
        pos++;
    }
    
    state.updatePending = true;
}

auto StatusBar::updateDimension( unsigned id, const std::string& text ) -> void {
    for(auto& part : state.parts) {
        if (part.id == id) {
            part.width = p.getWidth( text );
            state.updatePending = true;
            break;
        }
    }
}

auto StatusBar::updateText( unsigned id, std::string text, bool alignRight, int overrideForegroundColor ) -> bool {
    for(auto& part : state.parts) {
        if (part.id == id) {
            
            if (!part.visible) {
                part.overrideForegroundColor = overrideForegroundColor;
                part.text = text;
                part.alignRight = alignRight;
                part.visible = true;
                state.updatePending = true;
                
            } else if ( (part.text != text) || (part.overrideForegroundColor != overrideForegroundColor)
                || (part.alignRight != alignRight) ) {
                part.overrideForegroundColor = overrideForegroundColor;
                part.text = text;
                part.alignRight = alignRight;
                p.updatePart( part );
            }                       
            return true;
        }
    }
    return false;
}
    
auto StatusBar::updateImage( unsigned id, Image* image ) -> bool {    
    for (auto& part : state.parts) {
        if (part.id == id) {
            if (!part.visible) {
                part.image = image;
                part.visible = true;
                state.updatePending = true;
                
            } else if (part.image != image) {
                part.image = image;
                p.updatePart( part );
            }
            return true;
        }
    }
    return false;
}

auto StatusBar::updateSlider( unsigned id, unsigned position ) -> bool {
    for (auto& part : state.parts) {
        if (part.id == id) {
            if (!part.visible) {
                part.sliderPosition = position;
                part.visible = true;
                state.updatePending = true;

            } else if (part.sliderPosition != position) {
                part.sliderPosition = position;
                p.updatePart( part );
            }
            return true;
        }
    }
    return false;
}
    
auto StatusBar::updateVisible( unsigned id, bool visible ) -> bool {
    for(auto& part : state.parts) {
        if (part.id == id) {
            if (part.visible != visible) {
                part.visible = visible;
                state.updatePending = true;
            }
            return true;
        }
    }
    return false;
}
    
auto StatusBar::updateTooltip( unsigned id, std::string tooltip ) -> bool {    
    for(auto& part : state.parts) {
        if (part.id == id) {
            if (part.tooltip != tooltip) {
				part.tooltip = tooltip;
				
				if (part.visible)
					p.updatePart( part );
            }
            return true;
        }
    }
    return false;
}

auto StatusBar::updateSeparator( unsigned id, bool append ) -> bool {
    for(auto& part : state.parts) {
        if (part.id == id) {
            if (part.appendSeparator != append) {
                part.appendSeparator = append;
                state.updatePending = true;
            }
            return true;
        }
    }
    return false;
}

auto StatusBar::hideContent() -> void {
    for(auto& part : state.parts) {
        part.visible = false;
        part.position = -1;
    }
    
    state.updatePending = true;
}

auto StatusBar::clear() -> void {
    state.parts.clear();
    state.updatePending = true;
}

auto StatusBar::update(bool force) -> void {	
    if (force || state.updatePending) {
        p.update();
        state.updatePending = false;
    }
}

//widgets
auto Widget::focused() -> bool {
    return p.focused();
}

auto Widget::setEnabled(bool enabled) -> void {
    Sizable::state.enabled = enabled;
    p.setEnabled(enabled);
}

auto Widget::setFocused() -> void {
    return p.setFocused();
}

auto Widget::setFont(const std::string& font, bool specialFont) -> void {
    state.font = font;
    state.specialFont = specialFont;
    p.setFont(font);
}

auto Widget::font() -> std::string {
    return !state.font.empty() ? state.font :
    ( window() ? window()->widgetFont() : Font::system() );
}

auto Widget::setGeometry(Geometry geometry) -> void {
    state.geometry = geometry;
    p.setGeometry(geometry);
}

auto Widget::setVisible(bool visible) -> void {
    Sizable::state.visible = visible;
    p.setVisible(visible);
}

auto Widget::minimumSize() -> Size {
    return p.minimumSize();
}

auto Widget::setText(const std::string& text) -> void {
    state.text = text;
    p.setText(text);
}

auto Widget::setTooltip(const std::string& tooltip) -> void {
    state.tooltip = tooltip;
    p.setTooltip(tooltip);
}

auto Widget::setBackgroundColor(unsigned color) -> void {
    state.overrideBackgroundColor = true;
    state.backgroundColor = color;
    p.setBackgroundColor(color);
}

auto Widget::setForegroundColor(unsigned color) -> void {
    state.overrideForegroundColor = true;
    state.foregroundColor = color;
    p.setForegroundColor(color);
}

auto Widget::resetForegroundColor() -> void {
    state.overrideForegroundColor = false;
    state.foregroundColor = 0;
    p.setForegroundColor(0);
}
    
Widget::Widget() : p(*new pWidget(*this)), Sizable() { }
Widget::Widget(pWidget& p) : p(p), Sizable() { }
Widget::~Widget() { delete &p; }

auto LineEdit::setEditable(bool editable) -> void {
    state.editable = editable;
    p.setEditable(editable);
}

auto LineEdit::setDroppable(bool droppable) -> void {
    state.droppable = droppable;
    p.setDroppable(droppable);
}

auto LineEdit::text() -> std::string { 
    return p.text();
}

auto LineEdit::value() -> int {
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
    state.maxLength = maxLength;
    p.setMaxLength( maxLength );
}

LineEdit::LineEdit() : Widget(*new pLineEdit(*this)), p((pLineEdit&)Widget::p) { p.init(); }

auto MultilineEdit::setEditable(bool editable) -> void {
    state.editable = editable;
    p.setEditable(editable);
}

auto MultilineEdit::text() -> std::string { 
    return p.text();
}

MultilineEdit::MultilineEdit() : Widget(*new pMultilineEdit(*this)), p((pMultilineEdit&)Widget::p) { p.init(); }

auto Label::setAlign( Align align ) -> void {
    state.align = align;
    p.setAlign( align );
}

Label::Label() : Widget(*new pLabel(*this)), p((pLabel&)Widget::p) { p.init(); }

auto Hyperlink::setUri( std::string uri, std::string wrap ) -> void {
	state.uri = uri;
	state.wrap = wrap;
	
	p.setUri( uri, wrap );
}

Hyperlink::Hyperlink() : Widget(*new pHyperlink(*this)), p((pHyperlink&)Widget::p) { p.init(); }

auto SquareCanvas::setBorderColor(unsigned borderSize, unsigned borderColor) -> void {
    state.borderSize = borderSize;
    state.borderColor = borderColor;
    p.setBorderColor( borderSize, borderColor );
}

SquareCanvas::SquareCanvas() : Widget(*new pSquareCanvas(*this)), p((pSquareCanvas&)Widget::p) { p.init(); }

auto ImageView::setImage(Image* image) -> void {
    state.image = image;
    p.setImage(image);
}

auto ImageView::setUri( std::string uri ) -> void {
    state.uri = uri;

    p.setUri( uri );
}

ImageView::ImageView() : Widget(*new pImageView(*this)), p((pImageView&)Widget::p) { p.init(); }

auto Button::setImage(Image* image) -> void {
    state.image = image;
    p.setImage(image);
}

Button::Button() : Widget(*new pButton(*this)), p((pButton&)Widget::p) { p.init(); }

auto StepButton::setRange(int16_t minValue, int16_t maxValue) -> void {    
    state.minValue = minValue;
    state.maxValue = maxValue;
    
    p.updateRange();
}

auto StepButton::setValue(int16_t value) -> void {
    Widget::state.text = std::to_string( value );
    state.value = value;    
    p.setValue( value );
}

StepButton::StepButton() : Widget(*new pStepButton(*this)), p((pStepButton&)Widget::p) { p.init(); }

auto CheckButton::setChecked(bool checked) -> void {
    state.checked = checked;
    p.setChecked(checked);
}

auto CheckButton::toggle() -> void {
    state.checked ^= 1;
    p.setChecked(state.checked);
    if(onToggle) onToggle();
}

CheckButton::CheckButton() : Widget(*new pCheckButton(*this)), p((pCheckButton&)Widget::p) { p.init(); }

auto CheckBox::setChecked(bool checked) -> void {
    state.checked = checked;
    p.setChecked(checked);
}

auto CheckBox::toggle() -> void {
    state.checked ^= 1;
    p.setChecked(state.checked);
    if(onToggle) onToggle(state.checked);
}

CheckBox::CheckBox() : Widget(*new pCheckBox(*this)), p((pCheckBox&)Widget::p) { p.init(); }

auto ComboButton::append(const std::string& text, int userData) -> void {
    state.rows.push_back(text);
    state.userData.push_back(userData);
    p.append(text);
}

auto ComboButton::remove(unsigned selection) -> void {
    if(selection >= state.rows.size()) return;
    state.rows.erase(state.rows.begin() + selection);
    state.userData.erase(state.userData.begin() + selection);
    p.remove(selection);
}

auto ComboButton::reset() -> void {
    state.selection = 0;
    state.rows.clear();
    state.userData.clear();
    p.reset();
}

auto ComboButton::setSelection(unsigned selection) -> void {
    if(selection >= state.rows.size()) return;
    state.selection = selection;
    p.setSelection(selection);
}

auto ComboButton::activate(unsigned selection) -> void {
    setSelection(selection);
    if(onChange) onChange();
}

auto ComboButton::setSelectionByUserId(int userId) -> void {
    unsigned selection = 0;
    
    for( auto& _id : state.userData) {        
        if (_id == userId)
            break;
        
        selection++;
    }
    
    setSelection( selection );
}

auto ComboButton::setSelectionByRow(std::string row) -> void {
    unsigned selection = 0;

    bool found = false;
    for( auto& _row : state.rows) {
        if (_row == row) {
            found = true;
            break;
        }

        selection++;
    }

    setSelection( found ? selection : 0 );
}

auto ComboButton::setText(unsigned selection, const std::string& text) -> void {
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

ComboButton::ComboButton(bool hintVerticalScrollbar) : hintVerticalScrollbar(hintVerticalScrollbar),  Widget(*new pComboButton(*this)), p((pComboButton&)Widget::p) { p.init(); }

auto Slider::setLength(unsigned length) -> void {
    state.length = length;
    p.setLength(length);
}

auto Slider::setPosition(unsigned position) -> void {
    state.position = position;
    p.setPosition(position);
}

Slider::Slider(Orientation orientation) : orientation(orientation), Widget(*new pSlider(*this)), p((pSlider&)Widget::p) { p.init(); }

auto RadioBox::setGroup(std::vector<RadioBox*> group) -> void {
    for(auto& item : group) item->p.setGroup( item->state.group = group );
    if (group.size()) group.at(0)->setChecked();
}

auto RadioBox::setChecked() -> void {
    for(auto& item : state.group) item->state.checked = false;
    state.checked = true;
    p.setChecked();
}

auto RadioBox::activate() -> void {
    setChecked();
    if(onActivate) onActivate();
}

RadioBox::RadioBox() : Widget(*new pRadioBox(*this)), p((pRadioBox&)Widget::p) { p.init(); }

auto ProgressBar::setPosition(unsigned position) -> void {
    state.position = position;
    p.setPosition(position);
}

ProgressBar::ProgressBar() : Widget(*new pProgressBar(*this)), p((pProgressBar&)Widget::p) { p.init(); }

auto ListView::lockRedraw() -> void {
    p.lockRedraw();
}

auto ListView::unlockRedraw() -> void {
    p.unlockRedraw();
}

auto ListView::append(const std::vector<std::vector<std::string>>& rows, bool clearBefore) -> void {    
    p.lockRedraw();
    
    if (clearBefore)
        reset();
    
    for (auto& row : rows )
        append( row );
    
    p.unlockRedraw();
}

auto ListView::append(const std::vector<std::string>& row) -> void {
    state.rows.push_back(row);
    std::vector<Image*> images;
    for (unsigned i = 0; i < row.size(); i++) images.push_back(nullptr);
    state.images.push_back(images);
	state.rowTooltips.push_back({});
    p.append(row);
}

auto ListView::remove(unsigned selection) -> void {
    if(selection >= state.rows.size()) return;
    state.rows.erase(state.rows.begin() + selection);
    state.images.erase(state.images.begin() + selection);
	state.rowTooltips.erase(state.rowTooltips.begin() + selection);
    p.remove(selection);
}

auto ListView::reset() -> void {
    state.selected = false;
    state.selection = 0;
    state.rows.clear();
    state.images.clear();
	state.rowTooltips.clear();
    p.reset();
}

auto ListView::setSelection(unsigned selection) -> void {
    if(selection >= state.rows.size()) return;
    state.selected = true;
    state.selection = selection;
    p.setSelection(selection);
}

auto ListView::setSelected(bool selected) -> void {
    state.selected = selected;
    p.setSelected(selected);
}

auto ListView::setHeaderVisible(bool visible) -> void {
    state.headerVisible = visible;
    p.setHeaderVisible(visible);
}

auto ListView::setHeaderText(const std::vector<std::string>& text) -> void {
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
    if(selection >= state.rows.size()) return;
    std::vector<std::string>& row = state.rows.at(selection);
    if(position >= row.size()) return;
    row.at(position) = text;
    p.setText(selection, position, text);
}

auto ListView::setImage(unsigned selection, unsigned position, Image& image) -> void {
    if(selection >= state.images.size()) return;
    std::vector<Image*>& row = state.images[selection];
    if(position >= row.size()) return;
    row[position] = &image;
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

auto ListView::setRowTooltip(unsigned selection, std::string tooltip ) -> void {
    if(selection >= state.rowTooltips.size()) return;
	state.rowTooltips[selection] = tooltip;
	p.setRowTooltip(selection, tooltip);
}

auto ListView::getRowTooltip(unsigned selection) -> std::string {
    if(selection >= state.rowTooltips.size()) return "";
	return state.rowTooltips[selection];
}

auto ListView::colorRowTooltips(bool colorTip) -> void {
    state.colorRowTooltips = colorTip;
    p.colorRowTooltips( colorTip );
}

auto ListView::setSelectionColor(unsigned foregroundColor, unsigned backgroundColor) -> void {
    state.overrideSelectionColor = true;
    state.selectionForegroundColor = foregroundColor;
    state.selectionBackgroundColor = backgroundColor;
    p.setSelectionColor(foregroundColor, backgroundColor);
}


auto ListView::resetSelectionColor() -> void {
    state.overrideSelectionColor = false;
    state.selectionForegroundColor = 0;
    state.selectionBackgroundColor = 0;
    p.setSelectionColor();
}

auto ListView::setFirstRowColor(unsigned foregroundColor, unsigned backgroundColor) -> void {
    state.overrideFirstRowColor = true;
    state.firstRowForegroundColor = foregroundColor;
    state.firstRowBackgroundColor = backgroundColor;
    p.setFirstRowColor(foregroundColor, backgroundColor);
}


auto ListView::resetFirstRowColor() -> void {
    state.overrideFirstRowColor = false;
    state.firstRowForegroundColor = 0;
    state.firstRowBackgroundColor = 0;
    p.setFirstRowColor();
}

ListView::ListView() : Widget(*new pListView(*this)), p((pListView&)Widget::p) { p.init(); }

auto TreeViewItem::append(TreeViewItem& item) -> void {
    item.state.parentTreeViewItem = this;
    state.items.push_back(&item);
    p.append(item);
}

auto TreeViewItem::remove(TreeViewItem& item) -> void {
    if (Vector::eraseVectorElement<TreeViewItem*>(state.items, &item)) {
        if(item.selected())
            item.state.parentTreeView->state.selected = nullptr;
        item.state.parentTreeViewItem = nullptr;
        p.remove(item);
    }
}

auto TreeViewItem::reset() -> void {
    p.reset();
    state.items.clear();
}

auto TreeViewItem::setText(const std::string& text) -> void {
    state.text = text;
    p.setText(text);
}

auto TreeViewItem::setSelected() -> void {
    if (state.parentTreeView) state.parentTreeView->state.selected = this;
    p.setSelected();
}

auto TreeViewItem::selected() -> bool {
    if (state.parentTreeView) return state.parentTreeView->selected() == this;
    return false;
}

auto TreeViewItem::setExpanded(bool expanded) -> void {
    state.expanded = expanded;
	p.setExpanded(expanded);
}

auto TreeViewItem::setUserData(uintptr_t userData) -> void {
    state.userData = userData;
}

auto TreeViewItem::setImage(Image& image) -> void {
    state.image = &image;
    p.setImage(image);
}

auto TreeViewItem::setImageSelected(Image& image) -> void {
    state.imageSelected = &image;
    p.setImageSelected(image);
}

auto TreeViewItem::setImageExpanded(Image& image) -> void {
    state.imageExpanded = &image;
    p.setImageExpanded(image);
}

TreeViewItem::TreeViewItem() : p(*new pTreeViewItem(*this)) { p.init(); }
TreeViewItem::~TreeViewItem() { delete &p; }

auto TreeView::append(TreeViewItem& item) -> void {
    state.items.push_back(&item);
    p.append(item);
}

auto TreeView::remove(TreeViewItem& item) -> void {
    if (Vector::eraseVectorElement<TreeViewItem*>(state.items, &item)) {
        if(item.selected())
            item.state.parentTreeView->state.selected = nullptr;
        p.remove(item);
    }
}

auto TreeView::has(TreeViewItem& item) -> bool {
    return Vector::find(state.items, &item);
}

auto TreeView::reset() -> void {
    state.selected = nullptr;
    p.reset();
    state.items.clear();
}

TreeView::TreeView() : Widget(*new pTreeView(*this)), p((pTreeView&)Widget::p) { p.init(); }

auto Viewport::handle(bool hintRecreation) -> uintptr_t {
    return p.handle(hintRecreation);
}

auto Viewport::setDroppable(bool droppable) -> void {
    state.droppable = droppable;
    p.setDroppable(droppable);
}

auto Viewport::getMousePosition() -> Position& {
    
    return state.mousePos;
}

Viewport::Viewport() : Widget(*new pViewport(*this)), p((pViewport&)Widget::p) { p.init(); }

//menu
MenuBase::MenuBase(pMenuBase& p) : p(p), Base() {}
MenuBase::~MenuBase() {
    if(state.parentMenu) state.parentMenu->remove(*this);
    delete &p;
}

auto MenuBase::setEnabled(bool enabled) -> void {
    state.enabled = enabled;
    p.setEnabled(enabled);
}

auto MenuBase::setVisible(bool visible) -> void {
    state.visible = visible;
    p.setVisible(visible);
}

auto MenuBase::setText(const std::string& text) -> void {
    state.text = text;
    p.setText(text);
}

auto MenuBase::setIcon(Image& icon) -> void {
    if (state.icon == &icon)
        return;
    
    state.icon = &icon;
    p.setIcon(icon);
}

Menu::Menu() : MenuBase(*new pMenu(*this)), p((pMenu&)MenuBase::p) { p.init(); }
Menu::~Menu() {
    if(!parentMenu() && parentWindow()) {
        parentWindow()->remove(*this);
    }
}

auto Menu::showContextOnly(bool contextOnly) -> void {
    state.contextOnly = contextOnly;
}
    
auto Menu::append(MenuBase& item) -> void {
    childs.push_back( &item );
    item.state.parentMenu = this;
    p.append( item );
}

auto Menu::remove(MenuBase& item) -> void {
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

MenuItem::MenuItem() : MenuBase(*new pMenuItem(*this)), p((pMenuItem&)MenuBase::p) { p.init(); }

MenuSeparator::MenuSeparator() : MenuBase(*new pMenuSeparator(*this)), p((pMenuSeparator&)MenuBase::p) { p.init(); }

std::vector<MenuSeparator*> MenuSeparator::instances;

auto MenuSeparator::getInstance() -> MenuSeparator* {
    MenuSeparator* instance = new MenuSeparator;
    instances.push_back( instance );
    return instance;
}

auto MenuSeparator::cleanInstances() -> void {
    for(auto& instance : instances) delete instance;
}

MenuCheckItem::MenuCheckItem() : MenuBase(*new pMenuCheckItem(*this)), p((pMenuCheckItem&)MenuBase::p) { p.init(); }

auto MenuCheckItem::setChecked(bool checked) -> void {
    state.checked = checked;
    p.setChecked(checked);
}

auto MenuCheckItem::toggle() -> void {
    state.checked ^= 1;
    p.setChecked(state.checked);
    if(onToggle) onToggle();
}

MenuRadioItem::MenuRadioItem() : MenuBase(*new pMenuRadioItem(*this)), p((pMenuRadioItem&)MenuBase::p) { p.init(); }

MenuRadioItem::~MenuRadioItem() { group.clear(); }

auto MenuRadioItem::setChecked() -> void {
    for(auto& item : group) item->state.checked = false;
    state.checked = true;
    p.setChecked();
}

auto MenuRadioItem::activate() -> void {
    setChecked();
    if (onActivate) onActivate();
}

auto MenuRadioItem::setGroup(std::vector<MenuRadioItem*> group) -> void {
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

auto Timer::setData(unsigned data) -> void {
    state.data = data;
}

//browserWindow
std::function<void ()> BrowserWindow::onCall = nullptr;

BrowserWindow::BrowserWindow() : p(*new pBrowserWindow(*this)) { }
BrowserWindow::~BrowserWindow() {
    delete &p;
    if (state.orderBySelected) delete state.orderBySelected;
}

auto BrowserWindow::directory() -> std::string {
    if (onCall) onCall();
    return p.directory();
}

auto BrowserWindow::open() -> std::string {
	if (onCall) onCall();
    return p.file(false);
}

auto BrowserWindow::openMulti() -> std::vector<std::string> {
    if (onCall) onCall();
    return p.fileMulti();
}

auto BrowserWindow::save() -> std::string {
	if (onCall) onCall();
    return p.file(true);
}

auto BrowserWindow::setNonModal() -> BrowserWindow& {
	state.modal = false;
	return *this;
}

auto BrowserWindow::close() -> void {
    p.close();
}

auto BrowserWindow::setForeground() -> void {
    p.setForeground();
}
    
auto BrowserWindow::detached() -> bool {
    return p.detached();
}

auto BrowserWindow::visible() -> bool {
    return p.visible();
}
    
auto BrowserWindow::setTemplateId(int id) -> BrowserWindow& {
    state.templateId = id;
    return *this;
}

auto BrowserWindow::resizeTemplate(bool resize, int adjust) -> BrowserWindow& {
    state.resizeTemplate = resize;
    state.resizeAdjust = adjust;
    return *this;
}

auto BrowserWindow::addContentView(unsigned id, std::function<bool (std::string filePath, unsigned selection)> onDblClick) -> BrowserWindow& {
    state.contentView.id = id;
    state.contentView.onDblClick = onDblClick;
    return *this;
}
    
auto BrowserWindow::setContentViewFont(std::string font, bool specialFont) -> BrowserWindow& {
    state.contentView.font = font;
    state.contentView.specialFont = specialFont;
    return *this;
}

auto BrowserWindow::setContentViewWidth(unsigned boxWidth) -> BrowserWindow& {
    state.contentView.width = boxWidth;
    return *this;
}

auto BrowserWindow::setContentViewHeight(unsigned boxHeight) -> BrowserWindow& {
    state.contentView.height = boxHeight;
    return *this;
}

auto BrowserWindow::setContentViewBackground(unsigned color) -> BrowserWindow& {
    state.contentView.backgroundColor = color;
    state.contentView.overrideBackgroundColor = true;
    return *this;
}

auto BrowserWindow::setContentViewForeground(unsigned color) -> BrowserWindow& {
    state.contentView.foregroundColor = color;
    state.contentView.overrideForegroundColor = true;
    return *this;
}

auto BrowserWindow::setContentViewSelection(unsigned foregroundColor, unsigned backgroundColor) -> BrowserWindow& {
    state.contentView.selectionForegroundColor = foregroundColor;
    state.contentView.selectionBackgroundColor = backgroundColor;
    state.contentView.overrideSelectionColor = true;
    return *this;
}

auto BrowserWindow::setContentViewFirstRow(unsigned foregroundColor, unsigned backgroundColor) -> BrowserWindow& {
    state.contentView.firstRowForegroundColor = foregroundColor;
    state.contentView.firstRowBackgroundColor = backgroundColor;
    state.contentView.overrideFirstRowColor = true;
    return *this;
}

auto BrowserWindow::setContentViewColorTooltips(bool colorTooltips) -> BrowserWindow& {
    state.contentView.colorTooltips = colorTooltips;
    return *this;
}

auto BrowserWindow::setContentViewHint(std::string hint, std::string tooltip) -> BrowserWindow& {
    state.contentView.hint = hint;
    state.contentView.hintTooltip = tooltip;
    return *this;
}

auto BrowserWindow::setCallbacks( std::function<void (std::vector<std::string> filePaths, unsigned selection)> onOkClick, std::function<void ()> onCancelClick ) -> BrowserWindow& {
    state.onOkClick = onOkClick;
    state.onCancelClick = onCancelClick;
    return *this;
}

    
auto BrowserWindow::getContentViewSelection() -> unsigned {
    return p.contentViewSelection();
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

auto BrowserWindow::setOnChangeCallback( std::function<std::vector<BrowserWindow::Listing> (std::string filePath)> onSelectionChange ) -> BrowserWindow& {
    state.onSelectionChange = onSelectionChange;
    return *this;
}

auto BrowserWindow::setListings( std::vector<BrowserWindow::Listing>& listings ) -> void {
    p.setListings( listings );
}

auto BrowserWindow::addCustomButton( std::string text, std::function<bool (std::vector<std::string> filePaths, unsigned selection)> onClick, unsigned id, std::string toolTip ) -> BrowserWindow& {
    state.buttons.push_back({text, toolTip, onClick, id});            
    return *this;
}

auto BrowserWindow::setDefaultButtonText(std::string textOk, std::string textCancel) -> BrowserWindow& {
    state.textOk = textOk;
    state.textCancel = textCancel;
    return *this;
}

auto BrowserWindow::setDefaultButtonTooltip(std::string toolTip) -> BrowserWindow& {
	state.toolTip = toolTip;
	return *this;
}

auto BrowserWindow::showOrderControlForMultipleSelections( bool checked, std::string label, std::function<void (bool checked)> onToggle ) -> BrowserWindow& {
    if (!state.orderBySelected)
        state.orderBySelected = new BrowserWindow::CheckButton;

    state.orderBySelected->checked = checked;
    state.orderBySelected->text = label;
    state.orderBySelected->onToggle = onToggle;
    return *this;
}

auto BrowserWindow::hideOkButton() -> void {
    state.hideOkButton = true;
}

auto BrowserWindow::transformFilter( std::string description, const std::string& suffix ) -> std::string {
	
	std::vector<std::string> _suffix;
	_suffix.push_back( suffix );
	return transformFilter( description, _suffix );
}

auto BrowserWindow::transformFilter( std::string description, const std::vector<std::string>& suffix ) -> std::string {
    std::string out = "";

	for(auto& part : suffix) {
        if (part.empty())
            continue;

		out += "*." + part + ", ";
	}

    if (!out.empty())
        out = out.substr(0, out.size() - 2);
    else
        return description;

	return description + " (" + out + ")";
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
auto Font::system(unsigned size, const std::string& style, bool monospaced) -> std::string {
    return pFont::system(size, style, monospaced);
}

auto Font::system(const std::string& style, bool monospaced) -> std::string {
    return pFont::system(0, style, monospaced);
}

auto Font::systemFontFile() -> std::string {
    return pFont::systemFontFile();
}

auto Font::size(const std::string& font, const std::string& text) -> Size {
    return pFont::size(font, text);
}

auto Font::scale( unsigned pixel ) -> unsigned {
	return pFont::scale( pixel );
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

auto ThreadPriority::setPriority( Mode mode, float typicalProcessingTimeInMilliSeconds, float maxProcessingTimeInMilliSeconds ) -> bool {
    return pThreadPriority::setPriority( mode, typicalProcessingTimeInMilliSeconds, maxProcessingTimeInMilliSeconds );
}

auto Monitor::getDisplays() -> std::vector<Property> {
    return pMonitor::getDisplays();
};

auto Monitor::getSettings( unsigned displayId ) -> std::vector<Property> {
    return pMonitor::getSettings( displayId );
};

auto Monitor::getCurrentRefreshRate() -> float {
    return pMonitor::getCurrentRefreshRate();
}

auto Monitor::getCurrentResolution() -> Size {
    return pMonitor::getCurrentResolution();
}

}

