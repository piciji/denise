
/**
 * v 1.1.1
 */

#ifndef DRIVER_H
#define DRIVER_H

#include <string>
#include <vector>

#include "tools/hid.h"
#include "tools/shaderpass.h"

namespace DRIVER {

struct Video {	
    enum class ShaderType { GLSL, HLSL, NotSupported };
    enum class Filter { Nearest = 0, Linear = 1 };
  
    virtual auto init(uintptr_t handle) -> bool { return true; }
    virtual auto term() -> void {}
    virtual auto lock(unsigned*& data, unsigned& pitch, unsigned _width, unsigned _height) -> bool { return false; }
    virtual auto lock(float*& data, unsigned& pitch, unsigned _width, unsigned _height) -> bool { return false; }
    virtual auto lock(int32_t*& data, unsigned& pitch, unsigned _width, unsigned _height) -> bool { return false; }
    virtual auto unlock(bool disallowShader = false) -> void {}
    virtual auto redraw(bool disallowShader = false) -> void {}
    virtual auto clear() -> void {}
    virtual auto setFilter(Filter filter) -> void {}
	virtual auto setShader(std::vector<ShaderPass*> passes) -> void {}    
    virtual auto setShaderAttribute( std::string _program, std::string attribute, float value ) -> void {}
    virtual auto setShaderAttribute( std::string _program, std::string attribute, int value ) -> void {}
    virtual auto setShaderAttribute( std::string _program, std::string attribute, float* data, unsigned size) -> void {}
    virtual auto setShaderAttribute( std::string _program, std::string attribute, uint32_t* data, unsigned _width, unsigned _height) -> void {}
    virtual auto synchronize(bool state) -> void {}
	virtual auto hasSynchronized() -> bool { return false; }
    virtual auto hardSync(bool state) -> void {}
    virtual auto setThreaded(bool state) -> void {}
    virtual auto hasThreaded() -> bool { return false; }
	virtual auto showMessage(std::string message, bool critical = false) -> void {}
    
    virtual auto shaderFormat() -> ShaderType { return ShaderType::NotSupported; }
	/** direct 3D only */
	virtual auto hintExclusiveFullscreen(bool state, float rate = 0.0) -> void {}

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
    virtual auto setLatency(unsigned value) -> void {}
    virtual auto synchronize(bool state) -> void {}
    virtual auto addSamples( const uint8_t* buffer, unsigned size) -> void {}
    virtual auto getCenterBufferDeviation() -> double { return 0.0; }
    virtual auto expectFloatingPoint() -> bool { return true; }
    virtual auto getMinimumLatency() -> unsigned { return 1; }
    virtual auto setHighPriority(bool state) -> void {}
	virtual auto hasSynchronized() -> bool { return false; }

    virtual ~Audio() = default;
    static auto create(const std::string& driver) -> Audio*;
	static auto available() -> std::vector<std::string>;
	static auto preferred() -> std::string;    
};

struct Input {
    virtual auto init(uintptr_t handle) -> bool { return true; }
    virtual auto term() -> void {}
    virtual auto mAcquire() -> void {}
    virtual auto mUnacquire() -> void {}
    virtual auto mIsAcquired() -> bool { return false; }
	virtual auto poll() -> std::vector<Hid::Device*> { return {}; }
	
	virtual ~Input() = default;
    static auto create(const std::string& driver) -> Input*;
	static auto available() -> std::vector<std::string>;
	static auto preferred() -> std::string;
};

}

#endif
