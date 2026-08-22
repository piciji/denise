
#include "audio.h"
#include "../config.h"
#include "../../../data/icons.h"
#include "../../thread/emuThread.h"
#include "../../view/view.h"
#include "../../view/status.h"
#include "../../audio/manager.h"
#include "../../helper/fileHelper.h"
#include "../../helper/miscHelper.h"

#define mes this->tabWindow->message
#define _settings this->tabWindow->settings

namespace EmuConfigView {

AudioDriveLayout::TapeSelection::TapeSelection() {
    append( label, {0u, 0u}, 10 );
    append( combo, {0u, 0u} );
    append( spacer, {~0u, 0u} );
    append( reload, {0u, 0u} );

    setAlignment( 0.5 );
}

AudioDriveLayout::FloppySelection::FloppySelection() {
    append( label, {0u, 0u}, 10 );
    append( combo, {0u, 0u}, 10 );
    append( labelExt, {0u, 0u}, 10 );
    append( comboExt, {0u, 0u} );
    append( spacer, {~0u, 0u} );
    append( reload, {0u, 0u} );

    setAlignment( 0.5 );
}

AudioDriveLayout::AudioDriveLayout(Emulator::Interface* emulator) :
floppyVolume("%", true),
floppyVolumeExt("%", false),
tapeVolume("%", true),
tapeNoiseVolume("%", true) {
    append( floppyVolume, {~0u, 0u}, 10 );
    append( floppyVolumeExt, {~0u, 0u}, 10 );
    append( floppySelection, {~0u, 0u}, 10 );
    auto tapeMediaGroup = emulator->getTapeMediaGroup();

    if(tapeMediaGroup) {
        append(tapeVolume, {~0u, 0u}, 10);
        append(tapeSelection, {~0u, 0u}, 10);
        append(tapeNoiseVolume, {~0u, 0u});
    }

    setPadding(10);
    setFont(GUIKIT::Font::system("bold"));

    floppyVolume.slider.setLength( 301 );
    floppyVolume.updateValueWidth( "300 %" );

    floppyVolumeExt.slider.setLength( 301 );
    floppyVolumeExt.updateValueWidth( "300 %" );

    if(tapeMediaGroup) {
        tapeVolume.slider.setLength(301);
        tapeVolume.updateValueWidth("300 %");
        tapeNoiseVolume.slider.setLength(301);
        tapeNoiseVolume.updateValueWidth("300 %");
    }
}

AudioRecordLayout::Type::Type() {
    append(label, { 0u, 0u }, 10);
    append(mp3, { 0u, 0u }, 10);
    append(wav, { 0u, 0u });

    setAlignment(0.5);
    GUIKIT::RadioBox::setGroup(mp3, wav);
}

AudioRecordLayout::Location::Location() {
    
    append(label, {0u, 0u}, 5 );
    append(pathEdit, { ~0u, 0u }, 5);
    append(standard, { 0u, 0u }, 5);
    append(select, {0u, 0u} );
    
    pathEdit.setEditable( false );
    
    label.setFont(GUIKIT::Font::system("bold"));
    setAlignment( 0.5 );
}

AudioRecordLayout::Duration::Duration() : minutesSlider(""), secondsSlider("") {
    
    append(useTimeLimit, {0u, 0u}, 10 );
    append(minutesSlider, {~0u, 0u}, 10);
    append(secondsSlider, {~0u, 0u}, 20);
    append(record, {0u, 0u} );
    
    minutesSlider.slider.setLength( 121 );
    secondsSlider.slider.setLength( 60 );
    
    minutesSlider.updateValueWidth("999");
    secondsSlider.updateValueWidth("99");
    
    setAlignment( 0.5 );
}

AudioRecordLayout::AudioRecordLayout() {
    
    setPadding(10);
    
    append(location, {~0u, 0u}, 10 );
    append(type, { ~0u, 0u }, 10);
    append(duration, {~0u, 0u} );
    
    setFont(GUIKIT::Font::system("bold"));
}

BassControlLayout::TopLayout::TopLayout() :
frequency( "Hz" ) {    
    append( active, {0u, 0u}, 10 );
    append( frequency, {~0u, 0u}, 10 );
    append( reset, {0u, 0u} );
    
    frequency.slider.setLength( 181 );    
    frequency.updateValueWidth( "200 Hz" );
    
    setAlignment( 0.5 );
}

BassControlLayout::BottomLayout::BottomLayout() :
gain( "" ),
reduceClipping( "" ) {    
    append( gain, {~0u, 0u}, 10 );
    append( reduceClipping, {~0u, 0u} );
    
    gain.slider.setLength( 41 );
    reduceClipping.slider.setLength( 11 );
    
    gain.updateValueWidth( "30" );
    reduceClipping.updateValueWidth( "0.9" );
    
    setAlignment( 0.5 );
}

BassControlLayout::BassControlLayout() {
    append( top, {~0u, 0u}, 10 );
    append( bottom, {~0u, 0u} );
    setFont(GUIKIT::Font::system("bold"));
    setPadding( 10 );
}

EchoControlLayout::TopLayout::TopLayout() :
amp( "" ) {
    append( active, {0u, 0u}, 10 );
    append( echoReverb, {0u, 0u}, 10 );
    append( amp, {~0u, 0u}, 10 );
    append( reset, {0u, 0u} );

    amp.slider.setLength( 101 );
    amp.updateValueWidth( "0.99" );

    setAlignment( 0.5 );
}

EchoControlLayout::BottomLayout::BottomLayout() :
delay( "ms" ),
feedback( "" ) {
    append( delay, {~0u, 0u}, 10 );
    append( feedback, {~0u, 0u} );

    delay.slider.setLength( 101 );
    feedback.slider.setLength( 101 );

    delay.updateValueWidth( "1000 ms" );
    feedback.updateValueWidth( "0.99" );

    setAlignment( 0.5 );
}

EchoControlLayout::EchoControlLayout() {
    append( top, {~0u, 0u}, 10 );
    append( bottom, {~0u, 0u} );
    setFont(GUIKIT::Font::system("bold"));
    setPadding( 10 );
}

ReverbControlLayout::TopLayout::TopLayout() :
dryTime( "" ),
wetTime( "" ) {
    append( active, {0u, 0u}, 10 );
    append( dryTime, {~0u, 0u}, 10 );
    append( wetTime, {~0u, 0u}, 10 );
    append( reset, {0u, 0u} );
    
    dryTime.slider.setLength( 101 );    
    dryTime.updateValueWidth( "0.99" );
    wetTime.slider.setLength( 101 );    
    wetTime.updateValueWidth( "0.99" );
    
    setAlignment( 0.5 );
}

ReverbControlLayout::BottomLayout::BottomLayout() :
roomWidth( "" ),
roomSize( "" ),
damping( "" ) {    
    append( damping, {~0u, 0u}, 10 );
    append( roomWidth, {~0u, 0u}, 10 );
    append( roomSize, {~0u, 0u} );    
    
    roomWidth.slider.setLength( 101 );
    roomSize.slider.setLength( 101 );
    damping.slider.setLength( 101 );
    
    roomWidth.updateValueWidth( "0.99" );
    roomSize.updateValueWidth( "0.99" );
    damping.updateValueWidth( "0.99" );
    
    setAlignment( 0.5 );
}

ReverbControlLayout::ReverbControlLayout() {
    append( top, {~0u, 0u}, 10 );
    append( bottom, {~0u, 0u} );
    setFont(GUIKIT::Font::system("bold"));
    setPadding( 10 );
}

PanningControlLayout::TopLayout::TopLayout() :
separation("%") {
    append( active, {0u, 0u}, 10 );
    append( separation, {~0u, 0u}, 10 );
    append( reset, {0u, 0u} );
    separation.slider.setLength(21);
    separation.updateValueWidth( "000%" );

    setAlignment( 0.5 );
}

PanningControlLayout::MiddleLayout::MiddleLayout() :
leftMix( "" ),
rightMix( "" ) {
    append( leftChannel, {0u, 0u}, 10);
    append( leftMix, {~0u, 0u}, 10);
    append( rightMix, {~0u, 0u});
    
    leftMix.slider.setLength( 101 );
    rightMix.slider.setLength( 101 );
    
    leftMix.updateValueWidth( "0.99" );
    rightMix.updateValueWidth( "0.99" );
    
    leftChannel.setFont(GUIKIT::Font::system("bold"));
    
    setAlignment( 0.5 );
}

PanningControlLayout::BottomLayout::BottomLayout() :
leftMix( "" ),
rightMix( "" ) {
    append( rightChannel, {0u, 0u}, 10);
    append( leftMix, {~0u, 0u}, 10);
    append( rightMix, {~0u, 0u});
    
    leftMix.slider.setLength( 101 );
    rightMix.slider.setLength( 101 );
    
    leftMix.updateValueWidth( "0.99" );
    rightMix.updateValueWidth( "0.99" );
    
    rightChannel.setFont(GUIKIT::Font::system("bold"));
    
    setAlignment( 0.5 );
}

PanningControlLayout::PanningControlLayout() {
    append( top, {~0u, 0u}, 10 );
    append( middle, {~0u, 0u}, 10 );
    append( bottom, {~0u, 0u} );
    setFont(GUIKIT::Font::system("bold"));
    setPadding( 10 );
}

VolumeControlLayout::Info::Info() {
    append(label, {0u, 0u}, 5);
    append(value, {50u, 0u});
}

VolumeControlLayout::VolumeControlLayout() {
    append( volumeSlider, {30, ~0u}, 5 );
    append( info, {0u, 0u});
    volumeSlider.setLength(101);
}

AudioLayout::AudioLayout(TabWindow* tabWindow) {
    
    this->tabWindow = tabWindow;
    this->emulator = tabWindow->emulator;

    setMargin(10);

    driveLayout = new AudioDriveLayout(this->emulator);
    moduleList.setHeaderText( { "" } );
    moduleList.setHeaderVisible( false );
    
    recordAudioImage.loadPng((uint8_t*)Icons::recordAudio, sizeof(Icons::recordAudio));
    sineImage.loadPng((uint8_t*)Icons::sine, sizeof(Icons::sine));
    processorImage.loadPng((uint8_t*)Icons::processor, sizeof(Icons::processor));
    driveImage.loadPng((uint8_t*)Icons::drive, sizeof(Icons::drive));
    resetImage.loadPng((uint8_t*)Icons::back, sizeof(Icons::back));

    moduleList.append( {"SID"} );
    moduleList.setImage(0, 0, processorImage);

    moduleList.append( {"Drive"} );
    moduleList.setImage(moduleList.rowCount() - 1, 0, driveImage);
    moduleList.append( {"DSP"} );
    moduleList.setImage(moduleList.rowCount() - 1, 0, sineImage);    
    moduleList.append( {"Record"} );    
    moduleList.setImage(moduleList.rowCount() - 1, 0, recordAudioImage);

    bass.top.reset.setImage(&resetImage);
    echo.top.reset.setImage(&resetImage);
    reverb.top.reset.setImage(&resetImage);
    panning.top.reset.setImage(&resetImage);
        
    moduleList.setSelection(0);
    moduleFrame.append( moduleList, { GUIKIT::Font::scale(140), GUIKIT::Font::scale(dynamic_cast<LIBC64::Interface*>(emulator) ? 120 : 100)}, 15 );
    moduleFrame.append( volumeLayout, {0u, ~0u} );
    moduleFrame.setPadding(10);
    moduleFrame.setFont( GUIKIT::Font::system("bold") );
    
    moduleList.onChange = [this]() {

        if (!moduleList.selected())
            return;

        moduleSwitch.setSelection( moduleList.selection() );
    };
    
    append( moduleFrame, {0u, ~0u}, 10 );

    std::vector<unsigned> dim;
    if (dynamic_cast<LIBC64::Interface*>(emulator))
        dim = {5, 1, 1, 1, 2, 4, 4, 4, 4, 4, 4, 4, 4};
    else
        dim = {1,1};

    settingsLayout.build(tabWindow, emulator,
        {Emulator::Interface::Model::Purpose::SoundChip,
         Emulator::Interface::Model::Purpose::AudioSettings,
         Emulator::Interface::Model::Purpose::AudioResampler}, dim, dynamic_cast<LIBC64::Interface*>(emulator) ? 5 : 8);

    settingsLayout.setEvents();
    
    if (dynamic_cast<LIBC64::Interface*>(emulator)) {
        usbSidPicoLayout = new ModelLayout;

        usbSidPicoLayout->build(tabWindow, emulator,
            {Emulator::Interface::Model::Purpose::AudioExtern}, {1,1,1}, 8);

        usbSidPicoLayout->setMargin( 10 );

        settingsLayout.controlLayout.button.onActivate = [this]() {
            if (!picoWindow) {
                picoWindow = new PicoWindow;
                picoWindow->layout.append( *usbSidPicoLayout, {~0u, 0u} );
                picoWindow->layout.append( picoWindow->spacer, {0u, ~0u} );
                picoWindow->layout.setAlignment( 0.5 );
                picoWindow->append( picoWindow->layout );
            }

            MiscHelper::centerGeometry( picoWindow, {500, 170}, this->tabWindow->geometry() );
            picoWindow->setVisible(  );
            picoWindow->setFocused(  );
        };

        usbSidPicoLayout->setEvents();
    } else
        usbSidPicoLayout = nullptr;
    
    dspFrame.append( bass, {~0u, 0u}, 5 );
    dspFrame.append( echo, {~0u, 0u}, 5 );
    dspFrame.append( reverb, {~0u, 0u}, 5 );
    dspFrame.append( panning, {~0u, 0u} );

    append( moduleSwitch, {~0u, ~0u} );

    int layPos = 0;
    moduleSwitch.setLayout( layPos++, settingsLayout, {~0u, ~0u} );
    moduleSwitch.setLayout( layPos++, *driveLayout, {~0u, ~0u} );
    moduleSwitch.setLayout( layPos++, dspFrame, {~0u, ~0u} );
    moduleSwitch.setLayout( layPos++, audioRecord, {~0u, ~0u} );
    
    bass.top.active.onToggle = [this](bool checked) {
        
        _settings->set<bool>("audio_bass", checked );
        
        updateVisibility();
        emuThread->lock();
        audioManager->setAudioDsp();
        emuThread->unlock();
    };
    
    bass.top.frequency.slider.onChange = [this](unsigned position) {
        
        unsigned val = position + 20;
        
        _settings->set<unsigned>("audio_bass_freq", val );
        
        bass.top.frequency.value.setText( std::to_string(val) + " Hz" );

        emuThread->lock();
        audioManager->setAudioDsp();
        emuThread->unlock();
    };

    bass.top.reset.onActivate = [this]() {
        _settings->set<unsigned>("audio_bass_freq", 200 );
        _settings->set<unsigned>("audio_bass_gain", 10 );
        _settings->set<float>("audio_bass_clipping", 0.4);

        bass.top.frequency.slider.setPosition(200 - 20);
        bass.top.frequency.value.setText("200 Hz");
        bass.bottom.gain.slider.setPosition(10);
        bass.bottom.gain.value.setText("10");
        bass.bottom.reduceClipping.slider.setPosition((unsigned) (0.4 * 10.0));
        bass.bottom.reduceClipping.value.setText("0.4");

        emuThread->lock();
        audioManager->setAudioDsp();
        emuThread->unlock();
    };
    
    bass.bottom.gain.slider.onChange = [this](unsigned position) {
        _settings->set<unsigned>("audio_bass_gain", position );
        
        bass.bottom.gain.value.setText( std::to_string(position) );

        emuThread->lock();
        audioManager->setAudioDsp();
        emuThread->unlock();
    };
    
    bass.bottom.reduceClipping.slider.onChange = [this](unsigned position) {
        float val = (float)position / 10.0;
        
        _settings->set<float>("audio_bass_clipping", val);
        
        bass.bottom.reduceClipping.value.setText( GUIKIT::String::convertDoubleToString( val, 1) );

        emuThread->lock();
        audioManager->setAudioDsp();
        emuThread->unlock();
    };

    // echo
    echo.top.reset.onActivate = [this]() {
        _settings->set<float>("audio_echo_amp", 0.2);
        _settings->set<float>("audio_echo_feedback", 0.5);
        _settings->set<unsigned>("audio_echo_delay", 200);

        initDsp( &echo.top.amp, "audio_echo_amp", 0.2 );
        initDsp( &echo.bottom.feedback, "audio_echo_feedback", 0.5 );
        echo.bottom.delay.slider.setPosition(20);
        echo.bottom.delay.value.setText("200 ms");

        emuThread->lock();
        audioManager->setAudioDsp();
        emuThread->unlock();
    };

    echo.top.echoReverb.onActivate = [this]() {
        _settings->set<float>("audio_echo_amp", 0.25);
        _settings->set<float>("audio_echo_feedback", 0.6);
        _settings->set<unsigned>("audio_echo_delay", 200);

        _settings->set<float>("audio_reverb_drytime", 0.43);
        _settings->set<float>("audio_reverb_wettime", 0.3);
        _settings->set<float>("audio_reverb_damping", 1.0);
        _settings->set<float>("audio_reverb_roomwidth", 0.75);
        _settings->set<float>("audio_reverb_roomsize", 0.75);

        initDsp( &echo.top.amp, "audio_echo_amp", 0.25 );
        initDsp( &echo.bottom.feedback, "audio_echo_feedback", 0.6 );
        echo.bottom.delay.slider.setPosition(20);
        echo.bottom.delay.value.setText("200 ms");

        initDsp( &reverb.top.dryTime, "audio_reverb_drytime", 0.43 );
        initDsp( &reverb.top.wetTime, "audio_reverb_wettime", 0.3 );
        initDsp( &reverb.bottom.damping, "audio_reverb_damping", 1.0 );
        initDsp( &reverb.bottom.roomWidth, "audio_reverb_roomwidth", 0.75 );
        initDsp( &reverb.bottom.roomSize, "audio_reverb_roomsize", 0.75 );

        _settings->set<bool>("audio_reverb", true );
        reverb.top.active.setChecked();
        updateVisibility();

        emuThread->lock();
        audioManager->setAudioDsp();
        emuThread->unlock();
    };

    echo.top.active.onToggle = [this](bool checked) {
        _settings->set<bool>("audio_echo", checked );

        updateVisibility();

        emuThread->lock();
        audioManager->setAudioDsp();
        emuThread->unlock();
    };

    echo.bottom.delay.slider.onChange = [this](unsigned position) {
        unsigned val = position * 10;

        _settings->set<unsigned>("audio_echo_delay", val);

        echo.bottom.delay.value.setText( std::to_string(val) + " ms" );

        emuThread->lock();
        audioManager->setAudioDsp();
        emuThread->unlock();
    };

    setDspEvent( &echo.top.amp, "audio_echo_amp", 0.2 );
    setDspEvent( &echo.bottom.feedback, "audio_echo_feedback", 0.5 );
    
    // reverb
    reverb.top.active.onToggle = [this](bool checked) {
        
        _settings->set<bool>("audio_reverb", checked );
        
        updateVisibility();

        emuThread->lock();
        audioManager->setAudioDsp();
        emuThread->unlock();
    };           
    
    setDspEvent( &reverb.top.dryTime, "audio_reverb_drytime", 0.43 );
    setDspEvent( &reverb.top.wetTime, "audio_reverb_wettime", 0.4 );
    setDspEvent( &reverb.bottom.damping, "audio_reverb_damping", 0.8 );
    setDspEvent( &reverb.bottom.roomWidth, "audio_reverb_roomwidth", 0.56 );
    setDspEvent( &reverb.bottom.roomSize, "audio_reverb_roomsize", 0.56 );

    reverb.top.reset.onActivate = [this]() {
        _settings->set<float>("audio_reverb_drytime", 0.43);
        _settings->set<float>("audio_reverb_wettime", 0.4);
        _settings->set<float>("audio_reverb_damping", 0.8);
        _settings->set<float>("audio_reverb_roomwidth", 0.56);
        _settings->set<float>("audio_reverb_roomsize", 0.56);

        initDsp( &reverb.top.dryTime, "audio_reverb_drytime", 0.43 );
        initDsp( &reverb.top.wetTime, "audio_reverb_wettime", 0.4 );
        initDsp( &reverb.bottom.damping, "audio_reverb_damping", 0.8 );
        initDsp( &reverb.bottom.roomWidth, "audio_reverb_roomwidth", 0.56 );
        initDsp( &reverb.bottom.roomSize, "audio_reverb_roomsize", 0.56 );

        emuThread->lock();
        audioManager->setAudioDsp();
        emuThread->unlock();
    };
    
    // panning
    panning.top.active.onToggle = [this](bool checked) {
        
        _settings->set<bool>("audio_panning", checked );
        
        updateVisibility();

        emuThread->lock();
        audioManager->setAudioDsp();
        emuThread->unlock();
    };
    
    setDspEvent( &panning.middle.leftMix, "audio_panning_left0", 1.0 );
    setDspEvent( &panning.middle.rightMix, "audio_panning_left1", 0.0 );
    setDspEvent( &panning.bottom.leftMix, "audio_panning_right0", 0.0 );
    setDspEvent( &panning.bottom.rightMix, "audio_panning_right1", 1.0 );

    panning.top.reset.onActivate = [this]() {
        _settings->set<float>("audio_panning_left0", 1.0);
        _settings->set<float>("audio_panning_left1", 0.0);
        _settings->set<float>("audio_panning_right0", 0.0);
        _settings->set<float>("audio_panning_right1", 1.0);

        initDsp( &panning.middle.leftMix, "audio_panning_left0", 1.0 );
        initDsp( &panning.middle.rightMix, "audio_panning_left1", 0.0 );
        initDsp( &panning.bottom.leftMix, "audio_panning_right0", 0.0 );
        initDsp( &panning.bottom.rightMix, "audio_panning_right1", 1.0 );

        initSeparation();

        emuThread->lock();
        audioManager->setAudioDsp();
        emuThread->unlock();
    };

    setSeparation();
        
    audioRecord.duration.useTimeLimit.onToggle = [this](bool checked) {
        _settings->set<bool>( "audio_record_timelimit", checked);
        
        audioRecord.duration.setEnabled( checked );
        
        audioRecord.duration.useTimeLimit.setEnabled();            
        audioRecord.duration.record.setEnabled();

        emuThread->lock();
        audioManager->record.setTimeLimit();
        emuThread->unlock();
    };
    
    audioRecord.location.select.onActivate = [this]() {
        
        auto path = GUIKIT::BrowserWindow()
            .setTitle( trans->get( "record path" ) )
            .setWindow( *this->tabWindow )
            .directory();
        
        if (path.empty())
            return;
        
        path = GUIKIT::File::buildRelativePath(path);
        audioRecord.location.pathEdit.setText(path);
        audioRecord.location.pathEdit.setEnabled();
        
        _settings->set<std::string>( "audio_record_path", path );
    };

    audioRecord.location.standard.onActivate = [this]() {
        _settings->set<std::string>("audio_record_path", "");
        audioRecord.location.pathEdit.setText( FileHelper::generatedFolder(emulator, "audio_record_path", "recordings/audio", FileHelper::FLAG_VIEW) );
        audioRecord.location.pathEdit.setEnabled(false);
    };

    audioRecord.type.mp3.onActivate = [this]() {
        _settings->set<unsigned>("audio_record_type", 0);
    };

    audioRecord.type.wav.onActivate = [this]() {
        _settings->set<unsigned>("audio_record_type", 1);
    };
    
    audioRecord.duration.minutesSlider.slider.onChange = [this](unsigned position) {
        _settings->set<unsigned>( "audio_record_minutes", position );
        
        audioRecord.duration.minutesSlider.value.setText( std::to_string(position) );

        emuThread->lock();
        audioManager->record.setTimeLimit();
        emuThread->unlock();
    };
    
    audioRecord.duration.secondsSlider.slider.onChange = [this](unsigned position) {
        _settings->set<unsigned>( "audio_record_seconds", position );
        
        audioRecord.duration.secondsSlider.value.setText( std::to_string(position) );

        emuThread->lock();
        audioManager->record.setTimeLimit();
        emuThread->unlock();
    };
    
    audioRecord.duration.record.onToggle = [this]() {

        bool state = audioRecord.duration.record.checked();
        emuThread->lock();
        if (state) {
            std::string errorText;
            if (!audioManager->record.start(this->emulator, errorText)) {
                emuThread->unlock();
                mes->error( errorText );
                audioRecord.duration.record.setChecked(false);
                return;
            }

            view->setAudioRecordText();
        } else
            audioManager->record.finish();

        emuThread->unlock();
        audioRecord.duration.record.setText( trans->get( state ? "Stop" : "Record" ) );
    };

    driveLayout->floppyVolume.active.onToggle = [this](bool checked) {

        _settings->set<bool>( "audio_floppy", checked );

        if (emulator == activeEmulator) {
            emuThread->lock();
            audioManager->setDriveSounds();
            emuThread->unlock();
        }
    };

    driveLayout->floppyVolume.slider.onChange = [this](unsigned position) {
        _settings->set<unsigned>( "audio_floppy_volume", position );

        driveLayout->floppyVolume.value.setText( std::to_string( position ) + " %" );

        if (activeEmulator) {
            emuThread->lock();
            audioManager->drive.setVolume(activeEmulator, activeEmulator->getDiskMediaGroup(), (float) position * 0.01, false);
            emuThread->unlock();
        }
    };

    driveLayout->floppyVolumeExt.slider.onChange = [this](unsigned position) {
        _settings->set<unsigned>( "audio_floppy_volume_external", position );

        driveLayout->floppyVolumeExt.value.setText( std::to_string( position ) + " %" );

        if (activeEmulator) {
            emuThread->lock();
            audioManager->drive.setVolume(activeEmulator, activeEmulator->getDiskMediaGroup(), (float) position * 0.01, true);
            emuThread->unlock();
        }
    };

    driveLayout->floppySelection.combo.onChange = [this]() {

        std::string folder = driveLayout->floppySelection.combo.text();

        _settings->set<std::string>( audioManager->drive.getFloppyFolderIdent(emulator, false), folder );

        emuThread->lock();
        audioManager->drive.unload( emulator, emulator->getDiskMediaGroup(), false );
        if (emulator == activeEmulator)
            audioManager->setDriveSounds( false );
        emuThread->unlock();
    };

    driveLayout->floppySelection.comboExt.onChange = [this]() {

        std::string folder = driveLayout->floppySelection.comboExt.text();

        _settings->set<std::string>( audioManager->drive.getFloppyFolderIdent(emulator, true), folder );

        emuThread->lock();
        audioManager->drive.unload( emulator, emulator->getDiskMediaGroup(), true );
        if (emulator == activeEmulator)
            audioManager->setDriveSounds( false );
        emuThread->unlock();
    };

    driveLayout->floppySelection.reload.onActivate = [this]() {
        updateFloppyProfileList();

        driveLayout->floppySelection.combo.setSelectionByText( audioManager->drive.getFloppyFolder(emulator, false) );
        driveLayout->floppySelection.comboExt.setSelectionByText( audioManager->drive.getFloppyFolder(emulator, true) );

        // in case if content doesn't match setting anymore
        _settings->set<std::string>(audioManager->drive.getFloppyFolderIdent(emulator, false), driveLayout->floppySelection.combo.text() );
        _settings->set<std::string>(audioManager->drive.getFloppyFolderIdent(emulator, true), driveLayout->floppySelection.comboExt.text() );

        emuThread->lock();
        audioManager->drive.unload( emulator, emulator->getDiskMediaGroup(), false );
        audioManager->drive.unload( emulator, emulator->getDiskMediaGroup(), true );
        if (emulator == activeEmulator)
            audioManager->setDriveSounds( false );
        emuThread->unlock();
    };

    driveLayout->tapeVolume.active.onToggle = [this](bool checked) {

        _settings->set<bool>( "audio_tape", checked );

        if (emulator == activeEmulator) {
            emuThread->lock();
            audioManager->setDriveSounds();
            emuThread->unlock();
        }
    };

    driveLayout->tapeVolume.slider.onChange = [this](unsigned position) {

        _settings->set<unsigned>( "audio_tape_volume", position );

        driveLayout->tapeVolume.value.setText( std::to_string( position ) + " %" );

        if (activeEmulator) {
            emuThread->lock();
            audioManager->drive.setVolume(activeEmulator, activeEmulator->getTapeMediaGroup(), (float)position * 0.01, false);
            emuThread->unlock();
        }
    };

    driveLayout->tapeNoiseVolume.active.onToggle = [this](bool checked) {

        _settings->set<bool>( "audio_tape_noise", checked );

        if (emulator == activeEmulator) {
            emuThread->lock();
            audioManager->setTapeNoise();
            emuThread->unlock();
        }
    };

    driveLayout->tapeNoiseVolume.slider.onChange = [this](unsigned position) {

        _settings->set<unsigned>( "audio_tape_noise_volume", position );

        driveLayout->tapeNoiseVolume.value.setText( std::to_string( position ) + " %" );

        if (emulator == activeEmulator) {
            emuThread->lock();
            audioManager->setTapeNoise();
            emuThread->unlock();
        }
    };

    driveLayout->tapeSelection.combo.onChange = [this]() {

        std::string folder = driveLayout->tapeSelection.combo.text();

        _settings->set<std::string>( "audio_tape_folder", folder );

        emuThread->lock();
        audioManager->drive.unload( emulator, emulator->getTapeMediaGroup(), false );
        if (emulator == activeEmulator)
            audioManager->setDriveSounds( false );
        emuThread->unlock();
    };

    driveLayout->tapeSelection.reload.onActivate = [this]() {
        updateTapeProfileList();

        driveLayout->tapeSelection.combo.setSelectionByText( _settings->get<std::string>("audio_tape_folder", "") );

        // in case if content doesn't match setting anymore
        _settings->set<std::string>("audio_tape_folder", driveLayout->tapeSelection.combo.text() );

        emuThread->lock();
        audioManager->drive.unload( emulator, emulator->getTapeMediaGroup(), false );
        if (emulator == activeEmulator)
            audioManager->setDriveSounds( false );
        emuThread->unlock();
    };

    volumeLayout.volumeSlider.onChange = [this](unsigned position) {
        _settings->set<unsigned>("audio_volume", position);

        volumeLayout.info.value.setText( std::to_string(position) + "%");

        emuThread->lock();

        if (activeEmulator == emulator) {
            if (statusHandler)
                statusHandler->setVolumeSlider(position);

            audioManager->setVolume();
        }

        emuThread->unlock();
    };

    updateFloppyProfileList();

    updateTapeProfileList();

    loadSettings();
}

auto AudioLayout::updateVolumeSlider() -> void {
    unsigned volume = _settings->get<unsigned>("audio_volume", 100, {0, 100});
    volumeLayout.volumeSlider.setPosition(volume);
    volumeLayout.info.value.setText( std::to_string(volume) + "%");
}

auto AudioLayout::updateFloppyProfileList() -> void {

    driveLayout->floppySelection.combo.reset();
    driveLayout->floppySelection.comboExt.reset();

    auto baseFolder = program->soundFolder();

    auto list = GUIKIT::File::getFolderList(baseFolder + "floppy/" + emulator->ident);

    for (auto& info : list) {
        driveLayout->floppySelection.combo.append( info.name );
        driveLayout->floppySelection.comboExt.append( info.name );
    }
}

auto AudioLayout::updateTapeProfileList() -> void {

    driveLayout->tapeSelection.combo.reset();

    auto baseFolder = program->soundFolder();

    auto list = GUIKIT::File::getFolderList(baseFolder + "tape/" + emulator->ident);

    for (auto& info : list) {
        driveLayout->tapeSelection.combo.append( info.name );
    }
}

auto AudioLayout::setDspEvent(SliderLayout* sliderLayout, std::string ident, float defaultVal) -> void {

    sliderLayout->slider.onChange = [this, sliderLayout, ident](unsigned position) {

        float val = (float)position / 100.0;

        _settings->set<float>(ident, val);

        sliderLayout->value.setText( GUIKIT::String::convertDoubleToString(val, 2) );

        emuThread->lock();
        audioManager->setAudioDsp();
        emuThread->unlock();
    };    
}

auto AudioLayout::initDsp(SliderLayout* sliderLayout, std::string ident, float defaultVal) -> void {
    auto val = _settings->get<float>(ident, defaultVal,{0.0, 1.0});
    unsigned _pos = (val + 0.005) * 100.0;
    sliderLayout->slider.setPosition(_pos);
    sliderLayout->value.setText(GUIKIT::String::convertDoubleToString(val, 2));
}

auto AudioLayout::initSeparation() -> void {
    unsigned l0 = panning.middle.leftMix.slider.position();
    unsigned l1 = panning.middle.rightMix.slider.position();
    unsigned r0 = panning.bottom.leftMix.slider.position();
    unsigned r1 = panning.bottom.rightMix.slider.position();

    if(((l0 + l1) == 100) && ((r0 + r1) == 100) && (l0 == r1) && (l1 == r0)) {
        unsigned pos = ((float)(l0 - 50) * 20) / 50.0 + 0.5;
        panning.top.separation.value.setText( std::to_string(pos * 5) + "%" );
        panning.top.separation.slider.setPosition( pos );
    }
}

auto AudioLayout::setSeparation() -> void {

    panning.top.separation.slider.onChange = [this](unsigned position) {

        unsigned val = 50 + ((position * 50) / 20);
        float v1 = (float)val / 100.0;
        float v2 = (float)(100 - val) / 100.0;

        _settings->set<float>("audio_panning_left0", v1);
        _settings->set<float>("audio_panning_left1", v2);
        _settings->set<float>("audio_panning_right0", v2);
        _settings->set<float>("audio_panning_right1", v1);

        panning.top.separation.value.setText( std::to_string(position * 5) + "%" );

        panning.middle.leftMix.slider.setPosition(val);
        panning.middle.rightMix.slider.setPosition(100 - val);
        panning.bottom.leftMix.slider.setPosition(100 - val);
        panning.bottom.rightMix.slider.setPosition(val);

        panning.middle.leftMix.value.setText( GUIKIT::String::convertDoubleToString(v1, 2) );
        panning.middle.rightMix.value.setText( GUIKIT::String::convertDoubleToString(v2, 2) );
        panning.bottom.leftMix.value.setText( GUIKIT::String::convertDoubleToString(v2, 2) );
        panning.bottom.rightMix.value.setText( GUIKIT::String::convertDoubleToString(v1, 2) );

        emuThread->lock();
        audioManager->setAudioDsp();
        emuThread->unlock();
    };
}

auto AudioLayout::translate() -> void {
    settingsLayout.translate();
    if (usbSidPicoLayout)
        usbSidPicoLayout->translate();
    moduleFrame.setText(trans->get("selection"));
    
    bass.setText(trans->get("Bass Boost"));
    bass.top.active.setText(trans->get("enable"));
    bass.top.frequency.name.setText(trans->get("Cutoff frequency",{}, true));
    bass.bottom.gain.name.setText(trans->get("Gain",{}, true));
    bass.bottom.reduceClipping.name.setText(trans->get("Reduce Clipping",{}, true));

    echo.setText(trans->get("Echo"));
    echo.top.active.setText(trans->get("enable"));
    echo.top.echoReverb.setText(trans->get("Echo Reverb"));
    echo.top.amp.name.setText(trans->getA("amplify", true));
    echo.bottom.delay.name.setText(trans->getA("delay", true));
    echo.bottom.feedback.name.setText(trans->getA("Feedback", true));

    reverb.setText(trans->get("Reverb"));
    reverb.top.active.setText(trans->get("enable"));
    reverb.top.wetTime.name.setText(trans->get("Wet Time",{}, true));
    reverb.top.dryTime.name.setText(trans->get("Dry Time",{}, true));
    reverb.bottom.damping.name.setText(trans->get("Damping",{}, true));
    reverb.bottom.roomWidth.name.setText(trans->get("Room Width",{}, true));
    reverb.bottom.roomSize.name.setText(trans->get("Room Size",{}, true));

    panning.setText(trans->get("Balance"));
    panning.top.active.setText(trans->get("enable"));
    panning.top.separation.name.setText(trans->get("Stereo Separation"));
    panning.middle.leftChannel.setText(trans->get("left Channel"));
    panning.middle.leftMix.name.setText(trans->get("mix left"));
    panning.middle.rightMix.name.setText(trans->get("mix right"));
    panning.bottom.rightChannel.setText(trans->get("right Channel"));
    panning.bottom.leftMix.name.setText(trans->get("mix left"));
    panning.bottom.rightMix.name.setText(trans->get("mix right"));

    driveLayout->setText( trans->get("Drive Noise") );
    driveLayout->floppyVolume.active.setText( trans->get("Floppy") );
    driveLayout->floppyVolumeExt.name.setText( trans->get(dynamic_cast<LIBC64::Interface*>(emulator) ? "3.5-inch" : "external") );
    driveLayout->floppySelection.label.setText( trans->get("Floppy Profile") );
    driveLayout->floppySelection.labelExt.setText( dynamic_cast<LIBC64::Interface*>(emulator)
        ? trans->get("Floppy 3.5-inch Profile") : trans->get("External Floppy Profile") );
    driveLayout->floppySelection.reload.setText( trans->get("Reload") );
    driveLayout->floppySelection.reload.setTooltip( trans->get("reload samples tooltip") );
    driveLayout->tapeVolume.active.setText( trans->get("Tape") );
    driveLayout->tapeSelection.label.setText( trans->get("Tape Profile") );
    driveLayout->tapeSelection.reload.setText( trans->get("Reload") );
    driveLayout->tapeSelection.reload.setTooltip( trans->get("reload samples tooltip") );
    driveLayout->tapeNoiseVolume.active.setText( trans->get("Tape Noise") );

    audioRecord.setText(trans->get("Audio Record"));
    audioRecord.location.label.setText( trans->getA("folder", true) );
    audioRecord.location.standard.setText(trans->get("default"));
    audioRecord.location.select.setText(trans->get("select"));

    audioRecord.type.label.setText( trans->getA("format", true) );
    audioRecord.type.mp3.setText(trans->getA("MP3"));
    audioRecord.type.wav.setText(trans->getA("WAV"));

    audioRecord.duration.useTimeLimit.setText(trans->get("Recording time"));
    audioRecord.duration.minutesSlider.name.setText(trans->get("Minutes",{}, true));
    audioRecord.duration.secondsSlider.name.setText(trans->get("Seconds",{}, true));
    audioRecord.duration.record.setText( trans->get( audioManager->record.run(emulator) ? "Stop" : "Record") );

    int layPos = 0;
    moduleList.setText(layPos++, 0, trans->get("Chip"));
    moduleList.setText(layPos++, 0, trans->get("Drives"));
    moduleList.setText(layPos, 0, trans->get("DSP"));
    moduleList.setRowTooltip(layPos++, trans->get("Digital Signal Processing"));
    moduleList.setText(layPos++, 0, trans->get("Audio Record"));
    

    volumeLayout.info.label.setText( trans->getA("volume") );

    unsigned neededWidth = driveLayout->floppySelection.label.minimumSize().width;
    neededWidth = std::max(neededWidth, driveLayout->tapeSelection.label.minimumSize().width);
    neededWidth = SliderLayout::scale({&driveLayout->floppyVolume, &driveLayout->floppyVolumeExt, &driveLayout->tapeVolume, &driveLayout->tapeNoiseVolume}, "300 %", neededWidth);

    driveLayout->floppySelection.children[ 0 ].size.width = neededWidth;
    driveLayout->tapeSelection.children[ 0 ].size.width = neededWidth;
}

auto AudioLayout::loadSettings() -> void {
    settingsLayout.updateWidgets();
    if (usbSidPicoLayout)
        usbSidPicoLayout->updateWidgets();

    initDsp( &echo.top.amp, "audio_echo_amp", 0.2 );
    initDsp( &echo.bottom.feedback, "audio_echo_feedback", 0.5 );

    auto echoDelay = _settings->get<unsigned>("audio_echo_delay", 200, {0, 1000});
    echo.bottom.delay.slider.setPosition(echoDelay / 10);
    echo.bottom.delay.value.setText( std::to_string(echoDelay) + " ms");

    initDsp( &reverb.top.dryTime, "audio_reverb_drytime", 0.43 );
    initDsp( &reverb.top.wetTime, "audio_reverb_wettime", 0.4 );
    initDsp( &reverb.bottom.damping, "audio_reverb_damping", 0.8 );
    initDsp( &reverb.bottom.roomWidth, "audio_reverb_roomwidth", 0.56 );
    initDsp( &reverb.bottom.roomSize, "audio_reverb_roomsize", 0.56 );
    
    initDsp( &panning.middle.leftMix, "audio_panning_left0", 1.0 );
    initDsp( &panning.middle.rightMix, "audio_panning_left1", 0.0 );
    initDsp( &panning.bottom.leftMix, "audio_panning_right0", 0.0 );
    initDsp( &panning.bottom.rightMix, "audio_panning_right1", 1.0 );

    initSeparation();
    
    panning.top.active.setChecked( _settings->get<bool>("audio_panning", false ) );
    echo.top.active.setChecked( _settings->get<bool>("audio_echo", false ) );
    reverb.top.active.setChecked( _settings->get<bool>("audio_reverb", false ) );
    bass.top.active.setChecked(_settings->get<bool>("audio_bass", false));

    auto bassFreq = _settings->get<unsigned>("audio_bass_freq", 200, {20, 200});
    bass.top.frequency.slider.setPosition(bassFreq - 20);
    bass.top.frequency.value.setText(std::to_string(bassFreq) + " Hz");

    auto bassGain = _settings->get<unsigned>("audio_bass_gain", 10, {0, 40});
    bass.bottom.gain.slider.setPosition(bassGain);
    bass.bottom.gain.value.setText(std::to_string(bassGain));

    auto bassReduceClipping = _settings->get<float>("audio_bass_clipping", 0.4, {0.0, 1.0});
    bass.bottom.reduceClipping.slider.setPosition((unsigned) (bassReduceClipping * 10.0));
    bass.bottom.reduceClipping.value.setText(GUIKIT::String::convertDoubleToString(bassReduceClipping, 1));
    
    updateRecordingPath();
    
    unsigned value = _settings->get<unsigned>( "audio_record_minutes", 0, {0, 120} );
    
    audioRecord.duration.minutesSlider.value.setText( std::to_string(value) );
    
    audioRecord.duration.minutesSlider.slider.setPosition( value );
    
    value = _settings->get<unsigned>( "audio_record_seconds", 0, {0, 59} );

    audioRecord.duration.record.setChecked( audioManager->record.run(emulator) );

    audioRecord.duration.secondsSlider.value.setText( std::to_string(value) );
    
    audioRecord.duration.secondsSlider.slider.setPosition( value ); 
                 
    audioRecord.duration.useTimeLimit.setChecked( _settings->get<bool>( "audio_record_timelimit", false) );

    auto recordingType = _settings->get<unsigned>("audio_record_type", 0);
    switch (recordingType) {
        case 0:
        default:
            audioRecord.type.mp3.setChecked();
            break;
        case 1:
            audioRecord.type.wav.setChecked();
            break;
    }

    driveLayout->floppyVolume.active.setChecked( _settings->get<bool>( "audio_floppy", false) );

    unsigned floppyVolume = _settings->get<unsigned>("audio_floppy_volume", 200u, {0u, 300u});

    driveLayout->floppyVolume.slider.setPosition( floppyVolume );

    driveLayout->floppyVolume.value.setText(std::to_string(floppyVolume) + " %");

    unsigned floppyVolumeExt = _settings->get<unsigned>("audio_floppy_volume_external", 200u, {0u, 300u});

    driveLayout->floppyVolumeExt.slider.setPosition( floppyVolumeExt );

    driveLayout->floppyVolumeExt.value.setText(std::to_string(floppyVolumeExt) + " %");

    std::string folder = audioManager->drive.getFloppyFolder(emulator, false);;

    driveLayout->floppySelection.combo.setSelectionByText( folder );

    folder = driveLayout->floppySelection.combo.text();

    _settings->set<std::string>(audioManager->drive.getFloppyFolderIdent(emulator, false), folder);

    folder = audioManager->drive.getFloppyFolder(emulator, true);

    driveLayout->floppySelection.comboExt.setSelectionByText( folder );

    folder = driveLayout->floppySelection.comboExt.text();

    _settings->set<std::string>(audioManager->drive.getFloppyFolderIdent(emulator, true), folder);

    driveLayout->tapeVolume.active.setChecked( _settings->get<bool>( "audio_tape", false) );

    unsigned tapeVolume = _settings->get<unsigned>("audio_tape_volume", 100u, {0u, 300u});

    driveLayout->tapeVolume.slider.setPosition( tapeVolume );

    driveLayout->tapeVolume.value.setText(std::to_string(tapeVolume) + " %");

    folder = _settings->get<std::string>("audio_tape_folder", "");

    driveLayout->tapeSelection.combo.setSelectionByText( folder );

    folder = driveLayout->tapeSelection.combo.text();

    _settings->set<std::string>("audio_tape_folder", folder);

    driveLayout->tapeNoiseVolume.active.setChecked( _settings->get<bool>( "audio_tape_noise", false) );

    unsigned tapeNoiseVolume = _settings->get<unsigned>("audio_tape_noise_volume", 100u, {0u, 300u});

    driveLayout->tapeNoiseVolume.slider.setPosition( tapeNoiseVolume );

    driveLayout->tapeNoiseVolume.value.setText(std::to_string(tapeNoiseVolume) + " %");

    updateVolumeSlider();

    updateVisibility();
}

auto AudioLayout::updateRecordingPath() -> void {
    std::string _recordPath = _settings->get<std::string>("audio_record_path", "");
    audioRecord.location.pathEdit.setText(FileHelper::generatedFolder(emulator, "audio_record_path", "recordings/audio", FileHelper::FLAG_VIEW));
    audioRecord.location.pathEdit.setEnabled(!_recordPath.empty());
}

auto AudioLayout::updateVisibility() -> void {
    echo.setEnabled( echo.top.active.checked() );
    echo.top.active.setEnabled();

    reverb.setEnabled( reverb.top.active.checked() );
    reverb.top.active.setEnabled();
    
    panning.setEnabled( panning.top.active.checked() );
    panning.top.active.setEnabled();
    
    bass.setEnabled( bass.top.active.checked() );
    bass.top.active.setEnabled();
    
    audioRecord.duration.setEnabled( audioRecord.duration.useTimeLimit.checked() );
    audioRecord.duration.useTimeLimit.setEnabled();
    audioRecord.duration.record.setEnabled();
}

auto AudioLayout::toggleRecord() -> void {
    audioRecord.duration.record.toggle();
}

auto AudioLayout::stopRecord() -> void {

    if (audioRecord.duration.record.checked()) {
        audioRecord.duration.record.setText( trans->get( "Record" ) );
        audioRecord.duration.record.setChecked(false);
    }
}

auto AudioLayout::selectViewAudioRecord() -> void {
    moduleList.setSelection(moduleList.rowCount() - 1);
    moduleSwitch.setSelection(moduleList.selection());
}

}
