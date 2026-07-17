
auto View::loadDragnDropOverlay() -> void {
    for(int line = 0; line < 2; line++) {
        GUIKIT::File file(program->imgFolder() + "mediaSlot" + std::to_string(line) + ".png");
        GUIKIT::Image& dndOverlay = dndOverlays[line];

        if (!file.open())
            return;

        uint8_t* data = file.read();

        if (!data)
            return;

        if (!dndOverlay.loadPng(data, file.getSize()))
            return;
    }

    DRIVER::Video::DnDOverlayCallback cb;
    cb = [this](DRIVER::Viewport& vp, unsigned line) -> uint8_t* {
        GUIKIT::Image& dndOverlay = dndOverlays[line];

        vp.height = (dndOverlay.height * vp.width) / dndOverlay.width;

        return dndOverlay.resize( vp.width, vp.height );
    };
    videoDriver->setDragnDropOverlayCallback( cb );
}

auto View::loadPlaceholder() -> void {
    bool splashScreen = globalSettings->get<bool>("splash_screen", true);
    if (!splashScreen)
        return;

	GUIKIT::File file( program->imgFolder() + "startscreen.png" );
	
	if (!file.open())
		return;
	
	uint8_t* data = file.read();
	
	if (!data)
		return;	

	if (!placeholder.loadPng( data, file.getSize() ))
		return;

    unsigned frames = dynamic_cast<LIBC64::Interface*>(program->getLastUsedEmu()) ? 110 : 220;

    DRIVER::Video::SplashscreenCallback cb;
    cb = [this](DRIVER::Viewport& vp, bool hide) -> uint8_t* {
        if (hide) {
            placeholder.free();
            return nullptr;
        }
        auto& winVp = videoDriver->getViewport();

        float targetRatio = (float)placeholder.width / (float)placeholder.height;

        vp.width = winVp.width;
        vp.height = static_cast<int>(winVp.width / targetRatio);

        if (vp.height > winVp.height) {
            vp.height = winVp.height;
            vp.width = static_cast<int>(winVp.height * targetRatio);
        }

        vp.x = (winVp.width - vp.width) / 2;
        vp.y = (winVp.height - vp.height) / 2;

        return placeholder.resize(vp.width, vp.height);
    };

    videoDriver->showSplashScreen(frames, cb);
}

auto View::cursorForPlaceholderInUpperTriangle(GUIKIT::Position p) -> int {

    DRIVER::Viewport& viewport = videoDriver->getViewport();
    signed _w = viewport.width;
    signed _h = viewport.height;

    if (p.x >= viewport.x)
        p.x -= viewport.x;
    else
        return -1;

    if (p.y >= viewport.y)
        p.y -= viewport.y;
    else
        return -1;

	if (p.x > _w || p.y > _h)
		return -1;

    GUIKIT::Position a(0,0);
    GUIKIT::Position b(_w * 1.55, 0);
    GUIKIT::Position c(0 , _h * 0.75);

    return (((a.y - b.y) * (p.x - a.x) + (b.x - a.x) * (p.y - a.y)) < 0 ||
    ((b.y - c.y) * (p.x - b.x) + (c.x - b.x) * (p.y - b.y)) < 0 ||
    ((c.y - a.y) * (p.x - c.x) + (a.x - c.x) * (p.y - c.y)) < 0) ? 0 : 1;
}

auto View::cursorForPlaceholderInUpperTriangle() -> int {

    return cursorForPlaceholderInUpperTriangle( viewport.getMousePosition() );
}