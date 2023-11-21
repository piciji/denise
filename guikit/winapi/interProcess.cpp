
bool pInterProcess::anotherInstanceRunning = false;
pInterProcessParam* pInterProcess::param = nullptr;
HANDLE pInterProcess::fileMapping = nullptr;
Timer* pInterProcess::comTimer = nullptr;

auto pInterProcess::closeOtherInstances() -> void {
    if (!fileMapping) {
        if (Acquire()) {
            srand(time(NULL));
            if (!comTimer)
                comTimer = new Timer;

            comTimer->setData(rand());

            if (anotherInstanceRunning) {
                param->ident = comTimer->data();
                param->quitRequested = true;
            } else
                param->quitRequested = false;

            comTimer->setInterval(100);
            comTimer->onFinished = []() {
                pInterProcess::checkQuit();
            };
            comTimer->setEnabled();
        }
    }
}

auto pInterProcess::Acquire() -> bool {
    std::string szUniqueIdent = "process_" + Application::vendor + "_" + Application::name;

    fileMapping = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(pInterProcessParam), szUniqueIdent.c_str());

    if (fileMapping == nullptr)
        return false;

    if( ERROR_ALREADY_EXISTS == GetLastError() )
        anotherInstanceRunning = true;

    param = (pInterProcessParam*) MapViewOfFile( fileMapping, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);

    if (param == nullptr) {
        CloseHandle(fileMapping);
        fileMapping = nullptr;
        return false;
    }

    return true;
}

auto pInterProcess::checkQuit() -> void {
    if (!comTimer)
        return;

    if (param->quitRequested) {
        if (param->ident != pInterProcess::comTimer->data()) { // don't close itself
            param->quitRequested = false;
            comTimer->setEnabled(false);
            Application::onQuitRequest();
            return;
        }
    }
    comTimer->setEnabled();
}

auto pInterProcess::Release() -> void {
    if (comTimer) {
        comTimer->setEnabled(false);
       // delete comTimer;
       // comTimer = nullptr;
    }


    if (param)
        UnmapViewOfFile(param);

    if (fileMapping)
        CloseHandle(fileMapping);
}
