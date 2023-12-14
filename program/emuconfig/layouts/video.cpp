
VideoBaseLayout::View::Mode::Mode(bool withSpectrum) {
    if (withSpectrum) {
        append(palette,{0u, 0u}, 10);
        append(spectrum,{0u, 0u}, 20);
        GUIKIT::RadioBox::setGroup(palette, spectrum);
    }

    append(rgb,{0u, 0u}, 10);
    append(svideoCpu,{0u, 0u}, 10);
    append(svideoGpu,{0u, 0u}, 10);
    append(externGpu,{0u, 0u});

    append(spacer,{~0u, 0u});
    append(reset,{0u, 0u});

    GUIKIT::RadioBox::setGroup(rgb, svideoCpu, svideoGpu, externGpu);

    setAlignment(0.5);
}

VideoBaseLayout::View::Option::Option(bool withSpectrum) {
    if (withSpectrum) {
        append(newLuma, {0u, 0u}, 10);
        append(tvGamma, {0u, 0u}, 10);
    } else {
        append(tvGamma, {0u, 0u}, 10);
    }

    append(linearInterpolation, {0u, 0u});

    setAlignment(0.5);
}

VideoBaseLayout::View::View(bool withSpectrum) :
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

    setPadding(8);
    setFont(GUIKIT::Font::system("bold"));
}

VideoBaseLayout::Encoding::Encoding() :
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

VideoBaseLayout::LumaDelay::LumaDelay() :
lumaRise("px", true),
lumaFall("px", true) {
    append(lumaRise,{~0u, 0u}, 2);
    append(lumaFall,{~0u, 0u});

    lumaRise.slider.setLength(31);
    lumaFall.slider.setLength(31);

    setFont(GUIKIT::Font::system("bold"));
    setPadding(8);
}

VideoBaseLayout::VideoBaseLayout(bool withSpectrum) :
view(withSpectrum) {

    append(view, {~0u, 0u}, 5);
    append(encoding, {~0u, 0u}, 5);
    append(lumaDelay, {~0u, 0u});

}

VideoInternLayout::Misc::Option::Option() {
    append(hires, {0u, 0u}, 15);
    append(distortionHires, {0u, 0u});

    setAlignment(0.5);
}

VideoInternLayout::Misc::Misc() :
luminance("%", false),
lightFromCenter("%", true) {
    append(option, {~0u, 0u}, 3);
    append(lightFromCenter, {~0u, 0u}, 2);
    append(luminance, {~0u, 0u});

    lightFromCenter.slider.setLength(301);
    luminance.slider.setLength(501);

    setPadding(8);
    setFont(GUIKIT::Font::system("bold"));
}

VideoInternLayout::Subsampling::FirSharp::FirSharp() {
    append(sharpLeft, {0u, 0u}, 10);
    append(natural, {0u, 0u}, 10);
    append(sharpRight, {0u, 0u});

    GUIKIT::RadioBox::setGroup( sharpLeft, natural, sharpRight );

    setAlignment(0.5);
}

VideoInternLayout::Subsampling::Subsampling() :
firFilter("", false) {
    append(firSharp, {~0u, 0u}, 2);
    append(firFilter, {~0u, 0u}, 2);

    firFilter.slider.setLength(11);

    setPadding(8);
    setFont(GUIKIT::Font::system("bold"));
}

VideoInternLayout::Mask::Type::Type() {
    append(label, {175u, 0u}, 10);
    append(apertureMask, {0u, 0u}, 10);
    append(shadowMask, {0u, 0u}, 10);
    append(slotMask, {0u, 0u});

    GUIKIT::RadioBox::setGroup( apertureMask, shadowMask, slotMask );

    setAlignment(0.5);
}

VideoInternLayout::Mask::Mask() :
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

