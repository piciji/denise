
std::vector<DisplayFont> VideoLayout::displayFonts;

VideoBaseLayout::View::Mode::Mode(bool withSpectrum) {
    if (withSpectrum) {
        append(palette,{0u, 0u}, 10);
        append(spectrum,{0u, 0u}, 20);
        GUIKIT::RadioBox::setGroup(palette, spectrum);
    }

    append(rgb,{0u, 0u}, 10);
    append(cpu,{0u, 0u}, 10);
    append(gpu,{0u, 0u});

    append(spacer,{~0u, 0u});
    append(reset,{0u, 0u});

    GUIKIT::RadioBox::setGroup(rgb, cpu, gpu);

    setAlignment(0.5);
}

VideoBaseLayout::View::Option::Option(bool withSpectrum) {
    if (withSpectrum) {
        append(newLuma, {0u, 0u}, 10);
        append(tvGamma, {0u, 0u}, 10);
    } else {
        append(tvGamma, {0u, 0u}, 10);
    }

    append(linearInterpolation, {0u, 0u}, 10);
    append(cpuFilterThreaded, {0u, 0u});

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
    append(scanlines,{~0u, 0u}, withSpectrum ? 0 : 2);

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

    if (withSpectrum)
        append(lumaDelay, {~0u, 0u});
}

VideoShaderLayout::Main::Control::Control() {
    append(unload,{0u, 0u}, 10);
    append(spacer,{~0u, 0u});
    append(folder,{0u, 0u}, 5);
    append(internal,{0u, 0u}, 5);
    append(external,{0u, 0u}, 10);
    append(downloadSlang,{0u, 0u}, 10);
    append(prependPreset,{0u, 0u}, 10);
    append(appendPreset,{0u, 0u}, 10);
    append(load,{0u, 0u});

    GUIKIT::RadioBox::setGroup(internal, external);
    internal.setChecked();
    unload.setEnabled(false);
    prependPreset.setEnabled(false);
    appendPreset.setEnabled(false);

    setAlignment(0.5);
}

