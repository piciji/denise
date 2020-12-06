
auto FPSCounter::countFrames() -> void {
    fpsCollect++;
    time( &curr_t );

    if (curr_t != prev_t) {
        fps = fpsCollect;
        fpsCollect = 0;
        
        statusHandler->setFpsCounterUpdate();

        if (!VideoManager::synchronized)
            // check input polling and message loop every 50 ms
            program->loopFrames = (fps * 50 ) / 1000;
        else
            // check input polling every frame
            program->loopFrames = 0;
    }
    prev_t = curr_t;
}

auto FPSCounter::init() -> void {
    
    fps = fpsCollect = 0;
}
