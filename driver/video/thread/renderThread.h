
#pragma once
#include <thread>
#include <atomic>
#include <condition_variable>

#define RENDER_BUFFER_COUNT 2

namespace DRIVER {

    struct RenderBuffer {
        std::mutex sharedMutex;
        unsigned* data = nullptr;
        int32_t* dataInt = nullptr;
        float* dataFloat = nullptr;
        unsigned width = 0;
        unsigned height = 0;
        unsigned pitch = 0;
        bool updated = false;
        bool disallowShader = false;
    };

    struct RenderThread {

        RenderThread();
        virtual ~RenderThread();

        std::atomic<bool> ready;
        std::atomic<bool> kill;
        std::condition_variable cv;
        std::mutex accessMutex;

        RenderBuffer renderBuffers[RENDER_BUFFER_COUNT];
        RenderBuffer* lockedBuffer;
        uint8_t fillPos;
        uint8_t fetchPos;
        uint8_t frames;

        auto getBufferToDraw() -> RenderBuffer*;
        auto getBufferToRender() -> RenderBuffer*;
        auto getLastBufferToRender() -> RenderBuffer*;

        auto initWorker() -> void;
        auto deleteBuffer(RenderBuffer* buffer) -> void;
        auto enable(bool state) -> void;

        auto prepareBuffer(unsigned _width, unsigned _height) -> bool;
        auto lock(unsigned*& data, unsigned& pitch, unsigned _width, unsigned _height) -> bool;
        auto lock(int32_t*& data, unsigned& pitch, unsigned _width, unsigned _height) -> bool;
        auto lock(float*& data, unsigned& pitch, unsigned _width, unsigned _height) -> bool;
        auto unlock(bool disallowShader = false) -> void;
        auto reset() -> void;
        auto setThreadPriorityRealtime( std::thread& th ) -> void;
        auto wait() -> void;

        virtual auto refresh() -> void = 0;
        virtual auto resize(RenderBuffer* _buffer, unsigned _width, unsigned _height) -> void = 0;
        virtual auto calcPitch( unsigned _width ) -> unsigned { return _width; }
    };

}
