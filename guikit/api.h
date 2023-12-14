
/**
 * v 1.9.5
 */

#ifndef GUIKIT_H
#define GUIKIT_H

#include <string>
#include <vector>
#include <functional>
#include <algorithm>
#include <unordered_set>
#include <thread>
#include <iterator>

namespace GUIKIT {

struct Window;
struct StatusBar;
struct Menu;
struct Widget;
struct Layout;
struct TreeView;
struct Timer;
struct pWindow;
struct pStatusBar;
struct pTimer;
struct pMenuBase;
struct pMenu;
struct pMenuItem;
struct pMenuCheckItem;
struct pMenuRadioItem;
struct pMenuSeparator;
struct pWidget;
struct pLineEdit;
struct pMultilineEdit;
struct pLabel;
struct pHyperlink;
struct pSquareCanvas;
struct pButton;
struct pStepButton;
struct pCheckButton;
struct pCheckBox;
struct pComboButton;
struct pSlider;
struct pRadioBox;
struct pProgressBar;
struct pListView;
struct pTreeViewItem;
struct pTreeView;
struct pViewport;
struct pFrame;
struct pTabFrame;
struct pBrowserWindow;
struct pImageView;
struct Zip;
struct Gzip;
struct Tar;

struct Base {
    unsigned id;
    static std::vector<Base*> objects;
    static auto find(unsigned id) -> Base*;
protected:
    Base();
    virtual ~Base() { objects.at(id-100) = nullptr; }
};

struct Image {
    unsigned width;
    unsigned height;
    uint8_t* data = nullptr;
    bool alphaBlendApplied = false;
    int resourceId = -1; // win xp only 
    enum Format : unsigned { RGBA, BGRA } format;

    auto loadPng(const uint8_t* src, unsigned size) -> bool;
    auto generatePng( uint8_t* rgbData, unsigned width, unsigned height, unsigned& pngSize ) -> uint8_t*;
    auto alphaBlend(unsigned alphaColor) -> void;
	auto alphaMultiply() -> void;
    auto scaleNearest(unsigned outputWidth, unsigned outputHeight) -> void;
    auto switchBetweenBGRandRGB() -> void;
    auto empty() -> bool;
    auto free() -> void;
    auto create(unsigned _width, unsigned _height, uint8_t* src = nullptr) -> void;
    auto setResourceId( int rId ) -> void;
    //helper to convert binary file to comma separated hardcoded unsigned char sequence
    static auto getCharDataStringFromBinary(std::string inFile, std::string outFile) -> bool;    

    Image() : width(0), height(0), data(nullptr), format(RGBA) {}
    Image(unsigned width, unsigned height, uint8_t* src, Format format = RGBA);
    Image(const Image& source);
    Image(Image&& source);
    ~Image();

    Image& operator=(const Image& source);
    Image& operator=(Image&& source);
protected:
    auto read(uint8_t* p) -> unsigned;
    auto write(uint8_t* p, unsigned value) -> void;
    auto normalize(uint64_t color, unsigned sourceDepth, unsigned targetDepth) -> uint64_t;
};

struct Size {
    static const unsigned Maximum = ~0u;
    static const unsigned Minimum =  0u;

    unsigned width, height;
    Size() : width(0), height(0) {}
    Size(unsigned width, unsigned height) : width(width), height(height) {}
};

struct Position {
    signed x, y;
    Position() : x(0), y(0) {}
    Position(signed x, signed y) : x(x), y(y) {}
};

struct Geometry {
    signed x, y;
    unsigned width, height;
    Position position() const { return {x, y}; }
    auto size() const -> Size { return {width, height}; }
    Geometry() : x(0), y(0), width(0), height(0) {}
    Geometry(Position position, Size size) : x(position.x), y(position.y), width(size.width), height(size.height) {}
    Geometry(signed x, signed y, unsigned width, unsigned height) : x(x), y(y), width(width), height(height) {}
};

struct Mouse {
    enum class Button : unsigned { Left, Middle, Right };
};

struct CustomFont {
	std::string name;
    void* refPtr = nullptr;
    unsigned modifier = 0;
	uint8_t* data = nullptr;
	unsigned size;
    std::string filePath;
    int sizeAdjust = 0;
};

struct Application {    
    static std::function<void ()> loop;
	static std::function<void (std::string text)> onClipboardRequest;
    static std::function<void ()> onDisplayChange;
    static std::function<void ()> onQuitRequest;

    static auto initialize() -> void;
    static auto run() -> void;
    static auto processEvents() -> void;
    static auto quit() -> void;
    static auto isCocoa() -> bool;
    static auto isGtk() -> bool;
    static auto isWinApi() -> bool;
	static auto requestClipboardText() -> void;
    static auto setClipboardText( std::string text ) -> void;
#ifdef _WIN32
    static auto getUtf8CmdLine(std::vector<std::string>& out) -> bool;
#endif
    static auto closeOtherInstances() -> void;

    static bool isQuit;
    static int exitCode;
    static std::string name;
    static std::string vendor;

    struct Cocoa {
        static std::function<void ()> onAbout;
        static std::function<void ()> onPreferences;
		static std::function<void ()> onCustom1;
        static std::function<void ()> onQuit;
        static std::function<void ()> onDock;
        static std::function<void (std::string fileName)> onOpenFile;
    };

    Application() = delete;
};

struct Window : Base {
    enum class SIZE_MODE { Default, Minimized, Maximized };

    std::function<void ()> onClose = nullptr;
    std::function<void ()> onMove = nullptr;
    std::function<void (SIZE_MODE sizeMode )> onSize = nullptr;
    std::function<Size ()> onResizeStart = nullptr;
    std::function<void ()> onResizeEnd = nullptr;
    std::function<void (std::vector<std::string>)> onDrop = nullptr;
	std::function<bool ()> onContext = nullptr;
    std::function<void ()> onMinimize = nullptr;
    std::function<void ()> onUnminimize = nullptr; 
	std::function<void ()> onFocus = nullptr;
    std::function<void ()> onRealize = nullptr;
    std::function<void ()> onInactive = nullptr;
    std::function<void ()> onActive = nullptr;

    enum class Hints { Default, Video } hints = Hints::Default;
    
    enum class Cursor { Default, Pointer, Image } cursor = Cursor::Default;

    struct Cocoa {
        Window& window;
        enum AppMenuItem : unsigned { About = 0, Preferences = 2, Custom1 = 3, Hide = 5, HideOthers = 6, ShowAll = 7, Quit = 9 };
        auto setTitleForAppMenuItem(AppMenuItem appMenuItem, std::string title) -> void;
        auto setHiddenForAppMenuItem(AppMenuItem appMenuItem, bool state) -> void;
        auto keepMenuVisibilityOnDisplay(bool state = true) -> void;
        auto setDisableIconsInTopMenu(bool state) -> void;
        Cocoa(Window& window) : window(window) {}
    } cocoa;

    struct Winapi {
		std::function<void ()> onMenu = nullptr;
        Window& window;
        Winapi(Window& window) : window(window) {}
    } winapi;

