
#ifdef __APPLE__
    #include <OpenAL/al.h>
    #include <OpenAL/alc.h>
#else
    #include <AL/al.h>
    #include <AL/alc.h>
#endif

#include <cstring>
#include <thread>

#define AL_CHUNKS 16
#define AL_CHUNKS_MASK (AL_CHUNKS - 1)

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

namespace DRIVER {

struct OpenAL : public Audio {
    ALuint source;
    ALCcontext* context;
    ALCdevice* device;
    bool cleared;

    struct {
        bool priority;
        bool synchronize;
        unsigned frequency;
        unsigned latency;
        unsigned minimumLatency;
        uintptr_t handle;
    } settings;
    
	unsigned chunkSize;
    unsigned chunkSizeAll;
	uint8_t* chunkBuffer;
	ALuint* alBuffers;
	unsigned writeBuffer;
	unsigned chunkPosition;

	auto synchronize(bool state) -> void {
		settings.synchronize = state;
	}
	
	auto hasSynchronized() -> bool { return settings.synchronize; }

	auto setFrequency(unsigned value) -> void {
		settings.frequency = value;
		if (settings.handle) {
			init();
		}
	}

    auto getFrequency() -> unsigned {
        return settings.frequency;
    }

	auto setLatency(unsigned value) -> void {
		settings.latency = std::max<unsigned>(value, settings.minimumLatency);
		if (settings.handle) {
			init();
		}
	}
    
    auto setHighPriority(bool state) -> void {
        
        settings.priority = state;
    }
    
    auto getMinimumLatency() -> unsigned {
        
        return settings.minimumLatency;
    }

	auto init(uintptr_t handle) -> bool {
		settings.handle = handle;
		return init();
	}

	auto stop() -> void {
		ALint playing;
		alGetSourcei(source, AL_SOURCE_STATE, &playing);
		if (playing == AL_PLAYING) alSourceStop(source);
	}

	auto play() -> void {
		ALint playing;
		alGetSourcei(source, AL_SOURCE_STATE, &playing);
		if (playing != AL_PLAYING) alSourcePlay(source);
	}	

    auto init() -> bool {
		cleared = false;
        term();
            
        if (!(device = alcOpenDevice(NULL))) {
            term();
            return false;
        }

        if (!(context = alcCreateContext(device, NULL))) {
            term();
            return false;
        }

        alcMakeContextCurrent(context);

        alGenSources(1, &source);

        alListener3f(AL_POSITION, 0.0, 0.0, 0.0);
        alListener3f(AL_VELOCITY, 0.0, 0.0, 0.0);
        ALfloat listener_orientation[] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
        alListenerfv(AL_ORIENTATION, listener_orientation);

		if (alBuffers)
			alDeleteBuffers(AL_CHUNKS, alBuffers);
			
		alBuffers = new ALuint[AL_CHUNKS];
		alGenBuffers(AL_CHUNKS, alBuffers);				
        
		chunkSize = settings.frequency * settings.latency / AL_CHUNKS / 1000.0 + 0.5;
		chunkSize = chunkSize * 2 * sizeof( int16_t );
        chunkSizeAll = chunkSize * AL_CHUNKS;
				
		chunkBuffer = new uint8_t[chunkSize];
		
		writeBuffer = AL_CHUNKS;

		clear();	

		if (alIsSource(source) == AL_TRUE)
			alSourcef(source, AL_GAIN, 1.0);

		return true;
	}
	
    auto term() -> void {
		
		if (alIsSource(source) == AL_TRUE) {
            
            unqueueAllBuffers();
            
            if (alBuffers)
                alDeleteBuffers(AL_CHUNKS, alBuffers);
    
			alDeleteSources(1, &source);
            source = 0;
		}
        
		if (context) {
			alcMakeContextCurrent(0);
			alcDestroyContext(context);
			context = 0;
		}

		if (device) {
			alcCloseDevice(device);
			device = 0;
		}
		
        freeBuffer();
	}
	
    auto clear() -> void {

		if (cleared) return;
		
		if (chunkBuffer)
			memset(chunkBuffer, 0, chunkSize);		

        unqueueAllBuffers();
        
		writeBuffer = AL_CHUNKS;
        chunkPosition = 0;
		cleared = true;
	}
		
    auto unqueueAllBuffers() -> void {
        
        if (alIsSource(source) == AL_TRUE) {
            stop();
            
            int queued = 0;
            alGetSourcei(source, AL_BUFFERS_QUEUED, &queued);
            
            alSourceUnqueueBuffers(source, queued, &alBuffers[writeBuffer > AL_CHUNKS_MASK ? AL_CHUNKS_MASK : writeBuffer ]);
        }
    }
    
	auto unqueueProcessedBuffers() -> bool {
        ALint val;

        alGetSourcei(source, AL_BUFFERS_PROCESSED, &val);

        if (val <= 0)
           return false;

        alSourceUnqueueBuffers(source, val,
            &alBuffers[writeBuffer > AL_CHUNKS_MASK ? AL_CHUNKS_MASK : writeBuffer]);
        
        writeBuffer += val;
        return true;
	}
	
	auto addSamples( const uint8_t* buffer, unsigned size ) -> void {		

		while(size) {
			
			unsigned stepSize = std::min<unsigned>(chunkSize - chunkPosition, size);
			std::memcpy(chunkBuffer + chunkPosition, buffer, stepSize);
			
			buffer += stepSize;
			size -= stepSize;
			chunkPosition += stepSize;
			
			if ( chunkPosition != chunkSize )
				break;
            
            cleared = false;
							
			if (!writeBuffer) {
				// no empty buffer -> overrun
				while(1) {
					
					if (unqueueProcessedBuffers())
						// at least one buffer has played and is reusable
						break;
						
					if (!settings.synchronize)
						return;
						
                    if (!settings.priority)
                        std::this_thread::sleep_for( std::chrono::milliseconds(1) );
				}
			}
			
			ALuint alBuffer = alBuffers[--writeBuffer];
			
			alBufferData(alBuffer, AL_FORMAT_STEREO16, chunkBuffer, chunkSize, settings.frequency);
			
			chunkPosition = 0;
			
			alSourceQueueBuffers(source, 1, &alBuffer);
			
			if (alGetError() != AL_NO_ERROR)
				return;

			play();
		}
	}

	auto freeBuffer() -> void {
		if (chunkBuffer) {
			delete[] chunkBuffer;
			chunkBuffer = 0;
		}
	}
    
    auto expectFloatingPoint() -> bool { 
        return false;
    }

    auto getCenterBufferDeviation() -> double {    

        unqueueProcessedBuffers();
        
        int halfSize = (int) (chunkSizeAll / 2);

        int avail = std::max<int>(0,  (int)(writeBuffer * chunkSize) - (int)chunkPosition);                

        int deltaMid = avail - halfSize;
 
        return (double) deltaMid / halfSize;
    }

    OpenAL() {
		cleared = false;

		context = 0;
		device = 0;

		chunkBuffer = 0;
		chunkSize = 0;
		alBuffers = 0;

		settings.synchronize = false;
		settings.frequency = 48000;
		settings.latency = 64u;
        settings.minimumLatency = 40u;
        settings.priority = false;
		settings.handle = 0;		
	}
	
    ~OpenAL() { term(); }
};

}

#pragma clang diagnostic pop
