
bool pInterProcess::anotherInstanceRunning = false;
int pInterProcess::fd = -1;
sem_t* pInterProcess::semptr = nullptr;
caddr_t pInterProcess::memptr = nullptr;
GUIKIT::Timer pInterProcess::comTimer;

auto pInterProcess::closeOtherInstances() -> void {
    if (Acquire()) {
        srand(time(NULL));
        comTimer.setData(rand());

        if (!sem_wait(semptr)) {
            if (anotherInstanceRunning) {
                int val = comTimer.data();
                std::memcpy(memptr[1], &val, 4);
                memptr[0] = 1;
            } else
                memptr[0] = 0;

            sem_post(semptr);
        }

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

    ftruncate(fd, 5);

    memptr = mmap(NULL, 5, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if ((caddr_t) -1 == memptr)
        return false;

    semptr = sem_open(szUniqueIdent.c_str(), O_CREAT | O_EXCL, 0644, 0);
    if (semptr == (void*) -1) {
        if (errno == EEXIST)
            anotherInstanceRunning = true;

        semptr = sem_open(szUniqueIdent.c_str(), O_CREAT, 0644, 0);

        if (semptr == (void*) -1)
            return false;
    }

    return true;
}

auto pInterProcess::checkQuit() -> void {

    if (!sem_wait(semptr)) {
        if (memptr[0]) {
            int val = memptr[1] | (memptr[2] << 8) | (memptr[3] << 16) | (memptr[4] << 24);
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

    if (fd >= 0)
        close(fd);

    if (semptr)
        sem_close(semptr);
}