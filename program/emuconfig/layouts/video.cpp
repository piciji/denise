
VideoModeLayout::VideoModeLayout(bool withSpectrum) {
    
    if (withSpectrum) {
        append(palette,{0u, 0u}, 10);
        append(spectrum,{0u, 0u}, 20);
    }
    
    append(crtNone,{0u, 0u}, 10);
    append(crtCpu,{0u, 0u}, 10);
    append(crtGpu,{0u, 0u});    
    	
    append(spacer,{~0u, 0u});
    append(reset,{0u, 0u});

    GUIKIT::RadioBox::setGroup(palette, spectrum);
    GUIKIT::RadioBox::setGroup(crtNone, crtCpu, crtGpu);    

    setAlignment(0.5);
}

VideoOptionLayout::VideoOptionLayout(bool withSpectrum) {
    
    if (withSpectrum) {
        append(newLuma, {0u, 0u}, 10);    	    
        append(crtRealGamma, {0u, 0u}, 10);    
    }
	
	append(linearInterpolation, {0u, 0u});
    
    setAlignment(0.5);
}

VideoBaseLayout::VideoBaseLayout(bool withSpectrum) :
mode(withSpectrum),
option(withSpectrum),
phase("°", false) 
{
    append(mode, {~0u, 0u}, 2);
    append(option, {~0u, 0u}, 2);
        
    if (withSpectrum)
        append(phase, {~0u, 0u}, 2);
        
    append(saturation, {~0u, 0u}, 2);        
    append(contrast, {~0u, 0u}, 2);     
    append(brightness, {~0u, 0u}, 2);
    append(gamma, {~0u, 0u});    

    saturation.slider.setLength(201);
    gamma.slider.setLength(251);
    brightness.slider.setLength(201);
    contrast.slider.setLength(201);
    phase.slider.setLength(361);
    
    setFont(GUIKIT::Font::system("bold"));    
    setPadding(8);
}

VideoCrtLayout::VideoCrtLayout() : 
phaseError("°", true),
hanoverBars("%", true),
scanlines("%", true),
blur("%", true)
{
    append(phaseError,{~0u, 0u}, 2);
    append(hanoverBars,{~0u, 0u}, 2);
    append(scanlines,{~0u, 0u}, 2);
    append(blur,{~0u, 0u});
    
    phaseError.slider.setLength(181); // -45° <-> 45°  ( 0.5 steps )
	hanoverBars.slider.setLength(201); // saturation change -100% <-> 100%
	scanlines.slider.setLength(101);
	blur.slider.setLength(101);
    
    setFont(GUIKIT::Font::system("bold"));    
    setPadding(8);
}

VideoHFLayout::VideoHFLayout() : 
lumaRise("px", true),
lumaFall("px", true)
{
	append(lumaRise,{~0u, 0u}, 2);
    append(lumaFall,{~0u, 0u});
	
	lumaRise.slider.setLength(31);
	lumaFall.slider.setLength(31);
    
    setFont(GUIKIT::Font::system("bold"));    
    setPadding(8);
}

VideoFirSharpLayout::VideoFirSharpLayout() {
    
    append(sharpLeft, {0u, 0u}, 10);
    append(natural, {0u, 0u}, 10);
    append(sharpRight, {0u, 0u});
    
    GUIKIT::RadioBox::setGroup( sharpLeft, natural, sharpRight );
    
    setAlignment(0.5);
}

VideoGpuOptionLayout::VideoGpuOptionLayout() {
    append(hires, {0u, 0u}, 15);
    append(distortionHires, {0u, 0u});
    
    setAlignment(0.5);
}

VideoMaskTypeLayout::VideoMaskTypeLayout() {
    append(type, {175u, 0u}, 10);
    append(apertureMask, {0u, 0u}, 10);
    append(shadowMask, {0u, 0u}, 10);
    append(slotMask, {0u, 0u});
    
    GUIKIT::RadioBox::setGroup( apertureMask, shadowMask, slotMask );
    
    setAlignment(0.5);
}

VideoGpuBaseLayout::VideoGpuBaseLayout() :
firFilter("", false),
luminance("%", false),
lightFromCenter("%", true) {    
    append(option, {~0u, 0u}, 3);
    append(firSharp, {~0u, 0u}, 2);
    append(firFilter, {~0u, 0u}, 2);
    append(lightFromCenter, {~0u, 0u}, 2);
    append(luminance, {~0u, 0u});  
    
    firFilter.slider.setLength(11);
    lightFromCenter.slider.setLength(301);
    luminance.slider.setLength(501);
    
    setPadding(8);
    setFont(GUIKIT::Font::system("bold"));  
}

VideoMaskLayout::VideoMaskLayout() :
level("%", true),
pitch("mm", false),
dpi("dpi", false),
luminance("%", false) {
    append(level,{~0u, 0u}, 2);
    append(luminance,{~0u, 0u}, 2);
    append(type,{~0u, 0u}, 2);
    append(dpi,{~0u, 0u}, 2);
    append(pitch,{~0u, 0u});

    level.slider.setLength(101);
    dpi.slider.setLength(201);
    pitch.slider.setLength(101);
    luminance.slider.setLength(501);

    setPadding(8);
    setFont(GUIKIT::Font::system("bold"));
}

