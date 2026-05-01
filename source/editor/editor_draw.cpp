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

// Macro to avoid useless code repetition
void doSurroundingBorders(DoodadBrush* doodad_brush, PositionList &tilestoborder, Tile* buffer_tile, Tile* new_tile) {
	if (doodad_brush->doNewBorders() && g_settings.getInteger(Config::USE_AUTOMAGIC)) {
		const Position &position = new_tile->getPosition();
		tilestoborder.push_back(Position(position));
		if (buffer_tile->hasGround()) {
			for (int y = -1; y <= 1; y++) {
				for (int x = -1; x <= 1; x++) {
					tilestoborder.push_back(Position(position.x + x, position.y + y, position.z));
				}
			}
		} else if (buffer_tile->hasWall()) {
			tilestoborder.push_back(Position(position.x, position.y - 1, position.z));
			tilestoborder.push_back(Position(position.x - 1, position.y, position.z));
			tilestoborder.push_back(Position(position.x + 1, position.y, position.z));
			tilestoborder.push_back(Position(position.x, position.y + 1, position.z));
		}
	}
}

void removeDuplicateWalls(Tile* buffer, Tile* tile) {
	if (!buffer || buffer->items.empty() || !tile || tile->items.empty()) {
		return;
	}

	for (const Item* item : buffer->items) {
		if (item) {
			WallBrush* brush = item->getWallBrush();
			if (brush) {
				TileOperations::cleanWalls(tile, brush);
			}
		}
	}
}

