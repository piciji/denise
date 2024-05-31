
namespace LIBAMI {

auto DiskStructure::analyzeEXE(uint8_t*& data, unsigned& size) -> bool {
    unsigned rawSize;
    uint8_t* rawData;
    hd = false;

    while(true) {
        Filesystem fs(getADFCreationImageSize(), Filesystem::Structure::OFS);
        fs.format("Volume", true);

        if (!fs.createFile("binary", data, size))
            goto tryHD;

        if (!fs.createDir("s"))
            goto tryHD;

        if (!fs.changeDir("s"))
            goto tryHD;

        if (!fs.createFile("startup-sequence", "binary"))
            goto tryHD;

        fs.changeDir("/");

        fs.calculateChecksums();

        rawSize = fs.volSize();
        rawData = new uint8_t[rawSize];
        fs.exportMedia(rawData, rawSize);

        if (analyzeADF(rawData, rawSize)) {
            data = rawData;
            size = rawSize;
            virtualCreated = true;
            return true;
        }

tryHD:
        if (hd) { // already tried
            hd = false;
            break;
        } else
            hd = true;
    }

    return false;
}

}
