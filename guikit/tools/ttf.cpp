
TTF::TTF(const std::string& path) {
    file = new File(path);
}

TTF::~TTF() {
    file->unload();
    delete file;
}

template<typename T> auto TTF::readData(unsigned& offset, T& result ) -> bool {
    result = 0;
    uint8_t buffer[4];

    if (file->read(buffer, sizeof(T), offset) != sizeof(T))
        return false;

    offset += sizeof(T);
    for( unsigned i = 0; i < sizeof(T); i++ ) {

        unsigned shift = sizeof(T) - 1 - i;

        result |= buffer[i] << ( shift << 3 );
    }

    return true;
}

auto TTF::readBuffer(uint8_t* buffer, unsigned& offset, unsigned length) -> bool {
    if (file->read(buffer, length, offset) != length)
        return false;

    offset += length;
    return true;
}

auto TTF::getFontNames() -> std::vector<std::string> {
    std::vector<std::string> out;
    uint8_t buffer[4];
    unsigned offset = 0;
    unsigned countFonts;
    if (!file->open())
        return {};

    if (!readBuffer(buffer, offset, 4))
        return {};

    if (std::memcmp(buffer, "ttcf", 4) == 0) {
        offset += 4;
        if (!readData<uint32_t>(offset, countFonts))
            return {};

        unsigned fontOffset;
        for(unsigned i = 0; i < countFonts; i++) {
            if (!readData<uint32_t>(offset, fontOffset)) // offset first font
                return out;

            out.push_back(getFontName(fontOffset + 4));
        }
    } else
        out.push_back(getFontName(4));

    return out;
}

auto TTF::getFontName(unsigned offset) -> std::string {
    std::string out = "";
    unsigned offset2 = 0;
    unsigned nameOffset;
    uint8_t buffer[4];

    uint16_t nT;
    uint16_t nC;
    uint16_t so;

    if (!readData<uint16_t>(offset, nT)) // count table entries
        return "";

    offset += 6;

    for (int i = 0; i < nT; i++) {
        if (!readBuffer(buffer, offset, 4))
            return "";

        if (std::memcmp(buffer, "name", 4) == 0) {
            offset += 4;
            if (!readData<uint32_t>(offset, nameOffset)) // count fonts
                return "";

            offset += 4;
            offset2 = nameOffset + 2;

            if (!readData<uint16_t>(offset2, nC)) // count table entries
                return "";

            if (!readData<uint16_t>(offset2, so)) // count table entries
                return "";

            int winPos = -1;
            int macPos = -1;
            unsigned winNameId = 0;
            unsigned macNameId = 0;
            unsigned winLen = 0;
            unsigned macLen = 0;

            for (int j = 0; j < nC; j++) {
                uint16_t plat, enc, langId, nameId;
                if (!readData<uint16_t>(offset2, plat)) // count table entries
                    return "";
                if (!readData<uint16_t>(offset2, enc)) // count table entries
                    return "";
                if (!readData<uint16_t>(offset2, langId)) // count table entries
                    return "";
                if (!readData<uint16_t>(offset2, nameId)) // count table entries
                    return "";

                if (nameId == 4 /* || nameId == 25 || nameId == 16*/) { // full name
                    uint16_t strLen, strOffset;

                    if (!readData<uint16_t>(offset2, strLen)) // count table entries
                        return "";
                    if (!readData<uint16_t>(offset2, strOffset)) // count table entries
                        return "";

                    if (strLen) {
                        if ( (plat == 3) && (enc == 0 || enc == 1) && (winPos == -1 || langId == 0x409) ) {
                          //  if (nameId >= winNameId) {
                                winPos = strOffset;
                                winNameId = nameId;
                                winLen = strLen;
                            //}
                        }

                        if ( (plat == 1) && (enc == 0) && (macPos == -1 || langId == 0) ) {
                           // if (nameId >= macNameId) {
                                macPos = strOffset;
                                macNameId = nameId;
                                macLen = strLen;
                           // }
                        }
                    }
                } else
                    offset2 += 4;
            }

            if (winPos >= 0 || macPos >= 0) {
                if (winPos >= 0) {
                    unsigned offset3 = nameOffset + winPos + so;

                    uint8_t* nameBuf = new uint8_t[winLen];
                    uint8_t* resultBuf = new uint8_t[winLen / 2];

                    if (!readBuffer(nameBuf, offset3, winLen))
                        return "";

                    for (int c = 0; c < winLen / 2; c += 1) {
                        resultBuf[c] = nameBuf[c * 2 + 1];
                    }
                    out.assign((char*)resultBuf, winLen / 2);
                    delete[] nameBuf;
                    delete[] resultBuf;

                } else if (macPos >= 0) {
                    unsigned offset3 = nameOffset + macPos + so;
                    uint8_t* nameBuf = new uint8_t[macLen];

                    if (!readBuffer(nameBuf, offset3, macLen))
                        return "";

                    out.assign((char*)nameBuf, macLen);
                    delete[] nameBuf;
                }
            }

            break;
        }

        offset += 12;
    }

    return out;
}
