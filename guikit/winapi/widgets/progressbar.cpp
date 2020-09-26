
auto pProgressBar::minimumSize() -> Size {
	
	static Size containerSize = pWidget::getScaledContainerSize( {0, 23} );
	
    return containerSize;
}

auto pProgressBar::create() -> void {
    destroy(hwnd);
    destroy(hwndTip);
    
    hwnd = CreateWindow(PROGRESS_CLASS, L"",
        WS_CHILD | PBS_SMOOTH,
        0, 0, 0, 0, progressBar.window()->p.hwnd, (HMENU)(unsigned long long)progressBar.id, GetModuleHandle(0), 0);
    
    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)&progressBar);
    SendMessage(hwnd, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
    SendMessage(hwnd, PBM_SETSTEP, MAKEWPARAM(1, 0), 0);
}

auto pProgressBar::rebuild() -> void {
    if (hwnd)
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
