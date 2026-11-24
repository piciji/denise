
//#include "agnus.h"

namespace LIBAMI {

#define DEBUG_SCROLL_MAX 30

auto Agnus::startHblankDebug() -> void {
    denise.process(beamCon & VARBEAMEN ? -1 : 0);
    bool _variableH = (beamCon & VARBEAMEN) && (hTotal != 0xe2);
    bool _variableV = (beamCon & VARBEAMEN) && (vTotal != (ntsc ? 261 : 311));

    int width = denise.lineWidthMultiplier() * LINE_DEBUG_WIDTH;
    if (_variableH)
        width = denise.linePos;

    unsigned _pixPerDma = denise.pixelPerDma();

    uint8_t _pos = hPos;
    for (int i = 0; i < width;) {
        std::memset( debugger.frameLine, DebuggerSnapshot::dmaModes[debugger.dma[_pos].usage].vector, _pixPerDma );
        i += _pixPerDma;
        debugger.frameLine += _pixPerDma;

        if (++_pos > debugger.lastHpos)
            _pos = 0;
    }

    if (vPos == 0 || vPos == vTotal) {
        unsigned _opt = 0;
        if (denise.frameMode == Denise::HIRES_FRAME) _opt = _variableH ? 0 : Crop16::HIRES;
        else if (denise.frameMode == Denise::SHRES_FRAME) _opt = _variableH ? Crop16::HIRES : Crop16::SHRES;
        if (_variableH) _opt |= Crop16::NON_STANDARD_H;
        if (_variableV) _opt |= Crop16::NON_STANDARD_V;

        if ((frameOptions & Crop16::HRES) != (_opt & Crop16::HRES))
            _opt |= Crop16::HRES_CHANGE;

        _opt |= frameOptions & (Crop16::LACE | Crop16::LACE_TOGGLE);

        if (vPos == (_variableH ? vTotal : 0)) {
            uint16_t* _ptr = frameBuffer + LINE_RENDER_OFFSET;
            unsigned _width = denise.lineWidthMultiplier() * LINE_MAX_WIDTH;
            if (_variableH) {
                _width = hTotal;
                if (hBStop > hBStrt) {
                    int dis = hBStop - hBStrt - 1;
                    if (dis < _width)
                        _width -= dis;
                }
                _width *= _pixPerDma;
            }

            unsigned _height = ntsc ? 244 : 289;
            if (_variableV) {
                _height = vTotal;
                if (vBStop > vBStrt) {
                    int dis = vBStop - vBStrt - 1;
                    if (dis < _height)
                        _height -= dis;
                }
            }
            _height = _height * (frameOptions & Crop16::LACE ? 2 : 1) - ((frameOptions & Crop16::LACE_EVEN) ? 1 : 0);
            unsigned _pitch = LINE_BUFFER_WIDTH - _width;
            debugger.crop.apply( _ptr, _width, _height, _pitch, _opt );

        }

        if (vPos == 0) {
            auto& _crop = debugger.crop;
            if (frameOptions & Crop16::LACE_EVEN)
                lineVCounter--;

            if (paula.ledChange & 0x80)
                paula.informPowerLED(false);

            paula.sampleUpdate();

            int offsetX = 0;
            int offsetY = 0;
            int offsetFrameX = 0;
            int offsetFrameY = 0;
            bool endDmaView = false;

            if (debugger.scrollDirection != 0) {
                if (((debugger.scrollDirection == 1) && (debugger.scrollCounter < DEBUG_SCROLL_MAX))
                || ((debugger.scrollDirection == -1) && debugger.scrollCounter)) {
                    if (_crop.latest.width < width) {
                        offsetX = width - _crop.latest.width;
                        offsetX = offsetX - (offsetX * debugger.scrollCounter) / DEBUG_SCROLL_MAX;
                    }

                    if (_crop.latest.height < lineVCounter) {
                        offsetY = lineVCounter - _crop.latest.height;
                        offsetY = offsetY - (offsetY * debugger.scrollCounter) / DEBUG_SCROLL_MAX;
                    }

                    if (width > _crop.original.width) {
                        unsigned lastLeft = (width - _crop.original.width) + _crop.latest.left;
                        if  (lastLeft)
                            offsetFrameX = lastLeft - ((lastLeft * debugger.scrollCounter) / DEBUG_SCROLL_MAX);
                    }

                    if (lineVCounter > _crop.original.height) {
                        unsigned lastTop = (lineVCounter - _crop.original.height) + _crop.latest.top;
                        if (lastTop)
                            offsetFrameY = lastTop - ((lastTop * debugger.scrollCounter) / DEBUG_SCROLL_MAX);
                    }

                    if (debugger.scrollDirection == 1)
                        debugger.scrollCounter++;
                    else
                        debugger.scrollCounter--;

                } else {
                   if (debugger.scrollDirection == -1)
                       endDmaView = true;

                   debugger.scrollDirection = 0;
                }
            }

            if (!endDmaView) {
                system->videoRefresh(frameBuffer + (offsetFrameY * LINE_BUFFER_WIDTH) + offsetFrameX, width - offsetX, lineVCounter - offsetY,
                    LINE_BUFFER_WIDTH - (width - offsetX ), _opt | Crop16::SUPPRESS_CROP);
            }

            Emulator::copyMasked<uint8_t>(frameOptions, _opt, Crop16::HRES);
            lineVCounter = 0;

            if (endDmaView) {
                debugger.dmaLog &= ~DmaLogView;
                hBlank = false;
                return;
            }

            if (denise.isShres()) denise.frameMode = Denise::SHRES_FRAME;
            else if (denise.isHires()) denise.frameMode = Denise::HIRES_FRAME;
            else if (laceMode & 3) denise.frameMode = Denise::HIRES_FRAME;
            else denise.frameMode = Denise::LORES_FRAME;

            if (!(frameOptions & Crop16::LACE) && (laceMode & Crop16::LACE)) {
                laceMode |= Crop16::LACE_TOGGLE_ON; // first lace frame
                denise.setDisableSequencer( 0 );
            } else if ((frameOptions & Crop16::LACE) && !(laceMode & Crop16::LACE)) {
                laceMode |= Crop16::LACE_TOGGLE_OFF; // first non lace frame
                denise.setDisableSequencer( 0 );
            }

            Emulator::copyMasked<uint8_t>(frameOptions, laceMode, Crop16::LACE | Crop16::LACE_TOGGLE);
            laceMode &= ~Crop16::LACE_TOGGLE;
            if (frameOptions & Crop16::LACE_ODD)
                lineVCounter = 1;
        }
    }

    if (lineVCounter >= LINE_BUFFER_HEIGHT)
        lineVCounter = LINE_BUFFER_HEIGHT - 1;

    debugger.frameLine = debugger.dmaFrame + lineVCounter * LINE_BUFFER_WIDTH;
    denise.linePtr = frameBuffer + lineVCounter * LINE_BUFFER_WIDTH;
    denise.linePos = 0;
    lineVCounter += (frameOptions & Crop16::LACE) ? 2 : 1;
}

auto Agnus::startHblank() -> void {
    if (debugger.dmaLog & DmaLogView)
        return startHblankDebug();

    bool _vblank = vBlank && !vBlankStart;

    if (!vPos) { // line 0
        // https://eab.abime.net/showpost.php?p=1682826&postcount=340

        if ((model == OCS) && !system->crop.active())
            _vblank = false; // without crop -> we want to see last shorter line
        if ((model == OCS_A1000) && system->crop.active())
            _vblank = true; // with crop -> we need to drop last shorter line to get same cropped image size like the other models
    }

    if (_vblank && lineVCounter) {
        denise.process(beamCon & VARBEAMEN ? -1 : 0);
        if (frameOptions & Crop16::LACE_EVEN)
            lineVCounter--;

        bool _variableH = (beamCon & VARBEAMEN) && (hTotal != 0xe2);
        bool _variableV = (beamCon & VARBEAMEN) && (vTotal != (ntsc ? 261 : 311));

        int width = denise.lineWidthMultiplier() * LINE_MAX_WIDTH;
        if (_variableH)
            width = denise.linePos;

        sanitizeCrop(width, lineVCounter);

		if (frameOptions & Crop16::LACE_TOGGLE_ON) { // prevents artifacts, when toggling interlace
		    if (frameOptions & Crop16::LACE_EVEN) {
		        for(unsigned h = 0; h < lineVCounter; h += 2)
		            std::memcpy(frameBuffer + LINE_RENDER_OFFSET + ((h + 1) * LINE_BUFFER_WIDTH), frameBuffer + LINE_RENDER_OFFSET + (h + 0) * LINE_BUFFER_WIDTH, LINE_BUFFER_WIDTH << 1 );
		    } else {
		        for(unsigned h = 0; h < lineVCounter; h += 2)
		            std::memcpy(frameBuffer + LINE_RENDER_OFFSET + ((h + 0) * LINE_BUFFER_WIDTH), frameBuffer + LINE_RENDER_OFFSET + (h + 1) * LINE_BUFFER_WIDTH, LINE_BUFFER_WIDTH << 1 );
		    }
		}

        if (paula.ledChange & 0x80)
            paula.informPowerLED(false);

        unsigned _opt = 0;
        if (denise.frameMode == Denise::HIRES_FRAME) _opt = _variableH ? 0 : Crop16::HIRES;
        else if (denise.frameMode == Denise::SHRES_FRAME) _opt = _variableH ? Crop16::HIRES : Crop16::SHRES;
        if (_variableH) _opt |= Crop16::NON_STANDARD_H;
        if (_variableV) _opt |= Crop16::NON_STANDARD_V;

        paula.sampleUpdate();
        checkLostSync();

        if ((frameOptions & Crop16::HRES) != (_opt & Crop16::HRES))
            _opt |= Crop16::HRES_CHANGE;

        _opt |= frameOptions & (Crop16::LACE | Crop16::LACE_TOGGLE);

        system->videoRefresh(frameBuffer + LINE_RENDER_OFFSET, width, lineVCounter,
            LINE_BUFFER_WIDTH - width, _opt);

        Emulator::copyMasked<uint8_t>(frameOptions, _opt, Crop16::HRES);

        lineVCounter = 0;
        secureRA = system->runAhead.pos == 1;
    }

    hBlank = !_vblank;
}

auto Agnus::endHblank() -> void {
    denise.process(beamCon & VARBEAMEN ? -1 : 0);

    if (debugger.dmaLog & DmaLogView)
        return;

    if (hBlank) {
        denise.linePos = 0;

        if (lineVCounter == 0) {
            crop.reset();

            if (denise.isShres()) denise.frameMode = Denise::SHRES_FRAME;
            else if (denise.isHires()) denise.frameMode = Denise::HIRES_FRAME;
            else if (laceMode & 3) denise.frameMode = Denise::HIRES_FRAME;
            else denise.frameMode = Denise::LORES_FRAME;

            // Denise doesn't need to know of interlace or vertical position.
            if (!(frameOptions & 3) && (laceMode & 3)) {
				laceMode |= Crop16::LACE_TOGGLE_ON; // first lace frame
                denise.setDisableSequencer( 0 );
            } else if ((frameOptions & 3) && !(laceMode & 3)) {
                laceMode |= Crop16::LACE_TOGGLE_OFF; // first non lace frame
                denise.setDisableSequencer( 0 );
            }

            Emulator::copyMasked<uint8_t>(frameOptions, laceMode, Crop16::LACE | Crop16::LACE_TOGGLE);
            laceMode &= ~Crop16::LACE_TOGGLE;

            if (frameOptions & Crop16::LACE_ODD)
                lineVCounter = 1;

            if (secureRA) {
                secureRA = false;
                if (!denise.useSequencer()) {
                    // when blitter blocks CPU a long time and we end here before RA ... TURNIPS-WorkForNothing_qdfix.adf
                    denise.setDisableSequencer(0);
                }
            }
        }

        if (lineVCounter >= LINE_BUFFER_HEIGHT) // could happen, if beam position has been changed
            lineVCounter = LINE_BUFFER_HEIGHT - 1;

        denise.linePtr = frameBuffer + lineVCounter * LINE_BUFFER_WIDTH + LINE_RENDER_OFFSET;
        lineVCounter += (frameOptions & Crop16::LACE) ? 2 : 1;
        hBlank = false;
    }
}

auto Agnus::checkLostSync() -> void {
    if (lineVCounter < ((frameOptions & Crop16::LACE) ? 300 : 150)) {
        // could happen, if beam position has been changed or uncontrolled register usage
        lineVCounter = (frameOptions & Crop16::LACE) ? 300 : 150; // otherwise video driver could crash
        std::memset(frameBuffer, 0, LINE_BUFFER_WIDTH * LINE_BUFFER_HEIGHT ); // lost sync
    } else if ((beamCon & BLANKEN) && (beamCon & VARHSYEN) && !hasActiveEvent<EVENT_BLANKEN>()) { // Alcatraz-Blitter ECS
        std::memset(frameBuffer, 0, LINE_BUFFER_WIDTH * LINE_BUFFER_HEIGHT ); // lost sync
    }
}

auto Agnus::updateCropLeft(int pos) -> void {
    if (lineVCounter >= LINE_CROP_TEST)
        crop.left = pos;
}

auto Agnus::updateCropRight(int pos) -> void {
    if (lineVCounter >= LINE_CROP_TEST)
        crop.right = pos;
}

auto Agnus::updateCropTop() -> void {
    if (!crop.top)
        crop.top = lineVCounter;
}

auto Agnus::updateCropBottom() -> void {
    if (!crop.bottom)
        crop.bottom = lineVCounter;
}

auto Agnus::sanitizeCrop(int width, int height) -> void {
    crop.bottom = (height > crop.bottom) ? (height - crop.bottom) : 0;
    crop.right = (width > crop.right) ? (width - crop.right) : 0;
    if (frameOptions & Crop16::LACE_ODD) {
        crop.bottom += 1;
        if (crop.top)
            crop.top -= 1;
    }

    width >>= 1;
    height >>= 1;

    if (!denise.useSequencer() ) {
        crop.reset();
    } else {
        if (crop.left > width) crop.left = 0;
        if (crop.right > width) crop.right = 0;
        if (crop.top > height) crop.top = 0;
        if (crop.bottom > height) crop.bottom = 0;
    }
}

template<bool quadruple> auto Agnus::doubleResMidframe(bool fromHires) -> void {
    if (debugger.dmaLog & DmaLogView)
        return doubleResMidDebugframe<quadruple>(fromHires);

    uint16_t* curLinePtr;
    unsigned xStart;
    int y = (frameOptions & Crop16::LACE_ODD) ? 1 : 0;
    int inc = (frameOptions & Crop16::LACE) ? 2 : 1;

    unsigned _lineWidth = LINE_MAX_WIDTH;
    if (fromHires)
        _lineWidth <<= 1;

    for (; y < lineVCounter; y = y + inc ) {
        curLinePtr = frameBuffer + y * LINE_BUFFER_WIDTH + LINE_RENDER_OFFSET;
        xStart = curLinePtr != denise.linePtr ? (_lineWidth - 1) : (denise.linePos - 1);

        if constexpr (quadruple)
            quadruplePixel<uint16_t>(curLinePtr, xStart);
        else
            doublePixel<uint16_t>( curLinePtr, xStart );
    }

    if constexpr (quadruple)
        denise.linePos <<= 2;
    else
        denise.linePos <<= 1;
}

template<bool quadruple> auto Agnus::doubleResMidDebugframe(bool fromHires) -> void {
    uint16_t* curLinePtr;
    uint8_t* curDmaPtr;
    unsigned xStart;
    int y = (frameOptions & Crop16::LACE_ODD) ? 1 : 0;
    int inc = (frameOptions & Crop16::LACE) ? 2 : 1;

    unsigned _lineWidth = LINE_DEBUG_WIDTH;
    if (fromHires)
        _lineWidth <<= 1;

    for (; y < lineVCounter; y = y + inc ) {
        curLinePtr = frameBuffer + y * LINE_BUFFER_WIDTH;
        curDmaPtr = debugger.dmaFrame + y * LINE_BUFFER_WIDTH;
        xStart = curLinePtr != denise.linePtr ? (_lineWidth - 1) : (denise.linePos - 1);

        if constexpr (quadruple) {
            quadruplePixel<uint16_t>(curLinePtr, xStart);
            quadruplePixel<uint8_t>(curDmaPtr, xStart);
        } else {
            doublePixel<uint16_t>(curLinePtr, xStart);
            doublePixel<uint8_t>(curDmaPtr, xStart);
        }
    }

    if constexpr (quadruple)
        denise.linePos <<= 2;
    else
        denise.linePos <<= 1;
}

template<typename T> auto Agnus::doublePixel(T* _ptr, unsigned _xStart) -> void {
    T p;
    for (int _x = _xStart; _x >= 0; _x--) {
        p = *( _ptr + _x );
        *( _ptr + (_x * 2 + 1) ) = p;
        *( _ptr + (_x * 2) ) = p;
    }
}

template<typename T> auto Agnus::quadruplePixel(T* _ptr, unsigned _xStart) -> void {
    T p;
    for (int _x = _xStart; _x >= 0; _x--) {
        p = *(_ptr + _x);
        
        *(_ptr + (_x * 4 + 3)) = p;
        *(_ptr + (_x * 4 + 2)) = p;
        *(_ptr + (_x * 4 + 1)) = p;
        *(_ptr + (_x * 4)) = p;
    }
}

}
