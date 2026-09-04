
#include "interface.h"
#include "system/system.h"
#include "prg/prg.h"
#include "tape/tape.h"
#include "tape/structure.h"
#include "vicII/fast/vicIIFast.h"
#include "vicII/vicII.h"
#include "input/input.h"
#include "disk/iec.h"
#include "expansionPort/gameCart/gameCart.h"
#include "expansionPort/reu/reu.h"
#include "expansionPort/freezer/freezer.h"
#include "expansionPort/easyFlash/easyFlash.h"
#include "expansionPort/easyFlash/easyFlash3.h"
#include "expansionPort/retroReplay/retroReplay.h"
#include "expansionPort/gmod/gmod2.h"
#include "expansionPort/geoRam/geoRam.h"
#include "expansionPort/superCpu/superCpu.h"
#include "expansionPort/acia/acia.h"
#include "expansionPort/fastloader/fastloader.h"
#include "expansionPort/finalChessCard/chessCard.h"
#include "expansionPort/trilogicExpert/expert.h"
#include "disk/structure/structure.h"
#include "system/gluelogic.h"
#include "../tools/crop.h"

namespace LIBC64 {

const std::string Interface::Version = "234";
    
Interface::Interface() : Emulator::Interface( "C64" ) {
    
	prepareMedia();
	prepareFirmware();
	prepareDevices();
    prepareModels();
    preparePalettes();
    prepareExpansions();
    
    system = new System( this );    
}

Interface::~Interface() {
    if (system)
        delete system;
}

auto Interface::prepareMedia() -> void {
	mediaGroups.push_back({MediaGroupIdDisk, "Disk", MediaGroup::Type::Disk, {"d64", "g64", "p64", "d71", "g71", "p71", "d81", "g81", "p81"} });
	mediaGroups.push_back({MediaGroupIdTape, "Tape", MediaGroup::Type::Tape, {"tap"} });
	mediaGroups.push_back({MediaGroupIdProgram, "Program", MediaGroup::Type::Program, {"prg", "p00", "t64"} });
    mediaGroups.push_back({MediaGroupIdExpansionGame, "Cartridge", MediaGroup::Type::Expansion, {"bin", "crt"} });
    mediaGroups.push_back({MediaGroupIdExpansionEasyFlash, "EasyFlash", MediaGroup::Type::Expansion, {"bin", "crt"} });
    mediaGroups.push_back({MediaGroupIdExpansionEasyFlash3, "EasyFlash³", MediaGroup::Type::Expansion, {"bin", "crt"} });
    mediaGroups.push_back({MediaGroupIdExpansionFreezer, "Freezer", MediaGroup::Type::Expansion, {"bin", "crt"} });
    mediaGroups.push_back({MediaGroupIdExpansionRetroReplay, "Retro Replay", MediaGroup::Type::Expansion, {"bin", "crt"} });
    mediaGroups.push_back({MediaGroupIdExpansionGeoRam, "GeoRam", MediaGroup::Type::Expansion, {"bin", "georam"} });
    mediaGroups.push_back({MediaGroupIdExpansionReu, "REU", MediaGroup::Type::Expansion, {"bin", "crt", "prg", "reu"} });
    mediaGroups.push_back({MediaGroupIdExpansionRS232, "RS-232", MediaGroup::Type::Expansion });
    mediaGroups.push_back({MediaGroupIdExpansionFastloader, "Fast Loader", MediaGroup::Type::Expansion,{"bin", "crt","rom"} });
    mediaGroups.push_back({MediaGroupIdExpansionFinalChessCard, "Final Chesscard", MediaGroup::Type::Expansion,{"bin","rom"} });
    mediaGroups.push_back({MediaGroupIdExpansionSuperCpu, "SuperCPU", MediaGroup::Type::Expansion,{"bin","rom"} });
    mediaGroups.push_back({MediaGroupIdExpansionExpert, "Trilogic Expert", MediaGroup::Type::Expansion,{"crt"} });
    mediaGroups.push_back({MediaGroupIdExpansionGmod2, "GMod2", MediaGroup::Type::Expansion,{"bin", "crt"} });

    typedef Media::MemoryType MemType;

	{   auto& group = mediaGroups[MediaGroupIdDisk];
    
		group.media.push_back({0, "Device 8", 0, &group, MemType::Magnetic});
		group.media.push_back({1, "Device 9", 0, &group, MemType::Magnetic});
		group.media.push_back({2, "Device 10", 0, &group, MemType::Magnetic});
		group.media.push_back({3, "Device 11", 0, &group, MemType::Magnetic});
        group.selected = nullptr;
	}

	{   auto& group = mediaGroups[MediaGroupIdTape];
		group.media.push_back({0, "Datasette", 0, &group, MemType::Magnetic});
        group.selected = nullptr;
	}
    
	{   auto& group = mediaGroups[MediaGroupIdProgram];
		group.media.push_back({0, "Program 1", 0, &group, MemType::RAM});
        group.media.push_back({1, "Program 2", 0, &group, MemType::RAM});
        group.media.push_back({2, "Program 3", 0, &group, MemType::RAM});
        group.media.push_back({3, "Program 4", 0, &group, MemType::RAM});

        group.selected = &group.media[0];
	}
    
    {   auto& group = mediaGroups[MediaGroupIdExpansionGame];
		group.media.push_back({0, "Cartridge 1", 0, &group, MemType::ROM});
        group.media.push_back({1, "Cartridge 2", 0, &group, MemType::ROM});
        group.media.push_back({2, "Cartridge 3", 0, &group, MemType::ROM});
        group.media.push_back({3, "Cartridge 4", 0, &group, MemType::ROM});
        group.media.push_back({4, "Cartridge 5", 0, &group, MemType::ROM});
        group.media.push_back({5, "Cartridge 6", 0, &group, MemType::ROM});
        group.selected = &group.media[0];  
	}
    
    {   auto& group = mediaGroups[MediaGroupIdExpansionReu];
        group.media.push_back({0, "REU Memory 1", 0, &group, MemType::RAM});
        group.media.push_back({1, "REU Memory 2", 0, &group, MemType::RAM});
        group.media.push_back({2, "REU Memory 3", 0, &group, MemType::RAM});
        group.media.push_back({3, "REU Memory 4", 0, &group, MemType::RAM});
        group.media.push_back({4, "REU Rom 1", 0, &group, MemType::ROM});
	    group.media.push_back({5, "REU Rom 2", 0, &group, MemType::ROM});

        group.selected = &group.media[0];  
	}
    
    {   auto& group = mediaGroups[MediaGroupIdExpansionFreezer];
		group.media.push_back({0, "Freezer 1", 0, &group, MemType::ROM});
        group.media.push_back({1, "Freezer 2", 0, &group, MemType::ROM});
        group.media.push_back({2, "Freezer 3", 0, &group, MemType::ROM});
        group.media.push_back({3, "Freezer 4", 0, &group, MemType::ROM});
        group.media.push_back({4, "Freezer 5", 0, &group, MemType::ROM});
        group.media.push_back({5, "Freezer 6", 0, &group, MemType::ROM});
        group.selected = &group.media[0];  
	}

    {   auto& group = mediaGroups[MediaGroupIdExpansionEasyFlash];
		group.media.push_back({0, "EasyFlash 1", 0, &group, MemType::FLASH});
        group.media.push_back({1, "EasyFlash 2", 0, &group, MemType::FLASH});
        group.media.push_back({2, "EasyFlash 3", 0, &group, MemType::FLASH});
        group.media.push_back({3, "EasyFlash 4", 0, &group, MemType::FLASH});
        group.media.push_back({4, "EasyFlash 5", 0, &group, MemType::FLASH});
        group.media.push_back({5, "EasyFlash 6", 0, &group, MemType::FLASH});
        group.selected = &group.media[0];
	}

    {   auto& group = mediaGroups[MediaGroupIdExpansionEasyFlash3];
        group.media.push_back({0, "EF3 Slot 0", 0, &group, MemType::FLASH});
        group.media.push_back({1, "EF3 Slot 1", 0, &group, MemType::FLASH});
        group.media.push_back({2, "EF3 Slot 2", 0, &group, MemType::FLASH});
        group.media.push_back({3, "EF3 Slot 3", 0, &group, MemType::FLASH});
        group.media.push_back({4, "EF3 Slot 4", 0, &group, MemType::FLASH});
        group.media.push_back({5, "EF3 Slot 5", 0, &group, MemType::FLASH});
        group.media.push_back({6, "EF3 Slot 6", 0, &group, MemType::FLASH});
        group.media.push_back({7, "EF3 Slot 7", 0, &group, MemType::FLASH});
        group.selected = nullptr;
    }
    
    {   auto& group = mediaGroups[MediaGroupIdExpansionRetroReplay];
		group.media.push_back({0, "Retro Replay 1", 0, &group, MemType::FLASH});
        group.media.push_back({1, "Retro Replay 2", 0, &group, MemType::FLASH});
        group.media.push_back({2, "Retro Replay 3", 0, &group, MemType::FLASH});
        group.media.push_back({3, "Retro Replay 4", 0, &group, MemType::FLASH});
        group.media.push_back({4, "Retro Replay 5", 0, &group, MemType::FLASH});
        group.media.push_back({5, "Retro Replay 6", 0, &group, MemType::FLASH});
        group.selected = &group.media[0];  
	}

	{	auto& group = mediaGroups[MediaGroupIdExpansionGeoRam];
		group.media.push_back({0, "GeoRam Memory 1", 0, &group, MemType::SRAM});
		group.media.push_back({1, "GeoRam Memory 2", 0, &group, MemType::SRAM});
		group.media.push_back({2, "GeoRam Memory 3", 0, &group, MemType::SRAM});
		group.media.push_back({3, "GeoRam Memory 4", 0, &group, MemType::SRAM});
		group.selected = &group.media[0];
	}

    {	auto& group = mediaGroups[MediaGroupIdExpansionRS232];
        group.media.push_back({0, "RS-232 1", 0, &group, MemType::IP});
        group.media.push_back({1, "RS-232 2", 0, &group, MemType::IP});
        group.media.push_back({2, "RS-232 3", 0, &group, MemType::IP});
        group.media.push_back({3, "RS-232 4", 0, &group, MemType::IP});
        group.selected = &group.media[0];
    }

    {	auto& group = mediaGroups[MediaGroupIdExpansionFastloader];
        group.media.push_back({0, "Fast Loader 1", 0, &group, MemType::ROM});
        group.media.push_back({1, "Fast Loader 2", 0, &group, MemType::ROM});
        group.media.push_back({2, "Fast Loader 3", 0, &group, MemType::ROM});
	    group.media.push_back({3, "Fast Loader 4", 0, &group, MemType::ROM});
        group.selected = &group.media[0];
    }

    {   auto& group = mediaGroups[MediaGroupIdExpansionFinalChessCard];
	    group.media.push_back({0, "Final Chesscard CROM 1", 0, &group, MemType::ROM});
	    group.media.push_back({1, "Final Chesscard CROM 2", 0, &group, MemType::ROM});
	    group.media.push_back({2, "Final Chesscard BROM 1", 0, &group, MemType::ROM});
	    group.media.push_back({3, "Final Chesscard BROM 2", 0, &group, MemType::ROM});
	    group.media.push_back({4, "Final Chesscard RAM 1", 0, &group, MemType::SRAM});
	    group.media.push_back({5, "Final Chesscard RAM 2", 0, &group, MemType::SRAM});
	    group.selected = &group.media[0];
    }

    {   auto& group = mediaGroups[MediaGroupIdExpansionSuperCpu];
	    group.media.push_back({0, "SuperCPU 1", 0, &group, MemType::ROM});
	    group.media.push_back({1, "SuperCPU 2", 0, &group, MemType::ROM});
	    group.selected = &group.media[0];
    }

    {   auto& group = mediaGroups[MediaGroupIdExpansionExpert];
        group.media.push_back({0, "Trilogic Expert", 0, &group, MemType::RAM});
        group.selected = &group.media[0];
    }

    {   auto& group = mediaGroups[MediaGroupIdExpansionGmod2];
	    group.media.push_back({0, "GMod2 Flash 1", 0, &group, MemType::FLASH});
	    group.media.push_back({1, "GMod2 Flash 2", 0, &group, MemType::FLASH});
	    group.media.push_back({2, "GMod2 Flash 3", 0, &group, MemType::FLASH});

	    group.media.push_back({3, "GMod2 Eeprom 1", 0, &group, MemType::EEPROM});
	    group.media.push_back({4, "GMod2 Eeprom 2", 0, &group, MemType::EEPROM});
	    group.media.push_back({5, "GMod2 Eeprom 3", 0, &group, MemType::EEPROM});
	    group.selected = &group.media[0];
    }

    for(auto& group : mediaGroups) {
        group.expansion = nullptr;
        
        for(auto& media : group.media) {
            media.pcbLayout = nullptr;
            media.parent = nullptr;
        }
    }
       
    mediaGroups[MediaGroupIdExpansionReu].media[4].parent = &mediaGroups[MediaGroupIdExpansionReu].media[0];
    mediaGroups[MediaGroupIdExpansionReu].media[5].parent = &mediaGroups[MediaGroupIdExpansionReu].media[1];

    mediaGroups[MediaGroupIdExpansionFinalChessCard].media[2].parent = &mediaGroups[MediaGroupIdExpansionFinalChessCard].media[0];
    mediaGroups[MediaGroupIdExpansionFinalChessCard].media[3].parent = &mediaGroups[MediaGroupIdExpansionFinalChessCard].media[1];
    mediaGroups[MediaGroupIdExpansionFinalChessCard].media[4].parent = &mediaGroups[MediaGroupIdExpansionFinalChessCard].media[0];
    mediaGroups[MediaGroupIdExpansionFinalChessCard].media[5].parent = &mediaGroups[MediaGroupIdExpansionFinalChessCard].media[1];

    mediaGroups[MediaGroupIdExpansionGmod2].media[3].parent = &mediaGroups[MediaGroupIdExpansionGmod2].media[0];
    mediaGroups[MediaGroupIdExpansionGmod2].media[4].parent = &mediaGroups[MediaGroupIdExpansionGmod2].media[1];
    mediaGroups[MediaGroupIdExpansionGmod2].media[5].parent = &mediaGroups[MediaGroupIdExpansionGmod2].media[2];
}

auto Interface::prepareExpansions() -> void {
    expansions.push_back( { ExpansionIdNone, "Empty", Expansion::Type::Empty, nullptr, nullptr } );
    expansions.push_back( { ExpansionIdGame, "Cartridge", Expansion::Type::Standard, &mediaGroups[MediaGroupIdExpansionGame], nullptr } );
    expansions.push_back( { ExpansionIdEasyFlash, "EasyFlash", Expansion::Type::Flash, &mediaGroups[MediaGroupIdExpansionEasyFlash], nullptr } );
    expansions.push_back( { ExpansionIdEasyFlash3, "EasyFlash³", Expansion::Type::Flash | Expansion::Type::Freezer, &mediaGroups[MediaGroupIdExpansionEasyFlash3], nullptr } );
    expansions.push_back( { ExpansionIdFreezer, "Freezer", Expansion::Type::Freezer, &mediaGroups[MediaGroupIdExpansionFreezer], nullptr } );
    expansions.push_back( { ExpansionIdFastloader, "Fast Loader", Expansion::Type::Fastloader, &mediaGroups[MediaGroupIdExpansionFastloader], nullptr } );
    expansions.push_back( { ExpansionIdRetroReplay, "Retro Replay", Expansion::Type::Freezer | Expansion::Type::Flash, &mediaGroups[MediaGroupIdExpansionRetroReplay], nullptr } );
    expansions.push_back( { ExpansionIdGeoRam, "GeoRam", Expansion::Type::Ram | Expansion::Type::Battery, &mediaGroups[MediaGroupIdExpansionGeoRam], nullptr } );
    expansions.push_back( { ExpansionIdReu, "REU", Expansion::Type::Ram, &mediaGroups[MediaGroupIdExpansionReu], nullptr } );
	expansions.push_back( { ExpansionIdReuRetroReplay, "REU + Retro Replay", Expansion::Type::Ram | Expansion::Type::Freezer | Expansion::Type::Flash, &mediaGroups[MediaGroupIdExpansionReu], &mediaGroups[MediaGroupIdExpansionRetroReplay] } );
    expansions.push_back( { ExpansionIdRS232, "RS-232", Expansion::Type::RS232, &mediaGroups[MediaGroupIdExpansionRS232], nullptr } );
    expansions.push_back( { ExpansionIdGmod2, "GMod2", Expansion::Type::Standard | Expansion::Type::Flash | Expansion::Type::Eprom, &mediaGroups[MediaGroupIdExpansionGmod2], nullptr } );
    expansions.push_back( { ExpansionIdFinalChessCard, "Final Chesscard", Expansion::Type::Battery, &mediaGroups[MediaGroupIdExpansionFinalChessCard], nullptr } );
    expansions.push_back( { ExpansionIdSuperCpu, "SuperCPU", Expansion::Type::TurboCart | Expansion::Type::Ram, &mediaGroups[MediaGroupIdExpansionSuperCpu], nullptr } );
    expansions.push_back( { ExpansionIdSuperCpuReu, "SuperCPU + REU", Expansion::Type::TurboCart | Expansion::Type::Ram, &mediaGroups[MediaGroupIdExpansionSuperCpu], &mediaGroups[MediaGroupIdExpansionReu] });
    expansions.push_back( { ExpansionIdExpert, "Trilogic Expert", Expansion::Type::Ram | Expansion::Type::Freezer, &mediaGroups[MediaGroupIdExpansionExpert], nullptr } );
    
    {   auto& expansion = *getExpansionById( ExpansionIdGame );
        expansion.pcbs.push_back( {CartridgeIdDefault, "Default"} );
        expansion.pcbs.push_back( {CartridgeIdDefault8k, "Default 8k"} );
        expansion.pcbs.push_back( {CartridgeIdDefault16k, "Default 16k"} );
        expansion.pcbs.push_back( {CartridgeIdUltimax, "Ultimax"} );
        expansion.pcbs.push_back( {CartridgeIdOcean, "Ocean"} );
        expansion.pcbs.push_back( {CartridgeIdFunplay, "Funplay"} );
        expansion.pcbs.push_back( {CartridgeIdSuperGames, "Super Games"} );
        expansion.pcbs.push_back( {CartridgeIdSystem3, "System 3"} );
        expansion.pcbs.push_back( {CartridgeIdZaxxon, "Zaxxon"} );
        expansion.pcbs.push_back( {CartridgeIdMagicDesk, "Magic Desk"} );
        expansion.pcbs.push_back( {CartridgeIdMagicDesk2, "Magic Desk 2"} );
        expansion.pcbs.push_back( {CartridgeIdSimonsBasic, "Simons Basic"} );
        expansion.pcbs.push_back( {CartridgeIdWarpSpeed, "WarpSpeed"} );
        expansion.pcbs.push_back( {CartridgeIdMach5, "Mach 5"} );
        expansion.pcbs.push_back( {CartridgeIdRoss, "Ross"} );
        expansion.pcbs.push_back( {CartridgeIdWestermann, "Westermann"} );
        expansion.pcbs.push_back( {CartridgeIdPagefox, "Pagefox"} );
        expansion.pcbs.push_back( {CartridgeIdDinamic, "Dinamic"} );
        expansion.pcbs.push_back( {CartridgeIdComal80, "Comal-80"} );
        expansion.pcbs.push_back( {CartridgeIdSilverrock, "Silverrock"} );
        expansion.pcbs.push_back( {CartridgeIdRGCD, "RGCD"} );
        expansion.pcbs.push_back( {CartridgeIdRGCDHucky, "RGCD-Hucky"} );
        expansion.pcbs.push_back( {CartridgeIdEasyCalc, "Easy Calc"} );
        expansion.pcbs.push_back( {CartridgeIdHyperBasic, "Hyper-Basic"} );
        expansion.pcbs.push_back( {CartridgeIdBusinessBasic, "Kingsoft Business Basic"} );
        expansion.pcbs.push_back( {CartridgeIdStructuredBasic, "Structured Basic"} );
        expansion.pcbs.push_back( {CartridgeIdProphet64, "Prophet64"} );

        mediaGroups[MediaGroupIdExpansionGame].expansion = &expansion;
    }

    {   auto& expansion = *getExpansionById( ExpansionIdGmod2 );

        mediaGroups[MediaGroupIdExpansionGmod2].expansion = &expansion;
    }
                
    {   auto& expansion = *getExpansionById( ExpansionIdReu );
    
        mediaGroups[MediaGroupIdExpansionReu].expansion = &expansion;
    }
    
    {   auto& expansion = *getExpansionById( ExpansionIdFreezer );
        expansion.pcbs.push_back( {CartridgeIdDefault, "Default"} );
        expansion.pcbs.push_back( {CartridgeIdActionReplayMK2, "Action Replay MK2"} );
        expansion.pcbs.push_back( {CartridgeIdActionReplayMK3, "Action Replay MK3"} );
        expansion.pcbs.push_back( {CartridgeIdActionReplayMK4, "Action Replay MK4"} );
        expansion.pcbs.push_back( {CartridgeIdActionReplayV41AndHigher, "Action Replay V4"} );
        expansion.pcbs.push_back( {CartridgeIdFinalCartridge, "Final Cartridge"} );
        expansion.pcbs.push_back( {CartridgeIdFinalCartridgePlus, "Final Cartridge Plus"} );
        expansion.pcbs.push_back( {CartridgeIdFinalCartridge3, "Final Cartridge 3"} );
        expansion.pcbs.push_back( {CartridgeIdAtomicPower, "Atomic Power"} );
        expansion.pcbs.push_back( {CartridgeIdKcsPower, "KCS Power Cartridge"} );
        expansion.pcbs.push_back( {CartridgeIdDiashowMaker, "Diashow Maker"} );
        expansion.pcbs.push_back( {CartridgeIdSuperSnapshotV5, "Super Snapshot V5"} );
        
        mediaGroups[MediaGroupIdExpansionFreezer].expansion = &expansion;
    }
    
    {   auto& expansion = *getExpansionById( ExpansionIdEasyFlash );

        expansion.jumpers.push_back( {0, "Flash", false} );
    
        mediaGroups[MediaGroupIdExpansionEasyFlash].expansion = &expansion;
    }

    {   auto& expansion = *getExpansionById( ExpansionIdEasyFlash3 );

        expansion.pcbs.push_back( {0, "Slots 0 - 7"} );
        expansion.pcbs.push_back( {1, "Slot 0"} );

        mediaGroups[MediaGroupIdExpansionEasyFlash3].expansion = &expansion;
    }
    
    {   auto& expansion = *getExpansionById( ExpansionIdRetroReplay );
        expansion.pcbs.push_back( {CartridgeIdDefault, "Default"} );
        expansion.pcbs.push_back( {CartridgeIdRetroReplay, "Retro Replay"} );
        expansion.pcbs.push_back( {CartridgeIdNordicReplay, "Nordic Replay"} );
        
        expansion.jumpers.push_back( {0, "bank", false} );
        expansion.jumpers.push_back( {1, "flash", false} );
    
        mediaGroups[MediaGroupIdExpansionRetroReplay].expansion = &expansion;
    }

	{   auto& expansion = *getExpansionById( ExpansionIdGeoRam );
		
        mediaGroups[MediaGroupIdExpansionGeoRam].expansion = &expansion;
    }

    {   auto& expansion = *getExpansionById( ExpansionIdRS232 );
        expansion.pcbs.push_back( {CartridgeIdDefault, "Default"} );
        expansion.pcbs.push_back( {CartridgeIdSwiftlink, "Swiftlink"} );
        expansion.pcbs.push_back( {CartridgeIdTurbo232, "Turbo232"} );

        expansion.jumpers.push_back( {0, "IRQ", false} );
        expansion.jumpers.push_back( {1, "NMI", true} );
        expansion.jumpers.push_back( {2, "$DE00", true} );
        expansion.jumpers.push_back( {3, "IP232", true} );

        mediaGroups[MediaGroupIdExpansionRS232].expansion = &expansion;
    }

    {   auto& expansion = *getExpansionById( ExpansionIdFastloader );
        expansion.jumpers.push_back( {0, "Kernal Replacement", true} );
        expansion.pcbs.push_back( {CartridgeIdProfDos, "ProfDOS"} );
        expansion.pcbs.push_back( {CartridgeIdPrologicDos, "PrologicDOS"} );
        expansion.pcbs.push_back( {CartridgeIdTurboTrans, "Turbo Trans"} );
        expansion.pcbs.push_back( {CartridgeIdStarDos, "StarDOS"} );
        mediaGroups[MediaGroupIdExpansionFastloader].expansion = &expansion;
    }

    {   auto& expansion = *getExpansionById( ExpansionIdFinalChessCard );
        expansion.jumpers.push_back({0, "+10 MHz"} );
        expansion.jumpers.push_back({1, "+20 MHz"} );
        expansion.jumpers.push_back({2, "+30 MHz"} );
        expansion.jumpers.push_back({3, "+50 MHz"} );

        mediaGroups[MediaGroupIdExpansionFinalChessCard].expansion = &expansion;
    }

    {   auto& expansion = *getExpansionById( ExpansionIdSuperCpu );
        expansion.jumpers.push_back({0, "Turbo", true} );
        expansion.jumpers.push_back({1, "JiffyDOS"} );
        expansion.jumpers.push_back({2, "DRAM Boost"} );

        mediaGroups[MediaGroupIdExpansionSuperCpu].expansion = &expansion;
    }

    {   auto& expansion = *getExpansionById( ExpansionIdSuperCpuReu );
        expansion.jumpers.push_back({ 0, "Turbo", true });
        expansion.jumpers.push_back({ 1, "JiffyDOS" });
        expansion.jumpers.push_back({ 2, "DRAM Boost" });
    }

    {   auto& expansion = *getExpansionById( ExpansionIdExpert );
        expansion.jumpers.push_back( {0, "ON", false} );
        expansion.jumpers.push_back( {1, "PRG", false} );
        mediaGroups[MediaGroupIdExpansionExpert].expansion = &expansion;
    }

    for(auto& group : mediaGroups) {
        bool ef3 = group.id == MediaGroupIdExpansionEasyFlash3;

        for(auto& media : group.media) {
            if (ef3)
                media.pcbLayout = (media.id == 0) ? &group.expansion->pcbs[0] : nullptr;
            else
                media.pcbLayout = (!media.parent && group.expansion && group.expansion->pcbs.size())
                        ? &group.expansion->pcbs[0] : nullptr;
        }
    }
}

auto Interface::preparePalettes() -> void {
    
    palettes.push_back({ 0, "Colodore PAL", false, {
        {"Black", 0}, {"White", 0xffffff}, {"Red", 0x813338}, {"Cyan", 0x75cec8},
        {"Purple", 0x8e3c97}, {"Green", 0x56ac4d}, {"Blue", 0x2e2c9b}, {"Yellow", 0xedf171},
        {"Orange", 0x8e5029}, {"Brown", 0x553800}, {"Light Red", 0xc46c71}, {"Dark Gray", 0x4a4a4a},
        {"Medium Gray", 0x7b7b7b}, {"Light Green", 0xa9ff9f}, {"Light Blue", 0x706deb}, {"Light Gray", 0xb2b2b2}
    } });
    
    palettes.push_back({ 1, "Community Colors", false, {
        {"Black", 0}, {"White", 0xffffff}, {"Red", 0xaf2a29}, {"Cyan", 0x6ed8cc},
        {"Purple", 0xb03fb6}, {"Green", 0x4ac64a}, {"Blue", 0x3739c4}, {"Yellow", 0xe4ed4e},
        {"Orange", 0xb6591c}, {"Brown", 0x683808}, {"Light Red", 0xea746c}, {"Dark Gray", 0x4d4d4d},
        {"Medium Gray", 0x848484}, {"Light Green", 0xa6fa9e}, {"Light Blue", 0x707ce6}, {"Light Gray", 0xb6b6b5}
    } });

    palettes.push_back({ 2, "Pepto PAL", false, {
        {"Black", 0}, {"White", 0xffffff}, {"Red", 0x68372b}, {"Cyan", 0x70a4b2},
        {"Purple", 0x6f3d86}, {"Green", 0x588d43}, {"Blue", 0x352879}, {"Yellow", 0xb8c76f},
        {"Orange", 0x6f4f25}, {"Brown", 0x433900}, {"Light Red", 0x9a6759}, {"Dark Gray", 0x444444},
        {"Medium Gray", 0x6c6c6c}, {"Light Green", 0x9ad284}, {"Light Blue", 0x6c5eb5}, {"Light Gray", 0x959595}
    } });

    palettes.push_back({ 3, "Pepto PAL old VIC's", false, {
        {"Black", 0}, {"White", 0xffffff}, {"Red", 0x58291d}, {"Cyan", 0x91c6d5},
        {"Purple", 0x915ca8}, {"Green", 0x588d43}, {"Blue", 0x352879}, {"Yellow", 0xb8c76f},
        {"Orange", 0x916f43}, {"Brown", 0x433900}, {"Light Red", 0x9a6759}, {"Dark Gray", 0x353535},
        {"Medium Gray", 0x747474}, {"Light Green", 0x9ad284}, {"Light Blue", 0x7466be}, {"Light Gray", 0xb8b8b8}
    } });

    palettes.push_back({ 4, "Pepto NTSC", false, {
        {"Black", 0}, {"White", 0xffffff}, {"Red", 0x67372B}, {"Cyan", 0x70A3B1},
        {"Purple", 0x6F3D86}, {"Green", 0x588C42}, {"Blue", 0x342879}, {"Yellow", 0xB7C66E},
        {"Orange", 0x6F4E25}, {"Brown", 0x423800}, {"Light Red", 0x996659}, {"Dark Gray", 0x434343},
        {"Medium Gray", 0x6B6B6B}, {"Light Green", 0x9AD183}, {"Light Blue", 0x6B5EB5}, {"Light Gray", 0x959595}
    } });

    palettes.push_back({ 5, "Pepto NTSC Sony", false, {
        {"Black", 0}, {"White", 0xffffff}, {"Red", 0x7C352B}, {"Cyan", 0x5AA6B1},
        {"Purple", 0x694185}, {"Green", 0x5D8643}, {"Blue", 0x212E78}, {"Yellow", 0xCFBE6F},
        {"Orange", 0x894A26}, {"Brown", 0x5B3300}, {"Light Red", 0xAF6459}, {"Dark Gray", 0x434343},
        {"Medium Gray", 0x6B6B6B}, {"Light Green", 0xA0CB84}, {"Light Blue", 0x5665B3}, {"Light Gray", 0x959595}
    } });

    palettes.push_back({ 6, "Go Dot", false, {
        {"Black", 0}, {"White", 0xffffff}, {"Red", 0x880000}, {"Cyan", 0xAAFFEE},
        {"Purple", 0xCC44CC}, {"Green", 0x00CC55}, {"Blue", 0x0000AA}, {"Yellow", 0xEEEE77},
        {"Orange", 0xDD8855}, {"Brown", 0x664400}, {"Light Red", 0xFE7777}, {"Dark Gray", 0x333333},
        {"Medium Gray", 0x777777}, {"Light Green", 0xAAFF66}, {"Light Blue", 0x0088FF}, {"Light Gray", 0xBBBBBB}
    } });

    palettes.push_back({ 7, "Christopher Jam", false, {
        {"Black", 0}, {"White", 0xffffff}, {"Red", 0x7d202c}, {"Cyan", 0x4fb3a5},
        {"Purple", 0x84258c}, {"Green", 0x339840}, {"Blue", 0x2a1b9d}, {"Yellow", 0xbfd04a},
        {"Orange", 0x7f410d}, {"Brown", 0x4c2e00}, {"Light Red", 0xb44f5c}, {"Dark Gray", 0x3c3c3c},
        {"Medium Gray", 0x646464}, {"Light Green", 0x7ce587}, {"Light Blue", 0x6351db}, {"Light Gray", 0x939393}
    } });

    palettes.push_back({ 8, "RGB", false, {
        {"Black", 0}, {"White", 0xffffff}, {"Red", 0xFF0000}, {"Cyan", 0x00FFFF},
        {"Purple", 0xFF00FF}, {"Green", 0x00FF00}, {"Blue", 0x0000FF}, {"Yellow", 0xFFFF00},
        {"Orange", 0xFF8000}, {"Brown", 0x804000}, {"Light Red", 0xFF8080}, {"Dark Gray", 0x404040},
        {"Medium Gray", 0x808080}, {"Light Green", 0x80FF80}, {"Light Blue", 0x8080FF}, {"Light Gray", 0xC0C0C0}
    } });

    palettes.push_back({ 9, "C64 HQ", false, {
        {"Black", 0x0A0A0A}, {"White", 0xFFF8FF}, {"Red", 0x851F02}, {"Cyan", 0x65CDA8},
        {"Purple", 0xA73B9F}, {"Green", 0x4DAB19}, {"Blue", 0x1A0C92}, {"Yellow", 0xEBE353},
        {"Orange", 0xA94B02}, {"Brown", 0x441E00}, {"Light Red", 0xD28074}, {"Dark Gray", 0x464646},
        {"Medium Gray", 0x8B8B8B}, {"Light Green", 0x8EF68E}, {"Light Blue", 0x4D91D1}, {"Light Gray", 0xBABABA}
    } });

    palettes.push_back({ 10, "C64s", false, {
        {"Black", 0x000000}, {"White", 0xFCFCFC}, {"Red", 0xA80000}, {"Cyan", 0x54FCFC},
        {"Purple", 0xA800A8}, {"Green", 0x00A800}, {"Blue", 0x0000A8}, {"Yellow", 0xFCFC00},
        {"Orange", 0xA85400}, {"Brown", 0x802C00}, {"Light Red", 0xFC5454}, {"Dark Gray", 0x545454},
        {"Medium Gray", 0x808080}, {"Light Green", 0x54FC54}, {"Light Blue", 0x5454FC}, {"Light Gray", 0xA8A8A8}
    } });

    palettes.push_back({ 11, "Ccs64", false, {
        {"Black", 0x101010}, {"White", 0xFFFFFF}, {"Red", 0xE04040}, {"Cyan", 0x60FFFF},
        {"Purple", 0xE060E0}, {"Green", 0x40E040}, {"Blue", 0x4040E0}, {"Yellow", 0xFFFF40},
        {"Orange", 0xE0A040}, {"Brown", 0x9C7448}, {"Light Red", 0xFFA0A0}, {"Dark Gray", 0x545454},
        {"Medium Gray", 0x888888}, {"Light Green", 0xA0FFA0}, {"Light Blue", 0xA0A0FF}, {"Light Gray", 0xC0C0C0}
    } });

    palettes.push_back({ 12, "Frodo", false, {
        {"Black", 0x000000}, {"White", 0xFFFFFF}, {"Red", 0xCC0000}, {"Cyan", 0x00FFCC},
        {"Purple", 0xFF00FF}, {"Green", 0x00CC00}, {"Blue", 0x0000CC}, {"Yellow", 0xFFFF00},
        {"Orange", 0xFF8800}, {"Brown", 0x884400}, {"Light Red", 0xFF8888}, {"Dark Gray", 0x444444},
        {"Medium Gray", 0x888888}, {"Light Green", 0x88FF88}, {"Light Blue", 0x8888FF}, {"Light Gray", 0xCCCCCC}
    } });

    palettes.push_back({ 13, "PC64", false, {
        {"Black", 0x212121}, {"White", 0xFFFFFF}, {"Red", 0xB52121}, {"Cyan", 0x73FFFF},
        {"Purple", 0xB521B5}, {"Green", 0x21B521}, {"Blue", 0x2121B5}, {"Yellow", 0xFFFF21},
        {"Orange", 0xB57321}, {"Brown", 0x944221}, {"Light Red", 0xFF7373}, {"Dark Gray", 0x737373},
        {"Medium Gray", 0x949494}, {"Light Green", 0x73FF73}, {"Light Blue", 0x7373FF}, {"Light Gray", 0xB5B5B5}
    } });

    palettes.push_back({ 14, "Deekay", false, {
        {"Black", 0x000000}, {"White", 0xFFFFFF}, {"Red", 0x882000}, {"Cyan", 0x68D0A8},
        {"Purple", 0xA838A0}, {"Green", 0x50B818}, {"Blue", 0x181090}, {"Yellow", 0xF0E858},
        {"Orange", 0xA04800}, {"Brown", 0x472B1B}, {"Light Red", 0xC87870}, {"Dark Gray", 0x484848},
        {"Medium Gray", 0x808080}, {"Light Green", 0x98FF98}, {"Light Blue", 0x5090D0}, {"Light Gray", 0xB8B8B8}
    } });

    palettes.push_back({ 15, "Ptoing", false, {
        {"Black", 0x000000}, {"White", 0xFFFFFF}, {"Red", 0x8C3E34}, {"Cyan", 0x7ABFC7},
        {"Purple", 0x8D47B3}, {"Green", 0x68A941}, {"Blue", 0x3E31A2}, {"Yellow", 0xD0DC71},
        {"Orange", 0x905F25}, {"Brown", 0x574200}, {"Light Red", 0xBB776D}, {"Dark Gray", 0x545454},
        {"Medium Gray", 0x808080}, {"Light Green", 0xACEA88}, {"Light Blue", 0x7C70DA}, {"Light Gray", 0xABABAB}
    } });

	palettes.push_back({ 16, "AW1", false, {
        {"Black", 0}, {"White", 0xffffff}, {"Red", 0x9c493e}, {"Cyan", 0x81cbd5},
        {"Purple", 0x9e51c4}, {"Green", 0x74bb46}, {"Blue", 0x4d3cbb}, {"Yellow", 0xd6e26e},
        {"Orange", 0xa46d28}, {"Brown", 0x6f5700}, {"Light Red", 0xce857a}, {"Dark Gray", 0x636363},
        {"Medium Gray", 0x929292}, {"Light Green", 0xaff188}, {"Light Blue", 0x8d80f1}, {"Light Gray", 0xb6b6b6}
    } });

    palettes.push_back({ 17, "Ccs64 Dark External", false, {
        {"Black", 0}, {"White", 0xfffeff}, {"Red", 0x672e20}, {"Cyan", 0x6ca8ba},
        {"Purple", 0x70348a}, {"Green", 0x528f3a}, {"Blue", 0x2d1e7d}, {"Yellow", 0xbfcf69},
        {"Orange", 0x6f4919}, {"Brown", 0x3c3100}, {"Light Red", 0x9f6354}, {"Dark Gray", 0x3e3d3e},
        {"Medium Gray", 0x696869}, {"Light Green", 0x9bdd82}, {"Light Blue", 0x6958bf}, {"Light Gray", 0x999899}
    } });

    palettes.push_back({ 18, "Michiel PAL Board 1", false, {
        {"Black", 0x0},{"White", 0xfefefe},{"Red", 0x8b2c3f},{"Cyan", 0x62bdac},
        {"Purple", 0x9132a1},{"Green", 0x45a546},{"Blue", 0x3326ae},{"Yellow", 0xcad653},
        {"Orange", 0x8f4e13},{"Brown", 0x5d3a00},{"Light Red", 0xbf5e71},{"Dark Gray", 0x4a4a4a},
        {"Medium Gray", 0x747474},{"Light Green", 0x8bea8d},{"Light Blue", 0x705fea},{"Light Gray", 0xa0a0a0}}
    });

    palettes.push_back({ 19, "JAM PAL", false, {
        {"Black", 0x0},{"White", 0xffffff},{"Red", 0x72202c},{"Cyan", 0x4fb3a5},
        {"Purple", 0x84258c},{"Green", 0x339840},{"Blue", 0x2a1b9d},{"Yellow", 0xbfd04a},
        {"Orange", 0x7f410d},{"Brown", 0x4c2e00},{"Light Red", 0xb44f5c},{"Dark Gray", 0x3c3c3c},
        {"Medium Gray", 0x646464},{"Light Green", 0x7ce587},{"Light Blue", 0x6351db},{"Light Gray", 0x939393}}
    });

    palettes.push_back({ 20, "PALette 8565R2", false, {
        {"Black", 0x0},{"White", 0xffffff},{"Red", 0x893436},{"Cyan", 0x67beb9},
        {"Purple", 0x8c36a2},{"Green", 0x4ba646},{"Blue", 0x2d30a8},{"Yellow", 0xd2cf57},
        {"Orange", 0x8d501b},{"Brown", 0x533d00},{"Light Red", 0xbb6568},{"Dark Gray", 0x4e4e4e},
        {"Medium Gray", 0x767676},{"Light Green", 0x8ee989},{"Light Blue", 0x6669e1},{"Light Gray", 0xa3a3a3}}
    });

    for( auto& palette : palettes )
        for( auto& paletteColor : palette.paletteColors )
            paletteColor.updateChannels();
}

auto Interface::prepareModels() -> void {
	// select VIC-II model
	models.push_back({ModelIdVicIIModel, "VIC-II", Model::Type::Combo, Model::Purpose::GraphicChip, 0, {0, 7},
	{	"6569-R3 (PAL-B)", "8565 (PAL-B)", "6567-R8 (NTSC-M)", "8562 (NTSC-M)",
		"6569-R1 (PAL-B)", "6567-R56A (NTSC-M)", "6572 (PAL-N)", "6573 (PAL-M)" }});   		

    // use old or new ( 6526a ) cia chips
    models.push_back({ModelIdCiaRev, "CIA", Model::Type::Radio, Model::Purpose::Cia, 1, {0, 1}, {"6526", "8521"}});

	// 0 - off, 1 - on, means software decides
    models.push_back({ModelIdFilter, "SID Filter", Model::Type::Switch, Model::Purpose::AudioSettings, 1});
	// amplifies Sid 8580 digi sounds
	models.push_back({ModelIdDigiboost, "SID 8580 Digi Boost", Model::Type::Switch, Model::Purpose::AudioSettings, 0});
    // strengthen pseudo stereo effect
    models.push_back({ModelIdIntensifyPseudoStereo, "intensify Pseudo Stereo", Model::Type::Switch, Model::Purpose::AudioSettings, 0});
    // set reSIDfp waveform strength
    models.push_back({ModelIdSidWaveformStrength, "Wave Strength", Model::Type::Combo, Model::Purpose::AudioSettings, 0, {0, 2}, {"Average", "Weak", "Strong"}});
    // set SID driver
    models.push_back({ModelIdSidEngine, "SID Engine", Model::Type::Combo, Model::Purpose::AudioSettings, 0, {0, 3},
	{ "reSID", "residfp", "reSID old", "Chamberlin" }});
	// adjust center frequency for SID 6581
	models.push_back({ModelIdBias6581, "SID 6581 Curve", Model::Type::Slider, Model::Purpose::AudioSettings, 5500, {0, 10000}, {}, 400, 10000.0 });
    // adjust range for SID 6581
    models.push_back({ModelIdRange6581, "SID 6581 Range", Model::Type::Slider, Model::Purpose::AudioSettings, 5000, {0, 10000}, {}, 400, 10000.0 });
    // adjust center frequency for SID 8580
	models.push_back({ModelIdBias8580, "SID 8580 Curve", Model::Type::Slider, Model::Purpose::AudioSettings, 5000, {0, 10000}, {}, 400, 10000.0 });
    // use each 'x' sample. lower value means better quality but high cpu usage by resampler
    models.push_back({ModelIdSidSampleFetch, "Sample Interval", Model::Type::Radio, Model::Purpose::AudioResampler, 1, {0, 2}, {"Highest", "High", "Medium"}});
    // extra Sids
    models.push_back({ModelIdSidMulti, "Extra SIDs", Model::Type::Combo, Model::Purpose::AudioResampler, 0, {0, 7}, {"0", "1", "2", "3", "4", "5", "6", "7"}});
    
    models.push_back({ModelIdSidUsbPico, "USBSID-Pico", Model::Type::Switch, Model::Purpose::AudioExtern, 0});
    models.push_back({ModelIdSidUsbPicoBufferSize, "Pico Buffer Size", Model::Type::Slider, Model::Purpose::AudioExtern, 8192, {256, 16384}, {}, 64, 1.0 });
    models.push_back({ModelIdSidUsbPicoDiffSize, "Pico Diff Size", Model::Type::Slider, Model::Purpose::AudioExtern, 64, {16, 128}, {}, 14, 1.0 });

	models.push_back({ModelIdSid, "SID 1", Model::Type::Radio, Model::Purpose::SoundChip, 0, {0, 1}, {"8580", "6581"} });	
    models.push_back({ModelIdSid1Left, "SID 1 Left Channel", Model::Type::Switch, Model::Purpose::AudioResampler, 1});
    models.push_back({ModelIdSid1Right, "SID 1 Right Channel", Model::Type::Switch, Model::Purpose::AudioResampler, 0});
    models.push_back({ModelIdSid1Adr, "SID 1 Address", Model::Type::Combo, Model::Purpose::AudioSettings, 0, {0, (int)(SidManager::adrOptions.size() - 1)}, SidManager::adrOptions});

    models.push_back({ModelIdSid2, "SID 2", Model::Type::Radio, Model::Purpose::AudioSettings, 0, {0, 1}, {"8580", "6581"} });	
    models.push_back({ModelIdSid2Left, "SID 2 Left Channel", Model::Type::Switch, Model::Purpose::AudioResampler, 0});
    models.push_back({ModelIdSid2Right, "SID 2 Right Channel", Model::Type::Switch, Model::Purpose::AudioResampler, 1});
    models.push_back({ModelIdSid2Adr, "SID 2 Address", Model::Type::Combo, Model::Purpose::AudioSettings, 0, {0, (int)(SidManager::adrOptions.size() - 1)}, SidManager::adrOptions});
    
    models.push_back({ModelIdSid3, "SID 3", Model::Type::Radio, Model::Purpose::AudioSettings, 0, {0, 1}, {"8580", "6581"} });	
    models.push_back({ModelIdSid3Left, "SID 3 Left Channel", Model::Type::Switch, Model::Purpose::AudioResampler, 1});
    models.push_back({ModelIdSid3Right, "SID 3 Right Channel", Model::Type::Switch, Model::Purpose::AudioResampler, 0});
    models.push_back({ModelIdSid3Adr, "SID 3 Address", Model::Type::Combo, Model::Purpose::AudioSettings, 2, {0, (int)(SidManager::adrOptions.size() - 1)}, SidManager::adrOptions});
    
    models.push_back({ModelIdSid4, "SID 4", Model::Type::Radio, Model::Purpose::AudioSettings, 0, {0, 1}, {"8580", "6581"} });	
    models.push_back({ModelIdSid4Left, "SID 4 Left Channel", Model::Type::Switch, Model::Purpose::AudioResampler, 0});
    models.push_back({ModelIdSid4Right, "SID 4 Right Channel", Model::Type::Switch, Model::Purpose::AudioResampler, 1});
    models.push_back({ModelIdSid4Adr, "SID 4 Address", Model::Type::Combo, Model::Purpose::AudioSettings, 2, {0, (int)(SidManager::adrOptions.size() - 1)}, SidManager::adrOptions});
    
    models.push_back({ModelIdSid5, "SID 5", Model::Type::Radio, Model::Purpose::AudioSettings, 0, {0, 1}, {"8580", "6581"} });	
    models.push_back({ModelIdSid5Left, "SID 5 Left Channel", Model::Type::Switch, Model::Purpose::AudioResampler, 1});
    models.push_back({ModelIdSid5Right, "SID 5 Right Channel", Model::Type::Switch, Model::Purpose::AudioResampler, 0});
    models.push_back({ModelIdSid5Adr, "SID 5 Address", Model::Type::Combo, Model::Purpose::AudioSettings, 2, {0, (int)(SidManager::adrOptions.size() - 1)}, SidManager::adrOptions});
    
    models.push_back({ModelIdSid6, "SID 6", Model::Type::Radio, Model::Purpose::AudioSettings, 0, {0, 1}, {"8580", "6581"} });	
    models.push_back({ModelIdSid6Left, "SID 6 Left Channel", Model::Type::Switch, Model::Purpose::AudioResampler, 0});
    models.push_back({ModelIdSid6Right, "SID 6 Right Channel", Model::Type::Switch, Model::Purpose::AudioResampler, 1});
    models.push_back({ModelIdSid6Adr, "SID 6 Address", Model::Type::Combo, Model::Purpose::AudioSettings, 2, {0, (int)(SidManager::adrOptions.size() - 1)}, SidManager::adrOptions});
    
    models.push_back({ModelIdSid7, "SID 7", Model::Type::Radio, Model::Purpose::AudioSettings, 0, {0, 1}, {"8580", "6581"} });	
    models.push_back({ModelIdSid7Left, "SID 7 Left Channel", Model::Type::Switch, Model::Purpose::AudioResampler, 1});
    models.push_back({ModelIdSid7Right, "SID 7 Right Channel", Model::Type::Switch, Model::Purpose::AudioResampler, 0});
    models.push_back({ModelIdSid7Adr, "SID 7 Address", Model::Type::Combo, Model::Purpose::AudioSettings, 2, {0, (int)(SidManager::adrOptions.size() - 1)}, SidManager::adrOptions});
    
    models.push_back({ModelIdSid8, "SID 8", Model::Type::Radio, Model::Purpose::AudioSettings, 0, {0, 1}, {"8580", "6581"} });	
    models.push_back({ModelIdSid8Left, "SID 8 Left Channel", Model::Type::Switch, Model::Purpose::AudioResampler, 0});
    models.push_back({ModelIdSid8Right, "SID 8 Right Channel", Model::Type::Switch, Model::Purpose::AudioResampler, 1});
    models.push_back({ModelIdSid8Adr, "SID 8 Address", Model::Type::Combo, Model::Purpose::AudioSettings, 2, {0, (int)(SidManager::adrOptions.size() - 1)}, SidManager::adrOptions});

    // c64c use custom ic instead of discrete glue logic
    models.push_back({ModelIdGlueLogic, "Custom IC Glue Logic", Model::Type::Switch, Model::Purpose::Misc, 0});
	// disable grey dot bug for 85xx VIC-II
	models.push_back({ModelIdDisableGreyDotBug, "Disable Grey Dot Bug", Model::Type::Switch, Model::Purpose::Misc, 1});

    models.push_back({ModelId2Mhz, "C128 in C64 mode", Model::Type::Switch, Model::Purpose::Misc, 0});

    models.push_back({ModelIdDiskDrivesConnected, "Disk Drives", Model::Type::Combo, Model::Purpose::DriveSettings, 1, {0, 4},
                      { "0", "1", "2", "3", "4" }});

    models.push_back({ModelIdDiskDriveModel, "Drive Model", Model::Type::Combo, Model::Purpose::DriveSettings, 1, {0, 5},
                      { "1541", "1541-II", "1541-C", "1570", "1571", "1581" }});

    models.push_back({ModelIdDiskDriveSpeed, "Disk Speed", Model::Type::Slider, Model::Purpose::DriveSettings, 30000, {26000, 34000}, {}, 8000, 100.0 });

    models.push_back({ModelIdDiskDriveWobble, "Drive Wobble", Model::Type::Slider, Model::Purpose::DriveSettings, 20, {0, 500}, {}, 50, 100.0 });

    models.push_back({ModelIdDriveParallelCable, "Parallel Cable", Model::Type::Switch, Model::Purpose::DriveSettings, 0});
    models.push_back({ModelIdDriveRam20To3F, "$2000-$3FFF", Model::Type::Switch, Model::Purpose::DriveSettings, 0});
    models.push_back({ModelIdDriveRam40To5F, "$4000-$5FFF", Model::Type::Switch, Model::Purpose::DriveSettings, 0});
    models.push_back({ModelIdDriveRam60To7F, "$6000-$7FFF", Model::Type::Switch, Model::Purpose::DriveSettings, 0});
    models.push_back({ModelIdDriveRam80To9F, "$8000-$9FFF", Model::Type::Switch, Model::Purpose::DriveSettings, 0});
    models.push_back({ModelIdDriveRamA0ToBF, "$A000-$BFFF", Model::Type::Switch, Model::Purpose::DriveSettings, 0});

    models.push_back({ModelIdCiaBurstMode, "CIA Burst Modification", Model::Type::Switch, Model::Purpose::DriveSettings, 0});

    models.push_back({ModelIdDriveFastLoader, "Fast Loader", Model::Type::Combo, Model::Purpose::DriveSettings, 0, {0, 16},
        { "Manual", "SpeedDOS 1541", "DolphinDOS v2 1541", "DolphinDOS v2 Ultimate", "DolphinDOS v3 1541", "DolphinDOS v3 157x",
          "ProfDOS v1 1541", "ProfDOS R3/R4 1541", "ProfDOS R5 1570", "ProfDOS R6 1571", "PrologicDOS Classic 1541", "PrologicDOS 1541", "Turbo Trans", "ProSpeed 1571 v2.0", "StarDOS", "SuperCard+", "Disk Demon 1541"}});

    models.push_back({ModelIdTrackZeroSensor, "1541C Track-0 Sensor", Model::Type::Switch, Model::Purpose::DriveSettings, 0 });
    models.push_back({ModelIdTapeDrivesConnected, "Tape Drives", Model::Type::Combo, Model::Purpose::DriveSettings, 0, {0, 1},
                      { "0", "1" }});

    models.push_back({ModelIdTapeDriveWobble, "Tape Wobble", Model::Type::Switch, Model::Purpose::DriveSettings, 0});

    models.push_back({ModelIdCycleAccurateVideo, "Cycle Accurate Video", Model::Type::Switch, Model::Purpose::Performance, 1 });

    models.push_back({ModelIdDiskThread, "Disk Thread", Model::Type::Switch, Model::Purpose::Performance, 0 });

    models.push_back({ModelIdDiskOnDemand, "Disk On Demand", Model::Type::Switch, Model::Purpose::Performance, 1 });

    models.push_back({ModelIdReuRam, "REU Ram", Model::Type::Slider, Model::Purpose::Memory, 0, {0, 7}, { "128 KB", "256 KB", "512 KB", "1 MB", "2 MB", "4 MB", "8 MB", "16 MB" }});
    models.push_back({ModelIdGeoRam, "Geo Ram", Model::Type::Slider, Model::Purpose::Memory, 0, {0, 6}, { "64 KB", "128 KB", "256 KB", "512 KB", "1 MB", "2 MB", "4 MB" }});
    models.push_back({ModelIdSuperCpuRam, "SuperCPU Ram", Model::Type::Slider, Model::Purpose::Memory, 0, {0, 4}, { "none", "1 MB","4 MB", "8 MB", "16 MB" }});

    models.push_back({ModelIdEmulateDriveMechanics, "Emulate Mechanics", Model::Type::Switch, Model::Purpose::DriveMechanics, 0});
    models.push_back({ModelIdDriveStepperDelay, "Drive Stepper Delay", Model::Type::Slider, Model::Purpose::DriveMechanics, 90, {0, 140}, {}, 140, 10.0 });
    models.push_back({ModelIdDriveAcceleration, "Drive Acceleration", Model::Type::Slider, Model::Purpose::DriveMechanics, 704, {0, 1024}, {}, 256, 1.0 });
    models.push_back({ModelIdDriveDeceleration, "Drive Deceleration", Model::Type::Slider, Model::Purpose::DriveMechanics, 256, {0, 1024}, {}, 256, 1.0 });

    models.push_back({ModelIdDisableSpriteCollisions, "Disable Sprite Sprite Collisions", Model::Type::Switch, Model::Purpose::GraphicChip, 0});
    models.push_back({ModelIdDisableSpriteCharacterCollisions, "Disable Sprite Character Collisions", Model::Type::Switch, Model::Purpose::GraphicChip, 0});
}

auto Interface::prepareFirmware() -> void {
	firmwares.push_back({FirmwareIdKernal, "Kernal"});
	firmwares.push_back({FirmwareIdBasic, "Basic"});
	firmwares.push_back({FirmwareIdChar, "Char"});
    firmwares.push_back({FirmwareIdVC1541II, "VC1541-II"});
    firmwares.push_back({FirmwareIdVC1541, "VC1541"});
    firmwares.push_back({FirmwareIdVC1541C, "VC1541-C"});
    firmwares.push_back({FirmwareIdVC1571, "VC1571"});
    firmwares.push_back({FirmwareIdVC1570, "VC1570"});
    firmwares.push_back({FirmwareIdVC1581, "VC1581"});
    firmwares.push_back({FirmwareIdExpanded, "Expanded"});
}

auto Interface::prepareDevices() -> void {
	connectors.push_back( {0, "Port 1", Connector::Type::Port1} );
	connectors.push_back( {1, "Port 2", Connector::Type::Port2} );

    unsigned id = 0;
    
	{   Device device{ id++, "Unassigned", Device::Type::None };
        devices.push_back(device);
	}

	{   Device device{ id++, "Joypad #1", Device::Type::Joypad };
		device.inputs.push_back( {0, "Up", Key::Direction} );
		device.inputs.push_back( {1, "Down", Key::Direction} );
		device.inputs.push_back( {2, "Left", Key::Direction} );
		device.inputs.push_back( {3, "Right", Key::Direction} );
		device.inputs.push_back( {4, "Button 1", Key::Button} );
        device.inputs.push_back( {5, "Button 2", Key::Button} );

        device.addVirtual( "Button 1 Turbo", { 4 }, Key::Autofire );
        device.addVirtual( "Button 1 Autofire", { 4 }, Key::ToggleAutofire );
        device.addVirtual( "Button 2 Turbo", { 5 }, Key::Autofire );
        device.addVirtual( "Button 2 Autofire", { 5 }, Key::ToggleAutofire );
        device.addVirtual( "Left Turbo", { 2 }, Key::AutofireDirection );
        device.addVirtual( "Right Turbo", { 3 }, Key::AutofireDirection );

        device.addVirtual( "Diagonal Up-Right", { 0, 3 }, Key::JoyUpRight );
        device.addVirtual( "Diagonal Down-Right", { 1, 3 }, Key::JoyDownRight );
        device.addVirtual( "Diagonal Up-Left", { 0, 2 }, Key::JoyUpLeft );
        device.addVirtual( "Diagonal Down-Left", { 1, 2 }, Key::JoyDownLeft );
        
        devices.push_back(device);
        
        device.id = id++;
        device.name = "Joypad #2";
        devices.push_back(device);
	}

	{   Device device{ id++, "Mouse 1351 #1", Device::Type::Mouse };
		device.inputs.push_back( {0, "X-Axis"} );
		device.inputs.push_back( {1, "Y-Axis"} );
		device.inputs.push_back( {2, "Button Left"} );
		device.inputs.push_back( {3, "Button Right"} );

        devices.push_back(device);
        device.id = id++;
        device.name = "Mouse 1351 #2";
        devices.push_back(device);
	}

    {   Device device{ id++, "Mouse Neos #1", Device::Type::Mouse };
		device.inputs.push_back( {0, "X-Axis"} );
		device.inputs.push_back( {1, "Y-Axis"} );
		device.inputs.push_back( {2, "Button Left"} );
		device.inputs.push_back( {3, "Button Right"} );

        devices.push_back(device);
        device.id = id++;
        device.name = "Mouse Neos #2";
        devices.push_back(device);
	}
    
    {   Device device{ id++, "Paddles #1", Device::Type::Paddles };
		device.inputs.push_back( {0, "X-Axis"} );
		device.inputs.push_back( {1, "Y-Axis"} );
		device.inputs.push_back( {2, "Button X"} );
		device.inputs.push_back( {3, "Button Y"} );

        devices.push_back(device);
        device.id = id++;
        device.name = "Paddles #2";
        devices.push_back(device);
	}
    
    {   Device device{ id++, "Magnum Light Phaser", Device::Type::LightGun };
		device.inputs.push_back( {0, "X-Axis"} );
		device.inputs.push_back( {1, "Y-Axis"} );
		device.inputs.push_back( {2, "Trigger"} );

        devices.push_back(device);
	}    
    
    {   Device device{ id++, "Stack Light Rifle", Device::Type::LightGun };
		device.inputs.push_back( {0, "X-Axis"} );
		device.inputs.push_back( {1, "Y-Axis"} );
		device.inputs.push_back( {2, "Trigger"} );

        devices.push_back(device);
	}
    
    {   Device device{ id++, "Gun Stick #1", Device::Type::LightGun };
		device.inputs.push_back( {0, "X-Axis"} );
		device.inputs.push_back( {1, "Y-Axis"} );
		device.inputs.push_back( {2, "Trigger"} );

        devices.push_back(device);
        
        device.id = id++;
        device.name = "Gun Stick #2";
        devices.push_back(device);
	}
        
    {   Device device{ id++, "Inkwell Light Pen", Device::Type::LightPen };
		device.inputs.push_back( {0, "X-Axis"} );
		device.inputs.push_back( {1, "Y-Axis"} );
		device.inputs.push_back( {2, "Touch"} );
        device.inputs.push_back( {3, "Button"} );

        devices.push_back(device);
	}
        
    {   Device device{ id++, "Stack Light Pen", Device::Type::LightPen };
		device.inputs.push_back( {0, "X-Axis"} );
		device.inputs.push_back( {1, "Y-Axis"} );
		device.inputs.push_back( {2, "Touch"} );
        device.inputs.push_back( {3, "Button"} );

        devices.push_back(device);
	}
    
	{   Device device{ id++, "Keyboard", Device::Type::Keyboard };
	
		device.inputs.push_back( {0, "0", Key::D0 } ); device.inputs.push_back( {1, "1", Key::D1 } );
		device.inputs.push_back( {2, "2", Key::D2 } ); device.inputs.push_back( {3, "3", Key::D3 } );
		device.inputs.push_back( {4, "4", Key::D4 } ); device.inputs.push_back( {5, "5", Key::D5 } );
		device.inputs.push_back( {6, "6", Key::D6 } ); device.inputs.push_back( {7, "7", Key::D7 } );
		device.inputs.push_back( {8, "8", Key::D8 } ); device.inputs.push_back( {9, "9", Key::D9 } );
        
		device.inputs.push_back( {10, "A", Key::A } ); device.inputs.push_back( {11, "B", Key::B } );
		device.inputs.push_back( {12, "C", Key::C } ); device.inputs.push_back( {13, "D", Key::D } );
		device.inputs.push_back( {14, "E", Key::E } ); device.inputs.push_back( {15, "F", Key::F } );
		device.inputs.push_back( {16, "G", Key::G } ); device.inputs.push_back( {17, "H", Key::H } );
		device.inputs.push_back( {18, "I", Key::I } ); device.inputs.push_back( {19, "J", Key::J } );
		device.inputs.push_back( {20, "K", Key::K } ); device.inputs.push_back( {21, "L", Key::L } );
		device.inputs.push_back( {22, "M", Key::M } ); device.inputs.push_back( {23, "N", Key::N } );
		device.inputs.push_back( {24, "O", Key::O } ); device.inputs.push_back( {25, "P", Key::P } );
		device.inputs.push_back( {26, "Q", Key::Q } ); device.inputs.push_back( {27, "R", Key::R } );
		device.inputs.push_back( {28, "S", Key::S } ); device.inputs.push_back( {29, "T", Key::T } );
		device.inputs.push_back( {30, "U", Key::U } ); device.inputs.push_back( {31, "V", Key::V } );
		device.inputs.push_back( {32, "W", Key::W } ); device.inputs.push_back( {33, "X", Key::X } );
		device.inputs.push_back( {34, "Y", Key::Y } ); device.inputs.push_back( {35, "Z", Key::Z } );
        
		device.inputs.push_back( {36, "F1", Key::F1 } ); device.inputs.push_back( {37, "F3", Key::F3 } );
		device.inputs.push_back( {38, "F5", Key::F5 } ); device.inputs.push_back( {39, "F7", Key::F7 } );
        
		device.inputs.push_back( {40, "Cursor Right", Key::CursorRight } );
		device.inputs.push_back( {41, "Cursor Down", Key::CursorDown } );
		device.inputs.push_back( {42, "Backspace", Key::Backspace } );
        device.inputs.push_back( {43, "Return", Key::Return } ); 
		device.inputs.push_back( {44, "Plus", Key::Plus } );
        device.inputs.push_back( {45, "Minus", Key::Minus } ); 
		device.inputs.push_back( {46, "Left Shift", Key::ShiftLeft } );
		device.inputs.push_back( {47, "Right Shift", Key::ShiftRight } ); 
		device.inputs.push_back( {48, ".", Key::Period } );
        device.inputs.push_back( {49, ",", Key::Comma } );
		device.inputs.push_back( {50, ":", Key::Colon } );
        device.inputs.push_back( {51, ";", Key::Semicolon } ); 
		device.inputs.push_back( {52, "Pound", Key::Pound } ); 
        device.inputs.push_back( {53, "*", Key::Asterisk } ); 
		device.inputs.push_back( {54, "Space", Key::Space } );
        device.inputs.push_back( {55, "Arrow Left", Key::ArrowLeft } ); 
		device.inputs.push_back( {56, "Home", Key::Home } );
        device.inputs.push_back( {57, "Equal", Key::Equal } ); 
		device.inputs.push_back( {58, "C=", Key::Commodore } );
        device.inputs.push_back( {59, "RunStop", Key::RunStop } ); 
		device.inputs.push_back( {60, "Arrow Up", Key::ArrowUp } );
        device.inputs.push_back( {61, "/", Key::Slash } ); 
		device.inputs.push_back( {62, "Ctrl", Key::Ctrl} );
        device.inputs.push_back( {63, "@", Key::At} ); 				
		device.inputs.push_back( {64, "ShiftLock", Key::ShiftLock } ); 
		device.inputs.push_back( {65, "Restore", Key::Restore } );
        
        // virtual inputs (no physical keys)   
        // host could overmap default behaviour to match host keyboard layout better
        // otherwise you have to know the c64 keyboard layout to find the keys
        device.addVirtual( "!", { 47, 1 }, Key::ExclamationMark );
        device.addVirtual( "\"", { 47, 2 }, Key::DoubleQuotes );
        device.addVirtual( "#", { 47, 3 }, Key::NumberSign );
        device.addVirtual( "$", { 47, 4 }, Key::Dollar );
        device.addVirtual( "%", { 47, 5 }, Key::Percent );
        device.addVirtual( "&", { 47, 6 }, Key::Ampersand  );
        device.addVirtual( "´", { 47, 7 }, Key::Acute );
        device.addVirtual( "(", { 47, 8 }, Key::ParenthesesLeft );
        device.addVirtual( ")", { 47, 9 }, Key::ParenthesesRight );        
        device.addVirtual( "F2", { 47, 36 }, Key::F2 );
        device.addVirtual( "F4", { 47, 37 }, Key::F4);
        device.addVirtual( "F6", { 47, 38 }, Key::F6 );
        device.addVirtual( "F8", { 47, 39 }, Key::F8 );        
        device.addVirtual( "Cursor Left", { 47, 40 }, Key::CursorLeft );
        device.addVirtual( "Cursor Up", { 47, 41 }, Key::CursorUp );        
        device.addVirtual( ">", { 47, 48 }, Key::Greater );
        device.addVirtual( "<", { 47, 49 }, Key::Less );        
        device.addVirtual( "[", { 47, 50 }, Key::OpenSquareBracket );
        device.addVirtual( "]", { 47, 51 }, Key::ClosedSquareBracket );        
        device.addVirtual( "?", { 47, 61 }, Key::QuestionMark );
        device.addVirtual( "|", { 47, 45 }, Key::Pipe );
		
        devices.push_back(device);         
	}

    {   Device device{ id++, "CGA 4 Player", Device::Type::MultiPlayerAdapter };
        device.inputs.push_back({ 0, "Up", Key::Direction });
        device.inputs.push_back({ 1, "Down", Key::Direction });
        device.inputs.push_back({ 2, "Left", Key::Direction });
        device.inputs.push_back({ 3, "Right", Key::Direction });
        device.inputs.push_back({ 4, "Button 1", Key::Button });
        device.inputs.push_back({ 5, "Button 2", Key::Button });

        device.inputs.push_back({ 6, "Port3: Up", Key::Direction });
        device.inputs.push_back({ 7, "Port3: Down", Key::Direction });
        device.inputs.push_back({ 8, "Port3: Left", Key::Direction });
        device.inputs.push_back({ 9, "Port3: Right", Key::Direction });
        device.inputs.push_back({ 10, "Port3: Button 1", Key::Button });

        device.inputs.push_back({ 11, "Port4: Up", Key::Direction });
        device.inputs.push_back({ 12, "Port4: Down", Key::Direction });
        device.inputs.push_back({ 13, "Port4: Left", Key::Direction });
        device.inputs.push_back({ 14, "Port4: Right", Key::Direction });
        device.inputs.push_back({ 15, "Port4: Button 1", Key::Button });

        device.addVirtual("Button 1 Turbo", { 4 }, Key::Autofire);
        device.addVirtual("Button 1 Autofire", { 4 }, Key::ToggleAutofire);
        device.addVirtual("Button 2 Turbo", { 5 }, Key::Autofire);
        device.addVirtual("Button 2 Autofire", { 5 }, Key::ToggleAutofire);
        device.addVirtual("Left Turbo", { 2 }, Key::AutofireDirection);
        device.addVirtual("Right Turbo", { 3 }, Key::AutofireDirection);

        device.addVirtual("Port3: Button 1 Turbo", { 10 }, Key::Autofire);
        device.addVirtual("Port3: Button 1 Autofire", { 10 }, Key::ToggleAutofire);
        device.addVirtual("Port3: Left Turbo", { 8 }, Key::AutofireDirection);
        device.addVirtual("Port3: Right Turbo", { 9 }, Key::AutofireDirection);

        device.addVirtual("Port4: Button 1 Turbo", { 15 }, Key::Autofire);
        device.addVirtual("Port4: Button 1 Autofire", { 15 }, Key::ToggleAutofire);
        device.addVirtual("Port4: Left Turbo", { 13 }, Key::AutofireDirection);
        device.addVirtual("Port4: Right Turbo", { 14 }, Key::AutofireDirection);

        device.addVirtual("Diagonal Up-Right", { 0, 3 }, Key::JoyUpRight);
        device.addVirtual("Diagonal Down-Right", { 1, 3 }, Key::JoyDownRight);
        device.addVirtual("Diagonal Up-Left", { 0, 2 }, Key::JoyUpLeft);
        device.addVirtual("Diagonal Down-Left", { 1, 2 }, Key::JoyDownLeft);

        device.addVirtual("Port3: Diagonal Up-Right", { 6, 9 }, Key::JoyUpRight);
        device.addVirtual("Port3: Diagonal Down-Right", { 7, 9 }, Key::JoyDownRight);
        device.addVirtual("Port3: Diagonal Up-Left", { 6, 8 }, Key::JoyUpLeft);
        device.addVirtual("Port3: Diagonal Down-Left", { 7, 8 }, Key::JoyDownLeft);

        device.addVirtual("Port4: Diagonal Up-Right", { 11, 14 }, Key::JoyUpRight);
        device.addVirtual("Port4: Diagonal Down-Right", { 12, 14 }, Key::JoyDownRight);
        device.addVirtual("Port4: Diagonal Up-Left", { 11, 13 }, Key::JoyUpLeft);
        device.addVirtual("Port4: Diagonal Down-Left", { 12, 13 }, Key::JoyDownLeft);

        devices.push_back(device);
    }

    {   Device device{ id++, "Inception 8 Player", Device::Type::MultiPlayerAdapter };
        device.inputs.push_back({ 0, "Port1: Up", Key::Direction });
        device.inputs.push_back({ 1, "Port1: Down", Key::Direction });
        device.inputs.push_back({ 2, "Port1: Left", Key::Direction });
        device.inputs.push_back({ 3, "Port1: Right", Key::Direction });
        device.inputs.push_back({ 4, "Port1: Button 1", Key::Button });
        device.inputs.push_back({ 5, "Port1: Button 2", Key::Button });

        device.inputs.push_back({ 6, "Port2: Up", Key::Direction });
        device.inputs.push_back({ 7, "Port2: Down", Key::Direction });
        device.inputs.push_back({ 8, "Port2: Left", Key::Direction });
        device.inputs.push_back({ 9, "Port2: Right", Key::Direction });
        device.inputs.push_back({ 10, "Port2: Button 1", Key::Button });
        device.inputs.push_back({ 11, "Port2: Button 2", Key::Button });

        device.inputs.push_back({ 12, "Port3: Up", Key::Direction });
        device.inputs.push_back({ 13, "Port3: Down", Key::Direction });
        device.inputs.push_back({ 14, "Port3: Left", Key::Direction });
        device.inputs.push_back({ 15, "Port3: Right", Key::Direction });
        device.inputs.push_back({ 16, "Port3: Button 1", Key::Button });
        device.inputs.push_back({ 17, "Port3: Button 2", Key::Button });

        device.inputs.push_back({ 18, "Port4: Up", Key::Direction });
        device.inputs.push_back({ 19, "Port4: Down", Key::Direction });
        device.inputs.push_back({ 20, "Port4: Left", Key::Direction });
        device.inputs.push_back({ 21, "Port4: Right", Key::Direction });
        device.inputs.push_back({ 22, "Port4: Button 1", Key::Button });
        device.inputs.push_back({ 23, "Port4: Button 2", Key::Button });

        device.inputs.push_back({ 24, "Port5: Up", Key::Direction });
        device.inputs.push_back({ 25, "Port5: Down", Key::Direction });
        device.inputs.push_back({ 26, "Port5: Left", Key::Direction });
        device.inputs.push_back({ 27, "Port5: Right", Key::Direction });
        device.inputs.push_back({ 28, "Port5: Button 1", Key::Button });
        device.inputs.push_back({ 29, "Port5: Button 2", Key::Button });

        device.inputs.push_back({ 30, "Port6: Up", Key::Direction });
        device.inputs.push_back({ 31, "Port6: Down", Key::Direction });
        device.inputs.push_back({ 32, "Port6: Left", Key::Direction });
        device.inputs.push_back({ 33, "Port6: Right", Key::Direction });
        device.inputs.push_back({ 34, "Port6: Button 1", Key::Button });
        device.inputs.push_back({ 35, "Port6: Button 2", Key::Button });

        device.inputs.push_back({ 36, "Port7: Up", Key::Direction });
        device.inputs.push_back({ 37, "Port7: Down", Key::Direction });
        device.inputs.push_back({ 38, "Port7: Left", Key::Direction });
        device.inputs.push_back({ 39, "Port7: Right", Key::Direction });
        device.inputs.push_back({ 40, "Port7: Button 1", Key::Button });
        device.inputs.push_back({ 41, "Port7: Button 2", Key::Button });

        device.inputs.push_back({ 42, "Port8: Up", Key::Direction });
        device.inputs.push_back({ 43, "Port8: Down", Key::Direction });
        device.inputs.push_back({ 44, "Port8: Left", Key::Direction });
        device.inputs.push_back({ 45, "Port8: Right", Key::Direction });
        device.inputs.push_back({ 46, "Port8: Button 1", Key::Button });
        device.inputs.push_back({ 47, "Port8: Button 2", Key::Button });

        device.addVirtual("Port1: Button 1 Turbo", { 4 }, Key::Autofire);
        device.addVirtual("Port1: Button 1 Autofire", { 4 }, Key::ToggleAutofire);
        device.addVirtual("Port1: Button 2 Turbo", { 5 }, Key::Autofire);
        device.addVirtual("Port1: Button 2 Autofire", { 5 }, Key::ToggleAutofire);

        device.addVirtual("Port2: Button 1 Turbo", { 10 }, Key::Autofire);
        device.addVirtual("Port2: Button 1 Autofire", { 10 }, Key::ToggleAutofire);
        device.addVirtual("Port2: Button 2 Turbo", { 11 }, Key::Autofire);
        device.addVirtual("Port2: Button 2 Autofire", { 11 }, Key::ToggleAutofire);

        device.addVirtual("Port3: Button 1 Turbo", { 16 }, Key::Autofire);
        device.addVirtual("Port3: Button 1 Autofire", { 16 }, Key::ToggleAutofire);
        device.addVirtual("Port3: Button 2 Turbo", { 17 }, Key::Autofire);
        device.addVirtual("Port3: Button 2 Autofire", { 17 }, Key::ToggleAutofire);

        device.addVirtual("Port4: Button 1 Turbo", { 22 }, Key::Autofire);
        device.addVirtual("Port4: Button 1 Autofire", { 22 }, Key::ToggleAutofire);
        device.addVirtual("Port4: Button 2 Turbo", { 23 }, Key::Autofire);
        device.addVirtual("Port4: Button 2 Autofire", { 23 }, Key::ToggleAutofire);

        device.addVirtual("Port5: Button 1 Turbo", { 28 }, Key::Autofire);
        device.addVirtual("Port5: Button 1 Autofire", { 28 }, Key::ToggleAutofire);
        device.addVirtual("Port5: Button 2 Turbo", { 29 }, Key::Autofire);
        device.addVirtual("Port5: Button 2 Autofire", { 29 }, Key::ToggleAutofire);

        device.addVirtual("Port6: Button 1 Turbo", { 34 }, Key::Autofire);
        device.addVirtual("Port6: Button 1 Autofire", { 34 }, Key::ToggleAutofire);
        device.addVirtual("Port6: Button 2 Turbo", { 35 }, Key::Autofire);
        device.addVirtual("Port6: Button 2 Autofire", { 35 }, Key::ToggleAutofire);

        device.addVirtual("Port7: Button 1 Turbo", { 40 }, Key::Autofire);
        device.addVirtual("Port7: Button 1 Autofire", { 40 }, Key::ToggleAutofire);
        device.addVirtual("Port7: Button 2 Turbo", { 41 }, Key::Autofire);
        device.addVirtual("Port7: Button 2 Autofire", { 41 }, Key::ToggleAutofire);

        device.addVirtual("Port8: Button 1 Turbo", { 46 }, Key::Autofire);
        device.addVirtual("Port8: Button 1 Autofire", { 46 }, Key::ToggleAutofire);
        device.addVirtual("Port8: Button 2 Turbo", { 47 }, Key::Autofire);
        device.addVirtual("Port8: Button 2 Autofire", { 47 }, Key::ToggleAutofire);

        devices.push_back(device);
    }
        
    for (auto& device : devices) {
        device.userData = 0;
        
        for (auto& input : device.inputs) {
            input.type = input.name.find( "Axis" ) != std::string::npos ? 1 : 0;    
			input.guid = 0;
		}
    }
}

auto Interface::connect(unsigned connectorId, unsigned deviceId) -> void {
    
    system->input.connectControlport( getConnector( connectorId ), getDevice( deviceId ) );
}

auto Interface::connect(Connector* connector, Device* device) -> void {
    
    system->input.connectControlport( connector, device );
}

auto Interface::getConnectedDevice( Connector* connector ) -> Device* {
    
    auto device = system->input.getConnectedDevice( connector );
    
    if (!device)
        return getUnplugDevice();
    
    return device;
}

auto Interface::getCursorPosition( Device* device, int16_t& x, int16_t& y ) -> bool {
    
    return system->input.getCursorPosition( device, x, y );
}

auto Interface::power() -> void {
    system->power();
}

auto Interface::reset() -> void {
    
    if ( !system->expansionPort->resetButton() )
        system->power( true );
}

auto Interface::powerOff() -> void {
	system->powerOff();
}

auto Interface::run() -> void {    
    system->run();
}

auto Interface::runAhead(unsigned frames) -> void {
    system->setRunAhead(frames);
}

auto Interface::runAheadPerformance(bool state) -> void {
    system->runAhead.performance = state;
}

auto Interface::runAheadPreventJit(bool state) -> void {
    system->runAhead.preventJit = state;
    system->input.updateSampling();
}

auto Interface::getRegionEncoding() -> Region {
    return system->vicII->isNTSCEncoding() ? Region::Ntsc : Region::Pal;
}

auto Interface::getRegionGeometry() -> Region {
    return system->vicII->isNTSCGeometry() ? Region::Ntsc : Region::Pal;
}

auto Interface::getSubRegion() -> SubRegion {
	
	switch(system->vicII->getModel()) {
		default:
			return SubRegion::Pal_B;
			
		case VicIIBase::Model::MOS6567R8:
		case VicIIBase::Model::MOS8562:
		case VicIIBase::Model::MOS6567R56A:
			return SubRegion::Ntsc_M;
			
		case VicIIBase::Model::MOS6572:
			return SubRegion::Pal_N;
		case VicIIBase::Model::MOS6573:
			return SubRegion::Pal_M;
	}	
	
	return SubRegion::Pal_B;
}

auto Interface::insertDisk(Media* media, uint8_t* data, unsigned size) -> void {
    
    if (!media || !media->group->isDisk())
        return;
    
    system->iecBus.attach( media, data, size );
}

auto Interface::writeProtectDisk(Media* media, bool state) -> void {

    if (!media || !media->group->isDisk())
        return;

    system->iecBus.writeProtect( media, state );
}

auto Interface::isWriteProtectedDisk(Media* media) -> bool {

    if (!media || !media->group->isDisk())
        return false;
    
    return system->iecBus.isWriteProtected( media );
}

auto Interface::ejectDisk(Media* media) -> void {

    if (!media || !media->group->isDisk())
        return;

    system->iecBus.detach( media );
}

auto Interface::resetDrive(Media* media) -> void {
    system->iecBus.resetDrive( media );
}

auto Interface::hideDrive(Media* media) -> void {
    system->iecBus.hideDrive( media );
}

auto Interface::createDiskImage(unsigned typeId, std::string name, bool hd, bool ffs, bool bootable) -> Data {
	
    return DiskStructure::create( (DiskStructure::Type) typeId, name );
}

auto Interface::getDiskListing(Media* media) -> std::vector<Emulator::Interface::Listing> {
    
    if (!media || !media->group->isDisk())
        return {};
    
    return system->iecBus.getDiskListing( media );
}

auto Interface::getDiskPreview(uint8_t* data, unsigned size, Media* media) -> std::vector<Emulator::Interface::Listing> {
    
    DiskStructure structure(system);
	structure.number = media ? media->id : 0;
    
    if (!structure.attach( data, size ))
        return {};
        
    return structure.getListing();
}

auto Interface::selectDiskListing(Media* media, unsigned pos, uint8_t options) -> void {
    
    if (!media || !media->group->isDisk())
        return;

    system->iecBus.selectListing( media, pos, options );
}

auto Interface::selectDiskListing(Media* media, std::string fileName, uint8_t options) -> void {

    if (!media || !media->group->isDisk())
        return;

    system->iecBus.selectListing( media, fileName, options );
}

auto Interface::insertTape(Media* media, uint8_t* data, unsigned size) -> void {
		
    if (!media || !media->group->isTape())
        return;

    system->tape.load( data, size );
}

auto Interface::writeProtectTape(Media* media, bool state) -> void {

    if (!media || !media->group->isTape())
        return;

    system->tape.setWriteProtect( state );
}

auto Interface::isWriteProtectedTape(Media* media) -> bool {

    if (!media || !media->group->isTape())
        return false;
	
	return system->tape.isWriteProtected( );
}

auto Interface::ejectTape(Media* media) -> void {
		
    if (!media || !media->group->isTape())
        return;

    system->tape.unload();
}

auto Interface::controlTape(Media* media, TapeMode mode) -> void {

    system->tape.setMode( (Tape::Mode)mode );
}

auto Interface::getTapeControl(Media* media) -> TapeMode {
    
    if (!media)
        return TapeMode::Stop;
    
    return (TapeMode)system->tape.getMode();
}

auto Interface::getTapeListing(Media* media) -> std::vector<Emulator::Interface::Listing> {
    if (!media || !media->group->isTape())
        return {};

    return system->tape.getListing();
}

auto Interface::getTapePreview(uint8_t* data, unsigned size, Media* media) -> std::vector<Emulator::Interface::Listing> {
    TapeStructure structure(system->tape);
    structure.setData( data, size );

    return structure.getListing();
}

auto Interface::selectTapeListing(Media* media, unsigned pos, uint8_t options) -> void {

    system->tape.selectListing( pos, options );
}

auto Interface::createTapeImage(unsigned& imageSize) -> uint8_t* {
	return system->tape.createTap( imageSize );
}

auto Interface::insertExpansionImage(Media* media, uint8_t* data, unsigned size) -> void {
    
    auto group = media->group;
    
    if (!media || !group->isExpansion())
        return;        
    
    if (group->expansion->id == ExpansionIdGame)
        system->gameCart->setRom(media, data, size);
    else if (group->expansion->id == ExpansionIdReu)
        system->reu->setRom(media, data, size);
    else if (group->expansion->id == ExpansionIdFreezer)
        system->freezer->setRom(media, data, size);
    else if (group->expansion->id == ExpansionIdEasyFlash)
        system->easyFlash->setRom(media, data, size);
    else if (group->expansion->id == ExpansionIdEasyFlash3)
        system->easyFlash3->setRom(media, data, size);
    else if (group->expansion->id == ExpansionIdRetroReplay)
        system->retroReplay->setRom(media, data, size);
	else if (group->expansion->id == ExpansionIdGeoRam)
        system->geoRam->setRam(media, data, size);
    else if (group->expansion->id == ExpansionIdFastloader)
        system->fastloader->setRom(media, data, size);
    else if (group->expansion->id == ExpansionIdFinalChessCard)
        system->finalChessCard->setRom(media, data, size);
    else if (group->expansion->id == ExpansionIdSuperCpu)
        system->superCpu->setRom(media, data, size);
    else if (group->expansion->id == ExpansionIdExpert)
        system->expert->setRom(media, data, size);
    else if (group->expansion->id == ExpansionIdGmod2)
        system->gmod2->setRom(media, data, size);
}

auto Interface::writeProtectExpansion(Media* media, bool state) -> void {
    
    auto group = media->group;
    
    if (!media || !group->isExpansion())
        return;
    
    if (group->expansion->id == ExpansionIdEasyFlash) {
        if (system->easyFlash->media == media)
            system->easyFlash->setWriteProtect(state);

    } else if (group->expansion->id == ExpansionIdEasyFlash3) {
        system->easyFlash3->setWriteProtect( media, state );

    } else if (group->expansion->id == ExpansionIdRetroReplay) {
        if (system->retroReplay->media == media)
            system->retroReplay->setWriteProtect( state );
    } else if (group->expansion->id == ExpansionIdGeoRam) {
		if (system->geoRam->media == media)
            system->geoRam->setWriteProtect( state );
	} else if (group->expansion->id == ExpansionIdFinalChessCard) {
	    if (system->finalChessCard->mediaWrite == media)
	        system->finalChessCard->setWriteProtect( state );
	} else if (group->expansion->id == ExpansionIdGmod2) {
	    if (system->gmod2->media == media)
	        system->gmod2->setWriteProtect( state );
	    else if (system->gmod2->mediaSecondary == media)
	        system->gmod2->setSecondaryWriteProtect( state );
	}
}

auto Interface::isWriteProtectedExpansion(Media* media) -> bool {
    
    auto group = media->group;
    
    if (!media || !group->isExpansion())
        return false;
    
    if (group->expansion->id == ExpansionIdEasyFlash) {
        if (system->easyFlash->media == media)
            return system->easyFlash->isWriteProtected();

    } else if (group->expansion->id == ExpansionIdEasyFlash3) {
        return system->easyFlash3->isWriteProtected( media );

    } else if (group->expansion->id == ExpansionIdRetroReplay) {
        if (system->retroReplay->media == media)
            return system->retroReplay->isWriteProtected(  );
    } else if (group->expansion->id == ExpansionIdGeoRam) {
		if (system->geoRam->media == media)
			return system->geoRam->isWriteProtected();
    } else if (group->expansion->id == ExpansionIdFinalChessCard) {
        if (system->finalChessCard->mediaWrite == media)
            return system->finalChessCard->isWriteProtected();
    } else if (group->expansion->id == ExpansionIdGmod2) {
        if (system->gmod2->media == media)
            return system->gmod2->isWriteProtected();
        if (system->gmod2->mediaSecondary == media)
            return system->gmod2->isSecondaryWriteProtected();
    }

    return false;
}

auto Interface::ejectExpansionImage(Media* media) -> void {
    
    auto group = media->group;
    
    if (!media || !group->isExpansion())
        return;
    
    if (group->expansion->id == ExpansionIdGame)
        system->gameCart->setRom(media, nullptr, 0);
    else if (group->expansion->id == ExpansionIdReu) {
        system->reu->setRom(media, nullptr, 0);
    } else if (group->expansion->id == ExpansionIdFreezer)
        system->freezer->setRom(media, nullptr, 0);
    else if (group->expansion->id == ExpansionIdEasyFlash)
        system->easyFlash->setRom(media, nullptr, 0);
    else if (group->expansion->id == ExpansionIdEasyFlash3)
        system->easyFlash3->unsetRom(media);
    else if (group->expansion->id == ExpansionIdRetroReplay)
        system->retroReplay->setRom(media, nullptr, 0);
	else if (group->expansion->id == ExpansionIdGeoRam)
        system->geoRam->setRam( media, nullptr, 0 );
    else if (group->expansion->id == ExpansionIdRS232)
        system->acia->socket.disconnect();
    else if (group->expansion->id == ExpansionIdFastloader)
        system->fastloader->setRom(media, nullptr, 0);
    else if (group->expansion->id == ExpansionIdFinalChessCard)
        system->finalChessCard->setRom(media, nullptr, 0);
    else if (group->expansion->id == ExpansionIdSuperCpu)
        system->superCpu->setRom(media, nullptr, 0);
    else if (group->expansion->id == ExpansionIdExpert)
        system->expert->setRom(media, nullptr, 0);
    else if (group->expansion->id == ExpansionIdGmod2)
        system->gmod2->setRom(media, nullptr, 0);
}

auto Interface::createExpansionImage(Media* media, unsigned& imageSize) -> uint8_t* {
    auto group = media->group;

    if (!group->isExpansion())
        return nullptr;
    
    if (group->expansion->id == ExpansionIdEasyFlash)
        return system->easyFlash->createImage(imageSize);
    
    if (group->expansion->id == ExpansionIdRetroReplay)
        return system->retroReplay->createImage(media, imageSize);
	
	if (group->expansion->id == ExpansionIdGmod2)
		return system->gmod2->createImage(media, imageSize);
	
	if (group->expansion->id == ExpansionIdGeoRam)
		return system->geoRam->createImage( imageSize );

    if (group->expansion->id == ExpansionIdFinalChessCard)
        return system->finalChessCard->createImage(imageSize);
    
    return nullptr;
}

auto Interface::isExpansionBootable() -> bool {
    return system->expansionPort->isBootable();
}

auto Interface::insertProgram(Media* media, uint8_t* data, unsigned size) -> void {
		
    if (!media || !media->group->isProgram())
        return;
    
    auto prg = system->getPrgInstance( media );
    if (!prg)
        return;
    
    system->prgInUse = prg;
    prg->set( data, size );
}

auto Interface::ejectProgram(Media* media) -> void {
//    if (!media || !media->group->isProgram())
//        return;
//
//    auto prg = Prg::getInstance( media );
//    if (!prg)
//        return;
//    prg->unset();
}

auto Interface::getLoadedProgram(unsigned& size) -> uint8_t* {
	
	return Prg::getMemory( size, system->ram );
}

auto Interface::getProgramListing(Media* media) -> std::vector<Emulator::Interface::Listing> {
    if (!media || !media->group->isProgram())
        return {};
    
    auto prg = system->getPrgInstance( media );
    if (!prg)
        return {};
        
	return prg->getListing();
}

auto Interface::getProgramPreview(uint8_t* data, unsigned size) -> std::vector<Emulator::Interface::Listing> {
    Prg prg(system);
    prg.set(data, size);
    return prg.getListing();
}

auto Interface::selectProgramListing(Media* media, unsigned pos) -> bool {
    if (!media || !media->group->isProgram())
        return false;
    
    // c64 memory
    auto prg = system->getPrgInstance( media );
    if (!prg)
        return false;
    system->prgInUse = prg;
    return prg->select( pos );
}

auto Interface::convertPetsciiToScreencode(bool state) -> void {
    system->convertToScreencode = state;
}

auto Interface::loadWithColumn(bool state) -> void {
    system->loadWithColumn = state;
}

auto Interface::savestate(unsigned& size) -> uint8_t* {
	return system->serialize( size );
}

auto Interface::checkstate(uint8_t* data, unsigned size) -> bool { 
    return system->checkSerialization( data, size );
}    

auto Interface::loadstate(uint8_t* data, unsigned size) -> bool {
	return system->unserialize( data, size );
}

auto Interface::setFirmware(unsigned typeId, uint8_t* data, unsigned size, bool allowPatching) -> void {
	if (typeId >= firmwares.size()) return;
    
    system->setFirmware( typeId, data, size, allowPatching );
}

auto Interface::setModelValue(unsigned modelId, int value) -> void {
    switch (modelId) {
		case ModelIdSid:
            system->sidManager.setType( 0, (value & 1) ? Sid::Type::MOS_6581 : Sid::Type::MOS_8580 );
            break;
        case ModelIdSidEngine:
            system->sidManager.setEngineAll( (uint8_t)value );
            break;
        case ModelIdFilter:
            system->sidManager.setEnableFilterAll( value & 1 );
            break;
		case ModelIdDigiboost:
            system->sidManager.setDigiBoostAll( value & 1 );
			break;
        case ModelIdIntensifyPseudoStereo:
            system->sidManager.intensifyPseudoStereo( value & 1);
            break;
        case ModelIdBias6581:
            system->sidManager.adjustFilterCurve6581All( value );
			break;
        case ModelIdBias8580:
            system->sidManager.adjustFilterCurve8580All( value );
			break;
        case ModelIdRange6581:
            system->sidManager.adjustFilterRange6581All( value );
            break;
        case ModelIdSidWaveformStrength:
            system->sidManager.setWaveformStrength( (uint8_t)value );
            break;
        case ModelIdSidSampleFetch:
            system->sidManager.setResampleQuality( (uint8_t)value );
            system->updateStats();
            break;
        case ModelIdCiaRev:
            system->cia1.setNewVersion( value & 1 );
            system->cia2.setNewVersion( value & 1 );
            break;
        case ModelIdGlueLogic:
            system->glueLogic.setType( (GlueLogic::Type)(value & 1) );
            break;
		case ModelIdVicIIModel:
            system->vicIIFast.setModel( (VicIIBase::Model)value );
            system->vicIICycle.setModel( (VicIIBase::Model)value );
			system->updateStats();
			break;
		case ModelIdDisableGreyDotBug:
            system->vicIICycle.disableGreyDotBug( value & 1 );
			break;            
        case ModelIdSidMulti:
            system->useExtraSids( value & 7 );
            break;
        case ModelIdSid1Adr: system->sidManager.setIoMask( 0, value ); break;
        case ModelIdSid2Adr: system->sidManager.setIoMask( 1, value ); break;
        case ModelIdSid3Adr: system->sidManager.setIoMask( 2, value ); break;
        case ModelIdSid4Adr: system->sidManager.setIoMask( 3, value ); break;
        case ModelIdSid5Adr: system->sidManager.setIoMask( 4, value ); break;
        case ModelIdSid6Adr: system->sidManager.setIoMask( 5, value ); break;
        case ModelIdSid7Adr: system->sidManager.setIoMask( 6, value ); break;
        case ModelIdSid8Adr: system->sidManager.setIoMask( 7, value ); break;
            
        case ModelIdSid1Left: system->sidManager.useLeftChannel( 0, value & 1 ); break;
        case ModelIdSid2Left: system->sidManager.useLeftChannel( 1, value & 1 ); break;
        case ModelIdSid3Left: system->sidManager.useLeftChannel( 2, value & 1 ); break;
        case ModelIdSid4Left: system->sidManager.useLeftChannel( 3, value & 1 ); break;
        case ModelIdSid5Left: system->sidManager.useLeftChannel( 4, value & 1 ); break;
        case ModelIdSid6Left: system->sidManager.useLeftChannel( 5, value & 1 ); break;
        case ModelIdSid7Left: system->sidManager.useLeftChannel( 6, value & 1 ); break;
        case ModelIdSid8Left: system->sidManager.useLeftChannel( 7, value & 1 ); break;
        
        case ModelIdSid1Right: system->sidManager.useRightChannel( 0, value & 1 ); break;
        case ModelIdSid2Right: system->sidManager.useRightChannel( 1, value & 1 ); break;
        case ModelIdSid3Right: system->sidManager.useRightChannel( 2, value & 1 ); break;
        case ModelIdSid4Right: system->sidManager.useRightChannel( 3, value & 1 ); break;
        case ModelIdSid5Right: system->sidManager.useRightChannel( 4, value & 1 ); break;
        case ModelIdSid6Right: system->sidManager.useRightChannel( 5, value & 1 ); break;
        case ModelIdSid7Right: system->sidManager.useRightChannel( 6, value & 1 ); break;
        case ModelIdSid8Right: system->sidManager.useRightChannel( 7, value & 1 ); break;
        
        case ModelIdSid2: system->sidManager.setType( 1, (value & 1) ? Sid::Type::MOS_6581 : Sid::Type::MOS_8580 ); break;
        case ModelIdSid3: system->sidManager.setType( 2, (value & 1) ? Sid::Type::MOS_6581 : Sid::Type::MOS_8580 ); break;
        case ModelIdSid4: system->sidManager.setType( 3, (value & 1) ? Sid::Type::MOS_6581 : Sid::Type::MOS_8580 ); break;
        case ModelIdSid5: system->sidManager.setType( 4, (value & 1) ? Sid::Type::MOS_6581 : Sid::Type::MOS_8580 ); break;
        case ModelIdSid6: system->sidManager.setType( 5, (value & 1) ? Sid::Type::MOS_6581 : Sid::Type::MOS_8580 ); break;
        case ModelIdSid7: system->sidManager.setType( 6, (value & 1) ? Sid::Type::MOS_6581 : Sid::Type::MOS_8580 ); break;
        case ModelIdSid8: system->sidManager.setType( 7, (value & 1) ? Sid::Type::MOS_6581 : Sid::Type::MOS_8580 ); break;

        case ModelIdDiskDriveModel:
            system->iecBus.setDriveType( Drive::Type(value) );
            break;
        case ModelIdDiskDrivesConnected:
            system->iecBus.setDrivesEnabled( value );
            system->burstOrParallelUpdate();
            break;
        case ModelIdTapeDrivesConnected:
            system->tape.setEnabled( value & 1 );
            break;
        case ModelIdTapeDriveWobble:
            system->tape.setWobble( value & 1 );
            break;
        case ModelIdDiskDriveWobble:
            system->iecBus.setDriveWobble( value );
            break;
        case ModelIdDriveStepperDelay:
            system->iecBus.setStepperSeekTime( value );
            break;
        case ModelIdDiskDriveSpeed:
            system->iecBus.setDriveSpeed( value );
            break;
        case ModelIdCiaBurstMode:
            system->secondDriveCable.burstRequested = value & 1;
            system->burstOrParallelUpdate();
            break;
        case ModelIdDriveParallelCable:
            system->secondDriveCable.parallelRequested = value & 1;
            system->burstOrParallelUpdate();
            break;
        case ModelIdDriveFastLoader:
            system->iecBus.setSpeeder( value );
            break;
        case ModelIdEmulateDriveMechanics:
            system->iecBus.enableMechanics(value & 1);
            break;
        case ModelIdDriveAcceleration:
            system->iecBus.setMotorAcceleration(value);
            break;
        case ModelIdDriveDeceleration:
            system->iecBus.setMotorDeceleration(value);
            break;
        case ModelIdDriveRam20To3F:
            system->iecBus.setExpandedMemory( Drive::ExpandedMemMode::M20, value & 1 );
            break;
        case ModelIdDriveRam40To5F:
            system->iecBus.setExpandedMemory( Drive::ExpandedMemMode::M40, value & 1 );
            break;
        case ModelIdDriveRam60To7F:
            system->iecBus.setExpandedMemory( Drive::ExpandedMemMode::M60, value & 1 );
            break;
        case ModelIdDriveRam80To9F:
            system->iecBus.setExpandedMemory( Drive::ExpandedMemMode::M80, value & 1 );
            break;
        case ModelIdDriveRamA0ToBF:
            system->iecBus.setExpandedMemory( Drive::ExpandedMemMode::MA0, value & 1 );
            break;
        case ModelIdTrackZeroSensor:
            system->iecBus.setTrackZeroSensor(value & 1);
            break;
        case ModelIdCycleAccurateVideo:
            system->cycleRendererNextBoot = value & 1;
            break;
        case ModelIdDiskThread:
            system->iecBus.setThread( value & 1 );
            break;
        case ModelIdDiskOnDemand:
            system->diskSilence.active = value & 1;
            system->diskIdleOff();
            break;
        case ModelIdReuRam:
            system->reu->setRamSize( value );
            break;
        case ModelIdGeoRam:
            system->geoRam->setRamSize( value );
            break;
        case ModelIdSuperCpuRam:
            system->superCpu->setRamSize( value );
            break;
        case ModelId2Mhz:
            system->set2Mhz(value & 1);
            break;
        case ModelIdSidUsbPico:
            system->sidManager.enableUSBSID(value & 1);
            break;
        case ModelIdSidUsbPicoBufferSize:
            system->sidManager.setUSBSIDBuffSize(value);
            break;
        case ModelIdSidUsbPicoDiffSize:
            system->sidManager.setUSBSIDDiffSize(value);
            break;
        case ModelIdDisableSpriteCollisions:
            system->vicIICycle.disableSpriteSpriteCollisions(value & 1);
            system->vicIIFast.disableSpriteSpriteCollisions(value & 1);
            break;

        case ModelIdDisableSpriteCharacterCollisions:
            system->vicIICycle.disableSpriteForegroundCollisions(value & 1);
            system->vicIIFast.disableSpriteForegroundCollisions(value & 1);
            break;
    }
}

auto Interface::getModelValue(unsigned modelId) -> int {
    
    switch (modelId) {
		case ModelIdSid:			
            return (system->sidManager.getType(0) == Sid::Type::MOS_6581) ? 1 : 0;
        case ModelIdSidEngine:
            return (int)system->sidManager.getEngine();
        case ModelIdFilter:
            return system->sidManager.isFilterEnabled();
		case ModelIdDigiboost:
            return system->sidManager.getDigiBoost();
        case ModelIdIntensifyPseudoStereo:
            return system->sidManager.hasIntensifiedPseudoStereo();
        case ModelIdBias6581:
			return system->sidManager.getFilterCurve6581();
        case ModelIdBias8580:
            return system->sidManager.getFilterCurve8580();
        case ModelIdRange6581:
            return system->sidManager.getFilterRange6581();
        case ModelIdSidWaveformStrength:
            return system->sidManager.getWaveformStrength();
        case ModelIdSidSampleFetch:
            return system->sidManager.getResampleQuality();
        case ModelIdCiaRev:
            return system->cia1.isNewVersion();
        case ModelIdGlueLogic:
            return (int)system->glueLogic.type;
		case ModelIdVicIIModel:
			return (int)system->vicII->getModel();
		case ModelIdDisableGreyDotBug:
			return system->vicIICycle.hasGreyDotBugDisbled();
        case ModelIdSidMulti:
            return (int)system->requestedSids;
            
        case ModelIdSid1Adr: return system->sidManager.getIoPos(0);
        case ModelIdSid2Adr: return system->sidManager.getIoPos(1);
        case ModelIdSid3Adr: return system->sidManager.getIoPos(2);
        case ModelIdSid4Adr: return system->sidManager.getIoPos(3);
        case ModelIdSid5Adr: return system->sidManager.getIoPos(4);
        case ModelIdSid6Adr: return system->sidManager.getIoPos(5);
        case ModelIdSid7Adr: return system->sidManager.getIoPos(6);
        case ModelIdSid8Adr: return system->sidManager.getIoPos(7);
        
        case ModelIdSid1Left: return (int)system->sidManager.hasLeftChannel(0);
        case ModelIdSid2Left: return (int)system->sidManager.hasLeftChannel(1);
        case ModelIdSid3Left: return (int)system->sidManager.hasLeftChannel(2);
        case ModelIdSid4Left: return (int)system->sidManager.hasLeftChannel(3);
        case ModelIdSid5Left: return (int)system->sidManager.hasLeftChannel(4);
        case ModelIdSid6Left: return (int)system->sidManager.hasLeftChannel(5);
        case ModelIdSid7Left: return (int)system->sidManager.hasLeftChannel(6);
        case ModelIdSid8Left: return (int)system->sidManager.hasLeftChannel(7);
        
        case ModelIdSid1Right: return (int)system->sidManager.hasRightChannel(0);
        case ModelIdSid2Right: return (int)system->sidManager.hasRightChannel(1);
        case ModelIdSid3Right: return (int)system->sidManager.hasRightChannel(2);
        case ModelIdSid4Right: return (int)system->sidManager.hasRightChannel(3);
        case ModelIdSid5Right: return (int)system->sidManager.hasRightChannel(4);
        case ModelIdSid6Right: return (int)system->sidManager.hasRightChannel(5);
        case ModelIdSid7Right: return (int)system->sidManager.hasRightChannel(6);
        case ModelIdSid8Right: return (int)system->sidManager.hasRightChannel(7);
        
        case ModelIdSid2: return system->sidManager.getType(1) == Sid::Type::MOS_6581 ? 1 : 0;
        case ModelIdSid3: return system->sidManager.getType(2) == Sid::Type::MOS_6581 ? 1 : 0;
        case ModelIdSid4: return system->sidManager.getType(3) == Sid::Type::MOS_6581 ? 1 : 0;
        case ModelIdSid5: return system->sidManager.getType(4) == Sid::Type::MOS_6581 ? 1 : 0;
        case ModelIdSid6: return system->sidManager.getType(5) == Sid::Type::MOS_6581 ? 1 : 0;
        case ModelIdSid7: return system->sidManager.getType(6) == Sid::Type::MOS_6581 ? 1 : 0;
        case ModelIdSid8: return system->sidManager.getType(7) == Sid::Type::MOS_6581 ? 1 : 0;

        case ModelIdDiskDriveModel:         return (int)Drive::globalType;
        case ModelIdDiskDrivesConnected:    return system->iecBus.drivesConnected;
        case ModelIdTapeDrivesConnected:    return system->tape.isEnabled() ? 1 : 0;
        case ModelIdTapeDriveWobble:        return system->tape.hasWobble() ? 1 : 0;
        case ModelIdDiskDriveWobble:        return (int)system->iecBus.getDriveWobble();
        case ModelIdDiskDriveSpeed:         return (int)system->iecBus.getDriveSpeed();
        case ModelIdDriveStepperDelay:      return (int)(system->iecBus.getStepperSeekTime());

        case ModelIdCiaBurstMode:           return system->secondDriveCable.burstRequested;
        case ModelIdDriveParallelCable:     return system->secondDriveCable.parallelRequested;
        case ModelIdDriveFastLoader:        return (int)system->iecBus.drives[0]->speeder;
        case ModelIdEmulateDriveMechanics:  return system->iecBus.hasMechanics() ? 1 : 0;
        case ModelIdDriveAcceleration:      return system->iecBus.getMotorAcceleration();
        case ModelIdDriveDeceleration:      return system->iecBus.getMotorDeceleration();

        case ModelIdCycleAccurateVideo:     return system->cycleRendererNextBoot;
        case ModelIdDiskThread:             return (int)system->iecBus.useThread;
        case ModelIdDiskOnDemand:           return system->diskSilence.active;

        case ModelIdDriveRam20To3F:         return (int)system->iecBus.getExpandedMemory(Drive::ExpandedMemMode::M20);
        case ModelIdDriveRam40To5F:         return (int)system->iecBus.getExpandedMemory(Drive::ExpandedMemMode::M40);
        case ModelIdDriveRam60To7F:         return (int)system->iecBus.getExpandedMemory(Drive::ExpandedMemMode::M60);
        case ModelIdDriveRam80To9F:         return (int)system->iecBus.getExpandedMemory(Drive::ExpandedMemMode::M80);
        case ModelIdDriveRamA0ToBF:         return (int)system->iecBus.getExpandedMemory(Drive::ExpandedMemMode::MA0);
        case ModelIdTrackZeroSensor:        return (int)system->iecBus.hasTrackZeroSensor();
        case ModelIdReuRam:                 return (int)system->reu->getRamSize();
        case ModelIdGeoRam:                 return (int)system->geoRam->getRamSize();
        case ModelIdSuperCpuRam:            return (int)system->superCpu->getRamSize();
        case ModelId2Mhz:					return (int)system->mhz2 & 1;
        case ModelIdSidUsbPico:             return system->sidManager.hasUSBSID();
        case ModelIdSidUsbPicoBufferSize:   return system->sidManager.getUSBSIDBuffSize();
        case ModelIdSidUsbPicoDiffSize:     return system->sidManager.getUSBSIDDiffSize();
        case ModelIdDisableSpriteCollisions: return system->vicIICycle.collisionsSpiteSpriteDisabled();
        case ModelIdDisableSpriteCharacterCollisions: return system->vicIICycle.collisionsSpiteForegroundDisabled();
    }
    return 0;
}

auto Interface::cropFrame( CropType type, Crop crop ) -> void {
    system->cropFrame(type, crop);
}

auto Interface::cropWidth() -> unsigned {
    
    return system->crop->latest.width;
}

auto Interface::cropHeight() -> unsigned {
    
    return system->crop->latest.height;
}

auto Interface::cropTop() -> unsigned {
    
    return system->crop->latest.top;
}

auto Interface::cropLeft() -> unsigned {
    
    return system->crop->latest.left;
}

auto Interface::cropCoordUpdated(unsigned& top, unsigned& left) -> bool {
    auto& latest = system->crop->latest;
    top = latest.top;
    left = latest.left;
    return latest.topLeftChanged;
}

auto Interface::cropData() -> uint8_t* {
    return system->crop->latest.frame;
}

auto Interface::cropPitch() -> unsigned {
    return system->crop->latest.linePitch;
}

auto Interface::cropAlternatively(unsigned& width, unsigned& height, unsigned& pitch) -> uint8_t* {
    return system->crop->cropAlternatively(width, height, pitch);
}

auto Interface::setInputSampling(uint8_t mode) -> void {
    system->input.setSampling( mode );
}

auto Interface::enableFloppySounds(bool state) -> void {
    system->setFloppySounds( state );
}

auto Interface::enableTapeSounds(bool state) -> void {
    system->setTapeSounds( state );
}

auto Interface::setTapeLoadingNoise(unsigned volume) -> void {
    system->setTapeLoadingNoise( volume );
}

auto Interface::activateDebugCart( unsigned limitCycles ) -> void {
    system->activateDebugCart( limitCycles );
}

auto Interface::setWarpMode(unsigned config) -> void {
    system->setWarpMode( config );
}

auto Interface::setMemoryInitParams(MemoryPattern& pattern) -> void {

    system->memoryInit = pattern;
}

auto Interface::getMemoryInitPattern( uint8_t* pattern ) -> void {
    
    system->initRam( pattern );
}

auto Interface::setExpansion(unsigned expansionId) -> void {
    
    auto expansion = getExpansionById( expansionId );
    
    if (!expansion)
        return;
    
    system->setExpansion( (ExpansionId)expansion->id );
}

auto Interface::unsetExpansion() -> void {
    
    system->setExpansion( ExpansionIdNone );
}

auto Interface::isExpansionUnsupported() -> bool {
    return system->isExpansionUnsupported();
}

auto Interface::getExpansion() -> Expansion* {

    return getExpansionById( system->expansionPort->id );
}

auto Interface::setExpansionJumper( Media* media, unsigned jumperId, bool state ) -> void {
    auto group = media->group;
    
    if (!media || !group->isExpansion())
        return;
    
    if (group->expansion->id == ExpansionIdEasyFlash) {
        if (system->easyFlash->media == media)
            system->easyFlash->setJumper( jumperId, state );
    } else if (group->expansion->id == ExpansionIdRetroReplay) {        
        if (system->retroReplay->media == media)
            system->retroReplay->setJumper( jumperId, state );
    } else if (group->expansion->id == ExpansionIdRS232) {
        if (system->acia->media == media)
            system->acia->setJumper( jumperId, state );
    } else if (group->expansion->id == ExpansionIdFastloader) {
        if (system->fastloader->media == media)
            system->fastloader->setJumper( jumperId, state );
    } else if (group->expansion->id == ExpansionIdFinalChessCard) {
        if (system->finalChessCard->media == media)
            system->finalChessCard->setJumper( jumperId, state );
    } else if (group->expansion->id == ExpansionIdSuperCpu) {
        if (system->superCpu->media == media)
            system->superCpu->setJumper( jumperId, state );
    } else if (group->expansion->id == ExpansionIdExpert) {
        if (system->expert->media == media)
            system->expert->setJumper(jumperId, state);
    }
}

auto Interface::getExpansionJumper( Media* media, unsigned jumperId ) -> bool {
    auto group = media->group;
    
    if (!media || !group->isExpansion())
        return false;

    if (group->expansion->id == ExpansionIdEasyFlash)
        return system->easyFlash->getJumper(jumperId);

    else if (group->expansion->id == ExpansionIdRetroReplay)
        return system->retroReplay->getJumper(jumperId);

    else if (group->expansion->id == ExpansionIdRS232)
        return system->acia->getJumper(jumperId);

    else if (group->expansion->id == ExpansionIdFastloader)
        return system->fastloader->getJumper(jumperId);

    else if (group->expansion->id == ExpansionIdFinalChessCard)
        return system->finalChessCard->getJumper(jumperId);

    else if (group->expansion->id == ExpansionIdSuperCpu)
        return system->superCpu->getJumper(jumperId);

    else if (group->expansion->id == ExpansionIdExpert)
        return system->expert->getJumper(jumperId);

    return false;
}

auto Interface::getExpansionPreview(uint8_t* data, unsigned size) -> std::vector<Listing> {
    Cart cart(system);
    cart.rom = data;
    cart.romSize = size;
    if (!cart.readHeader())
        return {};
        
    cart.readChips();
    return cart.getListing();
}

auto Interface::hasFreezeButton() -> bool {
    return system->expansionPort->hasFreezeButton();
}

auto Interface::freezeButton() -> void {
    system->expansionPort->freeze();
}

auto Interface::hasCustomCartridgeButton() -> bool {
    return system->expansionPort->hasCustomButton();
}

auto Interface::customCartridgeButton() -> void {
    system->expansionPort->customButton();
}

auto Interface::analyzeExpansion(uint8_t* data, unsigned size, std::string suffix) -> Expansion* {
    
    return system->analyzeExpansion( data, size, suffix );
}

auto Interface::videoAddMeta(bool state) -> void {
    system->vicIIFast.setMeta( state );
}

auto Interface::setMonitorFpsRatio(double ratio) -> void {
    system->sidManager.updateChamberlinFrequencyAll( system->vicII->frequency() * ratio );

    system->hintSlowSpeed( ratio < 0.5 );
}

auto Interface::pasteText( std::string buffer ) -> void {

    system->pasteText( buffer );
}

auto Interface::copyText() -> std::string {

    return system->copyText( );
}

auto Interface::requestImmediateReturn() -> void {
    system->leaveEmulation = true;
}

auto Interface::prepareSocket( Media* media, std::string address, std::string port ) -> void {

    system->acia->prepareSocket( media, address, port );
}

auto Interface::getModelIdOfEnabledDrives(MediaGroup* group) -> unsigned {
    if (group) {
        if (group->isDisk())
            return ModelIdDiskDrivesConnected;

        if (group->isTape())
            return ModelIdTapeDrivesConnected;
    }
    return ~0;
}

auto Interface::getModelIdOfCycleRenderer() -> unsigned {
    return ModelIdCycleAccurateVideo;
}

auto Interface::autoStartedByMediaGroup() -> MediaGroup* {
    if (system->iecBus.wasAutostarted())
        return getDiskMediaGroup();

    if (system->tape.wasAutostarted())
        return getTapeMediaGroup();

    return nullptr;
}

auto Interface::toggle2Mhz() -> bool {
    return system->toggle2Mhz();
}

auto Interface::configRewind(unsigned steps, unsigned maxSizeInMb) -> void {
    system->history.config(steps, maxSizeInMb);
}

auto Interface::setRewind(bool state) -> void {
    if (system->powerOn)
        system->history.setRewind(state);
}

auto Interface::disassemble(DebuggerTheme theme, unsigned addr, unsigned& bytes) -> std::string {
    return system->disassemble( theme, addr, bytes);
}

auto Interface::disassembleData(DebuggerTheme theme, unsigned addr, unsigned bytes) -> std::string {
    return system->disassembleData( theme, (uint16_t)addr, bytes );
}

auto Interface::disassembleTrace(DebuggerTheme theme, unsigned i, uint16_t& flags) -> std::string {
    return system->disassembleTrace( theme, i, (uint8_t&)flags );
}

auto Interface::getDmaDump() -> uint8_t* {
    return system->vicII->debugger.dmaFrame;
}

auto Interface::editMemory(DebuggerTheme theme, uint32_t addr, std::vector<uint16_t> values) -> void {
    system->editMemory(theme, addr, values);
}

auto Interface::debuggerAdd(DebuggerTheme theme, DebuggerAction action, unsigned ident, unsigned data0, unsigned data1) -> void {
    system->debuggerAdd( theme, action, ident, data0, data1 );
}

auto Interface::debuggerRemove(DebuggerTheme theme, DebuggerAction action, std::optional<unsigned> ident) -> void {
    system->debuggerRemove( theme, action, ident );
}

auto Interface::setWatchpointCondition(DebuggerTheme theme, DebuggerAction action, unsigned ident, unsigned hitCount, unsigned hitCountMode, const std::string& expression, unsigned expressionMode) -> bool {
    return system->setWatchpointCondition(theme, action, ident, hitCount, hitCountMode, expression, expressionMode);
}

auto Interface::debuggerStepOver(DebuggerTheme theme, bool subroutineOnly) -> void {
    system->debuggerStepOver(theme, subroutineOnly);
}

auto Interface::debuggerStepInto(DebuggerTheme theme) -> void {
    system->debuggerStepInto(theme);
}

auto Interface::debuggerStepOut(DebuggerTheme theme) -> bool {
    return system->debuggerStepOut(theme);
}

auto Interface::getMemoryDumpBank(uint8_t bank, uint8_t* dump) -> void {
    system->getMemoryDumpBank(bank, dump);
}

auto Interface::getMemoryDumpPage(DebuggerTheme theme, uint8_t page, uint8_t* dump) -> void {
    system->getMemoryDumpPage(theme, page, dump);
}

auto Interface::getMemory(DebuggerTheme theme, DebuggerAction action, unsigned startAddr, unsigned endAddr, uint8_t* dump) -> void {
    system->getMemory(theme, action, startAddr, endAddr, dump);
}

}
