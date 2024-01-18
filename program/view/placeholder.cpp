
auto View::loadDragnDropOverlay() -> void {
    for(int line = 0; line < 2; line++) {
        GUIKIT::Image mediaImage;
        GUIKIT::File file(program->imgFolder() + "mediaSlot" + std::to_string(line) + ".png");

        if (!file.open())
            return;

        uint8_t* data = file.read();

        if (!data)
            return;

        if (!mediaImage.loadPng(data, file.getSize()))
            return;

        videoDriver->setDragnDropOverlay(mediaImage.data, mediaImage.width, mediaImage.height, line);
    }
}

auto View::loadPlaceholder() -> void {
    bool splashScreen = globalSettings->get<bool>("splash_screen", true);
    if (!splashScreen)
        return;

	if (!placeholder.empty())
		return;
	
	GUIKIT::File file( program->imgFolder() + "startscreen.png" );
	
	if (!file.open())
		return;
	
	uint8_t* data = file.read();
	
	if (!data)
		return;	
	
	if (!placeholder.loadPng( data, file.getSize() ))
		return;

    VideoManager::placeHolderFrames = 84;
}

auto View::renderPlaceholder() -> bool {
    if (GUIKIT::Application::isQuit)
		return false;

    videoDriver->setLinearFilter( true );

	unsigned gpu_pitch;
    unsigned* gpu_data = 0;
    unsigned _w, _h;

    if (placeholder.empty())
        return false;

	uint8_t* data = placeholder.data;

    if (videoDriver->lock(gpu_data, gpu_pitch, placeholder.width, placeholder.height, DRIVER::OPT_DisallowShader)) {
        for (_h = 0; _h < placeholder.height; _h++) {
            for (_w = 0; _w < placeholder.width; _w++) {
                *gpu_data++ = data[0] << 16 | data[1] << 8 | data[2];
                data += 4;
            }
            gpu_data += gpu_pitch - (placeholder.width );
        }

        videoDriver->unlockAndRedraw();
    }

    return true;
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