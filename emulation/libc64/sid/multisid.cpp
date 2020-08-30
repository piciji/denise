
#include "sid.h"

namespace LIBC64 {

std::vector<Sid*> Sid::useSids;
bool Sid::extraSids = false;
double Sid::leftSids = 0;
double Sid::rightSids = 0;    
    
std::vector<std::string> Sid::adrOptions = {"D400", "D420", "D440", "D460", "D480", "D4A0", "D4C0", "D4E0", "D500", "D520", "D540", "D560", "D580",
    "D5A0", "D5C0", "D5E0", "D600", "D620", "D640", "D660", "D680", "D6A0", "D6C0", "D6E0", "D700", "D720", "D740", "D760", "D780",
    "D7A0", "D7C0", "D7E0", "DE00", "DE20", "DE40", "DE60", "DE80", "DEA0", "DEC0", "DEE0", "DF00", "DF20", "DF40", "DF60", "DF80", "DFA0", "DFC0", "DFE0"};


auto Sid::clockMultiChips() -> void {
    
    static double sampleLeft, sampleRight;
    
    sampleLeft = sampleRight = 0.0;

    if (audioOut && (++sampleCounter == sampleLimit)) {
      
        sampleCounter = 0;
                
        for (auto useSid : useSids) {

            useSid->clock();
            
            if (useSid->leftChannel)
                sampleLeft += useSid->curSample;

            if (useSid->rightChannel)
                sampleRight += useSid->curSample;                
        }

        if (!leftSids) {
            sampleRight /= rightSids;
            sampleLeft = sampleRight;
            audioRefresh( Emulator::sclamp( 16, sampleLeft) );

        } else if (!rightSids) {
            sampleLeft /= leftSids;
            sampleRight = sampleLeft;
            audioRefresh( Emulator::sclamp( 16, sampleLeft) );

        } else {            
            sampleLeft /= leftSids;
            sampleRight /= rightSids;
            audioRefreshStereo( Emulator::sclamp( 16, sampleLeft), Emulator::sclamp( 16, sampleRight) );
        }                   
        
    } else {
        for (auto useSid : useSids)
            useSid->clock();
    }
}

auto Sid::getSidByAdr(uint16_t addr, bool ioArea) -> Sid* {
    
    addr &= 0xffe0;
    
    for (auto useSid : useSids) {
        if (addr == useSid->ioMask)
            return useSid;        
    }
    
    return ioArea ? nullptr : sid;
}

auto Sid::updateSidUsage() -> void {
    
    leftSids = rightSids = 0;
    useSids.clear();
    
    extraSids = system->extraSids;
    
    if (!extraSids) {
        system->updateStatsStereo();
        return;
    }

    if (sid->leftChannel || sid->rightChannel)
        useSids.push_back(sid);

    if (sid->leftChannel)
        leftSids++;

    if (sid->rightChannel)
        rightSids++;
    
    for (unsigned i = 0; i < 7; i++) {
        
        Sid* extraSid = sids[i];
           
        if (extraSid->leftChannel || extraSid->rightChannel)
            useSids.push_back( extraSid );
            
        if (extraSid->leftChannel)
            leftSids++;
        
        if (extraSid->rightChannel)
            rightSids++;
    }
    
    extraSids = !useSids.empty();
    
    system->updateStatsStereo();
}

auto Sid::isStereo() -> bool {
    
    if (!extraSids)
        return false;
    
    if (!leftSids || !rightSids)
        return false;
    
    for (auto useSid : useSids) {
        
        if (!useSid->leftChannel || !useSid->rightChannel)
            return true;
    }
    
    return false;
}

auto Sid::setIoMask( uint8_t pos ) -> void {
    
    if (pos >= adrOptions.size())
        return;
    
    ioPos = pos;
    
    auto str = adrOptions[pos];
    
    ioMask = std::stoul(str, nullptr, 16);
}

auto Sid::setEnableFilterAll( bool state ) -> void {
    sid->filter.setEnable( state );
    
    for (unsigned i = 0; i < 7; i++)
        sids[i]->filter.setEnable( state );
}

auto Sid::setFilterTypeAll( FilterType filterType ) -> void {
    sid->setFilterType(filterType);
    
    for (unsigned i = 0; i < 7; i++)
        sids[i]->setFilterType( filterType );
}

auto Sid::setTypeAll( Type type ) -> void {
    sid->setType( type );
    
    for (unsigned i = 0; i < 7; i++) 
        sids[i]->setType( type );
}

auto Sid::updateChamberlinFrequencyAll(double sampleRate) -> void {

    sid->chamberlinFilter.updateFrequency( sampleRate );

    for (unsigned i = 0; i < 7; i++)
        sids[i]->chamberlinFilter.updateFrequency( sampleRate );
}

auto Sid::adjustFilterBias6581All(int value) -> void {
    sid->filter.adjustFilterBias6581( value );
    
    for (unsigned i = 0; i < 7; i++)
        sids[i]->filter.adjustFilterBias6581( value );
}

auto Sid::adjustFilterBias8580All(int value) -> void {
    sid->filter.adjustFilterBias8580( value );
    
    for (unsigned i = 0; i < 7; i++)
        sids[i]->filter.adjustFilterBias8580( value );    
}

auto Sid::setDigiBoostAll( bool state ) -> void {
    sid->setDigiBoost( state );
    
    for (unsigned i = 0; i < 7; i++)
        sids[i]->setDigiBoost( state );
}

auto Sid::resetAll() -> void {
    sid->reset();

    for (unsigned i = 0; i < 7; i++)
        sids[i]->reset();
}



}
