
# pgo := instrument
# pgo := optimize
# gprof := 1

DEBUG ?= 0
name := Denise
translationFolder := translation
dataFolder := data
fontFolder := fonts
shaderFolder := shader
imgFolder := img
prefix := $(HOME)/.local
target := $(shell g++ --version | grep i686)

include data/Makefile

objects := program view config emuconfig mediaview archiveviewer states firmware cmd
objects += input audio video palette shader
objects += guikit libami libC64
objects += driver
ifeq ($(platform),windows)
    objects += dinput5 dinput7 dinput8 xaudio27 xaudio28 xaudio29
endif
#objects += m68000
objects += m6502 m6510 ciaBase cia6526 vicIIBase vicIICycle vicIIFast systemC64 sid tapeC64 inputC64 controlPortC64
objects += cartC64 gameCartC64 actionReplayC64 reuC64 easyFlashC64 retroReplayC64
objects += via iec prg64 drive1541 m6502custom structure1541

prgflags := -DAPP_NAME="\"$(name)\"" -DTRANSLATION_FOLDER="\"$(translationFolder)/\"" -DDATA_FOLDER="\"$(dataFolder)/\"" -DSHADER_FOLDER="\"$(shaderFolder)/\"" -DIMG_FOLDER="\"$(imgFolder)/\""
flags :=
link :=

ifeq ($(platform),windows)
    link += -static
else ifeq ($(platform),macosx)
    flags += -w -stdlib=libc++
    link += -lc++ -lobjc
else
    link += -lpthread -no-pie
endif

ifeq ($(DEBUG), 0)
    flags += -O3
    link += -s

    ifeq ($(platform),windows)
	link += -mwindows
    endif
else
    flags += -O0 -g

    ifeq ($(platform),windows)
	link += -mconsole
    endif
endif

ifeq ($(gprof), 1)
    link += -pg
    flags += -pg -no-pie
endif

ifeq ($(pgo),instrument)
    flags += -fprofile-generate
    link += -lgcov
else ifeq ($(pgo),optimize)
    flags += -fprofile-use -fprofile-correction
endif

include guikit/Makefile
link += $(uilink)

include driver/Makefile
link += $(drvlink)

all: build;

ifeq ($(platform),windows)
objects += resource
obj/resource.o: data/resource.rc
	windres data/resource.rc obj/resource.o
endif

%.o: $<; $(call compile)

obj/guikit.o: guikit/api.cpp
	$(compiler) $(uiflags) $(flags) -c $< -o $@

obj/driver.o: driver/driver.cpp
	$(compiler) $(drvflags) $(flags) -c $< -o $@

ifeq ($(platform),windows)
obj/dinput5.o:	driver/input/dinput/v5.cpp
	$(compiler) $(drvflags) $(flags) -c $< -o $@
obj/dinput7.o:	driver/input/dinput/v7.cpp
	$(compiler) $(drvflags) $(flags) -c $< -o $@
obj/dinput8.o:	driver/input/dinput/v8.cpp
	$(compiler) $(drvflags) $(flags) -c $< -o $@
	
obj/xaudio27.o:	driver/audio/xaudio2/xaudio27.cpp
	$(compiler) $(drvflags) $(flags) -c $< -o $@	
obj/xaudio28.o:	driver/audio/xaudio2/xaudio28.cpp
	$(compiler) $(drvflags) $(flags) -Wno-attributes -c $< -o $@
obj/xaudio29.o:	driver/audio/xaudio2/xaudio29.cpp
	$(compiler) $(drvflags) $(flags) -Wno-attributes -c $< -o $@
endif	

