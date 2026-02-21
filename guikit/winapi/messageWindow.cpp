
HHOOK pMessageWindow::hhookCBTProc = 0;
WNDPROC pMessageWindow::wndprocOrig = 0;

auto pMessageWindow::translateResponse(UINT response) -> MessageWindow::Response {
    if(response == IDOK) return MessageWindow::Response::Ok;
    if(response == IDCANCEL) return MessageWindow::Response::Cancel;
    if(response == IDYES) return MessageWindow::Response::Yes;
    if(response == IDNO) return MessageWindow::Response::No;

    return MessageWindow::Response::Cancel;
}

auto pMessageWindow::translateButtons(MessageWindow::Buttons buttons) -> UINT {
    if(buttons == MessageWindow::Buttons::Ok) return MB_OK;
    if(buttons == MessageWindow::Buttons::OkCancel) return MB_OKCANCEL;
    if(buttons == MessageWindow::Buttons::YesNo) return MB_YESNO;
    if(buttons == MessageWindow::Buttons::YesNoCancel) return MB_YESNOCANCEL;

    return MB_OK;
}

auto CALLBACK pMessageWindow::pfnCBTMsgBoxHook(int nCode, WPARAM wparam, LPARAM lparam) -> LRESULT {
    if (nCode == HC_ACTION) {
        CWPSTRUCT* pwp = (CWPSTRUCT*)lparam;

        if (pwp->message == WM_INITDIALOG) {
            DWORD darkMode = TRUE;
            DwmSetWindowAttribute(pwp->hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &darkMode, sizeof(darkMode));
            wndprocOrig = (WNDPROC)SetWindowLongPtr(pwp->hwnd, GWLP_WNDPROC, (LONG_PTR)subclassWndProc);
        }
    }
    return CallNextHookEx(hhookCBTProc, nCode, wparam, lparam);
}

auto CALLBACK pMessageWindow::subclassWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT {
    LRESULT rc = CallWindowProc(wndprocOrig, hwnd, msg, wparam, lparam);

    switch (msg) {
        case WM_INITDIALOG: {
            EnumChildWindows(hwnd, [](HWND hwnd, LPARAM lParam) WINAPI_LAMBDA {
                constexpr size_t classNameLen = 32;
                wchar_t className[classNameLen]{};
                GetClassName(hwnd, className, classNameLen);

                if (wcscmp(className, WC_BUTTON) == 0) {
                    SetWindowTheme(hwnd, L"Explorer", NULL);
                    pApplication::pAllowDarkModeForWindow(hwnd, true);
                    SendMessageW(hwnd, WM_THEMECHANGED, 0, 0);
                }

                return TRUE;
            }, lparam);
        } break;
        case WM_CTLCOLORDLG:
        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLORBTN: {
            SetTextColor((HDC)wparam, DARK_FG_COL);
            SetBkMode((HDC)(wparam), TRANSPARENT);
            return (INT_PTR)pApplication::darkBGTabBrush;
        }

        case WM_ERASEBKGND: {
            RECT rc;
            GetClientRect(hwnd, &rc);
            PAINTSTRUCT ps;
            BeginPaint(hwnd, &ps);
            FillRect(ps.hdc, &rc, pApplication::darkBGTabBrush);
            EndPaint(hwnd, &ps);
            return 1;
        }

        case WM_NCDESTROY:
            UnhookWindowsHookEx(hhookCBTProc);
            break;
    }
    
    return rc;
}

auto pMessageWindow::error(MessageWindow::State& state) -> MessageWindow::Response {
    if (pApplication::useDark)
        hhookCBTProc = SetWindowsHookEx(WH_CALLWNDPROC, pfnCBTMsgBoxHook, 0, GetCurrentThreadId());

    return translateResponse( MessageBox( state.window ? state.window->p.hwnd : 0,
        utf16_t(state.text), utf16_t(state.title), MB_TOPMOST | MB_ICONERROR | translateButtons(state.buttons)
    ));
}

auto pMessageWindow::information(MessageWindow::State& state) -> MessageWindow::Response {
    if (pApplication::useDark)
        hhookCBTProc = SetWindowsHookEx(WH_CALLWNDPROC, pfnCBTMsgBoxHook, 0, GetCurrentThreadId());

    return translateResponse( MessageBox( state.window ? state.window->p.hwnd : 0,
        utf16_t(state.text), utf16_t(state.title), MB_TOPMOST | MB_ICONINFORMATION | translateButtons(state.buttons)
    ));   
}

auto pMessageWindow::question(MessageWindow::State& state) -> MessageWindow::Response {
    if (pApplication::useDark)
        hhookCBTProc = SetWindowsHookEx(WH_CALLWNDPROC, pfnCBTMsgBoxHook, 0, GetCurrentThreadId());

    return translateResponse( MessageBox( state.window ? state.window->p.hwnd : 0,
        utf16_t(state.text), utf16_t(state.title), MB_TOPMOST | MB_ICONQUESTION | translateButtons(state.buttons)
    ));
}

auto pMessageWindow::warning(MessageWindow::State& state) -> MessageWindow::Response {
    if (pApplication::useDark)
        hhookCBTProc = SetWindowsHookEx(WH_CALLWNDPROC, pfnCBTMsgBoxHook, 0, GetCurrentThreadId());

    return translateResponse( MessageBox( state.window ? state.window->p.hwnd : 0,
        utf16_t(state.text), utf16_t(state.title), MB_TOPMOST | MB_ICONWARNING | translateButtons(state.buttons)
    ));
}
