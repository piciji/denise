
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

VideoShaderLayout::Main::Control::Control() {
    append(unload,{0u, 0u}, 10);
    append(prependPreset,{0u, 0u}, 10);
    append(appendPreset,{0u, 0u},10);
    append(spacer,{~0u, 0u});
    append(apply,{0u, 0u}, 10);
    append(save,{0u, 0u}, 10);
    append(load,{0u, 0u});

    setAlignment(0.5);
}

VideoShaderLayout::Main::Info::Info() {
    append(label,{0u, 0u}, 10);
    append(loaded,{~0u, 0u});
    append(toParams,{0u, 0u});

    setAlignment(0.5);
    loaded.setFont(GUIKIT::Font::system("bold"));
    toParams.setEnabled(false);
}

VideoShaderLayout::Main::Main() {
    append(control,{~0u, 0u}, 10);
    append(info,{~0u, 0u});

    setPadding(10);
    setFont(GUIKIT::Font::system("bold"));
}

VideoShaderLayout::Favourite::Control::Control() {
    append(spacer,{~0u, 0u});
    append(remove,{0u, 0u}, 20);
    append(add,{0u, 0u});
    setAlignment(0.5);
}

VideoShaderLayout::Favourite::Favourite() {
    append(list,{~0u, ~0u}, 10);
    append(control,{~0u, 0u});

    setPadding(10);
    setFont(GUIKIT::Font::system("bold"));
    list.setHeaderText({""});
    list.setHeaderVisible(true);
}

VideoShaderLayout::VideoShaderLayout() {
    append(main,{~0u, 0u}, 10);
    append(favourite,{~0u, ~0u});
}

VideoPassLayout::Settings::Identifier::Identifier() {
    append(fileIdent,{0u, 0u}, 10);
    append(filter,{0u, 0u}, 10);
    append(wrap,{0u, 0u}, 10);
    append(bufferType,{0u, 0u}, 10);
    append(mipmap,{0u, 0u}, 10);
    append(modulo,{0u, 0u}, 10);
    append(scaleX,{0u, 0u}, 10);
    append(scaleY,{0u, 0u});
}

VideoPassLayout::Settings::Data::Filter::Filter() {
    append(unspec,{0u, 0u}, 10);
    append(linear,{0u, 0u}, 10);
    append(nearest,{0u, 0u});

    GUIKIT::RadioBox::setGroup( unspec, linear, nearest );
    setAlignment(0.5);
}

VideoPassLayout::Settings::Data::Data() {
    append(fileIdent,{0u, 0u}, 10);
    append(filter,{0u, 0u}, 10);
    append(wrap,{0u, 0u}, 10);
    append(type,{0u, 0u}, 10);
    append(mipmap,{0u, 0u}, 10);
    append(modulo,{0u, 0u}, 10);
    append(scaleX,{0u, 0u}, 10);
    append(scaleY,{0u, 0u});
}

VideoPassLayout::Settings::Settings() {
    append(identifier, {0u, 0u}, 20);
    append(data, {0u, 0u});
}

VideoPassLayout::Control::Control() {
    append(up,{0u, 0u}, 10);
    append(down,{0u, 0u}, 10);
    append(hide,{0u, 0u});
    setAlignment(0.5);
}

VideoPassLayout::VideoPassLayout() {
    append(settings,{0u, 0u}, 20);
    append(control,{0u, 0u});

    setPadding(8);
    setFont(GUIKIT::Font::system("bold"));
}

VideoParamLayout::Control::Control() {
    append(spacer, {~0u, 0u});
    append(previous, {0u, 0u}, 20);
    append(next, {0u, 0u});
}

