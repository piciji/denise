
auto OpenGL::calcViewport() -> void {
    outputWidth = windowWidth;
    outputHeight = windowHeight;
    outputTop = 0;
    outputLeft = 0;

    bool integerScaling = settings.integerScaling || (settings.aspectMode == 2);
    int _height = integerScalingHeight;

    if ((integerScalingHeight == 0) || (outputHeight < integerScalingHeight))
        integerScaling = false;

    if (integerScaling) {
        while (outputHeight > _height)
            _height += integerScalingHeight;

        while (_height > outputHeight)
            _height -= integerScalingHeight;

        outputTop = (outputHeight - _height) / 2;
        outputHeight = _height;
    }

    if (settings.aspectMode == 1) {
        float _aspectWidth = 4.0;
        float _aspectHeight = 3.0;

        while(1) {
            _height = outputHeight;
            int _width = (unsigned)(((float(_height) / _aspectHeight) * _aspectWidth) + 0.5);

            if (_width > outputWidth) {
                if (integerScaling) {
                    _height = outputHeight - integerScalingHeight;

                    if (_height >= integerScalingHeight) {
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
    } else if (settings.aspectMode == 2) { // Native
        int _width = integerScalingWidth;

        if (_width > outputWidth)
            _width = outputWidth;
        else {
            while (outputWidth > _width)
                _width += integerScalingWidth;

            while (_width > outputWidth)
                _width -= integerScalingWidth;
        }

        outputLeft = (outputWidth - _width) / 2;
        outputWidth = _width;
    }

    viewport.x = outputLeft;
    viewport.y = outputTop;
    viewport.width = outputWidth;
    viewport.height = outputHeight;
}