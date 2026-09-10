
#include "settingsHelper.h"

#include "fileHelper.h"
#include "../cmd/cmd.h"
#include "../view/view.h"

auto SettingsHelper::undockSettings() -> bool {
    for (auto settings : settingsStorage) {
        auto guid = settings->getGuid();

        if (guid) {
            auto* emulator = (Emulator::Interface*)guid;

            settings->save( program->settingsFileFromEmuFolder(emulator->ident + "_") );
        } else {
            if (!settings->save( program->settingsFileFromEmuFolder("global_") ))
                return false;
        }
    }
    program->portable = true;
    return true;
}

auto SettingsHelper::saveSettings(bool onExit) -> void {
    bool errorShown = false;

    for (auto settings : settingsStorage) {

        auto guid = settings->getGuid();

        std::string path;

        if (guid) {
            Emulator::Interface* emulator = (Emulator::Interface*)guid;

            path = cmd->getCustomConfig(emulator);
            if (!path.empty()) {
                if (onExit)
                    continue;
            } else {
                path = globalSettings->get<std::string>(emulator->ident + "_custom_settings", "");

                if (path.empty()) {
                    path = program->settingsFileFromEmuFolder(emulator->ident + "_");

                    GUIKIT::File file(path);
                    if (!file.exists())
                        path = program->settingsFile(emulator->ident + "_");

                } else if (onExit) {
                    continue;
                } else {
                    path = FileHelper::getSettingsFolder(emulator) + path;
                }
            }
        } else {
            path = program->settingsFileFromEmuFolder("global_");

            GUIKIT::File file(path);
            if (!file.exists())
                path = program->settingsFile("global_");
        }

        if (!settings->save( path )) {
            if (!errorShown) {
                view->message->warning(trans->get("cfg_not_save", {{"%path%", path}}));
                errorShown = true;
            }
        }
    }
}

auto SettingsHelper::loadSettings() -> void {

    for(auto settings : settingsStorage) {

        auto guid = settings->getGuid();

        if (guid) {
            Emulator::Interface* emulator = (Emulator::Interface*)guid;

            std::string customConfig = cmd->getCustomConfig(emulator);

            if (!customConfig.empty()) {
                if (settings->load(customConfig)) {
                    globalSettings->set("last_used_emu", emulator->ident);
                    continue;
                } else
                    cmd->removeCustomConfig(emulator);
            }

            bool lastUsed = globalSettings->get<bool>( emulator->ident + "_load_last_settings" );

            if (lastUsed) {
                std::string path = globalSettings->get<std::string>(emulator->ident + "_custom_settings", "");
                if (!path.empty()) {
                    path = FileHelper::getSettingsFolder(emulator) + path;

                    if (settings->load(path))
                        continue;
                }
            }

            globalSettings->set<std::string>(emulator->ident + "_custom_settings", "");

            if (!settings->load(program->settingsFileFromEmuFolder(emulator->ident + "_")))
                settings->load(program->settingsFile(emulator->ident + "_"));

            unsetObsoleteConfigs(settings, emulator);

        } else {
            if (!settings->load(program->settingsFileFromEmuFolder("global_"))) {
                settings->load(program->settingsFile("global_"));
                program->portable = false;
            } else
                program->portable = true;

            unsetObsoleteConfigs(settings, nullptr);
        }
    }
}

