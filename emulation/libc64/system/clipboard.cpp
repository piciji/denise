
#include "clipboard.h"
#include "../../tools/petcii.h"
#include "system.h"

namespace LIBC64 {

    auto Clipboard::getText() -> std::string {

        uint8_t rows = 25;
        uint8_t cols = 40;
        Emulator::PetciiConversion conv;
        const char* lineEnding = "\n";
        uint8_t lineEndingLength = strlen(lineEnding);
        char* out;
        char* ptr;
        char* nonWhitespace;
        uint8_t* ram = system->ram;
        uint16_t addr = (system->vicBank << 14) | ((system->vicII->getReg18() & 0xf0) << 6);
        uint8_t data;

        out = new char[rows * (cols + lineEndingLength) + 1];

        ptr = out;

        for (uint8_t row = 0; row < rows; row++) {
            nonWhitespace = ptr;

            for (uint8_t col = 0; col < cols; col++) {

                data = ram[addr++];

                data = conv.decode<true>( conv.encodeScreencode(data) );

                *ptr++ = data;

                if (data != ' ')
                    nonWhitespace = ptr;
            }

            if (nonWhitespace < ptr)
                ptr = nonWhitespace;

            if (row == (rows - 1) )
                break;

            for (uint8_t i = 0; i < lineEndingLength; i++)
                *ptr++ = lineEnding[i];
        }

        *ptr = 0;

        std::string text = out;

        delete[] out;

        return text;
    }

}