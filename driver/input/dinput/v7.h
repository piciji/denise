
#pragma once

#define UNICODE

#include <functional>
#include "../../tools/hid.h"

namespace DRIVER {

using KeyCallback = std::function<void ()>;

struct DInput7 {
    
    virtual auto init(uintptr_t handle) -> bool { return true; }
    virtual auto term() -> void {}
    virtual auto mAcquire() -> void {}
    virtual auto mUnacquire() -> void {}
    virtual auto mIsAcquired() -> bool { return false; }
	virtual auto poll() -> std::vector<Hid::Device*> { return {}; }
	virtual auto setKeyboardCallback( KeyCallback* callback ) -> void {}
	
	virtual ~DInput7() = default;
    static auto create( ) -> DInput7*;
};    
    
}