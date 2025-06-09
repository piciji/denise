
#include "agnus.h"

namespace LIBAMI {

auto Agnus::startHblank() -> void {
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

        if (vBlankOffset < lineVCounter)
            lineVCounter -= vBlankOffset;
        
        int width = denise.hiresFrame ? (LINE_MAX_WIDTH << 1) : LINE_MAX_WIDTH;
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

        paula.sampleUpdate();
        system->videoRefresh(frameBuffer + (vBlankOffset * LINE_BUFFER_WIDTH) + LINE_RENDER_OFFSET, width, lineVCounter,
                             LINE_BUFFER_WIDTH - width, laceFrame | (denise.hiresFrame ? 4 : 0));

        lineVCounter = 0;
        vBlankOffset = 0;
        secureRA = system->runAhead.pos == 1;
    } else if (!lineCallback.called && (lineVCounter >= lineCallback.line)) {
        denise.process();
        system->videoMidScreenCallback(laceFrame | (denise.hiresFrame ? 4 : 0) );
        lineCallback.called = true;
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
        startHblank();
        if (system->isProcessFrame())
            observeFrameDuration();
    }
}

auto Agnus::endHblank() -> void {
    denise.process();
    if (hBlank) {
        denise.linePos = 0;

        if (lineVCounter == 0) {
            crop.reset();

            denise.hiresFrame = (laceMode & 3) ? true : denise.hires; // can switch to hires mid-frame
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

            if (laceMode & 3)
                lineCallback.called = true; // no interlace support for threaded CPU renderer
            else
                lineCallback.called = !lineCallback.use /*|| !lineCallback.line*/;

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
    unsigned limit = denise.hiresFrame ? (LINE_MAX_WIDTH << 1) : LINE_MAX_WIDTH;

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

auto Agnus::switchToHiresMidframe() -> void {
    uint16_t* curLinePtr;
    unsigned xStart;
    int y = (laceFrame & 2) ? 1 : 0;
    int inc = (laceFrame & 3) ? 2 : 1;

    for (; y < lineVCounter; y = y + inc ) {
        curLinePtr = frameBuffer + y * LINE_BUFFER_WIDTH + LINE_RENDER_OFFSET;
        xStart = curLinePtr != denise.linePtr ? (LINE_MAX_WIDTH - 1) : (denise.linePos - 1);

        doubleLoresPixel( curLinePtr, xStart );
    }
    denise.linePos <<= 1;
}

inline auto Agnus::doubleLoresPixel(uint16_t* _ptr, unsigned _xStart) -> void {
    uint16_t p;
    for (int _x = _xStart; _x >= 0; _x--) {
        p = *( _ptr + _x );
        *( _ptr + (_x * 2 + 1) ) = p;
        *( _ptr + (_x * 2) ) = p;
    }
}

}
