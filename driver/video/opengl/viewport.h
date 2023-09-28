
auto OpenGL::calcViewport() -> void {
    outputWidth = windowWidth;
    outputHeight = windowHeight;
    outputTop = 0;
    outputLeft = 0;

    bool native = settings.aspectMode == 2;
    bool crt = settings.aspectMode == 1;
    bool useIntegerScaling = settings.integerScaling || native;
    bool fraction = integerScaling.height & 1;
    int scalingHeight = integerScaling.height;
    int scalingWidth = integerScaling.width;
    bool useDoubleSize = integerScaling.doubleSize && native && settings.integerScaling;

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

    if (crt) {
        float _aspectWidth = 4.0;
        float _aspectHeight = 3.0;

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

                outputTop = (outputHeight - _height) / 2;
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