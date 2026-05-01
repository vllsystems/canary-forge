//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "app/main.h"
#include "editor/hotkey_manager.h"

#include <sstream>

HotkeyManager g_hotkeys;

HotkeyManager::HotkeyManager() :
	hotkeys_enabled(true) {
}

void HotkeyManager::SetHotkey(int index, Hotkey &hotkey) {
	ASSERT(index >= 0 && index <= 9);
	hotkeys[index] = hotkey;
}

const Hotkey &HotkeyManager::GetHotkey(int index) const {
	ASSERT(index >= 0 && index <= 9);
	return hotkeys[index];
}

void HotkeyManager::EnableHotkeys() {
	hotkeys_enabled = true;
}

void HotkeyManager::DisableHotkeys() {
	hotkeys_enabled = false;
}

bool HotkeyManager::AreHotkeysEnabled() const {
	return hotkeys_enabled;
}

std::string HotkeyManager::Serialize() const {
	std::ostringstream os;
	for (const auto &hotkey : hotkeys) {
		os << hotkey << '\n';
	}
	return os.str();
}

void HotkeyManager::Deserialize(const std::string &data) {
	std::istringstream is(data);
	std::string line;
	int index = 0;
	while (getline(is, line) && index < 10) {
		std::istringstream line_is(line);
		line_is >> hotkeys[index];
		++index;
	}
}
