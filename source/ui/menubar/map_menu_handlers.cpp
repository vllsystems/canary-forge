//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
// Map menu event handlers for MainMenuBar
//////////////////////////////////////////////////////////////////////

#include "app/main.h"

#include "ui/menubar/main_menubar.h"
#include "ui/gui.h"
#include "app/application.h"
#include "app/settings.h"
#include "editor/editor.h"
#include "game/items.h"
#include "game/materials.h"
#include "map/map.h"
#include "map/tile.h"

namespace OnMapRemoveCorpses {
	struct condition {
		condition() { }

		bool operator()(Map &map, Item* item, long long removed, long long done) {
			if (done % 0x800 == 0) {
				g_gui.SetLoadDone((unsigned int)(100 * done / map.getTileCount()));
			}

			return g_materials.isInTileset(item, "Corpses") && !item->isComplex();
		}
	};
}

void MainMenuBar::OnMapRemoveCorpses(wxCommandEvent &WXUNUSED(event)) {
	if (!g_gui.IsEditorOpen()) {
		return;
	}

	int ok = g_gui.PopupDialog("Remove Corpses", "Do you want to remove all corpses from the map?", wxYES | wxNO);

	if (ok == wxID_YES) {
		g_gui.GetCurrentEditor()->getSelection().clear();
		g_gui.GetCurrentEditor()->clearActions();

		OnMapRemoveCorpses::condition func;
		g_gui.CreateLoadBar("Searching map for items to remove...");

		int64_t count = RemoveItemOnMap(g_gui.GetCurrentMap(), func, false);

		g_gui.DestroyLoadBar();

		wxString msg;
		msg << count << " items deleted.";
		g_gui.PopupDialog("Search completed", msg, wxOK);
		g_gui.GetCurrentMap().doChange();
	}
}

namespace OnMapRemoveUnreachable {
	struct condition {
		condition() { }

		bool isReachable(Tile* tile) {
			if (tile == nullptr) {
				return false;
			}
			if (!tile->isBlocking()) {
				return true;
			}
			return false;
		}

		bool operator()(Map &map, Tile* tile, long long removed, long long done, long long total) {
			if (done % 0x1000 == 0) {
				g_gui.SetLoadDone((unsigned int)(100 * done / total));
			}

			const Position &pos = tile->getPosition();
			int sx = std::max(pos.x - 10, 0);
			int ex = std::min(pos.x + 10, 65535);
			int sy = std::max(pos.y - 8, 0);
			int ey = std::min(pos.y + 8, 65535);
			int sz, ez;

			if (pos.z < 8) {
				sz = 0;
				ez = 9;
			} else {
				// underground
				sz = std::max(pos.z - 2, rme::MapGroundLayer);
				ez = std::min(pos.z + 2, rme::MapMaxLayer);
			}

			for (int z = sz; z <= ez; ++z) {
				for (int y = sy; y <= ey; ++y) {
					for (int x = sx; x <= ex; ++x) {
						if (isReachable(map.getTile(x, y, z))) {
							return false;
						}
					}
				}
			}
			return true;
		}
	};
}

void MainMenuBar::OnMapRemoveUnreachable(wxCommandEvent &WXUNUSED(event)) {
	if (!g_gui.IsEditorOpen()) {
		return;
	}

	int ok = g_gui.PopupDialog("Remove Unreachable Tiles", "Do you want to remove all unreachable items from the map?", wxYES | wxNO);

	if (ok == wxID_YES) {
		g_gui.GetCurrentEditor()->getSelection().clear();
		g_gui.GetCurrentEditor()->clearActions();

		OnMapRemoveUnreachable::condition func;
		g_gui.CreateLoadBar("Searching map for tiles to remove...");

		long long removed = remove_if_TileOnMap(g_gui.GetCurrentMap(), func);

		g_gui.DestroyLoadBar();

		wxString msg;
		msg << removed << " tiles deleted.";

		g_gui.PopupDialog("Search completed", msg, wxOK);

		g_gui.GetCurrentMap().doChange();
	}
}

