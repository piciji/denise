
/**
 * v 1.3
 */

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <cstdint>

#include "tools/hid.h"
#include "tools/shaderpass.h"

namespace DRIVER {

struct Viewport {
    unsigned width = 0;
    unsigned height = 0;
    int x = 0;
    int y = 0;
};

struct Video {
    virtual auto init(uintptr_t handle) -> bool { return true; }
    virtual auto term() -> void {}

    // options: bit 0: hold last frame, 7: shader params dirty
    virtual auto lock(unsigned*& data, unsigned& pitch, unsigned _width, unsigned _height, uint8_t options = 0) -> bool { return false; }
    virtual auto lock(float*& data, unsigned& pitch, unsigned _width, unsigned _height, uint8_t options = 0) -> bool { return false; }
    virtual auto redraw(bool disallowShader = false) -> void {}
    virtual auto unlockAndRedraw(bool disallowShader = false, bool freeContext = false) -> void {}
    virtual auto clear() -> void {}
    virtual auto setLinearFilter(bool state) -> void {}
    virtual auto setShader(ShaderPreset* preset) -> void {}
    virtual auto synchronize(bool state) -> void {}
	virtual auto hasSynchronized() -> bool { return false; }
    virtual auto hardSync(bool state) -> void {}
    virtual auto setThreaded(bool state) -> void {}
    virtual auto hasThreaded() -> bool { return false; }

	virtual auto showMessage(std::string message, bool critical = false) -> void {}
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
	/** direct 3D only */
	virtual auto hasExclusiveFullscreen() -> bool { return false; }
	virtual auto hintExclusiveFullscreen(bool state, float rate = 0.0) -> void {}
    virtual auto disableExclusiveFullscreen() -> void {}

    virtual auto activateApp(bool state) -> void {}

    virtual auto canHardSync() -> bool { return false; }
    virtual auto canExclusiveFullscreen() -> bool { return false; }
    virtual auto setRotation(unsigned degree) -> void {}
    virtual auto getRotation() -> unsigned { return 0; }

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
    virtual auto setHighPriority(bool state) -> void {}
	virtual auto hasSynchronized() -> bool { return true; }

    virtual ~Audio() = default;
    static auto create(const std::string& driver) -> Audio*;
	static auto available() -> std::vector<std::string>;
	static auto preferred() -> std::string;    
};

struct Input {
    using KeyCallback = std::function<void ()>;

    virtual auto init(uintptr_t handle) -> bool { return true; }
    virtual auto term() -> void {}
    virtual auto mAcquire() -> void {}
    virtual auto mUnacquire() -> void {}
    virtual auto mIsAcquired() -> bool { return false; }
	virtual auto poll() -> std::vector<Hid::Device*> { return {}; }
    virtual auto setKeyboardCallback( KeyCallback* callback ) -> void {}
	
	virtual ~Input() = default;
    static auto create(const std::string& driver) -> Input*;
	static auto available() -> std::vector<std::string>;
	static auto preferred() -> std::string;
};

}
