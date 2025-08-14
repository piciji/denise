#include "mp3Writer.h"
#include <cstring>

namespace AudioRecord {

    MP3Writer::~MP3Writer() {
        if (mp3Buffer)
            delete[] mp3Buffer;
    }

    auto MP3Writer::init(std::string& path, unsigned sampleRate, bool useFloat) -> bool {

        file.setFile(path);

        if (!file.open(GUIKIT::File::Mode::Write))
            return false;

        this->useFloat = useFloat;
        lame = lame_init();
        lame_set_in_samplerate(lame, sampleRate);
        lame_set_num_channels(lame, 2);
        lame_set_VBR(lame, vbr_default);
        lame_init_params(lame);
        
        unsigned size = (unsigned)((float)sampleRate * 1.25f + 7200.0f); // worst case

        if (!mp3Buffer) {
            mp3Buffer = new uint8_t[size];
            mp3BufferSize = size;
        } else if (size != mp3BufferSize) {
            delete[] mp3Buffer;
            mp3Buffer = new uint8_t[size];
            mp3BufferSize = size;
        }

        size = useFloat ? (sampleRate << 3) : (sampleRate << 2);

        initSampleBuffer(size);

        return true;
    }

    auto MP3Writer::write(uint8_t* buf, unsigned size) -> void {
        if ( (bufferPos + size) <= bufferSize) {
            std::memcpy(sampleBuffer + bufferPos, buf, size);
            bufferPos += size;
            return;
        }

        int written = encode();
        
        if (written > 0)
            file.append(mp3Buffer, written);

        std::memcpy(sampleBuffer, buf, size);
        bufferPos = size;
    }

    auto MP3Writer::finish() -> void {
        if (bufferPos && sampleBuffer) {
            int written = encode();
            if (written > 0)
                file.append(mp3Buffer, written);
            bufferPos = 0;
        }

        lame_close(lame);
        file.unload();
    }

    auto MP3Writer::encode() -> int {
        if (useFloat) {
            float* _buf = (float*)sampleBuffer;
            return lame_encode_buffer_interleaved_ieee_float(lame, _buf, bufferPos >> 3, mp3Buffer, mp3BufferSize);
        }

        short* _buf = (short*)sampleBuffer;
        return lame_encode_buffer_interleaved(lame, _buf, bufferPos >> 2, mp3Buffer, mp3BufferSize);
    }
}