VideoBloomLayout::VideoBloomLayout() :
glow("%", true),
radius("px", false),
variance("", false),
weight("", true)
{
	append(glow, {~0u, 0u}, 2);
    append(weight, {~0u, 0u}, 2);
    append(radius, {~0u, 0u}, 2);
	append(variance, {~0u, 0u});    

    glow.slider.setLength(201);
    radius.slider.setLength(6);
	variance.slider.setLength(111);
    weight.slider.setLength(301);    
	
    setPadding(8);	
    setFont(GUIKIT::Font::system("bold"));    
}

VideoCrtGlitchLayout::VideoCrtGlitchLayout() :
lumaNoise("%", true),
chromaNoise("%", true),
radialDistortion("%", true),
randomLineOffset("%", true) {

    append(lumaNoise,{~0u, 0u}, 2);
    append(chromaNoise,{~0u, 0u}, 2);
    append(randomLineOffset,{~0u, 0u}, 2);
    append(radialDistortion,{~0u, 0u}); 
    
    lumaNoise.slider.setLength(1001);
    chromaNoise.slider.setLength(1001);
    randomLineOffset.slider.setLength(1001);
    radialDistortion.slider.setLength(101);
    
    setPadding(8);
    setFont(GUIKIT::Font::system("bold"));  
}

VideoVicIIGlitchLayout::VideoVicIIGlitchLayout() :
aec("%", true),
ba("%", true),
phi0("%", true),
ras("%", true),
cas("%", true) {
    append(aec,{~0u, 0u}, 2);
    append(ba,{~0u, 0u}, 2);
    append(phi0,{~0u, 0u}, 2);
    append(ras,{~0u, 0u}, 2);
    append(cas,{~0u, 0u}, 5);
    append(toggleAll,{0u, 0u});

    aec.slider.setLength(1001);
    ba.slider.setLength(1001);
    phi0.slider.setLength(1001);
    ras.slider.setLength(1001);
    cas.slider.setLength(1001);

    setPadding(8);
    setFont(GUIKIT::Font::system("bold"));
}

