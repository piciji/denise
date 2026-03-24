
//#include "agnus.h"

namespace LIBAMI {

#define DEBUG_SCROLL_MAX 30

auto Agnus::startHblankDebug() -> void {
    denise.process();

    int width = denise.lineWidthMultiplier() * LINE_DEBUG_WIDTH;
    unsigned _pixPerDma = denise.pixelPerDma();

    uint8_t _pos = hPos;
    for (int i = 0; i < width;) {
        std::memset( debugger.frameLine, DebuggerSnapshot::dmaModes[debugger.dma[_pos].usage].vector, _pixPerDma );
        i += _pixPerDma;
        debugger.frameLine += _pixPerDma;

        if (++_pos > debugger.lastHpos)
            _pos = 0;
    }

    if (vPos == 0) {
        auto& _crop = debugger.crop;
        if (laceFrame & 1)
            lineVCounter--;

        if (paula.ledChange & 0x80)
            paula.informPowerLED(false);

        paula.sampleUpdate();

        uint8_t _res = 0;
        if (denise.frameMode == Denise::HIRES_FRAME) _res = 4;
        else if (denise.frameMode == Denise::SHRES_FRAME) _res = 8;

        // fprintf( stderr, "v %i h %i f %i ", lineVCounter, denise.linePos, laceFrame | _res );

        unsigned _width = denise.lineWidthMultiplier() * LINE_MAX_WIDTH;
        unsigned _height = (ntsc ? 244 : 289) * (laceFrame & 3 ? 2 : 1) - ((laceFrame & 1) ? 1 : 0);
        unsigned _pitch = LINE_BUFFER_WIDTH - width;
        uint16_t* _ptr = frameBuffer + LINE_RENDER_OFFSET;
        _crop.apply( _ptr, _width, _height, _pitch, laceFrame | _res );

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
                LINE_BUFFER_WIDTH - (width - offsetX ), laceFrame | _res);
        }

        lineVCounter = 0;

        if (endDmaView) {
            debugger.dmaView = false;
            debugger.dmaLog = debugger.requestDmaLog;
            hBlank = false;
            return;
        }

        if (denise.isShres()) denise.frameMode = Denise::SHRES_FRAME;
        else if (denise.isHires()) denise.frameMode = Denise::HIRES_FRAME;
        else if (laceMode & 3) denise.frameMode = Denise::HIRES_FRAME;
        else denise.frameMode = Denise::LORES_FRAME;

        if (hTotalChanged) {
            hTotalChanged = false;
            std::memset(frameBuffer, 0, LINE_BUFFER_WIDTH * LINE_BUFFER_HEIGHT );
        } else if (!(laceFrame & 3) && (laceMode & 3)) {
            laceMode |= 0x80; // first lace frame
            denise.setDisableSequencer( 0 );
        } else if ((laceFrame & 3) && !(laceMode & 3)) {
            laceMode |= 0x40; // first non lace frame
            denise.setDisableSequencer( 0 );
        }

        laceFrame = laceMode;
        if (laceFrame & 2)
            lineVCounter = 1;
    }

    if (lineVCounter >= LINE_BUFFER_HEIGHT)
        lineVCounter = LINE_BUFFER_HEIGHT - 1;

    debugger.frameLine = debugger.dmaFrame + lineVCounter * LINE_BUFFER_WIDTH;
    denise.linePtr = frameBuffer + lineVCounter * LINE_BUFFER_WIDTH;
    denise.linePos = 0;
    lineVCounter += (laceFrame & 3) ? 2 : 1;
}

