
#define UNICODE
#define WINVER 0x0600
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN

#include <initguid.h>
#include <windows.h>
#include <shobjidl.h>
#include <uxtheme.h>
#include <shlobj.h>
#include <Shellapi.h>
#include <shlwapi.h>
#include <Commdlg.h>
#include <direct.h>
#include <dwmapi.h>
#include <windowsx.h>
#include <Vssym32.h>
#include "processref.h"
#include "doubleBuffer.h"

DEFINE_GUID(IID_IUnknown, 0x00000000, 0x0000, 0x0000, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);

enum IMMERSIVE_HC_CACHE_MODE { IHCM_USE_CACHED_VALUE, IHCM_REFRESH };
enum PreferredAppMode { Default, AllowDark, ForceDark, ForceLight, Max };

typedef void (WINAPI* SetWindowTheme_t)(HWND hwnd, LPCWSTR pszSubAppName, LPCWSTR pszSubIdList);
typedef int (WINAPI* IsAppThemed_t)();
typedef HPAINTBUFFER (WINAPI *FN_BeginBufferedPaint) (HDC hdcTarget, const RECT *prcTarget, BP_BUFFERFORMAT dwFormat, BP_PAINTPARAMS *pPaintParams, HDC *phdc);    
typedef HRESULT (WINAPI *FN_EndBufferedPaint) (HPAINTBUFFER hBufferedPaint, BOOL fUpdateTarget);
typedef HRESULT (WINAPI *DwmGetCompositionTimingInfo_t)(HWND hwnd, DWM_TIMING_INFO *pTimingInfo);
typedef HRESULT (WINAPI *DrawThemeParentBackground_t)(HWND hwnd, HDC hdc, const RECT *prc);
typedef HRESULT (WINAPI *DrawThemeBackground_t)(HTHEME hTheme, HDC hdc, int iPartId, int iStateId, LPCRECT pRect, LPCRECT pClipRect);
typedef HTHEME  (WINAPI *OpenThemeData_t)(HWND hwnd, LPCWSTR pszClassList);
typedef HRESULT (WINAPI *CloseThemeData_t)(HTHEME hTheme);

typedef void (WINAPI *RefreshImmersiveColorPolicyState_t)();
typedef bool (WINAPI *GetIsImmersiveColorUsingHighContrast_t)(IMMERSIVE_HC_CACHE_MODE mode);
typedef bool (WINAPI *ShouldAppsUseDarkMode_t)();
typedef bool (WINAPI *AllowDarkModeForWindow_t)(HWND hWnd, bool allow);
typedef bool (WINAPI *AllowDarkModeForApp_t)(bool allow);
typedef void (WINAPI *FlushMenuThemes_t)();

typedef bool (WINAPI *IsDarkModeAllowedForWindow_t)(HWND hWnd);
typedef bool (WINAPI *ShouldSystemUseDarkMode_t)();
typedef bool (WINAPI *SetPreferredAppMode_t)(PreferredAppMode appMode);
typedef bool (WINAPI *IsDarkModeAllowedForApp_t)();


namespace GUIKIT {

#define FixedStyle WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX | WS_MAXIMIZEBOX
#define ResizableStyle WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME
	
static const unsigned Windows2000  = 0x0500;
static const unsigned WindowsXP    = 0x0501;
static const unsigned WindowsVista = 0x0600;
static const unsigned Windows7     = 0x0601;
static const unsigned Windows8     = 0x0602;
static const unsigned Windows81    = 0x0603;
static const unsigned Windows10    = 0x0a00;
static const unsigned Windows11    = 0x0a01;

struct DropManager : public IDropTarget {
    DropManager(pWidget* refWidget);
    ~DropManager();

    pWidget* refWidget;
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppv);

    HRESULT STDMETHODCALLTYPE DragEnter(IDataObject* pdto, DWORD grfKeyState, POINTL ptl, DWORD* pdwEffect);
    HRESULT STDMETHODCALLTYPE DragOver(DWORD grfKeyState, POINTL ptl, DWORD* pdwEffect);
    HRESULT STDMETHODCALLTYPE DragLeave();
    HRESULT STDMETHODCALLTYPE Drop(IDataObject* pdto, DWORD grfKeyState, POINTL ptl, DWORD* pdwEffect);

    auto setPaths(IDataObject* pdto, std::vector<std::string>& paths) -> void;

private:
    LONG m_cRef;
};