VideoLayout::VideoLayout(TabWindow* tabWindow) :
base( dynamic_cast<LIBC64::Interface*>(tabWindow->emulator) )
{    
    this->tabWindow = tabWindow;
    this->emulator = tabWindow->emulator;
    
    appendHeader("");
    appendHeader("");
    appendHeader("");

    setMargin(10);
    setPadding(10);

    tab1.append(base, {~0u, 0u}, 5);
    tab1.append(crt, {~0u, 0u}, 5);
	tab1.append(hf, {~0u, 0u});
    
    tab2.append(gpuBase, {~0u, 0u}, 10);
    tab2.append(mask, {~0u, 0u}, 10);
	tab2.append(bloom, {~0u, 0u});    
        
    tab3.append(crtGlitch, {~0u, 0u}, 10 );
    if (dynamic_cast<LIBC64::Interface*>(emulator))
        tab3.append( vicIIGlitch, {~0u, 0u} );                    
       
    setLayout(0, tab1, {~0u, ~0u});
    setLayout(1, tab2, {~0u, ~0u});           
    setLayout(2, tab3, {~0u, ~0u});            
    
    setSliderAction<unsigned>( &base.gamma, "gamma", [this](unsigned value) { vManager()->setGamma( value ); },
          [this](unsigned position) { return position + 30; } );                
    setSliderAction<unsigned>( &base.saturation, "saturation", [this](unsigned value) { vManager()->setSaturation( value ); } );
    setSliderAction<unsigned>( &base.brightness, "brightness", [this](unsigned value) { vManager()->setBrightness( value ); } );
    setSliderAction<unsigned>( &base.contrast, "contrast", [this](unsigned value) { vManager()->setContrast( value ); } );
    setSliderAction<int>( &base.phase, "phase", [this](int value) { vManager()->setPhase( value ); }, [this](unsigned position) { return (int)position - 180; } );
    setSliderAction<unsigned>( &crt.scanlines, "scanlines", [this](unsigned value) { vManager()->setScanlines( value ); },
        [this](unsigned position) { return std::max(position, 1u); } );
    setSliderAction<unsigned>( &crt.blur, "blur", [this](unsigned value) { vManager()->setBlur( value ); } );
    setSliderAction<float>( &crt.phaseError, "phase_error", [this](float value) { vManager()->setPhaseError( value ); },
        [this](unsigned position) { return (float)((int)position - 90) / 2.0f; } );          
    setSliderAction<int>( &crt.hanoverBars, "hanover_bars", [this](int value) { vManager()->setHanoverBars( value ); }, [this](unsigned position) { return (int)position - 100; } );
    setSliderAction<float>( &mask.pitch, "mask_pitch", [this](float value) { vManager()->setMaskPitch( value ); },
        [this](unsigned position) { return (float)position / 100.0f; } );  
    setSliderAction<unsigned>( &mask.dpi, "mask_dpi", [this](unsigned value) { vManager()->setMaskDpi( value ); } );
    setSliderAction<unsigned>( &mask.level, "mask_level", [this](unsigned value) { vManager()->setMaskLevel( value ); },
        [this](unsigned position) { return std::max(position, 1u); } );
    setSliderAction<unsigned>( &mask.luminance, "mask_luminance", [this](unsigned value) { vManager()->setMaskLuminance( value ); } );
    setSliderAction<unsigned>( &gpuBase.firFilter, "fir_filter_length", [this](unsigned value) { vManager()->setFirFilterLength( value ); },
        [this](unsigned position) { return position * 2 + 1; } );  
    setSliderAction<float>( &crtGlitch.lumaNoise, "luma_noise", [this](float value) { vManager()->setLumaNoise( value ); },
        [this](unsigned position) { return (float)std::max(position, 1u) / 10.0f; });
    setSliderAction<float>( &crtGlitch.chromaNoise, "chroma_noise", [this](float value) { vManager()->setChromaNoise( value ); },
        [this](unsigned position) { return (float)std::max(position, 1u) / 10.0f; } );
    setSliderAction<unsigned>( &crtGlitch.radialDistortion, "radial_distortion", [this](unsigned value) { vManager()->setRadialDistortion( value ); },
        [this](unsigned position) { return std::max(position, 1u); } );
    setSliderAction<float>( &vicIIGlitch.aec, "aec_glitch", [this](float value) { vManager()->setAecGlitch( value ); },
        [this](unsigned position) { return (float)std::max(position, 1u) / 10.0f; } );
    setSliderAction<float>( &vicIIGlitch.ba, "ba_glitch", [this](float value) { vManager()->setBaGlitch( value ); },
        [this](unsigned position) { return (float)std::max(position, 1u) / 10.0f; } );
    setSliderAction<float>( &vicIIGlitch.phi0, "phi0_glitch", [this](float value) { vManager()->setPhi0Glitch( value ); },
        [this](unsigned position) { return (float)std::max(position, 1u) / 10.0f; } );
    setSliderAction<float>( &vicIIGlitch.ras, "ras_glitch", [this](float value) { vManager()->setRasGlitch( value ); },
        [this](unsigned position) { return (float)std::max(position, 1u) / 10.0f; } );
    setSliderAction<float>( &vicIIGlitch.cas, "cas_glitch", [this](float value) { vManager()->setCasGlitch( value ); },
        [this](unsigned position) { return (float)std::max(position, 1u) / 10.0f; } );
    setSliderAction<float>( &hf.lumaRise, "luma_rise", [this](float value) { vManager()->setLumaRise( value ); },
        [this](unsigned position) { return ((float)std::max(position, 1u) / 10.0f) + 1.0f; } );
    setSliderAction<float>( &hf.lumaFall, "luma_fall", [this](float value) { vManager()->setLumaFall( value ); },
        [this](unsigned position) { return ((float)std::max(position, 1u) / 10.0f) + 1.0f; } );
    setSliderAction<unsigned>( &gpuBase.lightFromCenter, "light_from_center", [this](unsigned value) { vManager()->setLightFromCenter( value ); },
        [this](unsigned position) { return std::max(position, 1u); } );
    setSliderAction<unsigned>( &gpuBase.luminance, "luminance", [this](unsigned value) { vManager()->setLuminance( value ); } );
	setSliderAction<unsigned>( &bloom.glow, "bloom_glow", [this](unsigned value) { vManager()->setBloomGlow( value ); },
        [this](unsigned position) { return std::max(position, 1u); } );
	setSliderAction<float>( &bloom.variance, "bloom_variance", [this](float value) { vManager()->setBloomVariance( value ); },
		[this](unsigned position) { return ((float)position / 10.0f) + 1.0f; } ); 
	setSliderAction<unsigned>( &bloom.radius, "bloom_radius", [this](unsigned value) { vManager()->setBloomRadius( value ); },
		[this](unsigned position) { return position + 1; } ); 
	setSliderAction<float>( &bloom.weight, "bloom_weight", [this](float value) { vManager()->setBloomWeight( value ); },
		[this](unsigned position) { return (float)std::max(position, 1u) / 100.0f; } );
    setSliderAction<float>( &crtGlitch.randomLineOffset, "random_line_offset", [this](float value) { vManager()->setRandomLineOffset( value ); },
        [this](unsigned position) { return (float)std::max(position, 1u) / 100.0f; } );
    
    base.option.newLuma.onToggle = [this](bool checked) {
        _settings->set<bool>( "video_new_luma" + this->sliderIdent(), checked);
        vManager()->updateData<bool>("video_new_luma", checked);
    };
    
    base.option.crtRealGamma.onToggle = [this](bool checked) {
        _settings->set<bool>( "video_crt_real_gamma" + this->sliderIdent(), checked);
        vManager()->updateData<bool>("video_crt_real_gamma", checked);
    };
	
	base.option.linearInterpolation.onToggle = [this](bool checked) {
		_settings->set<unsigned>("video_filter", checked ? 1 : 0 );
        emuThread->lock();
        program->setVideoFilter();
        emuThread->unlock();
	};
	
	base.option.linearInterpolation.setChecked( _settings->get<unsigned>("video_filter", 1u, {0u, 1u}) );
        
    mask.type.apertureMask.onActivate = [this]() {
        _settings->set<unsigned>( "video_mask_type" + this->sliderIdent(), (unsigned)VideoManager::MaskType::Aperture);
        vManager()->updateData<unsigned>("video_mask_type", (unsigned)VideoManager::MaskType::Aperture);
    };

    mask.type.shadowMask.onActivate = [this]() {
        _settings->set<unsigned>( "video_mask_type" + this->sliderIdent(), (unsigned)VideoManager::MaskType::ShadowMask);
        vManager()->updateData<unsigned>("video_mask_type", (unsigned)VideoManager::MaskType::ShadowMask);
    };
    
    mask.type.slotMask.onActivate = [this]() {
        _settings->set<unsigned>( "video_mask_type" + this->sliderIdent(), (unsigned)VideoManager::MaskType::SlotMask);
        vManager()->updateData<unsigned>("video_mask_type", (unsigned)VideoManager::MaskType::SlotMask);
    };
    
    base.mode.reset.onActivate = [this]() {
        vManager()->resetSettings();
        emuThread->lock();
        updatePresets();
        emuThread->unlock();
    };
    
    base.mode.palette.onActivate = [this]() {
        _settings->set<bool>( "video_spectrum", false);
        emuThread->lock();
        updatePresets();
        emuThread->unlock();
    };
    
    base.mode.spectrum.onActivate = [this]() {
        _settings->set<bool>("video_spectrum", true);
        emuThread->lock();
        updatePresets();
        emuThread->unlock();
    };       

    base.mode.crtNone.onActivate = [this]() {
        _settings->set<unsigned>("video_crt", (unsigned)VideoManager::CrtMode::None);
        emuThread->lock();
        program->fastForward( false );
        updatePresets();
        emuThread->unlock();
    };
    
    base.mode.crtCpu.onActivate = [this]() {
        _settings->set<unsigned>("video_crt", (unsigned)VideoManager::CrtMode::Cpu);
        emuThread->lock();
        program->fastForward( false );
		updatePresets();
        emuThread->unlock();
    };
    
    base.mode.crtGpu.onActivate = [this]() {
        _settings->set<unsigned>("video_crt", (unsigned)VideoManager::CrtMode::Gpu);
        emuThread->lock();
        program->fastForward( false );
		updatePresets();
        emuThread->unlock();
    };    
    
    gpuBase.option.distortionHires.onToggle = [this](bool checked) {
        _settings->set<bool>("video_distortion_hires" + this->sliderIdent(), checked);
        vManager()->updateData<bool>("video_distortion_hires", checked);
    };
    
    gpuBase.option.hires.onToggle = [this](bool checked) {
        _settings->set<bool>("video_hires" + this->sliderIdent(), checked);
        vManager()->updateData<bool>("video_hires", checked);
    };
	
    gpuBase.firSharp.sharpLeft.onActivate = [this]() {
        
        _settings->set<int>("video_fir_filter_sharp" + this->sliderIdent(), -1);
        vManager()->updateData<int>("video_fir_filter_sharp", -1);
    };

    gpuBase.firSharp.sharpRight.onActivate = [this]() {

        _settings->set<int>("video_fir_filter_sharp" + this->sliderIdent(), 1);
        vManager()->updateData<int>("video_fir_filter_sharp", 1);
    };

    gpuBase.firSharp.natural.onActivate = [this]() {

        _settings->set<int>("video_fir_filter_sharp" + this->sliderIdent(), 0);
        vManager()->updateData<int>("video_fir_filter_sharp", 0);
    };
    
    vicIIGlitch.toggleAll.onActivate = [this]() {
        
        bool b1 = vicIIGlitch.aec.active.checked();
        bool b2 = vicIIGlitch.ba.active.checked();
        bool b3 = vicIIGlitch.phi0.active.checked();
        bool b4 = vicIIGlitch.ras.active.checked();
        bool b5 = vicIIGlitch.cas.active.checked();
        bool _checked = b1 || b2 || b3 || b4 || b5;        
        
        vicIIGlitch.aec.active.setChecked( !_checked );
        vicIIGlitch.ba.active.setChecked( !_checked );
        vicIIGlitch.phi0.active.setChecked( !_checked );
        vicIIGlitch.ras.active.setChecked( !_checked );
        vicIIGlitch.cas.active.setChecked( !_checked );

        vicIIGlitch.aec.active.onToggle( !_checked );
        vicIIGlitch.ba.active.onToggle( !_checked );
        vicIIGlitch.phi0.active.onToggle( !_checked );
        vicIIGlitch.ras.active.onToggle( !_checked );
        vicIIGlitch.cas.active.onToggle( !_checked );
    };

    loadSettings(true);
}

