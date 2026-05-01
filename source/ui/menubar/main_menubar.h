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

#ifndef RME_MAIN_BAR_H_
#define RME_MAIN_BAR_H_

#include <wx/docview.h>

#include "ui/menubar/menu_action_ids.h"
namespace MenuBar {
	struct Action;

}

class MainFrame;

class MainMenuBar : public wxEvtHandler {
public:
	MainMenuBar(MainFrame* frame);
	virtual ~MainMenuBar();

	bool Load(const FileName &, wxArrayString &warnings, wxString &error);

	// Update
	// Turn on/off all buttons according to current editor state
	void Update();
	void UpdateFloorMenu(); // Only concerns the floor menu
	void UpdateIndicatorsMenu();

	void AddRecentFile(FileName file);
	void LoadRecentFiles();
	void SaveRecentFiles();
	void LoadScriptsMenu();
	std::vector<wxString> GetRecentFiles();

	// Interface
	void EnableItem(MenuBar::ActionID id, bool enable);
	void CheckItem(MenuBar::ActionID id, bool enable);
	bool IsItemChecked(MenuBar::ActionID id) const;

	// Event handlers for all menu buttons
	// File Menu
	void OnNew(wxCommandEvent &event);
	void OnOpen(wxCommandEvent &event);
	void OnGenerateMap(wxCommandEvent &event);
	void OnOpenRecent(wxCommandEvent &event);
	void OnSave(wxCommandEvent &event);
	void OnSaveAs(wxCommandEvent &event);
	void OnClose(wxCommandEvent &event);
	void OnPreferences(wxCommandEvent &event);
	void OnQuit(wxCommandEvent &event);

	// Import Menu
	// Export Menu
	void OnImportMap(wxCommandEvent &event);
	void OnImportMonsterData(wxCommandEvent &event);
	void OnImportNpcData(wxCommandEvent &event);
	void OnImportMinimap(wxCommandEvent &event);
	void OnImportBitmapToMap(wxCommandEvent &event);
	void OnExportMinimap(wxCommandEvent &event);
	void OnExportTilesets(wxCommandEvent &event);
	void OnReloadDataFiles(wxCommandEvent &event);

	// Edit Menu
	void OnUndo(wxCommandEvent &event);
	void OnRedo(wxCommandEvent &event);
	void OnBorderizeSelection(wxCommandEvent &event);
	void OnBorderizeMap(wxCommandEvent &event);
	void OnRandomizeSelection(wxCommandEvent &event);
	void OnRandomizeMap(wxCommandEvent &event);
	void OnJumpToBrush(wxCommandEvent &event);
	void OnJumpToItemBrush(wxCommandEvent &event);
	void OnGotoPreviousPosition(wxCommandEvent &event);
	void OnGotoPosition(wxCommandEvent &event);
	void OnMapRemoveItems(wxCommandEvent &event);
	void OnMapRemoveCorpses(wxCommandEvent &event);
	void OnMapRemoveUnreachable(wxCommandEvent &event);
	void OnMapRemoveEmptyMonsterSpawns(wxCommandEvent &event);
	void OnMapRemoveEmptyNpcSpawns(wxCommandEvent &event);
	void OnClearHouseTiles(wxCommandEvent &event);
	void OnClearModifiedState(wxCommandEvent &event);
	void OnToggleAutomagic(wxCommandEvent &event);
	void OnSelectionTypeChange(wxCommandEvent &event);
	void OnCut(wxCommandEvent &event);
	void OnCopy(wxCommandEvent &event);
	void OnPaste(wxCommandEvent &event);
	void OnSearchForItem(wxCommandEvent &event);
	void OnReplaceItems(wxCommandEvent &event);
	void OnSearchForStuffOnMap(wxCommandEvent &event);
	void OnSearchForUniqueOnMap(wxCommandEvent &event);
	void OnSearchForActionOnMap(wxCommandEvent &event);
	void OnSearchForContainerOnMap(wxCommandEvent &event);
	void OnSearchForWriteableOnMap(wxCommandEvent &event);

	// Select menu
	void OnSearchForStuffOnSelection(wxCommandEvent &event);
	void OnSearchForUniqueOnSelection(wxCommandEvent &event);
	void OnSearchForActionOnSelection(wxCommandEvent &event);
	void OnSearchForContainerOnSelection(wxCommandEvent &event);
	void OnSearchForWriteableOnSelection(wxCommandEvent &event);
	void OnSearchForItemOnSelection(wxCommandEvent &event);
	void OnReplaceItemsOnSelection(wxCommandEvent &event);
	void OnRemoveItemOnSelection(wxCommandEvent &event);
	void OnRemoveMonstersOnSelection(wxCommandEvent &event);
	void OnEditMonsterSpawnTime(wxCommandEvent &event);
	void OnCountMonstersOnSelection(wxCommandEvent &event);