    auto append(Menu& menu) -> void;
    auto append(Layout& layout) -> void;
    auto append(Widget& widget) -> void;
    auto hasAppended(Widget& widget) -> bool;
    auto append(StatusBar& statusBar) -> void;
    auto remove(Menu& menu) -> void;
    auto remove(Layout& layout) -> void;
    auto remove(Widget& widget) -> void;
    auto remove(StatusBar& statusBar) -> void;
    auto isApended(Menu& menu) -> bool;
    auto setBackgroundColor(uint8_t r, uint8_t g, uint8_t b) -> void {
        setBackgroundColor(r << 16 | g << 8 | b);
    }
    auto setBackgroundColor(unsigned color) -> void;
    auto setVisible(bool visible = true) -> bool;
    auto restore() -> void; // from minimized
    auto setFocused() -> void;
	auto setFocused(unsigned delay) -> void;
    auto setTitle(const std::string& title) -> void;
    auto setStatusVisible(bool visible = true) -> void;
    auto setMenuVisible(bool visible = true) -> void;
    auto setFullScreen(bool fullScreen = true) -> void;
    auto setResizable(bool resizable = true) -> void;
    auto setGeometry(Geometry geometry) -> void;
    auto isOffscreen() -> bool;
    auto setWidgetFont(const std::string& font) -> void;
    auto setDroppable(bool droppable = true) -> void;
    auto synchronizeLayout() -> void;
    auto handle() -> uintptr_t;
    auto setForeground() -> void;
    auto setFullscreenSetting( bool inUse, unsigned displayId = 0, unsigned settingId = 0 ) -> void;
    auto getCustomFullscreenRefreshRate() -> float;

    auto focused() -> bool;
    auto visible() const -> bool { return state.visible; }
    auto fullScreen() const -> bool { return state.fullScreen; }
    auto resizable() const -> bool { return state.resizable; }
    auto menuVisible() const -> bool { return state.menuVisible; }
    auto statusVisible() const -> bool { return state.statusVisible; }
    auto aspectRatio() const -> Size { return state.aspectRatio; }
    auto droppable() const -> bool { return state.droppable; }
    auto minimized() -> bool;
    auto title() const -> std::string { return state.title; }
    auto statusBar() -> StatusBar* { return state.statusBar; }
    auto widgetFont() const -> std::string { return state.widgetFont; }
    auto geometry() -> Geometry;
    auto changeCursor( Image& image, unsigned hotSpotX, unsigned hotSpotY ) -> void;
    auto setDefaultCursor( ) -> void;
    auto setPointerCursor( ) -> void;
    auto getScrollbarWidth() -> unsigned;
	auto tellMeShouldICreateTheUIRightAway() -> bool;
    auto setAspectRatio(Size ratio) -> void;
    auto applyMaximizeCorrection(Geometry& geo) -> void;
	
	static auto addCustomFont( CustomFont* customFont ) -> bool;
	static auto getCustomFont(void* refPtr) -> CustomFont*;

    struct {
        bool resizable = true;
        bool fullScreen = false;
        bool menuVisible = false;
        bool statusVisible = false;
        bool visible = false;
        bool droppable = false;
        std::string title;
        Geometry geometry = {100, 100, 400, 300};
        std::string widgetFont;
        std::vector<Menu*> menus;
        Layout* layout = nullptr;
        Image* cursorImage = nullptr;
        StatusBar* statusBar = nullptr;
        Size aspectRatio = {0,0};
    } state;

    struct {
        bool inUse = false;
        unsigned displayId;
        unsigned settingId;
    } fullscreenSetting;
	
	static std::vector<CustomFont*> customFonts;
    Timer* focusTimer = nullptr;
    
    pWindow& p;
    Window(Hints hints = Hints::Default);
    ~Window();
};

struct StatusBar : Base {    
    struct Part {
        unsigned id;
        unsigned width;
        std::string text = "";
		std::string tooltip = "";
        Image* image = nullptr;        
        std::function<void ()> onClick = nullptr;
        Menu* popupMenu = nullptr;
        int overrideForegroundColor = -1;
        bool alignRight = false;
        bool visible = false;
        int position = -1;
        bool appendSeparator = false;
    };
    
    auto window() const -> Window* { return state.window; }
    auto font() const -> std::string { return state.font; }
    auto text() const -> std::string { return state.text; }
    auto updatePending() const -> bool { return state.updatePending; }
	auto append(unsigned id, std::string text, std::function<void ()> onClick = nullptr, Menu* popupMenu = nullptr, int pos = -1) -> void;
	auto append(unsigned id, Image* image, std::function<void ()> onClick = nullptr, Menu* popupMenu = nullptr, int pos = -1) -> void;	  
    auto removePart( unsigned id ) -> void;
    
    auto updateText( unsigned id, std::string text, bool alignRight = false, int overrideForegroundColor = -1 ) -> bool;
    auto updateImage( unsigned id, Image* image ) -> bool;
    auto updateDimension( unsigned id, std::string text ) -> void;
    auto updateVisible( unsigned id, bool visible ) -> bool;
	auto updateTooltip( unsigned id, std::string tooltip ) -> bool;
	auto updateSeparator( unsigned id, bool append ) -> bool;
    
    auto setFont(std::string font) -> void;
    auto setText(std::string text) -> void; // simple single part usage
    auto clear() -> void;
    auto hideContent() -> void;
    auto update(bool force = false) -> void;
        
    struct {
        std::string font;
        std::string text;
        Window* window = nullptr;
        std::vector<Part> parts;
        bool updatePending = false;
    } state;
    
    pStatusBar& p;
    StatusBar();
    ~StatusBar();
};

struct Sizable : Base {
    auto enabled() const -> bool { return state.enabled; }
    auto visible() const -> bool { return state.visible; }
    auto parent() const -> Sizable* { return state.parent; }
    auto window() const -> Window* { return state.window; }

    virtual auto minimumSize() -> Size = 0;
    virtual auto setGeometry(Geometry geometry) -> void = 0;
    virtual auto setEnabled(bool enabled = true) -> void = 0;
    virtual auto setVisible(bool visible = true) -> void = 0;

    struct {
        bool enabled = true;
        bool visible = true;
        Sizable* parent = nullptr;
        Window* window = nullptr;
    } state;

protected:
    Sizable() {}
};

struct Widget : Sizable {
    std::function<void ()> onSize = nullptr;

    auto font() -> std::string;
    auto specialFont() -> bool { return state.specialFont; }
    auto geometry() const -> Geometry { return state.geometry; }
    auto text() -> std::string { return state.text; }
    auto tooltip() -> std::string { return state.tooltip; }
    auto isContainer() -> bool { return state.isContainer; }
    auto foregroundColor() -> unsigned { return state.foregroundColor; }
    auto backgroundColor() -> unsigned { return state.backgroundColor; }
    auto overrideForegroundColor() -> bool { return state.overrideForegroundColor; } 
    auto overrideBackgroundColor() -> bool { return state.overrideBackgroundColor; }

    auto focused() -> bool;
    auto setFocused() -> void;
    auto setEnabled(bool enabled = true) -> void;
    auto setVisible(bool visible = true) -> void;
    // "special font" hint is only handled in listviews at the moment.
    // in this case vertical spacing is completly removed.
    auto setFont(const std::string& font, bool specialFont = false) -> void;
    auto minimumSize() -> Size;
    auto setGeometry(Geometry geometry) -> void; //use this to control geometry manually for widgets without layout, layouts set widget geometry automatically
    auto setText(const std::string& text) -> void;
    auto setTooltip(const std::string& tooltip) -> void;
	// next two functions are not supported for all widgets	    
    // supported for listview, treeview
    auto setBackgroundColor(unsigned color) -> void;
    // supported for label, listview, treeview
    // gtk line edit
    auto setForegroundColor(unsigned color) -> void;
    auto resetForegroundColor() -> void;

