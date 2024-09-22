
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <pwd.h>
#include <pthread.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <fcntl.h>

namespace GUIKIT {

struct pApplication {
    static auto run() -> void;
    static auto processEvents() -> void;
    static auto quit() -> void;
    static auto initialize() -> void;
	static auto requestClipboardText() -> void;
    static auto setClipboardText( std::string text ) -> void;
	static auto pasteClipboardCallback(GtkClipboard* clipboard, const gchar* text, gpointer data) -> void;
    static auto fetchDesktopSession() -> void;
	static auto canGrabInputFocusAfterDnDFromOtherApp() -> bool;

    enum class DesktopSession { Cinnamon, KDE, Gnome, XFCE, Mate, Unity, Unknown };

    static DesktopSession desktopSession;
};

struct pWindow {
    Window& window;
    pViewport* viewport = nullptr;
    GtkWidget* widget;
    GtkWidget* verticalLayout;
    GtkWidget* menu;
	GtkWidget* contextMenu;
    GtkWidget* mainDisplay;
    GtkWidget* statusContainer;
    GtkAllocation lastAllocation;
    bool overrideBackgroundColor = false;
    unsigned backgroundColor;
    unsigned statusHeight = 0;
    unsigned menuHeight = 0;
    bool locked = false;
    Timer timer;
	Timer timerResize;
	Timer timerFullscreen;
    GdkCursor* cursor = nullptr;
	bool isMinimized = false;
    bool resizing = false;
    bool requestFullscreenToggle = false;
    gulong dragMotionId = 0;

    auto append(Menu& menu) -> void;
    auto append(Widget& widget) -> void;
    auto append(Layout& layout) -> void;
	auto append(StatusBar& statusBar) -> void;
    auto remove(Menu& menu) -> void;
    auto remove(Widget& widget) -> void;
    auto remove(Layout& layout) -> void {}
	auto remove(StatusBar& statusBar) -> void;
    auto setGeometry(Geometry geometry) -> void;
    auto setBackgroundColor(unsigned color) -> void;
    auto setFocused() -> void;
    auto setVisible(bool visible) -> bool;
    auto setResizable(bool resizable) -> void;
    auto setTitle(std::string text) -> void;
    auto setMenuVisible(bool visible) -> void;
    auto setStatusVisible(bool visible) -> void;
    auto setFullScreen(bool fullScreen) -> void;
    auto updateFullScreen( bool inUse, unsigned displayId = 0, unsigned settingId = 0) -> void;
    auto fullScreenToggleDelayed() -> bool;
    auto setDroppable(bool droppable) -> void;
    auto geometry() -> Geometry;
    auto focused() -> bool;
    auto calcMenuHeight() -> void;
    auto resize(Geometry geo) -> void;
    auto isOffscreen() -> bool { return false; } 
    auto handle() -> uintptr_t;
    auto changeCursor( Image& image, unsigned hotSpotX, unsigned hotSpotY ) -> void;
    auto setDefaultCursor() -> void;
    auto setPointerCursor() -> void;
    auto setIcon( std::string path ) -> bool;
	auto minimized() -> bool;
	auto restore() -> void;
	auto setForeground() -> void;
    auto getScrollbarWidth() -> unsigned { return 25; }
    auto applyAspectRatio() -> void;
    auto applyMaximizeCorrection(Geometry& geo) -> void {}
    auto updateGeometryHint() -> void;
    static auto mouseMove(GtkWidget* widget, GdkEventButton* event, pWindow* self) -> gboolean;
    static auto mousePress(GtkWidget* widget, GdkEventButton* event, pWindow* self) -> gboolean;
    static auto mouseRelease(GtkWidget* widget, GdkEventButton* event, pWindow* self) -> gboolean;
    static auto monitorsChanged(GdkScreen* screen, pWindow* self) -> void;

