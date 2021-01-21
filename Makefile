
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

prefix ?= /usr
#prefix ?= $(HOME)/.local

# temporary: to uninstall previous versions of Denise, will be removed in future releases
prefixOld := $(HOME)/.local

include data/Makefile

objects := program view config emuconfig emumodel mediaview archiveviewer states firmware cmd statusbar
objects += input audio video palette shader bass reverb panning audiorecord wavwriter cosine cosineSSE
objects += guikit libami libC64 autoloader
objects += driver
ifeq ($(platform),windows)
    objects += dinput5 dinput7 dinput8 xaudio27 xaudio28 xaudio29
endif
#objects += m68000
objects += m6510 ciaBase cia6526 vicIIBase vicIICycle vicIIFast systemC64 sid chamberlin tapeC64 inputC64 controlPortC64
objects += cartC64 gameCartC64 actionReplayC64 reuC64 easyFlashC64 retroReplayC64 clipboardC64
objects += m6502 via iec prg64 drive1541 structure1541
objects += thread icons

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
obj/m6510.o:	emulation/libc64/m6510/m6510.cpp
obj/m6502.o:	emulation/libc64/disk/cpu/m6502.cpp

obj/ciaBase.o:	emulation/cia/base.cpp	
obj/cia6526.o:	emulation/cia/m6526.cpp
obj/vicIIBase.o:emulation/libc64/vicII/base.cpp
obj/vicIICycle.o:emulation/libc64/vicII/vicII.cpp
obj/vicIIFast.o:emulation/libc64/vicII/fast/vicIIFast.cpp
obj/systemC64.o:emulation/libc64/system/system.cpp	
obj/cartC64.o:	emulation/libc64/expansionPort/cart/cart.cpp
obj/gameCartC64.o: emulation/libc64/expansionPort/gameCart/gameCart.cpp
obj/actionReplayC64.o: emulation/libc64/expansionPort/actionReplay/actionReplay.cpp
obj/reuC64.o:	emulation/libc64/expansionPort/reu/reu.cpp
obj/easyFlashC64.o: emulation/libc64/expansionPort/easyFlash/easyFlash.cpp
obj/retroReplayC64.o: emulation/libc64/expansionPort/retroReplay/retroReplay.cpp
obj/clipboardC64.o: emulation/libc64/system/clipboard.cpp
obj/sid.o: emulation/libc64/sid/sid.cpp
obj/chamberlin.o: emulation/libc64/sid/filter/chamberlin.cpp
	$(compiler) $(cppflags) $(flags) -ffast-math -fno-exceptions  $1 -c $< -o $@
obj/tapeC64.o:	emulation/libc64/tape/tape.cpp
obj/prg64.o:	emulation/libc64/prg/prg.cpp
obj/inputC64.o:	emulation/libc64/input/input.cpp
obj/controlPortC64.o: emulation/libc64/input/controlPort/controlPort.cpp

obj/via.o:	emulation/libc64/disk/via/via.cpp	
obj/iec.o:	emulation/libc64/disk/iec.cpp
obj/drive1541.o:emulation/libc64/disk/drive/drive1541.cpp
obj/structure1541.o:emulation/libc64/disk/structure/structure.cpp
	$(compiler) $(cppflags) $(flags) -Wno-stringop-overflow $1 -c $< -o $@
