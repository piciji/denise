
#include <mutex>
#include <atomic>
#include <cstring>

namespace DRIVER {

struct DragndropOverlay {

    DragndropOverlay() {
        slotCount = 0; // disable
        viewport.width = 0;
        viewport.height = 0;
        buffer = nullptr;
        enable = false;
    }

    virtual ~DragndropOverlay() {
        term();
    }

    const static unsigned SLOT_WIDTH = 10; // percent of screen
    const static unsigned SLOT_SPACE = 2; // percent of screen
    const static unsigned LINE_SPACE = 2; // percent of screen
    const static unsigned MAX_SLOTS = 6;
    const static unsigned MAX_LINES = 2;

    unsigned slotCount;
    Viewport viewport;

    struct Slot {
        float alpha;
        std::atomic<bool> alphaIncrease;
        int x;
        int y;
        unsigned width;
        unsigned height;
    };

    struct Line {
        uint8_t* imgData = nullptr;
        unsigned imgWidth = 0;
        unsigned imgHeight = 0;
        Slot slots[MAX_SLOTS];
    } lines[MAX_LINES];

    bool initialized = false;
    std::atomic<bool> enable;
    Video::DnDOverlayCallback callback = nullptr;

    unsigned texX = 0;
    unsigned texY = 0;
    unsigned texWidth = 0;
    unsigned texHeight = 0;

    uint8_t* buffer = nullptr;

    std::mutex updateMutex;

    auto sendDragnDropOverlayCoordinates(int x, int y) -> int {
        if (!enabled())
            return 0;

        int result = 0;

        for (int l = 0; l < MAX_LINES; l++) {
            Line& line = lines[l];
            if (line.imgData == nullptr)
                continue;

            for (int s = 0; s < slotCount; s++) {
                Slot& slot = line.slots[s];
                int compareX = slot.x + texX + viewport.x;
                int compareY = slot.y + texY + viewport.y;
                if ((compareX <= x) && ((compareX + slot.width) > x)) {

                    if ((compareY <= y) && ((compareY + slot.height) > y)) {

                        if (!slot.alphaIncrease)
                            slot.alphaIncrease = true;
                        result = 0x8000 | s | (l << 8);
                        continue;
                    }
                }

                if (slot.alphaIncrease)
                    slot.alphaIncrease = false;
            }
        }
        return result;
    }

    auto updateAlpha() -> bool {
        bool bufferChanged = false;

        for (int l = 0; l < MAX_LINES; l++) {
            Line& line = lines[l];
            if (line.imgData == nullptr)
                continue;

            for (int s = 0; s < slotCount; s++) {
                Slot& slot = line.slots[s];
                bufferChanged |= setAlpha(line, slot);
            }
        }

        if (bufferChanged)
            updateBuffer();

        return bufferChanged;
    }

    auto setAlpha(Line& line, Slot& slot) -> bool {
        if (slot.alphaIncrease) {
            if (slot.alpha < 1.0) slot.alpha += 0.05;
            else return false;
        } else {
            if (slot.alpha > 0.5) slot.alpha -= 0.05;
            else return false;
        }

        for (unsigned h = 0; h < slot.height; h++) {
            uint8_t* src = line.imgData + h * (line.imgWidth * 4);
            uint8_t* dest = buffer + (slot.y + h) * texWidth * 4 + slot.x * 4;

            for (unsigned w = 0; w < slot.width; w++) {
                dest[3] = slot.alpha * src[3];
                dest += 4;
                src += 4;
            }
        }

        return true;
    }

    virtual auto updateBuffer() -> void {}

