//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
// Edit menu event handlers for MainMenuBar
//////////////////////////////////////////////////////////////////////

#include "app/main.h"
#include <ranges>

#include "ui/menubar/main_menubar.h"
#include "ui/gui.h"
#include "app/application.h"
#include "ui/windows/find_item_window.h"
#include "ui/windows/result_window.h"
#include "app/settings.h"
#include "editor/editor.h"
#include "game/items.h"
#include "game/item.h"
#include "map/map.h"
#include "map/tile.h"
#include "client_assets.h"
#include "brushes/brush.h"
#include "rendering/map_window.h"

namespace OnMapRemoveItems {
	struct RemoveItemCondition {
		RemoveItemCondition(uint16_t itemId) :
			itemId(itemId) { }

		uint16_t itemId;

		bool operator()(Map &map, Item* item, int64_t removed, int64_t done) {
			if (done % 0x8000 == 0) {
				g_gui.SetLoadDone((uint32_t)(100 * done / map.getTileCount()));
			}
			return item->getID() == itemId && !item->isComplex();
		}
	};
}

void MainMenuBar::OnUndo(wxCommandEvent &WXUNUSED(event)) {
	g_gui.DoUndo();
}

void MainMenuBar::OnRedo(wxCommandEvent &WXUNUSED(event)) {
	g_gui.DoRedo();
}

namespace OnSearchForItem {
	struct Finder {
		Finder(uint16_t itemId, uint32_t maxCount, bool findTile = false) :
			itemId(itemId), maxCount(maxCount), findTile(findTile) { }

		bool findTile = false;
		uint16_t itemId;
		uint32_t maxCount;
		std::vector<std::pair<Tile*, Item*>> result;

		bool limitReached() const {
			return result.size() >= (size_t)maxCount;
		}

		void operator()(Map &map, Tile* tile, Item* item, long long done) {
			if (result.size() >= (size_t)maxCount) {
				return;
			}

			if (done % 0x8000 == 0) {
				g_gui.SetLoadDone((unsigned int)(100 * done / map.getTileCount()));
			}

			if (item->getID() == itemId) {
				result.push_back(std::make_pair(tile, item));
			}

			if (!findTile) {
				return;
			}

			if (tile->isHouseTile()) {
				return;
			}

			const auto &tileSearchType = static_cast<FindItemDialog::SearchTileType>(g_settings.getInteger(Config::FIND_TILE_TYPE));
			if (tileSearchType == FindItemDialog::SearchTileType::NoLogout && !tile->isNoLogout()) {
				return;
			}

			if (tileSearchType == FindItemDialog::SearchTileType::PlayerVsPlayer && !tile->isPVP()) {
				return;
			}

			if (tileSearchType == FindItemDialog::SearchTileType::NoPlayerVsPlayer && !tile->isNoPVP()) {
				return;
			}

			if (tileSearchType == FindItemDialog::SearchTileType::ProtectionZone && !tile->isPZ()) {
				return;
			}

			const auto it = std::ranges::find_if(result, [&tile](const auto &pair) {
				return pair.first == tile;
			});

			if (it != result.end()) {
				return;
			}

			result.push_back(std::make_pair(tile, nullptr));
		}
	};
}