VideoParamLayout::VideoParamLayout() {
    append(params, {~0u, ~0u}, 20);
    append(control, {~0u, 0u});

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
    pageUp.loadPng((uint8_t*)Icons::pageUp, sizeof(Icons::pageUp) );
    pageDown.loadPng((uint8_t*)Icons::pageDown, sizeof(Icons::pageDown) );
    pageUpGray.loadPng((uint8_t*)Icons::pageUpGray, sizeof(Icons::pageUpGray) );
    pageDownGray.loadPng((uint8_t*)Icons::pageDownGray, sizeof(Icons::pageDownGray) );

    tviBase.setUserData( (uintptr_t)1 );
    tviBase.setImage( imgFolderClosed );
    tviBase.setImageExpanded( imgFolderOpen );

    tviIntern.setUserData( (uintptr_t)11 );
    tviIntern.setImage(imgDocument);

    tviGlitch.setUserData( (uintptr_t)12 );
    tviGlitch.setImage(imgDocument);

    tviShader.setUserData( (uintptr_t)2 );
    tviShader.setImage(imgFolderClosed);
    tviShader.setImageExpanded(imgFolderOpen);

    tviParams.setUserData( (uintptr_t)3000 );
    tviParams.setImage(imgFolderClosed);
    tviParams.setImageExpanded(imgFolderOpen);

    tviBase.append(tviIntern);
    tviBase.append(tviGlitch);
    moduleTree.append(tviBase);

    moduleTree.append(tviShader);

    moduleSwitch.setLayout(1, layBase, {~0u, ~0u});
    moduleSwitch.setLayout(11, layIntern, {~0u, ~0u});
    moduleSwitch.setLayout(12, layGlitch, {~0u, ~0u});
    moduleSwitch.setLayout(2, layShader, {~0u, ~0u});
    moduleSwitch.setLayout(21, layPass, {~0u, ~0u});
    moduleSwitch.setLayout(3, layParam, {~0u, ~0u});

    tviBase.setExpanded();

    layNav.append( moduleTree, { GUIKIT::Font::scale(160), ~0u} );
    layNav.setPadding(10);
    layNav.setFont(GUIKIT::Font::system("bold"));
    append(layNav, {0u, ~0u}, 10);

    append( moduleSwitch, {~0u, ~0u} );

    layPass.control.up.setImage(&pageUpGray);
    layPass.control.up.setEnabled(false);
    layPass.control.down.setImage(&pageDownGray);
    layPass.control.down.setEnabled(false);

    moduleSwitch.setSelection( 1 );

    moduleTree.onChange = [this]() {
        auto item = moduleTree.selected();
        if (!item)
            return;

        unsigned navIdent = (unsigned)item->userData();

        if (navIdent >= 3000) {
            unsigned pos = navIdent - 3000;
            navIdent = 3;
            if (pos < params.size()) {
                selectedParamId = pos;
                buildParams(params[pos]);
            }
        } else if (navIdent >= 210 ) {
            ShaderPreset* preset = vManager()->getPreset();
            unsigned passPos = navIdent - 210;
            navIdent = 21;

            if (preset) {
                if (passPos < preset->passes.size()) {
                    ShaderPreset::Pass& pass = preset->passes[passPos];
                    selectedPassId = passPos;
                    buildPass(preset, pass);
                }
            }
        } 

        moduleSwitch.setSelection( navIdent );
    };

    setMargin(10);

    setSliderAction<unsigned>( &layBase.view.gamma, "gamma", [](unsigned position) { return position + 30; } );
    setSliderAction<unsigned>( &layBase.view.saturation, "saturation" );
    setSliderAction<unsigned>( &layBase.view.brightness, "brightness" );
    setSliderAction<unsigned>( &layBase.view.contrast, "contrast" );
    setSliderAction<int>( &layBase.view.phase, "phase", [](unsigned position) { return (int)position - 180; } );
    setSliderAction<unsigned>( &layBase.view.scanlines, "scanlines", [](unsigned position) { return std::max(position, 1u); } );
    setSliderAction<unsigned>( &layBase.view.interlace, "interlace", [](unsigned position) { return std::max(position, 1u); } );
    setSliderAction<unsigned>( &layBase.encoding.blur, "blur" );
    setSliderAction<float>( &layBase.encoding.phaseError, "phase_error", [](unsigned position) { return (float)((int)position - 90) / 2.0f; } );
    setSliderAction<int>( &layBase.encoding.hanoverBars, "hanover_bars", [](unsigned position) { return (int)position - 100; } );
    setSliderAction<float>( &layIntern.mask.pitch, "mask_pitch", [](unsigned position) { return (float)position / 100.0f; } );
    setSliderAction<unsigned>( &layIntern.mask.dpi, "mask_dpi" );
    setSliderAction<unsigned>( &layIntern.mask.level, "mask_level", [](unsigned position) { return std::max(position, 1u); } );
    setSliderAction<unsigned>( &layIntern.mask.luminance, "mask_luminance" );
    setSliderAction<unsigned>( &layIntern.subsampling.firFilter, "fir_filter_length", [](unsigned position) { return position * 2 + 1; } );
    setSliderAction<float>( &layGlitch.crt.lumaNoise, "luma_noise", [](unsigned position) { return (float)std::max(position, 1u) / 10.0f; });
    setSliderAction<float>( &layGlitch.crt.chromaNoise, "chroma_noise", [](unsigned position) { return (float)std::max(position, 1u) / 10.0f; } );
    setSliderAction<unsigned>( &layGlitch.crt.radialDistortion, "radial_distortion", [](unsigned position) { return std::max(position, 1u); } );
    setSliderAction<float>( &layGlitch.vicII.aec, "aec_glitch", [](unsigned position) { return (float)std::max(position, 1u) / 10.0f; } );
    setSliderAction<float>( &layGlitch.vicII.ba, "ba_glitch", [](unsigned position) { return (float)std::max(position, 1u) / 10.0f; } );
    setSliderAction<float>( &layGlitch.vicII.phi0, "phi0_glitch", [](unsigned position) { return (float)std::max(position, 1u) / 10.0f; } );
    setSliderAction<float>( &layGlitch.vicII.ras, "ras_glitch", [](unsigned position) { return (float)std::max(position, 1u) / 10.0f; } );
    setSliderAction<float>( &layGlitch.vicII.cas, "cas_glitch", [](unsigned position) { return (float)std::max(position, 1u) / 10.0f; } );
    setSliderAction<float>( &layBase.lumaDelay.lumaRise, "luma_rise", [](unsigned position) { return ((float)std::max(position, 1u) / 10.0f) + 1.0f; } );
    setSliderAction<float>( &layBase.lumaDelay.lumaFall, "luma_fall", [](unsigned position) { return ((float)std::max(position, 1u) / 10.0f) + 1.0f; } );
    setSliderAction<unsigned>( &layIntern.misc.lightFromCenter, "light_from_center", [](unsigned position) { return std::max(position, 1u); } );
    setSliderAction<unsigned>( &layIntern.misc.luminance, "luminance" );
	setSliderAction<unsigned>( &layIntern.bloom.glow, "bloom_glow", [](unsigned position) { return std::max(position, 1u); } );
	setSliderAction<float>( &layIntern.bloom.variance, "bloom_variance", [](unsigned position) { return ((float)position / 10.0f) + 1.0f; } );
	setSliderAction<unsigned>( &layIntern.bloom.radius, "bloom_radius", [](unsigned position) { return position + 1; } );
	setSliderAction<float>( &layIntern.bloom.weight, "bloom_weight", [](unsigned position) { return (float)std::max(position, 1u) / 100.0f; } );
    setSliderAction<float>( &layGlitch.crt.randomLineOffset, "random_line_offset", [](unsigned position) { return (float)std::max(position, 1u) / 100.0f; } );
    
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

    layShader.main.control.load.onActivate = [this]() {
        static const std::vector<std::string> suffixList = {"slang", "slangp"};
        auto path = GUIKIT::BrowserWindow()
                .setTitle(trans->getA("select slang shader"))
                .setPath( _settings->get<std::string>("slang_folder", "") )
                .setFilters({ GUIKIT::BrowserWindow::transformFilter("SLANG", suffixList ) })
                .open();

        if (path.empty())
            return;

        std::vector<std::string> brokenPaths;
        ShaderPreset* preset = vManager()->loadPreset(path, brokenPaths);

        if (preset) {
            buildShaderUI(preset);
            layShader.main.info.loaded.setText( GUIKIT::String::getFileName( path, true ) );
            _settings->set<std::string>("slang_folder", GUIKIT::File::getPath(path));
            _settings->set<std::string>("slang_loaded", path);
            layShader.main.control.setEnabled();
            layShader.main.control.apply.setEnabled(false);
        }

        showBrokenPaths(brokenPaths);
    };

    layShader.main.control.prependPreset.onActivate = [this]() {
        static const std::vector<std::string> suffixList = {"slang", "slangp"};
        auto path = GUIKIT::BrowserWindow()
                .setTitle(trans->getA("select slang shader"))
                .setPath( _settings->get<std::string>("slang_folder", "") )
                .setFilters({ GUIKIT::BrowserWindow::transformFilter("SLANG", suffixList ) })
                .open();

        if (path.empty())
            return;

        std::vector<std::string> brokenPaths;
        ShaderPreset* preset = vManager()->addPreset(path, true, brokenPaths);

        if (preset) {
            buildShaderUI(preset);
            layShader.main.info.loaded.setText( GUIKIT::String::getFileName( vManager()->getPresetPathCombined() ) );
            _settings->set<std::string>("slang_folder", GUIKIT::File::getPath(path));
            layShader.main.control.apply.setEnabled(false);
            videoDriver->setShader( preset );
        }
        showBrokenPaths(brokenPaths);
    };

    layShader.main.control.appendPreset.onActivate = [this]() {
        static const std::vector<std::string> suffixList = {"slang", "slangp"};
        auto path = GUIKIT::BrowserWindow()
                .setTitle(trans->getA("select slang shader"))
                .setPath( _settings->get<std::string>("slang_folder", "") )
                .setFilters({ GUIKIT::BrowserWindow::transformFilter("SLANG", suffixList ) })
                .open();

        if (path.empty())
            return;

        std::vector<std::string> brokenPaths;
        ShaderPreset* preset = vManager()->addPreset(path, false, brokenPaths);

        if (preset) {
            buildShaderUI(preset);
            layShader.main.info.loaded.setText( GUIKIT::String::getFileName( vManager()->getPresetPathCombined() ) );
            _settings->set<std::string>("slang_folder", GUIKIT::File::getPath(path));
            layShader.main.control.apply.setEnabled(false);
            videoDriver->setShader( preset );
        }
        showBrokenPaths(brokenPaths);
    };

    layShader.main.control.unload.onActivate = [this]() {
        vManager()->clearPreset();
        buildShaderUI(nullptr);
        layShader.main.info.loaded.setText( "" );
        _settings->set<std::string>("slang_loaded", "");
        layShader.main.control.setEnabled(false);
        layShader.main.control.load.setEnabled();
        videoDriver->setShader( nullptr );
        clearBrokenPaths();
    };

    layShader.main.control.save.onActivate = [this]() {
        static const std::vector<std::string> suffixList = {"slangp"};
        auto path = GUIKIT::BrowserWindow()
                .setTitle(trans->getA("select slang shader"))
                .setPath( _settings->get<std::string>("slang_folder_save", "") )
                .setFilters({ GUIKIT::BrowserWindow::transformFilter("SLANG", suffixList ) })
                .save();

        if (path.empty())
            return;

        if ( !GUIKIT::String::foundSubStr( path, ".slangp" ))
            path += ".slangp";

        if (vManager()->savePreset(path)) {
            layShader.main.info.loaded.setText( GUIKIT::String::getFileName( path, true ) );
            _settings->set<std::string>("slang_folder_save", GUIKIT::File::getPath(path));
            _settings->set<std::string>("slang_loaded", path);
        }
    };

    layShader.main.control.apply.onActivate = [this]() {
        videoDriver->setShader( vManager()->getPreset() );
    };

    layShader.favourite.control.add.onActivate = [this]() {
        std::string path = vManager()->getPresetPath();

        if (path.empty())
            return;

        int i = 0;

        while(1) {
            std::string fav = _settings->get<std::string>( "shader_fav_" + std::to_string(i), "");

            if (fav == path)
                return;

            if (fav == "") {
                layShader.favourite.list.append({path});
                _settings->set<std::string>( "shader_fav_" + std::to_string(i), path);
                break;
            }
            i++;
        }
    };

    layShader.favourite.control.remove.onActivate = [this]() {
        if (!layShader.favourite.list.selected())
            return;

        std::vector<std::string> storage;
        int selection = layShader.favourite.list.selection();
        layShader.favourite.list.reset();

        int i = 0;
        while(1) {
            std::string path = _settings->get<std::string>( "shader_fav_" + std::to_string(i), "");

            if (path.empty())
                break;

            if (i != selection)
                storage.push_back(path);

            _settings->set<std::string>( "shader_fav_" + std::to_string(i), "");
            i++;
        }

        i = 0;
        for(auto& fav : storage) {
            layShader.favourite.list.append({fav});
            _settings->set<std::string>( "shader_fav_" + std::to_string(i), fav);
            i++;
        }
    };

    layShader.favourite.list.onActivate = [this]() {
        int selection = layShader.favourite.list.selection();

        std::string path = layShader.favourite.list.text(selection, 0);

        std::vector<std::string> brokenPaths;
        ShaderPreset* preset = vManager()->loadPreset(path, brokenPaths);

        if (preset) {
            buildShaderUI(preset);
            layShader.main.info.loaded.setText( GUIKIT::String::getFileName( path, true ) );
            _settings->set<std::string>("slang_loaded", path);
            layShader.main.control.setEnabled();
            layShader.main.control.apply.setEnabled(false);
            videoDriver->setShader( preset );
        }
        showBrokenPaths(brokenPaths);
    };

    layPass.control.hide.onActivate = [this]() {
        auto pass = vManager()->togglePassUsage(selectedPassId);
        if(!pass)
            return;

        if (!pass->inUse) {
            layPass.control.hide.setText( trans->getA("unhide") );
            layPass.settings.setEnabled(false);
        } else {
            layPass.control.hide.setText( trans->getA("hide") );
            layPass.settings.setEnabled(true);
        }
        if (!layShader.main.control.apply.enabled())
            layShader.main.control.apply.setEnabled();
    };

    layPass.control.down.onClick = [this]() {
        unsigned passIdBefore = selectedPassId;
        vManager()->movePass( selectedPassId, false);

        if (passIdBefore != selectedPassId) {
            auto preset = vManager()->getPreset();

            for(int i = 0; i < preset->passes.size(); i++) {
                ShaderPreset::Pass& pass = preset->passes[i];

                std::string passIdent = std::to_string(i);
                if (!pass.alias.empty())
                    passIdent += " " + pass.alias;

                if (i < tviPasses.size())
                    tviPasses[i]->setText( passIdent );
            }
            updateMoveImg();
            if (!layShader.main.control.apply.enabled())
                layShader.main.control.apply.setEnabled();
        }

        tviPasses[selectedPassId]->setSelected();
    };

    layPass.control.up.onClick = [this]() {
        unsigned passIdBefore = selectedPassId;
        vManager()->movePass( selectedPassId, true);

        if (passIdBefore != selectedPassId) {
            auto preset = vManager()->getPreset();

            for(int i = 0; i < preset->passes.size(); i++) {
                ShaderPreset::Pass& pass = preset->passes[i];

                std::string passIdent = std::to_string(i);
                if (!pass.alias.empty())
                    passIdent += " " + pass.alias;

                if (i < tviPasses.size())
                    tviPasses[i]->setText( passIdent );
            }
            updateMoveImg();
            if (!layShader.main.control.apply.enabled())
                layShader.main.control.apply.setEnabled();
        }

        tviPasses[selectedPassId]->setSelected();
    };

    for(int i = 0; i < PARAMS_PER_PAGE; i++) {
        SliderLayoutAlt* sliLayout = new SliderLayoutAlt(true);

        sliLayout->updateWidget = [this, i](unsigned position) {
            float val = 0.0;
            auto preset = vManager()->getPreset();
            if (!preset)
                return val;

            if (selectedParamId < params.size()) {
                auto& tviParam = params[selectedParamId];
                unsigned offset = tviParam.starts + i;

                if (offset < preset->params.size()) {
                    ShaderPreset::Param& param = preset->params[offset];
                    val = (float) position * param.step + param.minimum;
                    param.value = val;
                }
            }
            return val;
        };

        sliLayout->requestDefault = [this, i]() {
            float val = 0.0;
            auto preset = vManager()->getPreset();
            if (!preset)
                return val;

            if (selectedParamId < params.size()) {
                auto& tviParam = params[selectedParamId];
                unsigned offset = tviParam.starts + i;

                if (offset < preset->params.size()) {
                    ShaderPreset::Param& param = preset->params[offset];
                    param.value = param.initial;
                    val = param.initial;
                }
            }
            return val;
        };

        paramSliders[i] = sliLayout;
    }

    layParam.control.previous.onActivate = [this]() {
        if (!selectedParamId || selectedParamId > params.size())
            return;

        selectedParamId -= 1;
        auto& param = params[selectedParamId];

        if (!param.tvi)
            tviParams.setSelected();
        else
            param.tvi->setSelected();

        buildParams(param);
        moduleSwitch.setSelection( 3 );
    };

    layParam.control.next.onActivate = [this]() {
        if (selectedParamId >= (params.size() - 1) )
            return;

        selectedParamId += 1;
        auto& param = params[selectedParamId];

        if (!param.tvi)
            tviParams.setSelected();
        else
            param.tvi->setSelected();

        buildParams(param);
        moduleSwitch.setSelection( 3 );
    };

    layShader.main.info.toParams.onActivate = [this]() {
        selectedParamId = 0;
        if (selectedParamId >= params.size())
            return;

        auto& param = params[selectedParamId];

        if (!param.tvi)
            tviParams.setSelected();
        else
            param.tvi->setSelected();

        buildParams(param);
        moduleSwitch.setSelection( 3 );
    };

    layPass.settings.data.filter.nearest.onActivate = [this]() {
        vManager()->setPassFilter(selectedPassId, ShaderPreset::FILTER_NEAREST);
    };

    layPass.settings.data.filter.linear.onActivate = [this]() {
        vManager()->setPassFilter(selectedPassId, ShaderPreset::FILTER_LINEAR);
    };

    layPass.settings.data.filter.unspec.onActivate = [this]() {
        vManager()->setPassFilter(selectedPassId, ShaderPreset::FILTER_UNSPEC);
    };

    loadSettings(true);
}

