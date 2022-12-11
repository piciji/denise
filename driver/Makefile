
# remove drivers you don't need or have problems on older OS like Win XP
# it's possible to remove all drivers
# sdlinput covers joypad support only and is not supported for windows and macOS anymore.

# some drivers like dinput (5, 7, 8) and xaudio (2.7, 2.8, 2.9) are supported in multiple versions.

ifeq ($(platform),windows)
    drv += wgl direct3d dsound dinput5 dinput7 dinput8 xaudio27 xaudio28 xaudio29 wasapi rawinput

else ifeq ($(platform),macosx)
    drv += cgl openal coreaudio iokit

else ifeq ($(platform),NetBSD)
	drv += glx openal pulseaudio xlib sdlinput

else
    drv += glx openal pulseaudio xlib udev sdlinput

endif
# show in screen status text for opengl
ifeq ($(Arch),)
drv += freetype
endif
env :=


ifeq ($(platform),windows)
    drvflags = $(cppflags)
    drvlink += $(if $(findstring openal,$(drv)),-lopenal32)

else ifeq ($(platform),macosx)
    env += /usr/local/bin/
    drvflags = $(objcppflags)
    drvlink = -framework Cocoa
    drvlink += $(if $(findstring openal,$(drv)),-framework OpenAL)
    
else
    drvflags = $(cppflags) 
    drvflags += $(shell pkg-config --cflags gtk+-3.0)
    ifeq ($(platform),NetBSD)
    drvlink = -Wl,-R,/usr/X11R7/lib -lX11 -lXext -lXfixes
    else
    drvlink = -lX11 -lXext -ldl -lXfixes
    endif
    drvlink += $(if $(findstring openal,$(drv)),-lopenal)
    ifneq ($(findstring xlib,$(drv)),)
	drvlink += $(if $(findstring sdl,$(drv)), $(shell $(env)sdl2-config --libs))
    endif
endif

ifneq ($(findstring freetype,$(drv)),)
    ifneq ($(findstring gl,$(drv)),)
	ifeq ($(platform),windows)
	    drvlink += -lfreetype
	else
		drvflags += $(shell $(env)pkg-config --cflags freetype2)
	    drvlink += $(shell $(env)pkg-config --static --libs freetype2)
	endif
    endif
endif

drvflags += $(foreach c,$(call strupper,$(drv)),-DDRV_$c)
drvflags += $(if $(findstring sdl,$(drv)),$(shell $(env)sdl2-config --cflags))

drvlink += $(if $(findstring direct3d,$(drv)),-ld3d9 -ld3dx9)
drvlink += $(if $(findstring dsound,$(drv)),-ldsound)
drvlink += $(if $(findstring dinput,$(drv)),-ldinput -ldxguid)
drvlink += $(if $(findstring wgl,$(drv)),-lopengl32)
drvlink += $(if $(findstring wasapi,$(drv)),-lksuser -luuid -lavrt)
# osx carbon UI is depracated for 64 bit. we need it for TIS services only
drvlink += $(if $(findstring iokit,$(drv)),-framework IOKit -framework Carbon)
drvlink += $(if $(findstring cgl,$(drv)),-framework OpenGL)
drvlink += $(if $(findstring coreaudio,$(drv)),-framework CoreAudio -framework AudioToolbox -framework AudioUnit)

drvlink += $(if $(findstring glx,$(drv)),-lGL)
drvlink += $(if $(findstring udev,$(drv)),-ludev)
drvlink += $(if $(findstring pulseaudio,$(drv)),-lpulse)