void MainMenuBar::OnMapRemoveEmptyMonsterSpawns(wxCommandEvent &WXUNUSED(event)) {
	if (!g_gui.IsEditorOpen()) {
		return;
	}

	int ok = g_gui.PopupDialog("Remove Empty Monsters Spawns", "Do you want to remove all empty monsters spawns from the map?", wxYES | wxNO);
	if (ok == wxID_YES) {
		Editor* editor = g_gui.GetCurrentEditor();
		editor->getSelection().clear();

		g_gui.CreateLoadBar("Searching map for empty monsters spawns to remove...");

		Map &map = g_gui.GetCurrentMap();
		MonsterVector monsters;
		TileVector toDeleteSpawns;
		for (const auto &spawnPosition : map.spawnsMonster) {
			Tile* tile = map.getTile(spawnPosition);
			if (!tile || !tile->spawnMonster) {
				continue;
			}

			const int32_t radius = tile->spawnMonster->getSize();

			bool empty = true;
			for (auto y = -radius; y <= radius; ++y) {
				for (auto x = -radius; x <= radius; ++x) {
					const auto creatureTile = map.getTile(spawnPosition + Position(x, y, 0));
					if (creatureTile) {
						for (const auto monster : creatureTile->monsters) {
							if (empty) {
								empty = false;
							}

							if (monster->isSaved()) {
								continue;
							}

							monster->save();
							monsters.push_back(monster);
						}
					}
				}
			}

			if (empty) {
				toDeleteSpawns.push_back(tile);
			}
		}

		for (const auto monster : monsters) {
			monster->reset();
		}

		BatchAction* batch = editor->createBatch(ACTION_DELETE_TILES);
		Action* action = editor->createAction(batch);

		const size_t count = toDeleteSpawns.size();
		size_t removed = 0;
		for (const auto &tile : toDeleteSpawns) {
			Tile* newtile = tile->deepCopy(map);
			map.removeSpawnMonster(newtile);
			delete newtile->spawnMonster;
			newtile->spawnMonster = nullptr;
			if (++removed % 5 == 0) {
				// update progress bar for each 5 spawns removed
				g_gui.SetLoadDone(100 * removed / count);
			}
			action->addChange(newd Change(newtile));
		}

		batch->addAndCommitAction(action);
		editor->addBatch(batch);

		g_gui.DestroyLoadBar();

		wxString msg;
		msg << removed << " empty monsters spawns removed.";
		g_gui.PopupDialog("Search completed", msg, wxOK);
		g_gui.GetCurrentMap().doChange();
	}
}

void MainMenuBar::OnMapRemoveEmptyNpcSpawns(wxCommandEvent &WXUNUSED(event)) {
	if (!g_gui.IsEditorOpen()) {
		return;
	}

	int ok = g_gui.PopupDialog("Remove Empty Npcs Spawns", "Do you want to remove all empty npcs spawns from the map?", wxYES | wxNO);
	if (ok == wxID_YES) {
		Editor* editor = g_gui.GetCurrentEditor();
		editor->getSelection().clear();

		g_gui.CreateLoadBar("Searching map for empty npcs spawns to remove...");

		Map &map = g_gui.GetCurrentMap();
		NpcVector npc;
		TileVector toDeleteSpawns;
		for (const auto &spawnPosition : map.spawnsNpc) {
			Tile* tile = map.getTile(spawnPosition);
			if (!tile || !tile->spawnNpc) {
				continue;
			}

			const int32_t radius = tile->spawnNpc->getSize();

			bool empty = true;
			for (int32_t y = -radius; y <= radius; ++y) {
				for (int32_t x = -radius; x <= radius; ++x) {
					Tile* creature_tile = map.getTile(spawnPosition + Position(x, y, 0));
					if (creature_tile && creature_tile->npc && !creature_tile->npc->isSaved()) {
						creature_tile->npc->save();
						npc.push_back(creature_tile->npc);
						empty = false;
					}
				}
			}

			if (empty) {
				toDeleteSpawns.push_back(tile);
			}
		}

		for (Npc* npc : npc) {
			npc->reset();
		}

		BatchAction* batch = editor->getHistoryActions()->createBatch(ACTION_DELETE_TILES);
		Action* action = editor->getHistoryActions()->createAction(batch);

		const size_t count = toDeleteSpawns.size();
		size_t removed = 0;
		for (const auto &tile : toDeleteSpawns) {
			Tile* newtile = tile->deepCopy(map);
			map.removeSpawnNpc(newtile);
			delete newtile->spawnNpc;
			newtile->spawnNpc = nullptr;
			if (++removed % 5 == 0) {
				// update progress bar for each 5 spawns removed
				g_gui.SetLoadDone(100 * removed / count);
			}
			action->addChange(newd Change(newtile));
		}

		batch->addAndCommitAction(action);
		editor->addBatch(batch);

		g_gui.DestroyLoadBar();

		wxString msg;
		msg << removed << " empty npcs spawns removed.";
		g_gui.PopupDialog("Search completed", msg, wxOK);
		g_gui.GetCurrentMap().doChange();
	}
}

