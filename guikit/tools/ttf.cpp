
#define TTF_CHECK_SIZE(_ptr)  if ((_ptr) > (data + size)) return "";

TTF::TTF(uint8_t* data, unsigned size) {
    this->data = data;
    this->size = size;
}

auto TTF::getFontName() -> std::string {
    std::string out = "";
    if (size < 16)
        return "";

    uint8_t* ptr = data;
    if (std::memcmp(ptr, "ttcf", 4) == 0) {
        ptr += 8;
        unsigned numFonts = (*ptr << 24) | (*(ptr + 1) << 16) | (*(ptr + 2) << 8) | *(ptr + 3);
        if (numFonts == 0)
            return "";

        ptr += 4;
        unsigned firstFontOffset = (*ptr << 24) | (*(ptr + 1) << 16) | (*(ptr + 2) << 8) | *(ptr + 3);
        ptr = data + firstFontOffset;
        TTF_CHECK_SIZE(ptr)
    }

    uint8_t* ptr2;
    ptr += 4;

    TTF_CHECK_SIZE(ptr+2)
    uint16_t nT = (*ptr << 8) | *(ptr + 1);
    ptr += 8;

    for (int i = 0; i < nT; i++) {
        TTF_CHECK_SIZE(ptr + 4)

        if (std::memcmp(ptr, "name", 4) == 0) {
            ptr += 8;
            TTF_CHECK_SIZE(ptr + 4)
            unsigned offset = (*ptr << 24) | (*(ptr + 1) << 16) | (*(ptr + 2) << 8) | *(ptr + 3);
            ptr += 8;
            ptr2 = data + offset + 2;
            TTF_CHECK_SIZE(ptr2 + 4)
            uint16_t nC = (*ptr2 << 8) | *(ptr2 + 1);
            ptr2 += 2;
            uint16_t so = (*ptr2 << 8) | *(ptr2 + 1);
            ptr2 += 2;
            int winPos = -1;
            int macPos = -1;
            unsigned winName = 0;
            unsigned macName = 0;
            unsigned winLen = 0;
            unsigned macLen = 0;

            for (int j = 0; j < nC; j++) {
                TTF_CHECK_SIZE(ptr2 + 12)
                uint16_t plat = (*ptr2 << 8) | *(ptr2 + 1);
                ptr2 += 2;
                uint16_t enc = (*ptr2 << 8) | *(ptr2 + 1);
                ptr2 += 2;
                uint16_t langId = (*ptr2 << 8) | *(ptr2 + 1);                
                ptr2 += 2;
                uint16_t nameId = (*ptr2 << 8) | *(ptr2 + 1);

                if (nameId == 4 /* || nameId == 25 || nameId == 16*/) { // full name
                    ptr2 += 2;
                    uint16_t strLen = (*ptr2 << 8) | *(ptr2 + 1);
                    ptr2 += 2;
                    uint16_t strOffset = (*ptr2 << 8) | *(ptr2 + 1);
                    ptr2 += 2;

                    if (strLen) {
                        if ( (plat == 3) && (enc == 0 || enc == 1) && (winPos == -1 || langId == 0x409) ) {
                            if (nameId > winName) {
                                winPos = strOffset;
                                winName = nameId;
                                winLen = strLen;
                            }
                        }

                        if ( (plat == 1) && (enc == 0) && (macPos == -1 || langId == 0) ) {
                            if (nameId > macName) {
                                macPos = strOffset;
                                macName = nameId;
                                macLen = strLen;
                            }
                        }
                    }
                } else
                    ptr2 += 6;
            }

            if (winPos >= 0 || macPos >= 0) {
                if (winPos >= 0) {
                    uint8_t* ptr3 = data + offset + winPos + so;
                    TTF_CHECK_SIZE(ptr3 + winLen)
                    uint8_t* buffer = new uint8_t[winLen / 2];
                    for (int c = 0; c < winLen / 2; c += 1) {
                        buffer[c] = *(ptr3 + 1);
                        ptr3 += 2;
                    }
                    out.assign((char*)buffer, winLen / 2);
                    delete[] buffer;
                } else if (macPos >= 0) {
                    uint8_t* ptr3 = data + offset + macPos + so;
                    TTF_CHECK_SIZE(ptr3 + macLen)
                    out.assign((char*)ptr3, macLen);
                }
            }

            break;
        }

        ptr += 16;
    }

    return out;
}

#undef TTF_CHECK_SIZE