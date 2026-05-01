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

namespace InternalGUI {
	void logErrorAndSetMessage(const std::string &message, wxString &error) {
		spdlog::error(message);
		error = message + ". Error:" + error;
		g_gui.DestroyLoadBar();
		g_gui.unloadMapWindow();
	}
}

bool GUI::loadMapWindow(wxString &error, wxArrayString &warnings, bool force /* = false*/) {
	if (!force && ClientAssets::isLoaded()) {
		return true;
	}

	// There is another version loaded right now, save window layout
	g_gui.SavePerspective();

	// Disable all rendering so the data is not accessed while reloading
	UnnamedRenderingLock();
	DestroyPalettes();
	DestroyMinimap();

	g_spriteAppearances.terminate();

	// Destroy the previous window
	unloadMapWindow();

	bool ret = LoadDataFiles(error, warnings);
	if (ret) {
		g_gui.LoadPerspective();
	}

	return ret;
}

bool GUI::LoadDataFiles(wxString &error, wxArrayString &warnings) {
	FileName data_path = GetDataDirectory();

	FileName exec_directory;
	try {
		exec_directory = dynamic_cast<wxStandardPaths &>(wxStandardPaths::Get()).GetExecutablePath();
	} catch (std::bad_cast &) {
		error = "Couldn't establish working directory...";
		return false;
	}

	g_spriteAppearances.init();

	g_gui.CreateLoadBar("Loading assets files");
	g_gui.SetLoadDone(0, "Loading assets file");
	spdlog::info("Loading assets");

	g_gui.SetLoadDone(20, "Loading client assets...");
	spdlog::info("Loading appearances");
	if (!ClientAssets::loadAppearanceProtobuf(error, warnings)) {

		InternalGUI::logErrorAndSetMessage("Couldn't load catalog-content.json", error);
		return false;
	}

	g_gui.SetLoadDone(30, "Loading items.xml ...");
	spdlog::info("Loading items");
	FileName itemsPath(exec_directory);
	itemsPath.AppendDir("data");
	itemsPath.AppendDir("items");
	itemsPath.SetFullName("items.xml");

	if (!g_items.loadFromGameXml(itemsPath, error, warnings)) {
		warnings.push_back("Couldn't load items.xml: " + error);
		spdlog::warn("[GUI::LoadDataFiles] {}: {}", itemsPath.GetFullPath().ToStdString(), error.ToStdString());
	}

	{
		std::string monstersLuaDir = g_settings.getString(Config::MONSTERS_LUA_DIRECTORY);
		if (monstersLuaDir.empty()) {
			warnings.push_back("Monsters Lua Directory is not configured. Set it in Edit > Preferences.");
			spdlog::warn("[GUI::LoadDataFiles] Monsters Lua Directory is not configured.");
		} else {
			g_gui.SetLoadDone(47, "Loading Canary monster Lua files...");
			wxString luaErr;
			wxArrayString luaWarn;
			if (!g_monsters.loadFromLuaDir(wxString(monstersLuaDir), luaErr, luaWarn)) {
				warnings.push_back("Error loading Canary monster Lua files: " + luaErr);
			}
			for (const auto &w : luaWarn) {
				warnings.push_back(w);
			}
		}
	}

	{
		std::string npcsLuaDir = g_settings.getString(Config::NPCS_LUA_DIRECTORY);
		if (npcsLuaDir.empty()) {
			warnings.push_back("NPCs Lua Directory is not configured. Set it in Edit > Preferences.");
			spdlog::warn("[GUI::LoadDataFiles] NPCs Lua Directory is not configured.");
		} else {
			g_gui.SetLoadDone(48, "Loading Canary NPC Lua files...");
			wxString luaErr;
			wxArrayString luaWarn;
			if (!g_npcs.loadFromLuaDir(wxString(npcsLuaDir), luaErr, luaWarn)) {
				warnings.push_back("Error loading Canary NPC Lua files: " + luaErr);
			}
			for (const auto &w : luaWarn) {
				warnings.push_back(w);
			}
		}
	}

	g_gui.SetLoadDone(50, "Loading materials.xml ...");
	spdlog::info("Loading materials");
	auto materialsPath = wxString(data_path.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR) + "materials/materials.xml");
	if (!g_materials.loadMaterials(materialsPath, error, warnings)) {
		warnings.push_back("Couldn't load materials.xml: " + error);
		spdlog::warn("[GUI::LoadDataFiles] {}: {}", materialsPath.ToStdString(), error.ToStdString());
	}

	g_gui.SetLoadDone(70, "Finishing...");
	spdlog::info("Finishing load map...");

	g_brushes.init();
	g_materials.createOtherTileset();
	g_materials.createNpcTileset();

	g_gui.DestroyLoadBar();
	spdlog::info("Assets loaded");
	return true;
}

