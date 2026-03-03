
#define DRV_UNIT
#define _USE_MATH_DEFINES
#include <cmath>
#include "driver.h"
#include "video/dragnDropOverlay.h"
#include "video/splashScreen.h"

#ifdef DRV_DIRECT3D9
	#include "video/d3d9/dvideo.cpp"
#endif

#ifdef DRV_DIRECT3D11
    #include "video/d3d11/d3d11.cpp"
#endif

#ifdef DRV_DSOUND
	#include "audio/dsound.cpp"
#endif

#if defined(DRV_XAUDIO27) || defined(DRV_XAUDIO28) || defined(DRV_XAUDIO29)
	#include "audio/xaudio2/xaudio2.h"
#endif

#if defined(DRV_DINPUT5) || defined(DRV_DINPUT7) || defined(DRV_DINPUT8)
    #include "input/dinput/dinput.h"
#endif

#ifdef DRV_RAWINPUT
	#include "input/rawinput/main.cpp"
#endif

#ifdef DRV_WGL
	#include "video/opengl3/wgl.cpp"
#endif

#ifdef DRV_CGL
	#include "video/opengl3/cgl.cpp"
#endif

#ifdef DRV_GLX
	#include "video/opengl3/glx.cpp"
#endif

#ifdef DRV_METAL
    #include "video/metal/metal.cpp"
#endif

#ifdef DRV_OPENAL
	#include "audio/openal.cpp"
#endif

#ifdef DRV_PULSEAUDIO
	#include "audio/pulseaudio.cpp"
#endif

#ifdef DRV_WASAPI
	#include "audio/wasapi.cpp"
#endif

#ifdef DRV_COREAUDIO
	#include "audio/coreaudio.cpp"
#endif

#ifdef DRV_COREAUDIO3
    #include "audio/coreaudio3.cpp"
#endif

#ifdef DRV_UDEV
	#include "input/udev.cpp"
#endif

#ifdef DRV_UHID
	#include "input/uhid.cpp"
#endif

#ifdef DRV_IOKIT
	#include "input/iokit/main.cpp"
#endif

#ifdef DRV_XLIB
	#include "input/xlib.cpp"
#endif

#ifdef DRV_XCB
	#include "input/xcb.cpp"
#endif

