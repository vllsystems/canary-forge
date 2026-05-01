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

Editor::Editor(CopyBuffer &copybuffer) :
	live_server(nullptr),
	live_client(nullptr),
	actionQueue(newd ActionQueue(*this)),
	selection(*this),
	copybuffer(copybuffer),
	replace_brush(nullptr) {
	wxString error;
	wxArrayString warnings;
	if (!g_gui.loadMapWindow(error, warnings)) {
		g_gui.PopupDialog("Error", error, wxOK);
		g_gui.ListDialog("Warnings", warnings);
	}

	MapVersion version;
	map.convert(version);

	map.height = 2048;
	map.width = 2048;

	static int unnamed_counter = 0;

	std::string sname = "Untitled-" + i2s(++unnamed_counter);
	map.name = sname + ".otbm";
	map.spawnmonsterfile = sname + "-monster.xml";
	map.spawnnpcfile = sname + "-npc.xml";
	map.housefile = sname + "-house.xml";
	map.zonefile = sname + "-zones.xml";
	map.description = "No map description available.";
	map.unnamed = true;

	map.doChange();
}

// Used for loading a new map from "open map" menu
Editor::Editor(CopyBuffer &copybuffer, const FileName &fn) :
	live_server(nullptr),
	live_client(nullptr),
	actionQueue(newd ActionQueue(*this)),
	selection(*this),
	copybuffer(copybuffer),
	replace_brush(nullptr) {
	MapVersion ver;
	if (!IOMapOTBM::getVersionInfo(fn, ver)) {
		spdlog::error("Could not open file {}. This is not a valid OTBM file or it does not exist.", nstr(fn.GetFullPath()));
		throw std::runtime_error("Could not open file \"" + nstr(fn.GetFullPath()) + "\".\nThis is not a valid OTBM file or it does not exist.");
	}

	if (ver.otbm != g_gui.getLoadedMapVersion().otbm) {
		auto result = g_gui.PopupDialog("Map error", "The loaded map appears to be a OTBM format that is not supported by the editor. Do you still want to attempt to load the map? Caution: this will close your current map!", wxYES | wxNO);
		if (result == wxID_YES) {
			if (!g_gui.CloseAllEditors()) {
				spdlog::error("All maps of different versions were not closed.");
				throw std::runtime_error("All maps of different versions were not closed.");
			}
		} else if (result == wxID_NO) {
			throw std::runtime_error("Maps of different versions can't be loaded at same time. Save and close your current map and try again.");
		}
	}

	bool success = true;
	wxString error;
	wxArrayString warnings;

	success = g_gui.loadMapWindow(error, warnings);
	if (!success) {
		g_gui.PopupDialog("Error", error, wxOK);
		auto clientDirectory = ClientAssets::getPath().ToStdString() + "/";
		if (!wxDirExists(wxString(clientDirectory))) {
			PreferencesWindow dialog(nullptr);
			dialog.getBookCtrl().SetSelection(4);
			dialog.ShowModal();
			dialog.Destroy();
		}
	} else {
		g_gui.ListDialog("Warnings", warnings);
	}

	if (success) {
		ScopedLoadingBar LoadingBar("Loading OTBM map...");
		success = map.open(nstr(fn.GetFullPath()));
	}
}

Editor::Editor(CopyBuffer &copybuffer, LiveClient* client) :
	live_server(nullptr),
	live_client(client),
	actionQueue(newd NetworkedActionQueue(*this)),
	selection(*this),
	copybuffer(copybuffer),
	replace_brush(nullptr) { }

Editor::~Editor() {
	delete actionQueue;
}

Action* Editor::createAction(ActionIdentifier type) {
	return actionQueue->createAction(type);
}

Action* Editor::createAction(BatchAction* parent) {
	return actionQueue->createAction(parent);
}

BatchAction* Editor::createBatch(ActionIdentifier type) {
	return actionQueue->createBatch(type);
}

void Editor::addBatch(BatchAction* action, int stacking_delay) {
	actionQueue->addBatch(action, stacking_delay);
}

void Editor::addAction(Action* action, int stacking_delay) {
	actionQueue->addAction(action, stacking_delay);
}