    static auto drawMain(GtkWidget* widget, cairo_t* context, Window* window) -> gboolean;
    static auto draw(GtkWidget* widget, cairo_t* context, Window* window) -> gboolean;
    static auto close(GtkWidget* widget, GdkEvent* event, Window* window) -> gint;
    static auto drop(GtkWidget* widget, GdkDragContext* context, gint x, gint y, GtkSelectionData* data, guint type, guint timestamp, Window* window) -> void;
    static auto dragMove(GtkWidget* widget, GdkDragContext* context, gint x, gint y, guint time, pWindow* self) -> gboolean;
    static auto dragEnd(GtkWidget* widget, GdkDragContext* context, guint time, pWindow* self) -> void;
    //static auto dragBegin(GtkWidget* widget, GdkDragContext* context, GtkSelectionData* data, guint type, guint timestamp, pWindow* self) -> void;
    static auto dragDataReceived(GtkWidget* widget, GdkDragContext* context, gint x, gint y, GtkSelectionData* data, guint type, guint timestamp, pWindow* self) -> void;
    static auto dragDrop(GtkWidget* widget, GdkDragContext* context, gint x, gint y, guint time, pWindow* self) -> gboolean;

    static auto configure(GtkWidget* widget, GdkEvent* event, pWindow* p) -> gboolean;
    static auto sizeAllocate(GtkWidget* widget, GtkAllocation* allocation, pWindow* p) -> void;
    static auto getPreferredWidth(GtkWidget* widget, int* minimalWidth, int* naturalWidth) -> void;
    static auto getPreferredHeight(GtkWidget* widget, int* minimalHeight, int* naturalHeight) -> void;
    static auto onButtonPressed(GtkWidget* widget, GdkEventButton* event, Window* window) -> gboolean;
    static auto stateChange(GtkWidget* widget, GdkEventWindowState* event, Window* window) -> gboolean;
    static auto onRealize(GtkWidget* widget, pWindow* self) -> void;
	
	auto moveWindow(GdkEvent* event) -> void;
	auto sizeWindow(GtkAllocation* allocation) -> void;
    
    static auto addCustomFont( CustomFont* customFont ) -> bool;

    pWindow(Window& window, Window::Hints hints = Window::Hints::Default);
};

struct pStatusBar {
    StatusBar& statusBar;
    GtkWidget* gridWidget = nullptr;
	PangoFontDescription* pfont = nullptr;
    std::vector<Widget*> usedWidgets;
    std::vector<GtkWidget*> helpers;
	unsigned statusHeight = 0;
    
    auto create() -> void;
    auto destroy() -> void;
    
    auto setFont(const std::string& font) -> void;
    auto setText(const std::string& text) -> void;
        
    auto update() -> void;
    auto updatePart( StatusBar::Part& part ) -> void;
    //auto updateTooltip( StatusBar::Part& part ) -> void;
    auto setVisible(bool visible) -> void;
    auto getHeight() -> unsigned;    
	auto getWidth(const std::string& text) -> unsigned { return 0; }
	
	static auto onClick(GtkWidget* widget, GdkEventButton* event, StatusBar::Part* part) -> void;
	static auto onEnter(GtkWidget* widget, GdkEventButton* event, StatusBar::Part* part) -> void;
	static auto onLeave(GtkWidget* widget, GdkEventButton* event, StatusBar::Part* part) -> void;
    
    pStatusBar(StatusBar& statusBar);
    ~pStatusBar();
};

struct pWidget {
    Widget& widget;
    GtkWidget* gtkWidget = nullptr;
    PangoFontDescription* pfont = nullptr;
    bool locked = false;	
	GtkWidget* parentWidget = nullptr;
	
	struct {
        bool updated = false;
        Size minimumSize = {0, 0};
    } calculatedMinimumSize;

    virtual auto focused() -> bool;
    virtual auto setFocused() -> void;
    virtual auto minimumSize() -> Size { return {0,0}; }
    virtual auto borderSize() -> unsigned { return 1; }
    virtual auto setEnabled(bool enabled) -> void;
    virtual auto setVisible(bool visible) -> void;
    virtual auto setFont(std::string font) -> void;
    virtual auto setGeometry(Geometry geometry) -> void;
    virtual auto setText(const std::string& text) -> void {}
    virtual auto destroy() -> void;
    virtual auto setForegroundColor(unsigned color) -> void;
    virtual auto setBackgroundColor(unsigned color) -> void;
	auto getMinimumSize() -> Size;
    auto getMinimumFontSize() -> Size;
    auto setTooltip(std::string tooltip) -> void;
    virtual auto add() -> void;
    virtual auto init() -> void {}
    virtual auto getContainerWidget(int selection = -1) -> GtkWidget* { return nullptr; }
    virtual auto getDisplacement() -> Position { return {0,0}; }
	static auto getScaledDim( unsigned value ) -> unsigned { return value; }

