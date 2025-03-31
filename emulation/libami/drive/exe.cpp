
namespace LIBAMI {

auto DiskStructure::buildAdfFromBinaries(const std::string& name, std::vector<Emulator::Interface::Item>& files) -> Emulator::Interface::Data {
    unsigned rawSize;
    uint8_t* rawData;
    Emulator::Interface::Item* startFile = nullptr;
    bool hasStartupSequence = false;
    std::string test;

    if (!files.size())
        return { nullptr, 0 };

    std::function<bool(Filesystem& fs, Emulator::Interface::Item&)>
    addItem = [&](Filesystem& fs, Emulator::Interface::Item& item) -> bool {
        if (item.isGroup) {
            if (!fs.createDir(item.name))
                return false;

            if (!fs.changeDir(item.name))
                return false;

            for (auto child : item.childs) {
                if (!addItem(fs, *child))
                    return false;
            }

            if (!fs.changeDir(".."))
                return false;
        } else {
            if (!fs.createFile(item.name, item.data.ptr, item.data.size))
                return false;
        }
   
        return true;
    };

    while(true) {        
        Filesystem fs(getADFCreationImageSize(), Filesystem::Structure::OFS);
        fs.format(!name.empty() ? name : "Volume", true);

        for(auto& item : files) {
            test = item.name;
            Emulator::String::toLowerCase(test);
            if (!hasStartupSequence && Emulator::String::foundSubStr(test, "startup-sequence"))
                hasStartupSequence = true;

            if (!item.isGroup && !hasStartupSequence && (!startFile || Emulator::String::endsWith(test, "exe")))
                startFile = &item;

            if (item.parent)
                continue;

            if (!addItem(fs, item))
                goto tryHD;
        }

        if (!hasStartupSequence && startFile) {
            if (!fs.changeDir("/"))
                goto tryHD;

            if (!fs.createDir("s"))
                goto tryHD;

            if (!fs.changeDir("s"))
                goto tryHD;

            if (!fs.createFile("startup-sequence", startFile->name))
                goto tryHD;
        }

        fs.calculateChecksums();

        rawSize = fs.volSize();
        rawData = new uint8_t[rawSize];
        if (!fs.exportMedia(rawData, rawSize)) {
            delete[] rawData;
            goto tryHD;
        }
        return {rawData, rawSize};

    tryHD:
        if (hd)
            break;
        
        hd = true;
    }

    return {nullptr, 0};
}

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
        if (!fs.exportMedia(rawData, rawSize)) {
            delete[] rawData;
            goto tryHD;
        }

        if (analyzeADF(rawData, rawSize)) {
            data = rawData;
            size = rawSize;
            virtualCreated = true;
            return true;
        }

        delete[] rawData;

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
