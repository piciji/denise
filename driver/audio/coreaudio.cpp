
#include <CoreAudio/CoreAudio.h>
#include <AudioUnit/AudioUnit.h>
#include <thread>
#include <atomic>
#include <cstdint>
#include <string>

#define CA_BUFFERS 8

namespace DRIVER {
    
struct CoreAudio : public Audio {
    
    AudioComponentInstance outputInstance = nullptr;      
    
    std::mutex m;
    std::mutex cvM;
    std::condition_variable cv;
    
    unsigned bytesAll = 0;
    std::atomic<unsigned> fillSize;   
    uint8_t* circularBuffer = nullptr;
    unsigned readPosition;
    unsigned writePosition;
    bool cleared = false;
    
    struct {
        bool synchronize = false;
        unsigned frequency = 48000;
        unsigned latency = 64;
        unsigned minimumLatency = 2;
        uintptr_t handle;
    } settings;
    
    auto synchronize(bool state) -> void {
		settings.synchronize = state;
        if(outputInstance)
            init();
	}
	
	auto hasSynchronized() -> bool { return settings.synchronize; }
    
    auto setFrequency(unsigned value) -> void {
		settings.frequency = value;
        if(outputInstance)
            init();
    }

    auto getFrequency() -> unsigned {
        return settings.frequency;
    }
    
    auto setLatency(unsigned value) -> void {
		settings.latency = std::max(settings.minimumLatency, value);
        
        if(outputInstance)
            init();
    }
    
    auto getMinimumLatency() -> unsigned {
        
        return settings.minimumLatency;
    }
    
    auto clear() -> void {
        if (cleared) return;
        memset(circularBuffer, 0, bytesAll);
        
        readPosition = 0;
        writePosition = 0;
        fillSize = 0;
        cleared = true;
    }
    
    auto init(uintptr_t handle) -> bool {
		settings.handle = handle;
        
		return init();
	}
    
    auto init() -> bool {
        cleared = false;
        term();
        
        AudioComponentDescription desc = {0};
        desc.componentType = kAudioUnitType_Output;
        desc.componentSubType = kAudioUnitSubType_DefaultOutput;
        desc.componentFlags = 0;
        desc.componentFlagsMask = 0;
        desc.componentManufacturer = kAudioUnitManufacturer_Apple;

        AudioComponent aComp = AudioComponentFindNext(nullptr, &desc);
        
        if (!aComp)
            return false;
        
        if (AudioComponentInstanceNew(aComp, &outputInstance))
            return false;        
        
        if (AudioUnitInitialize( outputInstance ))
            return false;        

        AudioStreamBasicDescription streamFormat;
        memset(&streamFormat, 0, sizeof(streamFormat));
        
        streamFormat.mSampleRate = (Float64)settings.frequency;
        streamFormat.mFormatID = kAudioFormatLinearPCM;
        streamFormat.mFormatFlags = kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
        streamFormat.mFramesPerPacket = 1;
        streamFormat.mChannelsPerFrame = 2;
        streamFormat.mBitsPerChannel = sizeof(int16_t) * 8;
        streamFormat.mBytesPerPacket = sizeof(int16_t) * streamFormat.mChannelsPerFrame;
        streamFormat.mBytesPerFrame = sizeof(int16_t) * streamFormat.mChannelsPerFrame;
        
        if (AudioUnitSetProperty (outputInstance, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0, &streamFormat, sizeof(streamFormat))) 
            return false;        
        
        AURenderCallbackStruct callback;
        callback.inputProc = &CoreAudio::getSamples;
        callback.inputProcRefCon = this;
        
        if (AudioUnitSetProperty (outputInstance, kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Input, 0, &callback, sizeof(AURenderCallbackStruct)))
            return false;        
        
        unsigned bytesPerFrame = sizeof(int16_t) * 2;
        unsigned framesAll = settings.frequency * settings.latency / 1000.0 + 0.5;
        
        unsigned inSize = sizeof(unsigned);
        
        // set latency in audio frames
        AudioUnitSetProperty (outputInstance, kAudioDevicePropertyBufferFrameSize, kAudioUnitScope_Global, 0, &framesAll, inSize);
        
        AudioUnitGetProperty (outputInstance, kAudioDevicePropertyBufferFrameSize, kAudioUnitScope_Global, 0, &framesAll, &inSize);
        
        bytesAll = framesAll * bytesPerFrame * CA_BUFFERS;
        circularBuffer = new uint8_t[ bytesAll ];
        
        AudioUnitSetProperty (outputInstance, kAudioUnitProperty_MaximumFramesPerSlice, kAudioUnitScope_Global, 0, &framesAll, inSize);
        
        AudioUnitGetProperty (outputInstance, kAudioUnitProperty_MaximumFramesPerSlice, kAudioUnitScope_Global, 0, &framesAll, &inSize);
        
        clear();

        if (AudioOutputUnitStart(outputInstance))
            return false;        
        
        return true;        
    }
    
