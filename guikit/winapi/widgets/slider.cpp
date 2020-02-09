
auto pSlider::minimumSize() -> Size {
	
	static Size containerSize = pWidget::getScaledContainerSize( {25, 25} );	
	
    if (slider.orientation == Slider::Orientation::VERTICAL)
        return {containerSize.width, 0};
        
    return {0, containerSize.height};
}

auto pSlider::setGeometry(Geometry geometry) -> void {
    
    if (slider.orientation == Slider::Orientation::VERTICAL) {
        pWidget::setGeometry({geometry.x, geometry.y - 7,geometry.width, geometry.height + 14});
        
    } else {
        pWidget::setGeometry({geometry.x - 7, geometry.y, geometry.width + 14, geometry.height});
    }
}

auto pSlider::setLength(unsigned length) -> void {
    if(!hwnd)
        return;
    
    length += (length == 0);
    SendMessage(hwnd, TBM_SETRANGE, (WPARAM)true, (LPARAM)MAKELONG(0, length - 1));
    SendMessage(hwnd, TBM_SETPAGESIZE, 0, (LPARAM)(length >> 3));
    slider.setPosition(0);
}

auto pSlider::setPosition(unsigned position) -> void {
    if(hwnd)
        SendMessage(hwnd, TBM_SETPOS, (WPARAM)true, (LPARAM)position);
}

auto pSlider::create() -> void {
    destroy(hwnd);
    destroy(hwndTip);
    
    hwnd = CreateWindow(
        TRACKBAR_CLASS, L"", WS_CHILD | WS_TABSTOP | TBS_NOTICKS | TBS_BOTH |
        (slider.orientation == Slider::Orientation::VERTICAL ? TBS_VERT : TBS_HORZ),
        0, 0, 0, 0, slider.window()->p.hwnd, (HMENU)(unsigned long long)slider.id, GetModuleHandle(0), 0);

    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)&slider);
}

auto pSlider::rebuild() -> void {
    create();
    setFont( widget.font() );
    unsigned position = slider.state.position;
    setLength(slider.state.length);
    slider.setPosition(position);
    pWidget::rebuild();
}

auto pSlider::onChange() -> void {
    unsigned position = SendMessage(hwnd, TBM_GETPOS, 0, 0);
    if(position == slider.state.position)
        return;
    
    slider.state.position = position;
    
    if(slider.onChange)
        slider.onChange();
}
