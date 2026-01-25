
#define SCROLLBAR_HEIGHT 20
#define DMA_SLOT_WIDTH 70

pLogicViewer::~pLogicViewer() {
    invalidateDrawArea();

    for (auto& brush : brushes)
        DeleteObject(brush.second);

    if (penDarkEdge)
        delete penDarkEdge;
    if (penFG)
        delete penFG;
}

auto pLogicViewer::create() -> void {
    destroy(hwnd);
    destroy(hwndTip);

    hwnd = CreateWindow(WC_STATIC, L"",
        WS_CHILD | SS_OWNERDRAW,
        0, 0, 0, 0, getParentHandle(), (HMENU)(unsigned long long)logicViewer.id, GetModuleHandle(0), 0);

    hwndScroller = CreateWindow( WC_SCROLLBAR, L"",
        WS_CHILD | SBS_HORZ,
        0, 0, 0, 0, getParentHandle(), (HMENU)(unsigned long long)logicViewer.id, GetModuleHandle(0), 0);

    if (pApplication::useDark) {
        SetWindowTheme(hwndScroller, L"Explorer", NULL);
        pApplication::pAllowDarkModeForWindow(hwndScroller, true);
        SendMessageW(hwndScroller, WM_THEMECHANGED, 0, 0);
    }

    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)&logicViewer);
    SetWindowLongPtr(hwndScroller, GWLP_USERDATA, (LONG_PTR)&logicViewer);
    wndprocOrig = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)subclassWndProc);
    wndprocOrigScroller = (WNDPROC)SetWindowLongPtr(hwndScroller, GWLP_WNDPROC, (LONG_PTR)subclassWndProcScroller);

    penFG = new Gdiplus::Pen(Gdiplus::Color(0xa4, 0xa4, 0xa4));
    penDarkEdge = new Gdiplus::Pen(Gdiplus::Color(0x64, 0x64, 0x64));

    scrollTimer.onFinished = [this]() {
        scrollToActive();
    };
    scrollTimer.setInterval(20);
    scrollTimer.setData(0);
}

auto pLogicViewer::setGeometry(Geometry geometry) -> void {
    if (!hwnd)
        return;

    Widget* parent = getParentTabWidget();

    if (parent) {
        auto geo = parent->geometry();

        geometry.x -= geo.x;
        geometry.y -= geo.y;
    }

    SetWindowPos(hwnd, NULL, geometry.x, geometry.y, geometry.width, geometry.height - SCROLLBAR_HEIGHT, SWP_NOZORDER | SWP_NOCOPYBITS);

    SetWindowPos(hwndScroller, NULL, geometry.x, geometry.y + geometry.height - SCROLLBAR_HEIGHT, geometry.width, SCROLLBAR_HEIGHT, SWP_NOZORDER | SWP_NOCOPYBITS);

    updateScrollRange();
}

auto pLogicViewer::setVisible(bool visible) -> void {
    if(hwndScroller)
        ShowWindow(hwndScroller, visible ? SW_SHOWNORMAL : SW_HIDE);

    pWidget::setVisible(visible);
}

auto pLogicViewer::setEnabled(bool enabled) -> void {
    if(hwndScroller)
        EnableWindow(hwndScroller, enabled);

    pWidget::setEnabled(enabled);
}

