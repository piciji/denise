
auto pProgressBar::minimumSize() -> Size {
	
	static Size containerSize = pWidget::getScaledContainerSize( {0, 23} );
	
    return containerSize;
}

auto pProgressBar::create() -> void {
    destroy(hwnd);
    destroy(hwndTip);
    
    hwnd = CreateWindow(PROGRESS_CLASS, L"",
        WS_CHILD | PBS_SMOOTH,
        0, 0, 0, 0, getParentHandle(), (HMENU)(unsigned long long)progressBar.id, GetModuleHandle(0), 0);
    
    if (pApplication::useDark) {
        SetWindowTheme(hwnd, L"", L"");
        SendMessage(hwnd, PBM_SETBKCOLOR, 0, DARK_BG_COL);
    }

    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)&progressBar);
    SendMessage(hwnd, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
    SendMessage(hwnd, PBM_SETSTEP, MAKEWPARAM(1, 0), 0);
    
    wndprocOrig = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)subclassWndProc);    
}

auto CALLBACK pProgressBar::subclassWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT {
    ProgressBar* progressBar = (ProgressBar*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if(progressBar == nullptr) return DefWindowProc(hwnd, msg, wparam, lparam);

    switch(msg) {   
        case WM_ERASEBKGND:
            return 0;
    }
    
    return CallWindowProc(progressBar->p.wndprocOrig, hwnd, msg, wparam, lparam);
}

auto pProgressBar::rebuild() -> void {
    if(!needRebuild())
        return;
    
    create();
    setFont( widget.font() );
    setPosition(progressBar.position());
    pWidget::rebuild();
}

auto pProgressBar::setPosition(unsigned position) -> void {
    if(hwnd)
        SendMessage(hwnd, PBM_SETPOS, (WPARAM)position, 0);
}

auto pProgressBar::setPositionThreaded(unsigned position) -> void {
    if(hwnd)
        PostMessage(hwnd, PBM_SETPOS, (WPARAM)position, 0);
}
