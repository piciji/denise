
#include "interface.h"
#include <pwd.h>

namespace GUIKIT {

struct pApplication {
    static CocoaDelegate* cocoaDelegate;
    static void run();
    static void processEvents();
    static void quit();
    static void initialize();
};

struct pWindow {
    Window& window;
    CocoaWindow* cocoaWindow = nullptr;
    bool locked = false;
    bool fullScreenToggleDelay = false;
    bool keepMenuVisibility = false;
    NSCursor* customCursor = nullptr;

    auto append(Menu& menu) -> void;
    auto append(Widget& widget) -> void;
    auto append(Layout& layout) -> void;
    auto remove(Menu& menu) -> void;
    auto remove(Widget& widget) -> void;
    auto remove(Layout& layout) -> void;
    auto setGeometry(Geometry geometry) -> void;
    auto setStatusFont(std::string font) -> void;
    auto setBackgroundColor(unsigned color) -> void;
    auto setFocused() -> void;
    auto setVisible(bool visible) -> void;
    auto setResizable(bool resizable) -> void;
    auto setStatusText(std::string text) -> void;
    auto setTitle(std::string text) -> void;
    auto setStatusVisible(bool visible) -> void;
    auto setMenuVisible(bool visible) -> void;
    auto keepMenuVisibilityOnDisplay(bool state) -> void;
    auto setFullScreen(bool fullScreen) -> void;
    auto setDroppable(bool droppable) -> void;
    auto geometry() -> Geometry;
    auto focused() -> bool;
    auto statusBarHeight() -> unsigned;
    auto statusBarReposition() -> void;
    auto moveEvent() -> void;
    auto sizeEvent() -> void;
    auto fullScreenToggleDelayed() -> bool { return fullScreenToggleDelay; }
    auto setTitleForAppMenuItem(Window::Cocoa::AppMenuItem appMenuItem, std::string title) -> void;
    auto isOffscreen() -> bool { return false; } 
    auto handle() -> uintptr_t;
    auto changeCursor( Image& image, unsigned hotSpotX, unsigned hotSpotY ) -> void;
    auto setDefaultCursor() -> void;
    bool disableIconsInTopMenu = false;

    static auto addCustomFont(CustomFont* customFont) -> bool;
    
    pWindow(Window& window);
    ~pWindow();
};

struct pWidget {
    Widget& widget;
    NSView* cocoaView = nullptr;

    virtual auto focused() -> bool;
    virtual auto setFocused() -> void;
    virtual auto minimumSize() -> Size { return {0,0}; }
    virtual auto borderSize() -> unsigned { return 1; }
    virtual auto setEnabled(bool enabled) -> void;
    virtual auto setVisible(bool visible) -> void;
    virtual auto setFont(std::string font) -> void;
    virtual auto setGeometry(Geometry geometry) -> void;
    virtual auto setText(std::string text) -> void {}
    virtual auto setTooltip(std::string tooltip) -> void;
    virtual auto setBackgroundColor(unsigned color) -> void {}
    virtual auto setForegroundColor(unsigned color) -> void {}
    auto add() -> void;
    virtual auto init() -> void {}
	static auto getScaledContainerSize( Size size ) -> Size {
		return size;
	}
	static auto getScaledDim( unsigned& value ) -> unsigned {}

    pWidget(Widget& widget);
    virtual ~pWidget();
};

struct pLineEdit : pWidget {
    LineEdit& lineEdit;

    auto minimumSize() -> Size;
    auto setEditable(bool editable) -> void;
    auto setText(std::string text) -> void;
    auto setMaxLength( unsigned maxLength ) -> void {}
    auto text() -> std::string;
    auto init() -> void;
    auto setDroppable(bool droppable) -> void;

    pLineEdit(LineEdit& lineEdit) : pWidget(lineEdit), lineEdit(lineEdit) { }
};

struct pLabel : pWidget {
    Label& label;

    auto minimumSize() -> Size;
    auto init() -> void;
    auto setGeometry(Geometry geometry) -> void;
    auto setText(std::string text) -> void;
    auto setEnabled(bool enabled) -> void;
    auto setForegroundColor(unsigned color) -> void;
    auto setAlign( Label::Align align ) -> void;

    pLabel(Label& label) : pWidget(label), label(label) { }
};

struct pHyperlink : pWidget {
    Hyperlink& hyperlink;

    auto minimumSize() -> Size;
    auto setText(std::string text) -> void;
    auto setUri( std::string uri, std::string wrap ) -> void;
    auto setGeometry(Geometry geometry) -> void;
    auto setEnabled(bool enabled) -> void;
    auto setForegroundColor(unsigned color) -> void;
    auto init() -> void;
    auto create() -> void;
    auto updateLink() -> void;

    pHyperlink(Hyperlink& hyperlink) : pWidget(hyperlink), hyperlink(hyperlink) { }
};

struct pSquareCanvas : pWidget {
    SquareCanvas& squareCanvas;
    NSImage* surface = nullptr;
    NSBitmapImageRep* bitmap = nullptr;
    
