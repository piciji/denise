
VideoBaseLayout::View::Mode::Mode(bool withSpectrum) {
    if (withSpectrum) {
        append(palette,{0u, 0u}, 10);
        append(spectrum,{0u, 0u}, 20);
        GUIKIT::RadioBox::setGroup(palette, spectrum);
    }

    append(rgb,{0u, 0u}, 10);
    append(svideoCpu,{0u, 0u}, 10);
    append(svideoGpu,{0u, 0u});

    append(spacer,{~0u, 0u});
    append(reset,{0u, 0u});

    GUIKIT::RadioBox::setGroup(rgb, svideoCpu, svideoGpu);

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
    append(lumaDelay, {~0u, 0u});

}

VideoShaderLayout::Main::Control::Control() {
    append(unload,{0u, 0u}, 10);
    append(save,{0u, 0u}, 10);
    append(folder,{0u, 0u}, 5);
    append(internal,{0u, 0u}, 5);
    append(external,{0u, 0u});
    append(spacer,{~0u, 0u});
    append(prependPreset,{0u, 0u}, 10);
    append(appendPreset,{0u, 0u}, 10);
    append(load,{0u, 0u});

    GUIKIT::RadioBox::setGroup(internal, external);
    internal.setChecked();
    unload.setEnabled(false);
    save.setEnabled(false);
    prependPreset.setEnabled(false);
    appendPreset.setEnabled(false);

    setAlignment(0.5);
}

