
#pragma once

#include <cstdint>
#include "hdController.h"
#include "hdBase.h"
#include "../drive/hardDrive.h"

namespace LIBAMI {

    struct Agnus;

    struct BuiltinHD : HDBase {

        enum class Command {
            // Standard
            Invalid, Reset, Read, Write, Update, Clear, Stop, Start, Flush,
            // Non Standard
            Motor, Seek, Format, Remove, ChangeNum, ChangeState, ProtStatus, RawRead, RawWrite,
            GetDriveType, GetNumTracks, AddChangeInt, RemChangeInt, GetGeometry,
            SCSICMD = 28,
        };

        struct scsiCmd {
            uint32_t scsi_Data; /* word aligned data for SCSI Data Phase */
            uint32_t scsi_Length; /* even length of Data area */
            uint32_t scsi_Actual; /* actual Data used */
            uint32_t scsi_Command; /* SCSI Command (same options as scsi_Data) */
            uint16_t scsi_CmdLength; /* length of Command */
            uint16_t scsi_CmdActual; /* actual Command used */
            uint8_t scsi_Flags; /* includes intended data direction */
            uint8_t scsi_Status; /* SCSI status of command */
            uint32_t scsi_SenseData; /* sense data: filled if SCSIF_[OLD]AUTOSENSE */
            uint16_t scsi_SenseLength; /* size of scsi_SenseData, also bytes to */
            uint16_t scsi_SenseActual; /* amount actually fetched (0 means no sense) */
        };

        BuiltinHD(HDController& board, HardDrive& hardDrive) : agnus(board.agnus), HDBase(board, hardDrive) {}

        ~BuiltinHD();

        Agnus& agnus;

        uint32_t memPtr;

        auto type() -> const uint8_t { return (uint8_t)ExpansionPort::Type::ZORROII | (uint8_t)ExpansionPort::Type::ROM_VECTOR; }

        auto readAutoConf(uint32_t addr) -> uint8_t;

        auto read(uint32_t addr) -> uint8_t;

        auto peekW(uint32_t addr) -> uint16_t;

        auto readW(uint32_t addr) -> uint16_t;

        auto writeW(uint32_t addr, uint16_t data) -> void;

        auto reset() -> void;

        auto handleCmd(bool delayed = false) -> void;

        auto handleInit() -> void;

        auto handleResource() -> void;

        auto handleInfoReq() -> void;

        auto handleInitSeg() -> void;

        auto scsiCommand(scsiCmd& cmd) -> void;

        auto scsiCopyData(scsiCmd& sc, uint8_t* data, unsigned len) -> void;

        auto scsiFail(scsiCmd& sc, int error = -1, unsigned lba = ~0) -> void;

        auto scsiResult(scsiCmd& sc, uint32_t len, uint8_t status = 0, uint16_t senseLength = 0) -> void;

        auto verify(uint64_t offset, unsigned length, uint32_t addr) -> int8_t;

        auto fixKick12() -> void;

        auto serialize(Emulator::Serializer& s, bool light = false) -> void {
            s.integer(memPtr);
        }
    };

}
