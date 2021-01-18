
auto pApplication::requestClipboardText() -> void {

    HANDLE hData;
    char* pszText;
    std::string out = "";

    if (!OpenClipboard(nullptr))
        return;

    hData = GetClipboardData(CF_TEXT);

    if (hData == nullptr) {
        CloseClipboard();
        return;
    }

    pszText = static_cast<char*>( GlobalLock(hData) );

    if (pszText)
        out.append(pszText);

    GlobalUnlock( hData );

    CloseClipboard();
	
	if (Application::onClipboardRequest)
		Application::onClipboardRequest( out );
}

auto pApplication::setClipboardText( std::string text ) -> void {
    HANDLE hData;
    HGLOBAL hMem;

    const char* in = text.c_str();

    const size_t len = strlen(in) + 1;

    hMem =  GlobalAlloc(GMEM_MOVEABLE, len);

    memcpy( GlobalLock(hMem), in, len );
    GlobalUnlock( hMem );

    if (!OpenClipboard(nullptr))
        return;

    EmptyClipboard();
    SetClipboardData( CF_TEXT, hMem );

    CloseClipboard();
}