template<typename T> auto VideoLayout::setSliderAction( SliderLayout* layout, std::string baseIdent, std::function<void ( T value )> callBack, std::function<T ( unsigned position )> callTransfer ) -> void {
    		
    if (layout->withActivator)
        layout->active.onToggle = [this, layout, baseIdent, callBack, callTransfer](bool checked) {
            _settings->set<bool>("video_" + baseIdent + "_use" + this->sliderIdent(), checked);
            layout->slider.setEnabled(checked);
            if (layout == &mask.level) {                
                mask.setEnabled( checked );
                layout->active.setEnabled();
                gpuBase.luminance.setEnabled( !checked );
            } else if (layout == &crtGlitch.radialDistortion) {
                gpuBase.option.distortionHires.setEnabled( checked );
            } else if (layout == &bloom.glow) {
                bloom.setEnabled( checked );
                layout->active.setEnabled();	
				bloom.weight.slider.setEnabled( bloom.weight.active.checked() );
			}
            
            unsigned position = layout->slider.position();
			T value = callTransfer( position );

            vManager()->updateData(baseIdent, checked ? value : T(0));
        };

    layout->slider.onChange = [this, layout, baseIdent, callBack, callTransfer](unsigned position) {
		T value = callTransfer( position );	
        auto unit = layout->unit;
        auto roundDigits = (layout == &mask.pitch || layout == &crtGlitch.randomLineOffset || layout == &bloom.weight) ? 2 : 1;
		
        _settings->set<T>("video_" + baseIdent + this->sliderIdent(), value);
		
		if (std::is_same<T, float>::value)
			layout->value.setText( GUIKIT::String::formatFloatingPoint(value, roundDigits) + " " + unit);
		else
			layout->value.setText( std::to_string(value) + " " + unit);

        if (layout->withActivator) {
            bool checked = layout->active.checked();
            vManager()->updateData(baseIdent, checked ? value : T(0));
        } else {
            vManager()->updateData(baseIdent, value);
        }
    };
}