    pWidget(Widget& widget);
    virtual ~pWidget();
};

struct pLineEdit : pWidget {
    LineEdit& lineEdit;

    auto minimumSize() -> Size;
    auto setEditable(bool editable) -> void;
    auto setText(const std::string& text) -> void;
    auto text() -> std::string;
    auto setMaxLength( unsigned maxLength ) -> void;
    auto init() -> void;
    static auto onChange(LineEdit* self) -> void;
    static auto onFocus(LineEdit* self) -> bool;
    static auto dropEvent(GtkWidget* widget, GdkDragContext* context, gint x, gint y, GtkSelectionData* data, guint type, guint timestamp, LineEdit* lineEdit) -> void;
    auto create() -> void;
    auto setDroppable(bool droppable) -> void;

    pLineEdit(LineEdit& lineEdit) : pWidget(lineEdit), lineEdit(lineEdit) { }
};

struct pMultilineEdit : pWidget {
    MultilineEdit& multilineEdit;
	GtkWidget* subWidget = nullptr;
	GtkTextBuffer* buffer = nullptr;

    auto setEditable(bool editable) -> void;
    auto setText(const std::string& text) -> void;
    auto text() -> std::string;
	auto setForegroundColor(unsigned color) -> void;
	auto destroy() -> void;
    
    auto init() -> void;
    static auto onChange(MultilineEdit* self) -> void;
    static auto onFocus(MultilineEdit* self) -> bool;
    auto create() -> void;
	
	pMultilineEdit(MultilineEdit& multilineEdit) : pWidget(multilineEdit), multilineEdit(multilineEdit) {}
};

struct pLabel : pWidget {
    Label& label;

    auto minimumSize() -> Size;
    auto setText(const std::string& text) -> void;
    auto init() -> void;
    auto create() -> void;
    auto setAlign( Label::Align align ) -> void;

    pLabel(Label& label) : pWidget(label), label(label) { }
};

struct pHyperlink : pWidget {
    Hyperlink& hyperlink;

    auto minimumSize() -> Size;
    auto setText(const std::string& text) -> void;
    auto setUri( std::string uri, std::string wrap ) -> void;
    auto init() -> void;
    auto create() -> void;
    auto updateLink() -> void;

    pHyperlink(Hyperlink& hyperlink) : pWidget(hyperlink), hyperlink(hyperlink) { }
};

struct pSquareCanvas : pWidget {
    SquareCanvas& squareCanvas;
    GdkPixbuf* surface = nullptr;
    
    auto init() -> void;
    auto create() -> void;
    auto destroy() -> void;
    auto redraw() -> void;
    auto update() -> void;
    auto setBackgroundColor( unsigned color ) -> void;
    auto setBorderColor(unsigned borderSize, unsigned borderColor) -> void;
    auto setGeometry(Geometry geometry) -> void;
    static auto mousePress(GtkWidget* widget, GdkEventButton* event, pSquareCanvas* self) -> gboolean;
    static auto mouseRelease(GtkWidget* widget, GdkEventButton* event, pSquareCanvas* self) -> gboolean;
    static auto expose(GtkWidget* widget, GdkEventExpose* event, pSquareCanvas* self) -> signed;
    
    pSquareCanvas(SquareCanvas& squareCanvas) : pWidget(squareCanvas), squareCanvas(squareCanvas) {}
};

struct pImageView : pWidget {
    ImageView& imageView;
    GdkPixbuf* surface = nullptr;

    auto setImage(Image* image) -> void;
    auto setUri( std::string uri ) -> void {}
    auto init() -> void;
    auto create() -> void;
    auto update() -> void;
    auto destroy() -> void;
    auto minimumSize() -> Size;
    auto setGeometry(Geometry geometry) -> void;
    static auto mousePress(GtkWidget* widget, GdkEventButton* event, pImageView* self) -> gboolean;
    static auto expose(GtkWidget* widget, GdkEventExpose* event, pImageView* self) -> signed;
    static auto mouseMove(GtkWidget* widget, GdkEventButton* event, pImageView* self) -> gboolean;

    pImageView(ImageView& imageView) : pWidget(imageView), imageView(imageView) {}
};

struct pButton : pWidget {
    Button& button;