bool Editor::canUndo() const {
	return actionQueue->canUndo();
}

bool Editor::canRedo() const {
	return actionQueue->canRedo();
}

void Editor::undo(int indexes) {
	if (indexes <= 0 || !actionQueue->canUndo()) {
		return;
	}

	while (indexes > 0) {
		if (!actionQueue->undo()) {
			break;
		}
		indexes--;
	}
	g_gui.UpdateActions();
	g_gui.RefreshView();
}

void Editor::redo(int indexes) {
	if (indexes <= 0 || !actionQueue->canRedo()) {
		return;
	}

	while (indexes > 0) {
		if (!actionQueue->redo()) {
			break;
		}
		indexes--;
	}
	g_gui.UpdateActions();
	g_gui.RefreshView();
}

void Editor::updateActions() {
	actionQueue->generateLabels();
	g_gui.UpdateMenus();
	g_gui.UpdateActions();
}

void Editor::resetActionsTimer() {
	actionQueue->resetTimer();
}

void Editor::clearActions() {
	actionQueue->clear();
	g_gui.UpdateActions();
}

bool Editor::hasChanges() const {
	if (map.hasChanged()) {
		if (map.getTileCount() == 0) {
			return actionQueue->hasChanges();
		}
		return true;
	}
	return false;
}

void Editor::clearChanges() {
	map.clearChanges();
}

void Editor::borderizeSelection() {
	if (selection.empty()) {
		g_gui.SetStatusText("No items selected. Can't borderize.");
		return;
	}

	Action* action = actionQueue->createAction(ACTION_BORDERIZE);
	for (const Tile* tile : selection) {
		Tile* new_tile = tile->deepCopy(map);
		TileOperations::borderize(new_tile, &map);
		new_tile->select();
		action->addChange(new Change(new_tile));
	}
	addAction(action);
	updateActions();
}

void Editor::borderizeMap(bool showdialog) {
	if (showdialog) {
		g_gui.CreateLoadBar("Borderizing map...");
	}

	uint64_t tiles_done = 0;
	for (TileLocation* tileLocation : map) {
		if (showdialog && tiles_done % 4096 == 0) {
			g_gui.SetLoadDone(static_cast<int32_t>(tiles_done / double(map.tilecount) * 100.0));
		}

		Tile* tile = tileLocation->get();
		ASSERT(tile);

		TileOperations::borderize(tile, &map);
		++tiles_done;
	}

	if (showdialog) {
		g_gui.DestroyLoadBar();
	}
}

void Editor::randomizeSelection() {
	if (selection.empty()) {
		g_gui.SetStatusText("No items selected. Can't randomize.");
		return;
	}

	Action* action = actionQueue->createAction(ACTION_RANDOMIZE);
	for (const Tile* tile : selection) {
		Tile* new_tile = tile->deepCopy(map);
		GroundBrush* brush = new_tile->getGroundBrush();
		if (brush && brush->isReRandomizable()) {
			brush->draw(&map, new_tile, nullptr);

			Item* old_ground = tile->ground;
			Item* new_ground = new_tile->ground;
			if (old_ground && new_ground) {
				new_ground->setActionID(old_ground->getActionID());
				new_ground->setUniqueID(old_ground->getUniqueID());
			}

			new_tile->select();
			action->addChange(new Change(new_tile));
		}
	}
	addAction(action);
	updateActions();
}

void Editor::randomizeMap(bool showdialog) {
	if (showdialog) {
		g_gui.CreateLoadBar("Randomizing map...");
	}

	uint64_t tiles_done = 0;
	for (TileLocation* tileLocation : map) {
		if (showdialog && tiles_done % 4096 == 0) {
			g_gui.SetLoadDone(static_cast<int32_t>(tiles_done / double(map.tilecount) * 100.0));
		}

		Tile* tile = tileLocation->get();
		ASSERT(tile);

		GroundBrush* groundBrush = tile->getGroundBrush();
		if (groundBrush) {
			Item* oldGround = tile->ground;

			uint16_t actionId, uniqueId;
			if (oldGround) {
				actionId = oldGround->getActionID();
				uniqueId = oldGround->getUniqueID();
			} else {
				actionId = 0;
				uniqueId = 0;
			}
			groundBrush->draw(&map, tile, nullptr);

			Item* newGround = tile->ground;
			if (newGround) {
				newGround->setActionID(actionId);
				newGround->setUniqueID(uniqueId);
			}
			tile->update();
		}
		++tiles_done;
	}

	if (showdialog) {
		g_gui.DestroyLoadBar();
	}
}