    struct {
        Geometry geometry = {0, 0, 0, 0};
        std::string font;
        bool specialFont = false;
        std::string text;
        std::string tooltip;
        unsigned foregroundColor = 0;
        bool overrideForegroundColor = false; 
        unsigned backgroundColor = 0;
        bool overrideBackgroundColor = false; 
        bool isContainer = false;
    } state;

    pWidget& p;

    ~Widget();
    Widget();
protected:
    Widget(pWidget& p);
};

struct LineEdit : Widget {
    std::function<void ()> onChange = nullptr;
    std::function<void ()> onFocus = nullptr;
    std::function<void (std::vector<std::string>)> onDrop = nullptr;

    auto editable() const -> bool { return state.editable; }
    auto droppable() const -> bool { return state.droppable; }
    auto maxLength() const -> unsigned { return state.maxLength; }
    auto text() -> std::string;
	auto value() -> int; // integer formated text, returns 0 if not possible
	auto setValue(int value) -> void;
    auto setEditable(bool editable = true) -> void;
    auto setDroppable(bool droppable = true) -> void;
    auto setMaxLength( unsigned maxLength ) -> void;
    
    struct {
        bool editable = true;
        bool droppable = false;
        unsigned maxLength = 0; // no limit
    } state;

    pLineEdit& p;
    LineEdit();
};

struct MultilineEdit : Widget {
    std::function<void ()> onChange = nullptr;
    std::function<void ()> onFocus = nullptr;

    auto editable() const -> bool { return state.editable; }
    auto text() -> std::string;
    auto setEditable(bool editable = true) -> void;
    
    struct {
        bool editable = true;
    } state;

    pMultilineEdit& p;
    MultilineEdit();
};

struct Label : Widget {
    enum class Align { Left, Right } ;
    
    struct {
        Align align = Align::Left;
    } state;
    
    auto align() -> Align { return state.align; }
    auto setAlign( Align align ) -> void;
    
    pLabel& p;
    Label();
};

struct Hyperlink : Widget {
    pHyperlink& p;
    
    auto setUri( std::string uri, std::string wrap = "" ) -> void;
    auto uri() -> std::string { return state.uri; };
    auto wrap() -> std::string { return state.wrap; };
    
    struct {
        std::string uri = "";
        std::string wrap = "";
    } state;
    
    Hyperlink();
};

struct SquareCanvas : Widget {
    std::function<void (Mouse::Button)> onMousePress = [](Mouse::Button button){};
    std::function<void (Mouse::Button)> onMouseRelease = [](Mouse::Button button){};

    pSquareCanvas& p;    
    auto setBorderColor(unsigned borderSize, unsigned borderColor) -> void;
    auto borderSize() -> unsigned { return state.borderSize; }
    auto borderColor() -> unsigned { return state.borderColor; }
    
    struct {
        unsigned borderColor = 0;
        unsigned borderSize = 0;
    } state;
    
    SquareCanvas();
};

struct ImageView : Widget {
    std::function<void ()> onClick = nullptr;

    auto setImage(Image* image) -> void;
    auto setUri( std::string uri ) -> void;

    auto uri() -> std::string { return state.uri; };

    struct {
        Image* image = nullptr;
        std::string uri = "";
    } state;

    pImageView& p;
    ImageView();
};

struct Button : Widget {
    std::function<void ()> onActivate = nullptr;
    pButton& p;
    Button();
};

struct StepButton : Widget {
    std::function<void ()> onChange = nullptr;
    
    auto setRange(int16_t minValue, int16_t maxValue) -> void;
    auto setValue(int16_t value) -> void;
    auto getValue() -> int16_t { return state.value; }
    
    struct {
        int16_t minValue = 0;
        int16_t maxValue = 0x7fff;
        int16_t value = 0;
    } state;
    
    pStepButton& p;
    StepButton();
};

struct CheckButton : Widget {
    std::function<void ()> onToggle = nullptr;

    auto setChecked(bool checked = true) -> void;
    auto checked() const -> bool { return state.checked; }
    auto toggle() -> void;

    struct { bool checked = false; } state;

    pCheckButton& p;
    CheckButton();
};

struct CheckBox : Widget {
    std::function<void (bool checked)> onToggle = nullptr;

    auto setChecked(bool checked = true) -> void;
    auto checked() const -> bool { return state.checked; }
    auto toggle() -> void;

    struct { bool checked = false; } state;

    pCheckBox& p;
    CheckBox();
};

struct ComboButton : Widget {
    std::function<void ()> onChange = nullptr;

    auto rows() const -> unsigned { return state.rows.size(); }
    auto selection() const -> unsigned { return state.selection; }
    auto userData() const -> int { return userData( state.selection ); }
    auto userData(unsigned selection) const -> int;
    auto text() const -> std::string { return text( state.selection ); }
    auto text(unsigned selection) const -> std::string;

    auto append(const std::string& text = "", int userData = 0) -> void;
    auto remove(unsigned selection) -> void;
    auto reset() -> void;
    auto setSelection(unsigned selection) -> void;
    auto activate(unsigned selection) -> void;
    auto setSelectionByUserId(int userId) -> void;
    auto setSelectionByRow(std::string row) -> void;
    auto setText(unsigned selection, const std::string& text) -> void;
    auto setUserData(unsigned selection, int userData) -> void;
    auto setText(const std::string& text) -> void = delete;

    struct {
        unsigned selection = 0;
        std::vector<std::string> rows;
        std::vector<int> userData;
    } state;

    bool hintVerticalScrollbar = false;
    pComboButton& p;
    ComboButton(bool hintVerticalScrollbar = false);
};

struct Slider : Widget {
    std::function<void (unsigned position)> onChange = nullptr;

    auto length() const -> unsigned { return state.length; }
    auto position() const -> unsigned { return state.position; }
    auto setLength(unsigned length) -> void;
    auto setPosition(unsigned position) -> void;

    auto text() -> std::string = delete;
    auto setText(const std::string& text) -> void = delete;

    enum Orientation : unsigned { HORIZONTAL, VERTICAL } orientation;

    struct {
        unsigned length = 101;
        unsigned position = 0;
    } state;

    pSlider& p;
    Slider(Orientation orientation);
};

struct HorizontalSlider : Slider {
    HorizontalSlider() : Slider(Orientation::HORIZONTAL) {}
};

struct VerticalSlider : Slider {
    VerticalSlider() : Slider(Orientation::VERTICAL) {}
};

struct RadioBox : Widget {
    std::function<void ()> onActivate = nullptr;

    template<typename... Args> static void setGroup(Args&&... args) { setGroup({&std::forward<Args>(args)...}); }
    static auto setGroup(std::vector<RadioBox*> group) -> void;
    auto setChecked() -> void;
    auto activate() -> void;
    auto checked() const -> bool { return state.checked; }
	auto getGroup() -> std::vector<RadioBox*> { return state.group; }

    struct {
        bool checked = false;
        std::vector<RadioBox*> group;
    } state;

