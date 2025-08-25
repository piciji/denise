
/**
 * v 2.2
 */

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <cstdint>

#include "tools/hid.h"
#include "tools/shaderpass.h"
#define MAX_SHADERS 64
#define MAX_TEXTURES 64
#define MAX_FRAME_HISTORY 128

namespace DRIVER {

struct Viewport {
    unsigned width = 0;
    unsigned height = 0;
    int x = 0;
    int y = 0;
};

struct DiskFile {
    std::string ident = "";
    std::string path = "";
    bool isLUT = false;
    uint8_t* data = nullptr;
    unsigned size = 0;
    unsigned width = 0;
    unsigned height = 0;
};

struct ScreenTextDescription {
	enum Position { POSITION_BOTTOM_RIGHT, POSITION_BOTTOM_CENTER, POSITION_BOTTOM_LEFT, POSITION_TOP_RIGHT, POSITION_TOP_CENTER, POSITION_TOP_LEFT };
	Position position = POSITION_BOTTOM_RIGHT;
	std::string fontPath;
	unsigned fontIndex;
	unsigned fontSize;
	unsigned fontColor;
	unsigned backgroundColor;
	unsigned warnColor;
	unsigned warnBackgroundColor;
	unsigned paddingHorizontal;
	int paddingVertical;
	unsigned marginHorizontal;
	int marginVertical;
};

enum Options { OPT_HoldFrame = 1, OPT_Interlace = 2, OPT_DisallowShader = 4, OPT_TakeScreenshot = 8, OPT_DisallowFilter = 16 };
enum Rotation { ROT_0, ROT_90, ROT_180, ROT_270 };

struct Video {
    using ScreenshotCallback = std::function<void (uint8_t* _data, unsigned _width, unsigned _height)>;

    virtual auto init(uintptr_t handle) -> bool { return true; }
    virtual auto term() -> void {}

    virtual auto lock(unsigned*& data, unsigned& pitch, unsigned _width, unsigned _height, uint8_t options = 0) -> bool { return false; }
    virtual auto lock(float*& data, unsigned& pitch, unsigned _width, unsigned _height, uint8_t options = 0) -> bool { return false; }
    virtual auto redraw(bool disallowShader = false) -> void {}
    virtual auto unlockAndRedraw() -> void {}
    virtual auto clear() -> void {}
    virtual auto setLinearFilter(bool state) -> void {}
    virtual auto setShader(ShaderPreset* preset) -> void {}
    virtual auto setShaderProgressCallback( std::function<void (int pass, bool hasErrors)> onCallback ) -> void {}
    virtual auto setShaderCacheCallback( std::function<void (DiskFile& diskFile)> onCallback ) -> void {}
    virtual auto useShaderCache(bool state) -> void {}
    virtual auto getShaderNativeVertexCode(std::string& slang, std::string& out) -> bool { return false; }
    virtual auto getShaderNativeFragmentCode(std::string& slang, std::string& out) -> bool { return false; }
    virtual auto synchronize(bool state) -> void {}
	virtual auto hasSynchronized() -> bool { return false; }
    virtual auto hardSync(bool state) -> void {}
    virtual auto setThreaded(bool state) -> void {}
    virtual auto hasThreaded() -> bool { return false; }
    virtual auto waitRenderThread() -> void {}
    virtual auto setProgressAnimation(uint8_t* _data, unsigned _width, unsigned _height) -> void {}

	virtual auto showScreenText(const std::string text, unsigned duration, bool warn = false) -> void {}
	virtual auto setScreenTextDescription(ScreenTextDescription& desc) -> void {}
	virtual auto freeFont() -> void {}
    virtual auto forceResize() -> void {}
    virtual auto freeContext() -> void {}
    virtual auto lockResize() -> void {}
    virtual auto unlockResize() -> void {}

    virtual auto hintResizing(bool state) -> void {}
    virtual auto needResizingPreparations(bool useEmuThread) -> bool { return false; }
    virtual auto prepareResizing() -> void {}
    virtual auto endResizing() -> void {}

