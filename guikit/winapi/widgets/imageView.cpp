
auto pImageView::minimumSize() -> Size {
    if (!imageView.state.image)
        return {0u, 0u};

    auto image = imageView.state.image;

    return {image->width, image->height};
}

auto pImageView::create() -> void {
    destroy(hwnd);
    destroy(hwndTip);

    hwnd = CreateWindow(WC_STATIC, L"",
                        WS_CHILD,
                        0, 0, 0, 0, getParentHandle(), (HMENU)(unsigned long long)imageView.id, GetModuleHandle(0), 0);

    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)&imageView);
    wndprocOrig = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)subclassWndProc);
}

auto pImageView::onLink() -> void {

    if (!hwnd)
        return;

    if (imageView.onClick)
        imageView.onClick();

    if (!imageView.state.uri.empty())
        ShellExecute(hwnd, L"open",  utf16_t(imageView.state.uri), NULL, NULL, SW_SHOWNORMAL );
}

auto pImageView::rebuild() -> void {
    if(!needRebuild())
        return;

    create();
    InvalidateRect(hwnd, 0, false);
    pWidget::rebuild();
}

auto CALLBACK pImageView::subclassWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT {
    ImageView* imageView = (ImageView*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if(imageView == nullptr)
        return DefWindowProc(hwnd, msg, wparam, lparam);

    Window* window = (Window*)imageView->Sizable::state.window;

    if(window == nullptr)
        return DefWindowProc(hwnd, msg, wparam, lparam);

    switch(msg) {
        case WM_GETDLGCODE: return DLGC_STATIC | DLGC_WANTCHARS;
        case WM_ERASEBKGND:
            return 0;
        case WM_PAINT:
        {
            if (!imageView->state.image)
                return 0;

            unsigned width = imageView->state.image->width;
            unsigned height = imageView->state.image->height;

            PAINTSTRUCT ps;
            BeginPaint(hwnd, &ps);
            HDC hdc = CreateCompatibleDC(ps.hdc);

            auto bitmap = CreateBitmapWithPremultipliedAlpha(*imageView->state.image);
            SelectObject(hdc, bitmap);

            RECT rc;
            GetClientRect(hwnd, &rc);
            DrawThemeParentBackground(hwnd, ps.hdc, &rc);

            BLENDFUNCTION bf{AC_SRC_OVER, 0, (BYTE) 255, AC_SRC_ALPHA};
            AlphaBlend(ps.hdc, 0, 0, width, height, hdc, 0, 0, width, height, bf);

            DeleteObject(bitmap);
            DeleteDC(hdc);

            EndPaint(hwnd, &ps);

            return 0;
        }
        case WM_SETCURSOR:
            HCURSOR hCursor;
            if (imageView->state.uri == "") {
                hCursor = LoadCursor(0, IDC_ARROW);
            } else {
                hCursor = LoadCursor(0, IDC_HAND);
            }

            if (hCursor) {
                SetCursor(hCursor);
            }

            return 1;

        case WM_LBUTTONDOWN:
            imageView->p.onLink(); break;
    }

    return DefWindowProc(hwnd, msg, wparam, lparam);
}