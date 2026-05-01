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
#include <ranges>

#include "editor/editor.h"
#include "game/materials.h"
#include "map/map.h"
#include "client_assets.h"
#include "game/complexitem.h"
#include "app/settings.h"
#include "ui/gui.h"
#include "rendering/map_display.h"
#include "brushes/brush.h"
#include "brushes/ground_brush.h"
#include "brushes/wall_brush.h"
#include "brushes/table_brush.h"
#include "brushes/carpet_brush.h"
#include "brushes/waypoint_brush.h"
#include "brushes/house_exit_brush.h"
#include "brushes/doodad_brush.h"
#include "brushes/monster_brush.h"
#include "brushes/npc_brush.h"
#include "brushes/spawn_monster_brush.h"
#include "brushes/spawn_npc_brush.h"
#include "ui/dialogs/preferences.h"

#include "live/live_server.h"
#include "live/live_client.h"
#include "live/live_action.h"

#include <filesystem>
#include <chrono>
#include <iostream>
#include "map/tile_operations.h"

namespace fs = std::filesystem;

void Editor::moveSelection(const Position &offset) {
	if (!CanEdit() || !hasSelection()) {
		return;
	}

	bool borderize = false;
	int drag_threshold = g_settings.getInteger(Config::BORDERIZE_DRAG_THRESHOLD);
	bool create_borders = g_settings.getInteger(Config::USE_AUTOMAGIC)
		&& g_settings.getInteger(Config::BORDERIZE_DRAG);

	TileSet storage;
	BatchAction* batch_action = actionQueue->createBatch(ACTION_MOVE);
	Action* action = actionQueue->createAction(batch_action);

	// Update the tiles with the new positions
	for (Tile* tile : selection) {
		Tile* new_tile = tile->deepCopy(map);
		Tile* storage_tile = map.allocator(tile->getLocation());

		ItemVector selected_items = new_tile->popSelectedItems();
		for (Item* item : selected_items) {
			storage_tile->addItem(item);
		}

		// Move monster spawns
		if (new_tile->spawnMonster && new_tile->spawnMonster->isSelected()) {
			storage_tile->spawnMonster = new_tile->spawnMonster;
			new_tile->spawnMonster = nullptr;
		}
		// Move monster
		const auto monstersSelection = new_tile->popSelectedMonsters();
		std::ranges::for_each(monstersSelection, [&](const auto monster) {
			storage_tile->addMonster(monster);
		});
		// Move npc
		if (new_tile->npc && new_tile->npc->isSelected()) {
			storage_tile->npc = new_tile->npc;
			new_tile->npc = nullptr;
		}
		// Move npc spawns
		if (new_tile->spawnNpc && new_tile->spawnNpc->isSelected()) {
			storage_tile->spawnNpc = new_tile->spawnNpc;
			new_tile->spawnNpc = nullptr;
		}

		if (storage_tile->ground) {
			storage_tile->house_id = new_tile->house_id;
			new_tile->house_id = 0;
			storage_tile->setMapFlags(new_tile->getMapFlags());
			new_tile->setMapFlags(TILESTATE_NONE);
			borderize = true;
		}

		storage.insert(storage_tile);
		action->addChange(new Change(new_tile));
	}
	batch_action->addAndCommitAction(action);

	// Remove old borders (and create some new?)
	if (create_borders && selection.size() < static_cast<size_t>(drag_threshold)) {
		action = actionQueue->createAction(batch_action);
		TileList borderize_tiles;
		// Go through all modified (selected) tiles (might be slow)
		for (const Tile* tile : storage) {
			const Position &pos = tile->getPosition();
			// Go through all neighbours
			Tile* t;
			t = map.getTile(pos.x, pos.y, pos.z);
			if (t && !t->isSelected()) {
				borderize_tiles.push_back(t);
			}
			t = map.getTile(pos.x - 1, pos.y - 1, pos.z);
			if (t && !t->isSelected()) {
				borderize_tiles.push_back(t);
			}
			t = map.getTile(pos.x, pos.y - 1, pos.z);
			if (t && !t->isSelected()) {
				borderize_tiles.push_back(t);
			}
			t = map.getTile(pos.x + 1, pos.y - 1, pos.z);
			if (t && !t->isSelected()) {
				borderize_tiles.push_back(t);
			}
			t = map.getTile(pos.x - 1, pos.y, pos.z);
			if (t && !t->isSelected()) {
				borderize_tiles.push_back(t);
			}
			t = map.getTile(pos.x + 1, pos.y, pos.z);
			if (t && !t->isSelected()) {
				borderize_tiles.push_back(t);
			}
			t = map.getTile(pos.x - 1, pos.y + 1, pos.z);
			if (t && !t->isSelected()) {
				borderize_tiles.push_back(t);
			}
			t = map.getTile(pos.x, pos.y + 1, pos.z);
			if (t && !t->isSelected()) {
				borderize_tiles.push_back(t);
			}
			t = map.getTile(pos.x + 1, pos.y + 1, pos.z);
			if (t && !t->isSelected()) {
				borderize_tiles.push_back(t);
			}
		}

		// Remove duplicates
		borderize_tiles.sort();
		borderize_tiles.unique();

		// Create borders
		for (const Tile* tile : borderize_tiles) {
			Tile* new_tile = tile->deepCopy(map);
			if (borderize) {
				TileOperations::borderize(new_tile, &map);
			}
			TileOperations::wallize(new_tile, &map);
			TileOperations::tableize(new_tile, &map);
			TileOperations::carpetize(new_tile, &map);
			if (tile->ground && tile->ground->isSelected()) {
				new_tile->selectGround();
			}
			action->addChange(new Change(new_tile));
		}
		batch_action->addAndCommitAction(action);
	}

	// New action for adding the destination tiles
	action = actionQueue->createAction(batch_action);
	for (Tile* tile : storage) {
		const Position &old_pos = tile->getPosition();
		Position new_pos = old_pos - offset;
		if (new_pos.z < rme::MapMinLayer && new_pos.z > rme::MapMaxLayer) {
			delete tile;
			continue;
		}

		TileLocation* location = map.createTileL(new_pos);
		Tile* old_dest_tile = location->get();
		Tile* new_dest_tile = nullptr;

		if (!tile->ground || g_settings.getInteger(Config::MERGE_MOVE)) {
			// Move items
			if (old_dest_tile) {
				new_dest_tile = old_dest_tile->deepCopy(map);
			} else {
				new_dest_tile = map.allocator(location);
			}
			new_dest_tile->merge(tile);
			delete tile;
		} else {
			// Replace tile instead of just merge
			tile->setLocation(location);
			new_dest_tile = tile;
		}
		action->addChange(new Change(new_dest_tile));
	}
	batch_action->addAndCommitAction(action);

	if (create_borders && selection.size() < static_cast<size_t>(drag_threshold)) {
		action = actionQueue->createAction(batch_action);
		TileList borderize_tiles;
		// Go through all modified (selected) tiles (might be slow)
		for (Tile* tile : selection) {
			bool add_me = false; // If this tile is touched
			const Position &pos = tile->getPosition();
			// Go through all neighbours
			Tile* t;
			t = map.getTile(pos.x - 1, pos.y - 1, pos.z);
			if (t && !t->isSelected()) {
				borderize_tiles.push_back(t);
				add_me = true;
			}
			t = map.getTile(pos.x - 1, pos.y - 1, pos.z);
			if (t && !t->isSelected()) {
				borderize_tiles.push_back(t);
				add_me = true;
			}
			t = map.getTile(pos.x, pos.y - 1, pos.z);
			if (t && !t->isSelected()) {
				borderize_tiles.push_back(t);
				add_me = true;
			}
			t = map.getTile(pos.x + 1, pos.y - 1, pos.z);
			if (t && !t->isSelected()) {
				borderize_tiles.push_back(t);
				add_me = true;
			}
			t = map.getTile(pos.x - 1, pos.y, pos.z);
			if (t && !t->isSelected()) {
				borderize_tiles.push_back(t);
				add_me = true;
			}
			t = map.getTile(pos.x + 1, pos.y, pos.z);
			if (t && !t->isSelected()) {
				borderize_tiles.push_back(t);
				add_me = true;
			}
			t = map.getTile(pos.x - 1, pos.y + 1, pos.z);
			if (t && !t->isSelected()) {
				borderize_tiles.push_back(t);
				add_me = true;
			}
			t = map.getTile(pos.x, pos.y + 1, pos.z);
			if (t && !t->isSelected()) {
				borderize_tiles.push_back(t);
				add_me = true;
			}
			t = map.getTile(pos.x + 1, pos.y + 1, pos.z);
			if (t && !t->isSelected()) {
				borderize_tiles.push_back(t);
				add_me = true;
			}
			if (add_me) {
				borderize_tiles.push_back(tile);
			}
		}

		// Remove duplicates
		borderize_tiles.sort();
		borderize_tiles.unique();

		// Create borders
		for (const Tile* tile : borderize_tiles) {
			if (!tile || !tile->ground) {
				continue;
			}
			if (tile->ground->getGroundBrush()) {
				Tile* new_tile = tile->deepCopy(map);
				if (borderize) {
					TileOperations::borderize(new_tile, &map);
				}
				TileOperations::wallize(new_tile, &map);
				TileOperations::tableize(new_tile, &map);
				TileOperations::carpetize(new_tile, &map);
				if (tile->ground->isSelected()) {
					new_tile->selectGround();
				}
				action->addChange(new Change(new_tile));
			}
		}
		batch_action->addAndCommitAction(action);
	}

	// Store the action for undo
	addBatch(batch_action);
	updateActions();
	selection.updateSelectionCount();
}