obj/libami.o:	emulation/libami/interface.cpp
obj/libC64.o:	emulation/libc64/interface.cpp
#obj/m68000.o:	libami/cpu/m680x0/m68000.cpp
obj/m6502.o:	emulation/processor/m65xx/m6502/m6502.cpp
obj/m6510.o:	emulation/processor/m65xx/m6510/m6510.cpp
obj/ciaBase.o:	emulation/cia/base.cpp	
obj/cia6526.o:	emulation/cia/m6526.cpp
obj/vicIIBase.o:emulation/libc64/vic/base.cpp
obj/vicIICycle.o:emulation/libc64/vic/vicII.cpp
obj/vicIIFast.o:emulation/libc64/vic/fast/vicIIFast.cpp
obj/systemC64.o:emulation/libc64/system/system.cpp	
obj/cartC64.o:	emulation/libc64/expansionPort/cart/cart.cpp
obj/gameCartC64.o: emulation/libc64/expansionPort/gameCart/gameCart.cpp
obj/actionReplayC64.o: emulation/libc64/expansionPort/actionReplay/actionReplay.cpp
obj/reuC64.o:	emulation/libc64/expansionPort/reu/reu.cpp
obj/easyFlashC64.o: emulation/libc64/expansionPort/easyFlash/easyFlash.cpp
obj/retroReplayC64.o: emulation/libc64/expansionPort/retroReplay/retroReplay.cpp
obj/sid.o:	emulation/libc64/sid/sid.cpp
obj/tapeC64.o:	emulation/libc64/tape/tape.cpp
obj/prg64.o:	emulation/libc64/prg/prg.cpp
obj/inputC64.o:	emulation/libc64/input/input.cpp
obj/controlPortC64.o: emulation/libc64/input/controlPort/controlPort.cpp

obj/via.o:	emulation/libc64/disk/via/via.cpp	
obj/iec.o:	emulation/libc64/disk/iec.cpp
obj/drive1541.o:emulation/libc64/disk/drive/drive1541.cpp
obj/m6502custom.o:emulation/libc64/disk/cpu/m6502custom.cpp
obj/structure1541.o:emulation/libc64/disk/structure/structure.cpp

obj/program.o:		program/program.cpp
	$(compiler) $(cppflags) $(prgflags) $(flags) $1 -c $< -o $@
obj/input.o:		program/input/manager.cpp
obj/view.o:		program/view/view.cpp
obj/config.o:		program/config/config.cpp
obj/emuconfig.o:	program/emuconfig/config.cpp
obj/mediaview.o:	program/media/media.cpp
obj/archiveviewer.o:	program/config/archiveViewer.cpp
obj/states.o:		program/states/states.cpp
obj/audio.o:		program/audio/manager.cpp
obj/firmware.o:		program/firmware/manager.cpp
obj/cmd.o:		program/cmd/cmd.cpp
obj/palette.o:		program/video/palette.cpp
obj/video.o:		program/video/manager.cpp
obj/shader.o:		program/video/shader.cpp

objects := $(patsubst %,obj/%.o,$(objects))
loname := $(call strlower,$(name))

