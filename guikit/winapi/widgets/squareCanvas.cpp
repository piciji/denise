
auto pSquareCanvas::create() -> void {
    destroy(hwnd);
    destroy(hwndTip);
    hwnd = CreateWindow(L"STATIC", L"",
        WS_CHILD,
        0, 0, 0, 0, squareCanvas.window()->p.hwnd, (HMENU)(unsigned long long)squareCanvas.id, GetModuleHandle(0), 0);
    
    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)&squareCanvas);
    wndprocOrig = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)subclassWndProc);
}

auto pSquareCanvas::rebuild() -> void {
    if (hwnd)
        return;
    
    create();
    InvalidateRect(hwnd, 0, false);
    pWidget::rebuild();
}

auto pSquareCanvas::setBackgroundColor( unsigned color ) -> void {
    if (hwnd)
        InvalidateRect(hwnd, 0, false);
}

auto pSquareCanvas::setBorderColor(unsigned borderSize, unsigned borderColor) -> void {
    if (hwnd)
        InvalidateRect(hwnd, 0, false);    
}

auto CALLBACK pSquareCanvas::subclassWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT {
    SquareCanvas* squareCanvas = (SquareCanvas*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if(squareCanvas == nullptr)
        return DefWindowProc(hwnd, msg, wparam, lparam);
    
    Window* window = (Window*)squareCanvas->Sizable::state.window;
    
    if(window == nullptr)
        return DefWindowProc(hwnd, msg, wparam, lparam);
        
    switch(msg) {
        case WM_GETDLGCODE: return DLGC_STATIC | DLGC_WANTCHARS;
        case WM_ERASEBKGND: 
        case WM_PAINT:
        {
            unsigned width = squareCanvas->Widget::state.geometry.width;
            unsigned height = squareCanvas->Widget::state.geometry.height;
            unsigned color = squareCanvas->Widget::state.backgroundColor;
            unsigned borderColor = squareCanvas->state.borderColor;
            unsigned borderSize = squareCanvas->state.borderSize;
            
            uint8_t r = (color >> 16) & 0xff;
            uint8_t g = (color >> 8) & 0xff;
            uint8_t b = (color >> 0) & 0xff;

            uint8_t bR = (borderColor >> 16) & 0xff;
            uint8_t bG = (borderColor >> 8) & 0xff;
            uint8_t bB = (borderColor >> 0) & 0xff;
            
            PAINTSTRUCT ps;
            BeginPaint(hwnd, &ps);
            HDC hdc = CreateCompatibleDC(ps.hdc);
            BITMAPINFO bmi{};
            bmi.bmiHeader.biSize = sizeof (BITMAPINFOHEADER);
            bmi.bmiHeader.biPlanes = 1;
            bmi.bmiHeader.biBitCount = 32;
            bmi.bmiHeader.biCompression = BI_RGB;
            bmi.bmiHeader.biWidth = width;
            bmi.bmiHeader.biHeight = -height;
            bmi.bmiHeader.biSizeImage = width * height * sizeof (uint32_t);
            void* bits = nullptr;
            HBITMAP bitmap = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
            if (bits) {                
                
                for (unsigned y = 0; y < height; y++) {
                    bool borderYPixel = 0;
                    if (y < borderSize)
                        borderYPixel = 1;
                    else if (y >= (height - borderSize) )
                        borderYPixel = 1;
                    
                    auto target = (uint8_t*) bits + y * width * sizeof (uint32_t);
                    for (unsigned x = 0; x < width; x++) {
                        bool borderXPixel = 0;
                        
                        if (!borderYPixel) {                            
                            if (x < borderSize)
                                borderXPixel = 1;
                            else if (x >= (width - borderSize))
                                borderXPixel = 1;
                        }
                        
                        target[0] = (borderYPixel || borderXPixel) ? bB : b;
                        target[1] = (borderYPixel || borderXPixel) ? bG : g;
                        target[2] = (borderYPixel || borderXPixel) ? bR : r;
                        target[3] = 0xff;
                        target += 4;
                    }
                }
            }
            SelectObject(hdc, bitmap);

            RECT rc;
            GetClientRect(hwnd, &rc);
            DrawThemeParentBackground(hwnd, ps.hdc, &rc);

            BLENDFUNCTION bf{AC_SRC_OVER, 0, (BYTE) 255, AC_SRC_ALPHA};
            AlphaBlend(ps.hdc, 0, 0, width, height, hdc, 0, 0, width, height, bf);

            DeleteObject(bitmap);
            DeleteDC(hdc);

            EndPaint(hwnd, &ps);
            
            return msg == WM_ERASEBKGND;
        }
        case WM_LBUTTONDOWN:
            squareCanvas->onMousePress(Mouse::Button::Left); break;
        case WM_MBUTTONDOWN:
            squareCanvas->onMousePress(Mouse::Button::Middle); break;
        case WM_RBUTTONDOWN:
            squareCanvas->onMousePress(Mouse::Button::Right); break;
        case WM_LBUTTONUP:
            squareCanvas->onMouseRelease(Mouse::Button::Left); break;
        case WM_MBUTTONUP:
            squareCanvas->onMouseRelease(Mouse::Button::Middle); break;
        case WM_RBUTTONUP:
            squareCanvas->onMouseRelease(Mouse::Button::Right); break;
    }
    
    return DefWindowProc(hwnd, msg, wparam, lparam);
}