	auto setImage(Image* image) -> void;
    auto minimumSize() -> Size;
    static auto onActivate(Button* self) -> void;
    auto setText(const std::string& text) -> void;
    auto init() -> void;
    auto create() -> void;

    pButton(Button& button) : pWidget(button), button(button) { }
};

struct pStepButton : pWidget {
    StepButton& stepButton;
    
    auto updateRange() -> void;
    auto setValue( int16_t value ) -> void;

    auto minimumSize() -> Size;
    auto init() -> void;
    auto create() -> void;
    static auto onChange(GtkAdjustment* adjustment, StepButton* self) -> void;

    pStepButton(StepButton& stepButton) : pWidget(stepButton), stepButton(stepButton) {}
};


struct pCheckButton : pWidget {
    CheckButton& checkButton;

    auto minimumSize() -> Size;
    auto setChecked(bool checked) -> void;
    auto setText(const std::string& text) -> void;
    static auto onToggle(GtkToggleButton* toggleButton, CheckButton* self) -> void;
    auto init() -> void;
    auto create() -> void;

    pCheckButton(CheckButton& checkButton) : pWidget(checkButton), checkButton(checkButton) { }
};

struct pCheckBox : pWidget {
    CheckBox& checkBox;

    auto minimumSize() -> Size;
	auto setGeometry(Geometry geometry) -> void;
    auto setChecked(bool checked) -> void;
    auto setText(const std::string& text) -> void;
    static auto onToggle(GtkToggleButton* toggleButton, CheckBox* self) -> void;
    auto init() -> void;
    auto create() -> void;

    pCheckBox(CheckBox& checkBox) : pWidget(checkBox), checkBox(checkBox) { }
};

struct pComboButton : pWidget {
    ComboButton& comboButton;

    auto append(std::string text) -> void;
    auto remove(unsigned selection) -> void;
    auto minimumSize() -> Size;
    auto reset() -> void;
    auto setSelection(unsigned selection) -> void;
    auto setText(unsigned selection, const std::string& text) -> void;
    auto init() -> void;
    auto create() -> void;
    static auto onChange(ComboButton* self) -> void;

    pComboButton(ComboButton& comboButton) : pWidget(comboButton), comboButton(comboButton) { }
};

struct pSlider : pWidget {
    Slider& slider;

    auto minimumSize() -> Size;
    auto setLength(unsigned length) -> void;
    auto setPosition(unsigned position) -> void;
	
    auto init() -> void;
    auto create() -> void;
    static auto onChange(GtkRange* gtkRange, Slider* self) -> void;

    pSlider(Slider& slider) : pWidget(slider), slider(slider) { }
};

struct pRadioBox : pWidget {
    RadioBox& radioBox;

    auto minimumSize() -> Size;
	auto setGeometry(Geometry geometry) -> void;
    auto setChecked() -> void;
    auto setGroup(const std::vector<RadioBox*>& group) -> void;

    auto init() -> void;
    auto create() -> void;
    auto setText(const std::string& text) -> void;
    static auto onActivate(GtkToggleButton* toggleButton, RadioBox* self) -> void;
    auto parent() -> pRadioBox&;

    pRadioBox(RadioBox& radioBox) : pWidget(radioBox), radioBox(radioBox) { }
};

struct pProgressBar : pWidget {
    ProgressBar& progressBar;

    auto minimumSize() -> Size;
    auto init() -> void;
    auto create() -> void;
    auto setPosition(unsigned position) -> void;

    pProgressBar(ProgressBar& progressBar) : pWidget(progressBar), progressBar(progressBar) { }
};

struct pListView : pWidget {
    ListView& listView;

    GtkWidget* subWidget = nullptr;
    GtkListStore* store = nullptr;
	
	GtkWidget* customTooltip = nullptr;
	Label* customTooltipLabel = nullptr;
    bool pressed = false;

