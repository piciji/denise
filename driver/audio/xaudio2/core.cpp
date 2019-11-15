
#include <atomic>
#include <cstring>

#define XA_CHUNKS 16
#define XA_CHUNKS_MASK (XA_CHUNKS - 1)

#ifndef COINIT_MULTITHREADED
#define COINIT_MULTITHREADED 0x00
#endif

namespace DRIVER {

struct XA_IDENT_CORE : XA_IDENT, public IXAudio2VoiceCallback {

    XA_IDENT_CORE(unsigned version = 0) : XA_IDENT() {
        pSourceVoice = nullptr;
        pMasterVoice = nullptr;
        threadSync = 0;
        pXAudio2 = nullptr;        
        
        unprocessedChunks = 0;
        circularBuffer = nullptr;
        chunkSize = chunkSizeAll = 0;
        chunkPosition = 0;
        writeChunk = 0;
        cleared = false;
        
        settings.synchronize = false;
        settings.frequency = 48000;
        settings.handle = nullptr;
        settings.latency = 64;
        settings.minimumLatency = 12;
    }

    virtual ~XA_IDENT_CORE() { term(); }

    STDMETHOD_(void, OnBufferStart) (void*) {}
    STDMETHOD_(void, OnBufferEnd) (void*) {
        // xaudio uses a callback when a transfered buffer was played completely.
        if (unprocessedChunks)
            unprocessedChunks--; // thread safe by c++11 atomix
        SetEvent(threadSync);
    }

    STDMETHOD_(void, OnLoopEnd) (void*) {}
    STDMETHOD_(void, OnStreamEnd) () {}
    STDMETHOD_(void, OnVoiceError) (void*, HRESULT) {}
    STDMETHOD_(void, OnVoiceProcessingPassEnd) () {}
    STDMETHOD_(void, OnVoiceProcessingPassStart) (UINT32) {}
    
    IXAudio2* pXAudio2;
    IXAudio2MasteringVoice* pMasterVoice;
    IXAudio2SourceVoice* pSourceVoice;
    HANDLE threadSync;

    std::atomic<uint8_t> unprocessedChunks;
    uint8_t* circularBuffer;
    unsigned writeChunk;
    unsigned chunkPosition;    
    unsigned chunkSize;
    unsigned chunkSizeAll;        
    
    bool cleared;
    
    struct {
        bool synchronize;
        unsigned frequency;
        unsigned latency;
        unsigned minimumLatency;
        HWND handle;
    } settings;
    
    auto init(uintptr_t handle) -> bool {
        settings.handle = (HWND) handle;
        return init();
    }

    auto synchronize(bool state) -> void {
        settings.synchronize = state;
    }
	
    auto setFrequency(unsigned value) -> void {
        settings.frequency = value;
        
        if (settings.handle)
            init();
    }
	
    auto setLatency(unsigned value) -> void {		
        settings.latency = std::max(settings.minimumLatency, value);
        
        if (settings.handle)
            init();        
    }
    
    auto getMinimumLatency() -> unsigned {
        
        return settings.minimumLatency;
    }
    
    inline auto writeAvailable() -> unsigned {
        return chunkSize * (XA_CHUNKS_MASK - unprocessedChunks);
        //return (chunkSize * (XA_CHUNKS - unprocessedChunks)) - chunkPosition;
    }

    auto term() -> void {
        if (pSourceVoice) {
            pSourceVoice->Stop(0, XAUDIO2_COMMIT_NOW);
            pSourceVoice->DestroyVoice();
            pSourceVoice = nullptr;
        }

        if (pMasterVoice) {
            pMasterVoice->DestroyVoice();       
            pMasterVoice = nullptr;
        }

        if (pXAudio2) {
            pXAudio2->Release();  
            pXAudio2 = nullptr;
        }

        if (threadSync) {
            CloseHandle(threadSync);
            threadSync = 0;
        }

        if (circularBuffer) {
            delete[] circularBuffer;
            circularBuffer = nullptr;
        }
        
        chunkPosition = writeChunk = unprocessedChunks = 0;
    }
    
    auto clear() -> void {        
        if (cleared) return;
        
        pSourceVoice->Stop(0, XAUDIO2_COMMIT_NOW);
        pSourceVoice->FlushSourceBuffers(); // trigger "bufferEnd" callback

        ResetEvent(threadSync);
        chunkPosition = writeChunk = unprocessedChunks = 0;
        std::memset( circularBuffer, 0, chunkSizeAll );

        pSourceVoice->Start(0, XAUDIO2_COMMIT_NOW);
        
        cleared = true;
    }
    
