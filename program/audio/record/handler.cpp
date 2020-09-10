
#include "handler.h"
#include "wavWriter.h"
#include "../../tools/chronos.h"
#include "../../program.h"
#include "../../tools/logger.h"
#include "../../emuconfig/config.h"

namespace AudioRecord {
    
auto Handler::record( Emulator::Interface* emulator, std::string& errorText ) -> bool {

    finish();
    
    if (emulator != activeEmulator) {
        errorText = trans->get("no emulation active");
        return false;     
    }

    std::string path = settings->get<std::string>( ident("audio_record_path"), "");
    
    if (path.empty()) {
        errorText = trans->get("no save location");
        return false;
    }
    
    std::string fileName = settings->get<std::string>( ident("record_ident"), "sample");
        
    sampleRate = settings->get<unsigned>("audio_frequency_v2", 48000);

    framesFlush = 0;

    framesTimeCheck = 0;		
    
    useFloat = audioDriver->expectFloatingPoint();
    
    std::string filePath = path + fileName + "_" + std::to_string( Chronos::getTimestampInSeconds() ) + ".wav";
    
    wavWriter = new WavWriter;

    if ( !wavWriter->create( filePath ) ) {

        delete wavWriter;

        wavWriter = nullptr;
        
        errorText = trans->get("file_creation_error", {{"%path%", filePath}});

        return false;
    }

    wavWriter->writeHeader( sampleRate, useFloat );    

    startTime = Chronos::getTimestampInMilliseconds();
    
    setTimeLimit();
    
    return true;
}

auto Handler::setTimeLimit() -> void {

    timeLimit = 0;	
    
    if (!activeEmulator)
        return;

    if ( settings->get<bool>( ident("audio_record_timelimit"), false) ) {

        unsigned min = settings->get<unsigned>( ident("audio_record_minutes"), 0, {0, 120});

        unsigned sec = settings->get<unsigned>( ident("audio_record_seconds"), 0, {0, 59});

        timeLimit = min * 60 + sec;

        timeLimit *= 1000;
    }
}	

inline auto Handler::run() -> bool {

    return wavWriter != nullptr;
}

auto Handler::write( uint8_t* buf, unsigned frames ) -> void {

    if (!run())
        return;

    wavWriter->write( buf, frames << (useFloat ? 3 : 2) );

    framesFlush += frames;			

    if (framesFlush >= (sampleRate << 1) ) {

        wavWriter->flush();

        framesFlush = 0;
    }	

    if (timeLimit) {
        framesTimeCheck += frames;

        if (framesTimeCheck >= (sampleRate >> 2) ) {

            checkTime();

            framesTimeCheck = 0;
        }	
    }
}

auto Handler::checkTime() -> void {

    unsigned curTime = Chronos::getTimestampInMilliseconds();

    if (timeLimit > (curTime - startTime) )
        return;

    finish();        
}

auto Handler::finish() -> void {

    if (!wavWriter)
        return;

    wavWriter->finish();

    delete wavWriter;

    wavWriter = nullptr;
    
    if (activeEmulator)
        EmuConfigView::TabWindow::getView(activeEmulator)->miscLayout->stopRecord();
}  

auto Handler::ident( std::string name ) -> std::string {
    
    return program->ident( activeEmulator, name );
}
    
}