
#define UNICODE
#define WINVER 0x0500
#define _WIN32_WINNT 0x0500
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN

#include <windows.h>
#include <shlobj.h>
#define _WIN32_WINNT 0x0600
#include <uxtheme.h>
#define _WIN32_WINNT 0x0500
#include <Shellapi.h>
#include <shlwapi.h>
#include <Commdlg.h>
#include <direct.h>

typedef HPAINTBUFFER (WINAPI *FN_BeginBufferedPaint) (HDC hdcTarget, const RECT *prcTarget, BP_BUFFERFORMAT dwFormat, BP_PAINTPARAMS *pPaintParams, HDC *phdc);    
typedef HRESULT (WINAPI *FN_EndBufferedPaint) (HPAINTBUFFER hBufferedPaint, BOOL fUpdateTarget);

namespace GUIKIT {

#define FixedStyle WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX | WS_BORDER
#define ResizableStyle WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME
	
static const unsigned Windows2000  = 0x0500;
static const unsigned WindowsXP    = 0x0501;
static const unsigned WindowsVista = 0x0600;
static const unsigned Windows7     = 0x0601;

struct pApplication {
    static auto run() -> void;
    static auto processEvents() -> void;
    static auto processMessage(MSG& msg) -> void;
    static auto quit() -> void;
    static auto initialize() -> void;
    static auto CALLBACK wndProc(WNDPROC windowProc, HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT;
    static auto currentWorkingDirectory() -> std::string;
    static std::string cwd; //current working directory
	static unsigned version;
    
    static HMODULE uxTheme;
    static FN_BeginBufferedPaint pfnBeginBufferedPaint;
    static FN_EndBufferedPaint pfnEndBufferedPaint;
};

struct pWindow {
    Window& window;
    HWND hwnd;
    HWND hstatus;
    HMENU hmenu;
	HMENU contextmenu;
    bool locked;

    HBRUSH brush;
    COLORREF brushColor;
    unsigned bgUpdateState; //0 - immediate, 1 - not during resize, 2 - delayed, 3 - update onetime

    HFONT hstatusfont;
    HCURSOR hCursor;

    auto append(Menu& menu) -> void;
    auto append(Widget& widget) -> void;
    auto append(Layout& layout) -> void;
    auto remove(Menu& menu) -> void;
    auto remove(Widget& widget) -> void;
    auto remove(Layout& layout) -> void {}
    auto setGeometry(Geometry geometry) -> void;
    auto setStatusFont(std::string font) -> void;
    auto setBackgroundColor(unsigned color) -> void;
    auto setFocused() -> void;
    auto setVisible(bool visible) -> void;
    auto setResizable(bool resizable) -> void;
    auto setStatusText(std::string text) -> void;
    auto setTitle(std::string text) -> void;
    auto setMenuVisible(bool visible) -> void;
    auto setStatusVisible(bool visible) -> void;
    auto setFullScreen(bool fullScreen) -> void;
    auto fullScreenToggleDelayed() -> bool { return locked; }
    auto setDroppable(bool droppable) -> void;
    auto frameMargin() -> Geometry;
    auto geometry() -> Geometry;
    auto focused() -> bool;
    auto isOffscreen() -> bool;
    auto handle() -> uintptr_t;

    auto onEraseBackground() -> bool;
    auto onClose() -> void;
    auto onMove() -> void;
    auto onSize() -> void;
    auto onDrop(WPARAM wparam) -> void;
    auto updateMenu() -> void;
    auto changeCursor( Image& image, unsigned hotSpotX, unsigned hotSpotY ) -> void;
    auto setDefaultCursor() -> void;
	static auto addCustomFont( CustomFont* customFont ) -> bool;
    static auto CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT;

    pWindow(Window& window);
    ~pWindow();
};

struct pWidget {
    Widget& widget;
    HWND hwnd;
    HWND hwndTip;
    HIMAGELIST imageList = nullptr;
    HFONT hfont;
    WNDPROC wndprocOrig;
    bool locked = false;
    
    struct {
        bool updated = false;
        Size minimumSize = {0, 0};
    } calculatedMinimumSize;