    struct GtkColumn {
        GtkTreeViewColumn* column;
        GtkCellRenderer* icon;
        GtkCellRenderer* text;
        GtkWidget* label;
    };
    std::vector<GtkColumn> column;
	
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
    auto create() -> void;
    auto setFont(std::string font) -> void;
    auto setImage(unsigned selection, unsigned position, Image& image) -> void;
    auto focused() -> bool;
    auto setFocused() -> void;
    auto setForegroundColor(unsigned color) -> void;
    auto setBackgroundColor(unsigned color) -> void;
	auto setRowTooltip(unsigned selection, std::string tooltip) -> void {}
	auto createCustomTooltip() -> void;
	auto colorRowTooltips( bool colorTip ) -> void {}
    auto lockRedraw() -> void {}
    auto unlockRedraw() -> void {}
    auto setSelectionColor(unsigned foregroundColor = 0, unsigned backgroundColor = 0) -> void;
	auto setFirstRowColor(unsigned foregroundColor = 0, unsigned backgroundColor = 0) -> void {}

    auto destroy() -> void;
	auto applyDataFunc(GtkTreeViewColumn* gtkColumn, GtkCellRenderer* renderer, GtkTreeIter* iter, GtkTreeModel* model) -> void;

    static auto onActivate(GtkTreeView* treeView, GtkTreePath* path, GtkTreeViewColumn* column, ListView* self) -> void;
    static auto onChange(GtkTreeView* treeView, ListView* self) -> void;
    static auto onPress(GtkTreeView* treeView, GdkEventButton* event, ListView* self) -> gboolean;
	static auto onTooltip(GtkWidget* widget, gint x, gint y, gboolean keyboard_tip, GtkTooltip* tooltip, ListView* self) -> gboolean;
	static auto dataFunc(GtkTreeViewColumn* column, GtkCellRenderer* renderer, GtkTreeModel* model, GtkTreeIter* iter, pListView* p) -> void;

    pListView(ListView& listView) : pWidget(listView), listView(listView) { }
    ~pListView() { destroy(); }
};

struct pTreeViewItem {
    TreeViewItem& treeViewItem;
    GtkTreeIter iter;
    GdkPixbuf* gdkimage = nullptr;
    GdkPixbuf* gdkimageSelected = nullptr;
	GdkPixbuf* gdkimageExpanded = nullptr;

    auto append(TreeViewItem& item) -> void;
    auto remove(TreeViewItem& item) -> void;
    auto reset() -> void;
    auto invalidateParent() -> void;
    auto init() -> void {}
    auto update(TreeViewItem* parent) -> void;
    auto setText(const std::string& text) -> void;
    auto setSelected() -> void;
    auto setExpanded(bool expanded) -> void;
    auto setImage(Image& image) -> void;
    auto setImageSelected(Image& image) -> void;
	auto setImageExpanded(Image& image) -> void;
    auto showImage(bool selected) -> void;
	auto updateImageExpanded() -> void;
    auto find( char* _iter ) -> TreeViewItem*;
    auto parentTreeView() -> TreeView*;
    auto addItem(TreeViewItem* parent) -> void;

    pTreeViewItem(TreeViewItem& treeViewItem) : treeViewItem(treeViewItem) {}
};

struct pTreeView : pWidget {
    TreeView& treeView;
    std::vector<Image*> images;
    std::vector<GdkPixbuf*> gdkImages;    

    GtkTreeStore* gtkTreeStore = nullptr;
    GtkTreeModel* gtkTreeModel = nullptr;
    GtkTreeSelection* gtkTreeSelection = nullptr;
    GtkTreeView* gtkTreeView = nullptr;
    GtkTreeViewColumn* gtkTreeViewColumn = nullptr;
    GtkCellRenderer* gtkCellPixbuf = nullptr;
    GtkCellRenderer* gtkCellText = nullptr;
    GtkWidget* subWidget = nullptr;
    
    auto append(TreeViewItem& item) -> void;
    auto remove(TreeViewItem& item) -> void;
    auto reset() -> void;
    auto rebuild() -> void;
    auto update() -> void;
    auto create() -> void;
    auto init() -> void;
    auto destroy() -> void;
	auto focused() -> bool;
    auto setFocused() -> void;
    auto getImage(Image* image) -> GdkPixbuf*;
    auto clearImages() -> void;
    auto setForegroundColor(unsigned color) -> void;
    auto setBackgroundColor(unsigned color) -> void;

