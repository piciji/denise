
#include "wavWriter.h"
#include <cstring>

namespace AudioRecord {

auto WavWriter::init(std::string& path, unsigned sampleRate, bool useFloat) -> bool {

    file.setFile(path);

    if (!file.open(GUIKIT::File::Mode::Write))
        return false;

    auto fp = file.getHandle();

    if (!fp)
        return false;

    std::string out = "RIFF----WAVEfmt ";

    fputs(out.c_str(), fp);

    writeChunk(16, 4);
    writeChunk(useFloat ? 3 : 1, 2);
    writeChunk(2, 2);
    writeChunk(sampleRate, 4);
    writeChunk((sampleRate * (useFloat ? 32 : 16) * 2) / 8, 4);
    writeChunk(useFloat ? 8 : 4, 2);
    writeChunk(useFloat ? 32 : 16, 2);

    chunkPos = ftell(fp);

    out = "data----";

    fputs(out.c_str(), fp);

    fflush(fp);

    unsigned size = useFloat ? (sampleRate << 3) : (sampleRate << 2);

    initSampleBuffer(size);

    return true;
}

auto WavWriter::write(uint8_t* buf, unsigned size) -> void {
    if ((bufferPos + size) <= bufferSize) {
        std::memcpy(sampleBuffer + bufferPos, buf, size);
        bufferPos += size;
        return;
    }

    file.append(sampleBuffer, bufferPos);

    std::memcpy(sampleBuffer, buf, size);
    bufferPos = size;
}

auto WavWriter::finish() -> void {
    if (bufferPos && sampleBuffer) {
        file.append(sampleBuffer, bufferPos);
        bufferPos = 0;
    }

    auto fp = file.getHandle();

    if (!fp)
        return;

    unsigned fileLength = ftell(fp);
    
    writeChunk( fileLength - chunkPos + 8, 4, chunkPos + 4 );
    
    writeChunk( fileLength - 8, 4, 4 );

    fflush(fp);

    file.unload();
}    
    
auto WavWriter::writeChunk( unsigned value, uint8_t size, unsigned offset ) -> void {
    uint8_t buf[4];
    
    auto fp = file.getHandle();
    
    for( unsigned i = 0; i < size; i++ ) {

        buf[i] = value & 0xff;

        value >>= 8;
    }
    
    if (offset)
        fseek(fp, offset, SEEK_SET);    
    
    fwrite(&buf[0], 1, size, fp);
}

}