void MainMenuBar::OnClearHouseTiles(wxCommandEvent &WXUNUSED(event)) {
	Editor* editor = g_gui.GetCurrentEditor();
	if (!editor) {
		return;
	}

	int ret = g_gui.PopupDialog(
		"Clear Invalid House Tiles",
		"Are you sure you want to remove all house tiles that do not belong to a house (this action cannot be undone)?",
		wxYES | wxNO
	);

	if (ret == wxID_YES) {
		// Editor will do the work
		editor->clearInvalidHouseTiles(true);
	}

	g_gui.RefreshView();
}

void MainMenuBar::OnClearModifiedState(wxCommandEvent &WXUNUSED(event)) {
	Editor* editor = g_gui.GetCurrentEditor();
	if (!editor) {
		return;
	}

	int ret = g_gui.PopupDialog(
		"Clear Modified State",
		"This will have the same effect as closing the map and opening it again. Do you want to proceed?",
		wxYES | wxNO
	);

	if (ret == wxID_YES) {
		// Editor will do the work
		editor->clearModifiedTileState(true);
	}

	g_gui.RefreshView();
}

void MainMenuBar::OnMapCleanHouseItems(wxCommandEvent &WXUNUSED(event)) {
	Editor* editor = g_gui.GetCurrentEditor();
	if (!editor) {
		return;
	}

	int ret = g_gui.PopupDialog(
		"Clear Moveable House Items",
		"Are you sure you want to remove all items inside houses that can be moved (this action cannot be undone)?",
		wxYES | wxNO
	);

	if (ret == wxID_YES) {
		// Editor will do the work
		// editor->removeHouseItems(true);
	}

	g_gui.RefreshView();
}

void MainMenuBar::OnMapEditTowns(wxCommandEvent &WXUNUSED(event)) {
	if (g_gui.GetCurrentEditor()) {
		wxDialog* town_dialog = newd EditTownsDialog(frame, *g_gui.GetCurrentEditor());
		town_dialog->ShowModal();
		town_dialog->Destroy();
	}
}

void MainMenuBar::OnMapEditItems(wxCommandEvent &WXUNUSED(event)) {
	;
}

void MainMenuBar::OnMapEditMonsters(wxCommandEvent &WXUNUSED(event)) {
	;
}

