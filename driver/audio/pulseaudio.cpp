
#include <pulse/pulseaudio.h>

namespace DRIVER {       
    
struct PulseAudio : public Audio {

    struct {
        pa_threaded_mainloop* mainloop;
        pa_context* context;
        pa_stream* stream;
        size_t bufferSize;
    } device;
    
    struct {
        bool synchronize;
        unsigned frequency;
        unsigned latency;
        unsigned minimumLatency;
        uintptr_t handle;        
    } settings;

    bool cleared;
    
	auto synchronize(bool state) -> void {
		settings.synchronize = state;
	}
	
	auto hasSynchronized() -> bool { return settings.synchronize; }

	auto setFrequency(unsigned value) -> void {
		settings.frequency = value;
        if (settings.handle)
            init();
	}

    auto getFrequency() -> unsigned {
        return settings.frequency;
    }

	auto setLatency(unsigned value) -> void {
        settings.latency = std::max(value, settings.minimumLatency);
        if (settings.handle)
            init();
	}   
    
    auto getMinimumLatency() -> unsigned {
        
        return settings.minimumLatency;
    }
    
    static auto context_state_cb(pa_context* c, void* data) -> void {
        PulseAudio* pulse = (PulseAudio*)data;

        switch (pa_context_get_state(c)) {
            case PA_CONTEXT_READY:
            case PA_CONTEXT_TERMINATED:
            case PA_CONTEXT_FAILED:
                pa_threaded_mainloop_signal( pulse->device.mainloop, 0);
                break;
            default:
                break;
        }
    }

    static auto stream_success_cb(pa_stream* s, int success, void* data) -> void {}

    static auto stream_state_cb(pa_stream* s, void* data) -> void {
        PulseAudio* pulse = (PulseAudio*)data;

        switch (pa_stream_get_state(s)) {
            case PA_STREAM_READY:
            case PA_STREAM_FAILED:
            case PA_STREAM_TERMINATED:
                pa_threaded_mainloop_signal( pulse->device.mainloop, 0);
                break;
            default:
                break;
        }
    }

    static auto stream_request_cb(pa_stream* s, size_t length, void* data) -> void {
        PulseAudio* pulse = (PulseAudio*)data;

        pa_threaded_mainloop_signal( pulse->device.mainloop, 0);
    }

    static auto stream_latency_update_cb(pa_stream* s, void* data) -> void {
        PulseAudio* pulse = (PulseAudio*)data;

        pa_threaded_mainloop_signal( pulse->device.mainloop, 0);
    }

    static auto buffer_attr_cb(pa_stream* s, void* data) -> void {
        PulseAudio* pulse = (PulseAudio*)data;
        
        const pa_buffer_attr* serverAttr = pa_stream_get_buffer_attr(s);
        
        if (serverAttr)
            pulse->device.bufferSize = serverAttr->tlength;
    }
    
    auto term() -> void {       
                
        if (device.mainloop)
            pa_threaded_mainloop_stop( device.mainloop );

        if (device.stream) {
            pa_stream_disconnect( device.stream );
            pa_stream_unref( device.stream );
            device.stream = nullptr;
        }

        if (device.context) {
            pa_context_disconnect(device.context);
            pa_context_unref(device.context);
            device.context = nullptr;
        }

        if (device.mainloop) {
            pa_threaded_mainloop_free(device.mainloop);
            device.mainloop = nullptr;          
        }
    }

	auto init(uintptr_t handle) -> bool {
		settings.handle = handle;
		return init();
	}

