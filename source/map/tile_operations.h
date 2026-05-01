//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_TILE_OPERATIONS_H
#define RME_TILE_OPERATIONS_H

class Tile;
class BaseMap;
class Item;
class WallBrush;

namespace TileOperations {
	void borderize(Tile* tile, BaseMap* parent);
	void addBorderItem(Tile* tile, Item* item);
	void cleanBorders(Tile* tile);

	void wallize(Tile* tile, BaseMap* parent);
	void addWallItem(Tile* tile, Item* item);
	void cleanWalls(Tile* tile, bool dontdelete = false);
	void cleanWalls(Tile* tile, WallBrush* brush);

	void cleanTables(Tile* tile, bool dontdelete = false);
	void tableize(Tile* tile, BaseMap* parent);

	void carpetize(Tile* tile, BaseMap* parent);
}

#endif
