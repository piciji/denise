#include "hdController.h"
#include "../agnus/agnus.h"
#include "../system/firmware.h"
#include "mtecAT500.h"
#include "builtinHD.h"

namespace LIBAMI {

    auto HDController::reset(bool softReset) -> void {
        ExpansionPort::reset(softReset);

        ident = Ident::BuiltIn;
        if (hardDrive.media->pcbLayout)
            ident = (Ident)hardDrive.media->pcbLayout->id;

        createPCB(ident);

        if (hardDrive.connected && hardDrive.attached()) {
            boardState = BoardState::AutoConf;
            pcb->reset();
        }        
    }

    auto HDController::createPCB(Ident ident) -> void {
        switch (ident) {
            default:
            case Ident::BuiltIn:
            case Ident::BuiltInRDB:
                if (!dynamic_cast<BuiltinHD*>(pcb)) {
                    delete pcb;
                    pcb = new BuiltinHD(*this, hardDrive);
                } break;
            case Ident::MtecAT500:
                if (!dynamic_cast<MtecAT500*>(pcb)) {
                    delete pcb;
                    pcb = new MtecAT500(*this, hardDrive);
                } break;
        }
    }

    auto HDController::readAutoConf(uint32_t addr) -> uint8_t {
        return pcb->readAutoConf(addr);
    }

    auto HDController::read(uint32_t addr) -> uint8_t {
        return pcb->read(addr);
    }

    auto HDController::readW(uint32_t addr)->uint16_t {
        return pcb->readW(addr);
    }

    auto HDController::write(uint32_t addr, uint8_t data) -> void {
        pcb->write(addr, data);
    }

    auto HDController::writeW(uint32_t addr, uint16_t data) -> void {
        pcb->writeW(addr, data);
    }

    auto HDController::serialize(Emulator::Serializer& s, bool light) -> void {
        ExpansionPort::serialize(s, light);

        s.integer((uint8_t&)ident);

        hardDrive.serialize(s, light, ident <= Ident::BuiltInRDB);

        if (!light && (s.mode() == Emulator::Serializer::Mode::Load)) {
            createPCB(ident);
            if (boardState == BoardState::Configured)
                add();

            if (hardDrive.connected)
                pcb->reset();
        }

        pcb->serialize(s, light);
    }
}