void Editor::drawInternal(Position offset, bool alt, bool dodraw) {
	if (!CanEdit()) {
		return;
	}

	Brush* brush = g_gui.GetCurrentBrush();
	if (!brush) {
		return;
	}

	if (brush->isDoodad()) {
		BatchAction* batch = actionQueue->createBatch(ACTION_DRAW);
		Action* action = actionQueue->createAction(batch);
		BaseMap* buffer_map = g_gui.doodad_buffer_map;

		Position delta_pos = offset - Position(0x8000, 0x8000, 0x8);
		PositionList tilestoborder;

		for (MapIterator it = buffer_map->begin(); it != buffer_map->end(); ++it) {
			Tile* buffer_tile = (*it)->get();
			Position pos = buffer_tile->getPosition() + delta_pos;
			if (!pos.isValid()) {
				continue;
			}

			TileLocation* location = map.createTileL(pos);
			Tile* tile = location->get();
			DoodadBrush* doodad_brush = brush->asDoodad();

			if (doodad_brush->placeOnBlocking() || alt) {
				if (tile) {
					bool place = true;
					if (!doodad_brush->placeOnDuplicate() && !alt) {
						for (ItemVector::const_iterator iter = tile->items.begin(); iter != tile->items.end(); ++iter) {
							if (doodad_brush->ownsItem(*iter)) {
								place = false;
								break;
							}
						}
					}
					if (place) {
						Tile* new_tile = tile->deepCopy(map);
						removeDuplicateWalls(buffer_tile, new_tile);
						doSurroundingBorders(doodad_brush, tilestoborder, buffer_tile, new_tile);
						new_tile->merge(buffer_tile);
						action->addChange(newd Change(new_tile));
					}
				} else {
					Tile* new_tile = map.allocator(location);
					removeDuplicateWalls(buffer_tile, new_tile);
					doSurroundingBorders(doodad_brush, tilestoborder, buffer_tile, new_tile);
					new_tile->merge(buffer_tile);
					action->addChange(newd Change(new_tile));
				}
			} else {
				if (tile && !tile->isBlocking()) {
					bool place = true;
					if (!doodad_brush->placeOnDuplicate() && !alt) {
						for (ItemVector::const_iterator iter = tile->items.begin(); iter != tile->items.end(); ++iter) {
							if (doodad_brush->ownsItem(*iter)) {
								place = false;
								break;
							}
						}
					}
					if (place) {
						Tile* new_tile = tile->deepCopy(map);
						removeDuplicateWalls(buffer_tile, new_tile);
						doSurroundingBorders(doodad_brush, tilestoborder, buffer_tile, new_tile);
						new_tile->merge(buffer_tile);
						action->addChange(newd Change(new_tile));
					}
				}
			}
		}
		batch->addAndCommitAction(action);

		if (tilestoborder.size() > 0) {
			Action* action = actionQueue->createAction(batch);

			// Remove duplicates
			tilestoborder.sort();
			tilestoborder.unique();

			for (PositionList::const_iterator it = tilestoborder.begin(); it != tilestoborder.end(); ++it) {
				Tile* tile = map.getTile(*it);
				if (tile) {
					Tile* new_tile = tile->deepCopy(map);
					TileOperations::borderize(new_tile, &map);
					TileOperations::wallize(new_tile, &map);
					action->addChange(newd Change(new_tile));
				}
			}
			batch->addAndCommitAction(action);
		}
		addBatch(batch, 2);
	} else if (brush->isHouseExit()) {
		HouseExitBrush* house_exit_brush = brush->asHouseExit();
		if (!house_exit_brush->canDraw(&map, offset)) {
			return;
		}

		House* house = map.houses.getHouse(house_exit_brush->getHouseID());
		if (!house) {
			return;
		}

		BatchAction* batch = actionQueue->createBatch(ACTION_DRAW);
		Action* action = actionQueue->createAction(batch);
		action->addChange(Change::Create(house, offset));
		batch->addAndCommitAction(action);
		addBatch(batch, 2);
	} else if (brush->isWaypoint()) {
		WaypointBrush* waypoint_brush = brush->asWaypoint();
		if (!waypoint_brush->canDraw(&map, offset)) {
			return;
		}

		Waypoint* waypoint = map.waypoints.getWaypoint(waypoint_brush->getWaypoint());
		if (!waypoint || waypoint->pos == offset) {
			return;
		}

		BatchAction* batch = actionQueue->createBatch(ACTION_DRAW);
		Action* action = actionQueue->createAction(batch);
		action->addChange(Change::Create(waypoint, offset));
		batch->addAndCommitAction(action);
		addBatch(batch, 2);
	} else if (brush->isWall()) {
		BatchAction* batch = actionQueue->createBatch(dodraw ? ACTION_DRAW : ACTION_ERASE);
		Action* action = actionQueue->createAction(batch);
		// This will only occur with a size 0, when clicking on a tile (not drawing)
		Tile* tile = map.getTile(offset);
		Tile* new_tile = nullptr;
		if (tile) {
			new_tile = tile->deepCopy(map);
		} else {
			new_tile = map.allocator(map.createTileL(offset));
		}

		if (dodraw) {
			bool b = true;
			brush->asWall()->draw(&map, new_tile, &b);
		} else {
			brush->asWall()->undraw(&map, new_tile);
		}
		action->addChange(newd Change(new_tile));
		batch->addAndCommitAction(action);
		addBatch(batch, 2);
	} else if (brush->isSpawnMonster() || brush->isMonster()) {
		BatchAction* batch = actionQueue->createBatch(dodraw ? ACTION_DRAW : ACTION_ERASE);
		Action* action = actionQueue->createAction(batch);

		Tile* tile = map.getTile(offset);
		Tile* new_tile = nullptr;
		if (tile) {
			new_tile = tile->deepCopy(map);
		} else {
			new_tile = map.allocator(map.createTileL(offset));
		}
		int param;
		if (brush->isMonster()) {
			param = g_gui.GetSpawnMonsterTime();
		} else {
			param = g_gui.GetBrushSize();
		}
		if (dodraw) {
			brush->draw(&map, new_tile, &param);
		} else {
			brush->undraw(&map, new_tile);
		}
		action->addChange(newd Change(new_tile));
		batch->addAndCommitAction(action);
		addBatch(batch, 2);
	} else if (brush->isSpawnNpc() || brush->isNpc()) {
		BatchAction* batch = actionQueue->createBatch(ACTION_DRAW);
		Action* action = actionQueue->createAction(batch);

		Tile* tile = map.getTile(offset);
		Tile* new_tile = nullptr;
		if (tile) {
			new_tile = tile->deepCopy(map);
		} else {
			new_tile = map.allocator(map.createTileL(offset));
		}
		int param;
		if (brush->isNpc()) {
			param = g_gui.GetSpawnNpcTime();
		} else {
			param = g_gui.GetBrushSize();
		}
		if (dodraw) {
			brush->draw(&map, new_tile, &param);
		} else {
			brush->undraw(&map, new_tile);
		}
		action->addChange(newd Change(new_tile));
		batch->addAndCommitAction(action);
		addBatch(batch, 2);
	}
}