    static auto onActivate(GtkTreeView* treeView, GtkTreePath* gtkPath, GtkTreeViewColumn* column, TreeView* self) -> void;
	static auto onCollapse(GtkTreeView* treeView, GtkTreeIter* iter, GtkTreePath* gtkPath, gpointer userData) -> void;
	static auto onExpand(GtkTreeView* treeView, GtkTreeIter* iter, GtkTreePath* gtkPath, gpointer userData) -> void;
    static auto onChange(GtkTreeSelection* selection, TreeView* self) -> void;
    
    pTreeView(TreeView& treeView) : pWidget(treeView), treeView(treeView) {}
    ~pTreeView() { clearImages(); destroy(); }
};

struct pViewport : public pWidget {
    Viewport& viewport;
    bool dragAnalyzed = false;
    std::vector<std::string> paths;

    auto handle(bool hintRecreation) -> uintptr_t;
    auto setDroppable(bool droppable) -> void;
    auto setGeometry(Geometry geometry) -> void;
    auto add() -> void;
    static auto dragMove(GtkWidget* widget, GdkDragContext* context, gint x, gint y, guint time, Viewport* viewport) -> gboolean;
    static auto dragEnd(GtkWidget* widget, GdkDragContext* context, Viewport* viewport) -> void;
    static auto dragDataReceived(GtkWidget* widget, GdkDragContext* context, gint x, gint y, GtkSelectionData* data, guint type, guint timestamp, Viewport* viewport) -> void;

    static auto mouseLeave(GtkWidget* widget, GdkEventButton* event, pViewport* self) -> gboolean;
    static auto mouseMove(GtkWidget* widget, GdkEventButton* event, pViewport* self) -> gboolean;
    static auto mousePress(GtkWidget* widget, GdkEventButton* event, pViewport* self) -> gboolean;
    static auto mouseRelease(GtkWidget* widget, GdkEventButton* event, pViewport* self) -> gboolean;
	static auto drawEvent(GtkWidget* widget, cairo_t* context, pViewport* self) -> gboolean;
    static auto monitorsChanged(GdkScreen* screen, pViewport* self) -> void;

    auto init() -> void;
    auto create() -> void;

    pViewport(Viewport& viewport) : pWidget(viewport), viewport(viewport) { }
};
//Layout Widgets are not directly accessable in frontend
struct pFrame : pWidget {
    Widget& widget;
    GtkWidget* box = nullptr;

    auto minimumSize() -> Size;
    auto setGeometry(Geometry geometry) -> void;
    auto setText(const std::string& text) -> void;
    auto init() -> void;
    auto create() -> void;
    auto setFont(std::string font) -> void;
    auto getContainerWidget(int selection = -1) -> GtkWidget*;
    auto getDisplacement() -> Position;
    auto frameSize() -> Size;
    auto borderSize() -> unsigned;
    auto setEnabled(bool enabled) -> void;

    pFrame(Widget& widget) : pWidget(widget), widget(widget) { }
};

struct pTabFrame : pWidget {
    TabFrameLayout::TabFrame& tabFrame;

    struct Tab {
        GtkWidget* child;
        GtkWidget* container;
        GtkWidget* layout;
        GtkWidget* image;
        GtkWidget* title;
    };
    std::vector<Tab> tabs;

    auto minimumSize() -> Size;
	auto borderSize() -> unsigned;
    auto init() -> void;
    auto create() -> void;
    auto append(std::string text, Image* image) -> void;
    auto remove(unsigned selection) -> void;
    auto setText(unsigned selection, const std::string& text) -> void;
    auto setSelection(unsigned selection) -> void;
    static auto onChange(GtkNotebook* notebook, GtkWidget* page, unsigned selection, TabFrameLayout::TabFrame* self) -> void;
    auto setImage(unsigned selection, Image& image) -> void;
    auto setFont(std::string font) -> void;
    auto getContainerWidget(int selection = -1) -> GtkWidget*;
    auto getDisplacement() -> Position;

    pTabFrame(TabFrameLayout::TabFrame& tabFrame) : pWidget(tabFrame), tabFrame(tabFrame) {}
};

struct pTimer {
    Timer& timer;
    guint gtimer = 0;
    auto setEnabled(bool enabled) -> void;
    auto setInterval(unsigned interval) -> void;
    auto killTimer() -> void;

    pTimer(Timer& timer) : timer(timer) {}
    ~pTimer() { killTimer(); }
};

struct pMenuBase {
    MenuBase& menuBase;
	
