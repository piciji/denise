
#include "../tools/win.h"
#include "../tools/tools.h"

#include <dsound.h>
#include <cstring>
#include <thread>

#define DS_CHUNKS 16
#define DS_CHUNKS_MASK (DS_CHUNKS - 1)

namespace DRIVER {

struct DAudio : Audio {
    LPDIRECTSOUND ds;
    DSBUFFERDESC dsbd;
    WAVEFORMATEX wfx;
    LPDIRECTSOUNDBUFFER dsb;

    uint8_t* chunkBuffer;
    unsigned chunkSize;
    unsigned chunkSizeAll;
    unsigned chunkPosition;
    uint8_t writeChunk;
    uint8_t readChunk;
    uint8_t distance;
    bool cleared;
    
    struct {
        bool priority;
        bool synchronize;
        unsigned frequency;
        unsigned latency;
        unsigned minimumLatency;
        HWND handle;
    } settings;

    auto stop() -> void { if (dsb) dsb->Stop(); }
	
    auto play() -> void { if (dsb) dsb->Play(0, 0, DSBPLAY_LOOPING); }
	
    auto term() -> void {
		if (chunkBuffer)
            delete[] chunkBuffer;
            
		chunkBuffer = 0;

		dxRelease(dsb);
		dxRelease(ds);
	}
    
    auto setHighPriority(bool state) -> void {
        
        settings.priority = state;
    }
	
    auto clear() -> void {
		if (cleared) return;
		readChunk = 0;
		writeChunk = DS_CHUNKS_MASK;
		distance = DS_CHUNKS_MASK;

		chunkPosition = 0;
        
		if (chunkBuffer)
            memset(chunkBuffer, 0, chunkSize);

		stop();
		dsb->SetCurrentPosition(0);

		DWORD size;
		void* output;
		dsb->Lock(0, chunkSizeAll, &output, &size, 0, 0, 0);
		memset(output, 0, size);
		dsb->Unlock(output, size, 0, 0);

		play();
		cleared = true;
	}
	
    auto addSamples( const uint8_t* buffer, unsigned size) -> void {
        
        DWORD pos, outSize;
		void* output;
        
        while(size) {
            unsigned stepSize = std::min( size, chunkSize - chunkPosition );

            std::memcpy( chunkBuffer + chunkPosition, buffer, stepSize );
            
            chunkPosition += stepSize;
            buffer += stepSize;
            size -= stepSize;
            
            if (chunkPosition == chunkSize) {
                chunkPosition = 0;
                cleared = false;
                
                if (settings.synchronize) {
                    while (distance >= DS_CHUNKS_MASK) {
                        dsb->GetCurrentPosition( &pos, 0 );
                        uint8_t activeChunk = pos / chunkSize;
                        
                        if (activeChunk == readChunk) {
                            
                            if (!settings.priority)
                                std::this_thread::sleep_for( std::chrono::milliseconds(1) );
                            
                            continue;
                        }
                        
                        distance -= (DS_CHUNKS + activeChunk - readChunk) % DS_CHUNKS;
                        readChunk = activeChunk;                                                

                        if (distance < 2) {
                            //buffer underflow; set max distance to recover quickly
                            distance = DS_CHUNKS_MASK;
                            writeChunk = (DS_CHUNKS_MASK + readChunk) % DS_CHUNKS;
                            break;
                        }
                    }
                }

                writeChunk = (writeChunk + 1) % DS_CHUNKS;
                distance = (distance + 1) % DS_CHUNKS;

                if (dsb->Lock(writeChunk * chunkSize, chunkSize, &output, &outSize, 0, 0, 0) == DS_OK) {
                    memcpy(output, chunkBuffer, chunkSize);
                    dsb->Unlock(output, outSize, 0, 0);
                }
            }
        }
    }    
    
    inline auto writeAvailable() -> unsigned {
        DWORD pos;
        dsb->GetCurrentPosition( &pos, 0 );
        uint8_t activeChunk = pos / chunkSize;
        
        uint8_t nwc = (writeChunk + 1) % DS_CHUNKS;
        
        if (activeChunk >= nwc)
            // inner ring distance
            return (activeChunk - nwc) * chunkSize;
        // outer ring distance
        return ((DS_CHUNKS_MASK - nwc) + activeChunk) * chunkSize;
    }
    
    auto getCenterBufferDeviation() -> double {

        int halfSize = (int) (chunkSizeAll / 2);

        int avail = writeAvailable();

        int deltaMid = avail - halfSize;

        return (double) deltaMid / halfSize;
    }

    auto init() -> bool {
		cleared = false;
		term();

        chunkSize = settings.frequency * settings.latency / DS_CHUNKS / 1000.0 + 0.5;       
        chunkSize = chunkSize * 2 * sizeof( int16_t );
        chunkSizeAll = chunkSize * DS_CHUNKS;
        
		chunkBuffer = new uint8_t[chunkSize];

		DWORD result = DirectSoundCreate(0, &ds, 0);
		if (result != DS_OK)
			return false;		

		ds->SetCooperativeLevel(settings.handle, DSSCL_PRIORITY);

		memset(&wfx, 0, sizeof (wfx));
		wfx.wFormatTag = WAVE_FORMAT_PCM;
		wfx.nChannels = 2;
		wfx.nSamplesPerSec = settings.frequency;
		wfx.wBitsPerSample = 16;
		wfx.nBlockAlign = wfx.wBitsPerSample / 8 * wfx.nChannels;
		wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;       
        
		memset(&dsbd, 0, sizeof (dsbd));
		dsbd.dwSize = sizeof (dsbd);
        dsbd.dwFlags = DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_GLOBALFOCUS;
		dsbd.dwBufferBytes = chunkSizeAll;

		GUID guidNULL;
		memset(&guidNULL, 0, sizeof (GUID));

		dsbd.guid3DAlgorithm = guidNULL;
		dsbd.lpwfxFormat = &wfx;
		ds->CreateSoundBuffer(&dsbd, &dsb, 0);
		dsb->SetFrequency(settings.frequency);

		clear();
		if (dsb) dsb->SetVolume(DSBVOLUME_MAX);

		return true;
	}

    auto init(uintptr_t handle) -> bool {
        settings.handle = (HWND) handle;
        return init();
    }

    auto synchronize(bool state) -> void {
        settings.synchronize = state;
    }
	
	auto hasSynchronized() -> bool { return settings.synchronize; }
	
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

    auto expectFloatingPoint() -> bool {
        return false;
    }

    DAudio() {
		chunkBuffer = 0;
		ds = 0;
		dsb = 0;
		settings.synchronize = false;
		settings.frequency = 48000;
		settings.latency = 64;
        settings.minimumLatency = 25u;
		settings.handle = nullptr;
        settings.priority = false;
        cleared = false;
	}
	
    ~DAudio() { term(); }
};

}