    auto addSamples( const uint8_t* buffer, unsigned size) -> void { 
        cleared = false;
  
        std::chrono::milliseconds duration(1);
        
        std::unique_lock<std::mutex> lk(cvM);
        
        while(size) {
                                    
            while (fillSize == bytesAll) {
                
                if (!settings.synchronize)
                    return;
                
                cv.wait_for(lk, duration);
            }
         
            unsigned stepSize = std::min( size, bytesAll - writePosition );
            
            stepSize = std::min( stepSize, writeAvailable() );
            
            std::unique_lock<std::mutex> lk2(m);
            
            std::memcpy(circularBuffer + writePosition, buffer, stepSize);
            
            lk2.unlock();
            
            size -= stepSize;
            writePosition += stepSize;
            buffer += stepSize;
            
            if (writePosition == bytesAll)
                writePosition = 0; // wrap arround                        
            
            fillSize += stepSize;
            if (fillSize > bytesAll)
                fillSize = bytesAll;
        }
    }  
    
    static auto getSamples(void* inRefCon, AudioUnitRenderActionFlags* ioActionFlags, const AudioTimeStamp* inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList* ioData) -> OSStatus {
        // runs in another thread, created by core audio
        
        auto instance = (CoreAudio*) inRefCon;
        
        return instance->_getSamples( ioActionFlags, inNumberFrames, ioData );
    }
    
    auto _getSamples(AudioUnitRenderActionFlags* ioActionFlags, UInt32 inNumberFrames, AudioBufferList* ioData) -> OSStatus {        
        
        if (!ioData || ioData->mNumberBuffers != 1)
            return noErr;
        
        unsigned writeSize = ioData->mBuffers[0].mDataByteSize;
        uint8_t* buf = (uint8_t*)ioData->mBuffers[0].mData;
        
        if (fillSize < writeSize) {
            //buffer underrun, fill with silence
            std::memset(buf, 0, writeSize);
            
            *ioActionFlags |= kAudioUnitRenderAction_OutputIsSilence;
            
        } else {             

            while (writeSize) {

                unsigned stepSize = std::min( writeSize, bytesAll - readPosition );
                
                std::unique_lock<std::mutex> lk(m);
                
                std::memcpy(buf, circularBuffer + readPosition, stepSize);
                
                lk.unlock();
                
                writeSize -= stepSize;
                readPosition += stepSize;
                buf += stepSize;
                
                if (readPosition == bytesAll)
                    readPosition = 0; // wrap around 
                
                if (fillSize <= stepSize)
                    fillSize = 0;
                else
                    fillSize -= stepSize;
            }            
        }
        
        // we notify main thread, in case it's waiting for empty buffer
        cv.notify_one();
        
        return noErr;
    }
    
    auto expectFloatingPoint() -> bool {
        return false;
    }
    
    inline auto writeAvailable() -> unsigned {
        
        return bytesAll - fillSize;
    }
    
    auto getCenterBufferDeviation() -> double {
        
        int halfSize = (int) (bytesAll / 2);
        
        int avail = writeAvailable();
        
        int deltaMid = avail - halfSize;
        
        return (double) deltaMid / halfSize;
    }

    
    auto term() -> void {
        if (circularBuffer)
            delete[] circularBuffer;
        
        circularBuffer = nullptr;
        
        AudioComponentInstanceDispose( outputInstance );
        outputInstance = nullptr;
    }
    
    ~CoreAudio() { term(); }
};
    
}