struct pApplication {
    static auto run() -> void;
    static auto processEvents() -> void;
    static auto processMessage(MSG& msg) -> void;
    static auto quit() -> void;
    static auto initialize() -> void;
    static auto CALLBACK wndProc(WNDPROC windowProc, HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT;
    static auto currentWorkingDirectory() -> std::string;
	static auto requestClipboardText() -> void;
    static auto setClipboardText( std::string text ) -> void;
    static auto getUtf8CmdLine(std::vector<std::string>& out) -> bool;
    static auto initDarkTheme(bool force) -> void;
    static auto setDarkMode(Application::DarkMode darkMode) -> void;
	
    static std::string cwd; //current working directory

    static bool useDark;
    static HBRUSH darkEdgeBrush;
    static HBRUSH darkBGHotBrush;
    static HBRUSH darkBGBrush;       
    static HBRUSH darkBGSofterBrush;
    static HBRUSH darkBGTabBrush; 
    static HBRUSH darkDisabledEdgeBrush;
    static HPEN darkEdgePen;

    static auto loadThemedFunctions() -> void;
    static auto hasAppThemed() -> bool;
    static auto preferDarkTheme() -> bool;

    static ProcessReference g_pProcRef;

    static HMODULE uxTheme;
    static FN_BeginBufferedPaint pfnBeginBufferedPaint;
    static FN_EndBufferedPaint pfnEndBufferedPaint;
    static DrawThemeParentBackground_t pDrawThemeParentBackground;
    static DrawThemeBackground_t pDrawThemeBackground;
    static OpenThemeData_t pOpenThemeData;
    static CloseThemeData_t pCloseThemeData;
    static SetWindowTheme_t pSetWindowTheme;
    static IsAppThemed_t pIsAppThemed;
    static RefreshImmersiveColorPolicyState_t pRefreshImmersiveColorPolicyState;
    static GetIsImmersiveColorUsingHighContrast_t pGetIsImmersiveColorUsingHighContrast;
    static ShouldAppsUseDarkMode_t pShouldAppsUseDarkMode;
    static AllowDarkModeForWindow_t pAllowDarkModeForWindow;
    static AllowDarkModeForApp_t pAllowDarkModeForApp;
    static FlushMenuThemes_t pFlushMenuThemes;
    static IsDarkModeAllowedForWindow_t pIsDarkModeAllowedForWindow;
    static ShouldSystemUseDarkMode_t pShouldSystemUseDarkMode;
    static SetPreferredAppMode_t pSetPreferredAppMode;
};

struct pWindow {
    Window& window;
    HWND hwnd;
    HMENU hmenu;
	HMENU contextmenu;
    bool locked;
    bool unfullscreenZoomed = false;

    HBRUSH brush;
    COLORREF brushColor;

    HCURSOR hCursor;
    Timer timerStatusUpdate;
	bool preventRedraw = false;

    auto append(Menu& menu) -> void;
    auto append(Widget& widget) -> void;
    auto append(Layout& layout) -> void;
    auto append(StatusBar& statusBar) -> void;
    auto remove(Menu& menu) -> void;
    auto remove(Widget& widget) -> void;
    auto remove(Layout& layout) -> void {}
    auto remove(StatusBar& statusBar) -> void;
    auto setGeometry(Geometry geometry) -> void;
    auto setStatusFont(std::string font) -> void;
    auto setBackgroundColor(unsigned color) -> void;
    auto setFocused() -> void;
    auto setVisible(bool visible) -> bool;
    auto restore() -> void;
    auto setResizable(bool resizable) -> void;
    auto setStatusText(std::string text) -> void;
    auto setTitle(std::string text) -> void;
    auto setMenuVisible(bool visible) -> void;
    auto setStatusVisible(bool visible) -> void;
    auto setFullScreen(bool fullScreen) -> void;
   // auto updateFullScreen( bool inUse, unsigned displayId = 0, unsigned settingId = 0) -> void;
    auto fullScreenToggleDelayed() -> bool { return locked; }
    auto setDroppable(bool droppable) -> void;
    auto frameMargin() -> Geometry;
    auto geometry() -> Geometry;
    auto focused() -> bool;
    auto minimized() -> bool;
    auto isOffscreen() -> bool;
    auto handle() -> uintptr_t;
    auto setForeground() -> void;
    auto getScrollbarWidth() -> unsigned { return 20; }
    auto applyAspectRatio() -> void {}
    auto applyMaximizeCorrection(Geometry& geo) -> void;
    static auto fixMenuBarInDarkMode(HWND& _hwnd) -> bool;