void Editor::clearInvalidHouseTiles(bool showdialog) {
	if (showdialog) {
		g_gui.CreateLoadBar("Clearing invalid house tiles...");
	}

	Houses &houses = map.houses;

	HouseMap::iterator iter = houses.begin();
	while (iter != houses.end()) {
		House* h = iter->second;
		if (map.towns.getTown(h->townid) == nullptr) {
			iter = houses.erase(iter);
		} else {
			++iter;
		}
	}

	uint64_t tiles_done = 0;
	for (MapIterator map_iter = map.begin(); map_iter != map.end(); ++map_iter) {
		if (showdialog && tiles_done % 4096 == 0) {
			g_gui.SetLoadDone(int(tiles_done / double(map.tilecount) * 100.0));
		}

		Tile* tile = (*map_iter)->get();
		ASSERT(tile);
		if (tile->isHouseTile()) {
			if (houses.getHouse(tile->getHouseID()) == nullptr) {
				tile->setHouse(nullptr);
			}
		}
		++tiles_done;
	}

	if (showdialog) {
		g_gui.DestroyLoadBar();
	}
}

void Editor::clearModifiedTileState(bool showdialog) {
	if (showdialog) {
		g_gui.CreateLoadBar("Clearing modified state from all tiles...");
	}

	uint64_t tiles_done = 0;
	for (MapIterator map_iter = map.begin(); map_iter != map.end(); ++map_iter) {
		if (showdialog && tiles_done % 4096 == 0) {
			g_gui.SetLoadDone(int(tiles_done / double(map.tilecount) * 100.0));
		}

		Tile* tile = (*map_iter)->get();
		ASSERT(tile);
		tile->unmodify();
		++tiles_done;
	}

	if (showdialog) {
		g_gui.DestroyLoadBar();
	}
}

///////////////////////////////////////////////////////////////////////////////
// Live!

bool Editor::IsLiveClient() const {
	return live_client != nullptr;
}

bool Editor::IsLiveServer() const {
	return live_server != nullptr;
}

bool Editor::IsLive() const {
	return IsLiveClient() || IsLiveServer();
}

bool Editor::IsLocal() const {
	return !IsLive();
}

LiveClient* Editor::GetLiveClient() const {
	return live_client;
}

LiveServer* Editor::GetLiveServer() const {
	return live_server;
}

LiveSocket &Editor::GetLive() const {
	if (live_server) {
		return *live_server;
	}
	return *live_client;
}

LiveServer* Editor::StartLiveServer() {
	ASSERT(IsLocal());
	live_server = newd LiveServer(*this);

	delete actionQueue;
	actionQueue = newd NetworkedActionQueue(*this);

	return live_server;
}

void Editor::BroadcastNodes(DirtyList &dirtyList) {
	if (IsLiveClient()) {
		live_client->sendChanges(dirtyList);
	} else {
		live_server->broadcastNodes(dirtyList);
	}
}

void Editor::CloseLiveServer() {
	ASSERT(IsLive());
	if (live_client) {
		live_client->close();

		delete live_client;
		live_client = nullptr;
	}

	if (live_server) {
		live_server->close();

		delete live_server;
		live_server = nullptr;

		delete actionQueue;
		actionQueue = newd ActionQueue(*this);
	}

	NetworkConnection &connection = NetworkConnection::getInstance();
	connection.stop();
}

void Editor::QueryNode(int ndx, int ndy, bool underground) {
	ASSERT(live_client);
	live_client->queryNode(ndx, ndy, underground);
}

void Editor::SendNodeRequests() {
	if (live_client) {
		live_client->sendNodeRequests();
	}
}