    virtual auto setDragnDropOverlay(uint8_t* _data, unsigned _width, unsigned _height, unsigned line = 0) -> void {}
    virtual auto setDragnDropOverlaySlots(unsigned slots) -> void {}
    virtual auto enableDragnDropOverlay(bool state) -> void {}
    virtual auto sendDragnDropOverlayCoordinates(int x, int y) -> int { return 0; }

    virtual auto setVRR(bool state, float speed = 0.0) -> void {}
    virtual auto hasVRR() -> bool { return false; }
    virtual auto changeThreadPriorityToRealtime(bool state) -> void {}

    virtual auto setAspectRatio(int mode, bool _integerScaling) -> void {} // mode: 0: off, 1: TV, 2: Native, 3: Native Alt
    virtual auto getAspectRatio() -> int { return 0; }
    virtual auto getViewport() -> Viewport& { static Viewport vp = {0}; return vp; }
    virtual auto setIntegerScalingDimension( unsigned _w, unsigned _h, bool _ds) -> void {}
    virtual auto getIntegerScalingDimension(unsigned& _w, unsigned& _h) -> void { }

    virtual auto shaderSupport() -> bool { return false; }
    virtual auto setScreenshotCallback(ScreenshotCallback callback) -> void {}
	/** direct 3D only */
	virtual auto hasExclusiveFullscreen() -> bool { return false; }
	virtual auto hintExclusiveFullscreen(bool state, float rate = 0.0) -> void {}
    virtual auto disableExclusiveFullscreen() -> void {}

    virtual auto activateApp(bool state) -> void {}

    virtual auto canHardSync() -> bool { return false; }
    virtual auto canExclusiveFullscreen() -> bool { return false; }
    virtual auto setRotation(Rotation rotation) -> void {}
    virtual auto getRotation() -> Rotation { return ROT_0; }

    virtual auto setHDR(bool state) -> void {}
    virtual auto setHDRParams(float maxNits, float paperWhiteNits, float contrast, bool expandGamut) -> void {}

    virtual ~Video() = default;
    static auto create(const std::string& driver) -> Video*;
	static auto available() -> std::vector<std::string>;
	static auto preferred() -> std::string;
};

struct Audio {
    virtual auto init(uintptr_t handle) -> bool { return true; }
    virtual auto term() -> void {}
    virtual auto clear() -> void {}        
    virtual auto setFrequency(unsigned value) -> void {}
    virtual auto getFrequency() -> unsigned { return 48000u; }
    virtual auto setLatency(unsigned value) -> void {}
    virtual auto synchronize(bool state) -> void {}
    virtual auto addSamples( const uint8_t* buffer, unsigned size) -> void {}
    virtual auto getCenterBufferDeviation() -> double { return 0.0; }
    virtual auto expectFloatingPoint() -> bool { return true; }
    virtual auto getMinimumLatency() -> unsigned { return 1; }
	virtual auto hasSynchronized() -> bool { return true; }

    virtual ~Audio() = default;
    static auto create(const std::string& driver) -> Audio*;
	static auto available() -> std::vector<std::string>;
	static auto preferred() -> std::string;    
};

struct Input {
    using KeyCallback = std::function<void ()>;
    enum class Button : unsigned { Left = 1u, Right = 2u, Middle = 4u, Back = 8u, Forward = 16u };

    virtual auto init(uintptr_t handle) -> bool { return true; }
    virtual auto term() -> void {}
    virtual auto mAcquire() -> void {}
    virtual auto mUnacquire() -> void {}
    virtual auto mIsAcquired() -> bool { return false; }
	virtual auto poll() -> std::vector<Hid::Device*> { return {}; }
    virtual auto setKeyboardCallback( KeyCallback* callback ) -> void {}
    // mac only (at the moment)
    virtual auto sentUIKeyPresses(bool keyDown, uint16_t keyCode) -> void {}
    virtual auto sentUIMousePresses(bool keyDown, Button button) -> void {}
    virtual auto sentUIMouseMovement(int deltaX, int deltaY) -> void {}
	
	virtual ~Input() = default;
    static auto create(const std::string& driver) -> Input*;
	static auto available() -> std::vector<std::string>;
	static auto preferred() -> std::string;
};

}
