
auto pViewport::create() -> void {
    destroy(hwnd);
    destroy(hwndTip);    
    
    //SetWindowLong(viewport.window()->p.hwnd, GWL_STYLE, (GetWindowLong(viewport.window()->p.hwnd, GWL_STYLE) & ~WS_CLIPCHILDREN));
    
    hwnd = CreateWindow(L"app_viewport", L"",
        WS_CHILD | WS_DISABLED,
        0, 0, 0, 0, getParentHandle(), (HMENU)(unsigned long long)viewport.id, GetModuleHandle(0), 0);

    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)&viewport);

    hideTimer.onFinished = [this]() {
        hideTimer.setEnabled(false);
        if (viewport.window() && (viewport.window()->cursor == Window::Cursor::Default) && !viewport.window()->fullScreen())
            SetCursor(NULL);        
    };
}

auto pViewport::rebuild() -> void {
    if(!needRebuild())
        return;

    _rebuild();
}

inline auto pViewport::_rebuild() -> void {
    create();
    setDroppable(viewport.droppable());
    pWidget::rebuild();
}

auto pViewport::setDroppable(bool droppable) -> void {
    if (hwnd) {
        if (droppable)  RegisterDragDrop(hwnd, &dropManager);
        else            RevokeDragDrop(hwnd);
    }
}

auto pViewport::handle(bool hintRecreation) -> uintptr_t {
    if (hintRecreation) {
        _rebuild();
        setGeometry(viewport.geometry());
        setVisible(true);
    }

    return (uintptr_t)hwnd;
}

auto pViewport::callDrops(std::vector<std::string>& paths) -> void {
    if(!paths.empty() && viewport.onDrop) {
        viewport.onDrop(paths);
    }
}

auto pViewport::callDragEnter(std::vector<std::string>& paths) -> void {
    if(!paths.empty() && viewport.onDragEnter) {
        viewport.onDragEnter(paths);
    }
}

auto pViewport::callDragLeave() -> void {
    if( viewport.onDragLeave) {
        viewport.onDragLeave();
    }
}

auto pViewport::callDragMove(POINTL ptl) -> void {
    if( viewport.onDragMove && hwnd) {
        POINT pt;
        pt.x = ptl.x;
        pt.y = ptl.y;
        ScreenToClient(hwnd, &pt);
        viewport.onDragMove(pt.x, pt.y);
    }
}

auto pViewport::hideCursorByInactivity(unsigned delayMS) -> void {
    hideTimer.setInterval(delayMS);    
}

auto CALLBACK pViewport::wndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT {
    Base* base = (Base*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if(base == nullptr) return DefWindowProc(hwnd, msg, wparam, lparam);
    if(!dynamic_cast<Viewport*>(base)) return DefWindowProc(hwnd, msg, wparam, lparam);
    Viewport& viewport = (Viewport&)*base;

    switch(msg) {
        //case WM_ERASEBKGND: 
          //  return 0;
        case WM_GETDLGCODE:
            return DLGC_STATIC | DLGC_WANTCHARS;
            
        case WM_MOUSEMOVE: {
            TRACKMOUSEEVENT tracker = {sizeof (TRACKMOUSEEVENT), TME_LEAVE, hwnd};
            TrackMouseEvent(&tracker);
            viewport.state.mousePos = {(signed)LOWORD(lparam), (signed)HIWORD(lparam)};

            if (Application::isQuit)
                break;

            if (viewport.onMouseMove)
                viewport.onMouseMove(viewport.state.mousePos);

            auto& hideTimer = viewport.p.hideTimer;
            if (viewport.window() && !viewport.window()->fullScreen() && hideTimer.interval())
                hideTimer.setEnabled();
        } break;
        
        case WM_MOUSELEAVE:
            viewport.p.hideTimer.setEnabled(false);
            if(!Application::isQuit && viewport.onMouseLeave)
                viewport.onMouseLeave();
            break;
            
        case WM_LBUTTONDOWN:
            if (!Application::isQuit && viewport.onMousePress) 
                viewport.onMousePress(Mouse::Button::Left);
            break;
                
        case WM_MBUTTONDOWN:
            if (!Application::isQuit && viewport.onMousePress) 
                viewport.onMousePress(Mouse::Button::Middle);            
            break;
        case WM_RBUTTONDOWN:
            if (!Application::isQuit && viewport.onMousePress) 
                viewport.onMousePress(Mouse::Button::Right);
            break;
            
        case WM_LBUTTONUP:
            if(!Application::isQuit && viewport.onMouseRelease)
                viewport.onMouseRelease(Mouse::Button::Left);
            break;
        case WM_MBUTTONUP:
            if(!Application::isQuit && viewport.onMouseRelease)
                viewport.onMouseRelease(Mouse::Button::Middle);
            break;
        case WM_RBUTTONUP:
            if(!Application::isQuit && viewport.onMouseRelease)
                viewport.onMouseRelease(Mouse::Button::Right);
            break;
    }

    return DefWindowProc(hwnd, msg, wparam, lparam);
}

