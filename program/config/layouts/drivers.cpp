
#include "drivers.h"
#include "../../program.h"
#include "../../thread/emuThread.h"
#include "../../audio/manager.h"
#include "../../input/manager.h"

namespace ConfigView {

VideoDriverLayout::Top::Top() {
    //  append(exclusiveFullscreen, {0u, 0u}, 10);
    append(hardSync, {0u, 0u});
    append(driver, {~0u, 0u});

    setAlignment(0.5);
}

VideoDriverLayout::VideoDriverLayout() {
    append(top, {~0u, 0u});

    setPadding( 10 );
    setFont(GUIKIT::Font::system("bold"));
}

AudioDriverLayout::Top::Top() {
    append(frequencyLabel, {0u, 0u}, 5);
    append(frequencyCombo, {0u, 0u}, 20);
    append(maxRateLabel, {0u, 0u}, 5);
    append(maxRateEditCombo, {0u, 0u});
    append(driver, {~0u, 0u});

    maxRateEditCombo.append("0.003", 30);
    maxRateEditCombo.append("0.0035", 35);
    maxRateEditCombo.append("0.004", 40);
    maxRateEditCombo.append("0.0045", 45);
    maxRateEditCombo.append("0.005", 50);
    maxRateEditCombo.append("0.0055", 55);
    maxRateEditCombo.append("0.006", 60);

    frequencyCombo.append( "44100 Hz", 44100 );
    frequencyCombo.append( "48000 Hz", 48000 );

    setAlignment( 0.5 );
}

AudioDriverLayout::Bottom::Bottom() : latency("ms") {
    append(latency, {~0u, 0u});

    latency.slider.setLength(120);

    setAlignment( 0.5 );
}

AudioDriverLayout::AudioDriverLayout() {
    append(top, {~0u, 0u}, 10);
    append(bottom, {~0u, 0u});

    setPadding( 10 );
    setFont(GUIKIT::Font::system("bold"));
}

InputDriverLayout::Top::Top() {
    append(driver, {~0u, 0u});
    setAlignment( 0.5 );
}

InputDriverLayout::InputDriverLayout() {
    append(top, {~0u, 0u});

    setPadding( 10 );
    setFont(GUIKIT::Font::system("bold"));
}

DriversLayout::DriversLayout() {
    setMargin(10);

    //  bool showExclusiveFullscreenCheck = false;
    bool showHardSync = false;

    auto selectedDriver = program->getVideoDriver();
    unsigned i = 0;
    for(auto& driver : videoDriver->available()) {
        vdl.top.driver.combo.append( driver );
        if (driver == selectedDriver) {
            vdl.top.driver.combo.setSelection( i );
        }
        // if(GUIKIT::String::foundSubStr(driver, "Direct3D"))
        //     showExclusiveFullscreenCheck = true;

        if(GUIKIT::String::foundSubStr(driver, "GL") || GUIKIT::String::foundSubStr(driver, "Direct3D11"))
            showHardSync = true;

        i++;
    }

    vdl.top.driver.combo.onChange = [this]() {
        globalSettings->set<std::string>("video_driver", vdl.top.driver.combo.text() );

        emuThread->lock();
        // disable warp before, otherwise if threaded CPU CRT is selected, the midline callback would be active with warp.
        // warp will not render all frames but mid scanline callback runs without finishing the frame
        // todo: solve this better, more self-acting
        program->setWarp(false);
        program->initVideo(true);
        updateDriverPropsVisibility();
        emuThread->unlock();
    };

    if (vdl.top.driver.combo.rows() == 1) vdl.top.driver.setEnabled(false);

    // if( showExclusiveFullscreenCheck ) {
    //     vdl.top.exclusiveFullscreen.setEnabled(false);
    //
    //     if (videoDriver->canExclusiveFullscreen()) {
    //         vdl.top.exclusiveFullscreen.setEnabled( true );
    //         vdl.top.exclusiveFullscreen.setChecked(globalSettings->get("exclusive_fullscreen", false));
    //     }
    // } else
    //     vdl.top.remove( vdl.top.exclusiveFullscreen );

    if( showHardSync ) {
        vdl.top.hardSync.setEnabled(videoDriver->canHardSync());
        vdl.top.hardSync.setChecked(globalSettings->get("hardsync", false));
    } else
        vdl.top.remove( vdl.top.hardSync );

    // vdl.top.exclusiveFullscreen.onToggle = [](bool checked) {
    //        emuThread->lock();
    // 	globalSettings->set("exclusive_fullscreen", checked);
    //
    // 	program->hintExclusiveFullscreen();
    //        emuThread->unlock();
    // };

    vdl.top.hardSync.onToggle = [](bool checked) {
        emuThread->lock();
        globalSettings->set("hardsync", checked);
        videoDriver->hardSync( checked );
        emuThread->unlock();
    };

    selectedDriver = program->getAudioDriver();
    i = 0;
    for(auto& driver : audioDriver->available()) {
        adl.top.driver.combo.append( driver );
        if (driver == selectedDriver) {
            adl.top.driver.combo.setSelection( i );
        }
        i++;
    }

    if (adl.top.driver.combo.rows() == 1) adl.top.driver.setEnabled(false);

    adl.top.driver.combo.onChange = [this]() {
        emuThread->lock();
        globalSettings->set<std::string>("audio_driver", adl.top.driver.combo.text() );
        audioManager->record.finish();
        program->initAudio();
        emuThread->unlock();
    };

    adl.top.frequencyCombo.onChange = [this]() {
        emuThread->lock();
        globalSettings->set<unsigned>("audio_frequency_v2", adl.top.frequencyCombo.userData());
        audioManager->record.finish();
        audioManager->setFrequency();
        audioManager->setResampler();
        audioManager->resetDriveSounds( );
        audioManager->setAudioDsp();
        emuThread->unlock();
    };

    adl.bottom.latency.slider.onChange = [this](unsigned position) {
        emuThread->lock();
        auto minimumLatency = audioDriver->getMinimumLatency();

        globalSettings->set<unsigned>("audio_latency", position + minimumLatency);
        updateLatencySlider();
        audioManager->record.finish();
        audioManager->setLatency();
        emuThread->unlock();
    };

    adl.top.maxRateEditCombo.onChange = [this]() {
        emuThread->lock();
        float _rate = (float)adl.top.maxRateEditCombo.userData() / 10000.0;

        globalSettings->set<float>("rate_control_delta", _rate );
        audioManager->setRateControl();
        emuThread->unlock();
    };

    float _rate = globalSettings->get<float>("rate_control_delta", 0.005, {0.0, 0.010});
    int curRate = (int)(_rate * 10000.0 + 0.5);
    adl.top.maxRateEditCombo.setSelectionByUserId( 50 );
    for(unsigned i = 0; i < adl.top.maxRateEditCombo.rows(); i++) {
        if(adl.top.maxRateEditCombo.userData(i) == curRate) {
            adl.top.maxRateEditCombo.setSelection(i);
            break;
        }
    }

    auto valFre = globalSettings->get<unsigned>("audio_frequency_v2", 48000);
    for(unsigned i = 0; i < adl.top.frequencyCombo.rows(); i++) {
        if(adl.top.frequencyCombo.userData(i) == valFre) {
            adl.top.frequencyCombo.setSelection(i);
            break;
        }
    }

    selectedDriver = program->getInputDriver();
    i = 0;
    for (auto& driver : inputDriver->available()) {
        idl.top.driver.combo.append(driver);
        if (driver == selectedDriver) {
            idl.top.driver.combo.setSelection(i);
        }
        i++;
    }

    if (idl.top.driver.combo.rows() == 1) idl.top.driver.setEnabled(false);

    idl.top.driver.combo.onChange = [this]() {
        emuThread->lock();
        globalSettings->set<std::string>("input_driver", idl.top.driver.combo.text());
        InputManager::rememberLastDeviceState();
        program->initInput();
        emuThread->unlock();
    };

    append(vdl, {~0u, 0u}, 10);
    append(adl, {~0u, 0u}, 10);
    append(idl, {~0u, 0u});

    updateLatencySlider();
}

auto DriversLayout::translate() -> void {
    vdl.setText( trans->get("video driver") );
    adl.setText( trans->get("audio driver") );
    idl.setText( trans->get("input driver") );
    vdl.top.driver.name.setText( trans->get("driver", {}, true) );
    adl.top.driver.name.setText( trans->get("driver", {}, true) );
    idl.top.driver.name.setText( trans->get("driver", {}, true) );

    //vdl.top.exclusiveFullscreen.setText( trans->get("exclusive_fullscreen") );
    //vdl.top.exclusiveFullscreen.setTooltip( trans->get("exclusive_fullscreen_tooltip") );
    vdl.top.hardSync.setText( trans->get("hard_sync") );
    vdl.top.hardSync.setTooltip( trans->get("hard_sync_tooltip") );

    adl.bottom.latency.name.setText( trans->get("latency", {}, true) );
    adl.top.frequencyLabel.setText( trans->get("frequency", {}, true) );

    adl.top.maxRateLabel.setText( trans->get("drc_delta", {}, true) );
    adl.top.maxRateLabel.setTooltip( trans->get("drc_delta_tooltip") );

    SliderLayout::scale({&adl.bottom.latency}, "120 ms");
}

auto DriversLayout::updateDriverPropsVisibility() -> void {
    //vdl.top.exclusiveFullscreen.setEnabled( videoDriver->canExclusiveFullscreen() );
    vdl.top.hardSync.setEnabled( videoDriver->canHardSync() );
}

auto DriversLayout::updateLatencySlider() -> void {

    auto valLatency = globalSettings->get<unsigned>("audio_latency", 30u, {1u, 120u});
    auto minimumLatency = audioDriver->getMinimumLatency();
    auto maximumLatency = 120;
    valLatency = std::max( valLatency, minimumLatency );
    globalSettings->set<unsigned>("audio_latency", valLatency);

    adl.bottom.latency.slider.setLength( maximumLatency - minimumLatency + 1 );
    adl.bottom.latency.value.setText(std::to_string( valLatency ) + " ms");
    adl.bottom.latency.slider.setPosition(valLatency - minimumLatency);
}

}