    pRadioBox& p;
    RadioBox();
};

struct ProgressBar : Widget {
    auto setPosition(unsigned position) -> void;
    auto position() -> unsigned { return state.position; }
    auto setText(const std::string& text) -> void = delete;
    auto text() -> std::string = delete;

    struct { unsigned position = 0; } state;

    pProgressBar& p;
    ProgressBar();
};

struct ListView : Widget {
    std::function<void ()> onActivate = nullptr;
    std::function<void ()> onChange = nullptr;

    auto headerVisible() const -> bool { return state.headerVisible; }
    auto rowCount() const -> unsigned { return state.rows.size(); }
    auto columnCount() const -> unsigned { return state.header.size(); }
    auto selection() const -> unsigned { return state.selection; }
    auto column() const -> unsigned { return state.column; }
    auto selected() const -> bool { return state.selected; }
    auto lockRedraw() -> void;
    auto unlockRedraw() -> void;

    auto append(const std::vector<std::string>& row) -> void;
    auto append(const std::vector<std::vector<std::string>>& rows, bool clearBefore = true) -> void;
    auto remove(unsigned selection) -> void;
    auto reset() -> void;
    auto setSelection(unsigned selection) -> void;
    auto setSelected(bool selected = true) -> void;
    auto setHeaderVisible(bool visible = true) -> void;
    auto setHeaderText(const std::vector<std::string>& text) -> void;
    auto setText(unsigned selection, const std::vector<std::string>& text) -> void;
    auto setText(unsigned selection, unsigned position, const std::string& text) -> void;
    auto setImage(unsigned selection, unsigned position, Image& image) -> void;
    auto getImage( unsigned selection, unsigned position ) -> Image*;
    auto countImages() -> unsigned;
    auto text(unsigned selection, unsigned position) -> std::string;
    auto text() -> std::string = delete;
    auto setText(const std::string& text) -> void = delete;
	auto setRowTooltip(unsigned selection, std::string tooltip ) -> void;
	auto getRowTooltip(unsigned selection) -> std::string;
    auto colorRowTooltips(bool colorTip) -> void;
    auto setSelectionColor(unsigned foregroundColor, unsigned backgroundColor) -> void;
    auto resetSelectionColor() -> void;
    auto overrideSelectionColor() -> bool { return state.overrideSelectionColor; }
    auto selectionForegroundColor() -> unsigned { return state.selectionForegroundColor; }
    auto selectionBackgroundColor() -> unsigned { return state.selectionBackgroundColor; }

    struct {
        bool headerVisible = false;
        unsigned selection = 0;
        unsigned column = 0;
        bool selected = false;
        bool colorRowTooltips = false;
        bool overrideSelectionColor = false;
        unsigned selectionForegroundColor;
        unsigned selectionBackgroundColor;
        std::vector<std::string> header;
        std::vector<std::vector<std::string>> rows;
		std::vector<std::string> rowTooltips;
        std::vector<std::vector<Image*>> images;
    } state;

    pListView& p;
    ListView();
};

struct TreeViewItem {
    auto append(TreeViewItem& item) -> void;
    auto remove(TreeViewItem& item) -> void;
    auto reset() -> void;
    auto setText(const std::string& text) -> void;
    auto setExpanded(bool expanded = true) -> void;
    auto setSelected() -> void;
    auto setUserData(uintptr_t userData) -> void;
    auto setImage(Image& image) -> void;
    auto setImageSelected(Image& image) -> void;
	auto setImageExpanded(Image& image) -> void;
    auto text() -> std::string { return state.text; }
    auto expanded() -> bool { return state.expanded; }
    auto selected() -> bool;
    auto itemCount() -> unsigned { return state.items.size(); }
    auto userData() -> uintptr_t { return state.userData; }
    auto parentItem() -> TreeViewItem* { return state.parentTreeViewItem; }
    auto parentView() -> TreeView* { return state.parentTreeView; }

    struct {
        TreeView* parentTreeView = nullptr;
        TreeViewItem* parentTreeViewItem = nullptr;
        std::vector<TreeViewItem*> items;
        std::string text;
        bool expanded = false;
        uintptr_t userData;
        Image* image = nullptr;
        Image* imageSelected = nullptr;
		Image* imageExpanded = nullptr;
    } state;

    pTreeViewItem& p;
    TreeViewItem();
    ~TreeViewItem();
};

struct TreeView : Widget {
    std::function<void ()> onActivate = nullptr;
    std::function<void ()> onChange = nullptr;
	std::function<void (TreeViewItem*)> onCollapse = nullptr;
	std::function<void (TreeViewItem*)> onExpand = nullptr;

    auto append(TreeViewItem& item) -> void;
    auto remove(TreeViewItem& item) -> void;
    auto reset() -> void;
    auto selected() -> TreeViewItem* { return state.selected; }
    auto itemCount() -> unsigned { return state.items.size(); }
    auto items() -> std::vector<TreeViewItem*>& { return state.items; }

    struct {
        std::vector<TreeViewItem*> items;
        TreeViewItem* selected = nullptr;
    } state;

    pTreeView& p;
    TreeView();
};

struct Viewport : Widget {
    std::function<void (std::vector<std::string>)> onDrop = nullptr;
    std::function<void (std::vector<std::string>)> onDragEnter = nullptr;
    std::function<void ()> onDragLeave = nullptr;
    std::function<void (int x, int y)> onDragMove = nullptr;

    std::function<void (Position&)> onMouseMove = nullptr;
    std::function<void ()> onMouseLeave = nullptr;
    std::function<void (Mouse::Button)> onMousePress = nullptr;
    std::function<void (Mouse::Button)> onMouseRelease = nullptr;
    auto text() -> std::string = delete;
    auto setText(const std::string& text) -> void = delete;

    auto droppable() -> bool const { return state.droppable; }
    auto handle(bool hintRecreation) -> uintptr_t;
    auto setDroppable(bool droppable = true) -> void;
    auto getMousePosition() -> Position&;

    struct {
        bool droppable = false;
        Position mousePos = {0,0};
    } state;

    pViewport& p;
    Viewport();
};

struct Layout : Sizable {        
    struct Children {
        Sizable* sizable;
        Size size;
        Position position;
        unsigned spacing;
        unsigned selection;
        bool synchronized;
    };
    
    auto append(Sizable& sizable, Size size, unsigned spacing = 0) -> void;
    auto remove(Sizable& sizable) -> bool;

    auto updateLayout() -> void;
    auto synchronizeLayout() -> void;
    auto resetSynchronisation() -> void;
    auto has(Sizable& sizable) -> Children*;
    auto update(Sizable& sizable, Size size, unsigned spacing = 0) -> void;
    auto update(Sizable& sizable, unsigned spacing) -> void;
    auto setEnabled(bool enabled = true) -> void;
    virtual auto setVisible(bool visible = true) -> void;

    auto setAlignment(double alignment) -> void;
    auto setMargin(unsigned margin) -> void;
    auto reset() -> void;
    static auto getParentWidget( Widget* widget, int& selection) -> Widget*;
    auto getFrameInnerGeometry(Geometry geometry) -> Geometry;
    static auto getParentTabOrSwitchLayout(Sizable* sizable) -> Layout*;
    static auto getTopMostTabOrSwitchLayout(Layout* layout) -> Layout*;
    