void Editor::drawInternal(const PositionVector &tilestodraw, bool alt, bool dodraw) {
	if (!CanEdit()) {
		return;
	}

	Brush* brush = g_gui.GetCurrentBrush();
	if (!brush) {
		return;
	}

#ifdef __DEBUG__
	if (brush->isGround() || brush->isWall()) {
		// Wrong function, end call
		return;
	}
#endif

	Action* action = actionQueue->createAction(dodraw ? ACTION_DRAW : ACTION_ERASE);

	if (brush->isOptionalBorder()) {
		// We actually need to do borders, but on the same tiles we draw to
		for (PositionVector::const_iterator it = tilestodraw.begin(); it != tilestodraw.end(); ++it) {
			TileLocation* location = map.createTileL(*it);
			Tile* tile = location->get();
			if (tile) {
				if (dodraw) {
					Tile* new_tile = tile->deepCopy(map);
					brush->draw(&map, new_tile);
					TileOperations::borderize(new_tile, &map);
					action->addChange(newd Change(new_tile));
				} else if (!dodraw && tile->hasOptionalBorder()) {
					Tile* new_tile = tile->deepCopy(map);
					brush->undraw(&map, new_tile);
					TileOperations::borderize(new_tile, &map);
					action->addChange(newd Change(new_tile));
				}
			} else if (dodraw) {
				Tile* new_tile = map.allocator(location);
				brush->draw(&map, new_tile);
				TileOperations::borderize(new_tile, &map);
				if (new_tile->size() == 0) {
					delete new_tile;
					continue;
				}
				action->addChange(newd Change(new_tile));
			}
		}
	} else {

		for (PositionVector::const_iterator it = tilestodraw.begin(); it != tilestodraw.end(); ++it) {
			TileLocation* location = map.createTileL(*it);
			Tile* tile = location->get();
			if (tile) {
				Tile* new_tile = tile->deepCopy(map);
				if (dodraw) {
					brush->draw(&map, new_tile, &alt);
				} else {
					brush->undraw(&map, new_tile);
				}
				action->addChange(newd Change(new_tile));
			} else if (dodraw) {
				Tile* new_tile = map.allocator(location);
				brush->draw(&map, new_tile, &alt);
				action->addChange(newd Change(new_tile));
			}
		}
	}
	addAction(action, 2);
}

