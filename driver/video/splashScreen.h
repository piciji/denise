
namespace DRIVER {

struct SplashScreen {
    std::atomic<unsigned> showFrames;
    float alpha;
    bool enable;
    bool initialized = false;
    Video::SplashscreenCallback callback;
    static constexpr float stepLength = 0.04f;

    enum Status { NO_UPDATE = 0, TEXTURE_UPDATE = 1, DATA_UPDATE = 2, FINISH = 3 };

    Viewport prevViewport;
    Viewport viewport;
    uint8_t* screenData;

    SplashScreen() {
        this->enable = false;
        this->initialized = false;
        this->screenData = nullptr;
    }

    auto update(Viewport& _viewport) -> Status {
        Status status = NO_UPDATE;

        if ((prevViewport.width != _viewport.width) || (prevViewport.height != _viewport.height)) {
            prevViewport = _viewport;
            this->screenData = callback( viewport, false );
            status = Status::TEXTURE_UPDATE;
        }

        if (!screenData) {
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
            if (status == NO_UPDATE)
                status = Status::DATA_UPDATE;
        }

        return status;
    }

    auto setAlpha() -> bool {
        alpha -= stepLength;

        if (alpha <= 0.0f)
            return false;

        for (unsigned h = 0; h < viewport.height; h++) {
            uint8_t* src = screenData + h * (viewport.width * 4);

            for (unsigned w = 0; w < viewport.width; w++) {
                src[3] = alpha * 255.0;
                src += 4;
            }
        }

        return true;
    }

    auto finish() -> void {
        enable = false;
        this->screenData = callback( viewport, true );
    }

    auto prepare(unsigned frames, Video::SplashscreenCallback callback) -> void {
        if (!initialized)
            return;
        this->showFrames = frames;
        this->callback = callback;
        alpha = 1.0;
        enable = true;
    }

    auto isVisible() -> bool {
        return enable && showFrames > 0;
    }

    auto hide() -> void {
        if (enable) {
            showFrames = 0;
        }
    }
};

}
