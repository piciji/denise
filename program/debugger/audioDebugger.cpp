
#include "audioDebugger.h"
#include "../program.h"

AudioDebugger::AudioDebugger( Emulator::Interface* emulator )
: Debugger( emulator, Mode::Audio ) {
    build();
}

AudioDebugger::Chip::Top::Voice::Wave::Wave() {
    append(label, {0u, 0u}, 10);
    append(noise, {0u, 0u}, 5);
    append(pulse, {0u, 0u}, 5);
    append(saw, {0u, 0u}, 5);
    append(tri, {0u, 0u});
    noise.setReadonly(  );
    pulse.setReadonly(  );
    saw.setReadonly(  );
    tri.setReadonly(  );
    setAlignment( 0.5 );
}

AudioDebugger::Chip::Top::Voice::Frequency::Frequency() {
    append(label, {0u, 0u}, 10);
    append(edit, {~0u, 0u});
    edit.setEditable( false );
    setAlignment( 0.5 );
}

AudioDebugger::Chip::Top::Voice::PulseWidth::PulseWidth() {
    append(label, {0u, 0u}, 10);
    append(edit, {~0u, 0u});
    edit.setEditable( false );
    setAlignment( 0.5 );
}

AudioDebugger::Chip::Top::Voice::Adsr::Adsr() {
    append(label, {0u, 0u}, 10);
    append(editA, {~0u, 0u}, 10);
    append(editD, {~0u, 0u}, 10);
    append(editS, {~0u, 0u}, 10);
    append(editR, {~0u, 0u});
    editA.setEditable( false );
    editD.setEditable( false );
    editS.setEditable( false );
    editR.setEditable( false );
    setAlignment( 0.5 );
}

AudioDebugger::Chip::Top::Voice::Control::Control() {
    append(label, {0u, 0u}, 10);
    append(test, {~0u, 0u});
    append(ring, {~0u, 0u});
    append(sync, {~0u, 0u});
    append(gate, {~0u, 0u});
    test.setReadonly(  );
    ring.setReadonly(  );
    sync.setReadonly(  );
    gate.setReadonly(  );
    setAlignment( 0.5 );
}

AudioDebugger::Chip::Top::Voice::Voice() {
    append( wave, {~0u, 0u}, 10 );
    append( frequency, {~0u, 0u}, 10 );
    append( pulseWidth, {~0u, 0u}, 10 );
    append( adsr, {~0u, 0u}, 10 );
    append( control, {~0u, 0u} );
    setPadding( 10 );
}

AudioDebugger::Chip::Top::Top() {
    append( voices[0], {~0u, 0u}, 10 );
    append( voices[1], {~0u, 0u}, 10 );
    append( voices[2], {~0u, 0u} );
}

AudioDebugger::Chip::Bottom::Mixer::Mode::Mode() {
    append(label, {0u, 0u}, 10);
    append(highPass, {0u, 0u}, 10);
    append(bandPass, {0u, 0u}, 10);
    append(lowPass, {0u, 0u});
    lowPass.setReadonly(  );
    highPass.setReadonly(  );
    bandPass.setReadonly(  );

    setAlignment( 0.5 );
}

AudioDebugger::Chip::Bottom::Mixer::Filter::Filter() {
    append(label, {0u, 0u}, 10);
    append(voice3, {0u, 0u}, 10);
    append(voice2, {0u, 0u}, 10);
    append(voice1, {0u, 0u});
    voice1.setReadonly(  );
    voice2.setReadonly(  );
    voice3.setReadonly(  );

    setAlignment( 0.5 );
}

AudioDebugger::Chip::Bottom::Mixer::Params::Params() {
    append(labelCutoff, {0u, 0u}, 10);
    append(editCutoff, {50u, 0u}, 10);
    append(labelResonance, {0u, 0u}, 10);
    append(editResonance, {50u, 0u});
    editCutoff.setEditable( false );
    editResonance.setEditable( false );

    setAlignment( 0.5 );
}

AudioDebugger::Chip::Bottom::Mixer::Mixer() {
    append(mode, {0u, 0u}, 10);
    append(filter, {0u, 0u}, 10);
    append(params, {0u, 0u});
    setPadding( 10 );
}

AudioDebugger::Chip::Bottom::Misc::Volume::Volume() {
    append(label, {0u, 0u}, 10);
    append(edit, {50u, 0u}, 10);
    append(disableVoice3, {0u, 0u});
    edit.setEditable( false );
    disableVoice3.setReadonly(  );

    setAlignment( 0.5 );
}

AudioDebugger::Chip::Bottom::Misc::Pot::Pot() {
    append(labelX, {0u, 0u}, 10);
    append(editX, {50u, 0u}, 10);
    append(labelY, {0u, 0u}, 10);
    append(editY, {50u, 0u});
    editY.setEditable( false );
    editY.setEditable( false );

    setAlignment( 0.5 );
}

AudioDebugger::Chip::Bottom::Misc::Misc() {
    append( volume, {0u, 0u}, 10 );
    append( pot, {0u, 0u}, 10 );
    setPadding( 10 );
}

AudioDebugger::Chip::Bottom::Bottom() {
    append( spacerL, {~0u, 0u} );
    append( mixer, {0u, 0u}, 10 );
    append( misc, {0u, 0u} );
    append( spacerR, {~0u, 0u} );
}

AudioDebugger::Chip::Chip() {
    append( top, {~0u, 0u}, 10 );
    append( bottom, {~0u, 0u} );
    setMargin( 10 );
}

auto AudioDebugger::buildTheme() -> GUIKIT::Layout* {
    int i = 0;

    for (auto& chip : chips) {
        tab.appendHeader( "SID " + std::to_string( i + 1 ) );
        tab.setLayout(i, chip, {~0u, ~0u});

        i++;
    }

    tab.setMargin(10);
    tab.setSelection(0);

    return &tab;
}

