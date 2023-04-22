
# pgo := instrument
# pgo := optimize
# gprof := 1

DEBUG ?= 0
FileAssociations ?= 0
Arch ?=

name := Denise
translationFolder := translation
dataFolder := data
fontFolder := fonts
shaderFolder := shader
imgFolder := img
soundFolder := sounds

prefix ?= /usr
#prefix ?= $(HOME)/.local

include data/Makefile

objects := program view config emuconfig emumodel mediaview archiveviewer states firmware cmd statusbar
objects += input audio video palette shader bass reverb panning audiorecord wavwriter sinc cosine cosineSSE driveSounds
objects += guikit libAmi libC64 autoloader fileloader renderthread emuthread
objects += driver
ifeq ($(platform),windows)
    objects += dinput5 dinput7 dinput8 xaudio27 xaudio28 xaudio29
endif
objects += systemAmi agnusAmi inputAmi controlPortAmi keyboardAmi blitter copper denise paula diskDriveAmi diskStructureAmi sectorBlockAmi filesystemAmi rtcAmi
objects += m6510 ciaBase cia6526 ciaNew vicIIBase vicIICycle vicIIFast systemC64 sid chamberlin tapeC64 tapeStructureC64 inputC64 controlPortC64 acia
objects += cartC64 gameCartC64 freezerC64 reuC64 easyFlashC64 easyFlash3C64 retroReplayC64 gmod2C64 clipboardC64 geoRamC64 fastloaderC64
objects += m6502 via iec prg64 driveC64 diskStructureC64 firmwareC64 pia traps64 virtualDrive64 wd1770
objects += m93c86 mx29lv640eb icons logos fonts socket fpaq0

objects += m68000 m68000Core

deps = $(objects)

prgflags := -DAPP_NAME="\"$(name)\"" -DTRANSLATION_FOLDER="\"$(translationFolder)/\"" -DDATA_FOLDER="\"$(dataFolder)/\"" -DSHADER_FOLDER="\"$(shaderFolder)/\"" -DIMG_FOLDER="\"$(imgFolder)/\"" -DSOUND_FOLDER="\"$(soundFolder)/\""
flags :=
link := $(architecture)

ifeq ($(platform),windows)
    link += -static -lws2_32
else ifeq ($(platform),macosx)
    flags += -w -stdlib=libc++
    link += -lc++ -lobjc
    nativeArch := $(shell uname -a)
    ifneq ($(findstring arm64,$(nativeArch)),)
        export MACOSX_DEPLOYMENT_TARGET=11
    else
        export MACOSX_DEPLOYMENT_TARGET=10.9
    endif
else ifeq ($(platform),BSD)
    link += -static-libgcc -static-libstdc++ -lpthread -no-pie
else
    link += -lpthread -no-pie
endif

ifeq ($(gprof), 1)    
    flags += -O1 -g -pg -no-pie
    link += -pg

    ifeq ($(platform),windows)
	link += -mwindows
    endif

else ifeq ($(DEBUG), 0)
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
obj/resource.o: data/resource_mingw.rc
	windres data/resource_mingw.rc obj/resource.o
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
obj/renderthread.o: driver/video/thread/renderThread.cpp
obj/emuthread.o:		program/thread/emuThread.cpp

obj/libAmi.o:	emulation/libami/interface.cpp
obj/libC64.o:	emulation/libc64/interface.cpp
obj/m6510.o:	emulation/libc64/m6510/m6510.cpp
obj/m6502.o:	emulation/libc64/disk/cpu/m6502.cpp

obj/ciaBase.o:	emulation/cia/base.cpp	
obj/cia6526.o:	emulation/cia/m6526.cpp
obj/ciaNew.o:	emulation/cia/new/cia.cpp

obj/m68000Core.o:	emulation/libami/cpu/m68000/m68000.cpp
obj/m68000.o:	emulation/libami/cpu/m68000.cpp
obj/agnusAmi.o:	emulation/libami/agnus/agnus.cpp
obj/blitter.o:	emulation/libami/agnus/blitter.cpp
obj/copper.o:	emulation/libami/agnus/copper.cpp
obj/denise.o:	emulation/libami/video/denise.cpp
obj/paula.o:	emulation/libami/paula/paula.cpp
obj/systemAmi.o: emulation/libami/system/system.cpp
obj/inputAmi.o:	emulation/libami/input/input.cpp
obj/controlPortAmi.o: emulation/libami/input/controlPort/controlPort.cpp
obj/keyboardAmi.o: emulation/libami/input/keyboard/keyboard.cpp
obj/diskDriveAmi.o: emulation/libami/drive/diskDrive.cpp
obj/diskStructureAmi.o: emulation/libami/drive/diskStructure.cpp
obj/sectorBlockAmi.o: emulation/libami/drive/sectorBlock.cpp
obj/filesystemAmi.o: emulation/libami/drive/filesystem.cpp
obj/rtcAmi.o: emulation/libami/system/rtc.cpp