void MainMenuBar::OnMapStatistics(wxCommandEvent &WXUNUSED(event)) {
	if (!g_gui.IsEditorOpen()) {
		return;
	}

	g_gui.CreateLoadBar("Collecting data...");

	Map* map = &g_gui.GetCurrentMap();

	int load_counter = 0;

	uint64_t tile_count = 0;
	uint64_t detailed_tile_count = 0;
	uint64_t blocking_tile_count = 0;
	uint64_t walkable_tile_count = 0;
	double percent_pathable = 0.0;
	double percent_detailed = 0.0;
	uint64_t spawn_monster_count = 0;
	uint64_t spawn_npc_count = 0;
	uint64_t monster_count = 0;
	uint64_t npc_count = 0;
	double monsters_per_spawn = 0.0;
	double npcs_per_spawn = 0.0;

	uint64_t item_count = 0;
	uint64_t loose_item_count = 0;
	uint64_t depot_count = 0;
	uint64_t action_item_count = 0;
	uint64_t unique_item_count = 0;
	uint64_t container_count = 0; // Only includes containers containing more than 1 item

	int town_count = map->towns.count();
	int house_count = map->houses.count();
	std::map<uint32_t, uint32_t> town_sqm_count;
	const Town* largest_town = nullptr;
	uint64_t largest_town_size = 0;
	uint64_t total_house_sqm = 0;
	const House* largest_house = nullptr;
	uint64_t largest_house_size = 0;
	double houses_per_town = 0.0;
	double sqm_per_house = 0.0;
	double sqm_per_town = 0.0;

	for (MapIterator mit = map->begin(); mit != map->end(); ++mit) {
		Tile* tile = (*mit)->get();
		if (load_counter % 8192 == 0) {
			g_gui.SetLoadDone((unsigned int)(int64_t(load_counter) * 95ll / int64_t(map->getTileCount())));
		}

		if (tile->empty()) {
			continue;
		}

		tile_count += 1;

		bool is_detailed = false;
#define ANALYZE_ITEM(_item)                                             \
	{                                                                   \
		item_count += 1;                                                \
		if (!(_item)->isGroundTile() && !(_item)->isBorder()) {         \
			is_detailed = true;                                         \
			const ItemType &it = g_items.getItemType((_item)->getID()); \
			if (it.moveable) {                                          \
				loose_item_count += 1;                                  \
			}                                                           \
			if (it.isDepot()) {                                         \
				depot_count += 1;                                       \
			}                                                           \
			if ((_item)->getActionID() > 0) {                           \
				action_item_count += 1;                                 \
			}                                                           \
			if ((_item)->getUniqueID() > 0) {                           \
				unique_item_count += 1;                                 \
			}                                                           \
			if (Container* c = dynamic_cast<Container*>((_item))) {     \
				if (c->getVector().size()) {                            \
					container_count += 1;                               \
				}                                                       \
			}                                                           \
		}                                                               \
	}
		if (tile->ground) {
			ANALYZE_ITEM(tile->ground);
		}

		for (Item* item : tile->items) {
			ANALYZE_ITEM(item);
		}
#undef ANALYZE_ITEM

		if (tile->spawnMonster) {
			spawn_monster_count += 1;
		}

		if (tile->spawnNpc) {
			spawn_npc_count += 1;
		}

		monster_count += tile->monsters.size();

		if (tile->npc) {
			npc_count += 1;
		}

		if (tile->isBlocking()) {
			blocking_tile_count += 1;
		} else {
			walkable_tile_count += 1;
		}

		if (is_detailed) {
			detailed_tile_count += 1;
		}

		load_counter += 1;
	}

	monsters_per_spawn = (spawn_monster_count != 0 ? double(monster_count) / double(spawn_monster_count) : -1.0);
	npcs_per_spawn = (spawn_npc_count != 0 ? double(npc_count) / double(spawn_npc_count) : -1.0);
	percent_pathable = 100.0 * (tile_count != 0 ? double(walkable_tile_count) / double(tile_count) : -1.0);
	percent_detailed = 100.0 * (tile_count != 0 ? double(detailed_tile_count) / double(tile_count) : -1.0);

	load_counter = 0;
	Houses &houses = map->houses;
	for (HouseMap::const_iterator hit = houses.begin(); hit != houses.end(); ++hit) {
		const House* house = hit->second;

		if (load_counter % 64) {
			g_gui.SetLoadDone((unsigned int)(95ll + int64_t(load_counter) * 5ll / int64_t(house_count)));
		}

		if (house->size() > largest_house_size) {
			largest_house = house;
			largest_house_size = house->size();
		}
		total_house_sqm += house->size();
		town_sqm_count[house->townid] += house->size();
	}

	houses_per_town = (town_count != 0 ? double(house_count) / double(town_count) : -1.0);
	sqm_per_house = (house_count != 0 ? double(total_house_sqm) / double(house_count) : -1.0);
	sqm_per_town = (town_count != 0 ? double(total_house_sqm) / double(town_count) : -1.0);

	Towns &towns = map->towns;
	for (std::map<uint32_t, uint32_t>::iterator town_iter = town_sqm_count.begin();
		 town_iter != town_sqm_count.end();
		 ++town_iter) {
		// No load bar for this, load is non-existant
		uint32_t town_id = town_iter->first;
		uint32_t town_sqm = town_iter->second;
		Town* town = towns.getTown(town_id);
		if (town && town_sqm > largest_town_size) {
			largest_town = town;
			largest_town_size = town_sqm;
		} else {
			// Non-existant town!
		}
	}

	g_gui.DestroyLoadBar();

	std::ostringstream os;
	os.setf(std::ios::fixed, std::ios::floatfield);
	os.precision(2);
	os << "Map statistics for the map \"" << map->getMapDescription() << "\"\n";
	os << "\tTile data:\n";
	os << "\t\tTotal number of tiles: " << tile_count << "\n";
	os << "\t\tNumber of pathable tiles: " << walkable_tile_count << "\n";
	os << "\t\tNumber of unpathable tiles: " << blocking_tile_count << "\n";
	if (percent_pathable >= 0.0) {
		os << "\t\tPercent walkable tiles: " << percent_pathable << "%\n";
	}
	os << "\t\tDetailed tiles: " << detailed_tile_count << "\n";
	if (percent_detailed >= 0.0) {
		os << "\t\tPercent detailed tiles: " << percent_detailed << "%\n";
	}

	os << "\tItem data:\n";
	os << "\t\tTotal number of items: " << item_count << "\n";
	os << "\t\tNumber of moveable tiles: " << loose_item_count << "\n";
	os << "\t\tNumber of depots: " << depot_count << "\n";
	os << "\t\tNumber of containers: " << container_count << "\n";
	os << "\t\tNumber of items with Action ID: " << action_item_count << "\n";
	os << "\t\tNumber of items with Unique ID: " << unique_item_count << "\n";

	os << "\tMonster data:\n";
	os << "\t\tTotal monster count: " << monster_count << "\n";
	os << "\t\tTotal monster spawn count: " << spawn_monster_count << "\n";
	os << "\t\tTotal npc count: " << npc_count << "\n";
	os << "\t\tTotal npc spawn count: " << spawn_npc_count << "\n";
	if (monsters_per_spawn >= 0) {
		os << "\t\tMean monsters per spawn: " << monsters_per_spawn << "\n";
	}

	if (npcs_per_spawn >= 0) {
		os << "\t\tMean npcs per spawn: " << npcs_per_spawn << "\n";
	}

	os << "\tTown/House data:\n";
	os << "\t\tTotal number of towns: " << town_count << "\n";
	os << "\t\tTotal number of houses: " << house_count << "\n";
	if (houses_per_town >= 0) {
		os << "\t\tMean houses per town: " << houses_per_town << "\n";
	}
	os << "\t\tTotal amount of housetiles: " << total_house_sqm << "\n";
	if (sqm_per_house >= 0) {
		os << "\t\tMean tiles per house: " << sqm_per_house << "\n";
	}
	if (sqm_per_town >= 0) {
		os << "\t\tMean tiles per town: " << sqm_per_town << "\n";
	}

	if (largest_town) {
		os << "\t\tLargest Town: \"" << largest_town->getName() << "\" (" << largest_town_size << " sqm)\n";
	}
	if (largest_house) {
		os << "\t\tLargest House: \"" << largest_house->name << "\" (" << largest_house_size << " sqm)\n";
	}

	os << "\n";
	os << "Generated by Canary's Map Editor version " + __RME_VERSION__ + "\n";

	wxDialog* dg = newd wxDialog(frame, wxID_ANY, "Map Statistics", wxDefaultPosition, wxDefaultSize, wxRESIZE_BORDER | wxCAPTION | wxCLOSE_BOX);
	wxSizer* topsizer = newd wxBoxSizer(wxVERTICAL);
	wxTextCtrl* text_field = newd wxTextCtrl(dg, wxID_ANY, wxstr(os.str()), wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY);
	text_field->SetMinSize(wxSize(400, 300));
	topsizer->Add(text_field, wxSizerFlags(5).Expand());

	wxSizer* choicesizer = newd wxBoxSizer(wxHORIZONTAL);
	wxButton* export_button = newd wxButton(dg, wxID_OK, "Export as XML");
	choicesizer->Add(export_button, wxSizerFlags(1).Center());
	export_button->Enable(false);
	choicesizer->Add(newd wxButton(dg, wxID_CANCEL, "OK"), wxSizerFlags(1).Center());
	topsizer->Add(choicesizer, wxSizerFlags(1).Center());
	dg->SetSizerAndFit(topsizer);
	dg->Centre(wxBOTH);

	int ret = dg->ShowModal();

	if (ret == wxID_OK) {
		// std::cout << "XML EXPORT";
	} else if (ret == wxID_CANCEL) {
		// std::cout << "OK";
	}
}

void MainMenuBar::OnMapCleanup(wxCommandEvent &WXUNUSED(event)) {
	int ok = g_gui.PopupDialog("Clean map", "Do you want to remove all invalid items from the map?", wxYES | wxNO);

	if (ok == wxID_YES) {
		g_gui.GetCurrentMap().cleanInvalidTiles(true);
	}
}

void MainMenuBar::OnMapProperties(wxCommandEvent &WXUNUSED(event)) {
	wxDialog* properties = newd MapPropertiesWindow(
		frame,
		static_cast<MapTab*>(g_gui.GetCurrentTab()),
		*g_gui.GetCurrentEditor()
	);

	if (properties->ShowModal() == 0) {
		// FAIL!
		g_gui.CloseAllEditors();
	}
	properties->Destroy();
}