auto SettingsHelper::unsetObsoleteConfigs(GUIKIT::Settings* settings, Emulator::Interface* emulator) -> void {
    if (!emulator) {
        if (GUIKIT::Application::isWinApi()) {
            if (!settings->get("unset_ds", false)) {
                if (settings->get<std::string>("audio_driver", "") == "DirectSound") {
                    settings->remove("audio_driver");
                    settings->set<unsigned>("audio_latency", 30);
                }
                settings->set("unset_ds", true);
            }
        } else if (GUIKIT::Application::isCocoa()) {
            if (!settings->get("unset_ca", false)) {
                if (settings->get<std::string>("audio_driver", "") == "CoreAudio") {
                    settings->remove("audio_driver");
                    settings->set<unsigned>("audio_latency", 30);
                }
                settings->set("unset_ca", true);
            }
        }
    } else {
        if (!settings->get("unset_video_mode_ident", false)) {
            static const std::string idents[] = {
                "_pal", "_pal_spectrum", "_pal_spectrum_crtcpu", "_pal_spectrum_crtgpu", "_pal_crtcpu", "_pal_crtgpu",
                "_ntsc","_ntsc_spectrum", "_ntsc_spectrum_crtcpu", "_ntsc_spectrum_crtgpu", "_ntsc_crtcpu", "_ntsc_crtgpu"
            };

            auto _crtMode = settings->get<unsigned>("video_crt", 0);
            auto _useSpectrum = settings->get<unsigned>("video_spectrum", 1);
            bool _pal = true;
            bool _c64 = dynamic_cast<LIBC64::Interface*>(emulator);

            if (_c64) {
                auto _region = settings->get<unsigned>("VIC-II", 0);
                _pal = _region <= 1 || _region == 4 || _region == 6 || _region == 7;

            } else {
                auto _region = settings->get<unsigned>("Region", 0);
                _pal = _region == 0;
            }

            std::string _identInUse = _pal ? "_pal" : "_ntsc";
            if (_useSpectrum && _c64)
                _identInUse += "_spectrum";
            if (_crtMode == 1)
                _identInUse += "_crtcpu";
            else if (_crtMode == 2)
                _identInUse += "_crtgpu";

            for (auto& ident : idents) {
                if (ident == _identInUse) {
                    settings->set<unsigned>("video_saturation", settings->get<unsigned>("video_saturation" + ident, 100u,{0u, 200u}));
                    settings->set<unsigned>("video_contrast", settings->get<unsigned>("video_contrast" + ident, 100u,{0u, 200u}));
                    settings->set<unsigned>("video_gamma", settings->get<unsigned>("video_gamma" + ident, 100u,{30u, 280u}));
                    settings->set<unsigned>("video_brightness", settings->get<unsigned>("video_brightness" + ident, 100u,{0u, 200u}));
                    if (_c64) {
                        settings->set<int>("video_phase", settings->get<int>("video_phase" + ident, 0u,{-180, 180}));
                        settings->set<bool>("video_new_luma", settings->get<bool>("video_new_luma" + ident, true));
                    }
                    settings->set<float>("video_phase_error", settings->get<float>("video_phase_error" + ident,  _pal ? (_c64 ? 22.5f : 3.5f ) : 0,{-45.0, 45.0}));
                    settings->set<bool>("video_phase_error_use", settings->get<bool>("video_phase_error_use" + ident, true));

                    settings->set<int>("video_hanover_bars", settings->get<int>("video_hanover_bars" + ident, -10, {-100, 100}));
                    settings->set<bool>("video_hanover_bars_use", settings->get<bool>("video_hanover_bars_use" + ident, true));
                    settings->set<unsigned>("video_blur", settings->get<unsigned>("video_blur" + ident, 30u,{0u, 100u}));
                    settings->set<bool>("video_blur_use", settings->get<bool>("video_blur_use" + ident, true));
                    settings->set<bool>("video_scanlines_use", settings->get<bool>("video_scanlines_use" + ident, false));
                    settings->set<unsigned>("video_scanlines", settings->get<unsigned>("video_scanlines" + ident, 33u,{0u, 100u}));
                    if (!_c64) {
                        settings->set<bool>("video_interlace_use", settings->get<bool>("video_interlace_use" + ident, true));
                        settings->set<unsigned>("video_interlace", settings->get<unsigned>("video_interlace" + ident, 0u,{0u, 100u}));
                    } else {
                        settings->set<bool>("video_luma_rise_use", settings->get<bool>("video_luma_rise_use" + ident, true));
                        settings->set<bool>("video_luma_fall_use", settings->get<bool>("video_luma_fall_use" + ident, true));
                        settings->set<float>("video_luma_rise", settings->get<float>("video_luma_rise" + ident,  2.0, {1.0, 4.0}));
                        settings->set<float>("video_luma_fall", settings->get<float>("video_luma_fall" + ident,  1.2, {1.0, 4.0}));
                    }
                }

                settings->remove("video_saturation" + ident);
                settings->remove("video_contrast" + ident);
                settings->remove("video_gamma" + ident);
                settings->remove("video_brightness" + ident);
                settings->remove("video_phase" + ident);
                settings->remove("video_phase_error" + ident);
                settings->remove("video_phase_error_use" + ident);
                settings->remove("video_new_luma" + ident);
                settings->remove("video_hanover_bars" + ident);
                settings->remove("video_hanover_bars_use" + ident);

                settings->remove("video_blur" + ident);
                settings->remove("video_blur_use" + ident);
                settings->remove("video_scanlines_use" + ident);
                settings->remove("video_scanlines" + ident);
                settings->remove("video_interlace_use" + ident);
                settings->remove("video_interlace" + ident);
                settings->remove("video_luma_rise_use" + ident);
                settings->remove("video_luma_rise" + ident);
                settings->remove("video_luma_fall_use" + ident);
                settings->remove("video_luma_fall" + ident);

                settings->remove("video_radial_distortion" + ident);
                settings->remove("video_radial_distortion_use" + ident);
                settings->remove("video_random_line_offset" + ident);
                settings->remove("video_luma_noise" + ident);
                settings->remove("video_chroma_noise" + ident);
                settings->remove("video_bloom_weight" + ident);
                settings->remove("video_bloom_weight_use" + ident);
                settings->remove("video_bloom_variance" + ident);
                settings->remove("video_bloom_glow" + ident);
                settings->remove("video_bloom_radius" + ident);
                settings->remove("video_aec_glitch_use" + ident);
                settings->remove("video_ba_glitch_use" + ident);
                settings->remove("video_phi0_glitch_use" + ident);
                settings->remove("video_ras_glitch_use" + ident);
                settings->remove("video_cas_glitch_use" + ident);
                settings->remove("video_aec_glitch" + ident);
                settings->remove("video_ba_glitch" + ident);
                settings->remove("video_phi0_glitch" + ident);
                settings->remove("video_ras_glitch" + ident);
                settings->remove("video_cas_glitch" + ident);
                settings->remove("video_fir_filter_length" + ident);
                settings->remove("video_tv_gamma" + ident);
                settings->remove("video_crt_real_gamma" + ident);

                settings->remove("video_fir_filter_sharp" + ident);
                settings->remove("video_mask_luminance" + ident);
                settings->remove("video_mask_level_use" + ident);
                settings->remove("video_mask_level" + ident);
                settings->remove("video_mask_dpi" + ident);
                settings->remove("video_mask_pitch" + ident);

                settings->remove("video_mask_type" + ident);
                settings->remove("video_distortion_hires" + ident);
                settings->remove("video_hires" + ident);
                settings->remove("video_luminance" + ident);
                settings->remove("video_light_from_center" + ident);
                settings->remove("video_light_from_center_use" + ident);
                settings->remove("video_chroma_noise_use" + ident);
                settings->remove("video_luma_noise_use" + ident);
                settings->remove("video_bloom_glow_use" + ident);
                settings->remove("video_random_line_offset_use" + ident);
            }

            settings->remove("video_crt");

            settings->set("unset_video_mode_ident", true);
        }

        if (!settings->get("unset_mid", false)) {
            for (auto setting : settings->getList()) {
                if (GUIKIT::String::findString(setting->getIdent(), "mouse")
                    || (setting->getIdent() == "hotkey_2")) {
                    auto parts = GUIKIT::String::split(setting->value, '|', true);
                    if (parts.size() == 5) {
                        if (parts[1].size() > 1) {
                            parts[1] = "1";
                            setting->value = GUIKIT::String::unsplit(parts, "|");
                        }
                    }
                }
            }

            settings->set("unset_mid", true);
        }

        if (dynamic_cast<LIBAMI::Interface*>(emulator)) {
            if (!settings->get("update_key_acute", false)) {
                if (settings->get<std::string>("keyboard_95", "") == "")
                    settings->set<std::string>("keyboard_95", "0|0|0|13|0");
                settings->set("update_key_acute", true);
            }

            if (!settings->get("update_joy_but3", false)) {
                settings->changeIdent("joypad#1_15", "joypad#1_18");
                settings->changeIdent("joypad#1_15_alt", "joypad#1_18_alt");
                settings->changeIdent("joypad#1_14", "joypad#1_17");
                settings->changeIdent("joypad#1_14_alt", "joypad#1_17_alt");
                settings->changeIdent("joypad#1_13", "joypad#1_16");
                settings->changeIdent("joypad#1_13_alt", "joypad#1_16_alt");
                settings->changeIdent("joypad#1_12", "joypad#1_15");
                settings->changeIdent("joypad#1_12_alt", "joypad#1_15_alt");
                settings->changeIdent("joypad#1_11", "joypad#1_14");
                settings->changeIdent("joypad#1_11_alt", "joypad#1_14_alt");
                settings->changeIdent("joypad#1_10", "joypad#1_13");
                settings->changeIdent("joypad#1_10_alt", "joypad#1_13_alt");

                settings->changeIdent("joypad#1_9", "joypad#1_10");
                settings->changeIdent("joypad#1_9_alt", "joypad#1_10_alt");
                settings->changeIdent("joypad#1_8", "joypad#1_9");
                settings->changeIdent("joypad#1_8_alt", "joypad#1_9_alt");
                settings->changeIdent("joypad#1_7", "joypad#1_8");
                settings->changeIdent("joypad#1_7_alt", "joypad#1_8_alt");
                settings->changeIdent("joypad#1_6", "joypad#1_7");
                settings->changeIdent("joypad#1_6_alt", "joypad#1_7_alt");

                settings->changeIdent("joypad#2_15", "joypad#2_18");
                settings->changeIdent("joypad#2_15_alt", "joypad#2_18_alt");
                settings->changeIdent("joypad#2_14", "joypad#2_17");
                settings->changeIdent("joypad#2_14_alt", "joypad#2_17_alt");
                settings->changeIdent("joypad#2_13", "joypad#2_16");
                settings->changeIdent("joypad#2_13_alt", "joypad#2_16_alt");
                settings->changeIdent("joypad#2_12", "joypad#2_15");
                settings->changeIdent("joypad#2_12_alt", "joypad#2_15_alt");
                settings->changeIdent("joypad#2_11", "joypad#2_14");
                settings->changeIdent("joypad#2_11_alt", "joypad#2_14_alt");
                settings->changeIdent("joypad#2_10", "joypad#2_13");
                settings->changeIdent("joypad#2_10_alt", "joypad#2_13_alt");

                settings->changeIdent("joypad#2_9", "joypad#2_10");
                settings->changeIdent("joypad#2_9_alt", "joypad#2_10_alt");
                settings->changeIdent("joypad#2_8", "joypad#2_9");
                settings->changeIdent("joypad#2_8_alt", "joypad#2_9_alt");
                settings->changeIdent("joypad#2_7", "joypad#2_8");
                settings->changeIdent("joypad#2_7_alt", "joypad#2_8_alt");
                settings->changeIdent("joypad#2_6", "joypad#2_7");
                settings->changeIdent("joypad#2_6_alt", "joypad#2_7_alt");
                settings->set("update_joy_but3", true);
            }
        }
    }
}

auto SettingsHelper::forceSavingSomeGlobalSettings( ) -> void {
    GUIKIT::Settings tempSettings;
    std::string path;
    bool useEmuFolder = true;

    if (!tempSettings.load(program->settingsFileFromEmuFolder("global_"))) {
        useEmuFolder = false;
        if (!tempSettings.load(program->settingsFile("global_")))
            return;
    }

    tempSettings.set<bool>("save_settings_on_exit", false);

    for( auto emulator : emulators ) {
        std::string _emuIdent = emulator->ident;

        auto state = globalSettings->get<bool>( _emuIdent + "_load_last_settings", false );
        auto customSetting = globalSettings->get<std::string>( _emuIdent + "_custom_settings", "");
        path = globalSettings->get<std::string>( _emuIdent + "_settings_path", "");

        tempSettings.set<bool>(_emuIdent + "_load_last_settings", state);
        tempSettings.set<std::string>(_emuIdent + "_custom_settings", customSetting);
        tempSettings.set<std::string>(_emuIdent + "_settings_path", path);
    }

    tempSettings.save( useEmuFolder ? program->settingsFileFromEmuFolder("global_") : program->settingsFile("global_") );
}
