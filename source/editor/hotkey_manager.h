//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_HOTKEY_MANAGER_H
#define RME_HOTKEY_MANAGER_H

#include "editor/hotkey.h"
#include <string>

class HotkeyManager {
public:
	HotkeyManager();

	void SetHotkey(int index, Hotkey &hotkey);
	const Hotkey &GetHotkey(int index) const;

	void EnableHotkeys();
	void DisableHotkeys();
	bool AreHotkeysEnabled() const;

	std::string Serialize() const;
	void Deserialize(const std::string &data);

private:
	Hotkey hotkeys[10];
	bool hotkeys_enabled;
};

extern HotkeyManager g_hotkeys;

#endif
