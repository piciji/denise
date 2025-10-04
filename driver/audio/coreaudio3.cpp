
#import <Foundation/Foundation.h>
#import <AudioToolbox/AudioToolbox.h>
#import <AVFoundation/AVFoundation.h>
#include <atomic>
#include <cstdint>
#include <algorithm>

namespace DRIVER {
    struct CoreAudio3;
};

@interface CoreAudio3Obj : NSObject {
   AUAudioUnit* au;
}

- (instancetype)initWithRate:(NSUInteger)rate reference:(DRIVER::CoreAudio3*)ref;
- (void)free;
@end

namespace DRIVER {
    
struct CoreAudio3 : public Audio {
    
    dispatch_semaphore_t semaphore;
    
    unsigned bytesAll = 0;
    std::atomic<unsigned> fillSize;
    uint8_t* circularBuffer = nullptr;
    unsigned readPosition;
    unsigned writePosition;
    bool cleared = false;
    CoreAudio3Obj* coreAudio3Obj;
    
    struct {
        bool synchronize = false;
        unsigned frequency = 48000;
        unsigned latency = 64;
        unsigned minimumLatency = 2;
        uintptr_t handle;
    } settings;
    
    auto synchronize(bool state) -> void {
        settings.synchronize = state;
        if(coreAudio3Obj)
            init();
    }
    
    auto hasSynchronized() -> bool { return settings.synchronize; }
    
    auto setFrequency(unsigned value) -> void {
        settings.frequency = value;
        if(coreAudio3Obj)
            init();
    }

    auto getFrequency() -> unsigned {
        return settings.frequency;
    }
    
    auto setLatency(unsigned value) -> void {
        settings.latency = std::max(settings.minimumLatency, value);
        if(coreAudio3Obj)
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
        semaphore = dispatch_semaphore_create(0);
        
        unsigned bytesPerFrame = sizeof(float) * 2;
        unsigned framesAll = settings.frequency * settings.latency / 1000.0f + 0.5f;
        bytesAll = framesAll * bytesPerFrame;
        circularBuffer = new uint8_t[ bytesAll ];
        clear();
        
        coreAudio3Obj = [[CoreAudio3Obj alloc] initWithRate:settings.frequency reference:this];
        if (coreAudio3Obj)
            return true;
        
        return false;
    }
    
    auto addSamples( const uint8_t* buffer, unsigned size) -> void {
        cleared = false;
        
        while(size) {
                                    
            while (fillSize == bytesAll) {
                
                if (!settings.synchronize)
                    return;
                
                dispatch_semaphore_wait(semaphore, DISPATCH_TIME_FOREVER);
            }
         
            unsigned stepSize = std::min( size, bytesAll - writePosition );
            
            stepSize = std::min( stepSize, writeAvailable() );
            
            std::memcpy(circularBuffer + writePosition, buffer, stepSize);
            
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
    
    auto getSamples(uint8_t* buf, size_t len) -> void {
        unsigned writeSize = len * 8;

        while (writeSize) {
            unsigned stepSize = std::min( writeSize, bytesAll - readPosition );
            
            if (fillSize < stepSize)
                stepSize = fillSize;
            
            if (stepSize == 0) { // buffer underrun, fill with silence
                std::memset(buf, 0, writeSize);
                break;
            }
            
            std::memcpy(buf, circularBuffer + readPosition, stepSize);
            
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
               
        dispatch_semaphore_signal(semaphore);
    }
    
    auto expectFloatingPoint() -> bool {
        return true;
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
        if (circularBuffer) {
            delete[] circularBuffer;
            circularBuffer = nullptr;
        }
        
        if (coreAudio3Obj) {
            [coreAudio3Obj free];
            [coreAudio3Obj release];
            coreAudio3Obj = nil;
        }
    }
    
    ~CoreAudio3() { term(); }
};
    
}


@implementation CoreAudio3Obj

- (instancetype)initWithRate:(NSUInteger)rate reference:(DRIVER::CoreAudio3*)ref {
    if (self = [super init]) {
        NSError* err;
        
        AudioComponentDescription desc = {0};
        desc.componentType = kAudioUnitType_Output;
        desc.componentSubType = kAudioUnitSubType_DefaultOutput;
        desc.componentFlags = 0;
        desc.componentFlagsMask = 0;
        desc.componentManufacturer = kAudioUnitManufacturer_Apple;

        au = [[AUAudioUnit alloc] initWithComponentDescription:desc error:&err];
        if (err != nil)
            return nil;

        AVAudioFormat* format = au.outputBusses[0].format;
        if (format.channelCount < 2)
            return nil;
        
        AVAudioFormat* renderFormat = [[AVAudioFormat alloc] initWithCommonFormat:AVAudioPCMFormatFloat32 sampleRate:rate channels:2 interleaved:true];
        
        [au.inputBusses[0] setFormat:renderFormat error:&err];
        if (err != nil)
            return nil;

        au.outputProvider = ^AUAudioUnitStatus(AudioUnitRenderActionFlags* actionFlags, const AudioTimeStamp* timestamp, AUAudioFrameCount frameCount, NSInteger inputBusNumber, AudioBufferList* inputData) {
            ref->getSamples((uint8_t*)inputData->mBuffers[0].mData, frameCount);
            return 0;
        };

        [au allocateRenderResourcesAndReturnError:&err];
        if (err != nil)
            return nil;

        [au startHardwareAndReturnError:&err];
        if (err != nil)
            return nil;
   }
   return self;
}

- (void)free {
    [au stopHardware];
    [au deallocateRenderResources];
}

@end
