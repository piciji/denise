
struct ProcessReference : public IUnknown {
    ProcessReference() : m_Thread(::GetCurrentThreadId()) {
        ::SHSetInstanceExplorer(this);
    }
    virtual ~ProcessReference() {
        ::SHSetInstanceExplorer(NULL);
        Release();
    }

    STDMETHODIMP_(ULONG) AddRef() {
        return ::InterlockedIncrement(&m_Ref);
    }

    STDMETHODIMP_(ULONG) Release() {
        const LONG ref = ::InterlockedDecrement(&m_Ref);

        if (!ref)
            ::PostThreadMessage(m_Thread, WM_NULL, 0, 0);

        return ref;
    }

    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) {
        if (riid == IID_IUnknown) {
            *ppv = static_cast<IUnknown*>(this);
            AddRef();
            return S_OK;
        }

        *ppv = NULL;
        return E_NOINTERFACE;
    }

private:
    LONG  m_Ref    = 1;
    DWORD m_Thread = 0;
};