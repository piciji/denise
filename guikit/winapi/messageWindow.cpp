
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

auto pMessageWindow::error(MessageWindow::State& state) -> MessageWindow::Response {
    return translateResponse( MessageBox( state.window ? state.window->p.hwnd : 0,
        utf16_t(state.text), utf16_t(state.title), MB_TOPMOST | MB_ICONERROR | translateButtons(state.buttons)
    ));
}

auto pMessageWindow::information(MessageWindow::State& state) -> MessageWindow::Response {
    return translateResponse( MessageBox( state.window ? state.window->p.hwnd : 0,
        utf16_t(state.text), utf16_t(state.title), MB_TOPMOST | MB_ICONINFORMATION | translateButtons(state.buttons)
    ));
}

auto pMessageWindow::question(MessageWindow::State& state) -> MessageWindow::Response {
    return translateResponse( MessageBox( state.window ? state.window->p.hwnd : 0,
        utf16_t(state.text), utf16_t(state.title), MB_TOPMOST | MB_ICONQUESTION | translateButtons(state.buttons)
    ));
}

auto pMessageWindow::warning(MessageWindow::State& state) -> MessageWindow::Response {
    return translateResponse( MessageBox( state.window ? state.window->p.hwnd : 0,
        utf16_t(state.text), utf16_t(state.title), MB_TOPMOST | MB_ICONWARNING | translateButtons(state.buttons)
    ));
}