    auto update(Viewport& _viewport) -> void {
        if (!callback)
            return;

        if ((viewport.width == _viewport.width) && (viewport.height == _viewport.height))
            return;

        updateMutex.lock();
        viewport = _viewport;
        unsigned _slotCount = slotCount;
        unsigned slotSpace = getSlotSpaceWidth();
        unsigned lineSpace = getLineSpaceWidth();
        unsigned slotWidth = getSlotWidth();
        updateMutex.unlock();

        if (buffer) {
            delete[] buffer;
            buffer = nullptr;
        }

        if (!_slotCount)
            return;

        texX = 0;
        texY = 0;

        int bufferHeight = 0;
        int bufferWidth = slotWidth * _slotCount;
        if (_slotCount > 1)
            bufferWidth += slotSpace * (_slotCount - 1);
        if (bufferWidth < viewport.width)
            texX += (viewport.width - bufferWidth) / 2;
        else if (bufferWidth > viewport.width)
            bufferWidth = viewport.width;

        int linePos = 0;
        for (auto& line : lines) {
            Viewport imgVp;
            imgVp.width = line.imgWidth = slotWidth;

            line.imgData = callback(imgVp, linePos++);

            line.imgHeight = imgVp.height;

            bufferHeight += line.imgHeight + lineSpace;
        }

        bufferHeight -= lineSpace;

        if (bufferHeight <= 0)
            return;

        if (bufferHeight < viewport.height)
            texY += (viewport.height - bufferHeight) / 2;
        else if (bufferHeight > viewport.height)
            bufferHeight = viewport.height;

        buffer = new uint8_t[bufferWidth * bufferHeight * 4];
        std::memset(buffer, 0, bufferWidth * bufferHeight * 4);

        int y = 0;
        for (auto& line: lines) {
            if (line.imgData == nullptr)
                continue;

            int x = 0;
            for (int s = 0; s < slotCount; s++) {
                Slot& slot = line.slots[s];
                slot.x = x;
                slot.y = y;
                slot.alpha = 0.5;
                slot.alphaIncrease = false;
                slot.height = line.imgHeight;
                slot.width = line.imgWidth;

                if ((slot.height + slot.y) > bufferHeight)
                    slot.height = bufferHeight - slot.y;
                if ((slot.width + slot.x) > bufferWidth)
                    slot.width = bufferWidth - slot.x;

                x += line.imgWidth + slotSpace;

                for (int h = 0; h < slot.height; h++) {
                    uint32_t* src = (uint32_t*) (line.imgData + h * (line.imgWidth * 4));
                    uint32_t* dest = (uint32_t*) (buffer + (h + slot.y) * bufferWidth * 4 + slot.x * 4);

                    for (int w = 0; w < slot.width; w++) {
                        uint32_t argb = *src++;
                        *dest++ = (argb & 0xffffff) | ((argb >> 25) << 24);
                    }
                }

                if (x >= bufferWidth)
                    break;
            }

            y += line.imgHeight + lineSpace;

            if (y >= bufferHeight)
                break;
        }

        texWidth = bufferWidth;
        texHeight = bufferHeight;
        buildTexture(bufferWidth, bufferHeight);
    }

    virtual auto buildTexture(unsigned width, unsigned height) -> void {}

    auto getSlotWidth() -> unsigned {
        if (slotCount == 0)
            return 0;

        int rel = SLOT_WIDTH + MAX_SLOTS - slotCount;

        return viewport.width * rel / 100;
    }

    auto getSlotSpaceWidth() -> unsigned {
        unsigned spaceWidth = 0;
        if (slotCount > 1)
            spaceWidth = viewport.width * SLOT_SPACE / 100;

        return spaceWidth;
    }

    auto getLineSpaceWidth() -> unsigned {
        return viewport.width * LINE_SPACE / 100;
    }

    auto enabled()->bool {
        return enable && initialized;
    }

    auto setEnable(bool state) -> void {
        enable = state;
    }

    auto setSlots(unsigned slotCount) -> void {
        if (this->slotCount == slotCount)
            return;

        if (slotCount > MAX_SLOTS)
            slotCount = MAX_SLOTS;

        updateMutex.lock();
        this->slotCount = slotCount;
        viewport.width = 0;
        viewport.height = 0;
        updateMutex.unlock();
    }

    virtual auto term() -> void {
        if (buffer) {
            delete[] buffer;
            buffer = nullptr;
        }
    }

    virtual auto init() -> bool { return false; }
};

}
