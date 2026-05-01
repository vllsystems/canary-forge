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

void GUI::SaveCurrentMap(FileName filename, bool showdialog) {
	MapTab* mapTab = GetCurrentMapTab();
	if (mapTab) {
		Editor* editor = mapTab->GetEditor();
		if (editor) {
			editor->saveMap(filename, showdialog);

			const std::string &filename = editor->getMap().getFilename();
			const Position &position = mapTab->GetScreenCenterPosition();
			std::ostringstream stream;
			stream << position;
			g_settings.setString(Config::RECENT_EDITED_MAP_PATH, filename);
			g_settings.setString(Config::RECENT_EDITED_MAP_POSITION, stream.str());
		}
	}

	UpdateTitle();
	root->UpdateMenubar();
	root->Refresh();
}

bool GUI::NewMap() {
	FinishWelcomeDialog();

	Editor* editor;
	try {
		editor = newd Editor(copybuffer);
	} catch (std::runtime_error &e) {
		PopupDialog(root, "Error!", wxString(e.what(), wxConvUTF8), wxOK);
		return false;
	}

	auto* mapTab = newd MapTab(tabbook, editor);
	mapTab->OnSwitchEditorMode(mode);
	editor->clearChanges();

	SetStatusText("Created new map");
	UpdateTitle();
	RefreshPalettes();
	root->UpdateMenubar();
	root->Refresh();
	return true;
}

void GUI::OpenMap() {
	wxString wildcard = MAP_LOAD_FILE_WILDCARD;
	wxFileDialog dialog(root, "Open map file", wxEmptyString, wxEmptyString, wildcard, wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE);

	if (dialog.ShowModal() == wxID_OK) {
		wxArrayString paths;
		dialog.GetPaths(paths);
		for (uint32_t i = 0; i < paths.GetCount(); ++i) {
			LoadMap(FileName(paths[i]));
		}
	}
}

void GUI::SaveMap() {
	if (!IsEditorOpen()) {
		return;
	}

	if (GetCurrentMap().hasFile()) {
		SaveCurrentMap(true);
	} else {
		wxString wildcard = MAP_SAVE_FILE_WILDCARD;
		wxFileDialog dialog(root, "Save...", wxEmptyString, wxEmptyString, wildcard, wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

		if (dialog.ShowModal() == wxID_OK) {
			SaveCurrentMap(dialog.GetPath(), true);
		}
	}
}

void GUI::SaveMapAs() {
	if (!IsEditorOpen()) {
		return;
	}

	wxString wildcard = MAP_SAVE_FILE_WILDCARD;
	wxFileDialog dialog(root, "Save As...", "", "", wildcard, wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

	if (dialog.ShowModal() == wxID_OK) {
		SaveCurrentMap(dialog.GetPath(), true);
		UpdateTitle();
		root->menu_bar->AddRecentFile(dialog.GetPath());
		root->UpdateMenubar();
	}
}

bool GUI::LoadMap(const FileName &fileName) {
	FinishWelcomeDialog();

	if (GetCurrentEditor() && !GetCurrentMap().hasChanged() && !GetCurrentMap().hasFile()) {
		g_gui.CloseCurrentEditor();
	}

	Editor* editor;
	try {
		editor = newd Editor(copybuffer, fileName);
	} catch (std::runtime_error &e) {
		PopupDialog(root, "Error!", wxString(e.what(), wxConvUTF8), wxOK);
		return false;
	}

	auto* mapTab = newd MapTab(tabbook, editor);
	mapTab->OnSwitchEditorMode(mode);

	root->AddRecentFile(fileName);

	mapTab->GetView()->FitToMap();
	UpdateTitle();
	ListDialog("Map loader errors", mapTab->GetMap()->getWarnings());
	// Npc and monsters
	root->DoQueryImportCreatures();

	FitViewToMap(mapTab);
	root->UpdateMenubar();

	std::string path = g_settings.getString(Config::RECENT_EDITED_MAP_PATH);
	if (!path.empty()) {
		FileName file(path);
		if (file == fileName) {
			std::istringstream stream(g_settings.getString(Config::RECENT_EDITED_MAP_POSITION));
			Position position;
			stream >> position;
			mapTab->SetScreenCenterPosition(position);
		}
	}

	for (const auto &palette : palettes) {
		palette->OnUpdate(mapTab->GetMap());
	}

	return true;
}
