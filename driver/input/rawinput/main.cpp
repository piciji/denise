
#include "../../tools/win.h"
#include "../../tools/hid.h"
#include "../../tools/tools.h"
#include "../../tools/crc32.h"
#include "../../tools/chronos.h"

#include <hidsdi.h>

namespace DRIVER {
	
#include "hid.cpp"    
#include "worker.cpp"
	
struct RawInput : Input {
	RawWorker rawWorker;
	HANDLE workerThread = nullptr;    
		
	auto poll() -> std::vector<Hid::Device*> {		
		std::vector<Hid::Device*> devices;				
		
		EnterCriticalSection( &rawWorker.mcsSc );
		
		if(rawWorker.deviceChanged) {
            rawWorker.initDevices();
		}
		
		rawWorker.keyboard.poll( devices );

		rawWorker.mouse.poll( devices );
		
		rawWorker.joypad.poll( devices );
				
		LeaveCriticalSection( &rawWorker.mcsSc );
		
		return devices;
	}
	
	auto init( uintptr_t handle ) -> bool {
		term();
		
		rawWorker.mouse.handle = (HWND)handle;
		
		InitializeCriticalSection( &rawWorker.mcsSc );
		
		workerThread = CreateThread(NULL, 0, RawWorker::EntryPoint, (void*)&rawWorker, 0, NULL);

		while(true) {
			Sleep(1);
			EnterCriticalSection( &rawWorker.mcsSc );
			bool ready = rawWorker.ready;
			LeaveCriticalSection( &rawWorker.mcsSc );
			if (ready) break;
		}
		
		return true;
	}	
	
	auto mAcquire() -> void {
		rawWorker.mouse.mAcquire();
	}
	
	auto mUnacquire() -> void {
		rawWorker.mouse.mUnacquire();
	}
	
	auto mIsAcquired() -> bool {
		return rawWorker.mouse.mIsAcquired();
	}
		
	auto term() -> void {
		if(workerThread) TerminateThread(workerThread, 0);
		rawWorker.term();
	}
	
	~RawInput() { term(); } 
};

}
