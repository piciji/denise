
auto pViewport::create() -> void {
    destroy(hwnd);
    destroy(hwndTip);    
    
    SetWindowLong(viewport.window()->p.hwnd, GWL_STYLE, (GetWindowLong(viewport.window()->p.hwnd, GWL_STYLE) & ~WS_CLIPCHILDREN));
    
    hwnd = CreateWindow(L"app_viewport", L"",
        WS_CHILD | WS_DISABLED,
        0, 0, 0, 0, getParentHandle(), (HMENU)(unsigned long long)viewport.id, GetModuleHandle(0), 0);

    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)&viewport);
}

auto pViewport::rebuild() -> void {
    if(!needRebuild())
        return;
    
    create();
    setDroppable(viewport.droppable());
    pWidget::rebuild();
}

auto pViewport::setDroppable(bool droppable) -> void {
    if (hwnd)
        DragAcceptFiles(hwnd, droppable);
}

auto pViewport::handle() -> uintptr_t {
    return (uintptr_t)hwnd;
}

auto CALLBACK pViewport::wndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT {
    Base* base = (Base*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if(base == nullptr) return DefWindowProc(hwnd, msg, wparam, lparam);
    if(!dynamic_cast<Viewport*>(base)) return DefWindowProc(hwnd, msg, wparam, lparam);
    Viewport& viewport = (Viewport&)*base;

    switch(msg) {
        //case WM_ERASEBKGND: 
          //  return 0;
        case WM_DROPFILES: {
            std::vector<std::string> paths = getDropPaths(wparam);
            if(!paths.empty() && viewport.onDrop) {
                viewport.onDrop(paths);
            }
            return false;  
        }
        case WM_GETDLGCODE:
            return DLGC_STATIC | DLGC_WANTCHARS;
            
        case WM_MOUSEMOVE: {
            TRACKMOUSEEVENT tracker = {sizeof (TRACKMOUSEEVENT), TME_LEAVE, hwnd};
            TrackMouseEvent(&tracker);
            viewport.state.mousePos = {(signed)LOWORD(lparam), (signed)HIWORD(lparam)};

            if (viewport.onMouseMove)
                viewport.onMouseMove(viewport.state.mousePos);
        } break;
        
        case WM_MOUSELEAVE:
            if(viewport.onMouseLeave) viewport.onMouseLeave();
            break;
            
        case WM_LBUTTONDOWN:
            if (viewport.onMousePress) 
                viewport.onMousePress(Mouse::Button::Left);
            break;
                
        case WM_MBUTTONDOWN:
            if (viewport.onMousePress) 
                viewport.onMousePress(Mouse::Button::Middle);            
            break;
        case WM_RBUTTONDOWN:
            if (viewport.onMousePress) 
                viewport.onMousePress(Mouse::Button::Right);
            break;
            
        case WM_LBUTTONUP:
            if(viewport.onMouseRelease)
                viewport.onMouseRelease(Mouse::Button::Left);
            break;
        case WM_MBUTTONUP:
            if(viewport.onMouseRelease)
                viewport.onMouseRelease(Mouse::Button::Middle);
            break;
        case WM_RBUTTONUP:
            if(viewport.onMouseRelease)
                viewport.onMouseRelease(Mouse::Button::Right);
            break;
    }

    return DefWindowProc(hwnd, msg, wparam, lparam);
}