    auto onEraseBackground() -> bool;
    auto onClose() -> void;
    auto onMove() -> void;
    auto onSize(WPARAM wparam) -> void;
    auto onSizing(int edge, RECT &rect) -> void;
    auto onDrop(WPARAM wparam) -> void;
    auto updateMenu() -> void;
    auto changeCursor( Image& image, unsigned hotSpotX, unsigned hotSpotY ) -> void;
    auto setDefaultCursor() -> void;
    auto setPointerCursor() -> void;
    auto setBlankCursor() -> void {}
    auto setDarkMode();
	static auto XPOrBelowOrWin7InXPMode() -> bool;
	
	static auto addCustomFont( CustomFont& customFont ) -> bool;
    static auto CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT;

    pWindow(Window& window, Window::Hints hints = Window::Hints::Default );
    ~pWindow();
};

struct pStatusBar {
    StatusBar& statusBar;
    
    HFONT hfont = nullptr;
    HWND hwnd = nullptr;
    WNDPROC wndprocOrig;
    HCURSOR hCursor;
    HWND hwndTip = nullptr;
    HBRUSH brush;
    StatusBar::Part* hoverPart;
    bool locked = false;
	bool disableLock = false;

    struct Trackbar {
        StatusBar::Part& part;
        HWND hwnd;
    };
    
    std::vector<StatusBar::Part*> usedParts;
    std::vector<Trackbar> trackBars;
    
    auto create() -> void;
    auto destroy() -> void;
    
    auto setFont(const std::string& font) -> void;
    auto setText(const std::string& text) -> void;
        
    auto drawItem(WPARAM wparam, LPARAM lparam) -> void;
    auto update() -> void;
    auto updatePart( StatusBar::Part& part ) -> void;
    auto updateTooltip( StatusBar::Part& part ) -> void { updatePart(part); }
    auto updatePosition() -> void;
    auto setStatusVisible(bool visible) -> void;
    auto getHeight() -> unsigned;
    auto getWidth(const std::string& text) -> unsigned;
    auto setTooltip(StatusBar::Part* part) -> void;
    auto setComposited(bool state) -> void;
	auto setLockDisabled(bool state) -> void;
    auto getSlider(StatusBar::Part& part) -> HWND;
    
    auto onClick(LPARAM lparam) -> void;
    auto getHoverPart(int xPos) -> StatusBar::Part*;
    static auto subclassWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT;
    
    pStatusBar(StatusBar& statusBar);
    ~pStatusBar();
};

struct pWidget {
    Widget& widget;
    HWND hwnd;
    HWND hwndTip;
    HIMAGELIST imageList = nullptr;
    HFONT hfont = nullptr;
    WNDPROC wndprocOrig;
    bool locked = false;
    TabFrameLayout* parentTabFrameLayout = nullptr;
    DoubleBuffer* doubleBuffer = nullptr;
    
    struct {
        bool updated = false;
        Size minimumSize = {0, 0};
    } calculatedMinimumSize;

    auto focused() -> bool;
    auto setFocused() -> void;
	auto getBackgroundBrush() -> HBRUSH;
	auto getParentHandle() -> HWND;
	auto getParentTabWidget() -> Widget*;
	auto needRebuild() -> bool;
	
    virtual auto minimumSize() -> Size { return {0,0}; }
    virtual auto borderSize() -> unsigned { return 2; }
    virtual auto setEnabled(bool enabled) -> void;
    virtual auto setEnabledThreaded(bool enabled) -> void;
    virtual auto setVisible(bool visible) -> void;
    virtual auto setFont(std::string font) -> void;
    virtual auto setText(const std::string& text) -> void;
    virtual auto setGeometry(Geometry geometry) -> void;
    virtual auto rebuild() -> void;
	virtual auto setForegroundColor(unsigned color) -> void {}
    virtual auto setForegroundColorThreaded(unsigned color) -> void {}
    virtual auto setBackgroundColor(unsigned color) -> void {}

    virtual auto callDrops(std::vector<std::string>& paths) -> void {}
    virtual auto callDragEnter(std::vector<std::string>& paths) -> void {}
    virtual auto callDragMove(POINTL ptl) -> void {}
    virtual auto callDragLeave() -> void {}

    virtual auto destroy() -> void { destroy(hwnd); }
    auto destroy(HWND& handle) -> void;
    auto destroyImageList() -> void;
    auto setTooltip(std::string tooltip) -> void;
    auto setEditFieldColor(WPARAM wparam) -> HBRUSH;
    virtual auto createTooltip(bool useBallon = true) -> void;
    auto getMinimumSize() -> Size;
	static auto getScaledContainerSize( Size size ) -> Size;
	static auto getScaledDim( unsigned value ) -> unsigned;
    virtual auto init() -> void {}
   