auto VideoLayout::countFloatingPoint(ShaderPreset::Param& param, int& places, int& decimalPlaces) -> void {
    int placesStep = 0;
    int placesMinimum = 0;
    int placesMaximum = 0;

    int decimalPlacesStep = GUIKIT::String::countDecimalPlaces( param.step, placesStep );
    int decimalPlacesMinimum = GUIKIT::String::countDecimalPlaces( param.minimum, placesMinimum );
    int decimalPlacesMaximum = GUIKIT::String::countDecimalPlaces( param.maximum, placesMaximum );

    decimalPlaces = std::max(decimalPlacesMinimum, decimalPlacesMaximum);
    decimalPlaces = std::max(decimalPlaces, decimalPlacesStep);
    if (decimalPlaces > 6)
        decimalPlaces = 6;

    places = std::max(placesMinimum, placesMaximum);
    places = std::max(places, placesStep);
}

auto VideoLayout::buildShaderUI(ShaderPreset* preset) -> void {
    for(auto tviPass : tviPasses) {
        tviShader.remove(*tviPass);
        delete tviPass;
    }

    for(auto& param : params) {
        if (param.tvi) {
            tviParams.remove(*param.tvi);
            delete param.tvi;
        }
    }

    tviPasses.clear();
    params.clear();
    moduleTree.remove(tviParams);
    layShader.main.info.toParams.setEnabled(false);

    if (!preset)
        return;

    for(int i = 0; i < preset->passes.size(); i++) {
        ShaderPreset::Pass& pass = preset->passes[i];
        auto tviPass = new GUIKIT::TreeViewItem;

        std::string passIdent = std::to_string(i);
        if (!pass.alias.empty())
            passIdent += " " + pass.alias;

        tviPass->setUserData( (uintptr_t)(210 + i) );
        tviPass->setText( passIdent );
        tviPass->setImage( imgDocument );
        tviShader.append(*tviPass);

        tviPasses.push_back(tviPass);
    }

    GUIKIT::TreeViewItem* tviParam = nullptr;
    unsigned paramCount = preset->params.size();
    unsigned i = 0;
    unsigned start = 0;
    unsigned pageElement = 0;
    for(auto& param : preset->params) {
        i++;

        if (++pageElement == PARAMS_PER_PAGE) {
            if (param.isDescriptor()) {
                params.push_back( {tviParam, start, pageElement - 1 } );
                pageElement = 1;
                start = i - 1;
            } else {
                params.push_back( {tviParam, start, pageElement } );
                pageElement = 0;
                start = i;
            }

            tviParam = new GUIKIT::TreeViewItem;
            tviParam->setUserData( (uintptr_t)(3000 + params.size() ) );
            tviParam->setImage( imgDocument );
        }
    }

    if (pageElement)
        params.push_back( {tviParam, start, pageElement } );
    else if (tviParam)
        delete tviParam;

    for(auto& param : params) {
        if (param.tvi) {
            param.tvi->setText( preset->params[param.starts].desc );
            tviParams.append(*param.tvi);
        }
    }

    if (params.size()) {
        moduleTree.append(tviParams);
        layShader.main.info.toParams.setEnabled();
    }

    tviShader.setExpanded();
    tviParams.setExpanded();
}

