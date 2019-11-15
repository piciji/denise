/**
    use this to traverse a big file with low memory usage
    for example to read header informations
**/
struct FileChunk {
    FILE* fp = nullptr;
    unsigned fsize;

    uint8_t* buffer = nullptr;
    unsigned buffferSize = 2 * 1024;
    unsigned bufferOffset = 0;

    bool fetchAhead = true;
    bool fetched = false;

    auto setFilePtr(FILE* fp) -> void {
        fseek(fp, 0, SEEK_END);
        fsize = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        this->fp = fp;
        fetched = false;
    }

    auto getByte(unsigned fileOffset) -> uint8_t {
        if (!fp) return 0;
        if (fileOffset >= fsize) return 0;

        if (fsize <= buffferSize) {
            if (!fetched) {
                fseek(fp, 0, SEEK_SET);
                fread(buffer, 1, fsize, fp);
                fetched = true;
            }
            return buffer[fileOffset];
        }

        int bufferPos = fileOffset - bufferOffset;

        if (!fetched || (bufferPos < 0) || (bufferPos >= buffferSize) ) { //refill
            unsigned shift = fetchAhead ? (buffferSize * 20) / 100
                : (buffferSize * 80) / 100;
            int seekPos = fileOffset - shift;
            
            if (seekPos < 0) seekPos = 0;
            else if ( (seekPos + buffferSize) > fsize) {
                seekPos = fsize - buffferSize;
            }
            fseek(fp, seekPos, SEEK_SET);
            fread(buffer, 1, buffferSize, fp);

            bufferOffset = seekPos;
            bufferPos = fileOffset - bufferOffset;
            fetched = true;
        }
        return buffer[bufferPos];
    }

    auto init() -> void {
        buffer = new uint8_t[buffferSize];
    }

    auto free() -> void {
        if(buffer) delete[](buffer);
        buffer = nullptr;
    }

    FileChunk() { init(); }
    ~FileChunk() { free(); }
};