VideoShaderLayout::Main::Info::Info() {
    append(label,{0u, 0u}, 5);
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

VideoPassLayout::Settings::Identifier::Identifier() {
    append(fileIdent,{0u, 0u}, 10);
    append(filter,{0u, 0u}, 12);
    append(wrap,{0u, 0u}, 10);
    append(bufferType,{0u, 0u}, 12);
    append(mipmap,{0u, 0u}, 10);
    append(modulo,{0u, 0u}, 12);
    append(scaleX,{0u, 0u}, 12);
    append(scaleY,{0u, 0u});
}

VideoPassLayout::Settings::Data::Filter::Filter() {
    append(unspec,{0u, 0u}, 10);
    append(linear,{0u, 0u}, 10);
    append(nearest,{0u, 0u});

    GUIKIT::RadioBox::setGroup( unspec, linear, nearest );
    setAlignment(0.5);
}

VideoPassLayout::Settings::Data::BufferFormat::BufferFormat() {
    append(unorm,{0u, 0u}, 10);
    append(srgb,{0u, 0u}, 10);
    append(fp,{0u, 0u});

    GUIKIT::RadioBox::setGroup( unorm, srgb, fp );
    setAlignment(0.5);
}

VideoPassLayout::Settings::Data::ScaleX::Control::Control() {
    std::vector<GUIKIT::RadioBox*> boxes;
    for(int i = 0; i < 5; i++) {
        radios[i].setText( std::to_string(i+1) + "x" );
        append(radios[i], {0u, 0u}, 10);
        boxes.push_back(&radios[i]);
    }

    GUIKIT::RadioBox::setGroup( boxes );
    setAlignment(0.5);
}

VideoPassLayout::Settings::Data::ScaleX::ScaleX() {
    append(label, {0u, 0u}, 10);
    append(control, {0u, 0u});

    setAlignment(0.5);
}

VideoPassLayout::Settings::Data::ScaleY::Control::Control() {
    std::vector<GUIKIT::RadioBox*> boxes;
    for(int i = 0; i < 5; i++) {
        radios[i].setText( std::to_string(i+1) + "x" );
        append(radios[i], {0u, 0u}, 10);
        boxes.push_back(&radios[i]);
    }

    GUIKIT::RadioBox::setGroup( boxes );
    setAlignment(0.5);
}

VideoPassLayout::Settings::Data::ScaleY::ScaleY() {
    append(label, {0u, 0u}, 10);
    append(control, {0u, 0u});

    setAlignment(0.5);
}

VideoPassLayout::Settings::Data::Data() {
    append(fileIdent,{0u, 0u}, 10);
    append(filter,{0u, 0u}, 10);
    append(wrap,{0u, 0u}, 10);
    append(bufferFormat,{0u, 0u}, 10);
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
    append(disable,{0u, 0u});
    setAlignment(0.5);
}

VideoPassLayout::VideoPassLayout() {
    append(settings,{0u, 0u}, 20);
    append(control,{0u, 0u}, 20);
    append(errorLabel,{0u, 0u}, 5);
    append(errorMessage, {~0u, 70u});
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

    tviBase.setUserData( (uintptr_t)1 );
    tviBase.setImage( imgDocument );

    tviShader.setUserData( (uintptr_t)2 );
    tviShader.setImage(imgFolderClosed);
    tviShader.setImageExpanded(imgFolderOpen);

    tviParams.setUserData( (uintptr_t)3000 );
    tviParams.setImage(imgFolderClosed);
    tviParams.setImageExpanded(imgFolderOpen);

    moduleTree.append(tviBase);
    moduleTree.append(tviShader);

    moduleSwitch.setLayout(1, layBase, {~0u, ~0u});
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
        program->fastForward( false );
        updatePresets(true, false);
        emuThread->unlock();
    };

    layBase.view.mode.svideoCpu.onActivate = [this]() {
        _settings->set<unsigned>("video_crt", (unsigned)VideoManager::CrtMode::Cpu);
        emuThread->lock();
        program->fastForward( false );
		updatePresets(true, false);
        emuThread->unlock();
    };

    layBase.view.mode.svideoGpu.onActivate = [this]() {
        _settings->set<unsigned>("video_crt", (unsigned)VideoManager::CrtMode::Gpu);
        emuThread->lock();
        program->fastForward( false );
		updatePresets(true, false);
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
        std::vector<std::string> brokenPaths;
        ShaderPreset* preset = vManager()->addPreset(path, true, brokenPaths);

        if (preset) {
            buildShaderUI(preset);
            layShader.main.info.loaded.setText( GUIKIT::String::getFileName( vManager()->getPresetPathCombined() ) );
            if (externalFolder())
                _settings->set<std::string>("slang_folder", GUIKIT::File::getPath(path));
            layShader.favourite.control.add.setEnabled();
            layBase.view.gamma.setEnabled( !layBase.view.mode.svideoGpu.checked() || !vManager()->shaderLumaChromaInput() );
        }
        emuThread->unlock();
        showBrokenPaths(brokenPaths);
    };

    layShader.main.control.appendPreset.onActivate = [this]() {
        auto path = openShaderFileDialog();
        if (path.empty())
            return;

        emuThread->lock();
        std::vector<std::string> brokenPaths;
        ShaderPreset* preset = vManager()->addPreset(path, false, brokenPaths);

        if (preset) {
            buildShaderUI(preset);
            layShader.main.info.loaded.setText( GUIKIT::String::getFileName( vManager()->getPresetPathCombined() ) );
            if (externalFolder())
                _settings->set<std::string>("slang_folder", GUIKIT::File::getPath(path));
            layShader.favourite.control.add.setEnabled();
            layBase.view.gamma.setEnabled( !layBase.view.mode.svideoGpu.checked() || !vManager()->shaderLumaChromaInput() );
        }
        emuThread->unlock();
        showBrokenPaths(brokenPaths);
    };

    layShader.main.control.unload.onActivate = [this]() {
        emuThread->lock();
        unloadShader();
        emuThread->unlock();
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
        view->updateShader();
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
        view->updateShader();
    };

    layShader.favourite.list.onActivate = [this]() {
        int selection = layShader.favourite.list.selection();
        std::string path = layShader.favourite.list.text(selection, 0);
        emuThread->lock();
        loadShader(path);
        emuThread->unlock();
    };

    layShader.favourite.list.onChange = [this]() {
        if (vManager()->getPreset())
            layShader.favourite.control.add.setEnabled();
        layShader.favourite.control.remove.setEnabled();
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
                    val = param.initial;
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

    layPass.settings.data.filter.nearest.onActivate = [this]() {
        emuThread->lock();
        vManager()->setPassFilter(selectedPassId, ShaderPreset::FILTER_NEAREST);
        emuThread->unlock();
    };

    layPass.settings.data.filter.linear.onActivate = [this]() {
        emuThread->lock();
        vManager()->setPassFilter(selectedPassId, ShaderPreset::FILTER_LINEAR);
        emuThread->unlock();
    };

    layPass.settings.data.filter.unspec.onActivate = [this]() {
        emuThread->lock();
        vManager()->setPassFilter(selectedPassId, ShaderPreset::FILTER_UNSPEC);
        emuThread->unlock();
    };

    layPass.settings.data.bufferFormat.unorm.onActivate = [this]() {
        emuThread->lock();
        vManager()->setPassFormat(selectedPassId, ShaderPreset::BUFFER_UNORM);
        emuThread->unlock();
    };

    layPass.settings.data.bufferFormat.srgb.onActivate = [this]() {
        emuThread->lock();
        vManager()->setPassFormat(selectedPassId, ShaderPreset::BUFFER_SRGB);
        emuThread->unlock();
    };

    layPass.settings.data.bufferFormat.fp.onActivate = [this]() {
        emuThread->lock();
        vManager()->setPassFormat(selectedPassId, ShaderPreset::BUFFER_FP);
        emuThread->unlock();
    };

    layPass.settings.data.mipmap.onToggle = [this](bool checked) {
        emuThread->lock();
        vManager()->setPassMipmap(selectedPassId, checked);
        emuThread->unlock();
    };

    for(int i = 0; i < 5; i++) {
        auto& radioX = layPass.settings.data.scaleX.control.radios[i];
        auto& radioY = layPass.settings.data.scaleY.control.radios[i];

        radioX.onActivate = [this, i]() {
            emuThread->lock();
            vManager()->setPassScaleX(selectedPassId, float(i+1));
            emuThread->unlock();
        };

        radioY.onActivate = [this, i]() {
            emuThread->lock();
            vManager()->setPassScaleY(selectedPassId, float(i+1));
            emuThread->unlock();
        };
    }

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

auto VideoLayout::buildShaderUI(ShaderPreset* preset, bool expand) -> void {
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
        tviPass->setImage( imgDocument );
        tviShader.append(*tviPass);

        tviPasses.push_back(tviPass);
    }

    GUIKIT::TreeViewItem* tviParam = nullptr;
    unsigned paramCount = preset->params.size();
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

    //if (expand) {
        tviShader.setExpanded();
        tviParams.setExpanded();
    //}
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
    layPass.control.disable.setText( trans->getA(pass.inUse ? "disable" : "enable") );
    layPass.control.synchronizeLayout();

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
        case ShaderPreset::BUFFER_UNORM: layPass.settings.data.bufferFormat.unorm.setChecked(); break;
        case ShaderPreset::BUFFER_SRGB: layPass.settings.data.bufferFormat.srgb.setChecked(); break;
        case ShaderPreset::BUFFER_FP: layPass.settings.data.bufferFormat.fp.setChecked(); break;
    }

    layPass.settings.data.mipmap.setChecked(pass.mipmap);
    layPass.settings.data.modulo.setText( std::to_string( pass.frameModulo ));

    std::string scaleX = "";
    std::string scaleY = "";

    GUIKIT::RadioBox* useRadioX = nullptr;
    if (pass.scaleTypeX != ShaderPreset::SCALE_ABSOLUTE) {
        for (int i = 0; i < 5; i++) {
            auto& radio = layPass.settings.data.scaleX.control.radios[i];
            if (pass.scaleX == float(i+1)) {
                useRadioX = &radio;
                break;
            }
        }
    }

    if (useRadioX) {
        useRadioX->setChecked();
        scaleX = "Input";
    } else {
        switch(pass.scaleTypeX) {
            default:
            case ShaderPreset::SCALE_INPUT: scaleX = "Input - " + GUIKIT::String::formatFloatingPoint(pass.scaleX, 2); break;
            case ShaderPreset::SCALE_VIEWPORT: scaleX = "Viewport - " + GUIKIT::String::formatFloatingPoint(pass.scaleX, 2); break;
            case ShaderPreset::SCALE_ABSOLUTE: scaleX = "Absolute - " + std::to_string( pass.absX ); break;
        }
    }

    GUIKIT::RadioBox* useRadioY = nullptr;
    if (pass.scaleTypeY != ShaderPreset::SCALE_ABSOLUTE) {
        for (int i = 0; i < 5; i++) {
            auto& radio = layPass.settings.data.scaleY.control.radios[i];
            if (pass.scaleY == float(i+1)) {
                useRadioY = &radio;
                break;
            }
        }
    }

    if (useRadioY) {
        useRadioY->setChecked();
        scaleY = "Input";
    } else {
        switch(pass.scaleTypeY) {
            default:
            case ShaderPreset::SCALE_INPUT: scaleY = "Input - " + GUIKIT::String::formatFloatingPoint(pass.scaleY, 2); break;
            case ShaderPreset::SCALE_VIEWPORT: scaleY = "Viewport - " + GUIKIT::String::formatFloatingPoint(pass.scaleY, 2); break;
            case ShaderPreset::SCALE_ABSOLUTE: scaleY = "Absolute - " + std::to_string( pass.absY ); break;
        }
    }

    layPass.settings.data.scaleX.label.setText( scaleX );
    layPass.settings.data.scaleY.label.setText( scaleY );
    layPass.settings.data.scaleX.synchronizeLayout();
    layPass.settings.data.scaleY.synchronizeLayout();

    if (!pass.error.empty()) {
        std::string _error = pass.error;
        GUIKIT::String::replace(_error, "\n", "\r\n");
        layPass.errorMessage.setText(_error);
    } else
        layPass.errorMessage.setText("");

    layPass.settings.setEnabled(pass.inUse);

    if (pass.inUse) {
        layPass.settings.data.scaleX.control.setEnabled(useRadioX != nullptr);
        layPass.settings.data.scaleY.control.setEnabled(useRadioY != nullptr);
    }

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

            unsigned position = layout->slider.position();
			T value = callTransfer( position );

            vManager()->updateData(baseIdent, checked ? value : T(0));
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

    std::vector<std::string> brokenPaths;
    ShaderPreset* preset = vManager()->getPreset(brokenPaths);
    if (preset) {
        buildShaderUI(preset, false);
        layShader.main.info.loaded.setText(GUIKIT::String::getFileName(vManager()->getPresetPath(), true));
        layShader.main.control.setEnabled();
        layShader.favourite.control.add.setEnabled();
        showBrokenPaths(brokenPaths);
    } else
        unloadShader();
	
	updateVisibillity();
}