auto VideoLayout::buildParams(TviParam& tviParam) -> void {
    ShaderPreset* preset = vManager()->getPreset();
    if (!preset)
        return;
    std::vector<SliderLayoutAlt*> sl;

    for(int i = 0; i < PARAMS_PER_PAGE; i++) {
        layParam.params.remove(*paramSliders[i]);
    }

    layParam.params.reset();
    int placesMax = 0;
    int decimalPlacesMax = 0;

    for(int i = 0; i < tviParam.counts; i++) {
        auto sliderLay = paramSliders[i];
        int offset = tviParam.starts + i;
        auto& shaderParam = preset->params[offset];


        std::string _desc = shaderParam.desc;
        if (GUIKIT::Application::isWinApi())
            GUIKIT::String::replace(_desc, "&", "&&");
        sliderLay->name.setText(_desc);

        int places = 0;
        int decimalPlaces = 0;
        countFloatingPoint(shaderParam, places, decimalPlaces);
        sliderLay->updateView(shaderParam.value, shaderParam.minimum, shaderParam.maximum, shaderParam.step, decimalPlaces);
        placesMax = std::max(placesMax, places);
        decimalPlacesMax = std::max(decimalPlacesMax, decimalPlaces);

        sl.push_back(sliderLay);

        layParam.params.append(*sliderLay, {~0u, 0u}, 10);
    }

    std::string s(placesMax + decimalPlacesMax + 1, '0');
    SliderLayoutAlt::scale(sl, s);

    layParam.control.previous.setEnabled(selectedParamId > 0);
    layParam.control.next.setEnabled(selectedParamId < (params.size() - 1) );

    layParam.params.synchronizeLayout();
}