build: $(objects)
    ifeq ($(platform),macosx)
	if [ -d out/$(name).app ]; then rm -r out/$(name).app; fi
	mkdir out/$(name).app
	mkdir out/$(name).app/Contents
	mkdir out/$(name).app/Contents/Frameworks
	mkdir out/$(name).app/Contents/MacOS
	mkdir out/$(name).app/Contents/Resources
	mkdir out/$(name).app/Contents/Resources/$(translationFolder)
	mkdir out/$(name).app/Contents/Resources/$(dataFolder)
	mkdir out/$(name).app/Contents/Resources/$(fontFolder)
	mkdir out/$(name).app/Contents/Resources/$(shaderFolder)
	mkdir out/$(name).app/Contents/Resources/$(imgFolder)

	cp data/Info.plist out/$(name).app/Contents/Info.plist
	cp data/$(translationFolder)/* out/$(name).app/Contents/Resources/$(translationFolder)/
	cp data/$(dataFolder)/* out/$(name).app/Contents/Resources/$(dataFolder)/
	cp data/$(fontFolder)/*.ttf out/$(name).app/Contents/Resources/$(fontFolder)/
	cp data/$(imgFolder)/bundle/* out/$(name).app/Contents/Resources/$(imgFolder)/
	cp -r data/shader/* out/$(name).app/Contents/Resources/$(shaderFolder)/
	
	cp data/img/$(loname).icns out/$(name).app/Contents/Resources/$(name).icns

	$(strip $(compiler) -o out/$(name).app/Contents/MacOS/$(name) $(objects) $(link))
	
    ifneq ($(findstring freetype,$(drv)),)
	install -m 755 /usr/local/lib/libfreetype.6.dylib out/$(name).app/Contents/Frameworks/
	install_name_tool -id @executable_path/../Frameworks/libfreetype.6.dylib out/$(name).app/Contents/Frameworks/libfreetype.6.dylib
	install_name_tool -change `otool -D /usr/local/lib/libfreetype.6.dylib | cut -d':' -f2` @executable_path/../Frameworks/libfreetype.6.dylib out/$(name).app/Contents/MacOS/$(name)
    endif
	
    ifneq ($(findstring sdlinput,$(drv)),)
	install -m 755 /usr/local/lib/libSDL2-2.0.0.dylib out/$(name).app/Contents/Frameworks/
	install_name_tool -id @executable_path/../Frameworks/libSDL2-2.0.0.dylib out/$(name).app/Contents/Frameworks/libSDL2-2.0.0.dylib	
	install_name_tool -change `otool -D /usr/local/lib/libSDL2-2.0.0.dylib | cut -d':' -f2` @executable_path/../Frameworks/libSDL2-2.0.0.dylib out/$(name).app/Contents/MacOS/$(name)
    endif
	
    else
	$(strip $(compiler) -o out/$(name) $(objects) $(link))
    endif

clean:
	-@$(call delete,obj/*.o)
	-@$(call delete,out/$(name)*)

install:
    ifeq ($(platform),windows)
	$(call copy,data/$(translationFolder),out/$(translationFolder))	
	$(call copy,data/$(dataFolder),out/$(dataFolder))
	$(call copy,data/$(imgFolder)/bundle,out/$(imgFolder))
	$(call copy,data/shader,out/$(shaderFolder), /S)
	$(call copy,readme.md,out)

    ifneq ($(findstring i686,$(target)),)
	$(call copy,"data/libs/shared/win32/*.dll",out)
    else
	$(call copy,"data/libs/shared/win64/*.dll",out)
    endif

    else ifeq ($(platform),macosx)
	dmgbuild -s data/dmgSettings.py "Denise" out/Denise.dmg
    else
	mkdir -p $(prefix)/bin/
	mkdir -p $(prefix)/share/icons/
	mkdir -p $(prefix)/share/applications/
	mkdir -p $(prefix)/$(loname)/$(translationFolder)/
	mkdir -p $(prefix)/$(loname)/$(dataFolder)/
	mkdir -p $(prefix)/$(loname)/$(fontFolder)/
	mkdir -p $(prefix)/$(loname)/$(imgFolder)/
	mkdir -p $(prefix)/$(loname)/$(shaderFolder)/

	install -D -m 755 out/$(name) $(prefix)/bin/$(name)
	install -D -m 644 data/img/$(loname).png $(prefix)/share/icons/$(loname).png
	install -D -m 644 data/$(loname).desktop $(prefix)/share/applications/$(loname).desktop
	install -D -m 644 data/$(translationFolder)/* $(prefix)/$(loname)/$(translationFolder)
	install -D -m 644 data/$(dataFolder)/* $(prefix)/$(loname)/$(dataFolder)
	install -D -m 644 data/$(fontFolder)/*.ttf $(prefix)/$(loname)/$(fontFolder)
	install -D -m 644 data/$(imgFolder)/bundle/* $(prefix)/$(loname)/$(imgFolder)
	cp -r data/$(shaderFolder)/* $(prefix)/$(loname)/$(shaderFolder)/
    endif

uninstall:
    ifeq ($(platform),windows)
    else ifeq ($(platform),macosx)
    else	
	if [ -f $(prefix)/bin/$(name) ]; then rm $(prefix)/bin/$(name); fi
	if [ -f $(prefix)/share/icons/$(loname).png ]; then rm $(prefix)/share/icons/$(loname).png; fi
	if [ -f $(prefix)/share/applications/$(loname).desktop ]; then rm $(prefix)/share/applications/$(loname).desktop; fi
	if [ -d $(prefix)/$(loname) ]; then rm -rf $(prefix)/$(loname); fi
    endif

