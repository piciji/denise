
pComboButton::~pComboButton() {
    for(auto _f : hfonts)
        pFont::free(_f);
}

auto pComboButton::append(std::string text, const std::string& _font) -> void {
    if (_font.empty())
        hfonts.push_back(nullptr);
    else
        hfonts.push_back(pFont::create(_font ));

    if (!hwnd)
        return;
    
    SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)(wchar_t*)utf16_t(text));
    
    if(SendMessage(hwnd, CB_GETCOUNT, 0, 0) == 1)
        setSelection(0);

    calculatedMinimumSize.updated = false;
}

auto pComboButton::minimumSize() -> Size {

	static Size containerSize = pWidget::getScaledContainerSize( { pApplication::useDark ? 26u : 24u, pApplication::useDark ? 6u : 8u } );
	
    if (calculatedMinimumSize.updated)
        return calculatedMinimumSize.minimumSize; 
    
    unsigned maximumWidth = 0;
    for (int i = 0; i < comboButton.rows(); i++) {
        auto text = comboButton.text(i);
        HFONT _hfont = hfonts[i];
        if (!_hfont)
            _hfont = hfont;
        maximumWidth = std::max<unsigned>(maximumWidth, pFont::size(_hfont, text).width);
    }
    
    calculatedMinimumSize.updated = true;
    
    calculatedMinimumSize.minimumSize = {maximumWidth + containerSize.width,
											pFont::size(hfont, " ").height + (pApplication::useDark ? 6 : 8)}; // don't use scaled height
	
    return calculatedMinimumSize.minimumSize;
}

auto pComboButton::remove(unsigned selection) -> void {
    if (selection < hfonts.size()) {
        pFont::free(hfonts[selection]);
        hfonts.erase(hfonts.begin() + selection);
    }

    if (!hwnd)
        return;
    
    SendMessage(hwnd, CB_DELETESTRING, selection, 0);
    if (selection == comboButton.selection())
        comboButton.setSelection(0);
}

auto pComboButton::reset() -> void {
    for (auto _f : hfonts)
        pFont::free(_f);

    hfonts.clear();

    if (!hwnd)
        return;

    SendMessage(hwnd, CB_RESETCONTENT, 0, 0);
}

auto pComboButton::setGeometry(Geometry geometry) -> void {
    if(!hwnd)
        return;
    
    pWidget::setGeometry({geometry.x, geometry.y, geometry.width, 1});
    //RECT rc;
  //  GetWindowRect(hwnd, &rc);
 //   unsigned _height = geometry.height - ((rc.bottom - rc.top) - 
   //     SendMessage(hwnd, CB_GETITEMHEIGHT, (WPARAM), 0));
    
    SendMessage(hwnd, CB_SETITEMHEIGHT, (WPARAM)-1, geometry.height - 6);
}

auto pComboButton::setSelection(unsigned selection) -> void {
    if(hwnd)
        SendMessage(hwnd, CB_SETCURSEL, selection == ~0 ? -1 : selection, 0);
}

auto pComboButton::setText(unsigned selection, const std::string& text) -> void {
    if(!hwnd)
        return;
    
    SendMessage(hwnd, CB_DELETESTRING, selection, 0);
    SendMessage(hwnd, CB_INSERTSTRING, selection, (LPARAM)(wchar_t*)utf16_t(text));
    setSelection(comboButton.selection());
    calculatedMinimumSize.updated = false;
}

auto pComboButton::setDroppable(bool droppable) -> void {
    if (hwnd)
        DragAcceptFiles(hwnd, droppable);
}

auto pComboButton::create() -> void {
    destroy(hwnd);
    destroy(hwndTip);
    
    hwnd = CreateWindow(
        WC_COMBOBOX, L"",
        WS_CHILD | WS_TABSTOP | CBS_DROPDOWNLIST | CBS_HASSTRINGS |
        (comboButton.hintVerticalScrollbar ? WS_VSCROLL : 0) | 
        ((comboButton.hintMultiFonts || pApplication::useDark) ? CBS_OWNERDRAWFIXED : 0),
        0, 0, 0, 0,
        getParentHandle(), (HMENU)(unsigned long long)comboButton.id, GetModuleHandle(0), 0
    );

    if (pApplication::useDark) {
        SetWindowTheme(hwnd, L"CFD", NULL);
        pApplication::pAllowDarkModeForWindow(hwnd, true);
        SendMessageW(hwnd, WM_THEMECHANGED, 0, 0);

        COMBOBOXINFO cbi{};
        cbi.cbSize = sizeof(COMBOBOXINFO);
        if (GetComboBoxInfo(hwnd, &cbi) && cbi.hwndList)
            SetWindowTheme(cbi.hwndList, L"DarkMode_Explorer", nullptr);       
    }

    //ComboBox_SetMinVisible(hwnd, 30);

    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)&comboButton);
    wndprocOrig = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)subclassWndProc);
}