    auto init() -> bool {
        term();
        cleared = false;
        
        const pa_buffer_attr* serverAttr = NULL;                
        pa_buffer_attr bufferAttr = {0};
        pa_sample_spec spec;
        
        memset(&spec, 0, sizeof(spec));

        device.mainloop = pa_threaded_mainloop_new();

        if (!device.mainloop)
            return false;

        device.context = pa_context_new(pa_threaded_mainloop_get_api( device.mainloop ), "pulseaudio");

        if (!device.context)
            return false;

        pa_context_set_state_callback( device.context, context_state_cb, this);

        if (pa_context_connect(device.context, NULL, PA_CONTEXT_NOFLAGS, NULL) < 0)
            return false;

        pa_threaded_mainloop_lock( device.mainloop );
        if (pa_threaded_mainloop_start( device.mainloop ) < 0)
           return false;

        pa_threaded_mainloop_wait( device.mainloop );

        if (pa_context_get_state( device.context ) != PA_CONTEXT_READY) {
            pa_threaded_mainloop_unlock( device.mainloop );
            return false;
        }

        spec.format   = PA_SAMPLE_FLOAT32LE;
        spec.channels = 2;
        spec.rate     = settings.frequency;
        
        device.stream = pa_stream_new( device.context, "audio", &spec, NULL );

        if ( !device.stream ) {
            pa_threaded_mainloop_unlock( device.mainloop );
            return false;       
        }

        pa_stream_set_state_callback( device.stream, stream_state_cb, this );
        pa_stream_set_write_callback( device.stream, stream_request_cb, this );
        pa_stream_set_latency_update_callback( device.stream, stream_latency_update_cb, this);
        pa_stream_set_buffer_attr_callback( device.stream, buffer_attr_cb, this);

        bufferAttr.maxlength = -1;
        bufferAttr.tlength   = pa_usec_to_bytes( settings.latency * PA_USEC_PER_MSEC, &spec);
        bufferAttr.prebuf    = -1;
        bufferAttr.minreq    = -1;
        bufferAttr.fragsize  = -1;

        //pa_stream_flags_t flags = (pa_stream_flags_t) (PA_STREAM_ADJUST_LATENCY | PA_STREAM_VARIABLE_RATE);

        if (pa_stream_connect_playback( device.stream, NULL, &bufferAttr, PA_STREAM_ADJUST_LATENCY, NULL, NULL) < 0) {
            pa_threaded_mainloop_unlock( device.mainloop );
            return false;  
        }      

        pa_threaded_mainloop_wait( device.mainloop );

        if (pa_stream_get_state(device.stream) != PA_STREAM_READY) {
            pa_threaded_mainloop_unlock( device.mainloop );
            return false;  
        }

        serverAttr = pa_stream_get_buffer_attr( device.stream );
        
        device.bufferSize = serverAttr ? serverAttr->tlength : bufferAttr.tlength;
        
        pa_threaded_mainloop_unlock( device.mainloop );        

        return true;
    }
    
    auto addSamples( const uint8_t* buffer, unsigned size) -> void {
        cleared = false;
        pa_threaded_mainloop_lock( device.mainloop );
        
        while (size) {
            unsigned writable = std::min(size, (unsigned)pa_stream_writable_size( device.stream ));

            if (writable) {
                pa_stream_write( device.stream, buffer, writable, NULL, 0, PA_SEEK_RELATIVE);
                
                buffer += writable;
                size -= writable;
                
            } else if ( settings.synchronize )
                // lets wait until there is an open slot for writing new samples.
                
                // this function unlocks the mainloop lock automatically
                // and locks again before it returns, otherwise the audio rendering
                // thread would be blocked too long.
                pa_threaded_mainloop_wait( device.mainloop );
            else
                break;
        }

        pa_threaded_mainloop_unlock( device.mainloop );
    }

    auto clear() -> void {
        if (cleared) return;

        pa_threaded_mainloop_lock(device.mainloop);
        pa_stream_cork(device.stream, true, stream_success_cb, device.mainloop);
        pa_stream_flush(device.stream, stream_success_cb, device.mainloop);
        pa_stream_cork(device.stream, false, stream_success_cb, device.mainloop);
        pa_threaded_mainloop_unlock( device.mainloop );

        cleared = true;
    }

    auto writeAvailable() -> unsigned {

        pa_threaded_mainloop_lock( device.mainloop );
        unsigned count = pa_stream_writable_size( device.stream );
        pa_threaded_mainloop_unlock( device.mainloop );
        
        return count;
    }
    
    auto getCenterBufferDeviation() -> double {    

        int halfSize = (int) (device.bufferSize / 2);

        int avail = writeAvailable();

        int deltaMid = avail - halfSize;
 
        return (double) deltaMid / halfSize;
    }
    
    auto expectFloatingPoint() -> bool { 
        return true;
    }

    PulseAudio() {
        device.context = nullptr;
        device.mainloop = nullptr;
        device.stream = nullptr;
        device.bufferSize = 0;
        
        settings.synchronize = false;
		settings.frequency = 48000;
		settings.latency = 64;
        settings.minimumLatency = 10;
		settings.handle = 0;
    }

    ~PulseAudio() { term(); }
};

}
