//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////
// Remere's Map Editor is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Remere's Map Editor is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////

#include "app/main.h"
#include <wx/mstream.h>
#include <wx/display.h>
#include <wx/stopwatch.h>
#include <wx/clipbrd.h>
#include <wx/progdlg.h>

#include "ui/gui.h"

#include "app/application.h"
#include "client_assets.h"
#include "ui/menubar/main_menubar.h"

#include "editor/editor.h"
#include "brushes/brush.h"
#include "map/map.h"
#include "rendering/sprites.h"
#include "game/materials.h"
#include "brushes/doodad_brush.h"
#include "brushes/spawn_monster_brush.h"

#include "ui/dialogs/common_windows.h"
#include "ui/windows/result_window.h"
#include "ui/windows/minimap_window.h"
#include "ui/palette/palette_window.h"
#include "rendering/map_display.h"
#include "app/application.h"
#include "ui/windows/welcome_dialog.h"
#include "brushes/spawn_npc_brush.h"
#include "ui/windows/actions_history_window.h"
#include "lua/lua_scripts_window.h"
#include "rendering/sprite_appearances.h"
#include "ui/dialogs/preferences.h"

#include "live/live_client.h"
#include "live/live_tab.h"
#include "live/live_server.h"

#ifdef __WXOSX__
	#include <AGL/agl.h>
#endif

#include <appearances.pb.h>

#include "ui/gui.h"
#include "app/application.h"
#include "editor/editor.h"
#include "map/map.h"
#include "rendering/map_display.h"
#include "ui/palette/palette_window.h"
#include "ui/map_tab.h"
#include "brushes/brush.h"
#include "brushes/doodad_brush.h"
#include "game/materials.h"
#include "game/items.h"
#include "game/monsters.h"
#include "game/npcs.h"
#include "rendering/sprites.h"
#include "client_assets.h"
#include "rendering/sprite_appearances.h"
#include "ui/windows/minimap_window.h"
#include "ui/windows/actions_history_window.h"
#include "live/live_server.h"
#include "ui/windows/welcome_dialog.h"

PaletteWindow* GUI::GetPalette() {
	if (palettes.empty()) {
		return nullptr;
	}
	return palettes.front();
}

PaletteWindow* GUI::NewPalette() {
	return CreatePalette();
}

void GUI::RefreshPalettes(Map* m, bool usedefault) {
	for (auto &palette : palettes) {
		const auto currentMap = IsEditorOpen() ? &GetCurrentMap() : nullptr;
		const auto defaultMap = usedefault ? currentMap : nullptr;
		palette->OnUpdate(m ? m : defaultMap);
	}
	SelectBrush();

	RefreshActions();
}

void GUI::RefreshOtherPalettes(PaletteWindow* p) {
	for (auto &palette : palettes) {
		if (palette != p) {
			palette->OnUpdate(IsEditorOpen() ? &GetCurrentMap() : nullptr);
		}
	}
	SelectBrush();
}

PaletteWindow* GUI::CreatePalette() {
	if (!ClientAssets::isLoaded()) {
		return nullptr;
	}

	auto* palette = newd PaletteWindow(root, g_materials.tilesets);
	aui_manager->AddPane(palette, wxAuiPaneInfo().Caption("Palette").TopDockable(false).BottomDockable(false));
	palette->OnUpdate(GetCurrentMapTab()->GetMap());
	aui_manager->Update();

	// Make us the active palette
	palettes.push_front(palette);
	// Select brush from this palette
	SelectBrushInternal(palette->GetSelectedBrush());

	RefreshPalettes();

	return palette;
}

void GUI::ActivatePalette(PaletteWindow* p) {
	palettes.erase(std::find(palettes.begin(), palettes.end(), p));
	palettes.push_front(p);
}

void GUI::DestroyPalettes() {
	for (auto palette : palettes) {
		aui_manager->DetachPane(palette);
		palette->Destroy();
		palette = nullptr;
	}
	palettes.clear();
	aui_manager->Update();
}

void GUI::RebuildPalettes() {
	// Palette lits might be modified due to active palette changes
	// Use a temporary list for iterating
	PaletteList tmp = palettes;
	for (auto &piter : tmp) {
		piter->ReloadSettings(IsEditorOpen() ? &GetCurrentMap() : nullptr);
	}
	aui_manager->Update();
}

void GUI::ShowPalette() {
	if (palettes.empty()) {
		return;
	}

	for (auto &palette : palettes) {
		if (aui_manager->GetPane(palette).IsShown()) {
			return;
		}
	}

	aui_manager->GetPane(palettes.front()).Show(true);
	aui_manager->Update();
}

void GUI::SelectPalettePage(PaletteType pt) {
	if (palettes.empty()) {
		CreatePalette();
	}
	PaletteWindow* p = GetPalette();
	if (!p) {
		return;
	}

	ShowPalette();
	p->SelectPage(pt);
	aui_manager->Update();
	SelectBrushInternal(p->GetSelectedBrush());
}

void GUI::CreateMinimap() {
	if (!ClientAssets::isLoaded()) {
		return;
	}

	if (minimap) {
		aui_manager->GetPane(minimap).Show(true);
	} else {
		minimap = newd MinimapWindow(root);
		minimap->Show(true);
		aui_manager->AddPane(minimap, wxAuiPaneInfo().Caption("Minimap"));
	}
	aui_manager->Update();
}

void GUI::HideMinimap() {
	if (minimap) {
		aui_manager->GetPane(minimap).Show(false);
		aui_manager->Update();
	}
}

void GUI::DestroyMinimap() {
	if (minimap) {
		aui_manager->DetachPane(minimap);
		aui_manager->Update();
		minimap->Destroy();
		minimap = nullptr;
	}
}

void GUI::UpdateMinimap(bool immediate) {
	if (IsMinimapVisible()) {
		if (immediate) {
			minimap->Refresh();
		} else {
			minimap->DelayedUpdate();
		}
	}
}

bool GUI::IsMinimapVisible() const {
	if (minimap) {
		const wxAuiPaneInfo &pi = aui_manager->GetPane(minimap);
		if (pi.IsShown()) {
			return true;
		}
	}
	return false;
}