auto pLogicViewer::rebuild() -> void {
    if(!needRebuild())
        return;

    invalidateDrawArea();

    create();
    scrollPos = 0;
    InvalidateRect(hwnd, 0, false);

    if(hwndScroller)
        SetWindowPos(hwndScroller, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

    pWidget::rebuild();
}

auto pLogicViewer::invalidateDrawArea() -> void {
    if (drawBmp) {
        DeleteObject(drawBmp);
        drawBmp = nullptr;
    }
    if (drawDC) {
        DeleteDC(drawDC);
        drawDC = nullptr;
    }
}

auto pLogicViewer::updateScrollRange() -> void {
    if (hwndScroller) {
        unsigned width = logicViewer.geometry().width;
        unsigned scrollWidth = calcFullWidth();

        if (scrollWidth > width) {
            scrollWidth = scrollWidth - width;
            ShowWindow(hwndScroller, SW_SHOWNORMAL);
        } else {
            ShowWindow(hwndScroller, SW_HIDE);
            scrollWidth = 0;
        }

        SetScrollRange(hwndScroller, SB_CTL, 0, scrollWidth, true);
    }
    update();
}

auto pLogicViewer::scroll(int delta) -> void {
    int min, max;

    if (GetScrollRange( hwndScroller, SB_CTL, &min, &max )) {
        scrollPos -= delta;
        if (scrollPos > max)
            scrollPos = max;
        else if (scrollPos < min)
            scrollPos = min;

        SetScrollPos(hwndScroller, SB_CTL, scrollPos, true);
    }
    update();
}

#define SCROLL_STEPS 8

auto pLogicViewer::scrollToActive() -> void {
    unsigned scrollSlot = 0;
    for (auto& logicState : logicViewer.state.logics) {
        if (!logicState.active)
            break;

        scrollSlot++;
    }

    unsigned targetPos = scrollSlot * (DMA_SLOT_WIDTH + 1);
    unsigned width = logicViewer.geometry().width >> 1;

    if (targetPos > width)
        targetPos -= width;
    else
        targetPos = 0;

    unsigned counter = scrollTimer.data();

    if ( ((counter + 1) >= SCROLL_STEPS) || (targetPos == scrollPos) ) {
        scrollPos = targetPos;
        scrollTimer.setEnabled( false );
        scrollTimer.setData(0);
    } else {
        if (targetPos < scrollPos)
            scrollPos -= (scrollPos - targetPos) / (SCROLL_STEPS - counter);
        else
            scrollPos += (targetPos - scrollPos) / (SCROLL_STEPS - counter);

        scrollTimer.setData(counter + 1);
        scrollTimer.setEnabled( true );
    }

    if (hwndScroller)
        SetScrollPos(hwndScroller, SB_CTL, scrollPos, true);

    update();
}

auto pLogicViewer::update() -> void {
    invalidateDrawArea();

    if (hwnd)
        RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
}

auto CALLBACK pLogicViewer::subclassWndProcScroller(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT {
    LogicViewer* logicViewer = (LogicViewer*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if(logicViewer == nullptr)
        return DefWindowProc(hwnd, msg, wparam, lparam);

    Window* window = (Window*)logicViewer->Sizable::state.window;

    if(window == nullptr)
        return DefWindowProc(hwnd, msg, wparam, lparam);

    switch(msg) {
        case WM_GETDLGCODE: return DLGC_STATIC | DLGC_WANTCHARS;
        case WM_ERASEBKGND:
            return 0;

        case WM_NCHITTEST:
            return HTCLIENT;
        case WM_MOUSEWHEEL: {
            logicViewer->p.scroll(GET_WHEEL_DELTA_WPARAM(wparam) / 4);
        } break;
    }

    return CallWindowProc(logicViewer->p.wndprocOrigScroller, hwnd, msg, wparam, lparam);
}

auto CALLBACK pLogicViewer::subclassWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT {
    LogicViewer* logicViewer = (LogicViewer*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if(logicViewer == nullptr)
        return DefWindowProc(hwnd, msg, wparam, lparam);

    Window* window = (Window*)logicViewer->Sizable::state.window;

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
            logicViewer->p.scroll(GET_WHEEL_DELTA_WPARAM(wparam) / 4);
        } break;
    }

    return CallWindowProc(logicViewer->p.wndprocOrig, hwnd, msg, wparam, lparam);
}

auto pLogicViewer::onChange(WPARAM wparam) -> void {
    invalidateDrawArea();

    if (hwndScroller)
        scrollTo(hwndScroller, wparam, scrollPos);

    if (hwnd)
        RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
}

auto pLogicViewer::drawItem(LPDRAWITEMSTRUCT lDraw) -> void {
    auto hdc = lDraw->hDC;
    auto rc = lDraw->rcItem;

    unsigned width = rc.right - rc.left;
    unsigned neededWidth = calcFullWidth();
    unsigned offset = 0;

    if (neededWidth <= width)
        width = neededWidth;
    else {
        offset = neededWidth - width;

        if (offset >= scrollPos)
            offset = scrollPos;
    }

    unsigned firstSlot = offset / (DMA_SLOT_WIDTH + 1);
    offset = offset % (DMA_SLOT_WIDTH + 1);
    unsigned buildSlots = (width / (DMA_SLOT_WIDTH + 1)) + 2;

    if (drawDC == nullptr)
        buildDrawArea(hdc, rc, firstSlot, buildSlots);

    if (drawDC) {
        BitBlt(hdc, rc.left, rc.top, width, rc.bottom - rc.top,
        drawDC, rc.left + offset, rc.top, SRCCOPY
        );
    }
}

auto pLogicViewer::buildDrawArea(HDC hdc, RECT rcWork, unsigned firstSlot, unsigned buildSlots) -> void {
    unsigned maxSlots = logicViewer.state.logics.size();
    unsigned fullWidth = buildSlots * (DMA_SLOT_WIDTH + 1);
    unsigned height = rcWork.bottom - rcWork.top;

    RECT rc = { 0, 0, (long)fullWidth, (long)height};

    drawDC = CreateCompatibleDC(hdc);
    drawBmp = CreateCompatibleBitmap(hdc, fullWidth, height);

    SelectObject(drawDC, drawBmp);
    SelectObject(drawDC, hfont);
    SelectObject(drawDC, pApplication::darkEdgePen);
    SelectObject(drawDC, pApplication::darkBGSofterBrush);

    SetBkMode(drawDC, TRANSPARENT);
    FillRect(drawDC, &rc, pApplication::darkBGSofterBrush);

    if (maxSlots == 0)
        return;

    Gdiplus::Graphics g(drawDC);
    g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

    rc.right = DMA_SLOT_WIDTH;

    if (firstSlot >= maxSlots)
        firstSlot = 0;

    buildSlots = firstSlot + buildSlots;
    if (buildSlots > maxSlots)
        buildSlots = maxSlots;

    for (unsigned i = firstSlot; i < buildSlots; i++) {
        auto& logicState = logicViewer.state.logics[i];
        buildDmaSlot(g, logicState, rc);
        rc.left = rc.right + 1;
        rc.right = rc.left + DMA_SLOT_WIDTH;
    }
}

auto pLogicViewer::buildDmaSlot(Gdiplus::Graphics& g, LogicState& logicState, RECT rc) -> void {
    Gdiplus::GraphicsPath path;
    SelectObject(drawDC, pApplication::darkEdgePen);
    MoveToEx(drawDC, rc.right, 0, NULL);
    LineTo(drawDC, rc.right, rc.bottom);
    bool addrLength = logicViewer.addrAs24bit() ? 6 : 4;

    rc.top += 5;
    rc.bottom = rc.top + 20;

    if (logicState.active) {
        SelectObject(drawDC, pApplication::darkFGPen);
        SetTextColor(drawDC, pApplication::useDark ? DARK_FG_COL : GetSysColor(COLOR_WINDOW));
    } else {
        SetTextColor(drawDC, pApplication::useDark ? DARK_DISABLE_COL : GetSysColor(COLOR_GRAYTEXT));
    }

    DrawText(drawDC, utf16_t(convertToHex(logicState.position)), -1, &rc, DT_CENTER);
    rc.top = rc.bottom;
    rc.bottom = rc.top + 5;

    if (logicState.display != LogicState::Display::None) {
        FillRect(drawDC, &rc, getBrush(logicState.color));
    }

    setBox(rc);

    if (logicState.display == LogicState::Display::None) {
        DrawText(drawDC, L"-", -1, &rc, DT_CENTER);
        setBox(rc);
        drawLine(rc);
        setBox(rc, 10);
        drawLine(rc);

    } else {
        DrawText(drawDC, utf16_t(logicState.usage), -1, &rc, DT_CENTER);
        setBox(rc);
        std::string _addr = logicViewer.hasSymbolicAddr() ? logicState.symbolicAddr : convertToHex(logicState.addr, addrLength);
        drawRectRounded(g, &path, rc, _addr, 5, logicState.active);
        setBox(rc, 10);
        drawRectRounded(g, &path, rc, convertToHex(logicState.data), 10, logicState.active);
    }

    setBox(rc, 10);

    if (logicState.display2 == LogicState::Display::None) {
        DrawText(drawDC, L"-", -1, &rc, DT_CENTER);
        setBox(rc);
        drawLine(rc);
        setBox(rc, 10);
        drawLine(rc);

    } else {
        DrawText(drawDC, utf16_t(logicState.usage2), -1, &rc, DT_CENTER);
        setBox(rc);
        drawRectRounded(g, &path, rc, convertToHex(logicState.addr2, addrLength), 5, logicState.active);
        setBox(rc, 10);
        drawRectRounded(g, &path, rc, convertToHex(logicState.data2), 10, logicState.active);
    }

    for (auto& watch : logicState.watches) {
        setBox(rc, 15);
        drawRect(watch.first, g, &path, rc, convertToHex(watch.second), 10, logicState.active);
    }
}

inline auto pLogicViewer::setBox(RECT& rc, unsigned offset) -> void {
    rc.top = rc.bottom + offset;
    rc.bottom = rc.top + 20;
}

inline auto pLogicViewer::drawLine(RECT& rc) -> void {
    unsigned center = (rc.bottom + rc.top) / 2;

    MoveToEx(drawDC, rc.left, center, NULL);
    LineTo(drawDC, rc.right, center);
}

inline auto pLogicViewer::drawRect(LogicState::Display display, Gdiplus::Graphics& g, Gdiplus::GraphicsPath* path, RECT& rc, const std::string& text, unsigned padding, bool active) -> void {
    switch (display) {
        default:
        case LogicState::Display::None:
            drawLine(rc);
            break;
        case LogicState::Display::SingleBlock:
            drawRectRounded(g, path, rc, text, padding, active);
            break;
        case LogicState::Display::BeginBlock:
            drawRectLeftRounded(g, path, rc, text, padding, active);
            break;
        case LogicState::Display::KeepBlock:
            drawRect(rc, text);
            break;
        case LogicState::Display::EndBlock:
            drawRectRightRounded(g, path, rc, text, padding, active);
            break;
    }
}

inline auto pLogicViewer::drawRect(RECT& rc, const std::string& text) -> void {
    MoveToEx(drawDC, rc.left, rc.top, NULL);
    LineTo(drawDC, rc.right, rc.top);

    MoveToEx(drawDC, rc.left, rc.bottom - 1, NULL);
    LineTo(drawDC, rc.right, rc.bottom - 1);

    DrawText(drawDC, utf16_t(text), -1, &rc, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
}

inline auto pLogicViewer::drawRectRounded(Gdiplus::Graphics& g, Gdiplus::GraphicsPath* path, RECT rc, const std::string& text, unsigned padding, bool active) -> void {
    GetRoundRectPath(path, Gdiplus::Rect(rc.left + padding, rc.top, rc.right - rc.left - (padding * 2), rc.bottom - rc.top), 10 );

    g.DrawPath(active ? penFG : penDarkEdge, path);

    unsigned center = (rc.bottom + rc.top) / 2;

    MoveToEx(drawDC, rc.left, center, NULL);
    LineTo(drawDC, rc.left + padding, center);

    MoveToEx(drawDC, rc.right - padding, center, NULL);
    LineTo(drawDC, rc.right, center);

    DrawText(drawDC, utf16_t(text), -1, &rc, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
}

inline auto pLogicViewer::drawRectLeftRounded(Gdiplus::Graphics& g, Gdiplus::GraphicsPath* path, RECT rc, const std::string& text, unsigned padding, bool active) -> void {
    GetRoundRectPathLeft(path, Gdiplus::Rect(rc.left + padding, rc.top, rc.right - rc.left - (padding), rc.bottom - rc.top), 10 );

    g.DrawPath(active ? penFG : penDarkEdge, path);

    unsigned center = (rc.bottom + rc.top) / 2;

    MoveToEx(drawDC, rc.left, center, NULL);
    LineTo(drawDC, rc.left + padding, center);

    rc.left += padding;
    DrawText(drawDC, utf16_t(text), -1, &rc, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
}

inline auto pLogicViewer::drawRectRightRounded(Gdiplus::Graphics& g, Gdiplus::GraphicsPath* path, RECT rc, const std::string& text, unsigned padding, bool active) -> void {
    GetRoundRectPathRight(path, Gdiplus::Rect(rc.left, rc.top, rc.right - rc.left - (padding), rc.bottom - rc.top), 10 );

    g.DrawPath(active ? penFG : penDarkEdge, path);

    unsigned center = (rc.bottom + rc.top) / 2;

    MoveToEx(drawDC, rc.right - padding, center, NULL);
    LineTo(drawDC, rc.right, center);

    rc.right -= padding;
    DrawText(drawDC, utf16_t(text), -1, &rc, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
}

inline auto pLogicViewer::getBrush(unsigned color) -> HBRUSH {
    for (auto& brush : brushes) {
        if (brush.first == color)
            return brush.second;
    }

    HBRUSH b = CreateSolidBrush( makeColorRef(color) );

    brushes.push_back({color, b});
    return b;
}

auto pLogicViewer::calcFullWidth() -> unsigned {
    unsigned slots = logicViewer.state.logics.size();
    return slots * DMA_SLOT_WIDTH + (slots - 1); // add grid lines
}

auto pLogicViewer::GetRoundRectPath(Gdiplus::GraphicsPath* pPath, Gdiplus::Rect r, int dia) -> void {
    Gdiplus::Rect Corner(r.X, r.Y, dia, dia);

    pPath->Reset();

    pPath->AddArc(Corner, 180, 90);

    Corner.X += (r.Width - dia - 1);
    pPath->AddArc(Corner, 270, 90);

    Corner.Y += (r.Height - dia - 1);
    pPath->AddArc(Corner, 0, 90);

    Corner.X -= (r.Width - dia - 1);
    pPath->AddArc(Corner, 90, 90);

    pPath->CloseFigure();
}

auto pLogicViewer::GetRoundRectPathLeft(Gdiplus::GraphicsPath* pPath, Gdiplus::Rect r, int dia) -> void {
    Gdiplus::Rect Corner(r.X, r.Y, dia, dia);
    pPath->Reset();
    pPath->AddLine(r.X + r.Width, r.Y, r.X + dia, r.Y);

    pPath->AddArc(Corner, 270, -90);

    pPath->AddLine(r.X, r.Y + dia, r.X, r.Y + (r.Height - dia));

    Corner.Y += (r.Height - dia - 1);
    pPath->AddArc(Corner, 180, -90);

    pPath->AddLine(r.X + dia, r.Y + r.Height - 1, r.X + r.Width, r.Y + r.Height - 1);
}

auto pLogicViewer::GetRoundRectPathRight(Gdiplus::GraphicsPath* pPath, Gdiplus::Rect r, int dia) -> void {
    Gdiplus::Rect Corner(r.X, r.Y, dia, dia);
    pPath->Reset();
    pPath->AddLine(r.X, r.Y, r.X + r.Width - dia - 1, r.Y);

    Corner.X += (r.Width - dia - 1);
    pPath->AddArc(Corner, 270, 90);

    pPath->AddLine(r.X + r.Width - 1, r.Y + dia, r.X + r.Width - 1, r.Y + (r.Height - dia));

    Corner.Y += (r.Height - dia - 1);
    pPath->AddArc(Corner, 0, 90);

    pPath->AddLine(r.X + r.Width - dia - 1, r.Y + r.Height - 1, r.X, r.Y + r.Height - 1);
}
