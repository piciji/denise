
add_library(coreC64 STATIC)

set(headers
        ../interface.h
        interface.h
        disk/iec.h
        disk/cpu/m6502.h
        disk/drive/drive.h
        disk/drive/mechanics.cpp
        disk/drive/mechanicsG64.cpp
        disk/drive//mechanicsP64.cpp
        disk/drive/profdos.cpp
        disk/drive/prologic.cpp
        disk/drive/serialization.cpp
        disk/drive/turbotrans.cpp
        disk/structure/dxx.cpp
        disk/structure/gxx.cpp
        disk/structure/listing.cpp
        disk/structure/prg.cpp
        disk/structure/pxx.cpp
        disk/structure/structure.h
        disk/via/register.cpp
        disk/via/serialization.cpp
        disk/via/via.h
        disk/virtual/virtualDrive.h
        disk/wd177x/wd1770.h
        expansionPort/acia/acia.h
        expansionPort/cart/cart.h
        expansionPort/easyFlash/eapi-am29f040.h
        expansionPort/easyFlash/eapi-mx29640b.h
        expansionPort/easyFlash/easyFlash.h
        expansionPort/easyFlash/easyFlash3.h
        expansionPort/fastloader/fastloader.h
        expansionPort/finalChessCard/chessCard.h
        expansionPort/freezer/actionReplayMK2.h
        expansionPort/freezer/actionReplayMK3.h
        expansionPort/freezer/actionReplayMK4.h
        expansionPort/freezer/actionReplayV4.h
        expansionPort/freezer/atomicPower.h
        expansionPort/freezer/diashowMaker.h
        expansionPort/freezer/finalCartridge.h
        expansionPort/freezer/finalCartridge3.h
        expansionPort/freezer/finalCartridgePlus.h
        expansionPort/freezer/freezer.h
        expansionPort/freezer/superSnapshotV5.h
        expansionPort/gameCart/businessBasic.h
        expansionPort/gameCart/cart16k.h
        expansionPort/gameCart/comal80.h
        expansionPort/gameCart/dinamic.h
        expansionPort/gameCart/easycalc.h
        expansionPort/gameCart/funplay.h
        expansionPort/gameCart/gameCart.h
        expansionPort/gameCart/hyperBasic.h
        expansionPort/gameCart/mach5.h
        expansionPort/gameCart/magicDesk.h
        expansionPort/gameCart/magicDesk2.h
        expansionPort/gameCart/ocean.h
        expansionPort/gameCart/pagefox.h
        expansionPort/gameCart/rgcd.h
        expansionPort/gameCart/ross.h
        expansionPort/gameCart/silverrock.h
        expansionPort/gameCart/simonsBasic.h
        expansionPort/gameCart/supergames.h
        expansionPort/gameCart/system3.h
        expansionPort/gameCart/warpSpeed.h
        expansionPort/gameCart/westermann.h
        expansionPort/gameCart/zaxxon.h
        expansionPort/geoRam/geoRam.h
        expansionPort/gmod/gmod2.h
        expansionPort/retroReplay/memoryHandler.cpp
        expansionPort/retroReplay/retroReplay.h
        expansionPort/trilogicExpert/expert.h
        expansionPort/reu/reu.h
        expansionPort/superCpu/superCpu.h
        expansionPort/uci/uci.h
        expansionPort/expansionPort.h
        input/controlPort/analogControl.h
        input/controlPort/cga.cpp
        input/controlPort/controlPort.h
        input/controlPort/GunStick.cpp
        input/controlPort/inception.cpp
        input/controlPort/InkwellLightpen.cpp
        input/controlPort/joypad.cpp
        input/controlPort/lightControl.h
        input/controlPort/MagnumLightPhaser.cpp
        input/controlPort/mouse1351.cpp
        input/controlPort/mouseNeos.cpp
        input/controlPort/paddles.cpp
        input/controlPort/StackLightpen.cpp
        input/controlPort/StackLightRifle.cpp
        input/input.h
        input/keyboard.h
        m6510/m6510.h
        m6510/opcodes.cpp
        m6510/optable.h
        prg/prg.h
        sid/filter/build.cpp
        sid/filter/external.cpp
        sid/filter/main.cpp
        sid/filter/opamp.cpp
        sid/filter/resid24.cpp
        sid/filter/separateInput.cpp
        sid/clone.cpp
        sid/envelope.cpp
        sid/register.cpp
        sid/serialization.cpp
        sid/reSid.h
        sid/tables.cpp
        sid/voice.cpp
        system/usbSidPico.h
        system/clipboard.h
        system/expansion.cpp
        system/firmware.h
        system/gluelogic.h
        system/keyBuffer.h
        system/map.cpp
        system/memory.h
        system/serialization.cpp
        system/sidManager.h
        system/sid.h
        system/system.h
        system/testbench.h
        tape/counter.cpp
        tape/fetch.cpp
        tape/serialization.cpp
        tape/structure.h
        tape/write.cpp
        traps/baseDevice.h
        traps/traps.h
        vicII/fast/register.cpp
        vicII/fast/scanline.cpp
        vicII/fast/serialization.cpp
        vicII/fast/silence.cpp
        vicII/fast/vicIIFast.h
        vicII/base.h
        vicII/cycleTable.cpp
        vicII/dma.cpp
        vicII/fetch.cpp
        vicII/flags.h
        vicII/register.cpp
        vicII/sequencer.cpp
        vicII/serialization.cpp
        vicII/silence.cpp
        vicII/vicII.h

        ../tools/bits.h
        ../tools/buffer.h
        ../tools/circularBuffer.h
        ../tools/clamp.h
        ../tools/crc32.h
        ../tools/crop.h
        ../tools/dac.h
        ../tools/uuid.h
        ../tools/flash040.h
        ../tools/fpaq0.h
        ../tools/gcr.h
        ../tools/listing.h
        ../tools/m93c86.h
        ../tools/macros.h
        ../tools/memchangetracker.h
        ../tools/mx29lv640eb.h
        ../tools/petcii.h
        ../tools/pia.h
        ../tools/powersupply.h
        ../tools/rand.h
        ../tools/sanitizer.h
        ../tools/serializer.h
        ../tools/socket.h
        ../tools/splines.h
        ../tools/systimer.h
        ../tools/string.h
)

