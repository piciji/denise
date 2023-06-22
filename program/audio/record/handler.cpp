
#include "handler.h"
#include "wavWriter.h"
#include "../../tools/chronos.h"
#include "../../program.h"
#include "../../tools/logger.h"
#include "../../emuconfig/config.h"
#include "../../view/status.h"
#include "../../thread/emuThread.h"

namespace AudioRecord {
    
auto Handler::start( Emulator::Interface* emulator, std::string& errorText ) -> bool {

    finish();
    
    if (emulator != activeEmulator) {
        errorText = trans->get("no emulation active");
        return false;     
    }
    
    GUIKIT::Settings* settings = program->getSettings( activeEmulator );

    std::string path = settings->get<std::string>( "audio_record_path", "");
    
    if (path.empty()) {
        errorText = trans->get("no save location");
        return false;
    }
    
    std::string fileName = settings->get<std::string>( "record_ident", "sample");

    sampleRate = audioDriver->getFrequency();

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

    statusHandler->updateAudioRecord( true );
    
    return true;
}

auto Handler::setTimeLimit() -> void {

    timeLimit = 0;	
    
    if (!activeEmulator)
        return;
    
    GUIKIT::Settings* settings = program->getSettings( activeEmulator );

    if ( settings->get<bool>( "audio_record_timelimit", false) ) {

        unsigned min = settings->get<unsigned>( "audio_record_minutes", 0, {0, 120});

        unsigned sec = settings->get<unsigned>( "audio_record_seconds", 0, {0, 59});

        timeLimit = min * 60 + sec;

        timeLimit *= 1000;
    }
}	

auto Handler::run(Emulator::Interface* emulator) -> bool {

    if (wavWriter == nullptr)
        return false;

    if (!emulator)
        return true;

    return activeEmulator == emulator;
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

    finish( true );
}

auto Handler::finish(bool timeup) -> void {

    if (!wavWriter)
        return;

    wavWriter->finish();

    delete wavWriter;

    wavWriter = nullptr;
    
    if (activeEmulator) {

        if (timeup && emuThread->enabled) {
            emuThread->finishAudioRecord = true;
        } else {
            auto emuView = EmuConfigView::TabWindow::getView(activeEmulator);

            if (emuView && emuView->audioLayout)
                emuView->audioLayout->stopRecord();
        }
    }

    statusHandler->updateAudioRecord( false );
}
    
}