void GUI::unloadMapWindow() {
	UnnamedRenderingLock();
	gfx.clear();
	current_brush = nullptr;
	previous_brush = nullptr;

	house_brush = nullptr;
	house_exit_brush = nullptr;
	waypoint_brush = nullptr;
	optional_brush = nullptr;
	eraser = nullptr;
	normal_door_brush = nullptr;
	locked_door_brush = nullptr;
	magic_door_brush = nullptr;
	quest_door_brush = nullptr;
	hatch_door_brush = nullptr;
	window_door_brush = nullptr;

	g_materials.clear();
	g_brushes.clear();
	g_items.clear();
	gfx.clear();

	FileName cdb = ClientAssets::getLocalPath();
	cdb.SetFullName("monsters.xml");
	g_monsters.saveToXML(cdb);
	cdb.SetFullName("npcs.xml");
	g_monsters.saveToXML(cdb);
	g_monsters.clear();
	g_npcs.clear();
}

void GUI::CreateLoadBar(wxString message, bool canCancel /* = false */) {
	progressText = message;

	progressFrom = 0;
	progressTo = 100;
	currentProgress = -1;

	progressBar = newd wxGenericProgressDialog("Loading", progressText + " (0%)", 100, root, wxPD_APP_MODAL | wxPD_SMOOTH | (canCancel ? wxPD_CAN_ABORT : 0));
	progressBar->SetSize(280, -1);
	progressBar->Show(true);

	for (int idx = 0; idx < tabbook->GetTabCount(); ++idx) {
		auto* mt = dynamic_cast<MapTab*>(tabbook->GetTab(idx));
		if (mt && mt->GetEditor()->IsLiveServer()) {
			mt->GetEditor()->GetLiveServer()->startOperation(progressText);
		}
	}
	progressBar->Update(0);
}

void GUI::SetLoadScale(int32_t from, int32_t to) {
	progressFrom = from;
	progressTo = to;
}

bool GUI::SetLoadDone(int32_t done, const wxString &newMessage) {
	if (done == 100) {
		DestroyLoadBar();
		return true;
	}

	int32_t newProgress = progressFrom + static_cast<int32_t>((done / 100.f) * (progressTo - progressFrom));
	newProgress = std::max<int32_t>(0, std::min<int32_t>(100, newProgress));

	bool messageChanged = !newMessage.empty() && newMessage != progressText;
	if (newProgress == currentProgress && !messageChanged) {
		return true;
	}

	if (!newMessage.empty()) {
		progressText = newMessage;
	}

	bool skip = false;
	if (progressBar) {
		progressBar->Update(
			newProgress,
			wxString::Format("%s (%d%%)", progressText, newProgress),
			&skip
		);
		currentProgress = newProgress;
	}

	for (int32_t index = 0; index < tabbook->GetTabCount(); ++index) {
		auto* mapTab = dynamic_cast<MapTab*>(tabbook->GetTab(index));
		if (mapTab && mapTab->GetEditor()) {
			LiveServer* server = mapTab->GetEditor()->GetLiveServer();
			if (server) {
				server->updateOperation(newProgress);
			}
		}
	}

	return skip;
}

void GUI::DestroyLoadBar() {
	if (progressBar) {
		progressBar->Show(false);
		currentProgress = -1;

		progressBar->Destroy();
		progressBar = nullptr;

		if (root->IsActive()) {
			root->Raise();
		} else {
			root->RequestUserAttention();
		}
	}
}