    auto getAllChildWidgets() -> std::vector<Widget*>;

    std::vector<Children> children;

    struct {
        Geometry geometry = {0, 0, 0, 0};
        double alignment = 0.0;
        unsigned margin = 0;
        unsigned padding = 0;
        bool synchronized = false;
    } state;
    
    ~Layout();
protected:        
    auto append(Sizable& sizable) -> void;
    auto cut(Sizable& sizable) -> void;
    auto addDisplacement(Geometry& geometry, unsigned offset) -> void;    
    auto addFrameSize(Size min) -> Size;    
    Widget* frameWidget = nullptr; //helper widget for layouting, e.g. tabframe, fieldset
};

struct FixedLayout : Layout {
    auto append(Widget& widget, Geometry geometry) -> void;
    auto append(Sizable& sizable, Size size, unsigned spacing = 0) -> void = delete;
    auto setAlignment(double alignment) -> void = delete;
    auto setMargin(unsigned margin) -> void = delete;
    auto minimumSize() -> Size { return {0,0}; }
protected:
    auto setGeometry(Geometry geometry) -> void;
};

struct SwitchLayout : Layout {
    
    auto setLayout(unsigned selection, Layout& layout, Size size, bool autoUpdate = true) -> void;
    auto remove(unsigned selection) -> void;
    auto append(Sizable& sizable, Size size, unsigned spacing = 0) -> void = delete;
    auto setAlignment(double alignment) -> void = delete;
    auto minimumSize() -> Size;
    auto setVisible(bool visible = true) -> void;
    auto setSelection(unsigned selection) -> void;
    auto selection() const -> unsigned;
    
    struct {
        unsigned selection = 0;
    } state;
protected:
    auto setGeometry(Geometry geometry) -> void;  
};

struct TabFrameLayout : Layout {
    friend class pTabFrame;
    friend class pApplication;
    friend class pLabel;
    friend class pFrame;
	friend class pWidget;
    friend class Layout;

    std::function<void ()> onChange = nullptr;

    auto append(Sizable& sizable, Size size, unsigned spacing = 0) -> void = delete;
    auto setAlignment(double alignment) -> void = delete;

    auto setLayout(unsigned selection, Layout& layout, Size size = {~0u, ~0u}, bool autoUpdate = true) -> void;
    auto appendHeader(std::string text, Image& image) -> void { appendHeader(text, &image); }
    auto appendHeader(std::string text, Image* image = nullptr) -> void;
    auto setHeader(unsigned selection, std::string text) -> void;
    auto setFont(const std::string& font) -> void;
    auto setSelection(unsigned selection) -> void;
    auto selection() const -> unsigned;
    auto header(unsigned selection) const -> std::string;
    auto remove(unsigned selection) -> void;
    auto setImage(unsigned selection, Image& image) -> void;
    auto setPadding(unsigned padding) -> void { state.padding = padding; }
    auto setVisible(bool visible = true) -> void;

    auto minimumSize() -> Size;

    static auto getParentTabFrame(Sizable* sizable) -> TabFrameLayout*;
    static auto getTopMostParentTabFrame(TabFrameLayout* tab) -> TabFrameLayout*;
    TabFrameLayout();

protected:
    auto setGeometry(Geometry geometry) -> void;
    struct TabFrame : Widget {
        std::function<void ()> onChange = nullptr;
        auto selection() const -> unsigned { return state.selection; }
        auto tabs() const -> unsigned { return state.header.size(); }
        auto text(unsigned selection) -> std::string {
            if(selection >= tabs()) return "";
            return state.header.at(selection);
        }

        struct {
            unsigned selection = 0;
            std::vector<std::string> header;
            std::vector<Image*> images;
        } state;

        pTabFrame& p;
        TabFrame();
    };
    auto getTabFrame() const -> TabFrame* { return (TabFrame*)frameWidget; }
};

struct HorizontalLayout : Layout {
    auto minimumSize() -> Size;
    static auto alignChildrenVertically( std::vector<HorizontalLayout*> layouts ) -> void;
protected:
    auto setGeometry(Geometry geometry) -> void;
};

struct FramedHorizontalLayout : HorizontalLayout {    
    auto setFont(const std::string& font) -> void;
    auto setText(const std::string& text) -> void;
    auto setPadding(unsigned padding) -> void;
    auto setForegroundColor(unsigned color) -> void;
    FramedHorizontalLayout();

protected: 
	struct Frame : Widget {
		pFrame& p;
		Frame();
	};
};

struct VerticalLayout : Layout {
    auto minimumSize() -> Size;
protected:
    auto setGeometry(Geometry geometry) -> void;
};

struct FramedVerticalLayout : VerticalLayout {
    auto setFont(const std::string& font) -> void;
    auto setText(const std::string& text) -> void;
    auto setPadding(unsigned padding) -> void;
    auto setForegroundColor(unsigned color) -> void;
    FramedVerticalLayout();

protected:
	struct Frame : Widget {
		pFrame& p;
		Frame();
	};
};

struct MenuBase : Base {
    auto enabled() const -> bool { return state.enabled; }
    auto visible() const -> bool { return state.visible; }
    auto text() const -> std::string { return state.text; }
    auto icon() const -> Image* { return state.icon; }
    auto setEnabled(bool enabled = true) -> void;
    auto setVisible(bool visible = true) -> void;
    auto setText(const std::string& text) -> void;
    auto setIcon(Image& icon) -> void;
    auto parentMenu() -> Menu* { return state.parentMenu; }
    auto parentWindow() -> Window* { return state.parentWindow; }

    struct {
        bool enabled = true;
        bool visible = true;
        std::string text = "";
        Image* icon = new Image;
        Menu* parentMenu = nullptr;
        Window* parentWindow = nullptr;
        bool checked = false;
    } state;

    pMenuBase& p;
    MenuBase(pMenuBase& p);
    virtual ~MenuBase();
};

struct Menu : MenuBase {
    std::function<void ()> onOpen = nullptr;
    
    auto append(MenuBase& item) -> void;
    auto remove(MenuBase& item) -> void;
    auto reset() -> void;
    
    auto contextOnly() const -> bool { return state.contextOnly; }
    auto showContextOnly(bool contextOnly) -> void;

    struct {
        bool contextOnly = false;
    } state;

    std::vector<MenuBase*> childs;
    pMenu& p;
    Menu();
    ~Menu();
};

struct MenuItem : MenuBase {
    std::function<void ()> onActivate = nullptr;

    pMenuItem& p;
    MenuItem();
};

struct MenuCheckItem : MenuBase {
    std::function<void ()> onToggle = nullptr;
    auto setChecked(bool checked = true) -> void;
    auto checked() const -> bool { return state.checked; }
    auto toggle() -> void;

    pMenuCheckItem& p;
    MenuCheckItem();
};

struct MenuRadioItem : MenuBase {
    std::function<void ()> onActivate = nullptr;
    template<typename... Args> static auto setGroup(Args&&... args) -> void { 
        setGroup({&std::forward<Args>(args)...});
    }
    static auto setGroup(std::vector<MenuRadioItem*> group) -> void;
    auto setChecked() -> void;
    auto checked() const -> bool { return state.checked; }
    auto activate() -> void;