auto VideoLayout::updatePresets(bool reloadDriver) -> void {
    
    auto [VPARAMS] = VideoManager::getInstance( emulator )->getSettings( );
    
    if (videoDriver && reloadDriver)
        VideoManager::getInstance( emulator )->reloadSettings();
    
	base.option.newLuma.setChecked( _newLuma );
    base.option.crtRealGamma.setChecked( _crtRealGamma );	
    base.saturation.slider.setPosition(_saturation);
    base.saturation.value.setText(std::to_string(_saturation) + " %");
    base.gamma.slider.setPosition(_gamma - 30 );
    base.gamma.value.setText( std::to_string(_gamma) + " %" );
    base.brightness.slider.setPosition(_brightness);
    base.brightness.value.setText(std::to_string(_brightness) + " %");   
    base.contrast.slider.setPosition(_contrast);
    base.contrast.value.setText(std::to_string(_contrast) + " %");
    base.phase.slider.setPosition(_phase + 180);
    base.phase.value.setText(std::to_string(_phase) + " °");           
	// crt
    crt.phaseError.active.setChecked( _usePhaseError );	
    crt.phaseError.slider.setPosition( int(_phaseError * 2.0) + 90);
    crt.phaseError.value.setText( GUIKIT::String::formatFloatingPoint(_phaseError, 1) + " °");
    crt.hanoverBars.active.setChecked( _useHanoverBars );	
    crt.hanoverBars.slider.setPosition( _hanoverBars + 100 );
    crt.hanoverBars.value.setText( std::to_string(_hanoverBars) + " %" );	
	crt.scanlines.active.setChecked( _useScanlines );	
    crt.scanlines.slider.setPosition( _scanlines );
    crt.scanlines.value.setText( std::to_string(_scanlines) + " %" );	
    crt.blur.active.setChecked( _useBlur );	
    crt.blur.slider.setPosition( _blur );
    crt.blur.value.setText( std::to_string(_blur) + " %" );	
	hf.lumaRise.active.setChecked( _useLumaRise );	
    hf.lumaRise.slider.setPosition( (unsigned)((_lumaRise - 1.0) * 10.0) );
    hf.lumaRise.value.setText( GUIKIT::String::formatFloatingPoint(_lumaRise, 1) + " px" );
    hf.lumaFall.active.setChecked( _useLumaFall );	
    hf.lumaFall.slider.setPosition( (unsigned)((_lumaFall - 1.0) * 10.0) );
    hf.lumaFall.value.setText( GUIKIT::String::formatFloatingPoint(_lumaFall, 1) + " px" );

    // shader features    
    crtGlitch.chromaNoise.active.setChecked( _useChromaNoise );	
    crtGlitch.chromaNoise.slider.setPosition( (unsigned)(_chromaNoise * 10.0) );
    crtGlitch.chromaNoise.value.setText( GUIKIT::String::formatFloatingPoint(_chromaNoise, 1) + " %" );
    crtGlitch.lumaNoise.active.setChecked(_useLumaNoise);
    crtGlitch.lumaNoise.slider.setPosition((unsigned)(_lumaNoise * 10.0));
    crtGlitch.lumaNoise.value.setText(GUIKIT::String::formatFloatingPoint(_lumaNoise, 1) + " %");    
    crtGlitch.radialDistortion.active.setChecked( _useRadialDistortion );	
    crtGlitch.radialDistortion.slider.setPosition( _radialDistortion );
    crtGlitch.radialDistortion.value.setText( std::to_string(_radialDistortion) + " %" );
    crtGlitch.randomLineOffset.active.setChecked( _useRandomLineOffset );	
    crtGlitch.randomLineOffset.slider.setPosition( (unsigned)(_randomLineOffset * 100.0) );
    crtGlitch.randomLineOffset.value.setText( GUIKIT::String::formatFloatingPoint(_randomLineOffset, 2) + " %" );   
    
	mask.level.active.setChecked( _useMaskLevel );
    mask.level.slider.setPosition( _maskLevel );
    mask.level.value.setText( std::to_string(_maskLevel) + " %" );
    mask.luminance.slider.setPosition(_maskLuminance);
    mask.luminance.value.setText(std::to_string(_maskLuminance) + " %");
    mask.pitch.slider.setPosition( _maskPitch * 100.0 );
    mask.pitch.value.setText( GUIKIT::String::formatFloatingPoint(_maskPitch, 2) + " mm" );
    mask.dpi.slider.setPosition( _maskDpi );
    mask.dpi.value.setText( std::to_string(_maskDpi) + " dpi" );

	bloom.glow.active.setChecked( _useBloomGlow );
    bloom.glow.slider.setPosition( _bloomGlow );
    bloom.glow.value.setText( std::to_string( _bloomGlow ) + " %" );
	bloom.weight.active.setChecked( _useBloomWeight );
    bloom.weight.slider.setPosition( _bloomWeight * 100.0 );	
    bloom.weight.value.setText( GUIKIT::String::formatFloatingPoint( _bloomWeight, 2 ) );
    bloom.variance.slider.setPosition( (unsigned)((_bloomVariance - 1.0) * 10.0) );
    bloom.variance.value.setText( GUIKIT::String::formatFloatingPoint( _bloomVariance, 1 ) );
    bloom.radius.slider.setPosition( _bloomRadius - 1 );
    bloom.radius.value.setText( std::to_string( _bloomRadius ) + " px" );
	
    vicIIGlitch.aec.active.setChecked( _useAecGlitch );	
    vicIIGlitch.aec.slider.setPosition( (unsigned)(_aecGlitch * 10.0) );
    vicIIGlitch.aec.value.setText( GUIKIT::String::formatFloatingPoint(_aecGlitch, 1) + " %" );
    vicIIGlitch.ba.active.setChecked( _useBaGlitch );	
    vicIIGlitch.ba.slider.setPosition( (unsigned)(_baGlitch * 10.0) );
    vicIIGlitch.ba.value.setText( GUIKIT::String::formatFloatingPoint(_baGlitch, 1) + " %" );
    vicIIGlitch.phi0.active.setChecked( _usePhi0Glitch );	
    vicIIGlitch.phi0.slider.setPosition( (unsigned)(_phi0Glitch * 10.0) );
    vicIIGlitch.phi0.value.setText( GUIKIT::String::formatFloatingPoint(_phi0Glitch, 1) + " %" );
    vicIIGlitch.ras.active.setChecked( _useRasGlitch );	
    vicIIGlitch.ras.slider.setPosition( (unsigned)(_rasGlitch * 10.0) );
    vicIIGlitch.ras.value.setText( GUIKIT::String::formatFloatingPoint(_rasGlitch, 1) + " %" );
    vicIIGlitch.cas.active.setChecked( _useCasGlitch );	
    vicIIGlitch.cas.slider.setPosition( (unsigned)(_casGlitch * 10.0) );
    vicIIGlitch.cas.value.setText( GUIKIT::String::formatFloatingPoint(_casGlitch, 1) + " %" );
    
    gpuBase.option.distortionHires.setChecked( _distortionHires );
    gpuBase.option.hires.setChecked( _hires );
    gpuBase.luminance.slider.setPosition( _luminance );
    gpuBase.luminance.value.setText( std::to_string(_luminance) + " %" );
    gpuBase.lightFromCenter.active.setChecked( _useLightFromCenter );	
    gpuBase.lightFromCenter.slider.setPosition( _lightFromCenter );
    gpuBase.lightFromCenter.value.setText( std::to_string(_lightFromCenter) + " %" );    
    gpuBase.firFilter.slider.setPosition( (unsigned)(_firFilterLength / 2) );
    gpuBase.firFilter.value.setText( std::to_string( _firFilterLength ) );
    
    if (_firFilterSharp == -1)
        gpuBase.firSharp.sharpLeft.setChecked();
    else if (_firFilterSharp == 1)
        gpuBase.firSharp.sharpRight.setChecked();
    else
        gpuBase.firSharp.natural.setChecked();
        
    if ( _maskType == (unsigned)VideoManager::MaskType::ShadowMask )    
        mask.type.shadowMask.setChecked();
    else if ( _maskType == (unsigned)VideoManager::MaskType::SlotMask )    
        mask.type.slotMask.setChecked();
    else
        mask.type.apertureMask.setChecked();    
	
	updateVisibillity();
}