    auto init() -> bool {
        WAVEFORMATEX wfx = {0};
        cleared = false;
        term();
        
        // we use a simple proportion to calculate the buffer size for a given latency and frequency
        // latency is the time between buffering and playing the sample.
        // hence we need sufficient buffer size.
        
        // bytes per second for requested sample rate = 1000 ms
        // buffer size                                = latency in ms
        // buffer size = (latency * bytes per second) / 1000  
        // furthermore we split the overall buffer in multiple chunks.
        // later we write complete chunks to audio driver
        chunkSize = settings.frequency * settings.latency / XA_CHUNKS / 1000.0 + 0.5;
        // translate in bytes: for two channels and 4 bytes a sample
        chunkSize = chunkSize * 2 * sizeof (float);
        // in a second step we calculate the complete size, this way
        // the overall size is dividable by chunk size
        chunkSizeAll = chunkSize * XA_CHUNKS;
    
        CoInitializeEx(0, COINIT_MULTITHREADED);

        if ( FAILED(XAudio2Create( &pXAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR)) )
            goto error;

    #if (_WIN32_WINNT >= 0x0602 /* Win 8 */)
        if (FAILED(pXAudio2->CreateMasteringVoice(&pMasterVoice, 2, settings.frequency, 0, (LPCWSTR)0, NULL, AudioCategory_GameEffects)))
            goto error;
    #else
        if (FAILED(pXAudio2->CreateMasteringVoice(&pMasterVoice, 2, settings.frequency, 0, 0, NULL)))
            goto error;
    #endif
                
        wfx.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
        wfx.nChannels = 2;
        wfx.wBitsPerSample = sizeof (float) * 8; // 32 bit -> 4 byte        
        wfx.nBlockAlign = wfx.wBitsPerSample / 8 * wfx.nChannels; // 2 * 4 byte for a left/right audio frame        
        
        wfx.nSamplesPerSec = settings.frequency;
        wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
        wfx.cbSize = 0;

        if (FAILED(pXAudio2->CreateSourceVoice(&pSourceVoice, &wfx,
                XAUDIO2_VOICE_NOSRC, XAUDIO2_DEFAULT_FREQ_RATIO,
                (IXAudio2VoiceCallback*)this, 0, 0)))
            goto error;        
        
        // create as "auto reset" event
        threadSync = CreateEvent(NULL, false, false, NULL);
        
        if (!threadSync)
            goto error;

        circularBuffer = new uint8_t[chunkSizeAll];
        
        if (!circularBuffer)
            goto error; 
        
        std::memset( circularBuffer, 0, chunkSizeAll );

        if (FAILED(pSourceVoice->Start(0, XAUDIO2_COMMIT_NOW)))
            goto error;

        return true;
        
        error:
        
        term();
        
        return false;
    }   

    auto addSamples( const uint8_t* buffer, unsigned size) -> void {        
        
        if (!settings.synchronize) {
            unsigned avail = writeAvailable();

            if (avail == 0) // discard all
                return;
           
            if (avail < size) // discard the rest, we can not wait :-)
                size = avail;
        }

        while (size) {
            // fill up current chunk
            unsigned stepSize = std::min( size, chunkSize - chunkPosition );

            std::memcpy( circularBuffer + writeChunk * chunkSize + chunkPosition, buffer, stepSize );

            chunkPosition += stepSize;
            buffer += stepSize;
            size -= stepSize;

            if (chunkPosition == chunkSize) { // transfer chunk to audio buffer
                // reset byte position within chunk
                chunkPosition = 0;
                cleared = false;
                
                XAUDIO2_BUFFER xa2buffer;

                while (unprocessedChunks == XA_CHUNKS_MASK)
                    // we need to wait until audio device has played samples worth of chunk size
                    WaitForSingleObject(threadSync, INFINITE);

                xa2buffer.Flags = 0;
                xa2buffer.AudioBytes = chunkSize;
                xa2buffer.pAudioData = circularBuffer + writeChunk * chunkSize;
                xa2buffer.PlayBegin = 0;
                xa2buffer.PlayLength = 0;
                xa2buffer.LoopBegin = 0;
                xa2buffer.LoopLength = 0;
                xa2buffer.LoopCount = 0;
                xa2buffer.pContext = NULL;

                if ((pSourceVoice->SubmitSourceBuffer( &xa2buffer, NULL )) != S_OK)
                    return;

                unprocessedChunks++; 
                // points to next chunk, wraps around ring buffer if needed
                writeChunk = (writeChunk + 1) & XA_CHUNKS_MASK;
            }
        }
    }

    auto expectFloatingPoint() -> bool { 
        return true;
    }

    auto getCenterBufferDeviation() -> double {    

        int halfSize = (int) (chunkSizeAll / 2);

        int avail = writeAvailable();

        int deltaMid = avail - halfSize;
 
        return (double) deltaMid / halfSize;
    }

};

}
