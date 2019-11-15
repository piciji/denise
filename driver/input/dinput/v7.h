
#pragma once

#define UNICODE

#include "../../tools/hid.h"

namespace DRIVER {
    
struct DInput7 {
    
    virtual auto init(uintptr_t handle) -> bool { return true; }
    virtual auto term() -> void {}
    virtual auto mAcquire() -> void {}
    virtual auto mUnacquire() -> void {}
    virtual auto mIsAcquired() -> bool { return false; }
	virtual auto poll() -> std::vector<Hid::Device*> { return {}; }
	
	virtual ~DInput7() = default;
    static auto create( ) -> DInput7*;
};    
    
}