    pWidget(Widget& widget);
    virtual ~pWidget();
};

struct pLineEdit : pWidget {
    LineEdit& lineEdit;

    auto minimumSize() -> Size;
    auto setEditable(bool editable) -> void;
    auto setText(const std::string& text) -> void;
    auto text() -> std::string;
    auto setDroppable(bool droppable) -> void;
    auto setMaxLength( unsigned maxLength ) -> void;
    auto setForegroundColor(unsigned color) -> void;
    auto setPlaceholder(const std::string& placeholder) -> void;
    auto setAlign( LineEdit::Align align ) -> void;

    pLineEdit(LineEdit& lineEdit) : pWidget(lineEdit), lineEdit(lineEdit) {}
    auto rebuild() -> void;
    auto onChange() -> void;
    auto onFocus() -> void;
    auto create() -> void;
    
    static auto CALLBACK subclassWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT;
};

struct pMultilineEdit : pWidget {
    MultilineEdit& multilineEdit;

    auto setEditable(bool editable) -> void;
    auto setText(const std::string& text) -> void;
    auto text() -> std::string;
    auto setForegroundColor(unsigned color) -> void;

    pMultilineEdit(MultilineEdit& multilineEdit) : pWidget(multilineEdit), multilineEdit(multilineEdit) {}
    auto rebuild() -> void;
    auto onChange() -> void;
    auto onFocus() -> void;
    auto create() -> void;
    
    static auto CALLBACK subclassWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT;
};

struct pLabel : pWidget {
    Label& label;

    auto minimumSize() -> Size;
    auto setText(const std::string& text) -> void;
    auto setTextThreaded(const std::string& text) -> void;
	auto setEnabled(bool enabled) -> void;
	auto setForegroundColor(unsigned color) -> void;
    auto setForegroundColorThreaded(unsigned color) -> void;
    auto setFont(std::string font) -> void;
    auto setAlign( Label::Align align ) -> void;
    auto drawItem(LPDRAWITEMSTRUCT lDraw) -> void;

    pLabel(Label& label) : pWidget(label), label(label) {}
    auto rebuild() -> void;
    auto create() -> void;
    static auto CALLBACK subclassWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT;
    auto buildLabel(HWND hwnd, HDC hdc, RECT& rc) -> void;
};

struct pHyperlink : pWidget {
    Hyperlink& hyperlink;
    
    pHyperlink(Hyperlink& hyperlink) : pWidget(hyperlink), hyperlink(hyperlink) {}
    auto minimumSize() -> Size;
    auto setText(const std::string& text) -> void;
    auto rebuild() -> void;
    auto create() -> void;
    auto generate() -> std::string;
    auto setUri( std::string uri, std::string wrap ) -> void; 

    static auto CALLBACK subclassWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT;    
};

struct pSquareCanvas : pWidget {
    SquareCanvas& squareCanvas;

    pSquareCanvas(SquareCanvas& squareCanvas) : pWidget(squareCanvas), squareCanvas(squareCanvas) {}
    auto rebuild() -> void;
    auto create() -> void;
    auto setBackgroundColor( unsigned color ) -> void;
    auto setBorderColor(unsigned borderSize, unsigned borderColor) -> void;
    static auto CALLBACK subclassWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT;
    auto drawItem(LPDRAWITEMSTRUCT lDraw) -> void;
};

struct pImageView : pWidget {
    ImageView& imageView;
    HCURSOR hCursor = nullptr;
    auto setImage(Image* image) -> void;
    auto setUri( std::string uri ) -> void {}
    auto onLink() -> void;
    auto rebuild() -> void;
    auto setEnabled(bool enabled) -> void;
    auto create() -> void;
    auto minimumSize() -> Size;
    auto updateCursor() -> void;
    auto drawItem(LPDRAWITEMSTRUCT lDraw) -> void;

    pImageView(ImageView& imageView) : pWidget(imageView), imageView(imageView) {}
    static auto CALLBACK subclassWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT;
};

struct pButton : pWidget {
    Button& button;
    HBITMAP hbitmap = nullptr;

    auto setImage(Image* image) -> void;
    auto setText(const std::string& text) -> void;
    auto setEnabled(bool enabled) -> void;
    auto setEnabledThreaded(bool enabled) -> void;
    auto customDraw(HWND hWnd, HDC hdc, RECT& rc, RECT& rcpaint) -> void;
    auto minimumSize() -> Size;
    auto onActivate() -> void;
    auto rebuild() -> void;
    auto create() -> void;
    
