
#pragma once

#define UNICODE

#include "../../tools/hid.h"

// Note: don't debug with selected dinput 5. mouse movement slows down.

namespace DRIVER {
    
struct DInput5 {
    
    virtual auto init(uintptr_t handle) -> bool { return true; }
    virtual auto term() -> void {}
    virtual auto mAcquire() -> void {}
    virtual auto mUnacquire() -> void {}
    virtual auto mIsAcquired() -> bool { return false; }
	virtual auto poll() -> std::vector<Hid::Device*> { return {}; }
	
	virtual ~DInput5() = default;
    static auto create( ) -> DInput5*;
};    
    
}