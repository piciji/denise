
#include <cstring>
#include "renderThread.h"
#include "../../tools/threadPriority.h"
// #include "../../../program/tools/logger.h"

namespace DRIVER {

    RenderThread::RenderThread() {

        kill = false;
        ready = false;
        lockedBuffer = nullptr;

        reset();
    }

    auto RenderThread::lock(float*& data, unsigned& pitch, unsigned _width, unsigned _height, uint8_t options) -> bool {

        if (!prepareBuffer(_width, _height, true))
            return false;

        lockedBuffer->options = options;
        if ((options & 1) && fillPos) {
            _width = lockedBuffer->pitch;
            RenderBuffer* srcBuffer = getReuseBuffer(_width, _height);
            if (srcBuffer && srcBuffer->dataFloat)
                std::memcpy(lockedBuffer->dataFloat, srcBuffer->dataFloat, _width * _height * 4 * 4);
        }

        pitch = lockedBuffer->pitch;
        data = lockedBuffer->dataFloat;

        return true;
    }

    auto RenderThread::lock(unsigned*& data, unsigned& pitch, unsigned _width, unsigned _height, uint8_t options) -> bool {

        if (!prepareBuffer(_width, _height, false))
            return false;

        lockedBuffer->options = options;
        if ((options & 1) && fillPos) {
            _width = lockedBuffer->pitch;
            RenderBuffer* srcBuffer = getReuseBuffer(_width, _height);
            if (srcBuffer && srcBuffer->data)
                std::memcpy(lockedBuffer->data, srcBuffer->data, _width * _height * 4);
        }

        pitch = lockedBuffer->pitch;
        data = lockedBuffer->data;

        return true;
    }

    auto RenderThread::getReuseBuffer(unsigned& _width, unsigned& _height) -> RenderBuffer* {
        RenderBuffer* srcBuffer = &renderBuffers[(fillPos == RENDER_BUFFER_COUNT) ? 0 : fillPos];

        if (_width > srcBuffer->pitch)
            _width = srcBuffer->pitch;
        if (_height > srcBuffer->height)
            _height = srcBuffer->height;

        return srcBuffer;
    }

    auto RenderThread::prepareBuffer(unsigned _width, unsigned _height, bool floatFormat) -> bool {

        lockedBuffer = getBufferToDraw();

        if (!lockedBuffer)
            // too fast
            return false;

        lockedBuffer->sharedMutex.lock();

        if (lockedBuffer->floatFormat != floatFormat || lockedBuffer->width != _width || lockedBuffer->height != _height) {
            deleteBuffer(lockedBuffer);
            lockedBuffer->width = _width;
            lockedBuffer->height = _height;
            lockedBuffer->pitch = calcPitch( _width );
            lockedBuffer->floatFormat = floatFormat;
            resize( lockedBuffer, _width, _height );
        }

        return true;
    }

    auto RenderThread::unlock() -> void {
        if (!lockedBuffer)
            return;

        lockedBuffer->sharedMutex.unlock();
        lockedBuffer = nullptr;

        accessMutex.lock();
        frames++;
        accessMutex.unlock();

        ready.store(1);
        cv.notify_one();
    }

    auto RenderThread::reset() -> void {
        for(auto& buffer : renderBuffers) {
            deleteBuffer( &buffer );

            buffer.width = 0;
            buffer.height = 0;
            buffer.pitch = 0;
            buffer.options = 0;
        }

        fillPos = 0;
        fetchPos = 0;
        frames = 0;
        lockedBuffer = nullptr;
    }

    auto RenderThread::deleteBuffer(RenderBuffer* buffer) -> void {
        if(buffer->data) {
            delete[] buffer->data;
            buffer->data = nullptr;
        }

        if(buffer->dataFloat) {
            delete[] buffer->dataFloat;
            buffer->dataFloat = nullptr;
        }
    }

    auto RenderThread::getBufferToDraw() -> RenderBuffer* {
        accessMutex.lock();

        if (frames == RENDER_BUFFER_COUNT) {
            accessMutex.unlock();
            return nullptr;
        }

        accessMutex.unlock();

        if (fillPos == RENDER_BUFFER_COUNT)
            fillPos = 0;

        return &renderBuffers[fillPos++];
    }

    auto RenderThread::getBufferToRender() -> RenderBuffer* {
        accessMutex.lock();

        if (!frames) {
            accessMutex.unlock();
            return nullptr;
        }

        accessMutex.unlock();

        if (fetchPos == RENDER_BUFFER_COUNT)
            fetchPos = 0;

        return &renderBuffers[fetchPos++];
    }
    
    auto RenderThread::getLastBufferToRender() -> RenderBuffer* {
        
        if (fetchPos)
            return &renderBuffers[fetchPos-1];
        
        return nullptr;
    }

    auto RenderThread::wait() -> void {

        while(ready) {
            std::this_thread::yield();
        }
    }

    auto RenderThread::enable(bool state) -> void {

        if (state) {
            while (kill) {
                std::this_thread::yield();
            }
            initWorker();
        } else {
            kill = true;
            cv.notify_one();
        }
    }
    
    auto RenderThread::changePriorityToRealtime(bool state) -> void {
        // only macOS need higher thread priorities to prevent scrolling hiccups.
        // there are some situations, when a realtime priority blocks the system forever, mostly "un-fullscreen" with higher rendering speed.
        // in these situations we temporarly decrese priority to normal
#ifdef __APPLE__
        realtime = state;
        updatePriority = true;
#endif
    }

    auto RenderThread::initWorker() -> void {

        std::thread worker([this] {

            std::chrono::milliseconds duration(5);
            std::mutex cvM;
            std::unique_lock<std::mutex> lk(cvM);
#ifdef __APPLE__
            if (ThreadPriority::setPriority( ThreadPriority::Mode::Realtime, 3.0, 5.0 )) {
            //     logger->log("increased render thread prio");
            }
            updatePriority = false;
            realtime = true;
#endif
            kill = false;
            ready = false;

            while (1) {

                while (!ready.load()) {

#ifdef __APPLE__
                    if (updatePriority) {
                        updatePriority = false;
                        
                        if (ThreadPriority::setPriority( realtime ? ThreadPriority::Mode::Realtime : ThreadPriority::Mode::Normal, 3.0, 5.0 )) {
                            // logger->log(realtime ? "render thread realtime prio" : "render thread normal prio");
                        }
                    }
#endif

                    if (cv.wait_for(lk, duration, [this]() {
                        return ready.load() || kill.load();
                    })) {
                        if (kill) {
#if defined(_WIN32) || defined(__APPLE__)
#else
                            // linux hack to prevent slowdown when toggling render thread
                            std::this_thread::sleep_for(std::chrono::milliseconds(20));
#endif
                            kill = false;
                            return;
                        }
                        break;
                    }
                }

                refresh();

                accessMutex.lock();
                ready.store( frames > 0 );
                accessMutex.unlock();
            }
        });

        worker.detach();
    }

    RenderThread::~RenderThread() {
        //enable(false);
    }
}
