
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
    audioManager->setSynchronize();
    audioManager->setRateControl();
    audioManager->setAudioDsp(); 
    audioManager->setStatistics();
    audioManager->setPriority();
    
    if ( !audioDriver->init( view->handle() ) ) {
        delete audioDriver;
        audioDriver = new DRIVER::Audio;
    }    
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