	struct Element {
		GtkWidget* widget = nullptr;
		GtkWidget* box = nullptr;
		GtkImage* gtkImage = nullptr;		
		GtkWidget* label = nullptr;
	} element, elementC;
	
    bool locked = false;
    bool updatedPadding = false;

    auto setEnabled(bool enabled) -> void;
    auto setVisible(bool visible) -> void;
    auto setText(const std::string& text) -> void;
    auto setIcon(Image& icon) -> void;
    auto destroy(Element& el) -> void;
	virtual auto destroy() -> void;
    virtual auto rebuild() -> void;
    virtual auto init() -> void {}
	auto updateItemBox(Element& el) -> void;

    pMenuBase(MenuBase& menuBase) : menuBase(menuBase) {}
    virtual ~pMenuBase();
};

struct pMenu : pMenuBase {
    Menu& menu;
    GtkWidget* gtkMenu = nullptr;
	GtkWidget* cgtkMenu = nullptr;
    std::string paddingLeft = ""; // only for check and radio childs

    auto append(MenuBase& item) -> void;
    auto remove(MenuBase& item) -> void;
    auto update(Window& window) -> void;
    auto destroy() -> void;
    auto rebuild() -> void;
    auto init() -> void;

    pMenu(Menu& menu);
    ~pMenu();
};

struct pMenuItem : pMenuBase {
    MenuItem& menuItem;

    auto init() -> void;
    static auto activate(MenuItem* self) -> void;
    pMenuItem(MenuItem& menuItem);
};

struct pMenuCheckItem : pMenuBase {
    MenuCheckItem& menuCheckItem;

    auto setChecked(bool checked) -> void;
    auto onToggle() -> void;
    auto init() -> void;
    static auto toggle(GtkCheckMenuItem* gtkCheckMenuItem, MenuCheckItem* self) -> void;
    pMenuCheckItem(MenuCheckItem& menuCheckItem);
};

struct pMenuRadioItem : pMenuBase {
    MenuRadioItem& menuRadioItem;

    auto setGroup(const std::vector<MenuRadioItem*>& group) -> void;
    auto setChecked() -> void;
    auto init() -> void;
    auto parent() -> pMenuRadioItem&;
    static auto activate(GtkCheckMenuItem* gtkCheckMenuItem, MenuRadioItem* self) -> void;
    pMenuRadioItem(MenuRadioItem& menuRadioItem);
};

struct pMenuSeparator : pMenuBase {
    MenuSeparator& menuSeparator;

    auto init() -> void;
    pMenuSeparator(MenuSeparator& menuSeparator);
};

struct pBrowserWindow {
	BrowserWindow& browserWindow;
	GtkWidget* dialog = nullptr;
	ListView* listView = nullptr;
	std::string selectedPath = "";
    std::vector<std::string> sortedFiles;
    GtkWidget* orderSelectedWidget = nullptr;
    bool multi = false;
	
    auto directory() -> std::string;
    auto fileGeneric(bool save, bool multi = false) -> std::vector<std::string>;
    auto file(bool save) -> std::string { return fileGeneric(save)[0]; }
    auto fileMulti() -> std::vector<std::string> { return fileGeneric(false, true); }
	auto close() -> void;
	auto detached() -> bool;
	auto visible() -> bool;
	auto setForeground() -> void;
	
	auto createPreview() -> GtkWidget*;
	auto contentViewSelection() -> unsigned;
    auto setListings( std::vector<BrowserWindow::Listing>& listings ) -> void;
	
	static auto responseHandler(GtkDialog* dialog, gint responseId, gpointer data) -> void;
    static auto closeHandler(GtkDialog* dialog, GdkEvent* event, gpointer data) -> void;
	static auto selectionHandler(GtkFileChooser* chooser, gpointer data) -> void;
    static auto onToggleOrder(GtkToggleButton* toggleButton, BrowserWindow* self) -> void;
	
