#include <atomic>
#include <initguid.h>
#include <avrt.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <audiopolicy.h>
#include <devicetopology.h>
#include <endpointvolume.h>
#include <functiondiscoverykeys_devpkey.h>

#define WS_CHUNKS 16
#define WS_CHUNKS_MASK (WS_CHUNKS - 1)

namespace DRIVER {

struct Wasapi : public Audio {
	
    Wasapi(bool exclusive) {        
        settings.exclusive = exclusive;
    }
    
	struct {
        bool priority;
        bool synchronize;
        unsigned latency;
        unsigned minimumLatency;
        bool exclusive;
    } settings;	
	
	IMMDeviceEnumerator* enumerator = nullptr;
	IMMDevice* audioDevice = nullptr;
	IAudioClient* audioClient = nullptr;
	IAudioRenderClient* renderClient = nullptr;	
    HANDLE eventHandle = nullptr;
    bool cleared;
		
	unsigned frameCount = 0; // frame: includes all channel samples
	unsigned frameSize = 0; // i.e float is 4 byte per sample -> 4 * 2 channels
	unsigned bufferSize = 0;  // frame count * frame size

    // for exclusive mode
    uint8_t* ringBuffer = nullptr;
    unsigned chunkPosition = 0;    
    unsigned readChunk;
    unsigned writeChunk;
    std::atomic<uint8_t> unprocessedChunks;
    std::atomic<bool> ready;
	CRITICAL_SECTION criticalSection;
    HANDLE thread = nullptr;   
    	
	auto available() -> unsigned { // in bytes
		unsigned padding = 0; // count of frames, not samples or bytes
		audioClient->GetCurrentPadding(&padding);
		return bufferSize - (padding * frameSize);
	}
	
	auto availableFrames() -> unsigned { 
		unsigned padding = 0; // count of frames, not samples or bytes
		audioClient->GetCurrentPadding(&padding);
		return frameCount - padding;
	}

	auto clear() -> void {   
		if (cleared) return;
        
        if (settings.exclusive)
            EnterCriticalSection( &criticalSection );
        
		audioClient->Stop();
		audioClient->Reset();
        if (ringBuffer)
            std::memset( ringBuffer, 0, bufferSize * WS_CHUNKS );
		audioClient->Start();
        readChunk = 0;
		writeChunk = WS_CHUNKS_MASK;
		unprocessedChunks = WS_CHUNKS_MASK;  // maximum distance between read and write
        chunkPosition = 0;   
        if (settings.exclusive)
            LeaveCriticalSection( &criticalSection );
		cleared = true;
	}
	
	auto synchronize(bool state) -> void {
		settings.synchronize = state;
	}
	
	auto hasSynchronized() -> bool { return settings.synchronize; }
	
	auto setLatency(unsigned value) -> void {
		settings.latency = value;
		if(audioDevice) 
            init();        
	}
    
    auto setHighPriority(bool state) -> void {
        
        settings.priority = state;
    }
    
    auto getMinimumLatency() -> unsigned {
        
        return settings.minimumLatency;
    }
    
    static auto CALLBACK worker(PVOID data) -> DWORD {
        
        Wasapi* wasapi = (Wasapi*)data;
        
        wasapi->worker();
        
        return 0;
    }       
        
    auto worker() -> void {
                
        SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL );
        
        while (ready) {  
                    
            if (WaitForSingleObject(eventHandle, INFINITE ) == WAIT_OBJECT_0) {

                uint8_t* target = nullptr; 

                if (renderClient->GetBuffer(frameCount, &target) == S_OK) {

                    EnterCriticalSection( &criticalSection );

                    std::memcpy(target, ringBuffer + readChunk * bufferSize, bufferSize);

                    LeaveCriticalSection( &criticalSection );

                    renderClient->ReleaseBuffer(frameCount, 0);
                }

                readChunk = ( readChunk + 1 ) & WS_CHUNKS_MASK;
                
                if (unprocessedChunks == 0) {
                    unprocessedChunks = WS_CHUNKS_MASK;
                    EnterCriticalSection( &criticalSection );
                    writeChunk = (WS_CHUNKS_MASK + readChunk) & WS_CHUNKS_MASK;
                    LeaveCriticalSection( &criticalSection );
                } else
                    unprocessedChunks--;                
            }
        }
        
