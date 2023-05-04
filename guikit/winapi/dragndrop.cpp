
DropManager::DropManager(pWidget* refWidget) : m_cRef(1), refWidget(refWidget) {
    pApplication::g_pProcRef.AddRef();
}

DropManager::~DropManager() {
    pApplication::g_pProcRef.Release();
}

HRESULT STDMETHODCALLTYPE DropManager::QueryInterface(REFIID riid, void **ppv) {
    if (riid == IID_IUnknown || riid == IID_IDropTarget) {
        *ppv = static_cast<IUnknown*>(this);
        AddRef();
        return S_OK;
    }
    *ppv = NULL;
    return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE DropManager::AddRef() {
    return InterlockedIncrement(&m_cRef);
}

ULONG STDMETHODCALLTYPE DropManager::Release() {
    LONG cRef = InterlockedDecrement(&m_cRef);
    if (cRef == 0)
        delete this;
    return cRef;
}

HRESULT STDMETHODCALLTYPE DropManager::DragEnter(IDataObject* pdto, DWORD grfKeyState, POINTL ptl, DWORD* pdwEffect) {
    std::vector<std::string> paths;
    setPaths(pdto, paths);

    refWidget->callDragEnter(paths);
    *pdwEffect &= DROPEFFECT_COPY;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE DropManager::DragOver(DWORD grfKeyState, POINTL ptl, DWORD* pdwEffect) {
    refWidget->callDragMove(ptl);
    *pdwEffect &= DROPEFFECT_COPY;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE DropManager::DragLeave() {
    refWidget->callDragLeave();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE DropManager::Drop(IDataObject* pdto, DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect) {
    std::vector<std::string> paths;
    setPaths(pdto, paths);

    refWidget->callDrops(paths);
    *pdwEffect &= DROPEFFECT_COPY;
    return S_OK;
}

auto DropManager::setPaths(IDataObject* pdto, std::vector<std::string>& paths) -> void {
    FORMATETC fmte = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    STGMEDIUM stgm;

    if (SUCCEEDED(pdto->GetData(&fmte, &stgm))) {
        HDROP hdrop = (HDROP)stgm.hGlobal;
        auto fileCount = DragQueryFile(hdrop, ~0, NULL, 0);

        for(unsigned n = 0; n < fileCount; n++) {
            auto length = DragQueryFile(hdrop, n, nullptr, 0);
            auto buffer = new wchar_t[length + 1];

            if(DragQueryFile(hdrop, n, buffer, length + 1)) {
                std::string path = utf8_t(buffer);
                std::replace( path.begin(), path.end(), '\\', '/');

                if (File::isDir(path) && path.back() != '/') path.push_back('/');
                paths.push_back(path);
            }

            delete[] buffer;
        }

        ReleaseStgMedium(&stgm);
    }
}