SET_SOURCE_FILES_PROPERTIES(system/usbSidPico.cpp
        PROPERTIES INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/deps/USBSID-Pico")

SET_SOURCE_FILES_PROPERTIES(sid/filter/chamberlin.cpp
        PROPERTIES COMPILE_FLAGS "-ffast-math -fno-exceptions")

set_source_files_properties(coreC64 ${headers}
        PROPERTIES HEADER_FILE_ONLY TRUE)

if(LIBUSB_FOUND)
    SET_SOURCE_FILES_PROPERTIES(system/usbSidPico.cpp PROPERTIES COMPILE_FLAGS "-DLIBUSB")
    target_link_libraries(coreC64 PRIVATE pico)
endif()

target_sources(coreC64 PRIVATE
        interface.cpp
        m6510/m6510.cpp
        m6510/m65Debugger.cpp;
        disk/cpu/m6502.cpp
        m6510/dasmHandler.cpp

        vicII/base.cpp
        vicII/vicII.cpp
        vicII/fast/vicIIFast.cpp
        system/system.cpp
        system/gluelogic.cpp
        system/firmware.cpp
        expansionPort/cart/cart.cpp
        system/keyBuffer.cpp
        expansionPort/gameCart/gameCart.cpp
        expansionPort/freezer/freezer.cpp
        expansionPort/reu/reu.cpp
        expansionPort/geoRam/geoRam.cpp
        expansionPort/easyFlash/easyFlash.cpp
        expansionPort/easyFlash/easyFlash3.cpp
        expansionPort/retroReplay/retroReplay.cpp
        expansionPort/trilogicExpert/expert.cpp
        expansionPort/gmod/gmod2.cpp
        expansionPort/fastloader/fastloader.cpp
        expansionPort/finalChessCard/chessCard.cpp
        expansionPort/superCpu/superCpu.cpp
        expansionPort/uci/uci.cpp

        system/clipboard.cpp
        sid/reSid.cpp
        sid/reSid24.cpp
        sid/chamberlin.cpp
        system/residfpHandler.cpp
        system/usbSidPico.cpp
        system/sidManager.cpp
        sid/filter/chamberlin.cpp
        tape/tape.cpp
        tape/structure.cpp
        prg/prg.cpp
        input/input.cpp
        input/controlPort/controlPort.cpp
        expansionPort/acia/acia.cpp
        disk/wd177x/wd1770.cpp

        traps/traps.cpp
        disk/virtual/virtualDrive.cpp
        disk/via/via.cpp
        disk/iec.cpp
        disk/drive/drive.cpp
        disk/structure/structure.cpp


        ../tools/pia.cpp
        ../tools/m93c86.cpp
        ../tools/mx29lv640eb.cpp
        ../tools/socket.cpp
        ../tools/fpaq0.cpp
        ../tools/expressionParser.cpp
        ../tools/watcher.cpp

        ${headers}
)

target_compile_options(coreC64 PRIVATE -Wno-shift-count-overflow)

SET_SOURCE_FILES_PROPERTIES(system/residfpHandler.cpp
        PROPERTIES INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/deps/libresidfp/src;")

if(ltoSupported)
   set_target_properties(coreC64 PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

target_link_libraries(coreC64 PRIVATE cia ciaNew cpu65c02 cpu65816 libresidfp)