        ExitThread(0);
    }

    auto addSamples( const uint8_t* buffer, unsigned size ) -> void {
        cleared = false;
        
        if (!settings.exclusive)
            return addSamplesShared( buffer, size );
        
        while(size) {            

            if (unprocessedChunks >= WS_CHUNKS_MASK) {
                if (settings.synchronize) {
                    if (!settings.priority)
                        std::this_thread::sleep_for( std::chrono::milliseconds(1) );
                    continue;                    
                }
            }
            
            unsigned stepSize = std::min(bufferSize - chunkPosition, size);
            
            EnterCriticalSection( &criticalSection );            
            
            std::memcpy( ringBuffer + (writeChunk * bufferSize) + chunkPosition, buffer, stepSize );

            LeaveCriticalSection( &criticalSection );
            
            buffer += stepSize;
            size -= stepSize;
            chunkPosition += stepSize;

            if (chunkPosition != bufferSize)
                return;

            chunkPosition = 0;     
            
            writeChunk = ( writeChunk + 1 ) & WS_CHUNKS_MASK;

            unprocessedChunks++;
        }
    }
    
	auto addSamplesShared( const uint8_t* buffer, unsigned size ) -> void {
		
		unsigned framesAvail = 0;
		unsigned bytesAvail;
		uint8_t* audioBuffa;
		
		while(size) {
				
			if (settings.synchronize) {
				while( 0 == (framesAvail = availableFrames())) {
                    if (!settings.priority)
                        std::this_thread::sleep_for( std::chrono::milliseconds(1) );
				}
			} else 
				if ( 0 == (framesAvail = availableFrames()) )
					return;
						
			bytesAvail = framesAvail * frameSize;
			bytesAvail = std::min( bytesAvail, size );
			// in case of buffer size is smaller than available size.
            // incomming size is always frame aligned.
			framesAvail = bytesAvail / frameSize;
			
			size -= bytesAvail;
			
			if(renderClient->GetBuffer(framesAvail, &audioBuffa) != S_OK)
				return;
				
			std::memcpy( audioBuffa, buffer, bytesAvail );
			
			renderClient->ReleaseBuffer(framesAvail, 0);
			
			buffer += bytesAvail;
		}
	}
	