namespace DRIVER {

auto Video::available() -> std::vector<std::string> {
    std::vector<std::string> out = {
    #ifdef DRV_DIRECT3D11
        "Direct3D11",
    #endif

	#ifdef DRV_DIRECT3D9
		"Direct3D9",
	#endif
    };
    
    #ifdef DRV_METAL
    if (@available(macOS 10.13, *)) {
        out.push_back("Metal");
    }
    #endif
        
	#if defined(DRV_WGL) || defined(DRV_CGL) || defined(DRV_GLX)
    out.push_back("OpenGL");
	#endif
	
    return out;
}

auto Video::preferred() -> std::string {
    #ifdef DRV_DIRECT3D11
        return "Direct3D11";
    #endif

	#ifdef DRV_DIRECT3D9
		return "Direct3D9";
	#endif

    #ifdef DRV_METAL
    if (@available(macOS 10.13, *)) {
        return "Metal";
    }
    #endif
    
	#if defined(DRV_WGL) || defined(DRV_CGL) || defined(DRV_GLX)
		return "OpenGL";
	#endif

	return "";
}

auto Video::create(const std::string& driver) -> Video* {
    #ifdef DRV_DIRECT3D11
        if(driver == "Direct3D11") return new D3D11( !Win::hasAppThemed() || (Win::getVersion() <= Win::Windows7) );
    #endif

    #ifdef DRV_DIRECT3D9
        if(driver == "Direct3D9") return new D3D9( !Win::hasAppThemed() || (Win::getVersion() <= Win::WindowsXP) );
	#endif

	#ifdef DRV_WGL
		if(driver == "OpenGL") return new WGL();
	#endif

	#ifdef DRV_CGL
		if(driver == "OpenGL") return new CGL();
	#endif

	#ifdef DRV_GLX
		if(driver == "OpenGL") return new GLX();
	#endif
    
    #ifdef DRV_METAL
    if (@available(macOS 10.13, *)) {
        if(driver == "Metal") return new METAL();
    }
    #endif

    return new Video;
}

auto Audio::available() -> std::vector<std::string> {
    std::vector<std::string> list;

    #ifdef DRV_XAUDIO29
		if (Win::getVersion() >= 0x0a00)
            list.push_back("XAudio 2.9");
	#endif

    #ifdef DRV_XAUDIO28
		if (Win::getVersion() >= 0x0602)
            list.push_back("XAudio 2.8");
	#endif

    #ifdef DRV_XAUDIO27
        if (Win::getVersion() >= 0x0501)
            list.push_back("XAudio 2.7");
	#endif

    #ifdef DRV_WASAPI
		list.push_back("Wasapi Exclusive");
	#endif

    #ifdef DRV_WASAPI
		list.push_back("Wasapi Shared");
	#endif

	#ifdef DRV_PULSEAUDIO
		list.push_back("PulseAudio");
	#endif

    #ifdef DRV_COREAUDIO3
        if (@available(macOS 10.13, *)) {
            list.push_back("CoreAudio3");
        }
    #endif
    
    #ifdef DRV_COREAUDIO
        list.push_back("CoreAudio");
    #endif

    #ifdef DRV_OPENAL
		list.push_back("OpenAL");
	#endif

    #ifdef DRV_DSOUND
		list.push_back("DirectSound");
	#endif

    return list;
}

auto Audio::preferred() -> std::string {

    #ifdef DRV_XAUDIO29
        if (Win::getVersion() >= 0x0a00)
            return "XAudio 2.9";
    #endif

    #ifdef DRV_XAUDIO28
        if (Win::getVersion() >= 0x0602)
            return "XAudio 2.8";
    #endif

    #ifdef DRV_XAUDIO27
        if (Win::getVersion() >= 0x0501)
            return "XAudio 2.7";
    #endif

    #ifdef DRV_WASAPI
		return "Wasapi Shared";
	#endif

    #ifdef DRV_DSOUND
		return "DirectSound";
	#endif

	#ifdef DRV_PULSEAUDIO
		return "PulseAudio";
	#endif
    
    #ifdef DRV_COREAUDIO3
        if (@available(macOS 10.13, *)) {
            return "CoreAudio3";
        }
    #endif

    #ifdef DRV_COREAUDIO
        return "CoreAudio";
    #endif

	#ifdef DRV_OPENAL
		return "OpenAL";
	#endif

	return "";
}

auto Audio::create(const std::string& driver) -> Audio* {
	#ifdef DRV_DSOUND
        if(driver == "DirectSound") return new DAudio();
	#endif

    #ifdef DRV_XAUDIO27
        if(driver == "XAudio 2.7" && Win::getVersion() >= 0x0501 ) return new XAudio2(27u);
	#endif

    #ifdef DRV_XAUDIO28
        if(driver == "XAudio 2.8" && Win::getVersion() >= 0x0602 ) return new XAudio2(28u);
	#endif

    #ifdef DRV_XAUDIO29
        if(driver == "XAudio 2.9" && Win::getVersion() >= 0x0a00) return new XAudio2(29u);
	#endif

	#ifdef DRV_PULSEAUDIO
		if(driver == "PulseAudio") return new PulseAudio();
	#endif
    
    #ifdef DRV_COREAUDIO3
        if (@available(macOS 10.13, *)) {
            if(driver == "CoreAudio3") return new CoreAudio3();
        }
    #endif

    #ifdef DRV_COREAUDIO
        if(driver == "CoreAudio") return new CoreAudio();
    #endif

	#ifdef DRV_OPENAL
        if(driver == "OpenAL") return new OpenAL();
	#endif

	#ifdef DRV_WASAPI
        if(driver == "Wasapi Shared") return new Wasapi(false);
        if(driver == "Wasapi Exclusive") return new Wasapi( true );
	#endif

    return new Audio;
}

auto Input::available() -> std::vector<std::string> {
	return {
	#ifdef DRV_DINPUT5
		"DirectInput 5",
	#endif

	#ifdef DRV_DINPUT7
		"DirectInput 7",
	#endif

	#ifdef DRV_DINPUT8
		"DirectInput 8",
	#endif

	#ifdef DRV_RAWINPUT
		"RawInput",
	#endif

    #ifdef DRV_COCOA_IOKIT
        "Cocoa/IoKit",
    #endif
        
    #ifdef DRV_IOKIT
        "IoKit",
    #endif

	#if defined(DRV_XCB) && defined(DRV_UDEV)
		"XCB/Udev",
	#endif

	#if defined(DRV_XCB) && defined(DRV_UHID)
		"XCB/Uhid",
	#endif

    #if defined(DRV_XLIB) && defined(DRV_UDEV)
		"Xlib/Udev",
	#endif

	#if defined(DRV_XLIB) && defined(DRV_UHID)
		"Xlib/Uhid",
	#endif

	};
}

auto Input::preferred() -> std::string {
    #ifdef DRV_RAWINPUT
        return "RawInput";
    #endif

    #ifdef DRV_DINPUT8
		return "DirectInput 8";
	#endif

    #ifdef DRV_DINPUT7
		return "DirectInput 7";
	#endif

    #ifdef DRV_DINPUT5
		return "DirectInput 5";
	#endif

    #ifdef DRV_COCOA_IOKIT
        return "Cocoa/IoKit";
    #endif
    
    #ifdef DRV_IOKIT
        return "IoKit";
    #endif

	#if defined(DRV_XCB) && defined(DRV_UDEV)
		return "XCB/Udev";
	#endif

	#if defined(DRV_XCB) && defined(DRV_UHID)
		return "XCB/Uhid";
	#endif

	#if defined(DRV_XLIB) && defined(DRV_UDEV)
		return "Xlib/Udev";
	#endif

	#if defined(DRV_XLIB) && defined(DRV_UHID)
		return "Xlib/Uhid";
	#endif

	return "";
}

auto Input::create(const std::string& driver) -> Input* {
	#ifdef DRV_DINPUT5
        if(driver == "DirectInput 5") return new DInput(0x500);
    #endif

    #ifdef DRV_DINPUT7
        if(driver == "DirectInput 7") return new DInput(0x700);
    #endif

    #ifdef DRV_DINPUT8
        if(driver == "DirectInput 8") return new DInput(0x800);
	#endif

	#ifdef DRV_RAWINPUT
		if (driver == "RawInput") return new RawInput();
	#endif

    #ifdef DRV_COCOA_IOKIT
        if(driver == "Cocoa/IoKit") return new Iokit(true);
    #endif
    
	#ifdef DRV_IOKIT
        if(driver == "IoKit") return new Iokit();
	#endif

	#if defined(DRV_XCB) && defined(DRV_UDEV)
		if(driver == "XCB/Udev") return new XCBInput("udev");
	#endif

	#if defined(DRV_XCB) && defined(DRV_UHID)
		if(driver == "XCB/Uhid") return new XCBInput("uhid");
	#endif

	#if defined(DRV_XLIB) && defined(DRV_UDEV)
		if(driver == "Xlib/Udev") return new XInput("udev");
	#endif

	#if defined(DRV_XLIB) && defined(DRV_UHID)
		if(driver == "Xlib/Uhid") return new XInput("uhid");
	#endif

    return new Input;
}

}