auto AudioDebugger::updateTheme() -> void {
    if (emulator != activeEmulator)
        return;

    if (isAmiga()) {
       // LIBAMI::DebuggerSnapshot& snap = *static_cast<LIBAMI::DebuggerSnapshot*>(snapshot);
    } else {
        LIBC64::DebuggerSnapshot& snap = *static_cast<LIBC64::DebuggerSnapshot*>(snapshot);
        updateSID( snap );
    }
}

auto AudioDebugger::updateSID(LIBC64::DebuggerSnapshot& snap) -> void {
    int c = 0;
    for (auto& sid : snap.sids ) {
        auto& chip = chips[c++];

        chip.setEnabled( sid.active );
        if (!sid.active)
            continue;

        int v = 0;
        for (auto& voice : sid.voices) {
            auto& vL = chip.top.voices[v++];

            updateReg( vL.wave.noise, !!(voice.wave & 8) );
            updateReg( vL.wave.pulse, !!(voice.wave & 4) );
            updateReg( vL.wave.saw, !!(voice.wave & 2) );
            updateReg( vL.wave.tri, !!(voice.wave & 1) );

            updateReg( vL.frequency.edit, voice.frequency );
            updateReg( vL.pulseWidth.edit, voice.pulseWidth );

            updateReg( vL.adsr.editA, voice.attack );
            updateReg( vL.adsr.editD, voice.delay );
            updateReg( vL.adsr.editS, voice.sustain );
            updateReg( vL.adsr.editR, voice.release );

            updateReg( vL.control.test, !!(voice.control & 8) );
            updateReg( vL.control.ring, !!(voice.control & 4) );
            updateReg( vL.control.sync, !!(voice.control & 2) );
            updateReg( vL.control.gate, !!(voice.control & 1) );
        }

        auto& mL = chip.bottom.mixer;
        updateReg( mL.mode.highPass, !!(sid.filter.mode & 4) );
        updateReg( mL.mode.bandPass, !!(sid.filter.mode & 2) );
        updateReg( mL.mode.lowPass, !!(sid.filter.mode & 1) );

        updateReg( mL.filter.voice1, !!(sid.filter.voices & 1) );
        updateReg( mL.filter.voice2, !!(sid.filter.voices & 2) );
        updateReg( mL.filter.voice3, !!(sid.filter.voices & 4) );

        updateReg( mL.params.editCutoff, sid.filter.cutOff );
        updateReg( mL.params.editResonance, sid.filter.resonance );

        auto& mcL = chip.bottom.misc;
        updateReg( mcL.volume.edit, sid.volume );
        updateReg( mcL.volume.disableVoice3, (sid.filter.mode & 8) && ((sid.filter.voices & 4) == 0) );
        updateReg( mcL.pot.editX, sid.potX );
        updateReg( mcL.pot.editY, sid.potY );
    }
}

auto AudioDebugger::translateTheme() -> void {
    for (auto& chip : chips) {
        int v = 1;
        for (auto& voice : chip.top.voices) {
            voice.wave.label.setText( "Waveform" );
            voice.wave.noise.setText( "Noise" );
            voice.wave.pulse.setText( "Pulse" );
            voice.wave.saw.setText( "Saw" );
            voice.wave.tri.setText( "Triangle" );
            voice.frequency.label.setText( "Frequency" );
            voice.pulseWidth.label.setText( "PulseWidth" );
            voice.adsr.label.setText( "ADSR" );
            voice.control.label.setText( "Control" );
            voice.control.test.setText( "Test" );
            voice.control.ring.setText( "Ring" );
            voice.control.sync.setText( "Sync" );
            voice.control.gate.setText( "Gate" );

            voice.setText( "Voice " + std::to_string(v++)  );

            GUIKIT::Layout::alignChildWidth({&voice.wave, &voice.frequency, &voice.pulseWidth, &voice.adsr, &voice.control});
        }

        auto& mixer = chip.bottom.mixer;
        mixer.setText( "Mixer / Filter" );

        mixer.mode.label.setText( "Mode" );
        mixer.mode.lowPass.setText( "Low Pass" );
        mixer.mode.highPass.setText( "High Pass" );
        mixer.mode.bandPass.setText( "Band Pass" );

        mixer.filter.label.setText( "Apply To" );
        mixer.filter.voice1.setText( "Voice 1" );
        mixer.filter.voice2.setText( "Voice 2" );
        mixer.filter.voice3.setText( "Voice 3" );

        mixer.params.labelCutoff.setText( "Cutoff" );
        mixer.params.labelResonance.setText( "Resonance" );

        auto& misc = chip.bottom.misc;
        misc.setText( "Volume / Pot" );
        misc.volume.label.setText( "Volume" );
        misc.volume.disableVoice3.setText( "Voice 3 inactive" );

        misc.pot.labelX.setText( "PotX" );
        misc.pot.labelY.setText( "PotY" );

        GUIKIT::Layout::alignChildWidth({&mixer.mode, &mixer.filter, &mixer.params});
        GUIKIT::Layout::alignChildWidth({&misc.volume, &misc.pot});
    }
}

auto AudioDebugger::initTheme() -> void {
    emulator->debuggerAdd( DebuggerTheme::Audio, DebuggerAction::None, 0);
}

auto AudioDebugger::closeTheme() -> void {
    emulator->debuggerRemove( DebuggerTheme::Audio, DebuggerAction::None);
}

auto AudioDebugger::saveIdent() -> std::string {
    return "debugger_video";
}

auto AudioDebugger::titleIdent() -> std::string {
    return emulator->ident + (isC64() ? " Debugger SID" : " Debugger Paula");
}
