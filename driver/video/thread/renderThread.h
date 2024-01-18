
#pragma once
#include <thread>
#include <atomic>
#include <condition_variable>

#define RENDER_BUFFER_COUNT 2

namespace DRIVER {

    struct RenderBuffer {
        std::mutex sharedMutex;
        unsigned* data = nullptr;
        float* dataFloat = nullptr;
        bool floatFormat = false;
        unsigned width = 0;
        unsigned height = 0;
        unsigned pitch = 0;
        uint8_t options = 0;
    };

    struct RenderThread {

        RenderThread();
        virtual ~RenderThread();

        std::atomic<bool> ready;
        std::atomic<bool> kill;
#ifdef __APPLE__
        std::atomic<bool> updatePriority;
        std::atomic<bool> realtime;
#endif
        std::condition_variable cv;
        std::mutex accessMutex;

        std::mutex resizeMutex;
        std::mutex resizeMutexThreaded;

        RenderBuffer renderBuffers[RENDER_BUFFER_COUNT];
        RenderBuffer* lockedBuffer;
        uint8_t fillPos;
        uint8_t fetchPos;
        uint8_t frames;

        auto getBufferToDraw() -> RenderBuffer*;
        auto getBufferToRender() -> RenderBuffer*;
        auto getReuseBuffer(unsigned& _width, unsigned& _height) -> RenderBuffer*;
        auto getLastBufferToRender() -> RenderBuffer*;

        auto initWorker() -> void;
        auto deleteBuffer(RenderBuffer* buffer) -> void;
        auto enable(bool state) -> void;
        auto changePriorityToRealtime(bool state) -> void;

        auto prepareBuffer(unsigned _width, unsigned _height, bool floatFormat) -> bool;
        auto lock(unsigned*& data, unsigned& pitch, unsigned _width, unsigned _height, uint8_t options) -> bool;
        auto lock(float*& data, unsigned& pitch, unsigned _width, unsigned _height, uint8_t options) -> bool;

        auto unlock() -> void;
        auto reset() -> void;
        auto wait() -> void;

        virtual auto refresh() -> void = 0;
        virtual auto resize(RenderBuffer* _buffer, unsigned _width, unsigned _height) -> void = 0;
        virtual auto calcPitch( unsigned _width ) -> unsigned { return _width; }
    };

}