    auto focused() -> bool;
    auto setFocused() -> void;
    virtual auto minimumSize() -> Size { return {0,0}; }
    virtual auto borderSize() -> unsigned { return 2; }
    virtual auto setEnabled(bool enabled) -> void;
    virtual auto setVisible(bool visible) -> void;
    virtual auto setFont(std::string font) -> void;
    virtual auto setText(std::string text) -> void;
    virtual auto setGeometry(Geometry geometry) -> void;
    virtual auto rebuild() -> void;
	virtual auto setForegroundColor(unsigned color) -> void {}
    virtual auto setBackgroundColor(unsigned color) -> void {}

    auto destroy() -> void { destroy(hwnd); }
    auto destroy(HWND& handle) -> void;
    auto destroyImageList() -> void;
    auto setTooltip(std::string tooltip) -> void;
    auto createTooltip() -> void;
    auto getMinimumSize() -> Size;
    virtual auto init() -> void {}

    pWidget(Widget& widget);
    virtual ~pWidget();
};

struct pLineEdit : pWidget {
    LineEdit& lineEdit;

    auto minimumSize() -> Size;
    auto setEditable(bool editable) -> void;
    auto setText(std::string text) -> void;
    auto text() -> std::string;
    auto setDroppable(bool droppable) -> void;
    auto setMaxLength( unsigned maxLength ) -> void;

    pLineEdit(LineEdit& lineEdit) : pWidget(lineEdit), lineEdit(lineEdit) {}
    auto rebuild() -> void;
    auto onChange() -> void;
    auto onFocus() -> void;
    auto create() -> void;
    
    static auto CALLBACK subclassWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT;
};

struct pLabel : pWidget {
    Label& label;

    auto minimumSize() -> Size;
    auto setText(std::string text) -> void;
	auto setEnabled(bool enabled) -> void;
	auto setForegroundColor(unsigned color) -> void;
    auto setFont(std::string font) -> void;
    auto setAlign( Label::Align align ) -> void;

    pLabel(Label& label) : pWidget(label), label(label) {}
    auto rebuild() -> void;
    auto create() -> void;
    static auto CALLBACK subclassWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT;
};

struct pHyperlink : pWidget {
    Hyperlink& hyperlink;
    
    pHyperlink(Hyperlink& hyperlink) : pWidget(hyperlink), hyperlink(hyperlink) {}
    auto minimumSize() -> Size;
    auto setText(std::string text) -> void;
    auto rebuild() -> void;
    auto create() -> void;
    auto generate() -> std::string;
    auto setUri( std::string uri, std::string wrap ) -> void;        
};

struct pSquareCanvas : pWidget {
    SquareCanvas& squareCanvas;

    pSquareCanvas(SquareCanvas& squareCanvas) : pWidget(squareCanvas), squareCanvas(squareCanvas) {}
    auto rebuild() -> void;
    auto create() -> void;
    auto setBackgroundColor( unsigned color ) -> void;
    auto setBorderColor(unsigned borderSize, unsigned borderColor) -> void;
    static auto CALLBACK subclassWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT;
};

struct pButton : pWidget {
    Button& button;

    auto minimumSize() -> Size;
    auto onActivate() -> void;
    auto rebuild() -> void;
    auto create() -> void;

    pButton(Button& button) : pWidget(button), button(button) {}
};

struct pCheckButton : pWidget {
    CheckButton& checkButton;

    auto minimumSize() -> Size;
    auto setChecked(bool checked) -> void;
    auto onToggle() -> void;
    auto rebuild() -> void;
    auto create() -> void;

    pCheckButton(CheckButton& checkButton) : pWidget(checkButton), checkButton(checkButton) {}
};

struct pCheckBox : pWidget {
    CheckBox& checkBox;

    auto minimumSize() -> Size;
    auto setChecked(bool checked) -> void;
    auto onToggle() -> void;
    auto rebuild() -> void;
    auto create() -> void;

    pCheckBox(CheckBox& checkBox) : pWidget(checkBox), checkBox(checkBox) {}
};

struct pComboButton : pWidget {
    ComboButton& comboButton;