auto VideoLayout::updateVisibillity() -> void {
	
	bool _pal = emulator->getRegionEncoding() == Emulator::Interface::Region::Pal;
	
	if (base.mode.spectrum.checked()) {
        base.phase.setEnabled(true);
        base.option.newLuma.setEnabled(true);
    } else {
        base.phase.setEnabled(false);
        base.option.newLuma.setEnabled(false);        
    }
		
    bool crtChecked = base.mode.crtCpu.checked() || base.mode.crtGpu.checked();
    bool crtGpuChecked = base.mode.crtGpu.checked();        
    
	crt.setEnabled( crtChecked );
    hf.setEnabled( crtChecked );
    
    if (crtChecked) {
        crt.phaseError.slider.setEnabled( crt.phaseError.active.checked() );
        crt.hanoverBars.setEnabled( _pal );
        crt.hanoverBars.slider.setEnabled( _pal && crt.hanoverBars.active.checked() );	
        crt.scanlines.slider.setEnabled( crt.scanlines.active.checked() );
        crt.blur.slider.setEnabled(  crt.blur.active.checked() );
        hf.lumaRise.slider.setEnabled( hf.lumaRise.active.checked() );
        hf.lumaFall.slider.setEnabled( hf.lumaFall.active.checked() );
    }
    
    base.option.crtRealGamma.setEnabled( crtChecked && base.mode.palette.checked() && _pal );
	
    if (videoDriver->shaderFormat() != DRIVER::Video::ShaderType::GLSL) {
        if (videoDriver->shaderFormat() == DRIVER::Video::ShaderType::HLSL) {
            if(crtGpuChecked) {
                base.mode.crtCpu.setChecked();
                _settings->set<unsigned>("video_crt", (unsigned)VideoManager::CrtMode::Cpu);
            }
			base.mode.crtGpu.setEnabled(false);
        }
                    
        tab2.setEnabled(false);
        tab3.setEnabled(false);
        return;
    }
    
    base.mode.crtGpu.setEnabled();
    tab2.setEnabled(crtGpuChecked);
    tab3.setEnabled(crtGpuChecked);
    
    if ( crtGpuChecked )
        // crt with gpu don't use blur setting
        crt.blur.setEnabled( false );
    
    gpuBase.option.hires.setEnabled();
    gpuBase.option.distortionHires.setEnabled( crtGlitch.radialDistortion.active.checked() );
    
    mask.setEnabled( mask.level.active.checked() );
    mask.level.active.setEnabled();    
	
    crtGlitch.radialDistortion.setEnabled();
    crtGlitch.radialDistortion.slider.setEnabled( crtGlitch.radialDistortion.active.checked() );  

    gpuBase.luminance.setEnabled( !mask.level.active.checked() );
    gpuBase.lightFromCenter.setEnabled();
    gpuBase.lightFromCenter.slider.setEnabled( gpuBase.lightFromCenter.active.checked() );    

    // only enabled, when crt over gpu active
	
	if (crtGpuChecked) {	
		crtGlitch.lumaNoise.slider.setEnabled( crtGlitch.lumaNoise.active.checked() );
		crtGlitch.chromaNoise.slider.setEnabled( crtGlitch.chromaNoise.active.checked() );
		crtGlitch.randomLineOffset.slider.setEnabled( crtGlitch.randomLineOffset.active.checked() );	
	
		bloom.setEnabled( bloom.glow.active.checked() );
		bloom.glow.active.setEnabled();					
		bloom.weight.slider.setEnabled( bloom.weight.active.checked() );				
		
		vicIIGlitch.aec.slider.setEnabled( dynamic_cast<LIBC64::Interface*>(emulator) && vicIIGlitch.aec.active.checked() );
		vicIIGlitch.ba.slider.setEnabled( dynamic_cast<LIBC64::Interface*>(emulator) && vicIIGlitch.ba.active.checked() );
		vicIIGlitch.phi0.slider.setEnabled( dynamic_cast<LIBC64::Interface*>(emulator) && vicIIGlitch.phi0.active.checked() );
		vicIIGlitch.ras.slider.setEnabled( dynamic_cast<LIBC64::Interface*>(emulator) && vicIIGlitch.ras.active.checked() );
		vicIIGlitch.cas.slider.setEnabled( dynamic_cast<LIBC64::Interface*>(emulator) && vicIIGlitch.cas.active.checked() );			        		  
	}
}

