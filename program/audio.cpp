
#include "program.h"
#include "audio/manager.h"

auto Program::initAudio() -> void {    
	if (audioDriver) delete audioDriver;
    
    if (cmd->noDriver) {
        audioDriver = new DRIVER::Audio;
        return;
    }
    
    audioDriver = DRIVER::Audio::create( getAudioDriver() );
    audioManager->setFrequency();
    audioManager->setLatency();
    audioManager->setSynchronize(!cmd->debug);
    audioManager->setRateControl();
    audioManager->setAudioDsp(); 
    audioManager->setStatistics();
    audioManager->setPriority();
    
    if ( !audioDriver->init( view->handle() ) ) {
        delete audioDriver;
        audioDriver = new DRIVER::Audio;
    }

    if (configView)
        configView->audioLayout->updateLatencySlider();
}

auto Program::getAudioDriver() -> std::string {
	auto curDriver = globalSettings->get<std::string>("audio_driver", "");
	auto drivers = DRIVER::Audio::available();
	
	for(auto& driver : drivers) {
		if(curDriver == driver) return driver;
	}
	return DRIVER::Audio::preferred();
}

auto Program::audioSample(int16_t sampleLeft, int16_t sampleRight) -> void {
    
    audioManager->process( sampleLeft, sampleRight );
}

auto Program::audioFlush() -> void {

    if (audioManager->bufferPos)
        audioManager->flush();
}

auto Program::mixDriveSound( Emulator::Interface::Media* media, Emulator::Interface::DriveSound driveSound, uint8_t data ) -> void {
    if (cmd->noDriver || cmd->debug)
        return;

    auto& stats = activeEmulator->getStatsForSelectedRegion();

    if (stats.sampleIntervall > 2) {
        if ( (stats.sampleIntervall * audioManager->bufferPos) > (256 << (uint8_t)stats.stereoSound) )
            audioManager->flush();
    }

    audioManager->drive.addSound( activeEmulator, media, (Mixer::Drive::DriveSound)driveSound, data );
}