void MainMenuBar::OnSearchForItem(wxCommandEvent &WXUNUSED(event)) {
	if (!g_gui.IsEditorOpen()) {
		return;
	}

	const auto &searchMode = static_cast<FindItemDialog::SearchMode>(g_settings.getInteger(Config::FIND_ITEM_MODE));

	FindItemDialog dialog(frame, "Search for Item");
	dialog.setSearchMode(searchMode);
	if (dialog.ShowModal() == wxID_OK) {
		g_settings.setInteger(Config::FIND_ITEM_MODE, static_cast<int>(dialog.getSearchMode()));
		g_settings.setInteger(Config::FIND_TILE_TYPE, static_cast<int>(dialog.getSearchTileType()));

		OnSearchForItem::Finder finder(dialog.getResultID(), (uint32_t)g_settings.getInteger(Config::REPLACE_SIZE), dialog.getSearchMode() == FindItemDialog::SearchMode::TileTypes);

		g_gui.CreateLoadBar("Searching map...");

		foreach_ItemOnMap(g_gui.GetCurrentMap(), finder, false);
		std::vector<std::pair<Tile*, Item*>> &result = finder.result;

		g_gui.DestroyLoadBar();

		if (finder.limitReached()) {
			wxString msg;
			msg << "The configured limit has been reached. Only " << finder.maxCount << " results will be displayed.";
			g_gui.PopupDialog("Notice", msg, wxOK);
		}

		SearchResultWindow* window = g_gui.ShowSearchWindow();
		window->Clear();

		const auto &searchTileType = dialog.getSearchTileType();

		for (auto it = result.begin(); it != result.end(); ++it) {
			const auto &tile = it->first;

			if (dialog.getSearchMode() == FindItemDialog::SearchMode::TileTypes) {
				wxString tileType;

				if (tile->isNoLogout() && searchTileType == FindItemDialog::SearchTileType::NoLogout) {
					tileType = "No Logout";
				} else if (tile->isPVP() && searchTileType == FindItemDialog::SearchTileType::PlayerVsPlayer) {
					tileType = "PVP";
				} else if (tile->isNoPVP() && searchTileType == FindItemDialog::SearchTileType::NoPlayerVsPlayer) {
					tileType = "No PVP";
				} else if (tile->isPZ() && searchTileType == FindItemDialog::SearchTileType::ProtectionZone) {
					tileType = "PZ";
				}

				window->AddPosition(tileType, tile->getPosition());
			} else {
				window->AddPosition(wxstr(it->second->getName()), tile->getPosition());
			}
		}
	}
	dialog.Destroy();
}

void MainMenuBar::OnReplaceItems(wxCommandEvent &WXUNUSED(event)) {
	if (!ClientAssets::isLoaded()) {
		return;
	}

	if (MapTab* tab = g_gui.GetCurrentMapTab()) {
		if (MapWindow* window = tab->GetView()) {
			window->ShowReplaceItemsDialog(false);
		}
	}
}

void MainMenuBar::OnSearchForStuffOnMap(wxCommandEvent &WXUNUSED(event)) {
	SearchItems(true, true, true, true);
}

void MainMenuBar::OnSearchForUniqueOnMap(wxCommandEvent &WXUNUSED(event)) {
	SearchItems(true, false, false, false);
}

void MainMenuBar::OnSearchForActionOnMap(wxCommandEvent &WXUNUSED(event)) {
	SearchItems(false, true, false, false);
}

void MainMenuBar::OnSearchForContainerOnMap(wxCommandEvent &WXUNUSED(event)) {
	SearchItems(false, false, true, false);
}

void MainMenuBar::OnSearchForWriteableOnMap(wxCommandEvent &WXUNUSED(event)) {
	SearchItems(false, false, false, true);
}

void MainMenuBar::OnSearchForStuffOnSelection(wxCommandEvent &WXUNUSED(event)) {
	SearchItems(true, true, true, true, true);
}

void MainMenuBar::OnSearchForUniqueOnSelection(wxCommandEvent &WXUNUSED(event)) {
	SearchItems(true, false, false, false, true);
}

void MainMenuBar::OnSearchForActionOnSelection(wxCommandEvent &WXUNUSED(event)) {
	SearchItems(false, true, false, false, true);
}

void MainMenuBar::OnSearchForContainerOnSelection(wxCommandEvent &WXUNUSED(event)) {
	SearchItems(false, false, true, false, true);
}

void MainMenuBar::OnSearchForWriteableOnSelection(wxCommandEvent &WXUNUSED(event)) {
	SearchItems(false, false, false, true, true);
}

