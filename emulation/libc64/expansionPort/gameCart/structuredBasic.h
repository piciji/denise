
/**
 * Structured Basic
 *
 * Written by
 *  ClausS
 *
 **/

#pragma once

namespace LIBC64 {

    // Structured Basic: 2 × 8 KB ROM bei $8000-$9FFF (ROML)
    // IO1 ($DE00-$DE03), READ/WRITE schaltet über die Adresse (A0/A1):
    //   $DE00/$DE01 -> ROM Bank 0
    //   $DE02       -> ROM Bank 1
    //   $DE03       -> RAM sichtbar @ $8000-$9FFF   (PLA bleibt 8K-GAME)
    //
    // ROMH ($A000-$BFFF) wird NIE belegt.

    struct StructuredBasic : GameCart {

        uint8_t bank = 0;        // 0/1 (nur ROM-Fälle)
        bool ramVisible = false;

        // 8K-GAME (GAME=1, EXROM=0)
        StructuredBasic(System* system) : GameCart(system, /*game=*/true, /*exrom=*/false) {
            ram = new uint8_t[0x2000];
        }

        ~StructuredBasic() override {
            delete[] ram;
        }

        auto reset(bool softReset = false) -> void override {
            bank       = 0;
            ramVisible = false;

            cRomL = getChip(0);      // ROM Bank 0 sichtbar
            cRomH = nullptr;         // ROMH unbenutzt

            exRom = false;           // 8K
            game  = true;            // 8K
            system->changeExpansionPortMemoryMode(exRom, game);
        }

        // IO1: WRITE schaltet per Adresse
        auto writeIo1(uint16_t addr, uint8_t value) -> void override {
            handleIo1(addr);
        }

        auto peekIo1(uint16_t addr) -> uint8_t override {
            return 0;
        }

        // IO1: READ schaltet ebenfalls
        auto readIo1(uint16_t addr) -> uint8_t override {
            handleIo1(addr);
            return 0;
        }

        auto peekRomL( uint16_t addr ) -> uint8_t override {
            return readRomL(addr);
        }

        // ROML: im RAM-Modus aus RAM lesen/schreiben, sonst Standard-ROM
        auto readRomL(uint16_t addr) -> uint8_t override {
            if (ramVisible) return ram[addr & 0x1FFF];
            return Cart::readRomL(addr);
        }

        auto writeRomL(uint16_t addr, uint8_t data) -> void override {
            if (ramVisible) {
                ram[addr & 0x1FFF] = data;
            }
            // ROM ignorieren
        }

        // Spiegel-Writes ins ROML-Fenster auch ins RAM mappen
        auto listenToWritesAt80To9F(uint16_t addr, uint8_t data) -> void override {
            if (ramVisible) ram[addr & 0x1FFF] = data;
        }
        auto listenToWritesAtA0ToBF(uint16_t, uint8_t) -> void override { /* ROMH unbenutzt */ }

        // Keine ROMH-Belegung → BASIC bleibt sichtbar
        auto isBootable() -> bool override { return false; }

        // Zwei 8-KB-Chips aus dem CRT (Bank 0/1 bei $8000)
        auto assumeChips() -> void override {
            Cart::assumeChips({ 8192, 8192 });
        }

        auto serializeSwitchedIn(Emulator::Serializer& s) -> void override {
            Cart::serialize( s );
            s.integer( ramVisible );
            s.integer( bank );
            s.array( ram, 8 * 1024 );
        }

    private:
        uint8_t* ram = nullptr; // 8 KB RAM-Fenster @ $8000-$9FFF

        void handleIo1(uint16_t addr) {
            switch (addr & 0x03) {
                case 0: // $DE00 -> ROM Bank 0
                case 1: // $DE01 -> ROM Bank 0
                    bank       = 0;
                    ramVisible = false;
                    cRomL      = getChip(0);
                    cRomH      = nullptr;
                    exRom      = false;  // 8K
                    game       = true;   // 8K
                    break;

                case 2: // $DE02 -> ROM Bank 1
                    bank       = 1;
                    ramVisible = false;
                    cRomL      = getChip(1);
                    cRomH      = nullptr;
                    exRom      = false;  // 8K
                    game       = true;   // 8K
                    break;

                case 3: // $DE03 -> RAM sichtbar (Modul NICHT aus!)
                default:
                    ramVisible = true;
                    cRomL      = nullptr;   // Zugriff geht über readRomL/writeRomL
                    cRomH      = nullptr;
                    exRom      = false;     // weiterhin 8K-GAME
                    game       = true;
                    break;
            }
          //  system->changeExpansionPortMemoryMode(exRom, game);
        }
    };

}