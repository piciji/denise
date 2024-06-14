
namespace LIBAMI {

template<bool CIA2> auto System::dongleCiaWrite(Cia<MOS_8520>::Lines* lines) -> void {
    switch(dongle.type) {
        case DongleRoboCop3:
            if (!CIA2) {
                if (lines->ioa & 0x80)
                    dongle.control ^= 1;
            }
            break;
        case DongleBat2:
            if (CIA2) {
                if ((lines->ioa & 0x80) == 0) {
                    dongle.control = 1;
                    dongle.clock = agnus.clock;
                }
            }
            break;
        default: break;

    }
}

template<bool CIA2> auto System::dongleCiaRead(Cia<MOS_8520>::Lines* lines, uint8_t& val) -> void {
    switch(dongle.type) {
        case DongleBat2:
            if (CIA2) {
                if (!dongle.control || (agnus.fallBackCycles(dongle.clock) > 0xe2) ) {
                    val &= ~0x10;
                    dongle.control = 0;
                } else
                    val |= 0x10;
            }
            break;
        default: break;
    }
}

template<bool portB> auto System::dongleJoydat(uint16_t& val) -> void {
    switch(dongle.type) {
        case DongleRoboCop3:
            if (portB && dongle.control) {
                val += 0x100;
            }
            break;
        case DongleCricketCaptain:
            if (!portB) {
                val &= ~0x3;
                if (dongle.control == 0)
                    val |= 0x1;
                else
                    val |= 0x2;
            }
            dongle.control ^= 1;
            break;
        case DongleLeaderBoard:
            if (portB) {
                val &= ~0x303;
                val |= 0x101;
            }
            break;

        case DongleRugbyCoach:
            if (portB) {
                val &= ~0x303;
                val |= 0x301;
            }
            break;
        case DongleStrikerManager:
            if (portB) {
                if (dongle.control >= 4) {
                    val &= ~0x0303;
                    val |= 0x0203;
                    dongle.control--;
                } else if (dongle.control > 0) {
                    val &= ~0x0303;
                    val |= 0x0200;
                }
            }
            break;
        default: break;
    }
}

auto System::donglePotGo(uint16_t& val) -> void {
    switch(dongle.type) {
        case DongleStrikerManager:
            if ((val & 0x0500) == 0x0500) {
                dongle.control++;
            } else {
                if (dongle.control > 0)
                    dongle.control--;
            }
            break;
        default: break;
    }
}

}