void MainMenuBar::OnSearchForItemOnSelection(wxCommandEvent &WXUNUSED(event)) {
	if (!g_gui.IsEditorOpen()) {
		return;
	}

	FindItemDialog dialog(frame, "Search on Selection", false, true);
	dialog.setSearchMode((FindItemDialog::SearchMode)g_settings.getInteger(Config::FIND_ITEM_MODE));
	if (dialog.ShowModal() == wxID_OK) {
		OnSearchForItem::Finder finder(dialog.getResultID(), (uint32_t)g_settings.getInteger(Config::REPLACE_SIZE), false);
		g_gui.CreateLoadBar("Searching on selected area...");

		foreach_ItemOnMap(g_gui.GetCurrentMap(), finder, true);
		std::vector<std::pair<Tile*, Item*>> &result = finder.result;

		g_gui.DestroyLoadBar();

		if (finder.limitReached()) {
			const auto message = wxString::Format("The configured limit has been reached. Only %lu results will be displayed.", finder.maxCount);
			g_gui.PopupDialog("Notice", message, wxOK);
		}

		SearchResultWindow* window = g_gui.ShowSearchWindow();
		window->Clear();
		for (std::vector<std::pair<Tile*, Item*>>::const_iterator iter = result.begin(); iter != result.end(); ++iter) {
			Tile* tile = iter->first;
			Item* item = iter->second;
			window->AddPosition(wxstr(item->getName()), tile->getPosition());
		}

		g_settings.setInteger(Config::FIND_ITEM_MODE, (int)dialog.getSearchMode());
	}

	dialog.Destroy();
}

void MainMenuBar::OnReplaceItemsOnSelection(wxCommandEvent &WXUNUSED(event)) {
	if (!ClientAssets::isLoaded()) {
		return;
	}

	if (MapTab* tab = g_gui.GetCurrentMapTab()) {
		if (MapWindow* window = tab->GetView()) {
			window->ShowReplaceItemsDialog(true);
		}
	}
}

void MainMenuBar::OnRemoveItemOnSelection(wxCommandEvent &WXUNUSED(event)) {
	if (!g_gui.IsEditorOpen()) {
		return;
	}

	FindItemDialog dialog(frame, "Remove Item on Selection", false, true);
	if (dialog.ShowModal() == wxID_OK) {
		g_gui.GetCurrentEditor()->clearActions();
		g_gui.CreateLoadBar("Searching item on selection to remove...");
		OnMapRemoveItems::RemoveItemCondition condition(dialog.getResultID());
		const auto itemsRemoved = RemoveItemOnMap(g_gui.GetCurrentMap(), condition, true);
		g_gui.DestroyLoadBar();

		g_gui.PopupDialog("Remove Item", wxString::Format("%lld items removed.", itemsRemoved), wxOK);
		g_gui.GetCurrentMap().doChange();
		g_gui.RefreshView();
	}
	dialog.Destroy();
}

void MainMenuBar::OnRemoveMonstersOnSelection(wxCommandEvent &WXUNUSED(event)) {
	if (!g_gui.IsEditorOpen()) {
		return;
	}

	g_gui.GetCurrentEditor()->clearActions();
	g_gui.CreateLoadBar("Searching monsters on selection to remove...");
	const auto monstersRemoved = RemoveMonstersOnMap(g_gui.GetCurrentMap(), true);
	g_gui.DestroyLoadBar();

	g_gui.PopupDialog("Remove Monsters", wxString::Format("%lld monsters removed.", monstersRemoved), wxOK);
	g_gui.GetCurrentMap().doChange();
	g_gui.RefreshView();
}

