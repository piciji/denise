
#include "../tools/image.h"

namespace DRIVER {

struct SplashScreen {
    Image bitmap;
    std::atomic<unsigned> showFrames;
    float alpha;
    bool enable;
    bool toggleRGB_BGR = false;
    bool initialized = false;
    static constexpr float stepLength = 0.04f;

    enum Status { NO_UPDATE = 0, TEXTURE_UPDATE = 1, DATA_UPDATE = 2, FINISH = 3 };

    Viewport viewport;
    Video::SplashscreenCallback cb;

    SplashScreen(bool toggleRGB_BGR = false) {
        this->toggleRGB_BGR = toggleRGB_BGR;
        this->enable = false;
        this->initialized = false;
    }

    auto update(Viewport& _viewport) -> Status {
        Status status = NO_UPDATE;

        if ((viewport.width != _viewport.width) || (viewport.height != _viewport.height)) {
            viewport = _viewport;
            bitmap.scale( viewport.width, viewport.height );
            status = Status::TEXTURE_UPDATE;
        }

        if (!bitmap.scaledData) {
            finish();
            return Status::FINISH;
        }

        if (showFrames) {
            --showFrames;
        } else {
            if (!setAlpha()) {
                finish();
                return Status::FINISH;
            }
            status = Status::DATA_UPDATE;
        }

        return status;
    }

    auto setAlpha() -> bool {
        alpha -= stepLength;

        if (alpha <= 0.0f)
            return false;

        for (unsigned h = 0; h < bitmap.scaledHeight; h++) {
            uint8_t* src = bitmap.scaledData + h * (bitmap.scaledWidth * 4);

            for (unsigned w = 0; w < bitmap.scaledWidth; w++) {
                src[3] = alpha * 255.0;
                src += 4;
            }
        }

        return true;
    }

    auto finish() -> void {
        enable = false;
        cb();
    }

    auto setImage(uint8_t* _data, unsigned _width, unsigned _height, unsigned showFrames, Video::SplashscreenCallback cb) -> void {
        if (!initialized)
            return;
        this->cb = cb;
        this->showFrames = showFrames;
        bitmap.setData(_data, _width, _height, toggleRGB_BGR);
        alpha = 1.0;
        enable = true;
    }

    auto hide() -> void {
        if (enable) {
            showFrames = 0;
        }
    }
};

}
