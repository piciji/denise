
#pragma once

#include <thread>
#include <cstring>

namespace Emulator {  
    
auto setThreadPriorityRealtime( std::thread& th ) -> void;

}
