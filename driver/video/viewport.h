
#pragma once

#include "../driver.h"

namespace DRIVER {

struct ViewScreen {
    enum class Mode { Window = 0, Crt = 1, Native = 2, NativeFree = 3 } mode;

    struct {
        unsigned width = 0;
        unsigned height = 0;
        bool doubleSize = false;
    } scaling;

    bool hasIntegerScaling;
    unsigned windowWidth;
    unsigned windowHeight;
    bool flipped;

    ViewScreen() {
        mode = Mode::Window;
        hasIntegerScaling = false;
        windowWidth = 0;
        windowHeight = 0;
        scaling.height = 0;
        scaling.width = 0;
        scaling.doubleSize = false;
        flipped = false;
    }

    auto update(Viewport& viewport) {
        update(viewport, windowWidth, windowHeight);
    }

    auto update( Viewport& viewport, unsigned outputWidth, unsigned outputHeight ) -> void {
        windowWidth = outputWidth;
        windowHeight = outputHeight;

        unsigned outputTop = 0;
        unsigned outputLeft = 0;

        bool native = mode == Mode::Native;
        bool nativeFree = mode == Mode::NativeFree;
        bool crt = mode == Mode::Crt;
        if (native && flipped)
            nativeFree = true;

        bool useIntegerScaling = (hasIntegerScaling || native) && !flipped;
        bool fraction = scaling.height & 1;
        int scalingHeight = scaling.height;
        int scalingWidth = scaling.width;
        bool useDoubleSize = scaling.doubleSize && native && hasIntegerScaling;

        if (!useDoubleSize) {
            scalingHeight >>= 1;
            scalingWidth >>= 1;
        } else
            fraction = false;

        if ((scalingHeight == 0) || (outputHeight < scalingHeight)) {
            useIntegerScaling = false;
        }

        int _height = scalingHeight;
        int _width = scalingWidth;
        int factorH = 1;
        int factorW = 1;

        if (useIntegerScaling) {
            while (outputHeight > _height) {
                factorH++;
                _height = (factorH * scalingHeight) + (fraction ? (factorH >> 1) : 0);
            }

            while (_height > (float)outputHeight) {
                factorH--;
                _height = (factorH * scalingHeight) + (fraction ? (factorH >> 1) : 0);
            }

            outputTop = (outputHeight - _height) / 2;
            outputHeight = _height;
        }

        if (crt || nativeFree) {
            float _aspectWidth;
            float _aspectHeight;

            if (crt) {
                _aspectWidth = flipped ? 3.0 : 4.0;
                _aspectHeight = flipped ? 4.0 : 3.0;

                while(1) {
                    _height = outputHeight;
                    _width = (unsigned)(((float(_height) / _aspectHeight) * _aspectWidth) + 0.5);

                    if (_width > outputWidth) {
                        if (useIntegerScaling) {
                            _height = outputHeight - scalingHeight;

                            if (_height >= scalingHeight) {
                                outputTop += (outputHeight - _height) / 2;
                                outputHeight = _height;
                                continue;
                            }
                        }

                        _height = (unsigned)(((float(outputWidth) / _aspectWidth) * _aspectHeight) + 0.5);
                        outputLeft = 0;
                        outputTop += (outputHeight - _height) / 2;
                        outputHeight = _height;

                    } else {
                        outputLeft = (outputWidth - _width) / 2;
                        outputWidth = _width;
                    }

                    break;
                }

            } else { // native free
                _aspectWidth = flipped ? scaling.height : scaling.width;
                _aspectHeight = flipped ? scaling.width : scaling.height;

                float screenAspect = (float)windowWidth / (float)outputHeight;
                float nativeAspect = _aspectWidth / _aspectHeight;

                float scaleX, scaleY;
                if (nativeAspect < screenAspect) {
                    scaleX = nativeAspect / screenAspect;
                    scaleY = 1.0f;
                } else {
                    scaleX = 1.0f;
                    scaleY = screenAspect / nativeAspect;
                }

                outputWidth = windowWidth * scaleX;
                outputHeight = outputHeight * scaleY;
                outputLeft = (windowWidth - outputWidth) / 2.0;
                outputTop = (windowHeight - outputHeight) / 2.0;
            }
        } else if (native) { // Native
            if (_width > outputWidth)
                _width = outputWidth;
            else {
                while (outputWidth > _width) {
                    if (factorW >= factorH)
                        break;

                    factorW++;
                    _width += scalingWidth;
                }

                while (_width > outputWidth) {
                    factorW--;
                    _width -= scalingWidth;
                }

                while(factorW < factorH) {
                    if (factorH > 1) {
                        factorH--;
                        _height = (factorH * scalingHeight) + (fraction ? (factorH >> 1) : 0);
                    } else
                        break;

                    outputTop = (windowHeight - _height) / 2;
                    outputHeight = _height;
                }
            }

            outputLeft = (outputWidth - _width) / 2;
            outputWidth = _width;
        }

        viewport.x = outputLeft;
        viewport.y = outputTop;
        viewport.width = outputWidth;
        viewport.height = outputHeight;
    }
};

}