obj/thread.o:emulation/tools/thread.cpp
obj/icons.o:data/icons.cpp

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
obj/archiveviewer.o:	program/config/archiveViewer.cpp
obj/states.o:		program/states/states.cpp
obj/audio.o:		program/audio/manager.cpp
obj/bass.o:		program/audio/dsp/bass.cpp
obj/reverb.o:		program/audio/dsp/reverb.cpp
obj/panning.o:		program/audio/dsp/panning.cpp
obj/cosine.o:		program/audio/resampler/cosine.cpp
obj/cosineSSE.o:	program/audio/resampler/cosineSSE.cpp
obj/audiorecord.o:	program/audio/record/handler.cpp
obj/wavwriter.o:	program/audio/record/wavWriter.cpp
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

    ifneq ($(findstring i686, $(shell g++ --version) ),)
	$(call copy,"data/libs/shared/win32/*.dll",out)
    else
	$(call copy,"data/libs/shared/win64/*.dll",out)
    endif

    else ifeq ($(platform),macosx)
	dmgbuild -s data/dmgSettings.py "Denise" out/Denise.dmg
    else
	# remove possible old installation
	if [ -f $(prefixOld)/bin/$(name) ]; then rm $(prefixOld)/bin/$(name); fi
	if [ -f $(prefixOld)/share/icons/$(loname).png ]; then rm $(prefixOld)/share/icons/$(loname).png; fi
	if [ -f $(prefixOld)/share/applications/$(loname).desktop ]; then rm $(prefixOld)/share/applications/$(loname).desktop; fi
	if [ -d $(prefixOld)/$(loname) ]; then rm -rf $(prefixOld)/$(loname); fi
	
	if [ -d $(prefix)/local ]; then	mkdir -p $(prefix)/local/bin/; else mkdir -p $(prefix)/bin/; fi
	mkdir -p $(prefix)/share/icons/
	mkdir -p $(prefix)/share/applications/
	mkdir -p $(prefix)/share/mime/packages/
	mkdir -p $(prefix)/share/$(loname)/$(translationFolder)/
	mkdir -p $(prefix)/share/$(loname)/$(dataFolder)/
	mkdir -p $(prefix)/share/$(loname)/$(fontFolder)/
	mkdir -p $(prefix)/share/$(loname)/$(imgFolder)/
	mkdir -p $(prefix)/share/$(loname)/$(shaderFolder)/

	if [ -d $(prefix)/local ]; then	\
	    install -D -m 755 out/$(name) $(prefix)/local/bin/$(name);	\
	else	\
	    install -D -m 755 out/$(name) $(prefix)/bin/$(name);	\
	fi
	install -D -m 644 data/img/$(loname).png $(prefix)/share/icons/$(loname).png
	install -D -m 644 data/$(loname).desktop $(prefix)/share/applications/$(loname).desktop
	install -D -m 644 data/application-x-$(loname).xml $(prefix)/share/mime/packages/application-x-$(loname).xml
	install -D -m 644 data/$(translationFolder)/* $(prefix)/share/$(loname)/$(translationFolder)
	install -D -m 644 data/$(dataFolder)/* $(prefix)/share/$(loname)/$(dataFolder)
	install -D -m 644 data/$(fontFolder)/*.ttf $(prefix)/share/$(loname)/$(fontFolder)
	install -D -m 644 data/$(imgFolder)/bundle/* $(prefix)/share/$(loname)/$(imgFolder)
	cp -r data/$(shaderFolder)/* $(prefix)/share/$(loname)/$(shaderFolder)/
    endif

uninstall:
    ifeq ($(platform),windows)
    else ifeq ($(platform),macosx)
    else	
	if [ -f $(prefix)/local/bin/$(name) ];	then rm $(prefix)/local/bin/$(name);	\
	elif [ -f $(prefix)/bin/$(name) ];	then rm $(prefix)/bin/$(name); fi
	
	if [ -f $(prefix)/share/icons/$(loname).png ]; then rm $(prefix)/share/icons/$(loname).png; fi
	if [ -f $(prefix)/share/applications/$(loname).desktop ]; then rm $(prefix)/share/applications/$(loname).desktop; fi
	if [ -f $(prefix)/share/mime/packages/application-x-$(loname).xml ]; then rm $(prefix)/share/mime/packages/application-x-$(loname).xml; fi
	if [ -d $(prefix)/share/$(loname) ]; then rm -rf $(prefix)/share/$(loname); fi
    endif