obj/vicIIBase.o:emulation/libc64/vicII/base.cpp
obj/vicIICycle.o:emulation/libc64/vicII/vicII.cpp
obj/vicIIFast.o:emulation/libc64/vicII/fast/vicIIFast.cpp
obj/systemC64.o:emulation/libc64/system/system.cpp
obj/firmwareC64.o:emulation/libc64/system/firmware.cpp
obj/cartC64.o:	emulation/libc64/expansionPort/cart/cart.cpp
obj/gameCartC64.o: emulation/libc64/expansionPort/gameCart/gameCart.cpp
obj/freezerC64.o: emulation/libc64/expansionPort/freezer/freezer.cpp
obj/reuC64.o:	emulation/libc64/expansionPort/reu/reu.cpp
obj/geoRamC64.o:emulation/libc64/expansionPort/geoRam/geoRam.cpp
obj/easyFlashC64.o: emulation/libc64/expansionPort/easyFlash/easyFlash.cpp
obj/easyFlash3C64.o: emulation/libc64/expansionPort/easyFlash/easyFlash3.cpp
obj/retroReplayC64.o: emulation/libc64/expansionPort/retroReplay/retroReplay.cpp
obj/gmod2C64.o: emulation/libc64/expansionPort/gmod/gmod2.cpp
obj/fastloaderC64.o: emulation/libc64/expansionPort/fastloader/fastloader.cpp
obj/clipboardC64.o: emulation/libc64/system/clipboard.cpp
obj/sid.o: emulation/libc64/sid/sid.cpp
obj/chamberlin.o: emulation/libc64/sid/filter/chamberlin.cpp
	$(compiler) $(cppflags) $(flags) -ffast-math -fno-exceptions  $1 -c $< -o $@
obj/tapeC64.o:	emulation/libc64/tape/tape.cpp
obj/tapeStructureC64.o: emulation/libc64/tape/structure.cpp
obj/prg64.o:	emulation/libc64/prg/prg.cpp
obj/inputC64.o:	emulation/libc64/input/input.cpp
obj/controlPortC64.o: emulation/libc64/input/controlPort/controlPort.cpp
obj/acia.o: emulation/libc64/expansionPort/acia/acia.cpp
obj/wd1770.o: emulation/libc64/disk/wd177x/wd1770.cpp

obj/traps64.o: emulation/libc64/traps/traps.cpp
obj/virtualDrive64.o: emulation/libc64/disk/virtual/virtualDrive.cpp
obj/via.o:	emulation/libc64/disk/via/via.cpp
obj/pia.o:	emulation/tools/pia.cpp
obj/iec.o:	emulation/libc64/disk/iec.cpp
obj/driveC64.o:emulation/libc64/disk/drive/drive.cpp
obj/diskStructureC64.o:emulation/libc64/disk/structure/structure.cpp
	$(compiler) $(cppflags) $(flags) -Wno-stringop-overflow $1 -c $< -o $@

obj/m93c86.o:emulation/tools/m93c86.cpp
obj/mx29lv640eb.o:emulation/tools/mx29lv640eb.cpp
obj/icons.o:data/icons/icons.cpp
obj/logos.o:data/icons/logos.cpp
obj/fonts.o:data/fonts/fonts.cpp
obj/socket.o:emulation/tools/socket.cpp
obj/fpaq0.o:emulation/tools/fpaq0.cpp

obj/program.o:		program/program.cpp
	$(compiler) $(cppflags) $(prgflags) $(flags) $1 -c $< -o $@