void Editor::destroySelection() {
	if (selection.size() == 0) {
		g_gui.SetStatusText("No selected items to delete.");
	} else {
		int tile_count = 0;
		int item_count = 0;
		int monsterCount = 0;
		PositionList tilestoborder;

		BatchAction* batch = actionQueue->createBatch(ACTION_DELETE_TILES);
		Action* action = actionQueue->createAction(batch);

		for (TileSet::iterator it = selection.begin(); it != selection.end(); ++it) {
			tile_count++;

			Tile* tile = *it;
			Tile* newtile = tile->deepCopy(map);

			ItemVector tile_selection = newtile->popSelectedItems();
			for (ItemVector::iterator iit = tile_selection.begin(); iit != tile_selection.end(); ++iit) {
				++item_count;
				// Delete the items from the tile
				delete *iit;
			}

			auto monstersSelection = newtile->popSelectedMonsters();
			std::ranges::for_each(monstersSelection, [&](auto monster) {
				++monsterCount;
				delete monster;
			});
			// Clear the vector to avoid being used anywhere else in this block with nullptrs
			monstersSelection.clear();

			/*
			for (auto monsterIt = monstersSelection.begin(); monsterIt != monstersSelection.end(); ++monsterIt) {
				++monsterCount;
				// Delete the monsters from the tile
				delete *monsterIt;
			}
			*/

			if (newtile->spawnMonster && newtile->spawnMonster->isSelected()) {
				delete newtile->spawnMonster;
				newtile->spawnMonster = nullptr;
			}
			// Npc
			if (newtile->npc && newtile->npc->isSelected()) {
				delete newtile->npc;
				newtile->npc = nullptr;
			}

			if (newtile->spawnNpc && newtile->spawnNpc->isSelected()) {
				delete newtile->spawnNpc;
				newtile->spawnNpc = nullptr;
			}

			if (g_settings.getInteger(Config::USE_AUTOMAGIC)) {
				for (int y = -1; y <= 1; y++) {
					for (int x = -1; x <= 1; x++) {
						const Position &position = tile->getPosition();
						tilestoborder.push_back(Position(position.x + x, position.y + y, position.z));
					}
				}
			}
			action->addChange(newd Change(newtile));
		}

		batch->addAndCommitAction(action);

		if (g_settings.getInteger(Config::USE_AUTOMAGIC)) {
			// Remove duplicates
			tilestoborder.sort();
			tilestoborder.unique();

			action = actionQueue->createAction(batch);
			for (PositionList::iterator it = tilestoborder.begin(); it != tilestoborder.end(); ++it) {
				TileLocation* location = map.createTileL(*it);
				Tile* tile = location->get();

				if (tile) {
					Tile* new_tile = tile->deepCopy(map);
					TileOperations::borderize(new_tile, &map);
					TileOperations::wallize(new_tile, &map);
					TileOperations::tableize(new_tile, &map);
					TileOperations::carpetize(new_tile, &map);
					action->addChange(newd Change(new_tile));
				} else {
					Tile* new_tile = map.allocator(location);
					TileOperations::borderize(new_tile, &map);
					if (new_tile->size()) {
						action->addChange(newd Change(new_tile));
					} else {
						delete new_tile;
					}
				}
			}

			batch->addAndCommitAction(action);
		}

		addBatch(batch);
		updateActions();

		wxString ss;
		ss << "Deleted " << tile_count << " tile" << (tile_count > 1 ? "s" : "") << " (" << item_count << " item" << (item_count > 1 ? "s" : "") << ")";
		g_gui.SetStatusText(ss);
	}
}