    static auto CALLBACK subclassWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT;

    pButton(Button& button) : pWidget(button), button(button) {}
};

struct pStepButton : pWidget {
    StepButton& stepButton;
    HWND buddyHwnd;
    bool lockChangeWhileStepping = false;
    
    auto updateRange() -> void;
    auto setValue( int16_t value ) -> void;

    auto minimumSize() -> Size;
    auto onStep(LPARAM lparam) -> void;
    auto rebuild() -> void;
    auto create() -> void;
    auto setVisible(bool visible) -> void;
    auto setEnabled(bool enabled) -> void;
    auto onChange() -> void;
    auto setForegroundColor(unsigned color) -> void;
    
    auto setGeometry(Geometry geometry) -> void;
    auto setFont(std::string font) -> void;
    
    static auto CALLBACK subclassWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT;
    static auto CALLBACK subclassWndProcBuddy(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT;

    pStepButton(StepButton& stepButton) : pWidget(stepButton), stepButton(stepButton) {}
};


struct pCheckButton : pWidget {
    CheckButton& checkButton;

    auto minimumSize() -> Size;
    auto setChecked(bool checked) -> void;
    auto onToggle() -> void;
    auto rebuild() -> void;
    auto create() -> void;
    
    static auto CALLBACK subclassWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT;

    pCheckButton(CheckButton& checkButton) : pWidget(checkButton), checkButton(checkButton) {}
};

struct pCheckBox : pWidget {
    CheckBox& checkBox;

    auto minimumSize() -> Size;
    auto setChecked(bool checked) -> void;
    auto onToggle() -> void;
    auto rebuild() -> void;
    auto create() -> void;
    auto onCustomDraw(LPARAM lparam) -> LRESULT;
    
    static auto CALLBACK subclassWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT;

    pCheckBox(CheckBox& checkBox) : pWidget(checkBox), checkBox(checkBox) {}
};

struct pComboButton : pWidget {
    ComboButton& comboButton;
    std::vector<HFONT> hfonts;

    auto append(std::string text, const std::string& _font) -> void;
    auto remove(unsigned selection) -> void;
    auto minimumSize() -> Size;
    auto reset() -> void;
    auto setGeometry(Geometry geometry) -> void;
    auto setSelection(unsigned selection) -> void;
    auto setText(unsigned selection, const std::string& text) -> void;
    auto rebuild() -> void;
    auto create() -> void;
    auto onChange() -> void;
    auto setDroppable(bool droppable) -> void;

    auto measureItem(LPMEASUREITEMSTRUCT lpmis) -> bool;
    auto drawItem(LPDRAWITEMSTRUCT lDraw) -> void;

    static auto CALLBACK subclassWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT;

    pComboButton(ComboButton& comboButton) : pWidget(comboButton), comboButton(comboButton) {}
    ~pComboButton();
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

    static auto CALLBACK subclassWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT;
    auto onCustomDraw(LPARAM lparam) -> LRESULT;
    
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
    auto onCustomDraw(LPARAM lparam) -> LRESULT;
    
    static auto CALLBACK subclassWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT;

    pRadioBox(RadioBox& radioBox) : pWidget(radioBox), radioBox(radioBox) {}
};

struct pProgressBar : pWidget {
    ProgressBar& progressBar;

    auto minimumSize() -> Size;
    auto rebuild() -> void;
    auto create() -> void;
    auto setPosition(unsigned position) -> void;
    auto setPositionThreaded(unsigned position) -> void;
    
    static auto CALLBACK subclassWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT;

    pProgressBar(ProgressBar& progressBar) : pWidget(progressBar), progressBar(progressBar) {}
};

struct pListView : pWidget {
    ListView& listView;
    std::vector<Image*> images;
    int lastItem = -1;

    HBRUSH bgBrush = nullptr;
    HBRUSH hiBrush = nullptr;
    std::vector<std::pair<unsigned, HBRUSH>> rowBrushes;