VideoShaderLayout::Main::Info::Info() {
    append(label,{0u, 0u}, 5);
    append(loaded,{~0u, 0u});
    append(shaderCache, {0u, 0u}, 10);
    append(clearCache,{0u, 0u}, 10);
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
    append(remove,{0u, 0u});
    append(spacer,{~0u, 0u});
    append(add,{0u, 0u});
    add.setEnabled(false);
    remove.setEnabled(false);
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

VideoPassLayout::Settings::Line::Line() {
    append(ident,{0u, 0u}, 10);
    append(value,{0u, 0u});

    setAlignment(0.5);
}

VideoPassLayout::Settings::FilterLine::FilterLine() {
    append(ident,{0u, 0u}, 10);
    append(unspec,{0u, 0u}, 10);
    append(linear,{0u, 0u}, 10);
    append(nearest,{0u, 0u}, 10);

    GUIKIT::RadioBox::setGroup( unspec, linear, nearest );
    setAlignment(0.5);
}

VideoPassLayout::Settings::MipmapLine::MipmapLine() {
    append(ident,{0u, 0u}, 10);
    append(checkBox,{0u, 0u});

    setAlignment(0.5);
}

VideoPassLayout::Settings::ScaleLine::ScaleLine() {
    append(ident,{0u, 0u}, 10);
    append(value,{0u, 0u}, 10);

    std::vector<GUIKIT::RadioBox*> boxes;
    for(int i = 0; i < SCALE_BOXES; i++) {
        radios[i].setText( std::to_string(i+1) + "x" );
        append(radios[i], {0u, 0u}, 10);
        boxes.push_back(&radios[i]);
    }

    GUIKIT::RadioBox::setGroup( boxes );

    setAlignment(0.5);
}

VideoPassLayout::Settings::Settings() {
    append(file, {0u, 0u}, 10);
    append(filter, {0u, 0u}, 10);
    append(wrap, {0u, 0u}, 10);
    append(bufferType, {0u, 0u}, 10);
    append(mipmap, {0u, 0u}, 10);
    append(modulo, {0u, 0u}, 10);
    append(scaleX, {0u, 0u}, 10);
    append(scaleY, {0u, 0u});
}

VideoPassLayout::Control::Control() {
    append(up,{0u, 0u}, 10);
    append(down,{0u, 0u}, 10);
    append(disable,{0u, 0u});
    append(spacer,{~0u, 0u});
    append(save,{0u, 0u});
    setAlignment(0.5);
}

VideoPassLayout::Generated::Generated() {
    append(errorLabel,{0u, 0u});
    append(spacer,{~0u, 0u});
    append(vertex,{0u, 0u}, 10);
    append(fragment,{0u, 0u});
    setAlignment(0.5);
}

VideoPassLayout::VideoPassLayout() {
    append(settings,{0u, 0u}, 20);
    append(control,{~0u, 0u}, 20);
    append(generated,{~0u, 0u}, 5);
    append(errorMessage, {~0u, ~0u});
    setPadding(8);
    setFont(GUIKIT::Font::system("bold"));
}

VideoParamLayout::Control::Control() {
    append(save, {0u, 0u});
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

VideoScreenTextLayout::ColorBoxLayout::Type::Type() {
    append(label, {0u, 0u}, 10);
    append(normal, {0u, 0u}, 10);
    append(warning, {0u, 0u});
    append(spacer, {~0u, 0u});
    append(reset, {0u, 0u});

    setAlignment(0.5);
    GUIKIT::RadioBox::setGroup( normal, warning );
    normal.setChecked();
}

VideoScreenTextLayout::ColorBoxLayout::Selection::Control::Control() {
    append(canvas[CONTROL_FG], {~0u, 30u}, 5);
    append(hex[CONTROL_FG], {~0u, 0u}, 5);
    append(canvas[CONTROL_BG], {~0u, 30u}, 5);
    append(hex[CONTROL_BG], {~0u, 0u});

    canvas[CONTROL_FG].setBorderColor(1, 0x333333);
    canvas[CONTROL_BG].setBorderColor(1, 0x333333);
}

VideoScreenTextLayout::ColorBoxLayout::Selection::ComponentBox::ComponentBox() {
    append(components[COMPONENT_R], {~0u, 0u}, 3);
    append(components[COMPONENT_G], {~0u, 0u}, 3);
    append(components[COMPONENT_B], {~0u, 0u}, 3);
    append(components[COMPONENT_A], {~0u, 0u});

    components[COMPONENT_R].slider.setLength(256);
    components[COMPONENT_G].slider.setLength(256);
    components[COMPONENT_B].slider.setLength(256);
    components[COMPONENT_A].slider.setLength(256);

    components[COMPONENT_R].updateValueWidth("999");
    components[COMPONENT_G].updateValueWidth("999");
    components[COMPONENT_B].updateValueWidth("999");
    components[COMPONENT_A].updateValueWidth("999");
}

VideoScreenTextLayout::ColorBoxLayout::Selection::Selection() {
    append(control, {60u, 0u}, 10);
    append(componentBox[COM_BOX_FG], {~0u, 0u}, 5);
    append(componentBox[COM_BOX_BG], {~0u, 0u});
}

VideoScreenTextLayout::ColorBoxLayout::ColorBoxLayout() {
    append(type, {~0u, 0u}, 5);
    append(selection, {~0u, 0u});

    setFont(GUIKIT::Font::system("bold"));
    setPadding(10);
}

VideoScreenTextLayout::Options::Font::Font() {
    append(labelFontSize, {0u, 0u}, 10);
    append(fontSize, {0u, 0u}, 10);
    append(labelFontType, {0u, 0u}, 10);
    append(fontType, {0u, 0u}, 10);
    append(removeFont, {0u, 0u}, 10);
    append(addFont, {0u, 0u});

    for(unsigned s = 8; s <= 36; s++) {
        fontSize.append(std::to_string(s), s);
    }

    setAlignment(0.5);
}

VideoScreenTextLayout::Options::Position::Position() {
    append(label, {0u, 0u}, 10);
    append(bottomLeft, {0u, 0u}, 10);
    append(bottomCenter, {0u, 0u}, 10);
    append(bottomRight, {0u, 0u}, 10);
    append(topLeft, {0u, 0u}, 10);
    append(topCenter, {0u, 0u}, 10);
    append(topRight, {0u, 0u});

    GUIKIT::RadioBox::setGroup(bottomLeft, bottomCenter, bottomRight, topLeft, topCenter, topRight);

    setAlignment(0.5);
}

VideoScreenTextLayout::Options::TextPadding::TextPadding() :
paddingVertical("", true) {
    append(paddingHorizontal, {~0u, 0u}, 10);
    append(paddingVertical, {~0u, 0u});

    paddingHorizontal.slider.setLength(61);
    paddingVertical.slider.setLength(31);
    setAlignment(0.5);
}

VideoScreenTextLayout::Options::TextMargin::TextMargin() :
marginHorizontal("%"),
marginVertical("%", true) {
    append(marginHorizontal, {~0u, 0u}, 10);
    append(marginVertical, {~0u, 0u});

    marginHorizontal.slider.setLength(101);
    marginVertical.slider.setLength(101);
    setAlignment(0.5);
}

VideoScreenTextLayout::Options::Options() {
    append(font, {0u, 0u}, 10);
    append(position, {0u, 0u}, 10);
    append(textPadding, {~0u, 0u}, 10);
    append(textMargin, {~0u, 0u});

    setFont(GUIKIT::Font::system("bold"));
    setPadding(10);
}

VideoScreenTextLayout::VideoScreenTextLayout() {
    append(colorBox, {~0u, 0u}, 10);
    append(options, {~0u, 0u});
}

VideoLayout::VideoLayout(TabWindow* tabWindow) :
layBase( dynamic_cast<LIBC64::Interface*>(tabWindow->emulator) ) {
    GUIKIT::TreeViewItem* tvi;
    this->tabWindow = tabWindow;
    this->emulator = tabWindow->emulator;
    imgFolderOpen.loadPng((uint8_t*)Icons::folderOpen, sizeof(Icons::folderOpen) );
    imgFolderClosed.loadPng((uint8_t*)Icons::folderClosed, sizeof(Icons::folderClosed) );
    imgDocument.loadPng((uint8_t*)Icons::document, sizeof(Icons::document) );
    imgError.loadPng((uint8_t*)Icons::error, sizeof(Icons::error) );
    pageUp.loadPng((uint8_t*)Icons::pageUp, sizeof(Icons::pageUp) );
    pageDown.loadPng((uint8_t*)Icons::pageDown, sizeof(Icons::pageDown) );
    pageUpGray.loadPng((uint8_t*)Icons::pageUpGray, sizeof(Icons::pageUpGray) );
    pageDownGray.loadPng((uint8_t*)Icons::pageDownGray, sizeof(Icons::pageDownGray) );
    retroarch.loadPng((uint8_t*)Icons::retroarch, sizeof(Icons::retroarch) );
    colorImage.loadPng((uint8_t*)Icons::color, sizeof(Icons::color));
    menuImage.loadPng((uint8_t*)Icons::menu, sizeof(Icons::menu));
    addImage.loadPng((uint8_t*)Icons::add, sizeof(Icons::add));
    delImage.loadPng((uint8_t*)Icons::del, sizeof(Icons::del));

    layScreenText.options.font.addFont.setImage(&addImage);
    layScreenText.options.font.removeFont.setImage(&delImage);

    layShader.main.control.downloadSlang.setImage( &retroarch );
    layShader.main.control.downloadSlang.setUri( "https://buildbot.libretro.com/assets/frontend/shaders_slang.zip" );

    tviBase.setUserData( (uintptr_t)1 );
    tviBase.setImage( colorImage );

    tviScreenText.setUserData( (uintptr_t)11 );
    tviScreenText.setImage( menuImage );

    tviShader.setUserData( (uintptr_t)2 );
    tviShader.setImage(imgFolderClosed);
    tviShader.setImageExpanded(imgFolderOpen);

    tviParams.setUserData( (uintptr_t)3000 );
    tviParams.setImage(imgFolderClosed);
    tviParams.setImageExpanded(imgFolderOpen);

    moduleTree.append(tviBase);
    moduleTree.append(tviScreenText);
    tviBase.setSelected();
    if (videoDriver->shaderSupport())
        moduleTree.append(tviShader);

    moduleSwitch.setLayout(1, layBase, {~0u, ~0u});
    moduleSwitch.setLayout(11, layScreenText, {~0u, ~0u});
    moduleSwitch.setLayout(2, layShader, {~0u, ~0u});
    moduleSwitch.setLayout(21, layPass, {~0u, ~0u});
    moduleSwitch.setLayout(3, layParam, {~0u, ~0u});

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
        moduleSwitch.synchronizeLayout();
    };

    setMargin(10);

    setSliderAction<unsigned>( &layBase.view.gamma, "gamma", [](unsigned position) { return position + 30; } );
    setSliderAction<unsigned>( &layBase.view.saturation, "saturation" );
    setSliderAction<unsigned>( &layBase.view.brightness, "brightness" );
    setSliderAction<unsigned>( &layBase.view.contrast, "contrast" );
    setSliderAction<int>( &layBase.view.phase, "phase", [](unsigned position) { return (int)position - 180; } );
    setSliderAction<unsigned>( &layBase.view.scanlines, "scanlines", [](unsigned position) { return std::max(position, 1u); } );
    setSliderAction<unsigned>( &layBase.view.interlace, "interlace", [](unsigned position) { return std::max(position, 0u); } );
    setSliderAction<unsigned>( &layBase.encoding.blur, "blur" );
    setSliderAction<float>( &layBase.encoding.phaseError, "phase_error", [](unsigned position) { return (float)((int)position - 90) / 2.0f; } );
    setSliderAction<int>( &layBase.encoding.hanoverBars, "hanover_bars", [](unsigned position) { return (int)position - 100; } );
    setSliderAction<float>( &layBase.lumaDelay.lumaRise, "luma_rise", [](unsigned position) { return ((float)std::max(position, 1u) / 10.0f) + 1.0f; } );
    setSliderAction<float>( &layBase.lumaDelay.lumaFall, "luma_fall", [](unsigned position) { return ((float)std::max(position, 1u) / 10.0f) + 1.0f; } );

    layBase.view.option.newLuma.onToggle = [this](bool checked) {
        _settings->set<bool>( "video_new_luma" + this->sliderIdent(), checked);
        vManager()->updateData<bool>("new_luma", checked);
    };

    layBase.view.option.tvGamma.onToggle = [this](bool checked) {
        _settings->set<bool>( "video_tv_gamma" + this->sliderIdent(), checked);
        vManager()->updateData<bool>("tv_gamma", checked);
    };
	
	layBase.view.option.linearInterpolation.onToggle = [this](bool checked) {
		_settings->set<bool>("video_filter", checked );
        emuThread->lock();
        program->setVideoFilter();
        emuThread->unlock();
	};

    layBase.view.option.cpuFilterThreaded.onToggle = [this](bool checked) {
        emuThread->lock();
        _settings->set<bool>("cpu_filter_threaded", checked);
        vManager()->setCrtThreaded( checked );
        emuThread->unlock();
    };

    layBase.view.mode.reset.onActivate = [this]() {
        vManager()->resetSettings();
        emuThread->lock();
        updatePresets(true, false);
        emuThread->unlock();
    };

    layBase.view.mode.palette.onActivate = [this]() {
        _settings->set<bool>( "video_spectrum", false);
        emuThread->lock();
        updatePresets(true, false);
        emuThread->unlock();
    };

    layBase.view.mode.spectrum.onActivate = [this]() {
        _settings->set<bool>("video_spectrum", true);
        emuThread->lock();
        updatePresets(true, false);
        emuThread->unlock();
    };

    layBase.view.mode.rgb.onActivate = [this]() {
        _settings->set<unsigned>("video_crt", (unsigned)VideoManager::CrtMode::None);
        emuThread->lock();
        program->setWarp( false );
        updatePresets(true, false);
        view->updateShader(emulator);
        emuThread->unlock();
    };

    layBase.view.mode.cpu.onActivate = [this]() {
        _settings->set<unsigned>("video_crt", (unsigned)VideoManager::CrtMode::Cpu);
        emuThread->lock();
        program->setWarp( false );
		updatePresets(true, false);
        view->updateShader(emulator);
        emuThread->unlock();
    };

    layBase.view.mode.gpu.onActivate = [this]() {
        _settings->set<unsigned>("video_crt", (unsigned)VideoManager::CrtMode::Gpu);
        emuThread->lock();
        program->setWarp( false );
		updatePresets(true, false);
        view->updateShader(emulator);
        emuThread->unlock();
    };

    layShader.main.control.load.onActivate = [this]() {
        auto path = openShaderFileDialog();
        if (path.empty())
            return;

        emuThread->lock();
        if (loadShader(path)) {
            layShader.favourite.control.add.setEnabled();
            if (externalFolder())
                _settings->set<std::string>("slang_folder", GUIKIT::File::getPath(path));
        }
        emuThread->unlock();
    };

    layShader.main.control.prependPreset.onActivate = [this]() {
        auto path = openShaderFileDialog();
        if (path.empty())
            return;

        emuThread->lock();
        std::vector<std::string> errors;
        ShaderPreset* preset = vManager()->addPreset(path, true, errors);

        if (preset) {
            buildShaderUI(preset);
            layShader.main.info.loaded.setText( vManager()->getPresetPathDetailed() );
            if (externalFolder())
                _settings->set<std::string>("slang_folder", GUIKIT::File::getPath(path));
            layShader.favourite.control.add.setEnabled();
            layBase.view.gamma.setEnabled( !layBase.view.mode.gpu.checked() || !vManager()->shaderLumaChromaInput() );
        }
        emuThread->unlock();
        showErrors(errors);
    };

    layShader.main.control.appendPreset.onActivate = [this]() {
        auto path = openShaderFileDialog();
        if (path.empty())
            return;

        emuThread->lock();
        std::vector<std::string> errors;
        ShaderPreset* preset = vManager()->addPreset(path, false, errors);

        if (preset) {
            buildShaderUI(preset);
            layShader.main.info.loaded.setText( vManager()->getPresetPathDetailed() );
            if (externalFolder())
                _settings->set<std::string>("slang_folder", GUIKIT::File::getPath(path));
            layShader.favourite.control.add.setEnabled();
            layBase.view.gamma.setEnabled( !layBase.view.mode.gpu.checked() || !vManager()->shaderLumaChromaInput() );
        }
        emuThread->unlock();
        showErrors(errors);
    };

    layShader.main.control.unload.onActivate = [this]() {
        emuThread->lock();
        unloadShader();
        emuThread->unlock();
        view->updateShader(emulator);
    };

    layPass.control.save.onActivate = [this]() {
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
            layShader.main.info.loaded.setText( vManager()->getPresetPathDetailed() );
            _settings->set<std::string>("slang_folder_save", GUIKIT::File::getPath(path));
        }
    };

    layParam.control.save.onActivate = [this]() {
        layPass.control.save.onActivate();
    };

    layShader.main.control.internal.onActivate = [this]() {
        _settings->set<bool>("shader_internal", true);
    };

    layShader.main.control.external.onActivate = [this]() {
        _settings->set<bool>("shader_internal", false);
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
        view->buildShader();
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

        layShader.favourite.control.remove.setEnabled(false);
        view->buildShader();
    };

    layShader.favourite.list.onActivate = [this]() {
        int selection = layShader.favourite.list.selection();
        std::string path = layShader.favourite.list.text(selection, 0);
        emuThread->lock();
        if (loadShader(path))
            view->updateShader(emulator);
        emuThread->unlock();
    };

    layShader.favourite.list.onChange = [this]() {
        if (vManager()->getPreset())
            layShader.favourite.control.add.setEnabled();
        layShader.favourite.control.remove.setEnabled();
    };

    codeWindow.setGeometry({ 100, 100, 600, 350 });
    codeLayout.append(codeViewer, {~0u, ~0u} );
    codeLayout.setMargin(10);
    codeWindow.append(codeLayout);

    codeWindow.onClose = [this]() {
        codeWindow.setVisible(false);
        this->tabWindow->setFocused(100);
    };

    layPass.generated.vertex.onActivate = [this]() {
        std::string code;
        ShaderPreset::Pass pass;
        if (vManager()->fetchShader(pass, selectedPassId)) {
            if (videoDriver->getShaderNativeVertexCode(pass.vertex, code))
                codeViewer.setText( code );
            else
                codeViewer.setText( code + "\n" + pass.vertex );

            codeWindow.setTitle( "Vertex" );
            codeWindow.setVisible();
            codeWindow.setFocused();
        }
    };

    layPass.generated.fragment.onActivate = [this]() {
        std::string code;
        ShaderPreset::Pass pass;
        if (vManager()->fetchShader(pass, selectedPassId)) {
            if (videoDriver->getShaderNativeFragmentCode(pass.fragment, code))
                codeViewer.setText( code );
            else
                codeViewer.setText( code + "\n" + pass.fragment );

            codeWindow.setTitle("Fragment");
            codeWindow.setVisible();
            codeWindow.setFocused();
        }
    };

    layPass.control.disable.onActivate = [this]() {
        emuThread->lock();
        auto pass = vManager()->togglePassUsage(selectedPassId);
        emuThread->unlock();
        if(!pass)
            return;

        if (pass->inUse) {
            layPass.control.disable.setText( trans->getA("disable") );
            layPass.settings.setEnabled(true);
        } else {
            layPass.control.disable.setText( trans->getA("enable") );
            layPass.settings.setEnabled(false);
        }
        clearShaderError();
        layPass.control.synchronizeLayout();
    };

    layPass.control.down.onClick = [this]() {
        if (!layPass.control.down.enabled())
            return;

        unsigned passIdBefore = selectedPassId;
        emuThread->lock();
        vManager()->movePass( selectedPassId, false);
        emuThread->unlock();

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
        }
        clearShaderError();
        tviPasses[selectedPassId]->setSelected();
    };

    layPass.control.up.onClick = [this]() {
        if (!layPass.control.up.enabled())
            return;

        unsigned passIdBefore = selectedPassId;
        emuThread->lock();
        vManager()->movePass( selectedPassId, true);
        emuThread->unlock();

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
        }
        clearShaderError();
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
                unsigned offset = tviParam.offsets[i];

                if (offset < preset->params.size()) {
                    ShaderPreset::Param& param = preset->params[offset];
                    val = (float) position * param.step + param.minimum;
                    vManager()->updateData(offset, val);
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
                unsigned offset = tviParam.offsets[i];

                if (offset < preset->params.size()) {
                    ShaderPreset::Param& param = preset->params[offset];
                    val = param.initialOverridden;
                    vManager()->updateData(offset, val);
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

        tviParams.setExpanded();
        if (!param.tvi)
            tviParams.setSelected();
        else
            param.tvi->setSelected();

        buildParams(param);
        moduleSwitch.setSelection( 3 );
    };

    layShader.main.info.clearCache.onActivate = [this]() {
        std::string cacheFolder = GUIKIT::System::getUserDataFolder(program->appFolder());
        if (cacheFolder == "./")
            return;

        GUIKIT::File::removeDirectory( cacheFolder + "cache" );
    };

    layShader.main.info.shaderCache.onToggle = [this](bool checked) {
        emuThread->lock();
        _settings->set<bool>("shader_cache", checked);
        if (emulator == activeEmulator)
            videoDriver->useShaderCache(checked);
        emuThread->unlock();
    };

    layPass.settings.filter.nearest.onActivate = [this]() {
        emuThread->lock();
        vManager()->setPassFilter(selectedPassId, ShaderPreset::FILTER_NEAREST);
        emuThread->unlock();
        clearShaderError();
    };

    layPass.settings.filter.linear.onActivate = [this]() {
        emuThread->lock();
        vManager()->setPassFilter(selectedPassId, ShaderPreset::FILTER_LINEAR);
        emuThread->unlock();
        clearShaderError();
    };

    layPass.settings.filter.unspec.onActivate = [this]() {
        emuThread->lock();
        vManager()->setPassFilter(selectedPassId, ShaderPreset::FILTER_UNSPEC);
        emuThread->unlock();
        clearShaderError();
    };

    layPass.settings.mipmap.checkBox.onToggle = [this](bool checked) {
        emuThread->lock();
        vManager()->setPassMipmap(selectedPassId, checked);
        emuThread->unlock();
        clearShaderError();
    };

    for(int i = 0; i < SCALE_BOXES; i++) {
        auto& radioX = layPass.settings.scaleX.radios[i];
        auto& radioY = layPass.settings.scaleY.radios[i];

        radioX.onActivate = [this, i]() {
            emuThread->lock();
            vManager()->setPassScaleX(selectedPassId, float(i+1));
            emuThread->unlock();
            clearShaderError();
        };

        radioY.onActivate = [this, i]() {
            emuThread->lock();
            vManager()->setPassScaleY(selectedPassId, float(i+1));
            emuThread->unlock();
            clearShaderError();
        };
    }

    layScreenText.colorBox.type.normal.onActivate = [this]() {
        prepareColBox();
        updateScreenText(true);
    };

    layScreenText.colorBox.type.warning.onActivate = [this]() {
        prepareColBox();
        updateScreenText(true);
    };

    layScreenText.colorBox.type.reset.onActivate = [this]() {
        _settings->remove("screen_text_color");
        _settings->remove("screen_text_bgcolor");
        _settings->remove("screen_warn_color");
        _settings->remove("screen_warn_bgcolor");

        prepareColBox();
        updateScreenText(true);
    };

    for(int compBox = 0; compBox < 2; compBox++) {
        for(int component = 0; component < 4; component++) {
            layScreenText.colorBox.selection.componentBox[compBox].components[component].slider.onChange = [this, compBox, component](unsigned position) {
                unsigned shifter;
                switch(component & 3) {
                    case 0: shifter = 16; break;
                    case 1: shifter = 8; break;
                    case 2: shifter = 0; break;
                    case 3: shifter = 24; break;
                }
                std::string& ident = layScreenText.colorBox.selection.componentBox[compBox].ident;
                unsigned& defaultCol = layScreenText.colorBox.selection.componentBox[compBox].defaultCol;
                unsigned col = _settings->get<unsigned>(ident, defaultCol);
                col &= ~(0xff << shifter);
                col |= position << shifter;
                _settings->set<unsigned>(ident, col);
                layScreenText.colorBox.selection.componentBox[compBox].components[component].value.setText( std::to_string(position) );
                layScreenText.colorBox.selection.control.canvas[compBox].setBackgroundColor(col);
                layScreenText.colorBox.selection.control.hex[compBox].setText( GUIKIT::String::convertIntToHex(col & 0xffffff, true) );
                updateScreenText(true);
            };
        }
    }

    layScreenText.options.textPadding.paddingHorizontal.slider.onChange = [this](unsigned position) {
        _settings->set<unsigned>("screen_text_padding_horizontal", position);
        layScreenText.options.textPadding.paddingHorizontal.value.setText( std::to_string(position) );
        updateScreenText(true);
    };

    layScreenText.options.textPadding.paddingVertical.slider.onChange = [this](unsigned position) {
        _settings->set<unsigned>("screen_text_padding_vertical", position);
        layScreenText.options.textPadding.paddingVertical.value.setText( std::to_string(position) );
        updateScreenText(true);
    };

    layScreenText.options.textPadding.paddingVertical.active.onToggle = [this](bool checked) {
        _settings->set<bool>("screen_text_padding_separate", checked);
        layScreenText.options.textPadding.paddingVertical.slider.setEnabled(checked);
        layScreenText.options.textPadding.paddingVertical.value.setEnabled(checked);
        updateScreenText(true);
    };

    layScreenText.options.textMargin.marginHorizontal.slider.onChange = [this](unsigned position) {
        _settings->set<unsigned>("screen_text_margin_horizontal", position);
        layScreenText.options.textMargin.marginHorizontal.setValue( GUIKIT::String::formatFloatingPoint((float)position / 5.0f, 2, true) );
        updateScreenText(true);
    };

    layScreenText.options.textMargin.marginVertical.slider.onChange = [this](unsigned position) {
        _settings->set<unsigned>("screen_text_margin_vertical", position);
        layScreenText.options.textMargin.marginVertical.setValue( GUIKIT::String::formatFloatingPoint((float)position / 5.0f, 2, true) );
        updateScreenText(true);
    };

    layScreenText.options.textMargin.marginVertical.active.onToggle = [this](bool checked) {
        _settings->set<bool>("screen_text_margin_separate", checked);
        layScreenText.options.textMargin.marginVertical.slider.setEnabled(checked);
        layScreenText.options.textMargin.marginVertical.value.setEnabled(checked);
        updateScreenText(true);
    };

    layScreenText.options.font.fontSize.onChange = [this]() {
        unsigned fontSize = layScreenText.options.font.fontSize.userData();
        _settings->set<unsigned>("screen_text_fontsize", fontSize);
        updateScreenText(true);
    };

    layScreenText.options.font.fontType.onChange = [this]() {
        int userData = layScreenText.options.font.fontType.userData();

        if (!userData) {
            _settings->set<std::string>("screen_text_font", "");
        } else {
            auto displayFont = getTTF(userData);
            if (displayFont)
                _settings->set<std::string>("screen_text_font", displayFont->file);
        }

        updateScreenText(false);
        updateFontVisibilities();
    };

    layScreenText.options.font.addFont.onActivate = [this]() {
        std::string filePath = GUIKIT::BrowserWindow()
            .setWindow(*this->tabWindow)
            .setTitle(trans->getA("select font"))
            .setFilters( { trans->getA("font") + " (*.ttf,*.otf,*.ttc)"} )
            .setPath( globalSettings->get<std::string>("font_path", "") )
            .allowSystemFiles()
            .open();

        if (filePath.empty())
            return;

        globalSettings->set<std::string>("font_path", GUIKIT::File::getPath(filePath), true);

        auto _fn = GUIKIT::String::getFileNameA(filePath);

        if (_fn.empty() || (!GUIKIT::String::findString(_fn, ".ttf")
                && !GUIKIT::String::findString(_fn, ".otf")
                && !GUIKIT::String::findString(_fn, ".ttc")))
            return;

        std::string _path = program->getCustomFontsFolder(true);

        if (GUIKIT::File::xcopy(filePath, _path + _fn)) {
            for (auto view : emuConfigViews) {
                if (view->videoLayout)
                    view->videoLayout->fillFontTypeList();
            }
        }
    };

    layScreenText.options.font.removeFont.onActivate = [this]() {
        int userData = layScreenText.options.font.fontType.userData();
        if (!userData)
            return;

        auto displayFont = getTTF(userData);
        if (!displayFont)
            return;

        auto _fn = displayFont->file;
        if (_fn.empty())
            return;

        if (!removeTTF(userData))
            return;

        std::string _path = program->getCustomFontsFolder();

        if (_path.empty())
            return;

        GUIKIT::File file(_path + _fn);
        if (file.exists()) {
            if (file.del()) {
                for (auto view : emuConfigViews) {
                    if (view->videoLayout) {
                        view->videoLayout->fillFontTypeList();
                        view->videoLayout->updateFontVisibilities();
                    }
                }

                if (emulator != activeEmulator)
                    program->updateOnScreenText(false);
                else
                    updateScreenText(false);
            }
        }
    };

    layScreenText.options.position.bottomRight.onActivate = [this]() {
        _settings->set<unsigned>("screen_text_position", DRIVER::ScreenTextDescription::POSITION_BOTTOM_RIGHT);
        updateScreenText(true);
    };

    layScreenText.options.position.bottomCenter.onActivate = [this]() {
        _settings->set<unsigned>("screen_text_position", DRIVER::ScreenTextDescription::POSITION_BOTTOM_CENTER);
        updateScreenText(true);
    };

    layScreenText.options.position.bottomLeft.onActivate = [this]() {
        _settings->set<unsigned>("screen_text_position", DRIVER::ScreenTextDescription::POSITION_BOTTOM_LEFT);
        updateScreenText(true);
    };

    layScreenText.options.position.topRight.onActivate = [this]() {
        _settings->set<unsigned>("screen_text_position", DRIVER::ScreenTextDescription::POSITION_TOP_RIGHT);
        updateScreenText(true);
    };

    layScreenText.options.position.topCenter.onActivate = [this]() {
        _settings->set<unsigned>("screen_text_position", DRIVER::ScreenTextDescription::POSITION_TOP_CENTER);
        updateScreenText(true);
    };

    layScreenText.options.position.topLeft.onActivate = [this]() {
        _settings->set<unsigned>("screen_text_position", DRIVER::ScreenTextDescription::POSITION_TOP_LEFT);
        updateScreenText(true);
    };

    fillFontTypeList();

    loadSettings(true);
}