    std::vector<MenuRadioItem*> group;
    pMenuRadioItem& p;
    MenuRadioItem();
    ~MenuRadioItem();
};

struct MenuSeparator : MenuBase {
    auto text() -> std::string = delete;
    auto setText(const std::string& text) -> void = delete;
    auto setIcon(Image& icon) -> void = delete;

    static auto getInstance() -> MenuSeparator*;
    static auto cleanInstances() -> void;
    static std::vector<MenuSeparator*> instances;

    pMenuSeparator& p;
    MenuSeparator();
};

struct Timer : Base {
    std::function<void ()> onFinished = nullptr;

    struct State {
        bool enabled = false;
        unsigned interval = 0;
        unsigned data = 0;
    } state;

    auto enabled() const -> bool { return state.enabled; }
    auto interval() const -> unsigned { return state.interval; }
    auto data() const -> unsigned { return state.data; }
    auto setEnabled(bool enabled = true) -> void;
    auto setInterval(unsigned intervalInMs) -> void;
    auto setData(unsigned data) -> void;

    pTimer& p;
    Timer();
    ~Timer();
};

struct BrowserWindow {
	static std::function<void ()> onCall;

	struct Listing {
		std::string entry;
		std::string tooltip = "";
	};    
    
    auto directory() -> std::string;
    auto open() -> std::string;
    auto openMulti() -> std::vector<std::string>;
    auto save() -> std::string;
    auto detached() -> bool;
    auto visible() -> bool;
    auto close() -> void;
    auto setForeground() -> void;
    auto setFilters(std::vector<std::string> filters) -> BrowserWindow&;
    auto setWindow(Window& window) -> BrowserWindow&;
    auto setPath(const std::string& path) -> BrowserWindow&;
    auto setTitle(const std::string& title) -> BrowserWindow&;
    auto setOnChangeCallback( std::function<std::vector<BrowserWindow::Listing> (std::string file)> onSelectionChange ) -> BrowserWindow&;
    auto addCustomButton( std::string text, std::function<bool (std::vector<std::string> filePaths, unsigned selection)> onClick, unsigned id = 0, std::string toolTip = "" ) -> BrowserWindow&;
    auto setDefaultButtonText(std::string textOk, std::string textCancel = "") -> BrowserWindow&;
	auto setDefaultButtonTooltip(std::string toolTip) -> BrowserWindow&;
	auto setNonModal() -> BrowserWindow&;
	auto setListings( std::vector<BrowserWindow::Listing>& listings ) -> void;
    auto hideOkButton() -> void;
    auto showOrderControlForMultipleSelections( bool checked, std::string label, std::function<void (bool checked)> onToggle ) -> BrowserWindow&;

    auto setTemplateId(int id) -> BrowserWindow&;
    auto addContentView(unsigned id, std::function<bool (std::string filePath, unsigned selection)> onDblClick) -> BrowserWindow&;
    auto setContentViewFont(std::string font, bool specialFont = false) -> BrowserWindow&;
    auto setContentViewWidth(unsigned boxWidth) -> BrowserWindow&;
    auto setContentViewHeight(unsigned boxHeight) -> BrowserWindow&;
    auto setContentViewBackground(unsigned color) -> BrowserWindow&;
    auto setContentViewForeground(unsigned color) -> BrowserWindow&;
    auto setContentViewSelection(unsigned foregroundColor, unsigned backgroundColor) -> BrowserWindow&;
    auto setContentViewColorTooltips(bool colorTooltips) -> BrowserWindow&;
    auto setContentViewHint(std::string hint, std::string tooltip = "") -> BrowserWindow&;

    auto setCallbacks( std::function<void (std::vector<std::string> filePaths, unsigned selection)> onOkClick, std::function<void ()> onCancelClick ) -> BrowserWindow&;
    auto getContentViewSelection() -> unsigned;
    auto resizeTemplate(bool resize, int adjust = 0) -> BrowserWindow&;    
    
    static auto transformFilter( std::string description, const std::vector<std::string>& suffix ) -> std::string;
	static auto transformFilter( std::string description, const std::string& suffix ) -> std::string;   
    
    struct CustomButton {        
        std::string text;
		std::string toolTip = "";
        std::function<bool (std::vector<std::string> filePaths, unsigned selection)> onClick = nullptr;
        unsigned id = 0; // for template usage
    };

    struct CheckButton {
        bool checked = false;
        std::string text = "";
        std::string toolTip = "";
        std::function<void (bool checked)> onToggle = nullptr;
    };
    
    // for displaying file content
    struct ContentView {
        unsigned id = 0; // for template usage
        std::string font = "";
        bool specialFont = false;
        unsigned width = 450;
        unsigned height = 200;
        unsigned foregroundColor = 0;
        bool overrideForegroundColor = false;
        unsigned backgroundColor = 0;
        bool overrideBackgroundColor = false;

        bool overrideSelectionColor = false;
        unsigned selectionForegroundColor = 0;
        unsigned selectionBackgroundColor = 0;

        bool colorTooltips = false;
        std::function<bool (std::string filePath, unsigned selection)> onDblClick = nullptr;
        std::string hint = "";
        std::string hintTooltip = "";
    };        
	
    struct State {
        std::vector<std::string> filters;
        Window* window = nullptr;
        std::string path = "";
        std::string title = "";
        std::function<std::vector<Listing> (std::string filePath)> onSelectionChange = nullptr;
        std::function<void (std::vector<std::string> filePaths, unsigned selection)> onOkClick = nullptr;
        std::function<void ()> onCancelClick = nullptr;
        std::vector<CustomButton> buttons;
        ContentView contentView;
        int templateId = -1;
        bool resizeTemplate = false;
        int resizeAdjust = 0;
        std::string textOk = "";
        std::string textCancel = "";
		std::string toolTip = "";
		bool modal = true;
        bool hideOkButton = false;
        CheckButton* orderBySelected = nullptr;
    } state;

    pBrowserWindow& p;    
    BrowserWindow();
    ~BrowserWindow();
};

struct MessageWindow {
    enum class Buttons : unsigned {
        Ok, OkCancel, YesNo, YesNoCancel
    };
    enum class Response : unsigned {
        Ok, Cancel, Yes, No
    };

    struct State {
        Window* window = nullptr;
        Buttons buttons = Buttons::Ok;
        std::string text = "";
        std::string title = "";
    } state;

    struct Trans { //supported for cocoa and gtk, windows translates buttons themselves (OS language)
        std::string ok;
        std::string yes;
        std::string no;
        std::string cancel;
    };

    auto error(Buttons = Buttons::Ok) -> Response;
    auto warning(Buttons = Buttons::Ok) -> Response;
    auto information(Buttons = Buttons::Ok) -> Response;
    auto question(Buttons = Buttons::YesNo) -> Response;

