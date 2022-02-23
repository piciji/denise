
// G-Sync, FreeSync support (VRR variable refresh rate)

auto OpenGL::initVRR(float speed) -> void {

    minimumCapTime = (1000000.0 / speed) + 0.5;

    lastCapTime = Chronos::getTimestampInMicroseconds();
}

auto OpenGL::waitVRR() -> void {

    lastCapTime += minimumCapTime;
    int64_t remaining  = lastCapTime - Chronos::getTimestampInMicroseconds();

    if (remaining <= 0) {
        lastCapTime = Chronos::getTimestampInMicroseconds();
        return;
    }

    if (remaining >= 3000) {

        remaining -= 1500;

        unsigned sleepInMilli = (unsigned) ((float) remaining / 1000.0);

#ifdef DRV_WGL
        Sleep(sleepInMilli);
#else
        usleep( sleepInMilli * 1000 )
#endif

        remaining = lastCapTime - Chronos::getTimestampInMicroseconds();
    }

    // we need exact frame pacing
    while(remaining > 0) {
        //std::this_thread::yield();
        remaining = lastCapTime - Chronos::getTimestampInMicroseconds();
    }
}