auto VideoLayout::updateVisibillity() -> void {
	bool _pal = emulator->getRegionEncoding() == Emulator::Interface::Region::Pal;
    bool isC64 = dynamic_cast<LIBC64::Interface*>(emulator);
    bool crtCpuChecked = layBase.view.mode.svideoCpu.checked();
    bool crtGpuChecked = layBase.view.mode.svideoGpu.checked();

    if (!videoDriver->shaderSupport()) {
        if(crtGpuChecked) {
            layBase.view.mode.rgb.setChecked();
            crtGpuChecked = false;
        }
        layBase.view.mode.svideoGpu.setEnabled(false);
    } else
        layBase.view.mode.svideoGpu.setEnabled();
	
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

    layBase.lumaDelay.setEnabled(isC64 && crtCpuChecked);
    if (isC64 && crtCpuChecked) {
        layBase.lumaDelay.lumaRise.slider.setEnabled(layBase.lumaDelay.lumaRise.active.checked());
        layBase.lumaDelay.lumaFall.slider.setEnabled(layBase.lumaDelay.lumaFall.active.checked());
    }
    
    layBase.view.option.tvGamma.setEnabled( (crtCpuChecked || crtGpuChecked) && layBase.view.mode.palette.checked() && _pal );
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
    layBase.view.mode.svideoGpu.setText( trans->get("Shader") );
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

    layShader.main.control.folder.setText( trans->getA("shader folder", true) );
    layShader.main.control.internal.setText( trans->getA("internal") );
    layShader.main.control.external.setText( trans->getA("external") );
    layShader.main.control.unload.setText( trans->getA("unload") );
    layShader.main.control.save.setText( trans->getA("save") );
    layShader.main.control.load.setText( trans->getA("load") );

    layShader.main.setText( trans->getA("Shader") );
    layShader.favourite.setText( trans->getA("favourites") );
    layShader.favourite.list.setHeaderText({trans->getA("selection")});

    layShader.main.info.label.setText( trans->getA("loaded", true) );
    layShader.main.info.toParams.setText( trans->getA("Parameter") );
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

    tviBase.setText( trans->getA("overview") );
    tviShader.setText( trans->getA("Shader") );
    tviParams.setText( trans->getA("Parameter") );

    layNav.setText( trans->getA("selection") );
    layPass.setText( trans->getA("Pass") );
    layParam.setText( trans->getA("Parameter") );

    layPass.settings.data.filter.nearest.setText( trans->getA("nearest") );
    layPass.settings.data.filter.linear.setText( trans->getA("linear") );
    layPass.settings.data.filter.unspec.setText( trans->getA("unspecified") );

    layPass.settings.data.bufferFormat.unorm.setText( trans->getA("buffer format unorm") );
    layPass.settings.data.bufferFormat.srgb.setText( trans->getA("buffer format srgb") );
    layPass.settings.data.bufferFormat.fp.setText( trans->getA("buffer format fp") );

    layPass.errorLabel.setText( trans->getA("error output", true) );

    for(int i = 0; i < PARAMS_PER_PAGE; i++) {
        paramSliders[i]->defaultButton.setText( trans->getA("default") );
    }

    SliderLayout::scale({&layBase.view.saturation, &layBase.view.gamma, &layBase.view.brightness, &layBase.view.contrast, &layBase.view.phase, &layBase.view.scanlines, &layBase.view.interlace, &layBase.encoding.phaseError, &layBase.encoding.hanoverBars, &layBase.encoding.blur, &layBase.lumaDelay.lumaRise, &layBase.lumaDelay.lumaFall},
                        "-100 %");
}

