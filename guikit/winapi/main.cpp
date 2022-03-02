#include "main.h"
#include "../tools/crc32.h"
#include <mmsystem.h>

#ifdef _MSC_VER
#pragma warning(disable:4996)
#define PATH_MAX MAX_PATH
#endif

namespace GUIKIT {

#include "tools.cpp"
#include "menu.cpp"
#include "browserWindow.cpp"
#include "messageWindow.cpp"
#include "statusbar.cpp"
#include "clipboard.cpp"
#include "display.cpp"
    
#include "widgets/widget.cpp"   
#include "widgets/button.cpp"
#include "widgets/stepbutton.cpp"
#include "widgets/lineedit.cpp"
#include "widgets/multilineedit.cpp"
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
#include "widgets/imageView.cpp"
   
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
    timeEndPeriod(1);
    CoUninitialize();
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
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED); // same as CoInitialize(0)
    timeBeginPeriod(1);
    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
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

    wc.hbrBackground = NULL;
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
    wchar_t* path = new wchar_t[PATH_MAX];
    _wgetcwd(path, PATH_MAX);
    cwd = utf8_t(path);
    delete[] path;
    return cwd;
}

auto CALLBACK pApplication::wndProc(WNDPROC windowProc, HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT {
    Base* base = (Base*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if(base == nullptr) return DefWindowProc(hwnd, msg, wparam, lparam);

    Window& window = dynamic_cast<Window*>(base) ? (Window&)*base : *((Widget*)base)->window();

    switch(msg) {  
        case WM_CTLCOLOREDIT: {
            Base* base = (Base*)GetWindowLongPtr((HWND)lparam, GWLP_USERDATA);
            if(base == nullptr) break;

            HBRUSH brush = ((Widget*)base)->p.getColor( wparam );

            if (!brush)
                break;

            return (LRESULT)brush;
        }
        case WM_CTLCOLORBTN:
        case WM_CTLCOLORSTATIC: {
            Base* base = (Base*)GetWindowLongPtr((HWND)lparam, GWLP_USERDATA);
            if(base == nullptr) break;

            if (dynamic_cast<LineEdit*>(base) || dynamic_cast<MultilineEdit*>(base) || dynamic_cast<StepButton*>(base)) {
                
                HBRUSH brush = ((Widget*) base)->p.getColor(wparam);

                if (!brush)
                    return windowProc(hwnd, WM_CTLCOLOREDIT, wparam, lparam);

                return (LRESULT) brush;
            }
            
			TabFrameLayout* parentTabFrameLayout = ((Widget*)base)->p.parentTabFrameLayout;
            
            if (parentTabFrameLayout) {
                if (!IsAppThemed()) break;

                SetBkMode((HDC)(wparam), TRANSPARENT);
                return (INT_PTR)pTabFrame::getTabBackgroundForControl( parentTabFrameLayout->frameWidget->p.hwnd, ((Widget*)base)->p.hwnd);

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
            if(dynamic_cast<StepButton*>(base) && HIWORD(wparam) == EN_CHANGE) { ((StepButton*)base)->p.onChange(); return false; }
            if(dynamic_cast<MultilineEdit*>(base) && HIWORD(wparam) == EN_SETFOCUS) { ((MultilineEdit*)base)->p.onFocus(); return false; }
            if(dynamic_cast<MultilineEdit*>(base) && HIWORD(wparam) == EN_CHANGE) { ((MultilineEdit*)base)->p.onChange(); return false; }

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
            if(dynamic_cast<CheckBox*>(base) && ((LPNMHDR)lparam)->code == NM_CUSTOMDRAW) { return ((CheckBox*)base)->p.onCustomDraw(lparam); }
            if(dynamic_cast<TreeView*>(base) && ((LPNMHDR)lparam)->code == TVN_SELCHANGED) { ((TreeView*)base)->p.onChange(); break; }
            if(dynamic_cast<TreeView*>(base) && ((LPNMHDR)lparam)->code == TVN_ITEMEXPANDED) { ((TreeView*)base)->p.onExpanded(lparam); break; }
            if(dynamic_cast<TreeView*>(base) && ((LPNMHDR)lparam)->code == NM_DBLCLK) { ((TreeView*)base)->p.onActivate(); break; }
            if(dynamic_cast<TreeView*>(base) && ((LPNMHDR)lparam)->code == NM_RETURN) { ((TreeView*)base)->p.onActivate(); break; }
            if(dynamic_cast<StepButton*>(base) && ((LPNMHDR)lparam)->code == UDN_DELTAPOS) { ((StepButton*)base)->p.onStep(lparam); break; }
            if(dynamic_cast<TabFrameLayout::TabFrame*>(base) && ((LPNMHDR)lparam)->code == TCN_SELCHANGE) { ((TabFrameLayout::TabFrame*)base)->p.onChange(); break; }            
            if(dynamic_cast<Hyperlink*>(base) && ((LPNMHDR)lparam)->code == NM_CLICK) {
                PNMLINK pNMLink = (PNMLINK) lparam;
                LITEM item = pNMLink->item;
                ShellExecute(NULL, L"open", item.szUrl, NULL, NULL, SW_SHOW);
                break;
            }
            if(dynamic_cast<StatusBar*>(base) && ((LPNMHDR)lparam)->code == NM_CLICK) {
                ((StatusBar*)base)->p.onClick(lparam);
                break;
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
            
            else if ( (lpmis != NULL) && (lpmis->CtlType == ODT_LISTVIEW) ) {
                unsigned id = LOWORD(wparam);
                HWND control = GetDlgItem(hwnd, id);
                base = (Base*) GetWindowLongPtr(control, GWLP_USERDATA);
                if(dynamic_cast<ListView*>(base)) {
                    ((ListView*)base)->p.measureItem( lpmis );
                    return true;
                }
            }
			break;
		}
		case WM_DRAWITEM: {

            unsigned id = LOWORD(wparam);
            HWND control = GetDlgItem(hwnd, id);                        
			LPDRAWITEMSTRUCT lDraw = (LPDRAWITEMSTRUCT) lparam;
            base = (Base*) GetWindowLongPtr(control, GWLP_USERDATA);
            
			if ((lDraw != NULL) && (lDraw->CtlType == ODT_MENU)) {
				if (pMenuBase::drawItem(lDraw))
					return true;
			}

            else if ((lDraw != NULL) && (lDraw->CtlType == ODT_LISTVIEW)) {
                
                if (dynamic_cast<ListView*> (base)) {
                    ((ListView*) base)->p.drawItem(lDraw);
                    return true;
                }
            } else {
                if(dynamic_cast<StatusBar*>(base)) {
                    ((StatusBar*)base)->p.drawItem(wparam, lparam);
                    return true;
                }
                
            }
			break;
		}		
        case WM_ENTERIDLE: {
            MSG _msg;
            while(!PeekMessage(&_msg, 0, 0, 0, PM_NOREMOVE)) {
                if (Application::loop)
                    Application::loop();
            }            
            break;
        }
        case WM_DISPLAYCHANGE: {
            pMonitor::fetchDisplays();

            if (Application::onDisplayChange)
                Application::onDisplayChange();
        }
        break;
    }
    return windowProc(hwnd, msg, wparam, lparam);
}

//window
pWindow::pWindow(Window& window, Window::Hints hints) : window(window) {
    locked = false;
    brush = 0;
	preventRedraw = false;
    hCursor = LoadCursor(0, IDC_ARROW);
    timerStatusUpdate.setInterval(100);
    timerStatusUpdate.onFinished = [this]() {
        timerStatusUpdate.setEnabled(false);
        if (this->window.statusBar())
            this->window.statusBar()->p.updatePosition();
    };
	
	if (hints == Window::Hints::Video) {
		Timer startUp;
		startUp.setInterval(1000);
		startUp.onFinished = [this]() {
			this->preventRedraw = true;
		};
	}

    Geometry geo = window.state.geometry;

	hwnd = CreateWindow( L"app_gui", L"", ResizableStyle | WS_CLIPCHILDREN, geo.x, geo.y, geo.width, geo.height, 0, 0, GetModuleHandle(0), 0);

    hmenu = CreateMenu();
	contextmenu = CreatePopupMenu();        

    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)&window);
    setDroppable(window.state.droppable);
}

pWindow::~pWindow() {
    DestroyMenu(hmenu);
	DestroyMenu(contextmenu);
    DestroyWindow(hwnd);
}

auto pWindow::handle() -> uintptr_t {
    return (uintptr_t)hwnd;
}

auto pWindow::tellMeShouldICreateTheUIRightAway() -> bool {
	return !IsAppThemed() || (pApplication::version <= WindowsXP);
}

auto CALLBACK pWindow::wndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT {
    if(Application::isQuit) return DefWindowProc(hwnd, msg, wparam, lparam);

    Base* base = (Base*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if(base == nullptr) return DefWindowProc(hwnd, msg, wparam, lparam);

    Window& window = dynamic_cast<Window*>(base) ? (Window&)*base : *((Widget*)base)->window();

    switch(msg) {
        case WM_CLOSE: window.p.onClose(); return true;
        case WM_MOVE: window.p.onMove(); break;
        case WM_SIZE: window.p.onSize( wparam ); break;
        case WM_SIZING:
            window.p.onSizing((int)wparam, *((LPRECT)lparam));
            break;
        case WM_DROPFILES: window.p.onDrop(wparam); return false;
		case WM_ENTERMENULOOP:
			if(window.winapi.onMenu) window.winapi.onMenu();
			break;
        case WM_ENTERSIZEMOVE:
            if (window.onResizeStart && !window.fullScreen())
                window.onResizeStart();

            return 0;
        case WM_EXITSIZEMOVE:

            if (window.onResizeEnd && !window.fullScreen())
                window.onResizeEnd();
            return 0;
        case WM_PAINT:

            break;
            
        case WM_ERASEBKGND: {
            if (!window.fullScreen() && window.p.preventRedraw)
                return 0;

            if(window.p.onEraseBackground())
                return 1;

            // use theme color
            break;
        } 
        case WM_ACTIVATE:
			if ((LOWORD(wparam) == WA_ACTIVE) && (LOWORD(wparam) != WA_CLICKACTIVE)) {
                if (window.statusBar())
                    window.statusBar()->p.updatePosition();

				if (window.onUnminimize)
                    window.onUnminimize();
                                    
			} else if (LOWORD(wparam) == WA_INACTIVE) {                
                if(window.onMinimize)
                    window.onMinimize();
			}
			break;
		case WM_ACTIVATEAPP:
			if (LOWORD(wparam) == WA_INACTIVE) {
				if(window.fullScreen())
                    ShowWindow( hwnd, SW_MINIMIZE );
			}
			break;
        case WM_SETFOCUS:
            if (window.fullScreen() && window.statusBar())
                window.statusBar()->p.updatePosition();

            if (window.onFocus)
                window.onFocus();
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
    if (window.statusBar())
        window.statusBar()->p.update();
}

auto pWindow::restore() -> void {
    ShowWindow(hwnd, SW_RESTORE);
}

auto pWindow::setResizable(bool resizable) -> void {
    SetWindowLongPtr(hwnd, GWL_STYLE, resizable ? ResizableStyle : FixedStyle);
    if( !window.fullScreen() ) setGeometry(window.state.geometry);
}

auto pWindow::setTitle(std::string text) -> void {
    SetWindowText(hwnd, utf16_t(text));
}

auto pWindow::setMenuVisible(bool visible) -> void {
    locked = true;
    SetMenu(hwnd, visible ? hmenu : 0);
    setGeometry( !window.fullScreen() ? window.state.geometry : geometry() );
    locked = false;
}

auto pWindow::setStatusVisible(bool visible) -> void {
    if (!window.statusBar())
        return;
    
    locked = true;
    window.statusBar()->p.setStatusVisible( visible );
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

auto pWindow::minimized() -> bool {
    
    return IsIconic( hwnd );
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
    if(window.statusBar() && window.statusVisible())
        statusHeight = window.statusBar()->p.getHeight();
    
    return {(signed)(abs(rc.left)), (signed)(abs(rc.top) + menuHeight), (unsigned)(rc.right - rc.left), (unsigned)((rc.bottom - rc.top) + statusHeight + menuHeight) };
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

    if (window.statusBar())
        window.statusBar()->p.updatePosition();

    if(window.state.layout) {
        window.state.layout->resetSynchronisation();
        Geometry geom = this->geometry();
        geom.x = geom.y = 0;
        window.state.layout->setGeometry(geom);
    }
    locked = false;
}

auto pWindow::setForeground() -> void {
    SetForegroundWindow(hwnd);
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
        SetWindowLongPtr(hwnd, GWL_STYLE, WS_VISIBLE | WS_CLIPCHILDREN | (window.resizable() ? ResizableStyle : FixedStyle));
        setGeometry(window.state.geometry);
		SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        pMonitor::resetSetting();
    } else {
        if (window.fullscreenSetting.inUse)
            pMonitor::setSetting( window.fullscreenSetting.displayId, window.fullscreenSetting.settingId );

        HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
        MONITORINFOEX info;
        memset(&info, 0, sizeof(MONITORINFOEX));
        info.cbSize = sizeof(MONITORINFOEX);
        GetMonitorInfo(monitor, &info);
        RECT rc = info.rcMonitor;
        Geometry geometry = {(signed)rc.left, (signed)rc.top, (unsigned)(rc.right - rc.left), (unsigned)(rc.bottom - rc.top) };
        SetWindowLongPtr(hwnd, GWL_STYLE, WS_VISIBLE | WS_POPUP);
        Geometry margin = frameMargin();
        setGeometry({
            geometry.x + margin.x, geometry.y + margin.y,
            geometry.width - margin.width, geometry.height - margin.height
        });
    }
    locked = false;
    if(window.onSize) window.onSize(Window::SIZE_MODE::Default);
}

auto pWindow::onDrop(WPARAM wparam) -> void {
    std::vector<std::string> paths = getDropPaths(wparam);
    if(paths.empty()) return;
    if(window.onDrop) window.onDrop(paths);
}

auto pWindow::onEraseBackground() -> bool {
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

    timerStatusUpdate.setEnabled();

    Geometry windowGeometry = geometry();
    window.state.geometry.x = windowGeometry.x;
    window.state.geometry.y = windowGeometry.y;

    if(window.onMove) window.onMove();
}

auto pWindow::onSize(WPARAM wparam) -> void {
    if(locked || window.fullScreen()) return;

    Geometry windowGeometry = geometry();
    window.state.geometry.width = windowGeometry.width;
    window.state.geometry.height = windowGeometry.height;

    if(window.state.layout) {
        window.state.layout->resetSynchronisation();
        Geometry geom = geometry();
        geom.x = geom.y = 0;
        window.state.layout->setGeometry(geom);
    }

    if (window.statusBar())
        window.statusBar()->p.updatePosition();

    Window::SIZE_MODE sMode = Window::SIZE_MODE::Default;
    if (wparam == SIZE_MINIMIZED)
        sMode = Window::SIZE_MODE::Minimized;
    else if (wparam == SIZE_MAXIMIZED)
        sMode = Window::SIZE_MODE::Maximized;

    if(window.onSize) window.onSize( sMode );
}

auto pWindow::onSizing(int edge, RECT &rect) -> void {
	timerStatusUpdate.setEnabled();	
	
    if (window.fullScreen() || (window.aspectRatio().width == 0))
        return;

    int frameX = GetSystemMetrics(SM_CXSIZEFRAME) * 2;
    int frameY = GetSystemMetrics(SM_CYSIZEFRAME) * 2 + /*GetSystemMetrics(SM_CYMENU) +*/ GetSystemMetrics(SM_CYCAPTION);

    if(window.statusBar() && window.statusVisible())
        frameY += window.statusBar()->p.getHeight();

    MENUBARINFO mbi;
    mbi.cbSize = sizeof(MENUBARINFO);
    mbi.rcBar = {0, 0, 0, 0};

    if (window.menuVisible() && GetMenuBarInfo(hwnd, OBJID_MENU, 0, &mbi)) {
        frameY += mbi.rcBar.bottom - mbi.rcBar.top;
        frameY += GetSystemMetrics( SM_CYMENU ) - GetSystemMetrics( SM_CYMENUSIZE );
    }

    float sizeX = (float)((rect.right - rect.left) - frameX);
    float sizeY = (float)((rect.bottom - rect.top) - frameY);

    float ratioX = (float)window.aspectRatio().width;
    float ratioY = (float)window.aspectRatio().height;

    switch (edge) {
        case WMSZ_BOTTOM:
        case WMSZ_TOP: {
            int size_x = frameX + (int)((sizeY * ratioX) / ratioY + 0.5);

            rect.left = (rect.left + rect.right) / 2 - size_x / 2;
            rect.right = rect.left + size_x;
        } break;
        case WMSZ_BOTTOMLEFT: {
            int size_x, size_y;

            if (sizeX * ratioY > sizeY * ratioX) {
                size_x = rect.right - rect.left;
                size_y = frameY + (int)(((float)(size_x - frameX) * ratioY) / ratioX + 0.5);
            } else {
                size_y = rect.bottom - rect.top;
                size_x = frameX + (int)(((float)(size_y - frameY) * ratioX) / ratioY + 0.5);
            }

            rect.left = rect.right - size_x;
            rect.bottom = rect.top + size_y;
        } break;
        case WMSZ_BOTTOMRIGHT: {
            int size_x, size_y;

            if (sizeX * ratioY > sizeY * ratioX) {
                size_x = rect.right - rect.left;
                size_y = frameY + (int)(((float)(size_x - frameX) * ratioY) / ratioX + 0.5);

                rect.right = rect.left + size_x;
                rect.bottom = rect.top + size_y;
            } else {
                size_y = rect.bottom - rect.top;
                size_x = frameX + (int)(((float)(size_y - frameY) * ratioX) / ratioY + 0.5);
            }

            rect.right = rect.left + size_x;
            rect.bottom = rect.top + size_y;
        } break;
        case WMSZ_LEFT:
        case WMSZ_RIGHT: {
            int size_y = frameY + (int)((sizeX * ratioY) / ratioX + 0.5);
            rect.top = (rect.top + rect.bottom) / 2 - size_y / 2;
            rect.bottom = rect.top + size_y;
        } break;
        case WMSZ_TOPLEFT: {
            int size_x, size_y;

            if (sizeX * ratioY > sizeY * ratioX) {
                size_x = rect.right - rect.left;
                size_y = frameY + (int)(((float)(size_x - frameX) * ratioY) / ratioX + 0.5);
            } else {
                size_y = rect.bottom - rect.top;
                size_x = frameX + (int)(((float)(size_y - frameY) * ratioX) / ratioY + 0.5);
            }

            rect.left = rect.right - size_x;
            rect.top = rect.bottom - size_y;
        } break;
        case WMSZ_TOPRIGHT: {
            int size_x, size_y;

            if (sizeX * ratioY > sizeY * ratioX) {
                size_x = rect.right - rect.left;
                size_y = frameY + (int)(((float)(size_x - frameX) * ratioY) / ratioX + 0.5);
            } else {
                size_y = rect.bottom - rect.top;
                size_x = frameX + (int)(((float)(size_y - frameY) * ratioX) / ratioY + 0.5);
            }

            rect.right = rect.left + size_x;
            rect.top = rect.bottom - size_y;
        } break;
    }
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

auto pWindow::append(StatusBar& statusBar) -> void {
    statusBar.p.create();
}

auto pWindow::remove(StatusBar& statusBar) -> void {
    statusBar.p.destroy();
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
        menu->p.update(&window);
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

auto pWindow::setPointerCursor() -> void {
    if (hCursor)
        DestroyCursor( hCursor );
        
    hCursor = LoadCursor(0, IDC_HAND); 
}

auto pWindow::addCustomFont( CustomFont* customFont ) -> bool {
	
	return pFont::create( customFont->data, customFont->size ) != nullptr;
}

}