    auto setText(const std::string& text) -> MessageWindow&;
    auto setTitle(const std::string& title) -> MessageWindow&;
    auto setWindow(Window& window) -> MessageWindow&;
    static auto translateOk(const std::string& str) -> void;
    static auto translateNo(const std::string& str) -> void;
    static auto translateYes(const std::string& str) -> void;
    static auto translateCancel(const std::string& str) -> void;
    static Trans trans;
};

struct Font {
    static auto system(unsigned size, const std::string& style = "", bool monospaced = false) -> std::string;
    static auto system(const std::string& style = "", bool monospaced = false) -> std::string;
    static auto size(const std::string& font, const std::string& text) -> Size;
	static auto scale( unsigned pixel ) -> unsigned;
    Font() = delete;
};

struct System {
    enum class Language { DE, UK, US, FR };
    static auto getUserDataFolder(std::string appIdent = "") -> std::string;
    static auto getResourceFolder(std::string appIdent) -> std::string;
    static auto getWorkingDirectory() -> std::string;
    static auto getDesktopSize() -> Size;
    static auto sleep(unsigned milliSeconds) -> void;
    static auto isOffscreen( Geometry geometry ) -> bool;
    static auto getOSLang() -> Language;
    static auto printToCmd( std::string str ) -> void;
    System() = delete;
};

struct Monitor {

    struct Property {
        unsigned id;
        std::string name;
    };

    static auto getDisplays() -> std::vector<Property>;
    static auto getSettings( unsigned displayId ) -> std::vector<Property>;
    static auto getCurrentRefreshRate() -> float;
    static auto getCurrentResolution() -> Size;

    Monitor() = delete;
};

// priority changes of std::threads isn't portable and needs to be handled carefully.
// furthermore it's not possible (Windows, macOS) to change priority of std::thread from other threads, so priority is changed only for caller thread
struct ThreadPriority {

    enum class Mode { Normal = 0, High = 1, Realtime = 2 };

    static auto setPriority( Mode mode, float typicalProcessingTimeInMilliSeconds = 0, float maxProcessingTimeInMilliSeconds = 0 ) -> bool;
};

struct File {
    struct Info {
        std::string name = "";
        std::string date = "";
        uint64_t size = 0;
        bool exists = true;
    };
    struct Item {
        unsigned id;
        Info info;
        bool isDirectory = false;
        Item* parent = nullptr;
        std::vector<Item*> childs;
    };
    enum class Mode { Read, Write, Update, Append };
    enum class Type { Default, Zip, TarGz, Gzip, Tar };

    //overall file access, compressed archives are not considered
    auto open(Mode mode = Mode::Read, bool createFolderIfNotExists = false) -> bool;
    auto read() -> uint8_t*;
	auto read(uint8_t* buffer, unsigned length, unsigned offset = 0) -> unsigned;
    auto write() -> bool;
    auto write(const uint8_t* buffer, unsigned length, unsigned offset = 0) -> unsigned;
    auto append(const uint8_t* buffer, unsigned length) -> unsigned;
    auto truncate() -> bool;
    auto getSize() const -> uint64_t { return fileInfo.size; }
    auto exists() const -> bool { return fileInfo.exists; }
    auto getDate() const -> std::string { return fileInfo.date; }
    auto getType() const -> Type { return type; }
    auto getFileName(bool removeExtension = false, bool truncateFromEnd = false) -> std::string;
    auto getPath() -> std::string;
    auto getExtension() -> std::string;
    auto getHandle() -> FILE* { return fp; }
    auto del() -> bool;
    
    auto setReadOnly() -> void { readOnly = true; }
    auto isReadOnly() -> bool { return readOnly; }
    // inform about data was changed by another task handling the same file
    auto forceDataChange() -> void { dataChanged = true; }
    auto wasDataChanged() -> bool { return dataChanged; }

    //archive access
    auto scanArchive() -> std::vector<Item>&;
    auto archiveData(unsigned id) -> uint8_t*;
    auto archiveDataSize(unsigned id) -> unsigned;
    
    //helper
    auto isSizeValid(unsigned maxSize) -> bool;
    auto isSizeValid(unsigned fileId, unsigned maxSize) -> bool;
    auto isArchived() -> bool;
    static auto SizeFormated(uint64_t bytes) -> std::string;

    //static
    static auto suppportedCompressionExtensions() -> std::vector<std::string> { 
        return {"zip", "gz", "tar", "tgz", "tar.gz"};
    }
    static auto suppportedCompressionFilter() -> std::string { return "zip, gz, tar, tgz, tar.gz (*.zip,*.gz,*.tar,*.tgz,*.tar.gz)"; }
    static auto getFolderList( std::string path, const std::string& subStr = "") -> std::vector<Info>;
    static auto getFolderListAlt( std::string path, const std::string& subStr = "", unsigned limit = 0 ) -> std::vector<std::string>;
    static auto isDir( std::string path ) -> bool;
    static auto createDir( std::string path, std::string basePath = "" ) -> bool;
    static auto beautifyPath(std::string path) -> std::string;
    static auto getOffsetDataStringFromBinary( std::string inFile, std::string outFile ) -> bool;

    static auto getPath( std::string _fn, bool returnSlashIfError = false ) -> std::string;
    static auto resolveRelativePath(std::string _fn, std::string relPath ) -> std::string;
    static auto isAbsolute(const std::string& path) -> bool;

    auto setFile(std::string filePath) -> void;
    auto getFile() const -> std::string { return filePath; }
    auto unload() -> void;
    auto reset() -> void;
    File(std::string filePath = "");
    ~File();
    File(const File& source);
    File& operator=(const File& source);
private:
    auto detectType() -> void;
    auto freeData(uint8_t** dataPtr) -> void;
    auto close() -> void;
    static auto setStats(std::string path, Info& info) -> void;
    static auto _createDir( std::string path ) -> bool;
    auto connectItems() -> void;

    std::string filePath = "";
    Type type;
    Mode mode;
    Info fileInfo;
    std::vector<Item> items;
    FILE* fp = nullptr;
    uint8_t* data = nullptr;
    bool dataChanged = false;
    bool readOnly = false;
    
    Zip* zip;
    Gzip* gzip;
    Tar* tar;
};

struct Setting {
    friend class Settings;

    unsigned uValue = ~0u; //for fast access without string conversion
    int iValue = 1 << ( (sizeof(int) << 3) - 1 ); //for fast access without string conversion
    std::string value = "";
    bool saveable = true;

    auto operator==(int data) -> bool;
    auto operator!=(int data) -> bool;
    auto operator=(int data) -> void;
    operator int();
    auto operator==(unsigned data) -> bool;
    auto operator!=(unsigned data) -> bool;
    auto operator=(unsigned data) -> void;
    operator unsigned();
	operator bool();
	
	std::vector<Setting*> childs;
	auto add(const std::string& ident) -> Setting*;
	auto set(std::string data) -> void;
	auto getIdent() -> std::string { return ident; }
    auto setIdent(std::string ident ) -> void { this->ident = ident; }
protected:
    std::string ident;
    Setting(const std::string& ident);
};

struct Settings {
    auto find(const std::string& ident) -> Setting*;
	auto findMulti(const std::string& ident) -> std::vector<Setting*>;
    auto add(const std::string& ident) -> Setting*;
    auto add(Setting* setting) -> void;
    auto remove(const std::string& ident) -> bool;
    auto clear() -> void;

    template<typename T> class type_info{};

    template<typename T>
    auto get(const std::string& ident, T defaultValue = T(), std::vector<T> range = {} ) -> T {
        auto result = get(type_info<T>(), ident, defaultValue);
		return !range.empty() ? std::min<T>( std::max<T>(result, (T)range[0]), (T)range[1] ) : result;
    }
	
