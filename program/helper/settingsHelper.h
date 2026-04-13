
#pragma once

#include "../../emulation/interface.h"
#include "../../guikit/api.h"
#include "../program.h"

struct SettingsHelper {

    static auto undockSettings() -> bool;
    static auto saveSettings(bool onExit = false) -> void;
    static auto loadSettings() -> void;

    static auto unsetObsoleteConfigs(GUIKIT::Settings* settings, Emulator::Interface* emulator) -> void;
    static auto forceSavingSomeGlobalSettings( ) -> void;
};
