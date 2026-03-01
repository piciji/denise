
auto pSquareCanvas::create() -> void {
    destroy(hwnd);
    destroy(hwndTip);
    
    hwnd = CreateWindow(WC_STATIC, L"",
        WS_CHILD | SS_OWNERDRAW,
        0, 0, 0, 0, getParentHandle(), (HMENU)(unsigned long long)squareCanvas.id, GetModuleHandle(0), 0);
    
    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)&squareCanvas);
    wndprocOrig = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)subclassWndProc);
}

pSquareCanvas::~pSquareCanvas() {
    if (hCursor)
        SetCursor(hCursor);
}

auto pSquareCanvas::rebuild() -> void {
    if(!needRebuild())
        return;
    
    create();
    InvalidateRect(hwnd, 0, false);
    pWidget::rebuild();
}

auto pSquareCanvas::setBackgroundColor( unsigned color ) -> void {
    if (hwnd)
        RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
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
            return 0;

        case WM_SETCURSOR:
            if (!squareCanvas->p.hCursor)
                squareCanvas->p.hCursor = LoadCursor(0, IDC_HAND);
            SetCursor(squareCanvas->p.hCursor);
            return 1;

        case WM_NCHITTEST:
            return HTCLIENT;

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

    return CallWindowProc(squareCanvas->p.wndprocOrig, hwnd, msg, wparam, lparam);
    //return pApplication::wndProc(squareCanvas->p.wndprocOrig, hwnd, msg, wparam, lparam);
}

auto pSquareCanvas::drawItem(LPDRAWITEMSTRUCT lDraw) -> void {
    auto hdc = lDraw->hDC;
    auto rc = lDraw->rcItem;

    HDC _hdc = CreateCompatibleDC(hdc);
    unsigned width = squareCanvas.geometry().width;
    unsigned height = squareCanvas.geometry().height;
    unsigned color = squareCanvas.backgroundColor();
    unsigned borderColor = squareCanvas.borderColor();
    unsigned borderSize = squareCanvas.borderSize();
    bool overrideBG = squareCanvas.overrideBackgroundColor();

    uint8_t r = (color >> 16) & 0xff;
    uint8_t g = (color >> 8) & 0xff;
    uint8_t b = (color >> 0) & 0xff;

    if (!overrideBG) {
        color = pApplication::useDark ? (unsigned)DARK_BG_COL : (unsigned)GetSysColor(COLOR_WINDOW);
        r = (color >> 16) & 0xff;
        g = (color >> 8) & 0xff;
        b = (color >> 0) & 0xff;
    }

    uint8_t bR = (borderColor >> 16) & 0xff;
    uint8_t bG = (borderColor >> 8) & 0xff;
    uint8_t bB = (borderColor >> 0) & 0xff;

    BITMAPINFO bmi{};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -((long)height);
    bmi.bmiHeader.biSizeImage = width * height * sizeof(uint32_t);
    void* bits = nullptr;
    HBITMAP bitmap = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);

    if (bits) {
        for (unsigned y = 0; y < height; y++) {
            bool borderYPixel = 0;
            if (y < borderSize)
                borderYPixel = 1;
            else if (y >= (height - borderSize))
                borderYPixel = 1;

            auto target = (uint8_t*)bits + y * width * sizeof(uint32_t);
            for (unsigned x = 0; x < width; x++) {
                bool borderXPixel = 0;

                if (!borderYPixel) {
                    if (x < borderSize)
                        borderXPixel = 1;
                    else if (x >= (width - borderSize))
                        borderXPixel = 1;
                }

                if (borderYPixel || borderXPixel) {
                    target[0] = bB;
                    target[1] = bG;
                    target[2] = bR;
                    target[3] = 0xff;
                } else {
                    target[0] = b;
                    target[1] = g;
                    target[2] = r;
                    target[3] = 0xff;
                }

                target += 4;
            }
        }
    }
    SelectObject(_hdc, bitmap);

    BLENDFUNCTION bf{ AC_SRC_OVER, 0, (BYTE)255, AC_SRC_ALPHA };
    AlphaBlend(hdc, 0, 0, width, height, _hdc, 0, 0, width, height, bf);

    DeleteObject(bitmap);
    DeleteDC(_hdc);
}