	pBrowserWindow(BrowserWindow& browserWindow);
	~pBrowserWindow();
};

struct pMessageWindow {
    static auto error(MessageWindow::State& state) -> MessageWindow::Response;
    static auto information(MessageWindow::State& state) -> MessageWindow::Response;
    static auto question(MessageWindow::State& state) -> MessageWindow::Response;
    static auto warning(MessageWindow::State& state) -> MessageWindow::Response;
    static auto translateResponse(gint response) -> MessageWindow::Response;
    static auto message(MessageWindow::State& state, GtkMessageType messageStyle) -> gint;
};

struct pFont {
    static auto setFont(GtkWidget* widget, const std::string& font) -> PangoFontDescription*;
    static auto setFont(GtkWidget* widget, gpointer font) -> void;
    static auto system(unsigned size, std::string style, bool monospaced = false) -> std::string;
	static auto systemFontFile() -> std::string;
    static auto create(const std::string& desc) -> PangoFontDescription*;
    static auto add( CustomFont* customFont ) -> bool;
    static auto free(PangoFontDescription* font) -> void;
    static auto size(PangoFontDescription* font, std::string text) -> Size;
    static auto size(std::string font, std::string text) -> Size;
	static auto convertCss(GtkWidget* widget, PangoFontDescription* font) -> std::string;
	static auto scale( unsigned pixel ) -> unsigned;
};

struct pSystem {
    static auto getUserDataFolder() -> std::string;
    static auto getResourceFolder(std::string appIdent) -> std::string;
    static auto getWorkingDirectory() -> std::string;
    static auto getIconFolder() -> std::string;
    static auto getDesktopSize() -> Size;
    static auto sleep(unsigned milliSeconds) -> void;
    static auto isOffscreen( Geometry geometry ) -> bool { return false; } 
    static auto getOSLang() -> System::Language;
    static auto printToCmd( std::string str ) -> void;
	static auto applyCss( GtkWidget* gtkWidget, std::string css ) -> void;
	static auto addCssClass(GtkWidget* widget, std::string cssClass) -> void;
	static auto removeCssClass(GtkWidget* widget, std::string cssClass) -> void;
	static auto getColorCss( unsigned color, bool useComplementaryColor = false ) -> std::string;
};

struct pMonitor {

    struct Device {
        unsigned id;
        std::string ident;
        unsigned pos;
        XRROutputInfo* outInfo;
        RRMode originalMode;
        RRMode activeMode;
        RROutput xid;
        int x;
        int y;
    };

    struct Setting {
        unsigned id;
        std::string ident;
        Device* parentDevice;
        RRMode rrMode;
    	float rate;
    };

    static Display* display;
    static XRRScreenResources* screens;
    static std::vector<Device> devices;
    static std::vector<Setting> settings;
    static Device* activeDevice;

    static auto connect() -> bool;
    static auto disconnect() -> void;
    static auto fetchDisplays() -> void;
    static auto getDisplays() -> std::vector<Monitor::Property>;

    static auto fetchSettings( Device* device ) -> void;
    static auto getSettings( unsigned displayId ) -> std::vector<Monitor::Property>;

    static auto setSetting( unsigned displayId, unsigned settingId ) -> bool;
    static auto resetSetting() -> bool;

    static auto getCurrentRefreshRate() -> float;
    static auto getCurrentResolution() -> Size;
    static auto getRefreshRate( unsigned displayId, unsigned settingId ) -> float { return 0.0; }
};

struct pThreadPriority {
    static auto setPriority(ThreadPriority::Mode mode, float minProcessingTimeInMilliSeconds = 0, float maxProcessingTimeInMilliSeconds = 0) -> bool;
};

struct pInterProcess {
    static int fd;
    static unsigned char* memptr;
    static sem_t* semptr;
    static Timer* comTimer;

    static auto closeOtherInstances() -> void;
    static auto Acquire() -> bool;
    static auto Release() -> void;
    static auto checkQuit() -> void;
};

static auto getDropPaths(GtkSelectionData* data) -> std::vector<std::string>;
static auto CreateColor(uint8_t r, uint8_t g, uint8_t b) -> GdkColor;
static auto CreatePixbuf(Image& image, unsigned size = 0) -> GdkPixbuf*;
static auto CreateImage(Image& image, unsigned size = 0) -> GtkImage*;
static auto setImage(GtkImage* gtkImage, Image& image, unsigned size = 0) -> void;
static auto CreateCursor( GtkWidget* widget, GdkPixbuf* pixbuf, unsigned hotSpotX, unsigned hotSpotY ) -> GdkCursor*;
static auto SetCursor( GtkWidget* widget, GdkCursor* cursor ) -> void;
}