auto VideoLayout::translate() -> void {
    setHeader(0, trans->get("view") );
    setHeader(1, trans->get("crt_shader") );
    setHeader(2, trans->get("crt_shader 2") );
    
    base.setText(trans->get("view"));	
    base.saturation.name.setText( trans->get("saturation", {}, true) );
    base.gamma.name.setText( trans->get("gamma", {},true) );
    base.brightness.name.setText( trans->get("brightness", {}, true) );
    base.contrast.name.setText( trans->get("contrast", {}, true) );
    base.phase.name.setText( trans->get("phase", {}, true) );
    base.option.newLuma.setText( trans->get("new_luma") );
    base.option.crtRealGamma.setText( trans->get("crt_real_gamma") );
	base.option.linearInterpolation.setText( trans->get("linear_interpolation") );
    base.mode.palette.setText( trans->get("palette") );
    base.mode.spectrum.setText( trans->get("color_spectrum") );    
    base.mode.reset.setText( trans->get("reset") );	
    base.mode.crtNone.setText( trans->get("crt_none") );
    base.mode.crtCpu.setText( trans->get("crt_cpu") );
    base.mode.crtGpu.setText( trans->get("crt_gpu") );
	
    crt.setText(trans->get("crt_emulation"));
	crt.phaseError.active.setText( trans->get("phase_error", {}, true) );
	crt.hanoverBars.active.setText( trans->get("hanover_bars", {}, true) );
	crt.scanlines.active.setText( trans->get("scanlines", {}, true) );
	crt.blur.active.setText( trans->get("blur", {}, true) );
	hf.setText(trans->get("rf_modulation"));
	hf.lumaRise.active.setText( trans->get("luma_rise", {}, true) );
    hf.lumaFall.active.setText( trans->get("luma_fall", {}, true) );

    gpuBase.setText(trans->get("GPU"));
    gpuBase.option.distortionHires.setText( trans->get("distortion_hires") );
    gpuBase.option.hires.setText( trans->get("hires") );
    gpuBase.firFilter.name.setText( trans->get("fir_filter_length", {}, true) );
    gpuBase.firSharp.sharpLeft.setText( trans->get("fir_filter_sharp_left") );
    gpuBase.firSharp.sharpRight.setText( trans->get("fir_filter_sharp_right") );
    gpuBase.firSharp.natural.setText( trans->get("fir_filter_natural") );    
    gpuBase.luminance.name.setText( trans->get("luminance", {}, true) );
    gpuBase.lightFromCenter.active.setText( trans->get("light_from_center", {}, true) );
    
	mask.setText( trans->get("mask") );
    mask.type.type.setText( trans->get("type", {}, true) );
    mask.type.apertureMask.setText( trans->get("aperture_mask") );
    mask.type.shadowMask.setText( trans->get("shadow_mask") );
    mask.type.slotMask.setText( trans->get("slot_mask") );
	mask.level.active.setText( trans->get("intensity", {}, true) );
    mask.luminance.name.setText( trans->get("luminance", {}, true) );
    mask.pitch.name.setText( trans->get("pitch", {}, true) );    
    mask.dpi.name.setText( trans->get("DPI", {}, true) );
	
	bloom.setText( trans->get("color_bloom") );
	bloom.glow.active.setText( trans->get("glow", {}, true) );
	bloom.radius.name.setText( trans->get("radius", {}, true) );
	bloom.variance.name.setText( trans->get("variance", {}, true) );
	bloom.weight.active.setText( trans->get("weight", {}, true) );

    crtGlitch.setText( trans->get("crt_glitches") );
    crtGlitch.lumaNoise.active.setText( trans->get("luma_noise", {}, true) );
    crtGlitch.chromaNoise.active.setText( trans->get("chroma_noise", {}, true) );
    crtGlitch.radialDistortion.active.setText( trans->get("radial_distortion", {}, true) );
    crtGlitch.randomLineOffset.active.setText( trans->get("random_line_offset", {}, true) );
    
    vicIIGlitch.setText( trans->get("vicII_glitches") );
    vicIIGlitch.toggleAll.setText( trans->get("toggle_all_glitches") );
    vicIIGlitch.aec.active.setText(trans->get("aec_glitch",{}, true));
    vicIIGlitch.ba.active.setText(trans->get("ba_glitch",{}, true));
    vicIIGlitch.phi0.active.setText(trans->get("phi_glitch",{}, true));
    vicIIGlitch.ras.active.setText(trans->get("ras_glitch",{}, true));
    vicIIGlitch.cas.active.setText(trans->get("cas_glitch",{}, true));
    
    SliderLayout::scale({&base.saturation, &base.gamma, &base.brightness, &base.contrast, &base.phase, &crt.phaseError, &crt.hanoverBars, &crt.scanlines, &crt.blur, &hf.lumaRise, &hf.lumaFall},
        "-100 %");
    unsigned neededWidth = SliderLayout::scale({&gpuBase.firFilter, &gpuBase.lightFromCenter, &gpuBase.luminance, &mask.level, &mask.luminance, &mask.dpi, &mask.pitch, &bloom.glow, &bloom.radius, &bloom.variance, &bloom.weight},
        "0.00 mm", mask.type.type.minimumSize().width );
    SliderLayout::scale({&crtGlitch.lumaNoise, &crtGlitch.chromaNoise, &crtGlitch.randomLineOffset, &crtGlitch.radialDistortion, &vicIIGlitch.aec, &vicIIGlitch.ba, &vicIIGlitch.phi0, &vicIIGlitch.ras, &vicIIGlitch.cas},
        "100.0 %");
    
    mask.type.children[ 0 ].size.width = neededWidth;
}

