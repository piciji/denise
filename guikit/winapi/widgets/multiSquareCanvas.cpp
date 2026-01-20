
#define SCROLLBAR_WIDTH 20

pMultiSquareCanvas::~pMultiSquareCanvas() {
    delete[] drawArea;
}

auto pMultiSquareCanvas::create() -> void {
    destroy(hwnd);
    destroy(hwndTip);

    hwnd = CreateWindow(WC_STATIC, L"",
        WS_CHILD | SS_OWNERDRAW,
        0, 0, 0, 0, getParentHandle(), (HMENU)(unsigned long long)multiSquareCanvas.id, GetModuleHandle(0), 0);

    hwndScroller = CreateWindow( WC_SCROLLBAR, L"",
        WS_CHILD | SBS_VERT,
        0, 0, 0, 0, getParentHandle(), (HMENU)(unsigned long long)multiSquareCanvas.id, GetModuleHandle(0), 0);

    if (!doubleBuffer)
        doubleBuffer = new DoubleBuffer();
    doubleBuffer->release();

    if (pApplication::useDark) {
        SetWindowTheme(hwndScroller, L"Explorer", NULL);
        pApplication::pAllowDarkModeForWindow(hwndScroller, true);
        SendMessageW(hwndScroller, WM_THEMECHANGED, 0, 0);
    }

    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)&multiSquareCanvas);
    SetWindowLongPtr(hwndScroller, GWLP_USERDATA, (LONG_PTR)&multiSquareCanvas);
    wndprocOrig = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)subclassWndProc);
    wndprocOrigScroller = (WNDPROC)SetWindowLongPtr(hwndScroller, GWLP_WNDPROC, (LONG_PTR)subclassWndProcScroller);
}

auto pMultiSquareCanvas::setGeometry(Geometry geometry) -> void {
    if (!hwnd)
        return;

    Widget* parent = getParentTabWidget();

    if (parent) {
        auto geo = parent->geometry();

        geometry.x -= geo.x;
        geometry.y -= geo.y;
    }

    SetWindowPos(hwnd, NULL, geometry.x, geometry.y, geometry.width - SCROLLBAR_WIDTH, geometry.height, SWP_NOZORDER | SWP_NOCOPYBITS);

    SetWindowPos(hwndScroller, NULL, geometry.x + geometry.width - SCROLLBAR_WIDTH, geometry.y, SCROLLBAR_WIDTH, geometry.height, SWP_NOZORDER | SWP_NOCOPYBITS);

    updateScrollRange();
}

auto pMultiSquareCanvas::setVisible(bool visible) -> void {
    if(hwndScroller)
        ShowWindow(hwndScroller, visible ? SW_SHOWNORMAL : SW_HIDE);

    pWidget::setVisible(visible);
}

auto pMultiSquareCanvas::setEnabled(bool enabled) -> void {
    if(hwndScroller)
        EnableWindow(hwndScroller, enabled);

    pWidget::setEnabled(enabled);
}

