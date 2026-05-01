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

#ifndef RME_OTBM_MAP_IO_H_
#define RME_OTBM_MAP_IO_H_

#include "io/iomap.h"
#include "io/otbm/otbm_types.h"

struct MapVersion;
class NodeFileReadHandle;
class NodeFileWriteHandle;
class Map;

class IOMapOTBM : public IOMap {
public:
	IOMapOTBM(MapVersion ver);
	~IOMapOTBM() { }

	static bool getVersionInfo(const FileName &identifier, MapVersion &out_ver);

	virtual bool loadMap(Map &map, const FileName &identifier);
	virtual bool saveMap(Map &map, const FileName &identifier);

protected:
	static bool getVersionInfo(NodeFileReadHandle* f, MapVersion &out_ver);

	virtual bool loadMap(Map &map, NodeFileReadHandle &handle);
	bool loadSpawnsMonster(Map &map, const FileName &dir);
	bool loadSpawnsMonster(Map &map, pugi::xml_document &doc);
	bool loadHouses(Map &map, const FileName &dir);
	bool loadHouses(Map &map, pugi::xml_document &doc);
	bool loadSpawnsNpc(Map &map, const FileName &dir);
	bool loadSpawnsNpc(Map &map, pugi::xml_document &doc);
	bool loadZones(Map &map, const FileName &dir);
	bool loadZones(Map &map, pugi::xml_document &doc);

	virtual bool saveMap(Map &map, NodeFileWriteHandle &handle);
	bool saveSpawns(Map &map, const FileName &dir);
	bool saveSpawns(Map &map, pugi::xml_document &doc);
	bool saveHouses(Map &map, const FileName &dir);
	bool saveHouses(Map &map, pugi::xml_document &doc);
	bool saveSpawnsNpc(Map &map, const FileName &dir);
	bool saveSpawnsNpc(Map &map, pugi::xml_document &doc);
	bool saveZones(Map &map, const FileName &dir);
	bool saveZones(Map &map, pugi::xml_document &doc);
};

#endif