void MainMenuBar::OnEditMonsterSpawnTime(wxCommandEvent &WXUNUSED(event)) {
	if (!g_gui.IsEditorOpen()) {
		return;
	}

	wxTextEntryDialog dialog(
		frame,
		"Enter the new spawn time (must be 1 or greater):",
		"Spawn Time:"
	);
	dialog.SetValue(wxString::Format("%d", g_gui.GetSpawnMonsterTime()));
	if (dialog.ShowModal() == wxID_OK) {
		long spawnTime;
		wxString inputValue = dialog.GetValue();
		if (!inputValue.IsNumber() || !inputValue.ToLong(&spawnTime) || spawnTime < 1 || spawnTime > std::numeric_limits<int32_t>::max()) {
			g_gui.PopupDialog("Error", "Invalid spawn time. Please enter a numeric value of 1 or greater.", wxOK);
			return;
		}

		g_gui.GetCurrentEditor()->clearActions();
		g_gui.CreateLoadBar("Editing monster spawn time on selection...");
		const auto monstersUpdated = EditMonsterSpawnTime(g_gui.GetCurrentMap(), true, static_cast<int32_t>(spawnTime));
		g_gui.DestroyLoadBar();

		if (monstersUpdated == 0) {
			g_gui.PopupDialog("Edit Monster Spawn Time", "No monsters found in the selected area.", wxOK);
		} else {
			g_gui.PopupDialog("Edit Monster Spawn Time", wxString::Format("%d monsters updated.", monstersUpdated), wxOK);
		}

		g_gui.GetCurrentMap().doChange();
		g_gui.RefreshView();
	}
}

void MainMenuBar::OnCountMonstersOnSelection(wxCommandEvent &WXUNUSED(event)) {
	if (!g_gui.IsEditorOpen()) {
		return;
	}

	g_gui.CreateLoadBar("Counting monsters on selection...");
	const auto result = CountMonstersOnMap(g_gui.GetCurrentMap(), true);
	g_gui.DestroyLoadBar();

	int64_t totalMonsters = result.first;
	const std::unordered_map<std::string, int64_t> &monsterCounts = result.second;

	wxString message = wxString::Format("There are %lld monsters in total.\n\n", totalMonsters);
	for (const auto &pair : monsterCounts) {
		message += wxString::Format("%s: %lld\n", pair.first, pair.second);
	}

	g_gui.PopupDialog("Count Monsters", message, wxOK);
}

void MainMenuBar::OnSelectionTypeChange(wxCommandEvent &WXUNUSED(event)) {
	g_settings.setInteger(Config::COMPENSATED_SELECT, IsItemChecked(MenuBar::SELECT_MODE_COMPENSATE));

	if (IsItemChecked(MenuBar::SELECT_MODE_CURRENT)) {
		g_settings.setInteger(Config::SELECTION_TYPE, SELECT_CURRENT_FLOOR);
	} else if (IsItemChecked(MenuBar::SELECT_MODE_LOWER)) {
		g_settings.setInteger(Config::SELECTION_TYPE, SELECT_ALL_FLOORS);
	} else if (IsItemChecked(MenuBar::SELECT_MODE_VISIBLE)) {
		g_settings.setInteger(Config::SELECTION_TYPE, SELECT_VISIBLE_FLOORS);
	}
}

void MainMenuBar::OnCopy(wxCommandEvent &WXUNUSED(event)) {
	g_gui.DoCopy();
}

void MainMenuBar::OnCut(wxCommandEvent &WXUNUSED(event)) {
	g_gui.DoCut();
}

void MainMenuBar::OnPaste(wxCommandEvent &WXUNUSED(event)) {
	g_gui.PreparePaste();
}

void MainMenuBar::OnToggleAutomagic(wxCommandEvent &WXUNUSED(event)) {
	g_settings.setInteger(Config::USE_AUTOMAGIC, IsItemChecked(MenuBar::AUTOMAGIC));
	g_settings.setInteger(Config::BORDER_IS_GROUND, IsItemChecked(MenuBar::AUTOMAGIC));
	if (g_settings.getInteger(Config::USE_AUTOMAGIC)) {
		g_gui.SetStatusText("Automagic enabled.");
	} else {
		g_gui.SetStatusText("Automagic disabled.");
	}
}

void MainMenuBar::OnBorderizeSelection(wxCommandEvent &WXUNUSED(event)) {
	if (!g_gui.IsEditorOpen()) {
		return;
	}

	g_gui.GetCurrentEditor()->borderizeSelection();
	g_gui.RefreshView();
}