    auto init() -> void;
    auto redraw() -> void;
    auto setBackgroundColor( unsigned color ) -> void;
    auto setBorderColor(unsigned borderSize, unsigned borderColor) -> void;
    auto setGeometry(Geometry geometry) -> void;
    
    pSquareCanvas(SquareCanvas& squareCanvas) : pWidget(squareCanvas), squareCanvas(squareCanvas) {}
};

struct pButton : pWidget {
    Button& button;

    auto minimumSize() -> Size;
    auto setGeometry(Geometry geometry) -> void;
    auto setText(std::string text) -> void;
    auto init() -> void;

    pButton(Button& button) : pWidget(button), button(button) { }
};

struct pCheckButton : pWidget {
    CheckButton& checkButton;

    auto minimumSize() -> Size;
    auto setGeometry(Geometry geometry) -> void;
    auto setChecked(bool checked) -> void;
    auto setText(std::string text) -> void;
    auto init() -> void;

    pCheckButton(CheckButton& checkButton) : pWidget(checkButton), checkButton(checkButton) { }
};

struct pCheckBox : pWidget {
    CheckBox& checkBox;

    auto minimumSize() -> Size;
    auto setChecked(bool checked) -> void;
    auto setText(std::string text) -> void;
    auto setGeometry(Geometry geometry) -> void;
    auto init() -> void;

    pCheckBox(CheckBox& checkBox) : pWidget(checkBox), checkBox(checkBox) { }
};

struct pComboButton : pWidget {
    ComboButton& comboButton;

    auto append(std::string text) -> void;
    auto remove(unsigned selection) -> void;
    auto minimumSize() -> Size;
    auto setGeometry(Geometry geometry) -> void;
    auto reset() -> void;
    auto setSelection(unsigned selection) -> void;
    auto setText(unsigned selection, std::string text) -> void;
    auto init() -> void;

    pComboButton(ComboButton& comboButton) : pWidget(comboButton), comboButton(comboButton) { }
};

struct pSlider : pWidget {
    Slider& slider;

    auto minimumSize() -> Size;
    auto setGeometry(Geometry geometry) -> void;
    auto setLength(unsigned length) -> void;
    auto setPosition(unsigned position) -> void;

    auto init() -> void;

    pSlider(Slider& slider) : pWidget(slider), slider(slider) { }
};

struct pRadioBox : pWidget {
    RadioBox& radioBox;
    NSView* inner = nil;
    
    auto minimumSize() -> Size;
    auto setGeometry(Geometry geometry) -> void;
    auto setChecked() -> void;
    auto setGroup(const std::vector<RadioBox*>& group) -> void {}
    
    auto focused() -> bool;
    auto setFocused() -> void;
    auto setEnabled(bool enabled) -> void;
    auto setFont(std::string font) -> void;
    auto setTooltip(std::string tooltip) -> void;

    auto init() -> void;
    auto setText(std::string text) -> void;

    pRadioBox(RadioBox& radioBox) : pWidget(radioBox), radioBox(radioBox) { }
};

struct pProgressBar : pWidget {
    ProgressBar& progressBar;

    auto minimumSize() -> Size;
    auto init() -> void;
    auto setPosition(unsigned position) -> void;

    pProgressBar(ProgressBar& progressBar) : pWidget(progressBar), progressBar(progressBar) { }
};

struct pListView : pWidget {
    ListView& listView;
    std::vector<std::vector<NSImage*>> images;

    auto append(const std::vector<std::string>& list) -> void;
    auto autoSizeColumns() -> void;
    auto remove(unsigned selection) -> void;
    auto reset() -> void;
    auto setHeaderText(std::vector<std::string> list) -> void;
    auto setHeaderVisible(bool visible) -> void;
    auto setSelection(unsigned selection) -> void;
    auto setSelected(bool selected) -> void;
    auto setText(unsigned selection, unsigned position, const std::string& text) -> void;
    auto init() -> void;
    auto setEnabled(bool enabled) -> void;
    auto setGeometry(Geometry geometry) -> void;
    auto setImage(unsigned selection, unsigned position, Image& image) -> void;
    auto releaseRowImages(unsigned selection) -> void;
    auto releaseAllImages() -> void;
    auto setBackgroundColor(unsigned color) -> void;

    pListView(ListView& listView) : pWidget(listView), listView(listView) { }
    ~pListView() { releaseAllImages(); }
};
    
struct pTreeViewItem {
    TreeViewItem& treeViewItem;
    TreeViewWrapper* wrapper = nil;
    NSImage* usensimage = nil;
    NSImage* nsimage = nil;
    NSImage* nsimageSelected = nil;
        
    auto append(TreeViewItem& item) -> void;
    auto remove(TreeViewItem& item) -> void;
    auto reset() -> void;
    auto update() -> void;
    auto init() -> void;
    auto setText(std::string text) -> void;
    auto setSelected() -> void;
    auto setExpanded(bool expanded) -> void;
    auto setImage(Image& image) -> void;
    auto setImageSelected(Image& image) -> void;
    auto parentTreeView() -> TreeView*;
    
