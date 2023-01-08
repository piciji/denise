
#pragma once

#define SERIAL_DEVICE_NOT_PRESENT 0x80

namespace LIBC64 {

struct BaseDevice {

    virtual auto get(uint8_t* data, unsigned int secondary) -> int {
        return SERIAL_DEVICE_NOT_PRESENT;
    }

    virtual auto put(uint8_t data, unsigned int secondary) -> int {
        return SERIAL_DEVICE_NOT_PRESENT;
    }

    virtual auto open(const uint8_t* name, unsigned int length, unsigned int secondary) -> int {
        return SERIAL_DEVICE_NOT_PRESENT;
    }

    virtual auto close(unsigned int secondary) -> int {
        return SERIAL_DEVICE_NOT_PRESENT;
    }

    virtual auto flush(unsigned int secondary) -> void {}

    virtual auto listen(unsigned int secondary) -> void {}

    virtual auto finish(bool sendFinishEvent) -> void {}

    virtual auto reset() -> void {}

};

}

#undef SERIAL_DEVICE_NOT_PRESENT