auto VideoLayout::sliderIdent() -> std::string {
	
	std::string ident = (emulator->getRegionEncoding() == Emulator::Interface::Region::Pal) ? "_pal" : "_ntsc";

    if (dynamic_cast<LIBC64::Interface*>(emulator) && layBase.view.mode.spectrum.checked())
        ident += "_spectrum";

	if (layBase.view.mode.svideoCpu.checked())
		ident += "_crtcpu";
	else if (layBase.view.mode.svideoGpu.checked())
		ident += "_crtgpu";

	return ident;
}

auto VideoLayout::loadSettings(bool init) -> void {
    VideoManager::CrtMode crtMode = (VideoManager::CrtMode)_settings->get<unsigned>("video_crt", (unsigned)VideoManager::CrtMode::None, {0u, 2u});
    
    if (crtMode == VideoManager::CrtMode::Gpu)
        layBase.view.mode.svideoGpu.setChecked();
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

    updatePresets(!init, true);

    layBase.view.option.linearInterpolation.setChecked( _settings->get<bool>("video_filter", true) );
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

    if (brokenPaths.size()) {
        auto label = new GUIKIT::Label;
        label->setText( trans->getA("broken paths", true) );
        label->setForegroundColor(0xff4500);
        label->setFont(GUIKIT::Font::system("bold"));
        layShader.main.brokenLabels.push_back(label);
        layShader.main.append(*label, {0u, 0u}, 2);
    }

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

auto VideoLayout::loadShader(std::string path) -> bool {
    std::vector<std::string> brokenPaths;
    ShaderPreset* preset = vManager()->loadPreset(path, brokenPaths);

    if (preset) {
        buildShaderUI(preset);
        layShader.main.info.loaded.setText( GUIKIT::String::getFileName( path, true ) );
        layShader.main.control.setEnabled();
        layBase.view.gamma.setEnabled( !layBase.view.mode.svideoGpu.checked() || !vManager()->shaderLumaChromaInput() );
    }
    showBrokenPaths(brokenPaths);
    return preset != nullptr;
}

auto VideoLayout::unloadShader() -> void {
    vManager()->clearPreset();
    buildShaderUI(nullptr);
    layShader.main.info.loaded.setText( "" );
    layShader.main.control.setEnabled(false);
    layShader.main.control.load.setEnabled();
    layShader.main.control.folder.setEnabled();
    layShader.main.control.internal.setEnabled();
    layShader.main.control.external.setEnabled();
    layShader.favourite.control.add.setEnabled(false);
    layBase.view.gamma.setEnabled();
    clearBrokenPaths();
}

auto VideoLayout::getShaderFolder() -> std::string {
    if (externalFolder())
        return _settings->get<std::string>("slang_folder", "");

    return program->shaderFolder();
}

auto VideoLayout::openShaderFileDialog() -> std::string {
    static const std::vector<std::string> suffixList = {"slang", "slangp"};

    return GUIKIT::BrowserWindow()
            .setTitle(trans->getA("select slang shader"))
            .setPath( getShaderFolder() )
            .setFilters({ GUIKIT::BrowserWindow::transformFilter("SLANG", suffixList ) })
            .open();
}

auto VideoLayout::presentShaderError() -> void {
    auto preset = vManager()->getPreset();

    unsigned passId = 0;
    for(auto& pass : preset->passes) {
        if (pass.inUse && !pass.error.empty()) {
            tviPasses[passId]->setImage(imgError);

            if (selectedPassId == passId) {
                std::string _error = pass.error;
                GUIKIT::String::replace(_error, "\n", "\r\n");
                layPass.errorMessage.setText(_error);
            }
        }

        passId++;
    }
}
