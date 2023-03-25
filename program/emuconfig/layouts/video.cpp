
VideoModeLayout::VideoModeLayout(bool withSpectrum) {
    if (withSpectrum) {
        append(palette,{0u, 0u}, 10);
        append(spectrum,{0u, 0u}, 20);
    }
    
    append(rgb,{0u, 0u}, 10);
    append(svideoCpu,{0u, 0u}, 10);
    append(svideoGpu,{0u, 0u});
    	
    append(spacer,{~0u, 0u});
    append(reset,{0u, 0u});

    GUIKIT::RadioBox::setGroup(palette, spectrum);
    GUIKIT::RadioBox::setGroup(rgb, svideoCpu, svideoGpu);

    setAlignment(0.5);
}

VideoOptionLayout::VideoOptionLayout(bool withSpectrum) {
    if (withSpectrum) {
        append(newLuma, {0u, 0u}, 10);    	    
        append(tvGamma, {0u, 0u}, 10);
    } else {
        append(tvGamma, {0u, 0u}, 10);
    }

	append(linearInterpolation, {0u, 0u});

    setAlignment(0.5);
}

VideoBaseLayout::VideoBaseLayout(bool withSpectrum) :
mode(withSpectrum),
option(withSpectrum),
phase("°", false),
scanlines("%", true),
interlace("%", true) {
    append(mode, {~0u, 0u}, 2);
    append(option, {~0u, 0u}, 2);
        
    if (withSpectrum)
        append(phase, {~0u, 0u}, 2);
        
    append(saturation, {~0u, 0u}, 2);        
    append(contrast, {~0u, 0u}, 2);     
    append(brightness, {~0u, 0u}, 2);
    append(gamma, {~0u, 0u}, 2);
    append(scanlines,{~0u, 0u}, 2);
    if (!withSpectrum)
        append(interlace,{~0u, 0u});

    saturation.slider.setLength(201);
    gamma.slider.setLength(251);
    brightness.slider.setLength(201);
    contrast.slider.setLength(201);
    phase.slider.setLength(361);
    scanlines.slider.setLength(101);
    interlace.slider.setLength(101);
    
    setFont(GUIKIT::Font::system("bold"));    
    setPadding(8);
}

VideoEncodingLayout::VideoEncodingLayout() :
phaseError("°", true),
hanoverBars("%", true),
blur("%", true) {
    append(phaseError,{~0u, 0u}, 2);
    append(hanoverBars,{~0u, 0u}, 2);
    append(blur,{~0u, 0u});
    
    phaseError.slider.setLength(181); // -45° <-> 45°  ( 0.5 steps )
	hanoverBars.slider.setLength(201); // saturation change -100% <-> 100%
	blur.slider.setLength(101);
    
    setFont(GUIKIT::Font::system("bold"));    
    setPadding(8);
}

VideoLumaDelayLayout::VideoLumaDelayLayout() :
lumaRise("px", true),
lumaFall("px", true) {
	append(lumaRise,{~0u, 0u}, 2);
    append(lumaFall,{~0u, 0u});
	
	lumaRise.slider.setLength(31);
	lumaFall.slider.setLength(31);
    
    setFont(GUIKIT::Font::system("bold"));    
    setPadding(8);
}

VideoSubsamplingLayout::FirSharpLayout::FirSharpLayout() {
    append(sharpLeft, {0u, 0u}, 10);
    append(natural, {0u, 0u}, 10);
    append(sharpRight, {0u, 0u});
    
    GUIKIT::RadioBox::setGroup( sharpLeft, natural, sharpRight );
    
    setAlignment(0.5);
}

VideoGpuMiscLayout::Options::Options() {
    append(hires, {0u, 0u}, 15);
    append(distortionHires, {0u, 0u});
    
    setAlignment(0.5);
}