    auto append(std::string text) -> void;
    auto remove(unsigned selection) -> void;
    auto minimumSize() -> Size;
    auto reset() -> void;
    auto setGeometry(Geometry geometry) -> void;
    auto setSelection(unsigned selection) -> void;
    auto setText(unsigned selection, std::string text) -> void;
    auto rebuild() -> void;
    auto create() -> void;
    auto onChange() -> void;

    pComboButton(ComboButton& comboButton) : pWidget(comboButton), comboButton(comboButton) {}
};

struct pSlider : pWidget {
    Slider& slider;

    auto minimumSize() -> Size;
    auto setLength(unsigned length) -> void;
    auto setPosition(unsigned position) -> void;
    auto setGeometry(Geometry geometry) -> void;

    auto rebuild() -> void;
    auto create() -> void;
    auto onChange() -> void;

    pSlider(Slider& slider) : pWidget(slider), slider(slider) {}
};

struct pRadioBox : pWidget {
    RadioBox& radioBox;

    auto minimumSize() -> Size;
    auto setChecked() -> void;
    auto setGroup(const std::vector<RadioBox*>& group) -> void {}

    auto rebuild() -> void;
    auto create() -> void;
    auto onActivate() -> void;

    pRadioBox(RadioBox& radioBox) : pWidget(radioBox), radioBox(radioBox) {}
};

struct pProgressBar : pWidget {
    ProgressBar& progressBar;

    auto minimumSize() -> Size;
    auto rebuild() -> void;
    auto create() -> void;
    auto setPosition(unsigned position) -> void;

    pProgressBar(ProgressBar& progressBar) : pWidget(progressBar), progressBar(progressBar) {}
};

struct pListView : pWidget {
    ListView& listView;
    std::vector<Image*> images;

    auto append(const std::vector<std::string>& list) -> void;
    auto autoSizeColumns() -> void;
    auto remove(unsigned selection) -> void;
    auto reset() -> void;
    auto setGeometry(Geometry geometry) -> void;
    auto setHeaderText(std::vector<std::string> list) -> void;
	auto buildHeaderText(std::vector<std::string> list) -> void;
    auto setHeaderVisible(bool visible) -> void;
    auto setSelection(unsigned selection) -> void;
    auto setSelected(bool selected) -> void;
    auto setText(unsigned selection, unsigned position, const std::string& text) -> void;
    auto rebuild() -> void;
    auto create() -> void;
    auto setFont(std::string font) -> void;
    auto setContent() -> void;
    auto setImage(unsigned selection, unsigned position, int imageListPos) -> void;
    auto setImage(unsigned selection, unsigned position, Image& image) -> void;
    auto buildImageList() -> void;
    auto addToImageList(Image& image, unsigned size) -> void;
	auto setBackgroundColor(unsigned color) -> void;
	auto setForegroundColor(unsigned color) -> void;

    static auto CALLBACK subclassWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT;
    auto onCustomDraw(LPARAM lparam) -> LRESULT;
    auto onChange(LPARAM lparam) -> void;
    auto onActivate() -> void;

    pListView(ListView& listView) : pWidget(listView), listView(listView) {}
};

struct pTreeViewItem {
    TreeViewItem& treeViewItem;
    HTREEITEM hTreeItem = nullptr;

    auto append(TreeViewItem& item) -> void;
    auto remove(TreeViewItem& item) -> void;
    auto reset() -> void;
	auto init() -> void {}
    auto update(TreeViewItem* parent) -> void;
    auto setText(std::string text) -> void;
    auto setSelected() -> void;
    auto setExpanded(bool expanded) -> void;
    auto setImage(Image& image) -> void;
    auto setImageSelected(Image& image) -> void;
    auto setImage() -> void;
    auto find( HTREEITEM _hTreeItem ) -> TreeViewItem*;
    auto parentTreeView() -> TreeView*;
    auto addItem(TreeViewItem* parent) -> void;

    pTreeViewItem(TreeViewItem& treeViewItem) : treeViewItem(treeViewItem) {}
};

struct pTreeView : pWidget {
    TreeView& treeView;
    std::vector<Image*> images;