auto VideoLayout::buildPass(ShaderPreset* preset, ShaderPreset::Pass& pass) -> void {
    layPass.settings.data.fileIdent.setText( GUIKIT::String::getFileName( pass.src ) );
    layPass.control.hide.setText( trans->getA(pass.inUse ? "hide" : "unhide") );

    switch(pass.filter) {
        default:
        case ShaderPreset::FILTER_UNSPEC: layPass.settings.data.filter.unspec.setChecked(); break;
        case ShaderPreset::FILTER_LINEAR: layPass.settings.data.filter.linear.setChecked(); break;
        case ShaderPreset::FILTER_NEAREST: layPass.settings.data.filter.nearest.setChecked(); break;
    }

    switch(pass.wrap) {
        default:
        case ShaderPreset::WRAP_EDGE: layPass.settings.data.wrap.setText("edge"); break;
        case ShaderPreset::WRAP_BORDER: layPass.settings.data.wrap.setText("border"); break;
        case ShaderPreset::WRAP_REPEAT: layPass.settings.data.wrap.setText("repeat"); break;
        case ShaderPreset::WRAP_MIRRORED_REPEAT: layPass.settings.data.wrap.setText("mirror"); break;
    }

    switch(pass.bufferType) {
        default:
        case ShaderPreset::BUFFER_UNORM: layPass.settings.data.type.setText("unorm"); break;
        case ShaderPreset::BUFFER_SRGB: layPass.settings.data.type.setText("srgb"); break;
        case ShaderPreset::BUFFER_FP: layPass.settings.data.type.setText("fp"); break;
    }

    layPass.settings.data.mipmap.setText(pass.mipmap ? "true" : "false");
    layPass.settings.data.modulo.setText( std::to_string( pass.frameModulo ));

    std::string scaleX = "";
    std::string scaleY = "";

    switch(pass.scaleTypeX) {
        default:
        case ShaderPreset::SCALE_INPUT: scaleX = "Input - " + GUIKIT::String::formatFloatingPoint(pass.scaleX, 2); break;
        case ShaderPreset::SCALE_VIEWPORT: scaleX = "Viewport - " + GUIKIT::String::formatFloatingPoint(pass.scaleX, 2); break;
        case ShaderPreset::SCALE_ABSOLUTE: scaleX = "Absolute - " + std::to_string( pass.absX ); break;
    }

    switch(pass.scaleTypeY) {
        default:
        case ShaderPreset::SCALE_INPUT: scaleY = "Input - " + GUIKIT::String::formatFloatingPoint(pass.scaleY, 2); break;
        case ShaderPreset::SCALE_VIEWPORT: scaleY = "Viewport - " + GUIKIT::String::formatFloatingPoint(pass.scaleY, 2); break;
        case ShaderPreset::SCALE_ABSOLUTE: scaleY = "Absolute - " + std::to_string( pass.absY ); break;
    }

    layPass.settings.data.scaleX.setText( scaleX );
    layPass.settings.data.scaleY.setText( scaleY );

    layPass.settings.setEnabled(pass.inUse);

    updateMoveImg();
}

