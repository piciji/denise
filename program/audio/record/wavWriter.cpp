
#include "wavWriter.h"

namespace AudioRecord {

auto WavWriter::create(std::string path) -> bool {

    file.setFile(path);

    if (!file.open(GUIKIT::File::Mode::Write))
        return false;

    return true;
}

auto WavWriter::writeHeader(unsigned sampleRate, bool useFloat) -> void {

    auto fp = file.getHandle();

    if (!fp)
        return;
    
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
}

auto WavWriter::write(uint8_t* buf, unsigned size) -> void {
    auto fp = file.getHandle();

    if (!fp)
        return;
    
    fwrite(buf, 1, size, fp);
}

auto WavWriter::flush() -> void {

    auto fp = file.getHandle();

    if (fp)
        fflush(fp);
}

auto WavWriter::finish() -> void {

    auto fp = file.getHandle();

    if (!fp)
        return;

    unsigned fileLength = ftell(fp);
    
    writeChunk( fileLength - chunkPos + 8, 4, chunkPos + 4 );
    
    writeChunk( fileLength - 8, 4, 4 );   

    fflush(fp);
}    
    
auto WavWriter::writeChunk( unsigned value, uint8_t size, unsigned offset ) -> void {
    
    uint8_t buffer[size];
    
    auto fp = file.getHandle();
    
    for( unsigned i = 0; i < size; i++ ) {

        buffer[i] = value & 0xff;

        value >>= 8;
    }
    
    if (offset)
        fseek(fp, offset, SEEK_SET);    
    
    fwrite(buffer, 1, size, fp);
}

}

