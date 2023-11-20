
int pInterProcess::fd = -1;
sem_t* pInterProcess::semptr = nullptr;
unsigned char* pInterProcess::memptr = nullptr;
GUIKIT::Timer pInterProcess::comTimer;

auto pInterProcess::closeOtherInstances() -> void {
    if (Acquire()) {
        srand(time(NULL));
        comTimer.setData(rand());
        unsigned val = comTimer.data();
        memptr[1] = val & 0xff;
        memptr[2] = (val >> 8) & 0xff;
        memptr[3] = (val >> 16) & 0xff;
        memptr[4] = (val >> 24) & 0xff;

        memptr[0] = 1;
        sem_post(semptr);

        comTimer.setInterval(100);
        comTimer.onFinished = []() {
            pInterProcess::checkQuit();
        };
        comTimer.setEnabled();
    }
}

auto pInterProcess::Acquire() -> bool {
    if (fd >= 0)
        return true;

    std::string szUniqueFile = "file_" + Application::vendor + "_" + Application::name;
    std::string szUniqueIdent = "process_" + Application::vendor + "_" + Application::name;

    fd = shm_open(szUniqueFile.c_str(), O_RDWR | O_CREAT, 0644);
    if (fd < 0)
        return false;

    int res = ftruncate(fd, 5);

    memptr = (unsigned char*)mmap(NULL, 5, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (!memptr)
        return false;

    semptr = sem_open(szUniqueIdent.c_str(), O_CREAT, 0644, 0);
    if (!semptr) {
//        if (errno == EEXIST)
//            anotherInstanceRunning = true;
//
//        semptr = sem_open(szUniqueIdent.c_str(), O_CREAT, 0644, 0);
//
//        if (!semptr)
            return false;
    }

    return true;
}

auto pInterProcess::checkQuit() -> void {
    if (!memptr || !semptr)
        return;

    if (!sem_wait(semptr)) {
        if (memptr[0]) {
            unsigned val = (memptr[1] << 0) | (memptr[2] << 8) | (memptr[3] << 16) | (memptr[4] << 24);
            if (val != pInterProcess::comTimer.data()) { // don't close itself
                memptr[0] = 0;
                sem_post(semptr);
                comTimer.setEnabled(false);
                Application::onQuitRequest();
                return;
            }
        }
        sem_post(semptr);
    }
    comTimer.setEnabled();
}

auto pInterProcess::Release() -> void {
    comTimer.setEnabled(false);
    comTimer.onFinished = nullptr;
    munmap(memptr, 5);
    memptr = nullptr;

    if (fd >= 0)
        close(fd);

    fd = -1;

    if (semptr)
        sem_close(semptr);

    semptr = nullptr;
}