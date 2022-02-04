
auto View::loadPlaceholder() -> void {

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
}

auto View::renderPlaceholder(bool blackScreen) -> void {

	if (cmd->debug || program->isRunning || GUIKIT::Application::isQuit)
		return;
	
	unsigned gpu_pitch;
    unsigned* gpu_data = 0;
    unsigned _w, _h;
	uint8_t* data = placeholder.data;	
	
	if (!blackScreen && !placeholder.empty()) {
		if (videoDriver->lock(gpu_data, gpu_pitch, placeholder.width, placeholder.height)) {

			for (_h = 0; _h < placeholder.height; _h++) {
				for (_w = 0; _w < placeholder.width; _w++) {
					*gpu_data++ = data[0] << 16 | data[1] << 8 | data[2];
					data += 4;
				}
				gpu_data += gpu_pitch - (placeholder.width );
			}

            videoDriver->unlockAndRedraw(true, true);
		}
	} else { // blackscreen
		if (videoDriver->lock(gpu_data, gpu_pitch, 256, 256)) {

			for (_h = 0; _h < 256; _h++) {
				for (_w = 0; _w < 256; _w++) {
					*gpu_data++ = 0;
				}
				gpu_data += gpu_pitch - 256;
			}

            videoDriver->unlockAndRedraw(true, true);
		}
	}
}

auto View::cursorForPlaceholderInUpperTriangle(GUIKIT::Position& p) -> bool {
    
    signed _w = viewport.geometry().width;
    signed _h = viewport.geometry().height;

    GUIKIT::Position a(0,0);
    GUIKIT::Position b(_w * 1.55, 0);
    GUIKIT::Position c(0 , _h * 0.75);

    return (((a.y - b.y) * (p.x - a.x) + (b.x - a.x) * (p.y - a.y)) < 0 ||
    ((b.y - c.y) * (p.x - b.x) + (c.x - b.x) * (p.y - b.y)) < 0 ||
    ((c.y - a.y) * (p.x - c.x) + (a.x - c.x) * (p.y - c.y)) < 0) ? false : true;        
}

auto View::cursorForPlaceholderInUpperTriangle() -> bool {

    return cursorForPlaceholderInUpperTriangle( viewport.getMousePosition() );
}