auto Agnus::startHblank() -> void {
    if (debugger.dmaView)
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
        denise.process();
        if (laceFrame & 1)
            lineVCounter--;

        if (lineVCounter < ((laceFrame & 3) ? 300 : 150)) {// could happen, if beam position has been changed or uncontrolled register usage
            lineVCounter = (laceFrame & 3) ? 300 : 150; // otherwise video driver could crash
            std::memset(frameBuffer, 0, LINE_BUFFER_WIDTH * LINE_BUFFER_HEIGHT ); // lost sync
        } else if (lineVCounter > ((laceFrame & 3) ? 600 : 300) ) {
            lineVCounter = (laceFrame & 3) ? 600 : 300;
        }
        
        if (laceFrame & 3)
            vBlankOffset <<= 1;

        // ECS Worms VBI use programmed vblank from v 2 - 5. The current programming would increase the number of visible lines beyond what is possible.
        // This needs to be compensated for. todo: solve this better

        if (vBlankOffset < lineVCounter)
            lineVCounter -= vBlankOffset;
        
        int width = denise.lineWidthMultiplier() * LINE_MAX_WIDTH;
        sanitizeCrop(width, lineVCounter);

		if (laceFrame & 0x80) {
		    if (laceFrame & 1) {
		        for(unsigned h = 0; h < lineVCounter; h += 2)
		            std::memcpy(frameBuffer + LINE_RENDER_OFFSET + ((h + 1) * LINE_BUFFER_WIDTH), frameBuffer + LINE_RENDER_OFFSET + (h + 0) * LINE_BUFFER_WIDTH, LINE_BUFFER_WIDTH << 1 );
		    } else {
		        for(unsigned h = 0; h < lineVCounter; h += 2)
		            std::memcpy(frameBuffer + LINE_RENDER_OFFSET + ((h + 0) * LINE_BUFFER_WIDTH), frameBuffer + LINE_RENDER_OFFSET + (h + 1) * LINE_BUFFER_WIDTH, LINE_BUFFER_WIDTH << 1 );
		    }
		}

        if (paula.ledChange & 0x80)
            paula.informPowerLED(false);

        uint8_t _res = 0;
        if (denise.frameMode == Denise::HIRES_FRAME) _res = 4;
        else if (denise.frameMode == Denise::SHRES_FRAME) _res = 8;

        paula.sampleUpdate();
        system->videoRefresh(frameBuffer + (vBlankOffset * LINE_BUFFER_WIDTH) + LINE_RENDER_OFFSET, width, lineVCounter,
            LINE_BUFFER_WIDTH - width, laceFrame | _res);

        lineVCounter = 0;
        vBlankOffset = 0;
        secureRA = system->runAhead.pos == 1;
    }

    if (_vblank) {
        hBlank = false;
    } else {
        if (beamCon & VARHSYEN) { // Alcatraz-Blitter ECS
            int _ht = (beamCon & VARBEAMEN) ? (hTotal + lol) : (0xe2 + lol);

            if ((hsStop > _ht) || (hsStop < hsStrt)) {
                denise.process();
                std::memset(denise.linePtr, 0, LINE_BUFFER_WIDTH << 1);
            }
        }
        hBlank = true;
    }

}

auto Agnus::startHsync() -> void {
    // needed if vposw write misses vblank start
    bool state = false;

    if (ntsc) {
        if (vPos == 3)
            state = true;
    } else {
        if (lof && vPos == 3) state = true;
        else if (!lof && vPos == 2) state = true;
    }

    if (state) {
        vBlank = true;
        vBlankStart = true;
        if (!debugger.dmaView)
            startHblank();
        if (system->isProcessFrame())
            observeFrameDuration();
    }

    if (denise.extblanken && ecs()) {
        if ((beamCon & BLANKEN) == 0)
            denise.csync(csyncPolTrue(true));
    }
}

auto Agnus::endHblank() -> void {
    denise.process();

    if (debugger.dmaView)
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
            if (hTotalChanged) {
                hTotalChanged = false;
                std::memset(frameBuffer, 0, LINE_BUFFER_WIDTH * LINE_BUFFER_HEIGHT );
            } else if (!(laceFrame & 3) && (laceMode & 3)) {
				laceMode |= 0x80; // first lace frame
                denise.setDisableSequencer( 0 );
            } else if ((laceFrame & 3) && !(laceMode & 3)) {
                laceMode |= 0x40; // first non lace frame
                denise.setDisableSequencer( 0 );
            }

            laceFrame = laceMode;
            if (laceFrame & 2)
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
        lineVCounter += (laceFrame & 3) ? 2 : 1;
        hBlank = false;
    }
}

auto Agnus::updateCropLeft(int pos) -> void {
    if (lineVCounter == LINE_CROP_TEST)
        crop.left = pos;
}

auto Agnus::updateCropRight(int pos) -> void {
    if (lineVCounter != LINE_CROP_TEST)
        return;

    unsigned diff = 0;
    unsigned limit = LINE_MAX_WIDTH;
    if (denise.frameMode == Denise::SHRES_FRAME) limit <<= 2;
    else if (denise.frameMode == Denise::HIRES_FRAME) limit <<= 1;

    if (limit > pos)
        diff = limit - pos;

    crop.right = diff;
}

auto Agnus::updateCropTop() -> void {
    if (!crop.top) {
        crop.top = lineVCounter;
        if (crop.top && (laceFrame & 2))
            crop.top -= 1;
    }
}

auto Agnus::updateCropBottom() -> void {
    if (!crop.bottom)
        crop.bottom = lineVCounter;
    if (crop.bottom && (laceFrame & 2))
        crop.bottom -= 1;
}

auto Agnus::sanitizeCrop(int width, int height) -> void {
    crop.bottom = (height > crop.bottom) ? (height - crop.bottom) : 0;

    width >>= 1;
    height >>= 1;

    if (!denise.useSequencer() || (laceFrame & 3) ) {
        crop.reset();
    } else {
        if (crop.left > width) crop.left = 0;
        if (crop.right > width) crop.right = 0;
        if (crop.top > height) crop.top = 0;
        if (crop.bottom > height) crop.bottom = 0;
    }
}

template<bool quadruple> auto Agnus::doubleResMidframe(bool fromHires) -> void {
    if (debugger.dmaView)
        return doubleResMidDebugframe<quadruple>(fromHires);

    uint16_t* curLinePtr;
    unsigned xStart;
    int y = (laceFrame & 2) ? 1 : 0;
    int inc = (laceFrame & 3) ? 2 : 1;

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
    int y = (laceFrame & 2) ? 1 : 0;
    int inc = (laceFrame & 3) ? 2 : 1;

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