    pTreeViewItem(TreeViewItem& treeViewItem) : treeViewItem(treeViewItem) {}
    ~pTreeViewItem();
};
    
struct pTreeView : pWidget {
    TreeView& treeView;
    
    auto append(TreeViewItem& item) -> void;
    auto remove(TreeViewItem& item) -> void;
    auto reset() -> void;
    auto update() -> void;
    auto init() -> void;
    auto setBackgroundColor(unsigned color) -> void;
    
    pTreeView(TreeView& treeView) : pWidget(treeView), treeView(treeView) {}
};
    
struct pViewport : public pWidget {
    Viewport& viewport;

    auto handle() -> uintptr_t;
    auto setDroppable(bool droppable) -> void;
    auto init() -> void;

    pViewport(Viewport& viewport) : pWidget(viewport), viewport(viewport) { }
};
//Layout Widgets are not directly accessable in frontend
struct pFrame : pWidget {
    Widget& widget;

    auto minimumSize() -> Size;
    auto setGeometry(Geometry geometry) -> void;
    auto setText(std::string text) -> void;
    auto setFont(std::string font) -> void;
    auto init() -> void;

    pFrame(Widget& widget) : pWidget(widget), widget(widget) { }
};

struct pTabFrame : pWidget {
    TabFrameLayout::TabFrame& tabFrame;
    std::vector<CocoaTabFrameItem*> tabs;
    std::vector<NSImage*> cocoaImages;

    auto minimumSize() -> Size;
    auto setGeometry(Geometry geometry) -> void;
    auto init() -> void;
    auto append(std::string text, Image* image) -> void;
    auto remove(unsigned selection) -> void;
    auto setText(unsigned selection, std::string text) -> void;
    auto setSelection(unsigned selection) -> void;
    auto setImage(unsigned selection, Image& image) -> void;

    pTabFrame(TabFrameLayout::TabFrame& tabFrame) : pWidget(tabFrame), tabFrame(tabFrame) {}
    ~pTabFrame();
};

struct pTimer {
    Timer& timer;
    CocoaTimer* cocoaTimer = nullptr;

    auto setEnabled(bool enabled) -> void;
    auto setInterval(unsigned interval) -> void;

    pTimer(Timer& timer);
    ~pTimer();
};

struct pMenuBase {
    MenuBase& menuBase;
    NSMenuItem* cocoaBase = nullptr;
    NSMenuItem* cocoaBaseContext = nullptr;

    auto setEnabled(bool enabled) -> void;
    auto setVisible(bool visible) -> void;
    virtual auto setText(const std::string& text) -> void;
    virtual void setIcon(Image& icon);
    virtual auto init() -> void {}

    pMenuBase(MenuBase& menuBase) : menuBase(menuBase) {}
    virtual ~pMenuBase();
};

struct pMenu : pMenuBase {
    Menu& menu;
    
    auto append(MenuBase& item) -> void;
    auto remove(MenuBase& item) -> void;
    auto update(Window& window) -> void;
    auto init() -> void;
    auto setText(const std::string& text) -> void;
    auto setIcon(Image& icon) -> void;

    pMenu(Menu& menu);
    ~pMenu();
};

struct pMenuItem : pMenuBase {
    MenuItem& menuItem;

    auto init() -> void;
    pMenuItem(MenuItem& menuItem);
};

struct pMenuCheckItem : pMenuBase {
    MenuCheckItem& menuCheckItem;

    auto setChecked(bool checked) -> void;
    auto init() -> void;
    pMenuCheckItem(MenuCheckItem& menuCheckItem);
};

struct pMenuRadioItem : pMenuBase {
    MenuRadioItem& menuRadioItem;

    auto setGroup(const std::vector<MenuRadioItem*>& group) -> void {}
    auto setChecked() -> void;
    auto init() -> void;
    pMenuRadioItem(MenuRadioItem& menuRadioItem);
};

struct pMenuSeparator : pMenuBase {
    MenuSeparator& menuSeparator;

    auto init() -> void;
    pMenuSeparator(MenuSeparator& menuSeparator);
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

    static auto message(MessageWindow::State& state, NSAlertStyle style) -> MessageWindow::Response;
};

struct pFont {
    static auto system(unsigned size, std::string style) -> std::string;
    static auto size(std::string font, std::string text) -> Size;
    static auto cocoaFont(std::string desc) -> NSFont*;
    static auto size(NSFont* font, std::string text) -> Size;
    static auto add( CustomFont* customFont ) -> bool;
};

struct pSystem {
    static auto getUserDataFolder() -> std::string;
    static auto getResourceFolder(std::string appIdent) -> std::string;
    static auto getWorkingDirectory() -> std::string;
    static auto getDesktopSize() -> Size;
    static auto sleep(unsigned milliSeconds) -> void;
    static auto isOffscreen( Geometry geometry ) -> bool { return false; }
    static auto getOSLang() -> System::Language;
};

auto NSMakeImage(Image& image, unsigned width = 0, unsigned height = 0) -> NSImage*;
static auto DropPathsOperation(id<NSDraggingInfo> sender) -> NSDragOperation;
auto getDropPaths(id<NSDraggingInfo> sender) -> std::vector<std::string>;
}