auto pMultiSquareCanvas::rebuild() -> void {
    if(!needRebuild())
        return;

    delete[] drawArea;
    drawArea = nullptr;
    create();
    scrollPos = 0;
    InvalidateRect(hwnd, 0, false);

    if(hwndScroller)
        SetWindowPos(hwndScroller, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

    pWidget::rebuild();
}

auto pMultiSquareCanvas::setPadding(unsigned padding) -> void {
    update();
}

auto pMultiSquareCanvas::updateScrollRange() -> void {
    if (hwndScroller) {
        unsigned rows = multiSquareCanvas.rows();
        unsigned squareSize = multiSquareCanvas.squareSize();
        unsigned padding = multiSquareCanvas.padding();
        unsigned height = multiSquareCanvas.geometry().height;
        unsigned neededHeight = rows * (squareSize + padding);

        if (neededHeight > height) {
            neededHeight = neededHeight - height;
            ShowWindow(hwndScroller, SW_SHOWNORMAL);
        } else {
            ShowWindow(hwndScroller, SW_HIDE);
            neededHeight = 0;
        }

        SetScrollRange(hwndScroller, SB_CTL, 0, neededHeight, true);
    }
}

auto pMultiSquareCanvas::update() -> void {
    buildDrawArea();
    if (hwnd)
        //InvalidateRect(hwnd, 0, true);
        RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
}

auto CALLBACK pMultiSquareCanvas::subclassWndProcScroller(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT {
    MultiSquareCanvas* multiSquareCanvas = (MultiSquareCanvas*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if(multiSquareCanvas == nullptr)
        return DefWindowProc(hwnd, msg, wparam, lparam);

    Window* window = (Window*)multiSquareCanvas->Sizable::state.window;

    if(window == nullptr)
        return DefWindowProc(hwnd, msg, wparam, lparam);

    switch(msg) {
        case WM_GETDLGCODE: return DLGC_STATIC | DLGC_WANTCHARS;
        case WM_ERASEBKGND: {
            return 0;
        }
        case WM_NCHITTEST:
            return HTCLIENT;
        case WM_MOUSEWHEEL: {
            multiSquareCanvas->p.scroll(GET_WHEEL_DELTA_WPARAM(wparam));
        } break;
    }

    return CallWindowProc(multiSquareCanvas->p.wndprocOrigScroller, hwnd, msg, wparam, lparam);
}

auto CALLBACK pMultiSquareCanvas::subclassWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT {
    MultiSquareCanvas* multiSquareCanvas = (MultiSquareCanvas*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if(multiSquareCanvas == nullptr)
        return DefWindowProc(hwnd, msg, wparam, lparam);

    Window* window = (Window*)multiSquareCanvas->Sizable::state.window;

    if(window == nullptr)
        return DefWindowProc(hwnd, msg, wparam, lparam);

    switch(msg) {
        case WM_GETDLGCODE: return DLGC_STATIC | DLGC_WANTCHARS;
        case WM_ERASEBKGND: {
            const auto& hMemDC = multiSquareCanvas->p.doubleBuffer->hMemDC;
            const auto* hdc = reinterpret_cast<HDC>(wparam);
            if (hdc != hMemDC)
                return 0;

            return 1;
        }
        case WM_NCHITTEST:
            return HTCLIENT;
        case WM_MOUSEWHEEL: {
            multiSquareCanvas->p.scroll(GET_WHEEL_DELTA_WPARAM(wparam));
        } break;
    }

    return CallWindowProc(multiSquareCanvas->p.wndprocOrig, hwnd, msg, wparam, lparam);
}

auto pMultiSquareCanvas::scroll(int delta) -> void {
    int min, max;

    if (GetScrollRange( hwndScroller, SB_CTL, &min, &max )) {
        scrollPos -= (delta / 4);
        if (scrollPos > max)
            scrollPos = max;
        else if (scrollPos < 0)
            scrollPos = 0;

        SetScrollPos(hwndScroller, SB_CTL, scrollPos, true);

        if (hwnd)
            RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
    }
}

auto pMultiSquareCanvas::onChange(WPARAM wparam) -> void {
    if (hwndScroller)
        scrollTo(hwndScroller, wparam, scrollPos);

    if (hwnd)
        RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
}

auto pMultiSquareCanvas::buildDrawArea() -> void {
    unsigned* dots = multiSquareCanvas.getDotPtr();
    delete[] drawArea;
    drawArea = nullptr;

    if (!dots)
        return;

    unsigned cols = multiSquareCanvas.cols();
    unsigned rows = multiSquareCanvas.rows();
    unsigned squareSize = multiSquareCanvas.squareSize();
    unsigned padding = multiSquareCanvas.padding();

    unsigned width = cols * (squareSize + padding);
    unsigned height = rows * (squareSize + padding);

    drawArea = new unsigned[width * height];

    unsigned yPos = 0;
    unsigned* target;

    for (unsigned r = 0; r < rows; r++) {
        unsigned xPos = 0;

        for (unsigned c = 0; c < cols; c++) {
            unsigned color = *dots++;
            if (!color)
                color = DARK_BG_SOFTER_COL | (0xff << 24);

            for (unsigned y = 0; y < squareSize; y++) {
                target = drawArea + (yPos + y) * width + xPos;

                for (unsigned x = 0; x < padding; x++) {
                    *target++ = DARK_BG_COL | (0xff << 24);
                }

                for (unsigned x = 0; x < squareSize; x++) {
                    *target++ = color;
                }
            }

            xPos += squareSize + padding;
        }

        yPos += squareSize;

        for (unsigned y = 0; y < padding; y++) {
            target = drawArea + (yPos + y) * width;

            for (unsigned x = 0; x < width; x++) {
                *target++ = DARK_BG_COL | (0xff << 24);
            }
        }

        yPos += padding;
    }
}

auto pMultiSquareCanvas::drawItem(LPDRAWITEMSTRUCT lDraw) -> void {
    auto hdc = lDraw->hDC;
    auto rc = lDraw->rcItem;

    const auto& hMemDC = doubleBuffer->hMemDC;

    if (doubleBuffer->ensure(hdc, rc)) {
        const int savedState = ::SaveDC(hMemDC);
        IntersectClipRect(
            hMemDC,
            rc.left, rc.top, rc.right, rc.bottom
        );

        FillRect(hMemDC, &rc, getBackgroundBrush());

        if (drawArea) {
            HDC _hdc = CreateCompatibleDC(hMemDC);
            unsigned width = multiSquareCanvas.geometry().width - SCROLLBAR_WIDTH;
            unsigned height = multiSquareCanvas.geometry().height;
            unsigned padding = multiSquareCanvas.padding();
            unsigned cols = multiSquareCanvas.cols();
            unsigned rows = multiSquareCanvas.rows();
            unsigned squareSize = multiSquareCanvas.squareSize();
            unsigned fullWidth = cols * (squareSize + padding);
            unsigned fullHeight = rows * (squareSize + padding);
            unsigned offset = 0;

            if (fullWidth <= width)
                width = fullWidth;

            if (fullHeight <= height)
                height = fullHeight;
            else {
                offset = fullHeight - height;

                if (offset >= scrollPos)
                    offset = scrollPos;
            }

            BITMAPINFO bmi{};
            bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            bmi.bmiHeader.biPlanes = 1;
            bmi.bmiHeader.biBitCount = 32;
            bmi.bmiHeader.biCompression = BI_RGB;
            bmi.bmiHeader.biWidth = width;
            bmi.bmiHeader.biHeight = -((long)height);
            bmi.bmiHeader.biSizeImage = width * height * sizeof(uint32_t);
            void* bits = nullptr;
            HBITMAP bitmap = CreateDIBSection(_hdc, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);

            if (bits) {
                for (unsigned y = 0; y < height; y++)
                    std::memcpy((uint8_t*)bits + y * width * 4, (uint8_t*)(drawArea + (offset + y) * fullWidth), width * 4);
            }

            SelectObject(_hdc, bitmap);

            BLENDFUNCTION bf{ AC_SRC_OVER, 0, (BYTE)255, AC_SRC_ALPHA };
            AlphaBlend(hMemDC, 0, 0, width, height, _hdc, 0, 0, width, height, bf);

            DeleteObject(bitmap);
            DeleteDC(_hdc);
        }

        RestoreDC(hMemDC, savedState);

        BitBlt(hdc, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
            hMemDC, rc.left, rc.top, SRCCOPY
        );
    }
}
