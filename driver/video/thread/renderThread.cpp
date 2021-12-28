
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

    auto RenderThread::lock(float*& data, unsigned& pitch, unsigned _width, unsigned _height) -> bool {

        if (!prepareBuffer(_width, _height))
            return false;

        pitch = _width;
        data = lockedBuffer->dataFloat;

        return true;
    }

    auto RenderThread::lock(int32_t*& data, unsigned& pitch, unsigned _width, unsigned _height) -> bool {

        if (!prepareBuffer(_width, _height))
            return false;

        pitch = _width;
        data = lockedBuffer->dataInt;

        return true;
    }

    auto RenderThread::lock(unsigned*& data, unsigned& pitch, unsigned _width, unsigned _height) -> bool {

        if (!prepareBuffer(_width, _height))
            return false;

        pitch = lockedBuffer->pitch;
        data = lockedBuffer->data;

        return true;
    }

    auto RenderThread::prepareBuffer(unsigned _width, unsigned _height) -> bool {

        lockedBuffer = getBufferToDraw();

        if (!lockedBuffer)
            // too fast
            return false;

        lockedBuffer->sharedMutex.lock();

        if (lockedBuffer->width != _width || lockedBuffer->height != _height) {
            deleteBuffer(lockedBuffer);
            lockedBuffer->width = _width;
            lockedBuffer->height = _height;
            lockedBuffer->pitch = calcPitch( _width );
            lockedBuffer->updated = true;
            resize( lockedBuffer, _width, _height );
        }

        return true;
    }

    auto RenderThread::unlock(bool disallowShader) -> void {
        if (!lockedBuffer)
            return;
        lockedBuffer->disallowShader = disallowShader;
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
            buffer.updated = false;
            buffer.disallowShader = false;
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

        if(buffer->dataInt) {
            delete[] buffer->dataInt;
            buffer->dataInt = nullptr;
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
        // needed for threaded CRT via CPU rendering
        if (lockedBuffer)
            lockedBuffer->sharedMutex.unlock();

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

    auto RenderThread::initWorker() -> void {

        std::thread worker([this] {

            std::chrono::milliseconds duration(5);
            std::mutex cvM;
            std::unique_lock<std::mutex> lk(cvM);

            //if (ThreadPriority::setPriority( ThreadPriority::Mode::Realtime, 3.0, 5.0 )) {
            //     logger->log("increased render thread prio");
            //}

            kill = false;
            ready = false;

            while (1) {

                while (!ready.load()) {
                    if (kill) {
                        kill = false;
                        return;
                    }

                    if (cv.wait_for(lk, duration, [this]() {
                        return ready.load();
                    }))
                        break;
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