VideoGpuMiscLayout::VideoGpuMiscLayout() :
luminance("%", false),
lightFromCenter("%", true) {
    append(options, {~0u, 0u}, 3);
    append(lightFromCenter, {~0u, 0u}, 2);
    append(luminance, {~0u, 0u});

    lightFromCenter.slider.setLength(301);
    luminance.slider.setLength(501);

    setPadding(8);
    setFont(GUIKIT::Font::system("bold"));
}

VideoMaskTypeLayout::VideoMaskTypeLayout() {
    append(type, {175u, 0u}, 10);
    append(apertureMask, {0u, 0u}, 10);
    append(shadowMask, {0u, 0u}, 10);
    append(slotMask, {0u, 0u});
    
    GUIKIT::RadioBox::setGroup( apertureMask, shadowMask, slotMask );
    
    setAlignment(0.5);
}

VideoSubsamplingLayout::VideoSubsamplingLayout() :
firFilter("", false) {
    append(firSharp, {~0u, 0u}, 2);
    append(firFilter, {~0u, 0u}, 2);

    firFilter.slider.setLength(11);

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
    tab1.append(encoding, {~0u, 0u}, 5);
	tab1.append(lumaDelay, {~0u, 0u});

    tab2.append(gpuMisc, {~0u, 0u}, 10);
    tab2.append(subsampling, {~0u, 0u}, 10);
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
    setSliderAction<unsigned>( &base.scanlines, "scanlines", [this](unsigned value) { vManager()->setScanlines( value ); },
        [this](unsigned position) { return std::max(position, 1u); } );
    setSliderAction<unsigned>( &base.interlace, "interlace", [this](unsigned value) { vManager()->setInterlace( value ); },
        [this](unsigned position) { return std::max(position, 1u); } );
    setSliderAction<unsigned>( &encoding.blur, "blur", [this](unsigned value) { vManager()->setBlur( value ); } );
    setSliderAction<float>( &encoding.phaseError, "phase_error", [this](float value) { vManager()->setPhaseError( value ); },
        [this](unsigned position) { return (float)((int)position - 90) / 2.0f; } );          
    setSliderAction<int>( &encoding.hanoverBars, "hanover_bars", [this](int value) { vManager()->setHanoverBars( value ); }, [this](unsigned position) { return (int)position - 100; } );
    setSliderAction<float>( &mask.pitch, "mask_pitch", [this](float value) { vManager()->setMaskPitch( value ); },
        [this](unsigned position) { return (float)position / 100.0f; } );  
    setSliderAction<unsigned>( &mask.dpi, "mask_dpi", [this](unsigned value) { vManager()->setMaskDpi( value ); } );
    setSliderAction<unsigned>( &mask.level, "mask_level", [this](unsigned value) { vManager()->setMaskLevel( value ); },
        [this](unsigned position) { return std::max(position, 1u); } );
    setSliderAction<unsigned>( &mask.luminance, "mask_luminance", [this](unsigned value) { vManager()->setMaskLuminance( value ); } );
    setSliderAction<unsigned>( &subsampling.firFilter, "fir_filter_length", [this](unsigned value) { vManager()->setFirFilterLength( value ); },
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
    setSliderAction<float>( &lumaDelay.lumaRise, "luma_rise", [this](float value) { vManager()->setLumaRise( value ); },
        [this](unsigned position) { return ((float)std::max(position, 1u) / 10.0f) + 1.0f; } );
    setSliderAction<float>( &lumaDelay.lumaFall, "luma_fall", [this](float value) { vManager()->setLumaFall( value ); },
        [this](unsigned position) { return ((float)std::max(position, 1u) / 10.0f) + 1.0f; } );
    setSliderAction<unsigned>( &gpuMisc.lightFromCenter, "light_from_center", [this](unsigned value) { vManager()->setLightFromCenter( value ); },
        [this](unsigned position) { return std::max(position, 1u); } );
    setSliderAction<unsigned>( &gpuMisc.luminance, "luminance", [this](unsigned value) { vManager()->setLuminance( value ); } );
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
        vManager()->updateData<bool>("new_luma", checked);
    };

    base.option.tvGamma.onToggle = [this](bool checked) {
        _settings->set<bool>( "video_tv_gamma" + this->sliderIdent(), checked);
        vManager()->updateData<bool>("tv_gamma", checked);
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
        vManager()->updateData<unsigned>("mask_type", (unsigned)VideoManager::MaskType::Aperture);
    };

    mask.type.shadowMask.onActivate = [this]() {
        _settings->set<unsigned>( "video_mask_type" + this->sliderIdent(), (unsigned)VideoManager::MaskType::ShadowMask);
        vManager()->updateData<unsigned>("mask_type", (unsigned)VideoManager::MaskType::ShadowMask);
    };
    
    mask.type.slotMask.onActivate = [this]() {
        _settings->set<unsigned>( "video_mask_type" + this->sliderIdent(), (unsigned)VideoManager::MaskType::SlotMask);
        vManager()->updateData<unsigned>("mask_type", (unsigned)VideoManager::MaskType::SlotMask);
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

    base.mode.rgb.onActivate = [this]() {
        _settings->set<unsigned>("video_crt", (unsigned)VideoManager::CrtMode::None);
        emuThread->lock();
        program->fastForward( false );
        updatePresets();
        emuThread->unlock();
    };
    
    base.mode.svideoCpu.onActivate = [this]() {
        _settings->set<unsigned>("video_crt", (unsigned)VideoManager::CrtMode::Cpu);
        emuThread->lock();
        program->fastForward( false );
		updatePresets();
        emuThread->unlock();
    };
    
    base.mode.svideoGpu.onActivate = [this]() {
        _settings->set<unsigned>("video_crt", (unsigned)VideoManager::CrtMode::Gpu);
        emuThread->lock();
        program->fastForward( false );
		updatePresets();
        emuThread->unlock();
    };

    gpuMisc.options.distortionHires.onToggle = [this](bool checked) {
        _settings->set<bool>("video_distortion_hires" + this->sliderIdent(), checked);
        vManager()->updateData<bool>("distortion_hires", checked);
    };

    gpuMisc.options.hires.onToggle = [this](bool checked) {
        _settings->set<bool>("video_hires" + this->sliderIdent(), checked);
        vManager()->updateData<bool>("hires", checked);
    };

    subsampling.firSharp.sharpLeft.onActivate = [this]() {
        
        _settings->set<int>("video_fir_filter_sharp" + this->sliderIdent(), -1);
        vManager()->updateData<int>("fir_filter_sharp", -1);
    };

    subsampling.firSharp.sharpRight.onActivate = [this]() {

        _settings->set<int>("video_fir_filter_sharp" + this->sliderIdent(), 1);
        vManager()->updateData<int>("fir_filter_sharp", 1);
    };

    subsampling.firSharp.natural.onActivate = [this]() {

        _settings->set<int>("video_fir_filter_sharp" + this->sliderIdent(), 0);
        vManager()->updateData<int>("fir_filter_sharp", 0);
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
                gpuMisc.luminance.setEnabled( !checked );
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
    base.option.tvGamma.setChecked( _tvGamma );
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
    base.scanlines.active.setChecked( _useScanlines );
    base.scanlines.slider.setPosition( _scanlines );
    base.scanlines.value.setText( std::to_string(_scanlines) + " %" );
    base.interlace.active.setChecked( _useInterlace );
    base.interlace.slider.setPosition( _interlace );
    base.interlace.value.setText( std::to_string(_interlace) + " %" );
	// crt
    encoding.phaseError.active.setChecked( _usePhaseError );
    encoding.phaseError.slider.setPosition( int(_phaseError * 2.0) + 90);
    encoding.phaseError.value.setText( GUIKIT::String::formatFloatingPoint(_phaseError, 1) + " °");
    encoding.hanoverBars.active.setChecked( _useHanoverBars );
    encoding.hanoverBars.slider.setPosition( _hanoverBars + 100 );
    encoding.hanoverBars.value.setText( std::to_string(_hanoverBars) + " %" );
    encoding.blur.active.setChecked( _useBlur );
    encoding.blur.slider.setPosition( _blur );
    encoding.blur.value.setText( std::to_string(_blur) + " %" );
    lumaDelay.lumaRise.active.setChecked( _useLumaRise );
    lumaDelay.lumaRise.slider.setPosition( (unsigned)((_lumaRise - 1.0) * 10.0) );
    lumaDelay.lumaRise.value.setText( GUIKIT::String::formatFloatingPoint(_lumaRise, 1) + " px" );
    lumaDelay.lumaFall.active.setChecked( _useLumaFall );
    lumaDelay.lumaFall.slider.setPosition( (unsigned)((_lumaFall - 1.0) * 10.0) );
    lumaDelay.lumaFall.value.setText( GUIKIT::String::formatFloatingPoint(_lumaFall, 1) + " px" );

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

    gpuMisc.options.distortionHires.setChecked( _distortionHires );
    gpuMisc.options.hires.setChecked( _hires );
    gpuMisc.luminance.slider.setPosition( _luminance );
    gpuMisc.luminance.value.setText( std::to_string(_luminance) + " %" );
    gpuMisc.lightFromCenter.active.setChecked( _useLightFromCenter );
    gpuMisc.lightFromCenter.slider.setPosition( _lightFromCenter );
    gpuMisc.lightFromCenter.value.setText( std::to_string(_lightFromCenter) + " %" );
    subsampling.firFilter.slider.setPosition( (unsigned)(_firFilterLength / 2) );
    subsampling.firFilter.value.setText( std::to_string( _firFilterLength ) );
    
    if (_firFilterSharp == -1)
        subsampling.firSharp.sharpLeft.setChecked();
    else if (_firFilterSharp == 1)
        subsampling.firSharp.sharpRight.setChecked();
    else
        subsampling.firSharp.natural.setChecked();
        
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
    bool isC64 = dynamic_cast<LIBC64::Interface*>(emulator);
	
	if (base.mode.spectrum.checked()) {
        base.phase.setEnabled(true);
        base.option.newLuma.setEnabled(true);
    } else {
        base.phase.setEnabled(false);
        base.option.newLuma.setEnabled(false);        
    }
    base.scanlines.slider.setEnabled( base.scanlines.active.checked() );
    base.interlace.slider.setEnabled( base.interlace.active.checked() );
		
    bool crtChecked = base.mode.svideoCpu.checked() || base.mode.svideoGpu.checked();
    bool crtGpuChecked = base.mode.svideoGpu.checked();

    encoding.setEnabled( crtChecked );
    lumaDelay.setEnabled( (isC64 && crtChecked) || crtGpuChecked );
    
    if (crtChecked) {
        encoding.phaseError.slider.setEnabled( encoding.phaseError.active.checked() );
        encoding.hanoverBars.setEnabled( _pal );
        encoding.hanoverBars.slider.setEnabled( _pal && encoding.hanoverBars.active.checked() );
        encoding.blur.slider.setEnabled(  encoding.blur.active.checked() );
    }

    if ((isC64 && crtChecked) || crtGpuChecked) {
        lumaDelay.lumaRise.slider.setEnabled(lumaDelay.lumaRise.active.checked());
        lumaDelay.lumaFall.slider.setEnabled(lumaDelay.lumaFall.active.checked());
    }
    
    base.option.tvGamma.setEnabled( crtChecked && base.mode.palette.checked() && _pal );
	
    if (videoDriver->shaderFormat() != DRIVER::Video::ShaderType::GLSL) {
        if (videoDriver->shaderFormat() == DRIVER::Video::ShaderType::HLSL) {
            if(crtGpuChecked) {
                base.mode.svideoCpu.setChecked();
                _settings->set<unsigned>("video_crt", (unsigned)VideoManager::CrtMode::Cpu);
            }
			base.mode.svideoGpu.setEnabled(false);
        }
                    
        tab2.setEnabled(false);
        tab3.setEnabled(false);
        return;
    }
    
    base.mode.svideoGpu.setEnabled();
    tab2.setEnabled(crtGpuChecked);
    tab3.setEnabled(crtGpuChecked);
    
    if ( crtGpuChecked )
        // crt with gpu don't use blur setting
        encoding.blur.setEnabled( false );

    gpuMisc.setEnabled();
    gpuMisc.options.hires.setEnabled();
    gpuMisc.options.distortionHires.setEnabled();

    mask.setEnabled( mask.level.active.checked() );
    mask.level.active.setEnabled();    
	
    crtGlitch.radialDistortion.setEnabled();
    crtGlitch.radialDistortion.slider.setEnabled( crtGlitch.radialDistortion.active.checked() );

    gpuMisc.luminance.setEnabled( !mask.level.active.checked() );
    gpuMisc.lightFromCenter.setEnabled();
    gpuMisc.lightFromCenter.slider.setEnabled( gpuMisc.lightFromCenter.active.checked() );

    bloom.setEnabled( bloom.glow.active.checked() );
    bloom.glow.active.setEnabled();
    bloom.weight.slider.setEnabled( bloom.weight.active.checked() );

    // only enabled, when GPU active
	
	if (crtGpuChecked) {	
		crtGlitch.lumaNoise.slider.setEnabled( crtGlitch.lumaNoise.active.checked() );
		crtGlitch.chromaNoise.slider.setEnabled( crtGlitch.chromaNoise.active.checked() );
		crtGlitch.randomLineOffset.slider.setEnabled( crtGlitch.randomLineOffset.active.checked() );

		vicIIGlitch.aec.slider.setEnabled( isC64 && vicIIGlitch.aec.active.checked() );
		vicIIGlitch.ba.slider.setEnabled( isC64 && vicIIGlitch.ba.active.checked() );
		vicIIGlitch.phi0.slider.setEnabled( isC64 && vicIIGlitch.phi0.active.checked() );
		vicIIGlitch.ras.slider.setEnabled( isC64 && vicIIGlitch.ras.active.checked() );
		vicIIGlitch.cas.slider.setEnabled( isC64 && vicIIGlitch.cas.active.checked() );
	}
}

auto VideoLayout::translate() -> void {
    setHeader(0, trans->get("view") );
    setHeader(1, trans->get("shader") );
    setHeader(2, trans->get("shader 2") );
    
    base.setText(trans->get("view"));	
    base.saturation.name.setText( trans->get("saturation", {}, true) );
    base.gamma.name.setText( trans->get("gamma", {},true) );
    base.brightness.name.setText( trans->get("brightness", {}, true) );
    base.contrast.name.setText( trans->get("contrast", {}, true) );
    base.phase.name.setText( trans->get("phase", {}, true) );
    base.option.newLuma.setText( trans->get("new_luma") );
    base.option.tvGamma.setText( trans->get("TV gamma") );
	base.option.linearInterpolation.setText( trans->get("linear_interpolation") );
    base.mode.palette.setText( trans->get("palette") );
    base.mode.spectrum.setText( trans->get("color_spectrum") );    
    base.mode.reset.setText( trans->get("reset") );	
    base.mode.rgb.setText( trans->get("RGB") );
    base.mode.svideoCpu.setText( trans->get("S/C-Video") );
    base.mode.svideoCpu.setTooltip( trans->get("S/C-Video tooltip") );
    base.mode.svideoGpu.setText( trans->get("S/C-Video on GPU") );
    base.mode.svideoGpu.setTooltip( trans->get("S/C-Video tooltip") );
    base.scanlines.active.setText( trans->get("scanlines", {}, true) );
    base.interlace.active.setText( trans->get("interlace", {}, true) );

    encoding.setText(trans->get("color encoding"));
    encoding.phaseError.active.setText( trans->get("phase_error", {}, true) );
    encoding.hanoverBars.active.setText( trans->get("hanover_bars", {}, true) );
    encoding.blur.active.setText( trans->get("blur", {}, true) );
    lumaDelay.setText(trans->get("luma delay"));
    lumaDelay.lumaRise.active.setText( trans->get("luma_rise", {}, true) );
    lumaDelay.lumaFall.active.setText( trans->get("luma_fall", {}, true) );

    gpuMisc.setText(trans->get("generic"));
    gpuMisc.options.distortionHires.setText( trans->get("distortion_hires") );
    gpuMisc.options.distortionHires.setTooltip( trans->get("distortion hires tooltip") );
    gpuMisc.options.hires.setText( trans->get("hires") );
    gpuMisc.luminance.name.setText( trans->get("luminance", {}, true) );
    gpuMisc.lightFromCenter.active.setText( trans->get("light_from_center", {}, true) );

    subsampling.setText(trans->get("chroma subsampling"));
    subsampling.firFilter.name.setText( trans->get("fir filter blur", {}, true) );
    subsampling.firSharp.sharpLeft.setText( trans->get("fir filter left") );
    subsampling.firSharp.sharpRight.setText( trans->get("fir filter right") );
    subsampling.firSharp.natural.setText( trans->get("fir filter natural") );

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
    
    SliderLayout::scale({&base.saturation, &base.gamma, &base.brightness, &base.contrast, &base.phase, &base.scanlines, &base.interlace, &encoding.phaseError, &encoding.hanoverBars, &encoding.blur, &lumaDelay.lumaRise, &lumaDelay.lumaFall},
        "-100 %");
    unsigned neededWidth = SliderLayout::scale({&subsampling.firFilter, &gpuMisc.lightFromCenter, &gpuMisc.luminance, &mask.level, &mask.luminance, &mask.dpi, &mask.pitch, &bloom.glow, &bloom.radius, &bloom.variance, &bloom.weight},
        "0.00 mm", mask.type.type.minimumSize().width );
    SliderLayout::scale({&crtGlitch.lumaNoise, &crtGlitch.chromaNoise, &crtGlitch.randomLineOffset, &crtGlitch.radialDistortion, &vicIIGlitch.aec, &vicIIGlitch.ba, &vicIIGlitch.phi0, &vicIIGlitch.ras, &vicIIGlitch.cas},
        "100.0 %");
    
    mask.type.children[ 0 ].size.width = neededWidth;
}

auto VideoLayout::sliderIdent() -> std::string {
	
	std::string ident = (emulator->getRegionEncoding() == Emulator::Interface::Region::Pal) ? "_pal" : "_ntsc";
	
    if (dynamic_cast<LIBC64::Interface*>(emulator) && base.mode.spectrum.checked())
        ident += "_spectrum";
	
	if (base.mode.svideoCpu.checked())
		ident += "_crtcpu";
	else if (base.mode.svideoGpu.checked())
		ident += "_crtgpu";
    		
	return ident;
}

auto VideoLayout::loadSettings(bool init) -> void {
    VideoManager::CrtMode crtMode = (VideoManager::CrtMode)_settings->get<unsigned>("video_crt", (unsigned)VideoManager::CrtMode::None, {0u, 2u});
    
    if (crtMode == VideoManager::CrtMode::Gpu)
        base.mode.svideoGpu.setChecked();
    else if (crtMode == VideoManager::CrtMode::Cpu)
        base.mode.svideoCpu.setChecked();
    else
        base.mode.rgb.setChecked();
    
    if (dynamic_cast<LIBC64::Interface*>(emulator) && _settings->get<bool>( "video_spectrum", true) )
        base.mode.spectrum.setChecked();
    else
        base.mode.palette.setChecked();
        
    updatePresets(!init);
}