VideoInternLayout::Bloom::Bloom() :
glow("%", true),
radius("px", false),
variance("", false),
weight("", true) {
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

VideoInternLayout::VideoInternLayout() {
    append(misc,{~0u, 0u}, 10);
    append(subsampling,{~0u, 0u}, 10);
    append(mask,{~0u, 0u}, 10);
    append(bloom,{~0u, 0u});
}

VideoGlitchLayout::Crt::Crt() :
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

VideoGlitchLayout::VicII::VicII() :
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

VideoGlitchLayout::VideoGlitchLayout(bool withVic) {
    append(crt,{~0u, 0u}, 10);
    if (withVic)
        append(vicII,{~0u, 0u});
}

VideoShaderLayout::Control::Control() {
    append(loaded,{0u, 0u}, 10);
    append(unload,{0u, 0u});
    append(spacer,{~0u, 0u});
    append(save,{0u, 0u}, 10);
    append(load,{0u, 0u});

    setPadding(8);
    setFont(GUIKIT::Font::system("bold"));
}

VideoShaderLayout::Favourite::Control::Control() {
    append(add,{0u, 0u}, 10);
    append(remove,{0u, 0u});
}

VideoShaderLayout::Favourite::Favourite() {
    append(list,{0u, 0u}, 10);
    append(control,{0u, 0u});

    setPadding(8);
    setFont(GUIKIT::Font::system("bold"));
}

VideoPassLayout::Load::Load() {
    append(label,{0u, 0u}, 10);
    append(button,{0u, 0u});
}

VideoPassLayout::VideoPassLayout() {
    append(load,{0u, 0u}, 10);
    append(filter,{0u, 0u}, 10);
    append(scale,{0u, 0u});

    setPadding(8);
    setFont(GUIKIT::Font::system("bold"));
}

VideoLayout::VideoLayout(TabWindow* tabWindow) :
layBase( dynamic_cast<LIBC64::Interface*>(tabWindow->emulator) ),
layGlitch( dynamic_cast<LIBC64::Interface*>(tabWindow->emulator) ) {
    GUIKIT::TreeViewItem* tvi;
    this->tabWindow = tabWindow;
    this->emulator = tabWindow->emulator;
    imgFolderOpen.loadPng((uint8_t*)Icons::folderOpen, sizeof(Icons::folderOpen) );
    imgFolderClosed.loadPng((uint8_t*)Icons::folderClosed, sizeof(Icons::folderClosed) );
    imgDocument.loadPng((uint8_t*)Icons::document, sizeof(Icons::document) );

    tviBase.setText( "view" );
    tviBase.setUserData( (uintptr_t)1 );
    tviBase.setImage( imgFolderClosed );
    tviBase.setImageExpanded( imgFolderOpen );

    tviIntern.setText( "intern" );
    tviIntern.setUserData( (uintptr_t)11 );
    tviIntern.setImage(imgDocument);

    tviGlitch.setText( "Glitches" );
    tviGlitch.setUserData( (uintptr_t)12 );
    tviGlitch.setImage(imgDocument);

    moduleSwitch.setLayout(1, layBase, {~0u, ~0u});
    moduleSwitch.setLayout(11, layIntern, {~0u, ~0u});
    moduleSwitch.setLayout(12, layGlitch, {~0u, ~0u});

    tviShader.setText( "Shader" );
    tviShader.setUserData( (uintptr_t)2 );
    tviShader.setImage(imgDocument);
    tviParams.setText( "Params" );
    tviParams.setUserData( (uintptr_t)3 );
    tviParams.setImage(imgDocument);

    tviBase.append(tviIntern);
    tviBase.append(tviGlitch);
    moduleTree.append(tviBase);

    moduleTree.append(tviShader);
    moduleTree.append(tviParams);

    moduleSwitch.setLayout(1, layBase, {~0u, ~0u});
    moduleSwitch.setLayout(11, layIntern, {~0u, ~0u});
    moduleSwitch.setLayout(12, layGlitch, {~0u, ~0u});
    moduleSwitch.setLayout(2, layShader, {~0u, ~0u});

    tviBase.setExpanded();


    append( moduleTree, { GUIKIT::Font::scale(150), ~0u}, 10 );

    append( moduleSwitch, {~0u, ~0u} );

    moduleSwitch.setSelection( 1 );

    moduleTree.onChange = [this]() {
        auto item = moduleTree.selected();

        if (!item)
            return;

        unsigned navIdent = (unsigned)item->userData();

        moduleSwitch.setSelection( navIdent );
    };

    setMargin(10);
    setPadding(10);

    setSliderAction<unsigned>( &layBase.view.gamma, "gamma", [this](unsigned value) { vManager()->setGamma( value ); },
          [this](unsigned position) { return position + 30; } );                
    setSliderAction<unsigned>( &layBase.view.saturation, "saturation", [this](unsigned value) { vManager()->setSaturation( value ); } );
    setSliderAction<unsigned>( &layBase.view.brightness, "brightness", [this](unsigned value) { vManager()->setBrightness( value ); } );
    setSliderAction<unsigned>( &layBase.view.contrast, "contrast", [this](unsigned value) { vManager()->setContrast( value ); } );
    setSliderAction<int>( &layBase.view.phase, "phase", [this](int value) { vManager()->setPhase( value ); }, [this](unsigned position) { return (int)position - 180; } );
    setSliderAction<unsigned>( &layBase.view.scanlines, "scanlines", [this](unsigned value) { vManager()->setScanlines( value ); },
        [this](unsigned position) { return std::max(position, 1u); } );
    setSliderAction<unsigned>( &layBase.view.interlace, "interlace", [this](unsigned value) { vManager()->setInterlace( value ); },
        [this](unsigned position) { return std::max(position, 1u); } );
    setSliderAction<unsigned>( &layBase.encoding.blur, "blur", [this](unsigned value) { vManager()->setBlur( value ); } );
    setSliderAction<float>( &layBase.encoding.phaseError, "phase_error", [this](float value) { vManager()->setPhaseError( value ); },
        [this](unsigned position) { return (float)((int)position - 90) / 2.0f; } );          
    setSliderAction<int>( &layBase.encoding.hanoverBars, "hanover_bars", [this](int value) { vManager()->setHanoverBars( value ); }, [this](unsigned position) { return (int)position - 100; } );
    setSliderAction<float>( &layIntern.mask.pitch, "mask_pitch", [this](float value) { vManager()->setMaskPitch( value ); },
        [this](unsigned position) { return (float)position / 100.0f; } );  
    setSliderAction<unsigned>( &layIntern.mask.dpi, "mask_dpi", [this](unsigned value) { vManager()->setMaskDpi( value ); } );
    setSliderAction<unsigned>( &layIntern.mask.level, "mask_level", [this](unsigned value) { vManager()->setMaskLevel( value ); },
        [this](unsigned position) { return std::max(position, 1u); } );
    setSliderAction<unsigned>( &layIntern.mask.luminance, "mask_luminance", [this](unsigned value) { vManager()->setMaskLuminance( value ); } );
    setSliderAction<unsigned>( &layIntern.subsampling.firFilter, "fir_filter_length", [this](unsigned value) { vManager()->setFirFilterLength( value ); },
        [this](unsigned position) { return position * 2 + 1; } );  
    setSliderAction<float>( &layGlitch.crt.lumaNoise, "luma_noise", [this](float value) { vManager()->setLumaNoise( value ); },
        [this](unsigned position) { return (float)std::max(position, 1u) / 10.0f; });
    setSliderAction<float>( &layGlitch.crt.chromaNoise, "chroma_noise", [this](float value) { vManager()->setChromaNoise( value ); },
        [this](unsigned position) { return (float)std::max(position, 1u) / 10.0f; } );
    setSliderAction<unsigned>( &layGlitch.crt.radialDistortion, "radial_distortion", [this](unsigned value) { vManager()->setRadialDistortion( value ); },
        [this](unsigned position) { return std::max(position, 1u); } );
    setSliderAction<float>( &layGlitch.vicII.aec, "aec_glitch", [this](float value) { vManager()->setAecGlitch( value ); },
        [this](unsigned position) { return (float)std::max(position, 1u) / 10.0f; } );
    setSliderAction<float>( &layGlitch.vicII.ba, "ba_glitch", [this](float value) { vManager()->setBaGlitch( value ); },
        [this](unsigned position) { return (float)std::max(position, 1u) / 10.0f; } );
    setSliderAction<float>( &layGlitch.vicII.phi0, "phi0_glitch", [this](float value) { vManager()->setPhi0Glitch( value ); },
        [this](unsigned position) { return (float)std::max(position, 1u) / 10.0f; } );
    setSliderAction<float>( &layGlitch.vicII.ras, "ras_glitch", [this](float value) { vManager()->setRasGlitch( value ); },
        [this](unsigned position) { return (float)std::max(position, 1u) / 10.0f; } );
    setSliderAction<float>( &layGlitch.vicII.cas, "cas_glitch", [this](float value) { vManager()->setCasGlitch( value ); },
        [this](unsigned position) { return (float)std::max(position, 1u) / 10.0f; } );
    setSliderAction<float>( &layBase.lumaDelay.lumaRise, "luma_rise", [this](float value) { vManager()->setLumaRise( value ); },
        [this](unsigned position) { return ((float)std::max(position, 1u) / 10.0f) + 1.0f; } );
    setSliderAction<float>( &layBase.lumaDelay.lumaFall, "luma_fall", [this](float value) { vManager()->setLumaFall( value ); },
        [this](unsigned position) { return ((float)std::max(position, 1u) / 10.0f) + 1.0f; } );
    setSliderAction<unsigned>( &layIntern.misc.lightFromCenter, "light_from_center", [this](unsigned value) { vManager()->setLightFromCenter( value ); },
        [this](unsigned position) { return std::max(position, 1u); } );
    setSliderAction<unsigned>( &layIntern.misc.luminance, "luminance", [this](unsigned value) { vManager()->setLuminance( value ); } );
	setSliderAction<unsigned>( &layIntern.bloom.glow, "bloom_glow", [this](unsigned value) { vManager()->setBloomGlow( value ); },
        [this](unsigned position) { return std::max(position, 1u); } );
	setSliderAction<float>( &layIntern.bloom.variance, "bloom_variance", [this](float value) { vManager()->setBloomVariance( value ); },
		[this](unsigned position) { return ((float)position / 10.0f) + 1.0f; } ); 
	setSliderAction<unsigned>( &layIntern.bloom.radius, "bloom_radius", [this](unsigned value) { vManager()->setBloomRadius( value ); },
		[this](unsigned position) { return position + 1; } ); 
	setSliderAction<float>( &layIntern.bloom.weight, "bloom_weight", [this](float value) { vManager()->setBloomWeight( value ); },
		[this](unsigned position) { return (float)std::max(position, 1u) / 100.0f; } );
    setSliderAction<float>( &layGlitch.crt.randomLineOffset, "random_line_offset", [this](float value) { vManager()->setRandomLineOffset( value ); },
        [this](unsigned position) { return (float)std::max(position, 1u) / 100.0f; } );
    
    layBase.view.option.newLuma.onToggle = [this](bool checked) {
        _settings->set<bool>( "video_new_luma" + this->sliderIdent(), checked);
        vManager()->updateData<bool>("new_luma", checked);
    };

    layBase.view.option.tvGamma.onToggle = [this](bool checked) {
        _settings->set<bool>( "video_tv_gamma" + this->sliderIdent(), checked);
        vManager()->updateData<bool>("tv_gamma", checked);
    };
	
	layBase.view.option.linearInterpolation.onToggle = [this](bool checked) {
		_settings->set<unsigned>("video_filter", checked ? 1 : 0 );
        emuThread->lock();
        program->setVideoFilter();
        emuThread->unlock();
	};

    layBase.view.option.linearInterpolation.setChecked( _settings->get<unsigned>("video_filter", 1u, {0u, 1u}) );

    layIntern.mask.type.apertureMask.onActivate = [this]() {
        _settings->set<unsigned>( "video_mask_type" + this->sliderIdent(), (unsigned)VideoManager::MaskType::Aperture);
        vManager()->updateData<unsigned>("mask_type", (unsigned)VideoManager::MaskType::Aperture);
    };

    layIntern.mask.type.shadowMask.onActivate = [this]() {
        _settings->set<unsigned>( "video_mask_type" + this->sliderIdent(), (unsigned)VideoManager::MaskType::ShadowMask);
        vManager()->updateData<unsigned>("mask_type", (unsigned)VideoManager::MaskType::ShadowMask);
    };

    layIntern.mask.type.slotMask.onActivate = [this]() {
        _settings->set<unsigned>( "video_mask_type" + this->sliderIdent(), (unsigned)VideoManager::MaskType::SlotMask);
        vManager()->updateData<unsigned>("mask_type", (unsigned)VideoManager::MaskType::SlotMask);
    };

    layBase.view.mode.reset.onActivate = [this]() {
        vManager()->resetSettings();
        emuThread->lock();
        updatePresets();
        emuThread->unlock();
    };

    layBase.view.mode.palette.onActivate = [this]() {
        _settings->set<bool>( "video_spectrum", false);
        emuThread->lock();
        updatePresets();
        emuThread->unlock();
    };

    layBase.view.mode.spectrum.onActivate = [this]() {
        _settings->set<bool>("video_spectrum", true);
        emuThread->lock();
        updatePresets();
        emuThread->unlock();
    };

    layBase.view.mode.rgb.onActivate = [this]() {
        _settings->set<unsigned>("video_crt", (unsigned)VideoManager::CrtMode::None);
        emuThread->lock();
        program->fastForward( false );
        updatePresets();
        emuThread->unlock();
    };

    layBase.view.mode.svideoCpu.onActivate = [this]() {
        _settings->set<unsigned>("video_crt", (unsigned)VideoManager::CrtMode::Cpu);
        emuThread->lock();
        program->fastForward( false );
		updatePresets();
        emuThread->unlock();
    };

    layBase.view.mode.svideoGpu.onActivate = [this]() {
        _settings->set<unsigned>("video_crt", (unsigned)VideoManager::CrtMode::Gpu);
        emuThread->lock();
        program->fastForward( false );
		updatePresets();
        emuThread->unlock();
    };

    layIntern.misc.option.distortionHires.onToggle = [this](bool checked) {
        _settings->set<bool>("video_distortion_hires" + this->sliderIdent(), checked);
        vManager()->updateData<bool>("distortion_hires", checked);
    };

    layIntern.misc.option.hires.onToggle = [this](bool checked) {
        _settings->set<bool>("video_hires" + this->sliderIdent(), checked);
        vManager()->updateData<bool>("hires", checked);
    };

    layIntern.subsampling.firSharp.sharpLeft.onActivate = [this]() {
        
        _settings->set<int>("video_fir_filter_sharp" + this->sliderIdent(), -1);
        vManager()->updateData<int>("fir_filter_sharp", -1);
    };

    layIntern.subsampling.firSharp.sharpRight.onActivate = [this]() {

        _settings->set<int>("video_fir_filter_sharp" + this->sliderIdent(), 1);
        vManager()->updateData<int>("fir_filter_sharp", 1);
    };

    layIntern.subsampling.firSharp.natural.onActivate = [this]() {

        _settings->set<int>("video_fir_filter_sharp" + this->sliderIdent(), 0);
        vManager()->updateData<int>("fir_filter_sharp", 0);
    };
    
    layGlitch.vicII.toggleAll.onActivate = [this]() {
        
        bool b1 = layGlitch.vicII.aec.active.checked();
        bool b2 = layGlitch.vicII.ba.active.checked();
        bool b3 = layGlitch.vicII.phi0.active.checked();
        bool b4 = layGlitch.vicII.ras.active.checked();
        bool b5 = layGlitch.vicII.cas.active.checked();
        bool _checked = b1 || b2 || b3 || b4 || b5;

        layGlitch.vicII.aec.active.setChecked( !_checked );
        layGlitch.vicII.ba.active.setChecked( !_checked );
        layGlitch.vicII.phi0.active.setChecked( !_checked );
        layGlitch.vicII.ras.active.setChecked( !_checked );
        layGlitch.vicII.cas.active.setChecked( !_checked );

        layGlitch.vicII.aec.active.onToggle( !_checked );
        layGlitch.vicII.ba.active.onToggle( !_checked );
        layGlitch.vicII.phi0.active.onToggle( !_checked );
        layGlitch.vicII.ras.active.onToggle( !_checked );
        layGlitch.vicII.cas.active.onToggle( !_checked );
    };

    loadSettings(true);
}

template<typename T> auto VideoLayout::setSliderAction( SliderLayout* layout, std::string baseIdent, std::function<void ( T value )> callBack, std::function<T ( unsigned position )> callTransfer ) -> void {
    		
    if (layout->withActivator)
        layout->active.onToggle = [this, layout, baseIdent, callBack, callTransfer](bool checked) {
            _settings->set<bool>("video_" + baseIdent + "_use" + this->sliderIdent(), checked);
            layout->slider.setEnabled(checked);
            if (layout == &layIntern.mask.level) {
                layIntern.mask.setEnabled( checked );
                layout->active.setEnabled();
                layIntern.misc.luminance.setEnabled( !checked );
            } else if (layout == &layIntern.bloom.glow) {
                layIntern.bloom.setEnabled( checked );
                layout->active.setEnabled();
                layIntern.bloom.weight.slider.setEnabled( layIntern.bloom.weight.active.checked() );
			}
            
            unsigned position = layout->slider.position();
			T value = callTransfer( position );

            vManager()->updateData(baseIdent, checked ? value : T(0));
        };

    layout->slider.onChange = [this, layout, baseIdent, callBack, callTransfer](unsigned position) {
		T value = callTransfer( position );	
        auto unit = layout->unit;
        auto roundDigits = (layout == &layIntern.mask.pitch || layout == &layGlitch.crt.randomLineOffset || layout == &layIntern.bloom.weight) ? 2 : 1;
		
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
    
	layBase.view.option.newLuma.setChecked( _newLuma );
    layBase.view.option.tvGamma.setChecked( _tvGamma );
    layBase.view.saturation.slider.setPosition(_saturation);
    layBase.view.saturation.value.setText(std::to_string(_saturation) + " %");
    layBase.view.gamma.slider.setPosition(_gamma - 30 );
    layBase.view.gamma.value.setText( std::to_string(_gamma) + " %" );
    layBase.view.brightness.slider.setPosition(_brightness);
    layBase.view.brightness.value.setText(std::to_string(_brightness) + " %");
    layBase.view.contrast.slider.setPosition(_contrast);
    layBase.view.contrast.value.setText(std::to_string(_contrast) + " %");
    layBase.view.phase.slider.setPosition(_phase + 180);
    layBase.view.phase.value.setText(std::to_string(_phase) + " °");
    layBase.view.scanlines.active.setChecked( _useScanlines );
    layBase.view.scanlines.slider.setPosition( _scanlines );
    layBase.view.scanlines.value.setText( std::to_string(_scanlines) + " %" );
    layBase.view.interlace.active.setChecked( _useInterlace );
    layBase.view.interlace.slider.setPosition( _interlace );
    layBase.view.interlace.value.setText( std::to_string(_interlace) + " %" );
	// crt
    layBase.encoding.phaseError.active.setChecked( _usePhaseError );
    layBase.encoding.phaseError.slider.setPosition( int(_phaseError * 2.0) + 90);
    layBase.encoding.phaseError.value.setText( GUIKIT::String::formatFloatingPoint(_phaseError, 1) + " °");
    layBase.encoding.hanoverBars.active.setChecked( _useHanoverBars );
    layBase.encoding.hanoverBars.slider.setPosition( _hanoverBars + 100 );
    layBase.encoding.hanoverBars.value.setText( std::to_string(_hanoverBars) + " %" );
    layBase.encoding.blur.active.setChecked( _useBlur );
    layBase.encoding.blur.slider.setPosition( _blur );
    layBase.encoding.blur.value.setText( std::to_string(_blur) + " %" );
    layBase.lumaDelay.lumaRise.active.setChecked( _useLumaRise );
    layBase.lumaDelay.lumaRise.slider.setPosition( (unsigned)((_lumaRise - 1.0) * 10.0) );
    layBase.lumaDelay.lumaRise.value.setText( GUIKIT::String::formatFloatingPoint(_lumaRise, 1) + " px" );
    layBase.lumaDelay.lumaFall.active.setChecked( _useLumaFall );
    layBase.lumaDelay.lumaFall.slider.setPosition( (unsigned)((_lumaFall - 1.0) * 10.0) );
    layBase.lumaDelay.lumaFall.value.setText( GUIKIT::String::formatFloatingPoint(_lumaFall, 1) + " px" );

    // shader features    
    layGlitch.crt.chromaNoise.active.setChecked( _useChromaNoise );
    layGlitch.crt.chromaNoise.slider.setPosition( (unsigned)(_chromaNoise * 10.0) );
    layGlitch.crt.chromaNoise.value.setText( GUIKIT::String::formatFloatingPoint(_chromaNoise, 1) + " %" );
    layGlitch.crt.lumaNoise.active.setChecked(_useLumaNoise);
    layGlitch.crt.lumaNoise.slider.setPosition((unsigned)(_lumaNoise * 10.0));
    layGlitch.crt.lumaNoise.value.setText(GUIKIT::String::formatFloatingPoint(_lumaNoise, 1) + " %");
    layGlitch.crt.radialDistortion.active.setChecked( _useRadialDistortion );
    layGlitch.crt.radialDistortion.slider.setPosition( _radialDistortion );
    layGlitch.crt.radialDistortion.value.setText( std::to_string(_radialDistortion) + " %" );
    layGlitch.crt.randomLineOffset.active.setChecked( _useRandomLineOffset );
    layGlitch.crt.randomLineOffset.slider.setPosition( (unsigned)(_randomLineOffset * 100.0) );
    layGlitch.crt.randomLineOffset.value.setText( GUIKIT::String::formatFloatingPoint(_randomLineOffset, 2) + " %" );

    layIntern.mask.level.active.setChecked( _useMaskLevel );
    layIntern.mask.level.slider.setPosition( _maskLevel );
    layIntern.mask.level.value.setText( std::to_string(_maskLevel) + " %" );
    layIntern.mask.luminance.slider.setPosition(_maskLuminance);
    layIntern.mask.luminance.value.setText(std::to_string(_maskLuminance) + " %");
    layIntern.mask.pitch.slider.setPosition( _maskPitch * 100.0 );
    layIntern.mask.pitch.value.setText( GUIKIT::String::formatFloatingPoint(_maskPitch, 2) + " mm" );
    layIntern.mask.dpi.slider.setPosition( _maskDpi );
    layIntern.mask.dpi.value.setText( std::to_string(_maskDpi) + " dpi" );

    layIntern.bloom.glow.active.setChecked( _useBloomGlow );
    layIntern.bloom.glow.slider.setPosition( _bloomGlow );
    layIntern.bloom.glow.value.setText( std::to_string( _bloomGlow ) + " %" );
    layIntern.bloom.weight.active.setChecked( _useBloomWeight );
    layIntern.bloom.weight.slider.setPosition( _bloomWeight * 100.0 );
    layIntern.bloom.weight.value.setText( GUIKIT::String::formatFloatingPoint( _bloomWeight, 2 ) );
    layIntern.bloom.variance.slider.setPosition( (unsigned)((_bloomVariance - 1.0) * 10.0) );
    layIntern.bloom.variance.value.setText( GUIKIT::String::formatFloatingPoint( _bloomVariance, 1 ) );
    layIntern.bloom.radius.slider.setPosition( _bloomRadius - 1 );
    layIntern.bloom.radius.value.setText( std::to_string( _bloomRadius ) + " px" );

    layGlitch.vicII.aec.active.setChecked( _useAecGlitch );
    layGlitch.vicII.aec.slider.setPosition( (unsigned)(_aecGlitch * 10.0) );
    layGlitch.vicII.aec.value.setText( GUIKIT::String::formatFloatingPoint(_aecGlitch, 1) + " %" );
    layGlitch.vicII.ba.active.setChecked( _useBaGlitch );
    layGlitch.vicII.ba.slider.setPosition( (unsigned)(_baGlitch * 10.0) );
    layGlitch.vicII.ba.value.setText( GUIKIT::String::formatFloatingPoint(_baGlitch, 1) + " %" );
    layGlitch.vicII.phi0.active.setChecked( _usePhi0Glitch );
    layGlitch.vicII.phi0.slider.setPosition( (unsigned)(_phi0Glitch * 10.0) );
    layGlitch.vicII.phi0.value.setText( GUIKIT::String::formatFloatingPoint(_phi0Glitch, 1) + " %" );
    layGlitch.vicII.ras.active.setChecked( _useRasGlitch );
    layGlitch.vicII.ras.slider.setPosition( (unsigned)(_rasGlitch * 10.0) );
    layGlitch.vicII.ras.value.setText( GUIKIT::String::formatFloatingPoint(_rasGlitch, 1) + " %" );
    layGlitch.vicII.cas.active.setChecked( _useCasGlitch );
    layGlitch.vicII.cas.slider.setPosition( (unsigned)(_casGlitch * 10.0) );
    layGlitch.vicII.cas.value.setText( GUIKIT::String::formatFloatingPoint(_casGlitch, 1) + " %" );

    layIntern.misc.option.distortionHires.setChecked( _distortionHires );
    layIntern.misc.option.hires.setChecked( _hires );
    layIntern.misc.luminance.slider.setPosition( _luminance );
    layIntern.misc.luminance.value.setText( std::to_string(_luminance) + " %" );
    layIntern.misc.lightFromCenter.active.setChecked( _useLightFromCenter );
    layIntern.misc.lightFromCenter.slider.setPosition( _lightFromCenter );
    layIntern.misc.lightFromCenter.value.setText( std::to_string(_lightFromCenter) + " %" );
    layIntern.subsampling.firFilter.slider.setPosition( (unsigned)(_firFilterLength / 2) );
    layIntern.subsampling.firFilter.value.setText( std::to_string( _firFilterLength ) );
    
    if (_firFilterSharp == -1)
        layIntern.subsampling.firSharp.sharpLeft.setChecked();
    else if (_firFilterSharp == 1)
        layIntern.subsampling.firSharp.sharpRight.setChecked();
    else
        layIntern.subsampling.firSharp.natural.setChecked();
        
    if ( _maskType == (unsigned)VideoManager::MaskType::ShadowMask )
        layIntern.mask.type.shadowMask.setChecked();
    else if ( _maskType == (unsigned)VideoManager::MaskType::SlotMask )
        layIntern.mask.type.slotMask.setChecked();
    else
        layIntern.mask.type.apertureMask.setChecked();
	
	updateVisibillity();
}

auto VideoLayout::updateVisibillity() -> void {
	
	bool _pal = emulator->getRegionEncoding() == Emulator::Interface::Region::Pal;
    bool isC64 = dynamic_cast<LIBC64::Interface*>(emulator);
	
	if (layBase.view.mode.spectrum.checked()) {
        layBase.view.phase.setEnabled(true);
        layBase.view.option.newLuma.setEnabled(true);
    } else {
        layBase.view.phase.setEnabled(false);
        layBase.view.option.newLuma.setEnabled(false);
    }
    layBase.view.scanlines.slider.setEnabled( layBase.view.scanlines.active.checked() );
    layBase.view.interlace.slider.setEnabled( layBase.view.interlace.active.checked() );
		
    bool crtChecked = layBase.view.mode.svideoCpu.checked() || layBase.view.mode.svideoGpu.checked();
    bool crtGpuChecked = layBase.view.mode.svideoGpu.checked();

    layBase.encoding.setEnabled( crtChecked );
    layBase.lumaDelay.setEnabled( (isC64 && crtChecked) || crtGpuChecked );
    
    if (crtChecked) {
        layBase.encoding.phaseError.slider.setEnabled( layBase.encoding.phaseError.active.checked() );
        layBase.encoding.hanoverBars.setEnabled( _pal );
        layBase.encoding.hanoverBars.slider.setEnabled( _pal && layBase.encoding.hanoverBars.active.checked() );
        layBase.encoding.blur.slider.setEnabled(  layBase.encoding.blur.active.checked() );
    }

    if ((isC64 && crtChecked) || crtGpuChecked) {
        layBase.lumaDelay.lumaRise.slider.setEnabled(layBase.lumaDelay.lumaRise.active.checked());
        layBase.lumaDelay.lumaFall.slider.setEnabled(layBase.lumaDelay.lumaFall.active.checked());
    }
    
    layBase.view.option.tvGamma.setEnabled( crtChecked && layBase.view.mode.palette.checked() && _pal );
	
    if (videoDriver->shaderFormat() == DRIVER::Video::ShaderType::NotSupported) {
        if(crtGpuChecked) {
            layBase.view.mode.svideoCpu.setChecked();
            _settings->set<unsigned>("video_crt", (unsigned)VideoManager::CrtMode::Cpu);
        }

        layBase.view.mode.svideoGpu.setEnabled(false);
        //tab2.setEnabled(false);
        //tab3.setEnabled(false);
        return;
    }

    layBase.view.mode.svideoGpu.setEnabled();
  //  tab2.setEnabled(crtGpuChecked);
    //tab3.setEnabled(crtGpuChecked);
    
    if ( crtGpuChecked )
        // crt with gpu don't use blur setting
        layBase.encoding.blur.setEnabled( false );

    layIntern.misc.setEnabled();
    layIntern.misc.option.hires.setEnabled();
    layIntern.misc.option.distortionHires.setEnabled();

    layIntern.mask.setEnabled( layIntern.mask.level.active.checked() );
    layIntern.mask.level.active.setEnabled();

    layGlitch.crt.radialDistortion.setEnabled();
    layGlitch.crt.radialDistortion.slider.setEnabled( layGlitch.crt.radialDistortion.active.checked() );

    layIntern.misc.luminance.setEnabled( !layIntern.mask.level.active.checked() );
    layIntern.misc.lightFromCenter.setEnabled();
    layIntern.misc.lightFromCenter.slider.setEnabled( layIntern.misc.lightFromCenter.active.checked() );

    layIntern.bloom.setEnabled( layIntern.bloom.glow.active.checked() );
    layIntern.bloom.glow.active.setEnabled();
    layIntern.bloom.weight.slider.setEnabled( layIntern.bloom.weight.active.checked() );

    // only enabled when GPU active
	
	if (crtGpuChecked) {
        layGlitch.crt.lumaNoise.slider.setEnabled( layGlitch.crt.lumaNoise.active.checked() );
        layGlitch.crt.chromaNoise.slider.setEnabled( layGlitch.crt.chromaNoise.active.checked() );
        layGlitch.crt.randomLineOffset.slider.setEnabled( layGlitch.crt.randomLineOffset.active.checked() );

        layGlitch.vicII.aec.slider.setEnabled( isC64 && layGlitch.vicII.aec.active.checked() );
        layGlitch.vicII.ba.slider.setEnabled( isC64 && layGlitch.vicII.ba.active.checked() );
        layGlitch.vicII.phi0.slider.setEnabled( isC64 && layGlitch.vicII.phi0.active.checked() );
        layGlitch.vicII.ras.slider.setEnabled( isC64 && layGlitch.vicII.ras.active.checked() );
        layGlitch.vicII.cas.slider.setEnabled( isC64 && layGlitch.vicII.cas.active.checked() );
	}
}

auto VideoLayout::translate() -> void {
    layBase.view.setText(trans->get("view"));

    layBase.view.saturation.name.setText( trans->get("saturation", {}, true) );
    layBase.view.gamma.name.setText( trans->get("gamma", {},true) );
    layBase.view.brightness.name.setText( trans->get("brightness", {}, true) );
    layBase.view.contrast.name.setText( trans->get("contrast", {}, true) );
    layBase.view.phase.name.setText( trans->get("phase", {}, true) );
    layBase.view.option.newLuma.setText( trans->get("new_luma") );
    layBase.view.option.tvGamma.setText( trans->get("TV gamma") );
    layBase.view.option.linearInterpolation.setText( trans->get("linear_interpolation") );
    layBase.view.mode.palette.setText( trans->get("palette") );
    layBase.view.mode.spectrum.setText( trans->get("color_spectrum") );
    layBase.view.mode.reset.setText( trans->get("reset") );
    layBase.view.mode.rgb.setText( trans->get("RGB") );
    layBase.view.mode.svideoCpu.setText( trans->get("S/C-Video") );
    layBase.view.mode.svideoCpu.setTooltip( trans->get("S/C-Video tooltip") );
    layBase.view.mode.svideoGpu.setText( trans->get("S/C-Video on GPU") );
    layBase.view.mode.svideoGpu.setTooltip( trans->get("S/C-Video tooltip") );
    layBase.view.scanlines.active.setText( trans->get("scanlines", {}, true) );
    layBase.view.interlace.active.setText( trans->get("interlace", {}, true) );

    layBase.encoding.setText(trans->get("color encoding"));
    layBase.encoding.phaseError.active.setText( trans->get("phase_error", {}, true) );
    layBase.encoding.hanoverBars.active.setText( trans->get("hanover_bars", {}, true) );
    layBase.encoding.blur.active.setText( trans->get("blur", {}, true) );
    layBase.lumaDelay.setText(trans->get("luma delay"));
    layBase.lumaDelay.lumaRise.active.setText( trans->get("luma_rise", {}, true) );
    layBase.lumaDelay.lumaFall.active.setText( trans->get("luma_fall", {}, true) );

    layIntern.misc.setText(trans->get("generic"));
    layIntern.misc.option.distortionHires.setText( trans->get("distortion_hires") );
    layIntern.misc.option.distortionHires.setTooltip( trans->get("distortion hires tooltip") );
    layIntern.misc.option.hires.setText( trans->get("hires") );
    layIntern.misc.luminance.name.setText( trans->get("luminance", {}, true) );
    layIntern.misc.lightFromCenter.active.setText( trans->get("light_from_center", {}, true) );

    layIntern.subsampling.setText(trans->get("chroma subsampling"));
    layIntern.subsampling.firFilter.name.setText( trans->get("fir filter blur", {}, true) );
    layIntern.subsampling.firSharp.sharpLeft.setText( trans->get("fir filter left") );
    layIntern.subsampling.firSharp.sharpRight.setText( trans->get("fir filter right") );
    layIntern.subsampling.firSharp.natural.setText( trans->get("fir filter natural") );

    layIntern.mask.setText( trans->get("mask") );
    layIntern.mask.type.label.setText( trans->get("type", {}, true) );
    layIntern.mask.type.apertureMask.setText( trans->get("aperture_mask") );
    layIntern.mask.type.shadowMask.setText( trans->get("shadow_mask") );
    layIntern.mask.type.slotMask.setText( trans->get("slot_mask") );
    layIntern.mask.level.active.setText( trans->get("intensity", {}, true) );
    layIntern.mask.luminance.name.setText( trans->get("luminance", {}, true) );
    layIntern.mask.pitch.name.setText( trans->get("pitch", {}, true) );
    layIntern.mask.dpi.name.setText( trans->get("DPI", {}, true) );

    layIntern.bloom.setText( trans->get("color_bloom") );
    layIntern.bloom.glow.active.setText( trans->get("glow", {}, true) );
    layIntern.bloom.radius.name.setText( trans->get("radius", {}, true) );
    layIntern.bloom.variance.name.setText( trans->get("variance", {}, true) );
    layIntern.bloom.weight.active.setText( trans->get("weight", {}, true) );

    layGlitch.crt.setText( trans->get("crt_glitches") );
    layGlitch.crt.lumaNoise.active.setText( trans->get("luma_noise", {}, true) );
    layGlitch.crt.chromaNoise.active.setText( trans->get("chroma_noise", {}, true) );
    layGlitch.crt.radialDistortion.active.setText( trans->get("radial_distortion", {}, true) );
    layGlitch.crt.randomLineOffset.active.setText( trans->get("random_line_offset", {}, true) );

    layGlitch.vicII.setText( trans->get("vicII_glitches") );
    layGlitch.vicII.toggleAll.setText( trans->get("toggle_all_glitches") );
    layGlitch.vicII.aec.active.setText(trans->get("aec_glitch",{}, true));
    layGlitch.vicII.ba.active.setText(trans->get("ba_glitch",{}, true));
    layGlitch.vicII.phi0.active.setText(trans->get("phi_glitch",{}, true));
    layGlitch.vicII.ras.active.setText(trans->get("ras_glitch",{}, true));
    layGlitch.vicII.cas.active.setText(trans->get("cas_glitch",{}, true));

    SliderLayout::scale({&layBase.view.saturation, &layBase.view.gamma, &layBase.view.brightness, &layBase.view.contrast, &layBase.view.phase, &layBase.view.scanlines, &layBase.view.interlace, &layBase.encoding.phaseError, &layBase.encoding.hanoverBars, &layBase.encoding.blur, &layBase.lumaDelay.lumaRise, &layBase.lumaDelay.lumaFall},
                        "-100 %");
    unsigned neededWidth = SliderLayout::scale({&layIntern.subsampling.firFilter, &layIntern.misc.lightFromCenter, &layIntern.misc.luminance, &layIntern.mask.level, &layIntern.mask.luminance, &layIntern.mask.dpi, &layIntern.mask.pitch, &layIntern.bloom.glow, &layIntern.bloom.radius, &layIntern.bloom.variance, &layIntern.bloom.weight},
                        "0.00 mm", layIntern.mask.type.label.minimumSize().width );
    SliderLayout::scale({&layGlitch.crt.lumaNoise, &layGlitch.crt.chromaNoise, &layGlitch.crt.randomLineOffset, &layGlitch.crt.radialDistortion, &layGlitch.vicII.aec, &layGlitch.vicII.ba, &layGlitch.vicII.phi0, &layGlitch.vicII.ras, &layGlitch.vicII.cas},
                        "100.0 %");

    layIntern.mask.type.children[ 0 ].size.width = neededWidth;
}

auto VideoLayout::sliderIdent() -> std::string {
	
	std::string ident = (emulator->getRegionEncoding() == Emulator::Interface::Region::Pal) ? "_pal" : "_ntsc";
	
    if (dynamic_cast<LIBC64::Interface*>(emulator) && layBase.view.mode.spectrum.checked())
        ident += "_spectrum";
	
	if (layBase.view.mode.svideoCpu.checked())
		ident += "_crtcpu";
	else if (layBase.view.mode.svideoGpu.checked())
		ident += "_crtgpu";
    else if (layBase.view.mode.externGpu.checked())
        ident += "_externgpu";

	return ident;
}

auto VideoLayout::loadSettings(bool init) -> void {
    VideoManager::CrtMode crtMode = (VideoManager::CrtMode)_settings->get<unsigned>("video_crt", (unsigned)VideoManager::CrtMode::None, {0u, 2u});
    
    if (crtMode == VideoManager::CrtMode::Gpu)
        layBase.view.mode.svideoGpu.setChecked();
    else if (crtMode == VideoManager::CrtMode::GpuExtern)
        layBase.view.mode.externGpu.setChecked();
    else if (crtMode == VideoManager::CrtMode::Cpu)
        layBase.view.mode.svideoCpu.setChecked();
    else
        layBase.view.mode.rgb.setChecked();
    
    if (dynamic_cast<LIBC64::Interface*>(emulator) && _settings->get<bool>( "video_spectrum", true) )
        layBase.view.mode.spectrum.setChecked();
    else
        layBase.view.mode.palette.setChecked();
        
    updatePresets(!init);
}