auto CALLBACK pComboButton::subclassWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT {
    ComboButton* comboButton = (ComboButton*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if(comboButton == nullptr) return DefWindowProc(hwnd, msg, wparam, lparam);

    switch(msg) {
        case WM_ERASEBKGND:
            return 0;
        case WM_DROPFILES: {
            std::vector<std::string> paths = getDropPaths(wparam);

            if (!paths.empty() && comboButton->onDrop)
                comboButton->onDrop(paths);

            return false;
        } break;
    }
    
    return CallWindowProc(comboButton->p.wndprocOrig, hwnd, msg, wparam, lparam);
    //return pApplication::wndProc(comboButton->p.wndprocOrig, hwnd, msg, wparam, lparam);
}

auto pComboButton::measureItem(LPMEASUREITEMSTRUCT lpmis) -> bool {
    Size _size;
    int maxHeight = 0;

    if (comboButton.hintMultiFonts) {
        for(int i = 0; i < comboButton.rows(); i++) {
            std::string _str = comboButton.text(i);
            HFONT _hfont = hfonts[i];
            if (!_hfont)
                _hfont = hfont;

            _size = pFont::size(_hfont, _str);
            if (_size.height > maxHeight)
                maxHeight = _size.height;
        }
    } else /*if (comboButton.rows())*/ {
        //std::string _str = comboButton.text(0);
        maxHeight = pFont::size(hfont, " ").height;
    }

    if (maxHeight) {
        lpmis->itemHeight = maxHeight;
        return true;
    }

    return false;
}

auto pComboButton::drawItem(LPDRAWITEMSTRUCT lDraw) -> void {
    auto lRow = lDraw->rcItem;
    auto text = comboButton.text(lDraw->itemID);
    HFONT _hfont = nullptr;
    int adjust = 0;

    if (lDraw->itemID < hfonts.size())
        _hfont = hfonts[lDraw->itemID];

    if (_hfont) {
        Size _size = pFont::size(_hfont, text);

        int _hItem = lRow.bottom - lRow.top;
        int _hText = _size.height;   

        if (_hItem > _hText)
            adjust = (float)(_hItem - _hText) / 2.0 + 0.5;
    }
    auto textRC = lRow;
    textRC.top += adjust; //center vertically
    textRC.bottom += adjust;
    textRC.left += 2;

    HFONT oldFont = nullptr;
    if (lDraw->itemID < hfonts.size())
        oldFont = (HFONT)SelectObject(lDraw->hDC, hfonts[lDraw->itemID]);

    SetBkMode(lDraw->hDC, TRANSPARENT);

    if (lDraw->itemState & ODS_SELECTED) {
        if (pApplication::useDark) {
            FillRect(lDraw->hDC, &lRow, pApplication::darkBGHotBrush);
            SetTextColor(lDraw->hDC, DARK_FG_COL);
        } else {
            FillRect(lDraw->hDC, &lRow, CreateSolidBrush(GetSysColor(COLOR_HIGHLIGHT)));
            SetTextColor(lDraw->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
        }        
    } else {
        if (pApplication::useDark) {
            FillRect(lDraw->hDC, &lRow, pApplication::darkBGSofterBrush);
            SetTextColor(lDraw->hDC, IsWindowEnabled(hwnd) ? DARK_FG_COL : GetSysColor(COLOR_GRAYTEXT));
        } else {
            FillRect(lDraw->hDC, &lRow, CreateSolidBrush(GetSysColor(COLOR_WINDOW)));
            SetTextColor(lDraw->hDC, GetSysColor(COLOR_WINDOWTEXT));
        }        
    }

    DrawText(lDraw->hDC, utf16_t(text.c_str()), -1, &textRC, DT_LEFT | DT_NOPREFIX);

    if (oldFont)
        SelectObject(lDraw->hDC, oldFont);
}

auto pComboButton::rebuild() -> void {
    if(!needRebuild())
        return;
        
    create();
    setFont( widget.font() );

    for (int i = 0; i < comboButton.rows(); i++)
        append(comboButton.state.rows[i], comboButton.state.fonts[i]);
    
    setSelection(comboButton.selection());
    setDroppable(comboButton.droppable());
    pWidget::rebuild();
}

auto pComboButton::onChange() -> void {
    unsigned selection = SendMessage(hwnd, CB_GETCURSEL, 0, 0);
    if(selection == comboButton.selection())
        return;
    
    comboButton.state.selection = selection;
    if(comboButton.onChange)
        comboButton.onChange();
}
