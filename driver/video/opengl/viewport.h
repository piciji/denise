
auto OpenGL::calcViewport() -> void {
    outputWidth = windowWidth;
    outputHeight = windowHeight;
    outputTop = 0;
    outputLeft = 0;

    bool integerScaling = settings.integerScaling;
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

    if (settings.aspectWidth != 1.0) {
        float _aspectWidth = settings.aspectWidth;
        float _aspectHeight = settings.aspectHeight;

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
    }

    viewport.x = outputLeft;
    viewport.y = outputTop;
    viewport.width = outputWidth;
    viewport.height = outputHeight;
}