void Editor::drawInternal(const PositionVector &tilestodraw, PositionVector &tilestoborder, bool alt, bool dodraw) {
	if (!CanEdit()) {
		return;
	}

	Brush* brush = g_gui.GetCurrentBrush();
	if (!brush) {
		return;
	}

	if (brush->isGround() || brush->isEraser()) {
		ActionIdentifier identifier = (dodraw && !brush->isEraser()) ? ACTION_DRAW : ACTION_ERASE;
		BatchAction* batch = actionQueue->createBatch(identifier);
		Action* action = actionQueue->createAction(batch);

		for (PositionVector::const_iterator it = tilestodraw.begin(); it != tilestodraw.end(); ++it) {
			TileLocation* location = map.createTileL(*it);
			Tile* tile = location->get();
			if (tile) {
				Tile* new_tile = tile->deepCopy(map);
				if (g_settings.getInteger(Config::USE_AUTOMAGIC)) {
					TileOperations::cleanBorders(new_tile);
				}
				if (dodraw) {
					if (brush->isGround() && alt) {
						std::pair<bool, GroundBrush*> param;
						if (replace_brush) {
							param.first = false;
							param.second = replace_brush;
						} else {
							param.first = true;
							param.second = nullptr;
						}
						g_gui.GetCurrentBrush()->draw(&map, new_tile, &param);
					} else {
						g_gui.GetCurrentBrush()->draw(&map, new_tile, nullptr);
					}
				} else {
					g_gui.GetCurrentBrush()->undraw(&map, new_tile);
					tilestoborder.push_back(*it);
				}
				action->addChange(newd Change(new_tile));
			} else if (dodraw) {
				Tile* new_tile = map.allocator(location);
				if (brush->isGround() && alt) {
					std::pair<bool, GroundBrush*> param;
					if (replace_brush) {
						param.first = false;
						param.second = replace_brush;
					} else {
						param.first = true;
						param.second = nullptr;
					}
					g_gui.GetCurrentBrush()->draw(&map, new_tile, &param);
				} else {
					g_gui.GetCurrentBrush()->draw(&map, new_tile, nullptr);
				}
				action->addChange(newd Change(new_tile));
			}
		}

		// Commit changes to map
		batch->addAndCommitAction(action);

		if (g_settings.getInteger(Config::USE_AUTOMAGIC)) {
			// Do borders!
			action = actionQueue->createAction(batch);
			for (PositionVector::const_iterator it = tilestoborder.begin(); it != tilestoborder.end(); ++it) {
				TileLocation* location = map.createTileL(*it);
				Tile* tile = location->get();
				if (tile) {
					Tile* new_tile = tile->deepCopy(map);
					if (brush->isEraser()) {
						TileOperations::wallize(new_tile, &map);
						TileOperations::tableize(new_tile, &map);
						TileOperations::carpetize(new_tile, &map);
					}
					TileOperations::borderize(new_tile, &map);
					action->addChange(newd Change(new_tile));
				} else {
					Tile* new_tile = map.allocator(location);
					if (brush->isEraser()) {
						// There are no carpets/tables/walls on empty tiles...
						// TileOperations::wallize(new_tile, map);
						// TileOperations::tableize(new_tile, map);
						// TileOperations::carpetize(new_tile, map);
					}
					TileOperations::borderize(new_tile, &map);
					if (new_tile->size() > 0) {
						action->addChange(newd Change(new_tile));
					} else {
						delete new_tile;
					}
				}
			}
			batch->addAndCommitAction(action);
		}

		addBatch(batch, 2);
	} else if (brush->isTable() || brush->isCarpet()) {
		BatchAction* batch = actionQueue->createBatch(ACTION_DRAW);
		Action* action = actionQueue->createAction(batch);

		for (PositionVector::const_iterator it = tilestodraw.begin(); it != tilestodraw.end(); ++it) {
			TileLocation* location = map.createTileL(*it);
			Tile* tile = location->get();
			if (tile) {
				Tile* new_tile = tile->deepCopy(map);
				if (dodraw) {
					g_gui.GetCurrentBrush()->draw(&map, new_tile, nullptr);
				} else {
					g_gui.GetCurrentBrush()->undraw(&map, new_tile);
				}
				action->addChange(newd Change(new_tile));
			} else if (dodraw) {
				Tile* new_tile = map.allocator(location);
				g_gui.GetCurrentBrush()->draw(&map, new_tile, nullptr);
				action->addChange(newd Change(new_tile));
			}
		}

		// Commit changes to map
		batch->addAndCommitAction(action);

		// Do borders!
		action = actionQueue->createAction(batch);
		for (PositionVector::const_iterator it = tilestoborder.begin(); it != tilestoborder.end(); ++it) {
			Tile* tile = map.getTile(*it);
			if (brush->isTable()) {
				if (tile && tile->hasTable()) {
					Tile* new_tile = tile->deepCopy(map);
					TileOperations::tableize(new_tile, &map);
					action->addChange(newd Change(new_tile));
				}
			} else if (brush->isCarpet()) {
				if (tile && tile->hasCarpet()) {
					Tile* new_tile = tile->deepCopy(map);
					TileOperations::carpetize(new_tile, &map);
					action->addChange(newd Change(new_tile));
				}
			}
		}
		batch->addAndCommitAction(action);

		addBatch(batch, 2);
	} else if (brush->isWall()) {
		BatchAction* batch = actionQueue->createBatch(ACTION_DRAW);
		Action* action = actionQueue->createAction(batch);

		if (alt && dodraw) {
			// This is exempt from USE_AUTOMAGIC
			g_gui.doodad_buffer_map->clear();
			BaseMap* draw_map = g_gui.doodad_buffer_map;

			for (PositionVector::const_iterator it = tilestodraw.begin(); it != tilestodraw.end(); ++it) {
				TileLocation* location = map.createTileL(*it);
				Tile* tile = location->get();
				if (tile) {
					Tile* new_tile = tile->deepCopy(map);
					TileOperations::cleanWalls(new_tile, brush->isWall());
					g_gui.GetCurrentBrush()->draw(draw_map, new_tile);
					draw_map->setTile(*it, new_tile, true);
				} else if (dodraw) {
					Tile* new_tile = map.allocator(location);
					g_gui.GetCurrentBrush()->draw(draw_map, new_tile);
					draw_map->setTile(*it, new_tile, true);
				}
			}
			for (PositionVector::const_iterator it = tilestodraw.begin(); it != tilestodraw.end(); ++it) {
				// Get the correct tiles from the draw map instead of the editor map
				Tile* tile = draw_map->getTile(*it);
				if (tile) {
					TileOperations::wallize(tile, draw_map);
					action->addChange(newd Change(tile));
				}
			}
			draw_map->clear(false);
			// Commit
			batch->addAndCommitAction(action);
		} else {
			for (PositionVector::const_iterator it = tilestodraw.begin(); it != tilestodraw.end(); ++it) {
				TileLocation* location = map.createTileL(*it);
				Tile* tile = location->get();
				if (tile) {
					Tile* new_tile = tile->deepCopy(map);
					// Wall cleaning is exempt from automagic
					TileOperations::cleanWalls(new_tile, brush->isWall());
					if (dodraw) {
						g_gui.GetCurrentBrush()->draw(&map, new_tile);
					} else {
						g_gui.GetCurrentBrush()->undraw(&map, new_tile);
					}
					action->addChange(newd Change(new_tile));
				} else if (dodraw) {
					Tile* new_tile = map.allocator(location);
					g_gui.GetCurrentBrush()->draw(&map, new_tile);
					action->addChange(newd Change(new_tile));
				}
			}

			// Commit changes to map
			batch->addAndCommitAction(action);

			if (g_settings.getInteger(Config::USE_AUTOMAGIC)) {
				// Do borders!
				action = actionQueue->createAction(batch);
				for (PositionVector::const_iterator it = tilestoborder.begin(); it != tilestoborder.end(); ++it) {
					Tile* tile = map.getTile(*it);
					if (tile) {
						Tile* new_tile = tile->deepCopy(map);
						TileOperations::wallize(new_tile, &map);
						// if(*tile == *new_tile) delete new_tile;
						action->addChange(newd Change(new_tile));
					}
				}
				batch->addAndCommitAction(action);
			}
		}

		actionQueue->addBatch(batch, 2);
	} else if (brush->isDoor()) {
		BatchAction* batch = actionQueue->createBatch(ACTION_DRAW);
		Action* action = actionQueue->createAction(batch);
		DoorBrush* door_brush = brush->asDoor();

		// Loop is kind of redundant since there will only ever be one index.
		for (PositionVector::const_iterator it = tilestodraw.begin(); it != tilestodraw.end(); ++it) {
			TileLocation* location = map.createTileL(*it);
			Tile* tile = location->get();
			if (tile) {
				Tile* new_tile = tile->deepCopy(map);
				// Wall cleaning is exempt from automagic
				if (brush->isWall()) {
					TileOperations::cleanWalls(new_tile, brush->asWall());
				}
				if (dodraw) {
					door_brush->draw(&map, new_tile, &alt);
				} else {
					door_brush->undraw(&map, new_tile);
				}
				action->addChange(newd Change(new_tile));
			} else if (dodraw) {
				Tile* new_tile = map.allocator(location);
				door_brush->draw(&map, new_tile, &alt);
				action->addChange(newd Change(new_tile));
			}
		}

		// Commit changes to map
		batch->addAndCommitAction(action);

		if (g_settings.getInteger(Config::USE_AUTOMAGIC)) {
			// Do borders!
			action = actionQueue->createAction(batch);
			for (PositionVector::const_iterator it = tilestoborder.begin(); it != tilestoborder.end(); ++it) {
				Tile* tile = map.getTile(*it);
				if (tile) {
					Tile* new_tile = tile->deepCopy(map);
					TileOperations::wallize(new_tile, &map);
					// if(*tile == *new_tile) delete new_tile;
					action->addChange(newd Change(new_tile));
				}
			}
			batch->addAndCommitAction(action);
		}

		addBatch(batch, 2);
	} else {
		Action* action = actionQueue->createAction(ACTION_DRAW);
		for (PositionVector::const_iterator it = tilestodraw.begin(); it != tilestodraw.end(); ++it) {
			TileLocation* location = map.createTileL(*it);
			Tile* tile = location->get();
			if (tile) {
				Tile* new_tile = tile->deepCopy(map);
				if (dodraw) {
					g_gui.GetCurrentBrush()->draw(&map, new_tile);
				} else {
					g_gui.GetCurrentBrush()->undraw(&map, new_tile);
				}
				action->addChange(newd Change(new_tile));
			} else if (dodraw) {
				Tile* new_tile = map.allocator(location);
				g_gui.GetCurrentBrush()->draw(&map, new_tile);
				action->addChange(newd Change(new_tile));
			}
		}
		addAction(action, 2);
	}
}