    auto append(TreeViewItem& item) -> void;
    auto remove(TreeViewItem& item) -> void;
    auto reset() -> void;
    auto rebuild() -> void;
    auto update() -> void;
    auto create() -> void;
    auto buildImageList() -> void;
    auto addToImageList(Image* image) -> int;
    auto setImageList() -> void;
    auto setFont(std::string font) -> void;
    auto onActivate() -> void;
    auto onChange() -> void;
	auto setBackgroundColor(unsigned color) -> void;
	auto setForegroundColor(unsigned color) -> void;

    static auto CALLBACK subclassWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT;
    pTreeView(TreeView& treeView) : pWidget(treeView), treeView(treeView) {}
};

struct pViewport : public pWidget {
    Viewport& viewport;

    auto handle() -> uintptr_t;
    auto setDroppable(bool droppable) -> void;
    static auto CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT;

    auto rebuild() -> void;
    auto create() -> void;

    pViewport(Viewport& viewport) : pWidget(viewport), viewport(viewport) {}
};
//Layout Widgets are not directly accessable in frontend
struct pFrame : pWidget {
    Widget& widget;

    auto minimumSize() -> Size;
    auto setGeometry(Geometry geometry) -> void;
    auto setText(std::string text) -> void;
    auto rebuild() -> void;
    auto create() -> void;

    pFrame(Widget& widget) : pWidget(widget), widget(widget) { }
};

struct pTabFrame : pWidget {
    TabFrameLayout::TabFrame& tabFrame;
    static HBRUSH bkgndBrush;

    auto minimumSize() -> Size;
    auto borderSize() -> unsigned;
    auto setGeometry(Geometry geometry) -> void;
    auto rebuild() -> void;
    auto create() -> void;
    auto append(std::string text, Image* image) -> void;
    auto remove(unsigned selection) -> void;
    auto setText(unsigned selection, std::string text) -> void;
    auto setSelection(unsigned selection) -> void;
    auto onChange() -> void;
    auto setImage(unsigned selection, Image& image) -> void;
    auto buildImageList() -> void;
    auto setFont(std::string font) -> void;
    static auto CALLBACK subclassWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT;
    static auto updateTabBackgroundForControl(HWND tab, HWND control) -> void;

    pTabFrame(TabFrameLayout::TabFrame& tabFrame) : pWidget(tabFrame), tabFrame(tabFrame) {}
};

struct pTimer {
    Timer& timer;
    UINT_PTR htimer;
    static std::vector<pTimer*> timers;
    static auto CALLBACK timeoutProc(HWND hwnd, UINT msg, UINT_PTR timerID, DWORD time) -> void;
    auto setEnabled(bool enabled) -> void;
    auto setInterval(unsigned interval) -> void;
    auto killTimer() -> void;

    pTimer(Timer& timer);
    ~pTimer();
};

struct pMenuBase {
    MenuBase& menuBase;
    HBITMAP hbitmap;
	HICON hicon;

    auto setEnabled(bool enabled) -> void;
    auto setVisible(bool visible) -> void;
    auto setText(const std::string& text) -> void;
    auto setIcon(Image& icon) -> void;
    auto freeIcon() -> void;
	auto setMenuInfo(HMENU parent) -> void;
	auto setMenuItemInfo(HMENU parent) -> void;
    auto parentWindow() -> Window*;
    auto parentMenu() -> Menu*;
	static auto findMenu(unsigned id) -> MenuBase*;
	static auto measureItem( LPMEASUREITEMSTRUCT lpmis ) -> bool;
	static auto drawItem( LPDRAWITEMSTRUCT lpdis ) -> bool;
    virtual auto init() -> void {}

    pMenuBase(MenuBase& menuBase) : menuBase(menuBase), hbitmap(0), hicon(0) {}
    virtual ~pMenuBase();
};

struct pMenu : pMenuBase {
    Menu& menu;
    HMENU hmenu;

    auto append(MenuBase& item) -> void;
    auto remove(MenuBase& item) -> void;
    auto update(Window& window) -> void;

