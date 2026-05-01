//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "app/main.h"

#include "map/tile_operations.h"
#include "map/tile.h"
#include "map/basemap.h"
#include "game/item.h"
#include "brushes/ground_brush.h"
#include "brushes/wall_brush.h"
#include "brushes/carpet_brush.h"
#include "brushes/table_brush.h"

void TileOperations::borderize(Tile* tile, BaseMap* parent) {
	GroundBrush::doBorders(parent, tile);
}

void TileOperations::addBorderItem(Tile* tile, Item* item) {
	if (!item) {
		return;
	}
	ASSERT(item->isBorder());
	tile->items.insert(tile->items.begin(), item);
}

void TileOperations::cleanBorders(Tile* tile) {
	if (tile->items.empty()) {
		return;
	}

	for (auto it = tile->items.begin(); it != tile->items.end();) {
		Item* item = (*it);
		if (!item->isBorder()) {
			break;
		}

		delete item;
		it = tile->items.erase(it);
	}
}

void TileOperations::wallize(Tile* tile, BaseMap* parent) {
	WallBrush::doWalls(parent, tile);
}

void TileOperations::addWallItem(Tile* tile, Item* item) {
	if (!item) {
		return;
	}
	ASSERT(item->isWall());
	tile->addItem(item);
}

void TileOperations::cleanWalls(Tile* tile, bool dontdelete) {
	if (tile->items.empty()) {
		return;
	}

	for (auto it = tile->items.begin(); it != tile->items.end();) {
		Item* item = (*it);
		if (item && item->isWall()) {
			if (!dontdelete) {
				delete item;
			}
			it = tile->items.erase(it);
		} else {
			++it;
		}
	}
}

void TileOperations::cleanWalls(Tile* tile, WallBrush* brush) {
	for (auto it = tile->items.begin(); it != tile->items.end();) {
		Item* item = (*it);
		if (item && item->isWall() && brush->hasWall(item)) {
			delete item;
			it = tile->items.erase(it);
		} else {
			++it;
		}
	}
}

void TileOperations::cleanTables(Tile* tile, bool dontdelete) {
	if (tile->items.empty()) {
		return;
	}

	for (auto it = tile->items.begin(); it != tile->items.end();) {
		Item* item = (*it);
		if (item && item->isTable()) {
			if (!dontdelete) {
				delete item;
			}
			it = tile->items.erase(it);
		} else {
			++it;
		}
	}
}

void TileOperations::tableize(Tile* tile, BaseMap* parent) {
	TableBrush::doTables(parent, tile);
}

void TileOperations::carpetize(Tile* tile, BaseMap* parent) {
	CarpetBrush::doCarpets(parent, tile);
}
