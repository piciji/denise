
add_library(coreAmiga STATIC)

set(headers
        ../interface.h
        interface.h

        agnus/agnus.h
        agnus/blanking.cpp
        agnus/blitter.h
        agnus/blitterCycle.cpp
        agnus/blitterLLE.cpp
        agnus/blitterSequencer.cpp
        agnus/copper.h
        agnus/dma.cpp
        agnus/events.cpp
        agnus/graphics.cpp
        agnus/memory.cpp
        agnus/minterm.cpp
        agnus/register.cpp
        agnus/serialization.cpp
        cpu/m68000.h
        cpu/m68000/alu.cpp
        cpu/m68000/exception.cpp
        cpu/m68000/instruction.cpp
        cpu/m68000/m68000.h
        cpu/m68000/memory.cpp
        cpu/m68000/optable.cpp
        drive/dms/dms.h
        drive/ipf/CapsAPI.h
        drive/ipf/CommonTypes.h
        drive/adf.cpp
        drive/diskDrive.h
        drive/hardDrive.h
        drive/hardDiskStructure.h
        drive/diskStructure.h
        drive/dms.cpp
        drive/exe.cpp
        drive/ext.cpp
        drive/ext2.cpp
        drive/filesystem.h
        drive/filesystemExt.h
        drive/instantDiskDrive.cpp
        drive/ipf.cpp
        drive/scp.cpp
        drive/scp/scp.h
        drive/sectorBlock.h
        input/input.h
        input/controlPort/analogControl.h
        input/controlPort/cd32pad.cpp
        input/controlPort/controlPort.h
        input/controlPort/fourPlayerAdapter.cpp
        input/controlPort/joypad.cpp
        input/controlPort/mouse.cpp
        input/keyboard/keyboard.h
        paula/audio.cpp
        paula/fdc.cpp
        paula/filter.cpp
        paula/irq.cpp
        paula/paula.h
        paula/pot.cpp
        paula/serial.cpp
        system/dongle.cpp
        system/rtc.h
        system/serialization.cpp
        system/system.h
        video/denise.h
        expansionPort/expansionPort.h
        expansionPort/fastMem.h
        expansionPort/mtecAT500.h
        expansionPort/builtinHD.h
        expansionPort/hdController.h
        system/firmware.h

        ../tools/bits.h
        ../tools/buffer.h
        ../tools/circularBuffer.h
        ../tools/clamp.h
        ../tools/crc32.h
        ../tools/crop.h
        ../tools/dac.h
        ../tools/DLLoader.h
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

set_source_files_properties(coreAmiga ${headers}
        PROPERTIES HEADER_FILE_ONLY TRUE)

target_sources(coreAmiga PRIVATE
        interface.cpp

        cpu/m68000/m68000.cpp
        cpu/m68000/dasmHandler.cpp
        cpu/m68000.cpp
        agnus/agnus.cpp
        agnus/blitter.cpp
        agnus/copper.cpp
        video/denise.cpp
        paula/paula.cpp
        system/system.cpp
        input/input.cpp
        input/controlPort/controlPort.cpp
        input/keyboard/keyboard.cpp
        drive/diskDrive.cpp
        drive/hardDrive.cpp
        drive/hardDiskStructure.cpp
        drive/diskStructure.cpp
        drive/sectorBlock.cpp
        drive/filesystem.cpp
        drive/filesystemExt.cpp
        drive/hunk.cpp
        drive/scp/scp.cpp
        system/rtc.cpp
        drive/dms/pfile.cpp
        expansionPort/expansionPort.cpp
        expansionPort/fastMem.cpp
        expansionPort/hdController.cpp
        expansionPort/mtecAT500.cpp
        expansionPort/builtinHD.cpp
        system/firmware.cpp
        ../tools/DLLoader.cpp
        ../tools/uuid.cpp
        ../tools/error.cpp

        ${headers}
)

target_link_libraries(coreAmiga PRIVATE ciaNew)