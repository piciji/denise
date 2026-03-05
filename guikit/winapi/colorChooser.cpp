
auto pColorChooser::choose(ColorChooser::State& state) -> std::optional<unsigned> {
    static COLORREF crCustColors[16];

    unsigned col = state.defaultColor;

    CHOOSECOLOR cc;
    std::memset(&cc, 0, sizeof(CHOOSECOLOR));
    cc.lStructSize = sizeof(CHOOSECOLOR);
    cc.lCustData = 0;
    cc.hwndOwner = state.window ? state.window->p.hwnd : nullptr;
    cc.rgbResult = RGB((col >> 16) & 0xff, (col >> 8) & 0xff, col & 0xff);
    cc.lpCustColors = crCustColors;
    cc.lpfnHook = pColorChooser::chooseColorDlgProc;
    cc.Flags = CC_FULLOPEN | CC_RGBINIT | CC_ENABLEHOOK;
    cc.lpTemplateName = NULL;

    if (ChooseColor(&cc)) {
        col = (unsigned)cc.rgbResult;
        return ((col >> 16) & 0xff) | ((col & 0xff) << 16) | (col & 0xff00);
    }

    return std::nullopt;
}

auto CALLBACK pColorChooser::chooseColorDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) -> uintptr_t {
    switch (message) {
        case WM_INITDIALOG: {
            if (pApplication::useDark) {
                DWORD darkMode = TRUE;
                DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &darkMode, sizeof(darkMode));

                EnumChildWindows(hwnd, [](HWND hwnd, LPARAM lParam) WINAPI_LAMBDA {
                    constexpr size_t classNameLen = 32;
                    wchar_t className[classNameLen]{};
                    GetClassName(hwnd, className, classNameLen);

                    if (wcscmp(className, WC_BUTTON) == 0) {
                        SetWindowTheme(hwnd, L"Explorer", NULL);
                        pApplication::pAllowDarkModeForWindow(hwnd, true);
                        SendMessageW(hwnd, WM_THEMECHANGED, 0, 0);
                    } else if (wcscmp(className, WC_EDIT) == 0) {
                        SetWindowTheme(hwnd, L"CFD", NULL);
                        pApplication::pAllowDarkModeForWindow(hwnd, true);
                        SendMessageW(hwnd, WM_THEMECHANGED, 0, 0);
                    }

                    return TRUE;
                }, lParam);
            }
            break;
        }

        case WM_CTLCOLOREDIT:
            if (pApplication::useDark) {
                SetBkMode((HDC)(wParam), TRANSPARENT);
                SetTextColor((HDC)wParam, DARK_FG_COL);
                SetBkColor((HDC)wParam, DARK_BG_SOFTER_COL);
                return (INT_PTR)pApplication::darkBGSofterBrush;
            } break;

        case WM_CTLCOLORLISTBOX:
        case WM_CTLCOLORDLG:
        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLORBTN: {
            if (pApplication::useDark) {
                SetBkMode((HDC)(wParam), TRANSPARENT);
                SetBkColor((HDC)wParam, DARK_BG_SOFTER_COL);
                SetTextColor((HDC)wParam, DARK_FG_COL);
                return (INT_PTR)pApplication::darkBGTabBrush;
            }
        } break;

        case WM_COMMAND: {
            int r = GetDlgItemInt(hwnd, 0x2C2, NULL, FALSE);
            int g = GetDlgItemInt(hwnd, 0x2C3, NULL, FALSE);
            int b = GetDlgItemInt(hwnd, 0x2C4, NULL, FALSE);

            if (GUIKIT::ColorChooser::onChoose)
                GUIKIT::ColorChooser::onChoose( r << 16 | g << 8 | b );
        } break;

        case WM_PRINTCLIENT:
            return TRUE;

        case WM_ERASEBKGND: {
            // RECT rc = {};
            // GetClientRect(hwnd, &rc);
            // FillRect(reinterpret_cast<HDC>(wParam), &rc, pApplication::darkBGTabBrush);
            // return TRUE;
            return 0;
        }
    }
    return FALSE;
}