    auto append(const std::vector<std::string>& list, bool preventColumnResizing = false) -> void;
    auto autoSizeColumns() -> void;
    auto remove(unsigned selection) -> void;
    auto reset() -> void;
    auto setGeometry(Geometry geometry) -> void;
    auto setHeaderText(std::vector<std::string> list) -> void;
	auto buildHeaderText(std::vector<std::string> list) -> void;
    auto setHeaderVisible(bool visible) -> void;
    auto setSelection(unsigned selection) -> void;
    auto setSelected(bool selected) -> void;
    auto setText(unsigned selection, unsigned position, const std::string& text, bool preventColumnResizing) -> void;
    auto rebuild() -> void;
    auto create() -> void;
    auto setFont(std::string font) -> void;
    auto setContent() -> void;
    auto setImage(unsigned selection, unsigned position, int imageListPos) -> void;
    auto setImage(unsigned selection, unsigned position, Image& image, bool preventColumnResizing = false) -> void;
    auto buildImageList() -> void;
    auto addToImageList(Image& image, unsigned size) -> void;
	auto setBackgroundColor(unsigned color) -> void;
	auto setForegroundColor(unsigned color) -> void;
    auto setDarkBackground() -> void;
    auto setDarkForeground() -> void;
    auto setRowTooltip(unsigned selection, std::string tooltip) -> void {}
    auto relayMesssageToToolTip(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam) -> void;
    auto updateRowToolTip(HWND hwnd, int curItem, RECT rect) -> void;
    auto colorRowTooltips( bool colorTip ) -> void {}
    auto lockRedraw() -> void;
    auto unlockRedraw() -> void;
    auto getFirstVisibleRow() -> unsigned;
    static auto getThemeHeaderColors(HPEN& captionPen) -> HBRUSH;

    static auto CALLBACK subclassWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT;    
    auto onCustomDraw(LPARAM lparam) -> LRESULT;
    auto createTooltip(bool useBallon = false) -> void;
    
    auto onChange(LPARAM lparam) -> void;
    auto onActivate(LPARAM lparam) -> void;
    auto onClick(LPARAM lparam) -> void;
    
    auto measureItem(LPMEASUREITEMSTRUCT lpmis) -> void;
    auto drawItem(LPDRAWITEMSTRUCT lDraw) -> void;
    auto clearBrush() -> void;
    auto setSelectionColor(unsigned foregroundColor = 0, unsigned backgroundColor = 0) -> void;
    auto findRowBrush(unsigned row) -> HBRUSH;
    auto updateRowColors() -> void;
    auto updateRowForegroundColors() -> void {}
    auto updateSpacing() -> void;

    pListView(ListView& listView) : pWidget(listView), listView(listView) {}
};

struct pTreeViewItem {
    TreeViewItem& treeViewItem;
    HTREEITEM hTreeItem = nullptr;

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
    auto onExpanded(LPARAM lparam) -> void;
	auto setBackgroundColor(unsigned color) -> void;
	auto setForegroundColor(unsigned color) -> void;
    auto setDarkBackground() -> void;
    auto setDarkForeground() -> void;

    static auto CALLBACK subclassWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT;
    pTreeView(TreeView& treeView) : pWidget(treeView), treeView(treeView) {}
};

struct pViewport : public pWidget {
    Viewport& viewport;
    DropManager dropManager;
    Timer hideTimer;

    auto handle(bool hintRecreation) -> uintptr_t;
    auto setDroppable(bool droppable) -> void;
    static auto CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT;

    auto rebuild() -> void;
    auto _rebuild() -> void;
    auto create() -> void;
    auto hideCursorByInactivity(unsigned delayMS) -> void;

    auto callDrops(std::vector<std::string>& paths) -> void;
    auto callDragEnter(std::vector<std::string>& paths) -> void;
    auto callDragMove(POINTL ptl) -> void;
    auto callDragLeave() -> void;

    pViewport(Viewport& viewport) : pWidget(viewport), viewport(viewport),  dropManager(this) {}
};
//Layout Widgets are not directly accessable in frontend
struct pFrame : pWidget {
    Widget& widget;
    HWND leftBorder;
    HWND rightBorder;
    HWND bottomBorder;
	HPEN pen = nullptr;
	bool roundedConrner = false;

    auto minimumSize() -> Size;
    auto setGeometry(Geometry geometry) -> void;
    auto rebuild() -> void;
    auto create() -> void;
    auto setVisible(bool visible) -> void;
	auto getBorderColor() -> COLORREF;
    auto setEnabled(bool enabled) -> void;
	
	auto setText(const std::string& text) -> void;
	auto setFont(std::string font) -> void;
    