obj/input.o:		program/input/manager.cpp
obj/view.o:		program/view/view.cpp
obj/statusbar.o:	program/view/status.cpp
obj/config.o:		program/config/config.cpp
obj/emuconfig.o:	program/emuconfig/config.cpp
obj/emumodel.o:		program/emuconfig/layouts/model.cpp
obj/mediaview.o:	program/media/media.cpp
obj/autoloader.o:	program/media/autoloader.cpp
obj/fileloader.o:	program/media/fileloader.cpp
obj/archiveviewer.o:	program/config/archiveViewer.cpp
obj/states.o:		program/states/states.cpp
obj/audio.o:		program/audio/manager.cpp
obj/bass.o:		program/audio/dsp/bass.cpp
obj/reverb.o:		program/audio/dsp/reverb.cpp
obj/panning.o:		program/audio/dsp/panning.cpp
obj/cosine.o:		program/audio/resampler/cosine.cpp
obj/cosineSSE.o:	program/audio/resampler/cosineSSE.cpp
#obj/cosinePrecise.o:program/audio/resampler/cosinePrecise.cpp
obj/sinc.o:		program/audio/resampler/sinc.cpp
#obj/linearResample.o: program/audio/resampler/linear.cpp
#obj/cubicResample.o: program/audio/resampler/cubic.cpp
#obj/hermiteResample.o: program/audio/resampler/hermite.cpp
obj/audiorecord.o:	program/audio/record/handler.cpp
obj/wavwriter.o:	program/audio/record/wavWriter.cpp
obj/driveSounds.o:	program/audio/mixer/drive.cpp
obj/firmware.o:		program/firmware/manager.cpp
obj/cmd.o:		program/cmd/cmd.cpp
obj/palette.o:		program/video/palette.cpp
obj/video.o:		program/video/manager.cpp
obj/shader.o:		program/video/shader.cpp

