
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
                        WS_CHILD | SS_OWNERDRAW,
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

auto pImageView::setEnabled(bool enabled) -> void {
    if (hwnd)
        InvalidateRect(hwnd, 0, false);
}

auto pImageView::setImage(Image* image) -> void {
    if (hwnd)
        InvalidateRect(hwnd, 0, false);
}

auto pImageView::rebuild() -> void {
    if(!needRebuild())
        return;

    create();
    InvalidateRect(hwnd, 0, false);
    pWidget::rebuild();
}

auto pImageView::updateCursor() -> void {
    if (hCursor)
        DestroyCursor( hCursor );

    if (imageView.enabled() && (!imageView.state.uri.empty() || imageView.onClick) )
        hCursor = LoadCursor(0, IDC_HAND);
    else
        hCursor = LoadCursor(0, IDC_ARROW);

    if (hCursor)
        SetCursor(hCursor);
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

        case WM_SETCURSOR:
            imageView->p.updateCursor();
            return 1;

        case WM_NCHITTEST:
            return HTCLIENT;

        case WM_LBUTTONDOWN:
            imageView->p.onLink();
            break;
    }

    return CallWindowProc(imageView->p.wndprocOrig, hwnd, msg, wparam, lparam);
    //return pApplication::wndProc(imageView->p.wndprocOrig, hwnd, msg, wparam, lparam);
}

auto pImageView::drawItem(LPDRAWITEMSTRUCT lDraw) -> void {
    if (!imageView.state.image)
        return;

    auto hdc = lDraw->hDC;
    auto rc = lDraw->rcItem;
    unsigned width = imageView.state.image->width;
    unsigned height = imageView.state.image->height;

    SetBkMode(hdc, TRANSPARENT);
    HBRUSH brush = getBackgroundBrush();
    if (brush)
        FillRect(hdc, &rc, brush);

    PAINTSTRUCT ps;
    BeginPaint(hwnd, &ps);
    HDC _hdc = CreateCompatibleDC(hdc);

    auto bitmap = CreateBitmapWithPremultipliedAlpha(*imageView.state.image);
    SelectObject(_hdc, bitmap);

    //RECT rc;
    //GetClientRect(hwnd, &rc);
   // if (pApplication::pDrawThemeParentBackground)
     //   pApplication::pDrawThemeParentBackground(hwnd, hdc, &rc);

    BLENDFUNCTION bf{ AC_SRC_OVER, 0, (BYTE)255, AC_SRC_ALPHA };
    AlphaBlend(hdc, 0, 0, width, height, _hdc, 0, 0, width, height, bf);

    DeleteObject(bitmap);
    DeleteDC(_hdc);
}