
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
