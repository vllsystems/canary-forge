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

namespace fs = std::filesystem;

bool Editor::importMiniMap(FileName filename, int import_type, int import_x_offset, int import_y_offset, int import_z_offset) {
	return false;
}

bool Editor::importMap(FileName filename, int import_x_offset, int import_y_offset, int import_z_offset, ImportType house_import_type, ImportType spawn_import_type, ImportType spawn_npc_import_type) {
	selection.clear();
	actionQueue->clear();

	Map imported_map;
	bool loaded = imported_map.open(nstr(filename.GetFullPath()));

	if (!loaded) {
		g_gui.PopupDialog("Error", "Error loading map!\n" + imported_map.getError(), wxOK | wxICON_INFORMATION);
		return false;
	}
	g_gui.ListDialog("Warning", imported_map.getWarnings());

	Position offset(import_x_offset, import_y_offset, import_z_offset);

	bool resizemap = false;
	bool resize_asked = false;
	int newsize_x = map.getWidth(), newsize_y = map.getHeight();
	int discarded_tiles = 0;

	g_gui.CreateLoadBar("Merging maps...");

	std::map<uint32_t, uint32_t> town_id_map;
	std::map<uint32_t, uint32_t> house_id_map;

	if (house_import_type != IMPORT_DONT) {
		for (TownMap::iterator tit = imported_map.towns.begin(); tit != imported_map.towns.end();) {
			Town* imported_town = tit->second;
			Town* current_town = map.towns.getTown(imported_town->getID());

			Position oldexit = imported_town->getTemplePosition();
			Position newexit = oldexit + offset;
			if (newexit.isValid()) {
				imported_town->setTemplePosition(newexit);
			}

			switch (house_import_type) {
				case IMPORT_MERGE: {
					town_id_map[imported_town->getID()] = imported_town->getID();
					if (current_town) {
						++tit;
						continue;
					}
					break;
				}
				case IMPORT_SMART_MERGE: {
					if (current_town) {
						// Compare and insert/merge depending on parameters
						if (current_town->getName() == imported_town->getName() && current_town->getID() == imported_town->getID()) {
							// Just add to map
							town_id_map[imported_town->getID()] = current_town->getID();
							++tit;
							continue;
						} else {
							// Conflict! Find a newd id and replace old
							uint32_t new_id = map.towns.getEmptyID();
							imported_town->setID(new_id);
							town_id_map[imported_town->getID()] = new_id;
						}
					} else {
						town_id_map[imported_town->getID()] = imported_town->getID();
					}
					break;
				}
				case IMPORT_INSERT: {
					// Find a newd id and replace old
					uint32_t new_id = map.towns.getEmptyID();
					imported_town->setID(new_id);
					town_id_map[imported_town->getID()] = new_id;
					break;
				}
				case IMPORT_DONT: {
					++tit;
					continue; // Should never happend..?
					break; // Continue or break ?
				}
			}

			map.towns.addTown(imported_town);

			tit = imported_map.towns.erase(tit);
		}

		for (HouseMap::iterator hit = imported_map.houses.begin(); hit != imported_map.houses.end();) {
			House* imported_house = hit->second;
			House* current_house = map.houses.getHouse(imported_house->id);
			imported_house->townid = town_id_map[imported_house->townid];

			const Position &oldexit = imported_house->getExit();
			imported_house->setExit(nullptr, Position()); // Reset it

			switch (house_import_type) {
				case IMPORT_MERGE: {
					house_id_map[imported_house->id] = imported_house->id;
					if (current_house) {
						++hit;
						Position newexit = oldexit + offset;
						if (newexit.isValid()) {
							current_house->setExit(&map, newexit);
						}
						continue;
					}
					break;
				}
				case IMPORT_SMART_MERGE: {
					if (current_house) {
						// Compare and insert/merge depending on parameters
						if (current_house->name == imported_house->name && current_house->townid == imported_house->townid) {
							// Just add to map
							house_id_map[imported_house->id] = current_house->id;
							++hit;
							Position newexit = oldexit + offset;
							if (newexit.isValid()) {
								imported_house->setExit(&map, newexit);
							}
							continue;
						} else {
							// Conflict! Find a newd id and replace old
							uint32_t new_id = map.houses.getEmptyID();
							house_id_map[imported_house->id] = new_id;
							imported_house->id = new_id;
						}
					} else {
						house_id_map[imported_house->id] = imported_house->id;
					}
					break;
				}
				case IMPORT_INSERT: {
					// Find a newd id and replace old
					uint32_t new_id = map.houses.getEmptyID();
					house_id_map[imported_house->id] = new_id;
					imported_house->id = new_id;
					break;
				}
				case IMPORT_DONT: {
					++hit;
					Position newexit = oldexit + offset;
					if (newexit.isValid()) {
						imported_house->setExit(&map, newexit);
					}
					continue; // Should never happend..?
					break;
				}
			}

			Position newexit = oldexit + offset;
			if (newexit.isValid()) {
				imported_house->setExit(&map, newexit);
			}
			map.houses.addHouse(imported_house);

			hit = imported_map.houses.erase(hit);
		}
	}

	// Monster spawn import
	std::map<Position, SpawnMonster*> spawn_monster_map;
	if (spawn_import_type != IMPORT_DONT) {
		for (SpawnNpcPositionList::iterator siter = imported_map.spawnsMonster.begin(); siter != imported_map.spawnsMonster.end();) {
			Position oldSpawnMonsterPos = *siter;
			Position newSpawnMonsterPos = *siter + offset;
			switch (spawn_import_type) {
				case IMPORT_SMART_MERGE:
				case IMPORT_INSERT:
				case IMPORT_MERGE: {
					Tile* imported_tile = imported_map.getTile(oldSpawnMonsterPos);
					if (imported_tile) {
						ASSERT(imported_tile->spawnMonster);
						spawn_monster_map[newSpawnMonsterPos] = imported_tile->spawnMonster;

						SpawnNpcPositionList::iterator next = siter;
						bool cont = true;
						Position next_spawn_monster;

						++next;
						if (next == imported_map.spawnsMonster.end()) {
							cont = false;
						} else {
							next_spawn_monster = *next;
						}
						imported_map.spawnsMonster.erase(siter);
						if (cont) {
							siter = imported_map.spawnsMonster.find(next_spawn_monster);
						} else {
							siter = imported_map.spawnsMonster.end();
						}
					}
					break;
				}
				case IMPORT_DONT: {
					++siter;
					break;
				}
			}
		}
	}

	// Npc spawn import
	std::map<Position, SpawnNpc*> spawn_npc_map;
	if (spawn_npc_import_type != IMPORT_DONT) {
		for (SpawnNpcPositionList::iterator siter = imported_map.spawnsNpc.begin(); siter != imported_map.spawnsNpc.end();) {
			Position oldSpawnNpcPos = *siter;
			Position newSpawnNpcPos = *siter + offset;
			switch (spawn_npc_import_type) {
				case IMPORT_SMART_MERGE:
				case IMPORT_INSERT:
				case IMPORT_MERGE: {
					Tile* importedTile = imported_map.getTile(oldSpawnNpcPos);
					if (importedTile) {
						ASSERT(importedTile->spawnNpc);
						spawn_npc_map[newSpawnNpcPos] = importedTile->spawnNpc;

						SpawnNpcPositionList::iterator next = siter;
						bool cont = true;
						Position next_spawn_npc;

						++next;
						if (next == imported_map.spawnsNpc.end()) {
							cont = false;
						} else {
							next_spawn_npc = *next;
						}
						imported_map.spawnsNpc.erase(siter);
						if (cont) {
							siter = imported_map.spawnsNpc.find(next_spawn_npc);
						} else {
							siter = imported_map.spawnsNpc.end();
						}
					}
					break;
				}
				case IMPORT_DONT: {
					++siter;
					break;
				}
			}
		}
	}

	// Plain merge of waypoints, very simple! :)
	for (WaypointMap::iterator iter = imported_map.waypoints.begin(); iter != imported_map.waypoints.end(); ++iter) {
		iter->second->pos += offset;
	}

	map.waypoints.waypoints.insert(imported_map.waypoints.begin(), imported_map.waypoints.end());
	imported_map.waypoints.waypoints.clear();

	uint64_t tiles_merged = 0;
	uint64_t tiles_to_import = imported_map.tilecount;
	for (MapIterator mit = imported_map.begin(); mit != imported_map.end(); ++mit) {
		if (tiles_merged % 8092 == 0) {
			g_gui.SetLoadDone(int(100.0 * tiles_merged / tiles_to_import));
		}
		++tiles_merged;

		Tile* import_tile = (*mit)->get();
		Position new_pos = import_tile->getPosition() + offset;
		if (!new_pos.isValid()) {
			++discarded_tiles;
			continue;
		}

		if (!resizemap && (new_pos.x > map.getWidth() || new_pos.y > map.getHeight())) {
			if (resize_asked) {
				++discarded_tiles;
				continue;
			} else {
				resize_asked = true;
				int ret = g_gui.PopupDialog("Collision", "The imported tiles are outside the current map scope. Do you want to resize the map? (Else additional tiles will be removed)", wxYES | wxNO);

				if (ret == wxID_YES) {
					// ...
					resizemap = true;
				} else {
					++discarded_tiles;
					continue;
				}
			}
		}

		if (new_pos.x > newsize_x) {
			newsize_x = new_pos.x;
		}
		if (new_pos.y > newsize_y) {
			newsize_y = new_pos.y;
		}

		imported_map.setTile(import_tile->getPosition(), nullptr);
		TileLocation* location = map.createTileL(new_pos);

		// Check if we should update any houses
		int new_houseid = house_id_map[import_tile->getHouseID()];
		House* house = map.houses.getHouse(new_houseid);
		if (import_tile->isHouseTile() && house_import_type != IMPORT_DONT && house) {
			// We need to notify houses of the tile moving
			house->removeTile(import_tile);
			import_tile->setLocation(location);
			house->addTile(import_tile);
		} else {
			import_tile->setLocation(location);
		}

		if (offset != Position(0, 0, 0)) {
			for (ItemVector::iterator iter = import_tile->items.begin(); iter != import_tile->items.end(); ++iter) {
				Item* item = *iter;
				if (Teleport* teleport = dynamic_cast<Teleport*>(item)) {
					teleport->setDestination(teleport->getDestination() + offset);
				}
			}
		}

		Tile* old_tile = map.getTile(new_pos);
		if (old_tile) {
			map.removeSpawnMonster(old_tile);
			map.removeSpawnNpc(old_tile);
		}
		import_tile->spawnMonster = nullptr;
		import_tile->spawnNpc = nullptr;

		map.setTile(new_pos, import_tile, true);
	}

	for (std::map<Position, SpawnMonster*>::iterator spawn_monster_iter = spawn_monster_map.begin(); spawn_monster_iter != spawn_monster_map.end(); ++spawn_monster_iter) {
		Position pos = spawn_monster_iter->first;
		TileLocation* location = map.createTileL(pos);
		Tile* tile = location->get();
		if (!tile) {
			tile = map.allocator(location);
			map.setTile(pos, tile);
		} else if (tile->spawnMonster) {
			map.removeSpawnMonsterInternal(tile);
			delete tile->spawnMonster;
		}
		tile->spawnMonster = spawn_monster_iter->second;

		map.addSpawnMonster(tile);
	}

	for (std::map<Position, SpawnNpc*>::iterator spawn_npc_iter = spawn_npc_map.begin(); spawn_npc_iter != spawn_npc_map.end(); ++spawn_npc_iter) {
		Position pos = spawn_npc_iter->first;
		TileLocation* location = map.createTileL(pos);
		Tile* tile = location->get();
		if (!tile) {
			tile = map.allocator(location);
			map.setTile(pos, tile);
		} else if (tile->spawnNpc) {
			map.removeSpawnNpcInternal(tile);
			delete tile->spawnNpc;
		}
		tile->spawnNpc = spawn_npc_iter->second;

		map.addSpawnNpc(tile);
	}

	g_gui.DestroyLoadBar();

	map.setWidth(newsize_x);
	map.setHeight(newsize_y);
	g_gui.PopupDialog("Success", "Map imported successfully, " + i2ws(discarded_tiles) + " tiles were discarded as invalid.", wxOK);

	g_gui.RefreshPalettes();
	g_gui.FitViewToMap();

	return true;
}
