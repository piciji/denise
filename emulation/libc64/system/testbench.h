
#pragma once

#include "system.h"
#include "../vicII/base.h"
#include "../expansionPort/reu/reu.h"

namespace LIBC64 {

    struct DebugCart {

        bool enable = false;
        bool exit = false;
        uint8_t exitCode;
        unsigned cycles;
        unsigned frames;
        unsigned frameCounter = 0;
        bool delayFrame = false;

        auto setExit(uint8_t exitCode) -> void {
            if (exit)
                return;
            this->exitCode = exitCode;
            exit = true;

            if (!vicII->inVisibleArea())
                system->leaveEmulation = true;
            else if (dynamic_cast<Reu*>(expansionPort))
                delayFrame = true;
        }

        auto set(bool enable, unsigned cycles = 0) -> void {
            this->enable = enable;
            this->cycles = cycles;
        }

        auto init() -> void {
            exit = false;
            delayFrame = false;

            if (!enable)
                return;

            frames = cycles / vicII->cyclesPerFrame();
            frames += 1;

            frameCounter = 0;
        }

        auto check() -> void {
            if (!enable)
                return;

            if (exit) {
                if (delayFrame) {
                    delayFrame = false;
                    return;
                }

                system->interface->exit(exitCode);
                return;
            }

            if (!cycles)
                return;

            if (++frameCounter == frames) {
                exit = true;
                system->interface->exit(1);
            }
        }
    };
}