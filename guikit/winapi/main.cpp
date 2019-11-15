#include "main.h"

namespace GUIKIT {

#include "tools.cpp"
#include "menu.cpp"
#include "browserWindow.cpp"
#include "messageWindow.cpp"
    
#include "widgets/widget.cpp"   
#include "widgets/button.cpp"   
#include "widgets/lineedit.cpp"
#include "widgets/label.cpp"
#include "widgets/hyperlink.cpp"
#include "widgets/squareCanvas.cpp"   
#include "widgets/checkbutton.cpp"
#include "widgets/checkbox.cpp"
#include "widgets/combobutton.cpp"
#include "widgets/slider.cpp"
#include "widgets/radiobox.cpp"
#include "widgets/progressbar.cpp"
#include "widgets/frame.cpp"
#include "widgets/tabframe.cpp"
#include "widgets/viewport.cpp"
#include "widgets/listview.cpp"
#include "widgets/treeview.cpp"
   
auto pApplication::run() -> void {
    if (Application::loop) {
        while(!Application::isQuit) {
            Application::loop();
            processEvents();
        }
    } else {
        MSG msg;
        while(GetMessage(&msg, 0, 0, 0)) {
            processMessage(msg);
        }
    }
}

auto pApplication::processEvents() -> void {
    MSG msg;
    while(PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) { processMessage(msg); }
}

auto pApplication::processMessage(MSG& msg) -> void {
    if(!IsDialogMessage(GetForegroundWindow(), &msg)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

auto pApplication::quit() -> void {
    PostQuitMessage(0);
    
    if (uxTheme)
        FreeLibrary(uxTheme);
}

std::string pApplication::cwd = "";
unsigned pApplication::version = 0;

HMODULE pApplication::uxTheme = nullptr;
FN_BeginBufferedPaint pApplication::pfnBeginBufferedPaint = nullptr;
FN_EndBufferedPaint pApplication::pfnEndBufferedPaint = nullptr;

auto pApplication::initialize() -> void {
    CoInitialize(0);
    InitCommonControls();

    WNDCLASS wc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hbrBackground = GetSysColorBrush(COLOR_3DFACE);
    wc.hCursor = LoadCursor(0, IDC_ARROW);
    wc.hIcon = LoadIcon(GetModuleHandle(0), MAKEINTRESOURCE(2));
    wc.hInstance = GetModuleHandle(0);
    wc.lpfnWndProc = pWindow::wndProc;
    wc.lpszClassName = L"app_gui";
    wc.lpszMenuName = 0;
    wc.style = CS_HREDRAW | CS_VREDRAW;
    RegisterClass(&wc);

    wc.hbrBackground = CreateSolidBrush(RGB(0, 0, 0));
    wc.hIcon = LoadIcon(0, IDI_APPLICATION);
    wc.lpfnWndProc = pViewport::wndProc;
    wc.lpszClassName = L"app_viewport";
    RegisterClass(&wc);

    currentWorkingDirectory();
	version = getVersion();
    
    if(version >= WindowsVista) {
        uxTheme = LoadLibraryA("UXTHEME.DLL");

        if (uxTheme) {
            pfnBeginBufferedPaint = (FN_BeginBufferedPaint)::GetProcAddress(uxTheme, "BeginBufferedPaint");
            pfnEndBufferedPaint = (FN_EndBufferedPaint)::GetProcAddress(uxTheme, "EndBufferedPaint");
        }
    }
}

auto pApplication::currentWorkingDirectory() -> std::string {
    if ( !cwd.empty() ) return cwd;
    wchar_t path[PATH_MAX] = L"";
    _wgetcwd(path, sizeof(path));
    cwd = utf8_t(path);
    return cwd;
}

auto CALLBACK pApplication::wndProc(WNDPROC windowProc, HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT {
    Base* base = (Base*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if(base == nullptr) return DefWindowProc(hwnd, msg, wparam, lparam);

    Window& window = dynamic_cast<Window*>(base) ? (Window&)*base : *((Widget*)base)->window();

    switch(msg) {
        case WM_CTLCOLORBTN:
        case WM_CTLCOLORSTATIC: {
            Base* base = (Base*)GetWindowLongPtr((HWND)lparam, GWLP_USERDATA);
            if(base == nullptr) break;

            if (dynamic_cast<LineEdit*>(base)) {
                return windowProc(hwnd, WM_CTLCOLOREDIT, wparam, lparam);
            }

            TabFrameLayout* tabFrameLayout = TabFrameLayout::getParentTabFrame( (Sizable*)base );
            if (tabFrameLayout) {
                if (!IsAppThemed()) break;

                SetBkMode((HDC)(wparam), TRANSPARENT);
                pTabFrame::updateTabBackgroundForControl( tabFrameLayout->frameWidget->p.hwnd, ((Widget*)base)->p.hwnd);
                return (INT_PTR)pTabFrame::bkgndBrush;

            } else if(window.p.brush) {
                SetBkColor((HDC)wparam, window.p.brushColor);
                return (INT_PTR)window.p.brush;
            }
            break;
        }

        case WM_COMMAND: {
            unsigned id = LOWORD(wparam);
            HWND control = GetDlgItem(hwnd, id);
            base = control ? (Base*)GetWindowLongPtr(control, GWLP_USERDATA) : Base::find(id);
            if(base == nullptr) break;
            if(dynamic_cast<MenuItem*>(base)) { ((MenuItem*)base)->p.onActivate(); return false; }
            if(dynamic_cast<MenuCheckItem*>(base)) { ((MenuCheckItem*)base)->p.onToggle(); return false; }
            if(dynamic_cast<MenuRadioItem*>(base)) { ((MenuRadioItem*)base)->p.onActivate(); return false; }
            if(dynamic_cast<LineEdit*>(base) && HIWORD(wparam) == EN_SETFOCUS) { ((LineEdit*)base)->p.onFocus(); return false; }
            if(dynamic_cast<LineEdit*>(base) && HIWORD(wparam) == EN_CHANGE) { ((LineEdit*)base)->p.onChange(); return false; }
            if(dynamic_cast<Button*>(base)) { ((Button*)base)->p.onActivate(); return false; }
            if(dynamic_cast<CheckButton*>(base)) { ((CheckButton*)base)->p.onToggle(); return false; }
            if(dynamic_cast<CheckBox*>(base)) { ((CheckBox*)base)->p.onToggle(); return false; }
            if(dynamic_cast<ComboButton*>(base) && HIWORD(wparam) == CBN_SELCHANGE) { ((ComboButton*)base)->p.onChange(); return false; }
            if(dynamic_cast<RadioBox*>(base)) { ((RadioBox*)base)->p.onActivate(); return false; }
            break;
        }
        
        case WM_NOTIFY: {
            unsigned id = LOWORD(wparam);
            HWND control = GetDlgItem(hwnd, id);
            base = (Base*)GetWindowLongPtr(control, GWLP_USERDATA);
            if(base == nullptr) break;
            if(dynamic_cast<ListView*>(base) && ((LPNMHDR)lparam)->code == LVN_ITEMCHANGED) { ((ListView*)base)->p.onChange(lparam); break; }
            if(dynamic_cast<ListView*>(base) && ((LPNMHDR)lparam)->code == LVN_ITEMACTIVATE) { ((ListView*)base)->p.onActivate(); break; }
            if(dynamic_cast<ListView*>(base) && ((LPNMHDR)lparam)->code == NM_CUSTOMDRAW) { return ((ListView*)base)->p.onCustomDraw(lparam); }
            if(dynamic_cast<TreeView*>(base) && ((LPNMHDR)lparam)->code == TVN_SELCHANGED) { ((TreeView*)base)->p.onChange(); break; }
            if(dynamic_cast<TreeView*>(base) && ((LPNMHDR)lparam)->code == NM_DBLCLK) { ((TreeView*)base)->p.onActivate(); break; }
            if(dynamic_cast<TreeView*>(base) && ((LPNMHDR)lparam)->code == NM_RETURN) { ((TreeView*)base)->p.onActivate(); break; }
            if(dynamic_cast<TabFrameLayout::TabFrame*>(base) && ((LPNMHDR)lparam)->code == TCN_SELCHANGE) { ((TabFrameLayout::TabFrame*)base)->p.onChange(); break; }            
            if(dynamic_cast<Hyperlink*>(base) && ((LPNMHDR)lparam)->code == NM_CLICK) {
                PNMLINK pNMLink = (PNMLINK) lparam;
                LITEM item = pNMLink->item;
                ShellExecute(NULL, L"open", item.szUrl, NULL, NULL, SW_SHOW);
            }
            
            break;
        }
		case WM_CONTEXTMENU: {
			if (!window.onContext) break;
			if (GetMenuItemCount(window.p.contextmenu) <= 0) break;			
			if (!window.onContext()) break;

			POINT pt;
			GetCursorPos(&pt);						
			int mid = TrackPopupMenuEx(window.p.contextmenu, TPM_RIGHTBUTTON | TPM_RETURNCMD, pt.x, pt.y, hwnd, NULL);
			if (mid) SendMessage(hwnd, WM_COMMAND, mid, 0);			
			break;
		}
        case WM_HSCROLL:
        case WM_VSCROLL: {
            base = (Base*)GetWindowLongPtr((HWND)lparam, GWLP_USERDATA);
            if(base == nullptr) break;
            if(dynamic_cast<Slider*>(base)) { ((Slider*)base)->p.onChange(); return true; }
            break;
        }
		case WM_MEASUREITEM: {
			LPMEASUREITEMSTRUCT lpmis = (LPMEASUREITEMSTRUCT)lparam;
			if ( (lpmis != NULL) && (lpmis->CtlType == ODT_MENU) ) {
				if (pMenuBase::measureItem(lpmis))
					return true;
			}
			break;
		}
		case WM_DRAWITEM: {
			LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT) lparam;
			if ((lpdis != NULL) && (lpdis->CtlType == ODT_MENU)) {
				if (pMenuBase::drawItem(lpdis))
					return true;
			}
			break;
		}			
    }
    return windowProc(hwnd, msg, wparam, lparam);
}

//window
pWindow::pWindow(Window& window) : window(window) {
    locked = false;
    bgUpdateState = 0;
    brush = 0;
    hstatusfont = 0;
    hCursor = LoadCursor(0, IDC_ARROW);

    Geometry geo = window.state.geometry;

    hwnd = CreateWindow(L"app_gui", L"", ResizableStyle, geo.x, geo.y, geo.width, geo.height, 0, 0, GetModuleHandle(0), 0);
    hmenu = CreateMenu();
	contextmenu = CreatePopupMenu();
    hstatus = CreateWindow(STATUSCLASSNAME, L"", WS_CHILD, 0, 0, 0, 0, hwnd, 0, GetModuleHandle(0), 0);

    setStatusFont(Font::system());
    SetWindowLongPtr(hstatus, GWL_STYLE, GetWindowLong(hstatus, GWL_STYLE) | WS_DISABLED);

    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)&window);
    setDroppable(window.state.droppable);
}

pWindow::~pWindow() {
    pFont::free(hstatusfont);
    DestroyWindow(hstatus);
    DestroyMenu(hmenu);
	DestroyMenu(contextmenu);
    DestroyWindow(hwnd);
}

auto pWindow::handle() -> uintptr_t {
    return (uintptr_t)hwnd;
}

auto CALLBACK pWindow::wndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT {
    if(Application::isQuit) return DefWindowProc(hwnd, msg, wparam, lparam);

    Base* base = (Base*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if(base == nullptr) return DefWindowProc(hwnd, msg, wparam, lparam);

    Window& window = dynamic_cast<Window*>(base) ? (Window&)*base : *((Widget*)base)->window();

    switch(msg) {
        case WM_CLOSE: window.p.onClose(); return true;
        case WM_MOVE: window.p.onMove(); break;
        case WM_SIZE: window.p.onSize(); break;
        case WM_DROPFILES: window.p.onDrop(wparam); return false;
		case WM_ENTERMENULOOP:
			if(window.winapi.onMenu) window.winapi.onMenu();
			break;
        case WM_ENTERSIZEMOVE:
            if(window.p.bgUpdateState > 0) window.p.bgUpdateState = 2;
            break;
        case WM_EXITSIZEMOVE:
            if(window.p.bgUpdateState > 0) {
                window.p.bgUpdateState = 3;
                InvalidateRect( hwnd, NULL, true );
            } break;
        case WM_PAINT:
            if(window.p.brush != 0 && window.p.bgUpdateState == 3 ) {
                window.p.bgUpdateState = 1;
                return true;
            } break;
        case WM_ERASEBKGND: 
			if(window.p.onEraseBackground()) return true;
			break;
		case WM_ACTIVATEAPP:
			if ((LOWORD(wparam) == WA_ACTIVE) || (LOWORD(wparam) == WA_CLICKACTIVE)) {
				
			} else if (LOWORD(wparam) == WA_INACTIVE) {
				if(window.fullScreen()) ShowWindow( hwnd, SW_MINIMIZE );
			}
			break;
        case WM_SETCURSOR:
            if (LOWORD(lparam) == HTCLIENT) {
                SetCursor( window.p.hCursor );
                return true;        			               
            }
            break;
    }
    return pApplication::wndProc(DefWindowProc, hwnd, msg, wparam, lparam);
}

auto pWindow::setDroppable(bool droppable) -> void {
    DragAcceptFiles(hwnd, droppable);
}

auto pWindow::setFocused() -> void {
    if(!window.visible()) window.setVisible(true);
    SetFocus(hwnd);
}

auto pWindow::setVisible(bool visible) -> void {
    ShowWindow(hwnd, visible ? SW_SHOWNORMAL : SW_HIDE);
}

auto pWindow::setResizable(bool resizable) -> void {
    SetWindowLongPtr(hwnd, GWL_STYLE, resizable ? ResizableStyle : FixedStyle);
    if( !window.fullScreen() ) setGeometry(window.state.geometry);
}

auto pWindow::setStatusFont(std::string font) -> void {
    pFont::free(hstatusfont);
    hstatusfont = pFont::create(font);
    SendMessage(hstatus, WM_SETFONT, (WPARAM)hstatusfont, 0);
}

auto pWindow::setTitle(std::string text) -> void {
    SetWindowText(hwnd, utf16_t(text));
}

auto pWindow::setStatusText(std::string text) -> void {
    SendMessage(hstatus, SB_SETTEXT, 0, (LPARAM)(wchar_t*)utf16_t(text));
}

auto pWindow::setMenuVisible(bool visible) -> void {
    locked = true;
    SetMenu(hwnd, visible ? hmenu : 0);
    setGeometry( !window.fullScreen() ? window.state.geometry : geometry() );
    locked = false;
}

auto pWindow::setStatusVisible(bool visible) -> void {
    locked = true;
    ShowWindow(hstatus, visible ? SW_SHOWNORMAL : SW_HIDE);
    setGeometry( !window.fullScreen() ? window.state.geometry : geometry() );
    locked = false;
}

auto pWindow::setBackgroundColor(unsigned color) -> void {
    if(brush) DeleteObject(brush);
    brushColor = RGB((color >> 16) & 0xff, (color >> 8) & 0xff, color & 0xff);
    brush = CreateSolidBrush(brushColor);
}

auto pWindow::focused() -> bool {
    return GetForegroundWindow() == hwnd;
}

auto pWindow::frameMargin() -> Geometry {
    static int menuWhatIsThis = GetSystemMetrics( SM_CYMENU ) - GetSystemMetrics( SM_CYMENUSIZE );
    
    unsigned style = window.resizable() ? ResizableStyle : FixedStyle;
    if(window.fullScreen()) style = 0;

    RECT rc = {0, 0, 0, 0};
    
    // menu dimension is supported for one row only.
    // hence we don't use it here
    AdjustWindowRect(&rc, style, false);
    
    MENUBARINFO mbi;
    mbi.cbSize = sizeof(MENUBARINFO);
    mbi.rcBar = {0, 0, 0, 0};
    unsigned menuHeight = 0;
    
    if (window.menuVisible() && GetMenuBarInfo(hwnd, OBJID_MENU, 0, &mbi)) {        
        menuHeight = mbi.rcBar.bottom - mbi.rcBar.top;
        menuHeight += menuWhatIsThis;
    }
            
    unsigned statusHeight = 0;
    if(window.statusVisible()) {
        RECT src;
        GetClientRect(hstatus, &src);
        statusHeight = src.bottom - src.top;
    }
    return {abs(rc.left), abs(rc.top) + menuHeight , (rc.right - rc.left), (rc.bottom - rc.top) + statusHeight + menuHeight };
}

auto pWindow::setGeometry(Geometry geometry) -> void {
    locked = true;
    Geometry margin = frameMargin();

    SetWindowPos(
        hwnd, NULL,
        geometry.x - margin.x, geometry.y - margin.y,
        geometry.width + margin.width, geometry.height + margin.height,
        SWP_NOZORDER | SWP_FRAMECHANGED
    );

    SetWindowPos(hstatus, NULL, 0, 0, 0, 0, SWP_NOZORDER | SWP_FRAMECHANGED);

    if(window.state.layout) {
        Geometry geom = this->geometry();
        geom.x = geom.y = 0;
        window.state.layout->setGeometry(geom);
    }
    locked = false;
}

auto pWindow::isOffscreen() -> bool {
    Geometry geo = window.state.geometry;
    Geometry margin = frameMargin();
    geo.x -= margin.x;
    geo.y -= margin.y;
    geo.width += margin.width;
    geo.height += margin.height;
    
    return pSystem::isOffscreen( geo );
}

auto pWindow::geometry() -> Geometry {
    Geometry margin = frameMargin();

    RECT rc;
    if(IsIconic(hwnd)) { //is minimized
        WINDOWPLACEMENT wp;
        GetWindowPlacement(hwnd, &wp);
        rc = wp.rcNormalPosition;
    } else {
        GetWindowRect(hwnd, &rc);
    }

    signed x = rc.left + margin.x;
    signed y = rc.top + margin.y;
    unsigned width = (rc.right - rc.left) - margin.width;
    unsigned height = (rc.bottom - rc.top) - margin.height;

    return {x, y, width, height};
}

auto pWindow::setFullScreen(bool fullScreen) -> void {
    if (!window.resizable()) return;
    locked = true;
    if(!fullScreen) {
        SetWindowLongPtr(hwnd, GWL_STYLE, WS_VISIBLE | (window.resizable() ? ResizableStyle : FixedStyle));
        setGeometry(window.state.geometry);
		SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    } else {
        HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
        MONITORINFOEX info;
        memset(&info, 0, sizeof(MONITORINFOEX));
        info.cbSize = sizeof(MONITORINFOEX);
        GetMonitorInfo(monitor, &info);
        RECT rc = info.rcMonitor;
        Geometry geometry = {rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top};
        SetWindowLongPtr(hwnd, GWL_STYLE, WS_VISIBLE | WS_POPUP);
        Geometry margin = frameMargin();
        setGeometry({
            geometry.x + margin.x, geometry.y + margin.y,
            geometry.width - margin.width, geometry.height - margin.height
        });
    }
    locked = false;
    if(window.onSize) window.onSize();
}

auto pWindow::onDrop(WPARAM wparam) -> void {
    std::vector<std::string> paths = getDropPaths(wparam);
    if(paths.empty()) return;
    if(window.onDrop) window.onDrop(paths);
}

auto pWindow::onEraseBackground() -> bool {
    if(bgUpdateState == 2) return true;
    if(brush == 0) return false;
    RECT rc;
    GetClientRect(hwnd, &rc);
    PAINTSTRUCT ps;
    BeginPaint(hwnd, &ps);
    FillRect(ps.hdc, &rc, brush);
    EndPaint(hwnd, &ps);
    return true;
}

auto pWindow::onClose() -> void {
    if(window.onClose) window.onClose();
    else window.setVisible(false);
}

void pWindow::onMove() {
    if(locked || window.fullScreen()) return;

    Geometry windowGeometry = geometry();
    window.state.geometry.x = windowGeometry.x;
    window.state.geometry.y = windowGeometry.y;

    if(window.onMove) window.onMove();
}

auto pWindow::onSize() -> void {
    if(locked || window.fullScreen()) return;

    SetWindowPos(hstatus, NULL, 0, 0, 0, 0, SWP_NOZORDER | SWP_FRAMECHANGED);

    Geometry windowGeometry = geometry();
    window.state.geometry.width = windowGeometry.width;
    window.state.geometry.height = windowGeometry.height;

    if(window.state.layout) {
        Geometry geom = geometry();
        geom.x = geom.y = 0;
        window.state.layout->setGeometry(geom);
    }

    if(window.onSize) window.onSize();
}

auto pWindow::append(Menu& menu) -> void {
    updateMenu();
}

auto pWindow::remove(Menu& menu) -> void {
    updateMenu();
}

auto pWindow::append(Widget& widget) -> void {
    widget.p.rebuild();
}

auto pWindow::remove(Widget& widget) -> void {
    widget.p.destroy();
}

auto pWindow::append(Layout& layout) -> void {
    Geometry geom = window.state.geometry;
    geom.x = geom.y = 0;
    layout.setGeometry(geom);
}

auto pWindow::updateMenu() -> void {
    if(hmenu) DestroyMenu(hmenu);
    hmenu = CreateMenu();
	if(contextmenu) DestroyMenu(contextmenu);
    contextmenu = CreatePopupMenu();

    for(auto& menu : window.state.menus) {
        menu->p.update(window);
        if(menu->visible()) {
			unsigned enabled = menu->enabled() ? 0 : MF_GRAYED;
            AppendMenu(hmenu, MF_STRING | MF_POPUP | enabled, (UINT_PTR)menu->p.hmenu, utf16_t(menu->text()) );
			AppendMenu(contextmenu, MF_STRING | MF_POPUP | enabled, (UINT_PTR)menu->p.hmenu, utf16_t(menu->text()) );

			menu->p.setMenuItemInfo(contextmenu);
        }
    }
    SetMenu(hwnd, window.menuVisible() ? hmenu : 0);
}

auto pWindow::changeCursor( Image& image, unsigned hotSpotX, unsigned hotSpotY ) -> void {
        
    if (hCursor)
        DestroyCursor( hCursor );
    
    hCursor = nullptr;
    
    if(image.empty()) {
        
        setDefaultCursor();
                
    } else {        
        
        HBITMAP hBitmap = CreateBitmap( image );
    
        hCursor = CreateHCursor( hBitmap, hotSpotX, hotSpotY ); 
    }       
}

auto pWindow::setDefaultCursor() -> void {
    if (hCursor)
        DestroyCursor( hCursor );
        
    hCursor = LoadCursor(0, IDC_ARROW); 
}

auto pWindow::addCustomFont( CustomFont* customFont ) -> bool {
	
	return pFont::create( customFont->data, customFont->size ) != nullptr;
}

}