	// Map menu
	void OnMapEditTowns(wxCommandEvent &event);
	void OnMapEditItems(wxCommandEvent &event);
	void OnMapEditMonsters(wxCommandEvent &event);
	void OnMapCleanHouseItems(wxCommandEvent &event);
	void OnMapCleanup(wxCommandEvent &event);
	void OnMapProperties(wxCommandEvent &event);
	void OnMapStatistics(wxCommandEvent &event);

	// View Menu
	void OnToolbars(wxCommandEvent &event);
	void OnNewView(wxCommandEvent &event);
	void OnToggleFullscreen(wxCommandEvent &event);
	void OnZoomIn(wxCommandEvent &event);
	void OnZoomOut(wxCommandEvent &event);
	void OnZoomNormal(wxCommandEvent &event);
	void OnChangeViewSettings(wxCommandEvent &event);

	// Network menu
	void OnStartLive(wxCommandEvent &event);
	void OnJoinLive(wxCommandEvent &event);
	void OnCloseLive(wxCommandEvent &event);

	// Window Menu
	void OnMinimapWindow(wxCommandEvent &event);
	void OnActionsHistoryWindow(wxCommandEvent &event);
	void OnNewPalette(wxCommandEvent &event);
	void OnTakeScreenshot(wxCommandEvent &event);
	void OnSelectTerrainPalette(wxCommandEvent &event);
	void OnSelectDoodadPalette(wxCommandEvent &event);
	void OnSelectItemPalette(wxCommandEvent &event);
	void OnSelectHousePalette(wxCommandEvent &event);
	void OnSelectMonsterPalette(wxCommandEvent &event);
	void OnSelectNpcPalette(wxCommandEvent &event);
	void OnSelectWaypointPalette(wxCommandEvent &event);
	void OnSelectZonesPalette(wxCommandEvent &event);
	void OnSelectRawPalette(wxCommandEvent &event);

	// Floor menu
	void OnChangeFloor(wxCommandEvent &event);

	// About Menu
	void OnDebugViewDat(wxCommandEvent &event);
	void OnGotoWebsite(wxCommandEvent &event);
	void OnAbout(wxCommandEvent &event);
	void OnSearchForDuplicateItemsOnMap(wxCommandEvent &event);
	void OnSearchForDuplicateItemsOnSelection(wxCommandEvent &event);
	void OnRemoveForDuplicateItemsOnMap(wxCommandEvent &event);
	void OnRemoveForDuplicateItemsOnSelection(wxCommandEvent &event);
	void OnSearchForWallsUponWallsOnMap(wxCommandEvent &event);
	void OnSearchForWallsUponWallsOnSelection(wxCommandEvent &event);

protected:
	// Load and returns a menu item, also sets accelerator
	wxObject* LoadItem(pugi::xml_node node, wxMenu* parent, wxArrayString &warnings, wxString &error);
	// Checks the items in the menus according to the settings (in config)
	void LoadValues();
	void SearchItems(bool unique, bool action, bool container, bool writable, bool onSelection = false);
	void SearchDuplicatedItems(bool onSelection = false);
	void RemoveDuplicatesItems(bool onSelection = false);
	void SearchWallsUponWalls(bool onSelection = false);

protected:
	MainFrame* frame;
	wxMenuBar* menubar;
	wxMenu* scriptsMenu = nullptr;

	// Used so that calling Check on menu items don't trigger events (avoids infinite recursion)
	bool checking_programmaticly;

	std::map<MenuBar::ActionID, std::list<wxMenuItem*>> items;

	// Hardcoded recent files
	wxFileHistory recentFiles;

	std::map<std::string, MenuBar::Action*> actions;

	DECLARE_EVENT_TABLE();
};

namespace MenuBar {
	struct Action {
		Action() :
			id(0), kind(wxITEM_NORMAL) { }
		Action(std::string s, int id, wxItemKind kind, wxCommandEventFunction handler) :
			id(id), setting(0), name(s), kind(kind), handler(handler) { }

		int id;
		int setting;
		std::string name;
		wxItemKind kind;
		wxCommandEventFunction handler;
	};
}

#endif