deps := $(patsubst %,obj/%.d,$(deps))
objects := $(patsubst %,obj/%.o,$(objects))
-include $(wildcard $(deps))
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
	mkdir out/$(name).app/Contents/Resources/$(soundFolder)

	cp data/Info.plist out/$(name).app/Contents/Info.plist
	cp data/$(translationFolder)/* out/$(name).app/Contents/Resources/$(translationFolder)/
	cp data/$(dataFolder)/* out/$(name).app/Contents/Resources/$(dataFolder)/
	cp data/$(fontFolder)/*.ttf out/$(name).app/Contents/Resources/$(fontFolder)/
	cp data/$(imgFolder)/bundle/* out/$(name).app/Contents/Resources/$(imgFolder)/
	cp -r data/$(soundFolder)/* out/$(name).app/Contents/Resources/$(soundFolder)/
	cp -r data/$(shaderFolder)/* out/$(name).app/Contents/Resources/$(shaderFolder)/
	cp -r data/txt/licence.md out/$(name).app/Contents/Resources/
	cp -r readme.md out/$(name).app/Contents/Resources/
	
	cp data/img/$(loname).icns out/$(name).app/Contents/Resources/$(name).icns

	find obj -iname "*.d" -type f -exec sed -i '' '1 s/$$(wildcard //;1 s/.o:/.o: $$\(wildcard/;$$ s/)//;$$ s/$$/\)/' {} \;

	$(strip $(compiler) -o out/$(name).app/Contents/MacOS/$(name) $(objects) $(link))
	
    ifneq ($(findstring freetype,$(drv)),)
	install -m 755 /usr/local/lib/libfreetype.6.dylib out/$(name).app/Contents/Frameworks/
	install_name_tool -id @executable_path/../Frameworks/libfreetype.6.dylib out/$(name).app/Contents/Frameworks/libfreetype.6.dylib
	install_name_tool -change `otool -D /usr/local/lib/libfreetype.6.dylib | cut -d':' -f2` @executable_path/../Frameworks/libfreetype.6.dylib out/$(name).app/Contents/MacOS/$(name)
    endif
	codesign --force --deep -s - out/$(name).app
else ifeq ($(platform),windows)
	$(strip $(compiler) -o out/$(name) $(objects) $(link))
else ifeq ($(platform),BSD)
	@sed -i '' '1 s/$$(wildcard //g;1 s/.o:/.o: $$\(wildcard/g;$$ s/)//g;$$ s/$$/\)/g' obj/*.d

	$(strip $(compiler) -o out/$(loname) $(objects) $(link))
else
	@sed -i '1 s/$$(wildcard //g;1 s/.o:/.o: $$\(wildcard/g;$$ s/)//g;$$ s/$$/\)/g' obj/*.d

	$(strip $(compiler) -o out/$(loname) $(objects) $(link))
endif

.PHONY: help
help:
	@echo Options: DEBUG=[0\|1] FileAssociations=[0\|1] Arch=[arm64-apple-macos11\|...]
	@echo Targets:
	@grep -E '^[a-zA-Z0-9_-]+:.*?## .*$$' $(MAKEFILE_LIST) \
	| sed -n 's/^\(.*\): \(.*\)##\(.*\)/\1\3/p' \
	| column -t  -s ' '

clean: ## Clean
	-@$(call delete,obj/*.o)
	-@$(call delete,obj/*.d)
ifneq ($(platform),x)
		-@$(call delete,out/$(name)*)
else
		-@$(call delete,out/$(name)*)
		-@$(call delete,out/$(loname)*)
endif

install: ## Install
    ifeq ($(platform),windows)
	$(call copy,data/$(translationFolder),out/$(translationFolder))	
	$(call copy,data/$(dataFolder),out/$(dataFolder))
	$(call copy,data/$(imgFolder)/bundle,out/$(imgFolder))
	$(call copy,data/$(soundFolder),out/$(soundFolder), /S)
	$(call copy,data/$(shaderFolder),out/$(shaderFolder), /S)
	$(call copy,readme.md,out)
	$(call copy,data/txt/licence.md,out)

    ifneq ($(findstring i686, $(shell g++ --version) ),)
	$(call copy,"data/libs/shared/win32/D3D*.dll",out)
    else
	$(call copy,"data/libs/shared/win64/D3D*.dll",out)
    endif

    else ifeq ($(platform),macosx)
	dmgbuild -s data/dmgSettings.py "Denise" out/Denise.dmg
    else

	if [ -d $(prefix)/local ]; then	mkdir -p $(prefix)/local/bin/; else mkdir -p $(prefix)/bin/; fi
	mkdir -p $(prefix)/share/icons/
	mkdir -p $(prefix)/share/applications/
	mkdir -p $(prefix)/share/mime/packages/
	mkdir -p $(prefix)/share/$(loname)/$(translationFolder)/
	mkdir -p $(prefix)/share/$(loname)/$(dataFolder)/
	mkdir -p $(prefix)/share/$(loname)/$(fontFolder)/
	mkdir -p $(prefix)/share/$(loname)/$(imgFolder)/
	mkdir -p $(prefix)/share/$(loname)/$(soundFolder)/
	mkdir -p $(prefix)/share/$(loname)/$(shaderFolder)/

	if [ -d $(prefix)/local ]; then	\
	    install -m 755 out/$(loname) $(prefix)/local/bin/$(loname);	\
	else	\
	    install -m 755 out/$(loname) $(prefix)/bin/$(loname);	\
	fi
	install -m 644 data/img/$(loname).png $(prefix)/share/icons/$(loname).png
	install -m 644 data/$(loname).desktop $(prefix)/share/applications/$(loname).desktop

    ifeq ($(FileAssociations), 1)
	install -m 644 data/application-x-$(loname).xml $(prefix)/share/mime/packages/application-x-$(loname).xml
	if [ $(shell which update-mime-database) ]; then update-mime-database $(prefix)/share/mime; fi;
	if [ $(shell which update-desktop-database) ]; then update-desktop-database $(prefix)/share/applications; fi;
    endif
	install -m 644 data/$(translationFolder)/* $(prefix)/share/$(loname)/$(translationFolder)
	install -m 644 data/$(dataFolder)/* $(prefix)/share/$(loname)/$(dataFolder)
	install -m 644 data/$(fontFolder)/*.ttf $(prefix)/share/$(loname)/$(fontFolder)
	install -m 644 data/$(imgFolder)/bundle/* $(prefix)/share/$(loname)/$(imgFolder)
	cp -r data/$(soundFolder)/* $(prefix)/share/$(loname)/$(soundFolder)/
	cp -r data/$(shaderFolder)/* $(prefix)/share/$(loname)/$(shaderFolder)/
    endif

uninstall: ## Unistall
    ifeq ($(platform),windows)
    else ifeq ($(platform),macosx)
    else	
	if [ -f $(prefix)/local/bin/$(loname) ];	then rm $(prefix)/local/bin/$(loname);	\
	elif [ -f $(prefix)/bin/$(loname) ];	then rm $(prefix)/bin/$(loname); fi
	if [ -f $(prefix)/local/bin/$(name) ];	then rm $(prefix)/local/bin/$(name);	\
	elif [ -f $(prefix)/bin/$(name) ];	then rm $(prefix)/bin/$(name); fi
	
	if [ -f $(prefix)/share/icons/$(loname).png ]; then rm $(prefix)/share/icons/$(loname).png; fi
	if [ -f $(prefix)/share/applications/$(loname).desktop ]; then rm $(prefix)/share/applications/$(loname).desktop; fi
	if [ -f $(prefix)/share/mime/packages/application-x-$(loname).xml ]; then \
		rm $(prefix)/share/mime/packages/application-x-$(loname).xml; \
	    if [ $(shell which update-mime-database) ]; then update-mime-database $(prefix)/share/mime; fi; \
	    if [ $(shell which update-desktop-database) ]; then update-desktop-database $(prefix)/share/applications; fi; \
	fi
	if [ -d $(prefix)/share/$(loname) ]; then rm -rf $(prefix)/share/$(loname); fi
    endif
