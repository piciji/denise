
#include "program.h"
#include "audio/manager.h"
#include "emuconfig/layouts/audio.h"

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
    
    if ( !audioDriver->init( view->handle() ) ) {
        delete audioDriver;
        audioDriver = new DRIVER::Audio;
    }
    // driver initialization could use different frequency than user requested
    audioManager->setResampler();
    audioManager->resetDriveSounds();
    audioManager->setAudioDsp();

    if (configView)
        configView->driversLayout->updateLatencySlider();
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

auto Program::mixDriveSound( Emulator::Interface::Media* media, Emulator::Interface::DriveSound driveSound, bool alternate, uint8_t data ) -> void {
    if (cmd->noDriver || cmd->debug)
        return;

    auto& stats = activeEmulator->getStatsForSelectedRegion();

    if (stats.sampleIntervall > 2) {
        if ( (stats.sampleIntervall * audioManager->bufferPos) > (256 << (uint8_t)stats.stereoSound) )
            audioManager->flush();
    }

    audioManager->drive.addSound( activeEmulator, media, (Mixer::Drive::DriveSound)driveSound, alternate, data );
}

auto Program::toggleRecord() -> void {
    auto emuView = EmuConfigView::TabWindow::getView(activeEmulator);
    if (emuView && emuView->audioLayout) {
        emuView->audioLayout->toggleRecord();
    } else if (audioManager) {
        emuThread->lock();
        std::string errorText;
        if (!audioManager->record.toggle(activeEmulator, errorText))
            statusHandler->setMessage(errorText, true);
        if (view)
            view->setAudioRecordText();

        emuThread->unlock();
    }
}