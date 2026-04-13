
#include "miscHelper.h"

#include "../../emulation/libc64/interface.h"
#include "../program.h"
#include "../tools/chronos.h"
#include "../view/view.h"
#include "../view/status.h"
#include "../tools/filesetting.h"
#include "../tools/filepool.h"
#include "../emuconfig/config.h"
#include "../emuconfig/layouts/system.h"
#include "../states/states.h"
#include "settingsHelper.h"

auto MiscHelper::addCustomFont() -> void {
    for(auto emulator : emulators) {
        if ( dynamic_cast<LIBC64::Interface*>(emulator)) {
            GUIKIT::CustomFont font;
            font.name = "C64 Pro";
            font.refPtr = (void*)emulator;
            font.filePath = program->fontFolder() + "C64_Pro-STYLE121.ttf";
            font.modifier = 0xee << 8;
            bool useCustomFont = GUIKIT::Window::addCustomFont( font );
            ((LIBC64::Interface*) emulator)->convertPetsciiToScreencode(useCustomFont);
            ((LIBC64::Interface*) emulator)->loadWithColumn( Program::getSettings(emulator)->get<bool>("autostart_load_with_column") );

        } else if ( dynamic_cast<LIBAMI::Interface*>(emulator)) {
            GUIKIT::CustomFont font;
            font.name = "TopazPlus a500a1000a2000";
            //font.name = "Topaz a500a1000a2000";
            font.refPtr = (void*)emulator;
            font.filePath = program->fontFolder() + "TopazPlus_a500_v1.0.ttf";
            //font.filePath = program->fontFolder() + "Topaz_a500_v1.0.ttf";

            font.sizeAdjust = 1;
            font.modifier = 0;
            GUIKIT::Window::addCustomFont( font );
        }
    }
}

auto MiscHelper::libraryMissing(std::string plugin) -> void {
    static unsigned ts = 0;
    if (!program->initialized)
        return;

    unsigned tsTemp = Chronos::getTimestampInMilliseconds();
    if (tsTemp - ts < 1000)
        return;

    if (plugin == "CAPS") {
        static int informed = 0;
        if (view && (informed < 20) ) {
            view->message->error(trans->get("SPS plugin missing"));
            ts = Chronos::getTimestampInMilliseconds();
            informed++;
        }
    }
}

auto MiscHelper::setExpansionSelection( Emulator::Interface* emulator ) -> void {
    auto settings = Program::getSettings( emulator );

    for( auto& mediaGroup : emulator->mediaGroups ) {

        if ( mediaGroup.selected ) {

            auto mediaId = settings->get<unsigned>( _underscore( mediaGroup.name ) + "_selected", mediaGroup.media[0].id );

            auto media = emulator->getMedia( mediaGroup, mediaId );

            if (media && !media->secondary)
                mediaGroup.selected = media;
        }

        if (mediaGroup.isHardDisk()) {
            for (auto& media : mediaGroup.media) {
                auto pcbId = settings->get<unsigned>(_underscore(media.name) + "_pcb", mediaGroup.expansion->pcbs[0].id);

                auto pcbLayout = emulator->getPCB(*mediaGroup.expansion, pcbId);

                media.pcbLayout = pcbLayout ? pcbLayout : &mediaGroup.expansion->pcbs[0];
            }
        }
    }

    for ( auto& expansion : emulator->expansions ) {

        if (!expansion.mediaGroup || (expansion.pcbs.size() == 0) )
            continue;

        for(auto& media : expansion.mediaGroup->media) {

            if (!media.pcbLayout || media.secondary) {
                continue;
            }

            auto pcbId = settings->get<unsigned>( _underscore( media.name ) + "_pcb", expansion.pcbs[0].id );

            auto pcbLayout = emulator->getPCB( expansion, pcbId );

            media.pcbLayout = pcbLayout ? pcbLayout : &expansion.pcbs[0];
        }
    }
}

auto MiscHelper::removeExpansion( bool bootableOnly ) -> void {
    if (!activeEmulator)
        return;

    if (bootableOnly && !activeEmulator->isExpansionBootable())
        return;

    auto expansion = activeEmulator->getExpansion();

    if (!expansion || expansion->isEmpty())
        return;

    auto medias = expansion->mediaGroup->media;
    if (expansion->mediaGroupExpanded)
        medias = GUIKIT::Vector::concat( medias, expansion->mediaGroupExpanded->media );

    for( auto& media : medias) {
        filePool->assign( _ident(activeEmulator, media.name), nullptr);
        activeEmulator->ejectMedium( &media );
        auto state = States::getInstance( activeEmulator );
        if (state)
            state->updateImage( nullptr, &media );
    }

    activeEmulator->unsetExpansion();

    Program::getSettings( activeEmulator )->set<unsigned>("expansion", 0);

    auto emuView = EmuConfigView::TabWindow::getView(activeEmulator);
    if (emuView && emuView->systemLayout)
        emuView->systemLayout->setExpansion( nullptr );

    if (activeEmulator) {
        activeEmulator->powerOff();
        activeEmulator->power();
    }
}

auto MiscHelper::prepareSocket(Emulator::Interface::Media* media, Emulator::Interface* emulator, std::string address) -> void {
    std::string port;

    auto parts = GUIKIT::String::split( address, ':' );

    if (parts.size() > 1) {

        port = parts[ parts.size() - 1 ];

        address = parts[0];
    }

    emulator->prepareSocket( media, address, port );
}

auto MiscHelper::initExpansionRom(Emulator::Interface* emulator, const std::string& ident, const std::string& file) -> void {
    auto fSetting = FileSetting::getInstance( emulator, _underscore(ident) );
    fSetting->setPath(program->dataFolder() + file);
    fSetting->setFile(file);
    fSetting->setId(0);
    fSetting->setSaveable(false);
}

auto MiscHelper::toggle2Mhz() -> void {
    if (dynamic_cast<LIBC64::Interface*>(activeEmulator) && !hasSuperCpuActive()) {
        bool state = dynamic_cast<LIBC64::Interface*>(activeEmulator)->toggle2Mhz();
        if (statusHandler)
            statusHandler->setMessage(trans->getA(state ? "CPU Turbo 2 MHz" : "CPU Turbo disabled"), false, true);
    }
}

auto MiscHelper::hasSuperCpuActive() -> bool {
    if (dynamic_cast<LIBC64::Interface*>(activeEmulator)) {
        auto expansion = activeEmulator->getExpansion();
        if (!expansion)
            return false;

        if ((expansion->id == LIBC64::Interface::ExpansionId::ExpansionIdSuperCpu) || (expansion->id == LIBC64::Interface::ExpansionId::ExpansionIdSuperCpuReu))
            return true;
    }
    return false;
}