    static auto CALLBACK subclassWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT;
    static auto CALLBACK subclassWndProcL(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT;
    static auto CALLBACK subclassWndProcR(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT;
    static auto CALLBACK subclassWndProcB(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT;

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
    auto setText(unsigned selection, const std::string& text) -> void;
    auto setSelection(unsigned selection) -> void;
    auto onChange() -> void;
    auto setImage(unsigned selection, Image& image) -> void;
    auto buildImageList() -> void;
    auto setFont(std::string font) -> void;
    static auto CALLBACK subclassWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT;
    static auto getTabBackgroundForControl(HWND tab, HWND control) -> HBRUSH;
    static auto paintTab(HWND hWnd, HDC hdc, const RECT& rect) -> void;

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
    auto update(Window* window) -> void;

    pMenu(Menu& menu) : pMenuBase(menu), menu(menu) { hmenu = nullptr; }
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

struct FileDialogEventHandler :
    public IFileDialogControlEvents,
    public IFileDialogEvents {

    BrowserWindow* browserWindow;
    BrowserWindow::State* state;
    IFileDialog* pDlg = nullptr;
    
    STDMETHODIMP QueryInterface(REFIID riid, void** ppvObject);
    STDMETHODIMP_(ULONG) AddRef() { return 1; }
    STDMETHODIMP_(ULONG) Release() { return 1; }

    // IFileDialogEvents
    STDMETHODIMP OnFileOk(IFileDialog* pfd) { return S_OK; }
    STDMETHODIMP OnFolderChanging(IFileDialog* pfd, IShellItem* psiFolder) { return S_OK; }
    STDMETHODIMP OnFolderChange(IFileDialog* pfd);
    STDMETHODIMP OnSelectionChange(IFileDialog* pfd);
    STDMETHODIMP OnShareViolation(IFileDialog* pfd, IShellItem* psi, FDE_SHAREVIOLATION_RESPONSE* pResponse) { return S_OK; }
    STDMETHODIMP OnTypeChange(IFileDialog* pfd) { return S_OK; }
    STDMETHODIMP OnOverwrite(IFileDialog* pfd, IShellItem* psi, FDE_OVERWRITE_RESPONSE* pResponse) { return S_OK; }
    
    // IFileDialogControlEvents methods
    IFACEMETHODIMP OnItemSelected(IFileDialogCustomize* pfdc, DWORD dwIDCtl, DWORD dwIDItem) { return S_OK; }
    IFACEMETHODIMP OnButtonClicked(IFileDialogCustomize* pfdc, DWORD dwIDCtl);
    IFACEMETHODIMP OnCheckButtonToggled(IFileDialogCustomize*, DWORD, BOOL);
    IFACEMETHODIMP OnControlActivating(IFileDialogCustomize* pfdc, DWORD dwIDCtl) { return S_OK; }
    
    auto getFilePath(IFileDialog* pfd, std::string& folderPath, std::vector<std::string>& multiples) -> std::string;
};

struct pBrowserWindow {
    BrowserWindow& browserWindow;
    IFileDialog* pDlg = nullptr;
    FileDialogEventHandler* pDialogEventHandler = nullptr;
    DWORD cookie;
    static HWND dummyParent;
    HFONT listFont = nullptr;
    HBRUSH listBgBrush = nullptr;
    HBRUSH listHiBrush = nullptr;
    HBRUSH firstRowBrush = nullptr;
    std::string selectedPath = "";
    BrowserWindow::CustomButton* selectedButton = nullptr;
    unsigned contentSelection = 0;
    HWND dialogHwnd = nullptr;
    HWND hwndTip = nullptr;
    HWND hDlg = nullptr;
    int lastItem = -1;
    std::vector<std::string> toolTips;
    std::vector<std::string> selectedFiles;
    bool multi = false;

    struct Button {
        HWND hwnd;
        bool checkbox;
        int width;
        int height;
        int relativeX;
        int relativeY;
    };    
    std::vector<Button> buttons;
    
    bool inited = false;
    int widthCustomView = 0;
    int customGapTop = 0;
    int customGapBottom = 0;
    int listRelativeX = 0;
    int listWidth = 0;
    int listItemWidth = 0;
    int listItemHeight = 0;

    auto directory() -> std::string;
    auto directoryVista() -> std::string;
    auto fileGeneric(bool save, bool multi = false) -> std::vector<std::string>;
    auto file(bool save) -> std::string { return fileGeneric(save)[0]; }
    auto fileMulti() -> std::vector<std::string> { return fileGeneric(false, true); }

    auto fileVista(bool save, bool multi = false) -> std::vector<std::string>;
    auto close() -> void; 
    auto setForeground() -> void;
    auto resize(HWND fileDialogView, bool init = false) -> void;
    auto adjustDialogByScreenResolution(HWND fileDialogView, HWND listBox) -> void;
    auto detached() -> bool { return false; }
    auto visible() -> bool;
    auto contentViewSelection() -> unsigned;
    auto createTooltip(HWND hwnd) -> void;
    auto setToolTip(HWND hwnd, int curItem, RECT rect) -> void;
	auto setButtonTooltip(HWND buttonHwnd, std::string tooltip) -> void;
    auto relayMesssageToToolTip(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam) -> void;
    auto setListings( std::vector<BrowserWindow::Listing>& listings ) -> void;
    auto getSelectedPath(HWND dlg) -> std::string;
    auto splitFilenames(std::string& fileLine, std::vector<std::string>& files ) -> void;
    
    auto getIFileParent() -> HWND;
    
    static auto CALLBACK PathCallbackProc(HWND hwnd, UINT msg, LPARAM lparam, LPARAM lpdata) -> int;
    static auto CALLBACK CustomWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT;
    static auto CALLBACK OfnHookProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) -> UINT_PTR;
    auto wndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT;
    
    static auto CALLBACK subclassListbox(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT;
    
    pBrowserWindow(BrowserWindow& browserWindow);
    ~pBrowserWindow();
};

struct pMessageWindow {
    static HHOOK hhookCBTProc;
    static WNDPROC wndprocOrig;
    static auto error(MessageWindow::State& state) -> MessageWindow::Response;
    static auto information(MessageWindow::State& state) -> MessageWindow::Response;
    static auto question(MessageWindow::State& state) -> MessageWindow::Response;
    static auto warning(MessageWindow::State& state) -> MessageWindow::Response;
    static auto translateResponse(UINT response) -> MessageWindow::Response;
    static auto translateButtons(MessageWindow::Buttons buttons) -> UINT;

    static auto CALLBACK subclassWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT;
    static auto CALLBACK pfnCBTMsgBoxHook(int nCode, WPARAM wparam, LPARAM lparam) ->LRESULT;
};

struct pFont {
    static auto system(unsigned size, std::string style, bool monospaced = false) -> std::string;
    static auto create(const std::string& desc) -> HFONT;
	static auto add(CustomFont& customFont) -> bool;
    static auto free(HFONT& hfont) -> void;
    static auto size(HFONT hfont, std::string text) -> Size;
    static auto size(const std::string& font, const std::string& text) -> Size;
	static auto findMemoryFont(std::string name) -> HFONT;
    static auto dpi() -> Position;
    static auto scale( unsigned pixel ) -> unsigned;
    static auto systemFontFile() -> std::string;
};

struct pSystem {
    static auto getUserDataFolder() -> std::string;
    static auto getResourceFolder(std::string appIdent) -> std::string;
    static auto getWorkingDirectory() -> std::string;
    static auto getExecutableDirectory() -> std::string;
    static auto getDesktopSize() -> Size;
    static auto sleep(unsigned milliSeconds) -> void;
    static auto isOffscreen( Geometry geometry ) -> bool;
    static auto getOSLang() -> System::Language;
    static auto printToCmd( std::string str ) -> void;    

    static bool offscreen;
    static std::vector<RECT> clientRects;

};

struct pMonitor {

    struct Device {
        unsigned id;
        std::string ident;
        DISPLAY_DEVICE displayDevice;
        DEVMODE originalSetting;
    };

    struct Setting {
        unsigned id;
        std::string ident;
        Device* parentDevice;
        DEVMODE devMode;
        float rate;
    };

    static std::vector<Device> devices;
    static std::vector<Setting> settings;
    static Device* activeDevice;
    static Setting* activeSetting;

    static auto fetchDisplays() -> void;
    static auto getDisplays() -> std::vector<Monitor::Property>;

    static auto fetchSettings( Device* device ) -> void;
    static auto getSettings( unsigned displayId ) -> std::vector<Monitor::Property>;

    static auto setSetting( unsigned displayId, unsigned settingId ) -> bool;
    static auto resetSetting() -> void;

    static auto getCurrentRefreshRate() -> float;
    static auto getCurrentResolution() -> Size { return {0,0}; }
    static auto getRefreshRate( unsigned displayId, unsigned settingId ) -> float;

    static auto getTimingInfo() -> DwmGetCompositionTimingInfo_t;
};

struct pThreadPriority {
    static auto setPriority(ThreadPriority::Mode mode, float minProcessingTimeInMilliSeconds = 0, float maxProcessingTimeInMilliSeconds = 0) -> bool;
};

struct pInterProcessParam {
    bool quitRequested;
    int ident;
};

struct pInterProcess {
    static bool anotherInstanceRunning;
    static HANDLE fileMapping;
    static pInterProcessParam* param;
    static Timer* comTimer;

    static auto closeOtherInstances() -> void;
    static auto Acquire() -> bool;
    static auto Release() -> void;
    static auto checkQuit() -> void;
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
}