void MainMenuBar::OnBorderizeMap(wxCommandEvent &WXUNUSED(event)) {
	if (!g_gui.IsEditorOpen()) {
		return;
	}

	int ret = g_gui.PopupDialog("Borderize Map", "Are you sure you want to borderize the entire map (this action cannot be undone)?", wxYES | wxNO);
	if (ret == wxID_YES) {
		g_gui.GetCurrentEditor()->borderizeMap(true);
	}

	g_gui.RefreshView();
}

void MainMenuBar::OnRandomizeSelection(wxCommandEvent &WXUNUSED(event)) {
	if (!g_gui.IsEditorOpen()) {
		return;
	}

	g_gui.GetCurrentEditor()->randomizeSelection();
	g_gui.RefreshView();
}

void MainMenuBar::OnRandomizeMap(wxCommandEvent &WXUNUSED(event)) {
	if (!g_gui.IsEditorOpen()) {
		return;
	}

	int ret = g_gui.PopupDialog("Randomize Map", "Are you sure you want to randomize the entire map (this action cannot be undone)?", wxYES | wxNO);
	if (ret == wxID_YES) {
		g_gui.GetCurrentEditor()->randomizeMap(true);
	}

	g_gui.RefreshView();
}

void MainMenuBar::OnJumpToBrush(wxCommandEvent &WXUNUSED(event)) {
	if (!ClientAssets::isLoaded()) {
		return;
	}

	// Create the jump to dialog
	FindDialog* dlg = newd FindBrushDialog(frame);

	// Display dialog to user
	dlg->ShowModal();

	// Retrieve result, if null user canceled
	const Brush* brush = dlg->getResult();
	if (brush) {
		g_gui.SelectBrush(brush, TILESET_UNKNOWN);
	}
	delete dlg;
}

void MainMenuBar::OnJumpToItemBrush(wxCommandEvent &WXUNUSED(event)) {
	if (!ClientAssets::isLoaded()) {
		return;
	}

	// Create the jump to dialog
	FindItemDialog dialog(frame, "Jump to Item");
	dialog.setSearchMode((FindItemDialog::SearchMode)g_settings.getInteger(Config::JUMP_TO_ITEM_MODE));
	if (dialog.ShowModal() == wxID_OK) {
		// Retrieve result, if null user canceled
		const Brush* brush = dialog.getResult();
		if (brush) {
			g_gui.SelectBrush(brush, TILESET_RAW);
		}
		g_settings.setInteger(Config::JUMP_TO_ITEM_MODE, (int)dialog.getSearchMode());
	}
	dialog.Destroy();
}

void MainMenuBar::OnGotoPreviousPosition(wxCommandEvent &WXUNUSED(event)) {
	MapTab* mapTab = g_gui.GetCurrentMapTab();
	if (mapTab) {
		mapTab->GoToPreviousCenterPosition();
	}
}

void MainMenuBar::OnGotoPosition(wxCommandEvent &WXUNUSED(event)) {
	if (!g_gui.IsEditorOpen()) {
		return;
	}

	// Display dialog, it also controls the actual jump
	GotoPositionDialog dlg(frame, *g_gui.GetCurrentEditor());
	dlg.ShowModal();
}

void MainMenuBar::OnMapRemoveItems(wxCommandEvent &WXUNUSED(event)) {
	if (!g_gui.IsEditorOpen()) {
		return;
	}

	FindItemDialog dialog(frame, "Item Type to Remove");
	if (dialog.ShowModal() == wxID_OK) {
		uint16_t itemid = dialog.getResultID();

		g_gui.GetCurrentEditor()->getSelection().clear();
		g_gui.GetCurrentEditor()->clearActions();

		OnMapRemoveItems::RemoveItemCondition condition(itemid);
		g_gui.CreateLoadBar("Searching map for items to remove...");

		int64_t count = RemoveItemOnMap(g_gui.GetCurrentMap(), condition, false);

		g_gui.DestroyLoadBar();

		wxString msg;
		msg << count << " items deleted.";

		g_gui.PopupDialog("Search completed", msg, wxOK);
		g_gui.GetCurrentMap().doChange();
		g_gui.RefreshView();
	}
	dialog.Destroy();
}