auto VideoLayout::fillFontTypeList() -> void {
    std::vector<std::string> list;
    auto& fontTypes = layScreenText.options.font.fontType;

    int selUserId = 0;
    if (fontTypes.rows())
        selUserId = fontTypes.userData();

    fontTypes.reset();
    fontTypes.append( trans->getA("default"), 0 );

    list = GUIKIT::File::getFolderListAlt(program->fontFolder(), {".ttf", ".otf", ".ttc"}, false);
    for(auto& file : list)
        addTTF(1, file);

    list = GUIKIT::File::getFolderListAlt(program->getCustomFontsFolder(), {".ttf", ".otf", ".ttc"}, false);
    for(auto& file : list)
        addTTF(2, file);

    std::sort(displayFonts.begin(), displayFonts.end(), [](DisplayFont& a, DisplayFont& b) -> bool {
        std::string _sA = a.name;
        std::string _sB = b.name;
        GUIKIT::String::toLowerCase(_sA);
        GUIKIT::String::toLowerCase(_sB);
        return _sA < _sB;
    });

    for(auto& displayFont : displayFonts)
        fontTypes.append( displayFont.name, displayFont.ident );

    fontTypes.setSelectionByUserId(selUserId);
}

auto VideoLayout::addTTF(unsigned mode, const std::string& _fontFile) -> void {
    static int counter = 0;
    uint16_t ident = mode << 14;

    for(auto& displayFont : displayFonts) {
        if ((displayFont.ident & 0xc000) == ident && displayFont.file == _fontFile)
            return;
    }
    std::string out = _fontFile;
    std::string screenTextFontPath = "";

    if (mode == 1) {
        screenTextFontPath = program->fontFolder() + _fontFile;
    } else if (mode == 2) {
        screenTextFontPath = program->getCustomFontsFolder() + _fontFile;
    }

    if (!screenTextFontPath.empty()) {
        GUIKIT::File file(screenTextFontPath);
        if (file.open()) {
            uint8_t* _data = file.read();
            if (_data) {
                GUIKIT::TTF ttf(_data, file.getSize());
                out = ttf.getFontName();
                if (out.empty())
                    out = _fontFile;
            }
        }
        file.unload();
    }

    ident += counter++;
    displayFonts.push_back({_fontFile, out, ident});
}