	auto init() -> bool {
   
        term();
        WAVEFORMATEXTENSIBLE wf;
        REFERENCE_TIME devicePeriod;
		cleared = false;
				
		if(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&enumerator) != S_OK)
            return false;
		if(enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &audioDevice) != S_OK)
            return false;
        
        if(audioDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&audioClient) != S_OK)
            return false;	
        
        if (settings.exclusive) {
            IPropertyStore* propertyStore = nullptr;
            if (audioDevice->OpenPropertyStore(STGM_READ, &propertyStore) != S_OK)
                return false;
            PROPVARIANT propVariant;
            if (propertyStore->GetValue(PKEY_AudioEngine_DeviceFormat, &propVariant) != S_OK)
                return false;
            wf = *(WAVEFORMATEXTENSIBLE*) propVariant.blob.pBlobData;
            wf.Format.nChannels = 2;            
            propertyStore->Release();
            
            if (audioClient->GetDevicePeriod(nullptr, &devicePeriod) != S_OK)
                return false;
            
            settings.minimumLatency = devicePeriod / 10000;
            
            // expect latency in 100ns units: 1 ms = 1'000 micro = 1'000'000 ns = 10'000 units
            // respect driver reported minimum latency
            auto latency = std::max(devicePeriod, (REFERENCE_TIME) settings.latency * 10000);
            
            auto result = audioClient->Initialize(AUDCLNT_SHAREMODE_EXCLUSIVE, AUDCLNT_STREAMFLAGS_EVENTCALLBACK, latency, latency, &wf.Format, nullptr);            
            
            if (result == AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED) {
                // generated buffer for a given latency needs to be aligned
                // i.e: 2 channels, 4 bytes a sample = 8 bytes for an audio frame -> possible buffer sizes: 8, 16, 24 and so on
                if (audioClient->GetBufferSize(&frameCount) != S_OK)
                    return false;
                audioClient->Release();
                            
                // simple proportion to get buffer size for a given frequency and latency
                // when: sample rate = 1000ms
                // then: frame count = latency     
                // latency = (frame count * 1000) / sample rate
                // latency 100 ns units = latency * 10000
                latency = (REFERENCE_TIME) (10000.0 * 1000.0 * (double)frameCount / (double)wf.Format.nSamplesPerSec + 0.5);               
                                
                // reinit with corrected latency
                if (audioDevice->Activate(IID_IAudioClient, CLSCTX_ALL, nullptr, (void**) &audioClient) != S_OK)            
                    return false;                                   
                
                result = audioClient->Initialize(AUDCLNT_SHAREMODE_EXCLUSIVE, AUDCLNT_STREAMFLAGS_EVENTCALLBACK, latency, latency, &wf.Format, nullptr);
            }                        
            
            if (result != S_OK)
                return false;
            
            // create as auto reset event
            eventHandle = CreateEvent(nullptr, false, false, nullptr);
            
            if(audioClient->SetEventHandle(eventHandle) != S_OK)
                return false;            
            
        } else {
                
            WAVEFORMATEX* waveFormatEx = nullptr;
            if(audioClient->GetMixFormat(&waveFormatEx) != S_OK)
                return false;
            
            wf = *(WAVEFORMATEXTENSIBLE*)waveFormatEx;      
            wf.Format.nChannels = 2;
            CoTaskMemFree(waveFormatEx);

            if(audioClient->GetDevicePeriod(&devicePeriod, nullptr))
                return false;

            auto latency = std::max(devicePeriod, (REFERENCE_TIME) settings.latency * 10000);

            settings.minimumLatency = devicePeriod / 10000;

            if(audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, latency, 0, &wf.Format, nullptr) != S_OK)
                return false;
        }

		if(audioClient->GetService(__uuidof(IAudioRenderClient), (void**)&renderClient) != S_OK)
            return false;
		if(audioClient->GetBufferSize(&frameCount) != S_OK)
            return false;
        
		frameSize = wf.Format.nBlockAlign;        
		bufferSize = frameCount * frameSize;	                          		
        
        if (settings.exclusive) {
            // is handled in another thread, because of Dynamic Rate Control and vsync.
            // in exclusive mode we wait for an event to signal the current buffer is processed
            // and copy the complete buffer for a given latency in the engine.
            // there is no chunked copy (DRC) and we have to always wait for the event to fire.
            // this way we could miss a vblank.
            ringBuffer = new uint8_t[bufferSize * WS_CHUNKS];
            InitializeCriticalSection(&criticalSection);            
        }
        
        clear();
        
        if (settings.exclusive) {
            ready = true;
            thread = CreateThread(NULL, 0, Wasapi::worker, this, 0, NULL);
        }
        
		return true;
	}

	auto init(uintptr_t handle) -> bool {
		return init();
	}
		
	auto term() -> void {
        ready = false;
        if (thread) {
            WaitForSingleObject( thread, INFINITE );
            CloseHandle( thread );
            DeleteCriticalSection( &criticalSection );
            thread = nullptr;
        }   
                
		if(enumerator) enumerator->Release(), enumerator = nullptr;
		if(audioClient) audioClient->Stop();
		if(renderClient) renderClient->Release(), renderClient = nullptr;
		if(audioClient) audioClient->Release(), audioClient = nullptr;
		if(audioDevice) audioDevice->Release(), audioDevice = nullptr;        
        if(eventHandle) CloseHandle(eventHandle), eventHandle = nullptr;        
        if(ringBuffer) delete[] ringBuffer, ringBuffer = nullptr;    
	}
    
    auto expectFloatingPoint() -> bool { 
        return frameSize == 8;
    }

    auto getCenterBufferDeviation() -> double {    
        int halfSize;
        int avail;
        
        if (settings.exclusive) {
            halfSize = (int) ((bufferSize * WS_CHUNKS) / 2);    
            avail = ((WS_CHUNKS - unprocessedChunks) * bufferSize) - chunkPosition;
        } else {
            halfSize = (int) (bufferSize / 2);  
            avail = available();
        }        
        
        int deltaMid = avail - halfSize;
 
        return (double) deltaMid / halfSize;
    }
	
	Wasapi() {
        settings.synchronize = false;
        settings.priority = false;
    }
	
	~Wasapi() { term(); }
};

}