auto VideoLayout::updateMoveImg() -> void {
    auto preset = vManager()->getPreset();
    if (preset) {
        GUIKIT::Image* imgUp = &pageUp;
        GUIKIT::Image* imgDown = &pageDown;

        if (selectedPassId == 0)
            imgUp = &pageUpGray;

        if (selectedPassId == (preset->passes.size() - 1) )
            imgDown = &pageDownGray;

        if (layPass.control.up.image() != imgUp) {
            layPass.control.up.setImage(imgUp);
            layPass.control.up.setEnabled( imgUp == &pageUp );
        }
        if (layPass.control.down.image() != imgDown) {
            layPass.control.down.setImage(imgDown);
            layPass.control.down.setEnabled( imgDown == &pageDown );
        }
    }
}

template<typename T> auto VideoLayout::setSliderAction( SliderLayout* layout, std::string baseIdent, std::function<T ( unsigned position )> callTransfer ) -> void {
    		
    if (layout->withActivator)
        layout->active.onToggle = [this, layout, baseIdent, callTransfer](bool checked) {
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

    layout->slider.onChange = [this, layout, baseIdent, callTransfer](unsigned position) {
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
    layBase.view.mode.externGpu.setText( trans->get("extern GPU") );
    layBase.view.scanlines.active.setText( trans->get("scanlines", {}, true) );
    layBase.view.interlace.active.setText( trans->get("interlace", {}, true) );

    layBase.encoding.setText(trans->get("color encoding"));
    layBase.encoding.phaseError.active.setText( trans->get("phase_error", {}, true) );
    layBase.encoding.hanoverBars.active.setText( trans->get("hanover_bars", {}, true) );
    layBase.encoding.blur.active.setText( trans->get("blur", {}, true) );
    layBase.lumaDelay.setText(trans->get("luma delay"));
    layBase.lumaDelay.lumaRise.active.setText( trans->get("luma_rise", {}, true) );
    layBase.lumaDelay.lumaFall.active.setText( trans->get("luma_fall", {}, true) );

    layIntern.misc.setText(trans->get("miscellaneous"));
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

    layShader.main.control.prependPreset.setText( trans->getA("prepend preset") );
    layShader.main.control.appendPreset.setText( trans->getA("append preset") );
    layShader.main.control.unload.setText( trans->getA("unload") );
    layShader.main.control.apply.setText( trans->getA("apply") );
    layShader.main.control.save.setText( trans->getA("save") );
    layShader.main.control.load.setText( trans->getA("load") );

    layShader.main.setText( trans->getA("Shader") );
    layShader.favourite.setText( trans->getA("favourites") );
    layShader.favourite.list.setHeaderText({trans->getA("selection")});

    layShader.main.info.label.setText( trans->getA("loaded", true) );
    layShader.main.info.toParams.setText( trans->getA("params") );
    layShader.favourite.control.add.setText( trans->getA("add") );
    layShader.favourite.control.remove.setText( trans->getA("remove") );

    layPass.settings.identifier.fileIdent.setText( trans->getA("file", true) );
    layPass.settings.identifier.filter.setText( trans->getA("filter", true) );
    layPass.settings.identifier.wrap.setText( trans->getA("Wrap", true) );
    layPass.settings.identifier.bufferType.setText( trans->getA("buffer format", true) );
    layPass.settings.identifier.mipmap.setText( trans->getA("Mipmap", true) );
    layPass.settings.identifier.modulo.setText( trans->getA("Modulo", true) );
    layPass.settings.identifier.scaleX.setText( trans->getA("Scaling X", true) );
    layPass.settings.identifier.scaleY.setText( trans->getA("Scaling Y", true) );

    layParam.control.previous.setText( trans->getA("previous") );
    layParam.control.next.setText( trans->getA("next") );

    tviBase.setText( trans->getA("generic") );
    tviIntern.setText( trans->getA("internal Shader") );
    tviGlitch.setText( trans->getA("Glitches") );
    tviShader.setText( trans->getA("external Shader") );
    tviParams.setText( trans->getA("Parameter") );

    layNav.setText( trans->getA("selection") );
    layPass.setText( trans->getA("Pass") );
    layParam.setText( trans->getA("Parameter") );

    layPass.settings.data.filter.nearest.setText( trans->getA("nearest") );
    layPass.settings.data.filter.linear.setText( trans->getA("linear") );
    layPass.settings.data.filter.unspec.setText( trans->getA("unspec") );

    for(int i = 0; i < PARAMS_PER_PAGE; i++) {
        paramSliders[i]->defaultButton.setText( trans->getA("default") );
    }

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

    int i = 0;
    layShader.favourite.list.reset();
    while(1) {
        std::string fav = _settings->get<std::string>( "shader_fav_" + std::to_string(i), "");
        if (fav.empty())
            break;

        layShader.favourite.list.append({fav});
        i++;
    }

    std::string shaderPath = _settings->get<std::string>("slang_loaded", "");
    bool shaderLoaded = false;

    if (!shaderPath.empty()) {
        std::vector<std::string> brokenPaths;
        ShaderPreset* preset = vManager()->loadPreset(shaderPath, brokenPaths);

        if (preset) {
            buildShaderUI(preset);
            layShader.main.info.loaded.setText(GUIKIT::String::getFileName(shaderPath, true));
            shaderLoaded = true;
            videoDriver->setShader( preset );
        }
        showBrokenPaths(brokenPaths);
    }

    if (!shaderLoaded)
        layShader.main.control.unload.onActivate();
    else
        layShader.main.control.setEnabled();
        
    updatePresets(!init);

    layBase.view.option.linearInterpolation.setChecked( _settings->get<unsigned>("video_filter", 1u, {0u, 1u}) );
}

auto VideoLayout::clearBrokenPaths() -> void {
    std::vector<std::string> brokenPaths;
    showBrokenPaths(brokenPaths);
}

auto VideoLayout::showBrokenPaths(std::vector<std::string>& brokenPaths) -> void {
    bool hasLabels = layShader.main.brokenLabels.size();
    for(auto brokenLabel : layShader.main.brokenLabels) {
        layShader.main.remove(*brokenLabel);
        delete brokenLabel;
    }
    layShader.main.brokenLabels.clear();

    for (auto& brokenPath : brokenPaths) {
        auto label = new GUIKIT::Label;
        label->setText(brokenPath);
        label->setForegroundColor(0xff4500);
        layShader.main.brokenLabels.push_back(label);
        layShader.main.append(*label, {0u, 0u}, 2);
    }

    if (hasLabels || brokenPaths.size())
        layShader.synchronizeLayout();
}
