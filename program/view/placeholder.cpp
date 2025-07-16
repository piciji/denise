
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
    VideoManager::placeHolderSplashScreen = true;
}

auto View::renderPlaceholder(uint8_t gpuOptions) -> bool {
    unsigned _width, _height;
    uint8_t* _data;
    unsigned gpu_pitch;
    unsigned* gpu_data = nullptr;
    float* gpu_data_float = nullptr;
    unsigned _w, _h;
    ColorLumaChroma clc;
    ColorRgb rgb;
    bool useImgViewer = imageViewer && imageViewer->overrideImage.data;
    gpuOptions &= (DRIVER::OPT_DisallowShader | DRIVER::OPT_TakeScreenshot);

    if (GUIKIT::Application::isQuit)
        return false;

    if (useImgViewer) {
        _width = imageViewer->overrideImage.width;
        _height = imageViewer->overrideImage.height;
        _data = imageViewer->overrideImage.data;
    } else if (!placeholder.empty()) {
        _width = placeholder.width;
        _height = placeholder.height;
        _data = placeholder.data;

        videoDriver->setLinearFilter( true );
        gpuOptions |= (uint8_t)DRIVER::OPT_DisallowShader;
    } else
        return false;

    if (useImgViewer && (activeVideoManager->crtMode == VideoManager::CrtMode::Gpu) && activeVideoManager->shaderLumaChromaInput()) {
        if (videoDriver->lock(gpu_data_float, gpu_pitch, _width, _height, gpuOptions)) {
            for (_h = 0; _h < _height; _h++) {
                for (_w = 0; _w < _width; _w++) {
                    rgb.r = _data[0];
                    rgb.g = _data[1];
                    rgb.b = _data[2];
                    VideoManager::convertRGBToYUV(&clc, &rgb);

                    *gpu_data_float++ = clc.y / 255.0;
                    *gpu_data_float++ = clc.u_i / 255.0;
                    *gpu_data_float++ = clc.v_q / 255.0;
                    *gpu_data_float++ = 0;

                    _data += 4;
                }
                gpu_data_float += gpu_pitch - _width;
            }

            videoDriver->unlockAndRedraw();
        }
    } else if (videoDriver->lock(gpu_data, gpu_pitch, _width, _height, gpuOptions)) {
        for (_h = 0; _h < _height; _h++) {
            for (_w = 0; _w < _width; _w++) {
                *gpu_data++ = _data[0] << 16 | _data[1] << 8 | _data[2];
                _data += 4;
            }
            gpu_data += gpu_pitch - _width;
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