	template<typename T> auto getOrInit(const std::string& ident, T defaultValue = T(), std::vector<T> range = {}) -> Setting* {
		auto setting = find( ident );
		if(setting) defaultValue = get(ident, defaultValue, range);
		set(type_info<T>(), ident, defaultValue, true);
		return find( ident );
	}
	
    template<typename T> auto set(const std::string& ident, T value, bool saveable = true) -> void {
        if (ident.empty()) return;
        set(type_info<T>(), ident, value, saveable);
    }

    auto setSaveable( const std::string& ident, bool state ) -> void;
    auto load(const std::string& path, unsigned maxFileSize = 1 * 1024 * 1024, bool themed = false) -> bool;
    auto save(const std::string& path) -> bool;
    auto getList() -> std::vector<Setting*>& { return list; }
    
    auto setGuid(void* guid) -> void { this->guid = guid; }
    auto getGuid() -> void* { return guid; }
    
    ~Settings();

private:
    auto set(type_info<bool> t, const std::string& ident, bool value, bool saveable) -> void;
    auto set(type_info<int> t, const std::string& ident, int value, bool saveable) -> void;
    auto set(type_info<unsigned> t, const std::string& ident, unsigned value, bool saveable) -> void;
    auto set(type_info<float> t, const std::string& ident, float value, bool saveable) -> void;
    auto set(type_info<double> t, const std::string& ident, double value, bool saveable) -> void;
    auto set(type_info<std::string> t, const std::string& ident, std::string value, bool saveable) -> void;

    auto get(type_info<bool> t, const std::string& ident, bool defaultValue) -> bool;
    auto get(type_info<int> t, const std::string& ident, int defaultValue) -> int;
    auto get(type_info<unsigned> t, const std::string& ident, unsigned defaultValue) -> unsigned;
    auto get(type_info<float> t, const std::string& ident, float defaultValue) -> float;
    auto get(type_info<double> t, const std::string& ident, double defaultValue) -> double;
    auto get(type_info<std::string> t, const std::string& ident, std::string defaultValue) -> std::string;

    auto loadEx(const std::string& path, int depth = 0, const char separator = '=') -> bool;
    auto stripCommentsAndDetectIncludes(std::string& line) -> bool;

    auto getReferences() -> std::vector<std::string>& { return references; }
    auto getPath() -> std::string& { return path; }

    std::vector<std::string> references;
    std::string path;
    std::vector<Setting*> list;
    void* guid = nullptr;
};

struct Translation {
    auto getA(std::string ident, bool addColon = false) -> std::string;
    auto get(std::string ident, const std::vector<std::vector<std::string>>& replaces = {}, bool addColon = false) -> std::string;
    auto read( std::string path, unsigned maxFileSize = 1 * 1024 * 1024 ) -> bool;
    auto clear() -> void;
    auto removeDigit(std::string& str) -> std::string;

    struct Data {
        std::string ident;
        std::string text;
    };
private:
    std::vector<Data> list;
};

//helpers
struct String {
    static auto toLowerCase( std::string& str ) -> std::string&;
    static auto toUpperCase( std::string& str ) -> std::string&;
    static auto trim(std::string& str) -> std::string&;
    static auto delSpaces(std::string& str) -> std::string&;
    static auto capitalize(std::string& str) -> std::string&;
    static auto split(const std::string& str, char delimiter, bool trimParts = true) -> std::vector<std::string>;
    static auto explode(std::string str, std::string delimiter) -> std::vector<std::string>;
    static auto unsplit( const std::vector<std::string>& parts, std::string delimiter ) -> std::string;
    static auto foundSubStr(std::string& str, std::string subStr) -> bool;
    static auto findString(const std::string& strHaystack, const std::string& strNeedle) -> bool; // ignore case
    static auto endsWith(std::string& str, std::string suffix) -> bool;
    static auto removeQuote(std::string& str) -> std::string&;
    static auto remove(std::string& str, const std::vector<std::string>& subStr) -> std::string&;
    static auto replace(std::string& str, const std::string& search, const std::string& replace) -> std::string&;
    static auto isNumber(const std::string& str) -> bool;
    static auto isFloatNumber(const std::string& str) -> bool;
	static auto convertToNumber(std::string str) -> int;
    static auto convertIntToHex( int number, bool prepend_0x = true ) -> std::string;
    static auto convertHexToInt( std::string hex, int defaultValueByFailure = 0 ) -> int;
    static auto formatFloatingPoint(double value, uint8_t roundDecimal = 0, bool cutTrailingZero = false) -> std::string;
    static auto prependZero( std::string str, unsigned width ) -> std::string;
    static auto prependLeft( std::string str, char placeHolder, unsigned width ) -> std::string;
    static auto removeDuplicates( std::vector<std::string>& strs ) -> void;
    static auto convertDoubleToString(double value, unsigned precision = 18) -> std::string;
    static auto findOccurencesOf( std::string str, std::string subStr ) -> unsigned;
    static auto getFileName(std::string path, bool removeExtension = false) -> std::string;
    static auto getExtension(const std::string& str, const std::string& defaultExt, int maxParts = 1, int maxPartSize = 3) -> std::string;
    static auto removeExtension(std::string str, int maxParts = 1, int maxPartSize = 3) -> std::string;

    template<typename T> static auto addThousandSeparator(T digit) -> std::string {
        return addThousandSeparator( std::to_string( digit ) );
    }
    static auto addThousandSeparator(std::string digit) -> std::string;
    String() = delete;
};

struct Vector {
	template<typename T>
    static auto eraseVectorElement(std::vector<T>& v, T element) -> bool {
		for (unsigned i = 0; i < v.size(); i++)
			if (v[i] == element) return v.erase(v.begin() + i), true;
		return false;
	}
    
	template<typename T>
    static auto eraseVectorPos(std::vector<T>& v, unsigned position, unsigned length=1) -> bool {
		if (v.size() < (position + length)) return false;
		for (unsigned l = 0; l < length; l++) v.erase(v.begin() + position);
		return true;
	}
    template<typename T>
    static auto find(std::vector<T>& v, T element) -> bool {        
        return std::find(v.begin(), v.end(), element) != v.end();
    }
    template<typename T>
    static auto findPos(std::vector<T>& v, T element) -> int {
		for (unsigned i = 0; i < v.size(); i++)
			if (v[i] == element) return i;
		return -1;
	}
    template<typename T>
    static auto combine(std::vector<T>& target, const std::vector<T>& source) -> void {
        target.insert( target.end(), source.begin(), source.end() );
    }
    template <typename T> 
    static auto concat(std::vector<T>& v1, std::vector<T>& v2) -> std::vector<T> {
        std::vector<T> concated = std::vector<T>();
        copy(v1.begin(), v1.end(), back_inserter( concated ));
        copy(v2.begin(), v2.end(), back_inserter( concated ));
        return concated;
    }
    template <typename T> 
    static auto insert(std::vector<T>& v, T value, unsigned pos) -> void {
        auto itPos = v.begin() + ( (pos >= v.size()) ? v.size() : pos);
        v.insert( itPos, value );
    }
    Vector() = delete;
};

struct Utf8 {
    // returns utf8 length of each scancode
    static auto encode( unsigned code, std::vector<uint8_t>& out ) -> unsigned;
    // returns scancode
    static auto decode( std::string text, unsigned& pos ) -> unsigned;
};

}

#endif