    pMenu(Menu& menu) : pMenuBase(menu), menu(menu) {}
    ~pMenu();
};

struct pMenuItem : pMenuBase {
    MenuItem& menuItem;

    auto onActivate() -> void;
    pMenuItem(MenuItem& menuItem) : pMenuBase(menuItem), menuItem(menuItem) {}
};

struct pMenuCheckItem : pMenuBase {
    MenuCheckItem& menuCheckItem;

    auto setChecked(bool checked) -> void;
    auto onToggle() -> void;
    pMenuCheckItem(MenuCheckItem& menuCheckItem) : pMenuBase(menuCheckItem), menuCheckItem(menuCheckItem) {}
};

struct pMenuRadioItem : pMenuBase {
    MenuRadioItem& menuRadioItem;

    auto setGroup(const std::vector<MenuRadioItem*>& group) -> void {}
    auto setChecked() -> void;
    auto onActivate() -> void;

    pMenuRadioItem(MenuRadioItem& menuRadioItem) : pMenuBase(menuRadioItem), menuRadioItem(menuRadioItem) {}
};

struct pMenuSeparator : pMenuBase {
    MenuSeparator& menuSeparator;

    pMenuSeparator(MenuSeparator& menuSeparator) : pMenuBase(menuSeparator), menuSeparator(menuSeparator) {}
};

struct pBrowserWindow {
    static auto directory(BrowserWindow::State& state) -> std::string;
    static auto file(BrowserWindow::State& state, bool save) -> std::string;
};

struct pMessageWindow {
    static auto error(MessageWindow::State& state) -> MessageWindow::Response;
    static auto information(MessageWindow::State& state) -> MessageWindow::Response;
    static auto question(MessageWindow::State& state) -> MessageWindow::Response;
    static auto warning(MessageWindow::State& state) -> MessageWindow::Response;
    static auto translateResponse(UINT response) -> MessageWindow::Response;
    static auto translateButtons(MessageWindow::Buttons buttons) -> UINT;
};

struct pFont {
    static auto system(unsigned size, std::string style) -> std::string;
    static auto create(std::string desc) -> HFONT;
	static auto create(uint8_t* data, unsigned size) -> HFONT;
    static auto free(HFONT& hfont) -> void;
    static auto size(HFONT hfont, std::string text) -> Size;
    static auto size(std::string font, std::string text) -> Size;
	static auto findMemoryFont(std::string name) -> HFONT;
};

struct pSystem {
    static auto getUserDataFolder() -> std::string;
    static auto getResourceFolder(std::string appIdent) -> std::string;
    static auto getWorkingDirectory() -> std::string;
    static auto getDesktopSize() -> Size;
    static auto sleep(unsigned milliSeconds) -> void;
    static auto isOffscreen( Geometry geometry ) -> bool;
    static auto getOSLang() -> System::Language;
};

struct utf16_t {
    operator wchar_t*() { return buffer; }
    operator const wchar_t*() const { return buffer; }
    utf16_t(const std::string& str = "", unsigned CodePage = CP_UTF8);
    ~utf16_t() { if(buffer) delete[] buffer; }
private: wchar_t* buffer = nullptr;
};

struct utf8_t {
    operator std::string() { return (std::string)buffer; }
    operator const std::string() const { return (std::string)buffer; }
    utf8_t(const wchar_t* s = L"");
    ~utf8_t() { if(buffer) delete[] buffer; }
    utf8_t(const utf8_t&) = delete;
    utf8_t& operator=(const utf8_t&) = delete;
private: char* buffer = nullptr;
};

auto CreateBitmap(Image& image, bool structureOnly = false) -> HBITMAP;
auto CreateBitmapWithPremultipliedAlpha(Image& image) -> HBITMAP;
auto CreateHCursor( HBITMAP hBitmap, unsigned hotSpotX, unsigned hotSpotY ) -> HCURSOR;
auto CreateHIcon(Image& image) -> HICON;
auto getDropPaths(WPARAM wparam) -> std::vector<std::string>;
auto getVersion() -> unsigned;
}