auto VideoLayout::sliderIdent() -> std::string {
	
	std::string ident = (emulator->getRegionEncoding() == Emulator::Interface::Region::Pal) ? "_pal" : "_ntsc";
	
    if (dynamic_cast<LIBC64::Interface*>(emulator) && base.mode.spectrum.checked())
        ident += "_spectrum";
	
	if (base.mode.crtCpu.checked())
		ident += "_crtcpu";
	else if (base.mode.crtGpu.checked())
		ident += "_crtgpu";
    		
	return ident;
}

auto VideoLayout::loadSettings(bool init) -> void {
    VideoManager::CrtMode crtMode = (VideoManager::CrtMode)_settings->get<unsigned>("video_crt", (unsigned)VideoManager::CrtMode::None, {0u, 2u});
    
    if (crtMode == VideoManager::CrtMode::Gpu)
        base.mode.crtGpu.setChecked();
    else if (crtMode == VideoManager::CrtMode::Cpu)
        base.mode.crtCpu.setChecked();
    else
        base.mode.crtNone.setChecked();    
    
    if (dynamic_cast<LIBC64::Interface*>(emulator) && _settings->get<bool>( "video_spectrum", true) )
        base.mode.spectrum.setChecked();
    else
        base.mode.palette.setChecked();
        
    updatePresets(!init);
}