auto VideoLayout::getTTF(uint16_t ident) -> DisplayFont* {
    for(auto& displayFont : displayFonts) {
        if (displayFont.ident == ident)
            return &displayFont;
    }
    return nullptr;
}

auto VideoLayout::getTTF(std::string _file) -> DisplayFont* {
    for(auto& displayFont : displayFonts) {
        if (displayFont.file == _file)
            return &displayFont;
    }
    return nullptr;
}

auto VideoLayout::removeTTF(uint16_t ident) -> bool {
    for(int i = 0; i < displayFonts.size(); i++) {
        DisplayFont& displayFont = displayFonts[i];
        if (displayFont.ident == ident) {
            return GUIKIT::Vector::eraseVectorPos(displayFonts, i);
        }
    }
    return false;
}

auto VideoLayout::updateFontVisibilities() -> void {
    int userId = layScreenText.options.font.fontType.userData();
    if (!userId)
        _settings->set<std::string>("screen_text_font", "");
    layScreenText.options.font.removeFont.setEnabled( (userId >> 14) == 2 );
}

auto VideoLayout::updateScreenText(bool keepFontPath) -> void {
    if (emulator != activeEmulator)
        return;

    program->updateOnScreenText(keepFontPath);
    if (statusHandler)
        statusHandler->setMessage( trans->getA("changes applied"), 4, layScreenText.colorBox.type.warning.checked() );
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

auto VideoLayout::buildShaderUI(ShaderPreset* preset, bool selectIt) -> void {

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
    layPass.errorMessage.setText("");

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

        if (pass.inUse && !pass.error.empty())
            tviPass->setImage(imgError);
        else
            tviPass->setImage( imgDocument );
        tviShader.append(*tviPass);

        tviPasses.push_back(tviPass);
    }

    GUIKIT::TreeViewItem* tviParam = nullptr;
    unsigned pageElement = 0;
    std::vector<unsigned> offsets;
    bool isDescriptor;
    unsigned countDescriptors = 0;

    for(unsigned i = 0; i < preset->params.size(); i++) {
        auto& param = preset->params[i];

        if (GUIKIT::String::findString(param.id, "autoEmu_"))
            continue;

        offsets.push_back(i);
        isDescriptor = param.isDescriptor();
        if (isDescriptor)
            countDescriptors++;

        int adjust = -3 + (countDescriptors >> 1);
        if (adjust > 0)
            adjust = 0;

        if (++pageElement == (PARAMS_PER_PAGE + adjust) ) {
            if (isDescriptor) {
                std::vector<unsigned> offsetsTemp;
                for(int o = offsets.size() - 1; o >= 0; o--) {
                    unsigned offset = offsets[o];
                    auto& p = preset->params[offset];
                    if (!p.isDescriptor()) {
                        break;
                    }

                    GUIKIT::Vector::insert(offsetsTemp, offset, 0);
                    offsets.pop_back();
                }

                params.push_back( {tviParam, offsets} );
                offsets.clear();
                offsets = offsetsTemp;
                pageElement = countDescriptors = offsetsTemp.size();

            } else {
                params.push_back( {tviParam, offsets} );
                pageElement = 0;
                offsets.clear();
                countDescriptors = 0;
            }

            tviParam = new GUIKIT::TreeViewItem;
            tviParam->setUserData( (uintptr_t)(3000 + params.size() ) );
            tviParam->setImage( imgDocument );
        }
    }

    if (pageElement)
        params.push_back( {tviParam, offsets} );
    else if (tviParam)
        delete tviParam;

    for(auto& param : params) {
        if (param.tvi) {
            param.tvi->setText( preset->params[param.offsets[0]].desc );
            tviParams.append(*param.tvi);
        }
    }

    if (params.size()) {
        moduleTree.append(tviParams);
        layShader.main.info.toParams.setEnabled();
    }

    tviShader.setExpanded();
    if (selectIt && !tviBase.selected() && !tviScreenText.selected()) {
        tviShader.setSelected();
        moduleSwitch.setSelection( 2 );
    }
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

    for(int i = 0; i < tviParam.offsets.size(); i++) {
        auto sliderLay = paramSliders[i];
        int offset = tviParam.offsets[i];
        if (offset >= preset->params.size())
            break;

        auto& shaderParam = preset->params[offset];

        if (shaderParam.isDescriptor())
            sliderLay->name.setFont(GUIKIT::Font::system("bold"));
        else
            sliderLay->name.setFont(GUIKIT::Font::system());

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
    layPass.settings.file.value.setText( GUIKIT::String::getFileName( pass.src ) );
    layPass.control.disable.setText( trans->getA(pass.inUse ? "disable" : "enable") );

    switch(pass.filter) {
        default:
        case ShaderPreset::FILTER_UNSPEC: layPass.settings.filter.unspec.setChecked(); break;
        case ShaderPreset::FILTER_LINEAR: layPass.settings.filter.linear.setChecked(); break;
        case ShaderPreset::FILTER_NEAREST: layPass.settings.filter.nearest.setChecked(); break;
    }

    switch(pass.wrap) {
        default:
        case ShaderPreset::WRAP_EDGE: layPass.settings.wrap.value.setText("edge"); break;
        case ShaderPreset::WRAP_BORDER: layPass.settings.wrap.value.setText("border"); break;
        case ShaderPreset::WRAP_REPEAT: layPass.settings.wrap.value.setText("repeat"); break;
        case ShaderPreset::WRAP_MIRRORED_REPEAT: layPass.settings.wrap.value.setText("mirror"); break;
    }

    layPass.settings.bufferType.value.setText( vManager()->translateShaderBufferType(pass.bufferType) );
    layPass.settings.mipmap.checkBox.setChecked(pass.mipmap);
    layPass.settings.modulo.value.setText( std::to_string( pass.frameModulo ));

    std::string scaleX = "";
    std::string scaleY = "";

    GUIKIT::RadioBox* useRadioX = nullptr;
    if (pass.scaleTypeX != ShaderPreset::SCALE_ABSOLUTE) {
        for (int i = 0; i < SCALE_BOXES; i++) {
            auto& radio = layPass.settings.scaleX.radios[i];
            if (pass.scaleX == float(i+1)) {
                useRadioX = &radio;
                break;
            }
        }
    }

    if (useRadioX) {
        useRadioX->setChecked();
        scaleX = pass.scaleTypeX == ShaderPreset::SCALE_INPUT ? "Input" : "Viewport";
    } else {
        switch(pass.scaleTypeX) {
            default:
            case ShaderPreset::SCALE_INPUT: scaleX = "Input: " + GUIKIT::String::formatFloatingPoint(pass.scaleX, 2); break;
            case ShaderPreset::SCALE_VIEWPORT: scaleX = "Viewport: " + GUIKIT::String::formatFloatingPoint(pass.scaleX, 2); break;
            case ShaderPreset::SCALE_ABSOLUTE: scaleX = "Absolute: " + std::to_string( pass.absX ); break;
        }
    }

    GUIKIT::RadioBox* useRadioY = nullptr;
    if (pass.scaleTypeY != ShaderPreset::SCALE_ABSOLUTE) {
        for (int i = 0; i < SCALE_BOXES; i++) {
            auto& radio = layPass.settings.scaleY.radios[i];
            if (pass.scaleY == float(i+1)) {
                useRadioY = &radio;
                break;
            }
        }
    }

    if (useRadioY) {
        useRadioY->setChecked();
        scaleY = pass.scaleTypeY == ShaderPreset::SCALE_INPUT ? "Input" : "Viewport";
    } else {
        switch(pass.scaleTypeY) {
            default:
            case ShaderPreset::SCALE_INPUT: scaleY = "Input: " + GUIKIT::String::formatFloatingPoint(pass.scaleY, 2); break;
            case ShaderPreset::SCALE_VIEWPORT: scaleY = "Viewport: " + GUIKIT::String::formatFloatingPoint(pass.scaleY, 2); break;
            case ShaderPreset::SCALE_ABSOLUTE: scaleY = "Absolute: " + std::to_string( pass.absY ); break;
        }
    }

    layPass.settings.scaleX.value.setText( scaleX );
    layPass.settings.scaleY.value.setText( scaleY );

    if (!pass.error.empty()) {
        std::string _error = pass.error;
        layPass.generated.errorLabel.setForegroundColor(ERROR_COLOR);
        layPass.errorMessage.setText(_error);
    } else {
        layPass.errorMessage.setText("");
        layPass.generated.errorLabel.resetForegroundColor();
    }

    layPass.settings.setEnabled(pass.inUse);

    if (pass.inUse) {
        layPass.settings.scaleX.setEnabled(useRadioX != nullptr);
        layPass.settings.scaleY.setEnabled(useRadioY != nullptr);
        layPass.settings.scaleX.ident.setEnabled();
        layPass.settings.scaleY.ident.setEnabled();
        layPass.settings.scaleX.value.setEnabled();
        layPass.settings.scaleY.value.setEnabled();
    }

    updateMoveImg();

    GUIKIT::HorizontalLayout::alignChildrenVertically({&layPass.settings.file,&layPass.settings.filter,&layPass.settings.wrap,
                                                       &layPass.settings.bufferType,&layPass.settings.mipmap,&layPass.settings.modulo,
                                                       &layPass.settings.scaleX,&layPass.settings.scaleY}, 0, 20);

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

            unsigned position = layout->slider.position();
			T value = callTransfer( position );

            vManager()->updateData(baseIdent, checked ? value : T(0));

            if (baseIdent == "interlace") {
                vManager()->updateData<bool>("interlace_fields", checked);
            }
        };

    layout->slider.onChange = [this, layout, baseIdent, callTransfer](unsigned position) {
		T value = callTransfer( position );	
        auto unit = layout->unit;
		
        _settings->set<T>("video_" + baseIdent + this->sliderIdent(), value);
		
		if (std::is_same<T, float>::value)
			layout->value.setText( GUIKIT::String::formatFloatingPoint(value, 1) + " " + unit);
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

auto VideoLayout::updatePresets(bool reloadDriver, bool reloadPreset) -> void {
    
    auto [VPARAMS] = VideoManager::getInstance( emulator )->getSettings( );
    
    if (videoDriver && reloadDriver)
        VideoManager::getInstance( emulator )->reloadSettings(reloadPreset);

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

    std::vector<std::string> errors;
    ShaderPreset* preset = vManager()->getPreset(errors);
    if (preset) {
        buildShaderUI(preset, reloadPreset);
        layShader.main.info.loaded.setText( vManager()->getPresetPathDetailed() );
        layShader.main.control.setEnabled();
        layShader.favourite.control.add.setEnabled();
        showErrors(errors);
    } else
        unloadShader();
	
	updateVisibillity();
}

auto VideoLayout::updateVisibillity() -> void {
	bool _pal = emulator->getRegionEncoding() == Emulator::Interface::Region::Pal;
    bool isC64 = dynamic_cast<LIBC64::Interface*>(emulator);
    bool crtCpuChecked = layBase.view.mode.cpu.checked();
    bool crtGpuChecked = layBase.view.mode.gpu.checked();

    if (!videoDriver->shaderSupport()) {
        if(crtGpuChecked) {
            layBase.view.mode.rgb.setChecked();
            crtGpuChecked = false;
        }
        layBase.view.mode.gpu.setEnabled(false);
    } else
        layBase.view.mode.gpu.setEnabled();
	
	if (layBase.view.mode.spectrum.checked()) {
        layBase.view.phase.setEnabled();
        layBase.view.option.newLuma.setEnabled();
    } else {
        layBase.view.phase.setEnabled(false);
        layBase.view.option.newLuma.setEnabled(false);
    }

    layBase.view.gamma.setEnabled( !crtGpuChecked || !vManager()->shaderLumaChromaInput() );

    layBase.view.scanlines.setEnabled(crtCpuChecked);
    if (crtCpuChecked)
        layBase.view.scanlines.slider.setEnabled( layBase.view.scanlines.active.checked() );

    layBase.view.interlace.slider.setEnabled( layBase.view.interlace.active.checked() );

    layBase.encoding.setEnabled( crtCpuChecked );
    if (crtCpuChecked) {
        layBase.encoding.phaseError.slider.setEnabled( layBase.encoding.phaseError.active.checked() );
        layBase.encoding.hanoverBars.setEnabled( _pal );
        layBase.encoding.hanoverBars.slider.setEnabled( _pal && layBase.encoding.hanoverBars.active.checked() );
        layBase.encoding.blur.slider.setEnabled(  layBase.encoding.blur.active.checked() );
    }

    if (isC64) {
        layBase.lumaDelay.setEnabled(crtCpuChecked);
        if (crtCpuChecked) {
            layBase.lumaDelay.lumaRise.slider.setEnabled(layBase.lumaDelay.lumaRise.active.checked());
            layBase.lumaDelay.lumaFall.slider.setEnabled(layBase.lumaDelay.lumaFall.active.checked());
        }
    }
    
    layBase.view.option.tvGamma.setEnabled( (crtCpuChecked || crtGpuChecked) && layBase.view.mode.palette.checked() && _pal );

    layBase.view.option.cpuFilterThreaded.setEnabled( crtCpuChecked );
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
    layBase.view.option.cpuFilterThreaded.setText( trans->get("own thread") );
    layBase.view.mode.palette.setText( trans->get("palette") );
    layBase.view.mode.spectrum.setText( trans->get("color_spectrum") );
    layBase.view.mode.reset.setText( trans->get("reset") );
    layBase.view.mode.rgb.setText( trans->get("RGB") );
    layBase.view.mode.cpu.setText( trans->get("S/C-Video CPU") );
    layBase.view.mode.cpu.setTooltip( trans->get("S/C-Video tooltip") );
    layBase.view.mode.gpu.setText( trans->get("Shader GPU") );
    layBase.view.scanlines.active.setText( trans->get("scanlines", {}, true) );
    layBase.view.interlace.active.setText( trans->get("interlace", {}, true) );

    layBase.encoding.setText(trans->get("color encoding"));
    layBase.encoding.phaseError.active.setText( trans->get("phase_error", {}, true) );
    layBase.encoding.hanoverBars.active.setText( trans->get("hanover_bars", {}, true) );
    layBase.encoding.blur.active.setText( trans->get("blur", {}, true) );
    layBase.lumaDelay.setText(trans->get("luma delay"));
    layBase.lumaDelay.lumaRise.active.setText( trans->get("luma_rise", {}, true) );
    layBase.lumaDelay.lumaFall.active.setText( trans->get("luma_fall", {}, true) );

    layShader.main.control.prependPreset.setText( trans->getA("prepend preset") );
    layShader.main.control.appendPreset.setText( trans->getA("append preset") );

    layShader.main.control.folder.setText( trans->getA("folder", true) );
    layShader.main.control.internal.setText( trans->getA("internal") );
    layShader.main.control.external.setText( trans->getA("external") );
    layShader.main.control.external.setTooltip( trans->getA("shader hints") );
    layShader.main.control.unload.setText( trans->getA("unload") );
    layPass.control.save.setText( trans->getA("save") );
    layShader.main.control.load.setText( trans->getA("load") );
    layPass.control.save.setTooltip( trans->getA("save parameter tooltip") );

    layShader.main.setText( trans->getA("Shader") );
    layShader.favourite.setText( trans->getA("favourites") );
    layShader.favourite.list.setHeaderText({trans->getA("selection")});

    layShader.main.info.label.setText( trans->getA("loaded", true) );
    layShader.main.info.clearCache.setText( trans->getA("clear cache") );
    layShader.main.info.shaderCache.setText( trans->get("shader cache") );
    layShader.main.info.toParams.setText( trans->getA("Parameter") );
    layShader.favourite.control.add.setText( trans->getA("add") );
    layShader.favourite.control.remove.setText( trans->getA("remove") );

    layPass.settings.file.ident.setText( trans->getA("file", true) );
    layPass.settings.filter.ident.setText( trans->getA("filter", true) );
    layPass.settings.wrap.ident.setText( trans->getA("Wrap", true) );
    layPass.settings.bufferType.ident.setText( trans->getA("buffer format", true) );
    layPass.settings.mipmap.ident.setText( trans->getA("Mipmap", true) );
    layPass.settings.mipmap.checkBox.setText( trans->getA("enabled") );
    layPass.settings.modulo.ident.setText( trans->getA("Modulo", true) );
    layPass.settings.scaleX.ident.setText( trans->getA("Scaling X", true) );
    layPass.settings.scaleY.ident.setText( trans->getA("Scaling Y", true) );

    layParam.control.save.setText( trans->getA("save parameter") );
    layParam.control.save.setTooltip( trans->getA("save parameter tooltip") );
    layParam.control.previous.setText( trans->getA("previous") );
    layParam.control.next.setText( trans->getA("next") );

    layScreenText.colorBox.setText( trans->getA("color selection") );
    layScreenText.colorBox.type.label.setText( trans->getA("selection", true) );
    layScreenText.colorBox.type.normal.setText( trans->getA("text color") );
    layScreenText.colorBox.type.warning.setText( trans->getA("warn color") );
    layScreenText.colorBox.type.reset.setText( trans->getA("reset") );

    layScreenText.colorBox.selection.componentBox[COM_BOX_FG].components[COMPONENT_R].name.setText(trans->getA("red"));
    layScreenText.colorBox.selection.componentBox[COM_BOX_FG].components[COMPONENT_G].name.setText(trans->getA("green"));
    layScreenText.colorBox.selection.componentBox[COM_BOX_FG].components[COMPONENT_B].name.setText(trans->getA("blue"));
    layScreenText.colorBox.selection.componentBox[COM_BOX_FG].components[COMPONENT_A].name.setText(trans->getA("alpha"));

    layScreenText.options.setText(trans->getA("options"));
    layScreenText.options.font.fontType.setText(0, trans->getA("default"));
    layScreenText.options.font.labelFontSize.setText( trans->getA("Font Size", true) );
    layScreenText.options.font.labelFontType.setText( trans->getA("font", true) );
    layScreenText.options.font.addFont.setText( trans->getA("add") );
    layScreenText.options.font.removeFont.setText( trans->getA("remove") );
    layScreenText.options.position.label.setText( trans->getA("position", true) );
    layScreenText.options.position.bottomLeft.setText( trans->getA("bottom left") );
    layScreenText.options.position.bottomCenter.setText( trans->getA("bottom center") );
    layScreenText.options.position.bottomRight.setText( trans->getA("bottom right") );
    layScreenText.options.position.topLeft.setText( trans->getA("top left") );
    layScreenText.options.position.topCenter.setText( trans->getA("top center") );
    layScreenText.options.position.topRight.setText( trans->getA("top right") );
    layScreenText.options.textPadding.paddingHorizontal.name.setText( trans->getA("Padding", true) );
    layScreenText.options.textMargin.marginHorizontal.name.setText( trans->getA("Margin", true) );

    tviBase.setText( trans->getA("overview") );
    tviScreenText.setText( trans->getA("screen text") );
    tviShader.setText( trans->getA("Shader") );
    tviParams.setText( trans->getA("Parameter") );

    layNav.setText( trans->getA("selection") );
    layPass.setText( trans->getA("Pass") );
    layParam.setText( trans->getA("Parameter") );

    layPass.settings.filter.nearest.setText( trans->getA("nearest") );
    layPass.settings.filter.linear.setText( trans->getA("linear") );
    layPass.settings.filter.unspec.setText( trans->getA("unspecified") );

    layPass.generated.errorLabel.setText( trans->getA("error output", true) );
    layPass.generated.vertex.setText( trans->getA("native Vertex code") );
    layPass.generated.fragment.setText( trans->getA("native Fragment code") );

    layShader.main.control.downloadSlang.setTooltip( trans->getA("shader download") );

    for(int i = 0; i < PARAMS_PER_PAGE; i++) {
        paramSliders[i]->defaultButton.setText( trans->getA("default") );
    }

    SliderLayout::scale({&layBase.view.saturation, &layBase.view.gamma, &layBase.view.brightness, &layBase.view.contrast, &layBase.view.phase, &layBase.view.scanlines, &layBase.view.interlace, &layBase.encoding.phaseError, &layBase.encoding.hanoverBars, &layBase.encoding.blur, &layBase.lumaDelay.lumaRise, &layBase.lumaDelay.lumaFall},
                        "-100 %");

    for(int compBox = 0; compBox < 2; compBox++) {
        std::vector<SliderLayout*> sliders;
        for(int component = 0; component < 4; component++) {
            sliders.push_back(&layScreenText.colorBox.selection.componentBox[compBox].components[component]);
        }
        SliderLayout::scale(sliders, "999");
    }

    SliderLayout::scale({&layScreenText.options.textPadding.paddingHorizontal, &layScreenText.options.textMargin.marginHorizontal}, "99.9 %");
    SliderLayout::scale({&layScreenText.options.textPadding.paddingVertical, &layScreenText.options.textMargin.marginVertical}, "99.9 %");
}

auto VideoLayout::sliderIdent() -> std::string {
	
	std::string ident = (emulator->getRegionEncoding() == Emulator::Interface::Region::Pal) ? "_pal" : "_ntsc";

    if (dynamic_cast<LIBC64::Interface*>(emulator) && layBase.view.mode.spectrum.checked())
        ident += "_spectrum";

	if (layBase.view.mode.cpu.checked())
		ident += "_crtcpu";
	else if (layBase.view.mode.gpu.checked())
		ident += "_crtgpu";

	return ident;
}

auto VideoLayout::loadSettings(bool init) -> void {
    VideoManager::CrtMode crtMode = (VideoManager::CrtMode)_settings->get<unsigned>("video_crt", (unsigned)VideoManager::CrtMode::None, {0u, 2u});
    
    if (crtMode == VideoManager::CrtMode::Gpu)
        layBase.view.mode.gpu.setChecked();
    else if (crtMode == VideoManager::CrtMode::Cpu)
        layBase.view.mode.cpu.setChecked();
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

    updatePresets(!init, true);

    layBase.view.option.linearInterpolation.setChecked( _settings->get<bool>("video_filter", true) );

    layBase.view.option.cpuFilterThreaded.setChecked( _settings->get<bool>("cpu_filter_threaded", true) );

    layShader.main.info.shaderCache.setChecked( _settings->get<bool>("shader_cache", true) );

    bool shaderInternal = _settings->get<bool>("shader_internal", true);

    if (shaderInternal)
        layShader.main.control.internal.setChecked();
    else
        layShader.main.control.external.setChecked();

    unsigned screenTextFontSize = _settings->get<unsigned>("screen_text_fontsize", 18, {8, 36});
    std::string screenTextFont = _settings->get<std::string>("screen_text_font", "");
    unsigned screenTextPosition = _settings->get<unsigned>("screen_text_position", 0);

    layScreenText.options.font.fontSize.setSelectionByUserId(screenTextFontSize);

    auto displayFont = getTTF(screenTextFont);
    if (displayFont)
        layScreenText.options.font.fontType.setSelectionByUserId(displayFont->ident);

    updateFontVisibilities();

    switch((DRIVER::ScreenTextDescription::Position)screenTextPosition) {
        default:
        case DRIVER::ScreenTextDescription::POSITION_BOTTOM_RIGHT: layScreenText.options.position.bottomRight.setChecked(); break;
        case DRIVER::ScreenTextDescription::POSITION_BOTTOM_CENTER: layScreenText.options.position.bottomCenter.setChecked(); break;
        case DRIVER::ScreenTextDescription::POSITION_BOTTOM_LEFT: layScreenText.options.position.bottomLeft.setChecked(); break;
        case DRIVER::ScreenTextDescription::POSITION_TOP_RIGHT: layScreenText.options.position.topRight.setChecked(); break;
        case DRIVER::ScreenTextDescription::POSITION_TOP_CENTER: layScreenText.options.position.topCenter.setChecked(); break;
        case DRIVER::ScreenTextDescription::POSITION_TOP_LEFT: layScreenText.options.position.topLeft.setChecked(); break;
    }

    prepareColBox();

    unsigned screenTextPaddingHorizontal = _settings->get<unsigned>("screen_text_padding_horizontal", 10, {0, 60});
    unsigned screenTextPaddingVertical = _settings->get<unsigned>("screen_text_padding_vertical", 8, {0, 30});

    layScreenText.options.textPadding.paddingHorizontal.slider.setPosition( screenTextPaddingHorizontal );
    layScreenText.options.textPadding.paddingHorizontal.value.setText( std::to_string(screenTextPaddingHorizontal) );
    layScreenText.options.textPadding.paddingVertical.slider.setPosition( screenTextPaddingVertical );
    layScreenText.options.textPadding.paddingVertical.value.setText( std::to_string(screenTextPaddingVertical) );

    unsigned screenTextMarginHorizontal = _settings->get<unsigned>("screen_text_margin_horizontal", 10, {0, 100});
    unsigned screenTextMarginVertical = _settings->get<unsigned>("screen_text_margin_vertical", 12, {0, 100});

    layScreenText.options.textMargin.marginHorizontal.slider.setPosition( screenTextMarginHorizontal );
    layScreenText.options.textMargin.marginHorizontal.setValue( GUIKIT::String::formatFloatingPoint((float)screenTextMarginHorizontal / 5.0, 2, true) );
    layScreenText.options.textMargin.marginVertical.slider.setPosition( screenTextMarginVertical );
    layScreenText.options.textMargin.marginVertical.setValue( GUIKIT::String::formatFloatingPoint((float)screenTextMarginVertical / 5.0, 2, true) );

    bool paddingSeparate = _settings->get<bool>("screen_text_padding_separate", true);
    bool marginSeparate = _settings->get<bool>("screen_text_margin_separate", false);

    layScreenText.options.textPadding.paddingVertical.active.setChecked( paddingSeparate );
    layScreenText.options.textMargin.marginVertical.active.setChecked( marginSeparate );

    layScreenText.options.textPadding.paddingVertical.slider.setEnabled(paddingSeparate);
    layScreenText.options.textPadding.paddingVertical.value.setEnabled(paddingSeparate);
    layScreenText.options.textMargin.marginVertical.slider.setEnabled(marginSeparate);
    layScreenText.options.textMargin.marginVertical.value.setEnabled(marginSeparate);
}

auto VideoLayout::prepareColBox() -> void {
    auto& area = layScreenText.colorBox.selection;
    bool warn = layScreenText.colorBox.type.warning.checked();

    if (warn) {
        area.componentBox[COM_BOX_FG].ident = "screen_warn_color";
        area.componentBox[COM_BOX_BG].ident = "screen_warn_bgcolor";
        area.componentBox[COM_BOX_FG].defaultCol = (255 << 24) | (177 << 16) | (3 << 8) | (23 << 0);
        area.componentBox[COM_BOX_BG].defaultCol = (255 << 24) | (95 << 16) | (169 << 8) | (132 << 0);
    } else {
        area.componentBox[COM_BOX_FG].ident = "screen_text_color";
        area.componentBox[COM_BOX_BG].ident = "screen_text_bgcolor";
        area.componentBox[COM_BOX_FG].defaultCol = ~0;
        area.componentBox[COM_BOX_BG].defaultCol = (255 << 24) | (69 << 16) | (128 << 8) | (116 << 0);
    }

    for(int compBox = 0; compBox < 2; compBox++) {
        std::string& ident = area.componentBox[compBox].ident;
        unsigned& defaultCol = area.componentBox[compBox].defaultCol;
        unsigned color = _settings->get<unsigned>(ident, defaultCol);

        for(int component = 0; component < 4; component++) {
            uint8_t colComponent;

            switch(component & 3) {
                case 0: colComponent = (color >> 16) & 0xff; break;
                case 1: colComponent = (color >> 8) & 0xff; break;
                case 2: colComponent = (color >> 0) & 0xff; break;
                case 3: colComponent = (color >> 24) & 0xff; break;
            }

            area.componentBox[compBox].components[component].slider.setPosition( colComponent );
            area.componentBox[compBox].components[component].value.setText( std::to_string(colComponent) );
        }

        area.control.canvas[compBox].setBackgroundColor(color);
        area.control.hex[compBox].setText( GUIKIT::String::convertIntToHex(color & 0xffffff, true) );
    }
}

auto VideoLayout::clearErrors() -> void {
    showErrors({});
}

auto VideoLayout::showErrors(const std::vector<std::string>& errors) -> void {
    bool hasLabels = layShader.main.errorLabels.size();
    for(auto errorLabel : layShader.main.errorLabels) {
        layShader.main.remove(*errorLabel);
        delete errorLabel;
    }
    layShader.main.errorLabels.clear();
    unsigned errSize = errors.size();

    if (errSize) {
        auto label = new GUIKIT::Label;
        label->setText( trans->getA("corrupted files", true) );
        label->setForegroundColor(ERROR_COLOR);
        label->setFont(GUIKIT::Font::system("bold"));
        layShader.main.errorLabels.push_back(label);
        layShader.main.append(*label, {0u, 0u}, 2);
    }

    for (auto& error : errors) {
        auto label = new GUIKIT::Label;
        label->setText(error);
        label->setForegroundColor(ERROR_COLOR);
        layShader.main.errorLabels.push_back(label);
        layShader.main.append(*label, {0u, 0u}, 2);
    }

    if (hasLabels || errSize)
        layShader.synchronizeLayout();

    tviShader.setImage(errSize ? imgError : imgFolderClosed);
    tviShader.setImageExpanded(errSize ? imgError : imgFolderOpen);
}

auto VideoLayout::loadShader(std::string path) -> bool {
    std::vector<std::string> errors;
    ShaderPreset* preset = vManager()->loadPreset(path, errors);

    if (preset) {
        buildShaderUI(preset, true);
        layShader.main.info.loaded.setText( vManager()->getPresetPathDetailed() );
        layShader.main.control.setEnabled();
        layBase.view.gamma.setEnabled( !layBase.view.mode.gpu.checked() || !vManager()->shaderLumaChromaInput() );
        view->updateShader(emulator);
        enableGPUMode(true);
    }
    showErrors(errors);
    return preset != nullptr;
}

auto VideoLayout::enableGPUMode(bool state) -> void {
    if (state) {
        if (!layBase.view.mode.gpu.checked() && videoDriver->shaderSupport())
            layBase.view.mode.gpu.activate();
    } else if (!layBase.view.mode.rgb.checked())
        layBase.view.mode.rgb.activate();
}

auto VideoLayout::unloadShader() -> void {
    vManager()->clearPreset();
    buildShaderUI(nullptr);
    layShader.main.info.loaded.setText( "" );

    layShader.main.control.unload.setEnabled(false);
    layShader.main.control.appendPreset.setEnabled(false);
    layShader.main.control.prependPreset.setEnabled(false);

    layShader.favourite.control.add.setEnabled(false);
    layBase.view.gamma.setEnabled();
    clearErrors();

    if (!videoDriver->shaderSupport()) {
        moduleTree.remove(tviParams);
        moduleTree.remove(tviShader);
        if (!tviScreenText.selected()) {
            tviBase.setSelected();
            moduleSwitch.setSelection( 1 );
        }
    } else if (!tviBase.selected() && !tviScreenText.selected()) {
        tviShader.setSelected();
        moduleSwitch.setSelection( 2 );
    }
}

auto VideoLayout::addShaderUI() -> void {
    if (!moduleTree.has(tviShader)) {
        moduleTree.append(tviShader);
    }
}

auto VideoLayout::getShaderFolder() -> std::string {
    if (externalFolder())
        return _settings->get<std::string>("slang_folder", "");

    return program->shaderFolder();
}

auto VideoLayout::openShaderFileDialog() -> std::string {
    static const std::vector<std::string> suffixList = {"slang", "slangp"};

    return GUIKIT::BrowserWindow()
            .setWindow( *(this->tabWindow) )
            .setTitle(trans->getA("select slang shader"))
            .setPath( getShaderFolder() )
            .setFilters({ GUIKIT::BrowserWindow::transformFilter("SLANG", suffixList ) })
            .open();
}

auto VideoLayout::clearShaderError() -> void {
    auto preset = vManager()->getPreset();
    if (!preset)
        return;

    for(auto pass : tviPasses) {
        if (pass->image() != &imgDocument)
            pass->setImage(imgDocument);
    }

    layPass.errorMessage.setText("");
}

auto VideoLayout::presentShaderError() -> void {
    std::vector<std::string> errors;
    auto preset = vManager()->getPreset(errors);
    if (!preset)
        return;

    unsigned passId = 0;
    for(auto& pass : preset->passes) {
        if (pass.inUse && !pass.error.empty()) {
            tviPasses[passId]->setImage(imgError);

            if (selectedPassId == passId) {
                std::string _error = pass.error;
                layPass.errorMessage.setText(_error);
                layPass.generated.errorLabel.setForegroundColor(ERROR_COLOR);
            }
        }

        passId++;
    }

    showErrors(errors);
}
