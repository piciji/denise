
#include "handler.h"
#include "wavWriter.h"
#ifdef LIBLAME
#include "mp3Writer.h"
#endif
#include "../../tools/chronos.h"
#include "../../program.h"
#include "../../tools/logger.h"
#include "../../emuconfig/config.h"
#include "../../view/status.h"
#include "../../thread/emuThread.h"
#include "../../view/view.h"

namespace AudioRecord {
    
auto Handler::start( Emulator::Interface* emulator, std::string& errorText ) -> bool {

    finish();
    
    if (emulator != activeEmulator) {
        errorText = trans->get("no emulation active");
        return false;     
    }
    
    GUIKIT::Settings* settings = program->getSettings( activeEmulator );

    std::string path = program->generatedFolder(emulator, "audio_record_path", "recordings/audio", true);
    
    std::string fileName = settings->get<std::string>( "audio_record_ident", "sample");

    Type type = (Type)settings->get<unsigned>( "audio_record_type", 0);

    sampleRate = audioDriver->getFrequency();

    framesTimeCheck = 0;		
    
    useFloat = audioDriver->expectFloatingPoint();
    
    std::string filePath = path + fileName + "_" + std::to_string(Chronos::getTimestampInSecondsReal());
    
    switch (type) {
        case WAV:
            filePath += ".wav";
            baseWriter = new WavWriter;
            break;
        case MP3:
        default:
            filePath += ".mp3";
#ifdef LIBLAME
            baseWriter = new MP3Writer;
#else
            baseWriter = new BaseWriter;
#endif
            break;
    }

    if (!baseWriter->init(filePath, sampleRate, useFloat)) {

        delete baseWriter;

        baseWriter = nullptr;
        
        errorText = trans->get("file_creation_error", {{"%path%", filePath}});

        return false;
    }

    startTime = Chronos::getTimestampInMilliseconds();
    
    setTimeLimit();

    if (statusHandler)
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

    if (baseWriter == nullptr)
        return false;

    if (!emulator)
        return true;

    return activeEmulator == emulator;
}

auto Handler::write( uint8_t* buf, unsigned frames ) -> void {

    if (!run())
        return;

    baseWriter->write(buf, frames << (useFloat ? 3 : 2));

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

    if (!baseWriter)
        return;

    baseWriter->finish();

    delete baseWriter;

    baseWriter = nullptr;
    
    if (activeEmulator) {

        if (timeup && emuThread->enabled) {
            emuThread->events |= EmuThread::EVT_FINISH_AUDIO_RECORD;
        } else {
            auto emuView = EmuConfigView::TabWindow::getView(activeEmulator);

            if (emuView && emuView->audioLayout)
                emuView->audioLayout->stopRecord();
            if (view)
                view->setAudioRecordText();
        }
    }

    if (statusHandler)
        statusHandler->updateAudioRecord( false );
}

auto Handler::toggle(Emulator::Interface* emulator, std::string& errorText) -> bool {
    if (run()) {
        finish();
    } else {
        if (!start(emulator, errorText)) {
            return false;
        }
    }
    return true;
}
    
}
