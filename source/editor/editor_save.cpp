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
#include <fmt/chrono.h>
#include <iostream>

namespace fs = std::filesystem;

void Editor::saveMap(FileName filename, bool showdialog) {
	std::string savefile = filename.GetFullPath().mb_str(wxConvUTF8).data();
	bool save_as = false;
	bool save_otgz = false;

	if (savefile.empty()) {
		savefile = map.filename;

		FileName c1(wxstr(savefile));
		FileName c2(wxstr(map.filename));
		save_as = c1 != c2;
	}

	// If not named yet, propagate the file name to the auxilliary files
	if (map.unnamed) {
		FileName _name(filename);
		_name.SetExt("xml");

		_name.SetName(filename.GetName() + "-monster");
		map.spawnmonsterfile = nstr(_name.GetFullName());
		_name.SetName(filename.GetName() + "-npc");
		map.spawnnpcfile = nstr(_name.GetFullName());
		_name.SetName(filename.GetName() + "-house");
		map.housefile = nstr(_name.GetFullName());
		_name.SetName(filename.GetName() + "-zones");
		map.zonefile = nstr(_name.GetFullName());

		map.unnamed = false;
	}

	// File object to convert between local paths etc.
	FileName converter;
	converter.Assign(wxstr(savefile));
	std::string map_path = nstr(converter.GetPath(wxPATH_GET_SEPARATOR | wxPATH_GET_VOLUME));

	// Make temporary backups
	// converter.Assign(wxstr(savefile));
	std::string backup_otbm, backup_house, backup_spawn, backup_spawn_npc, backup_zones;

	if (converter.GetExt() == "otgz") {
		save_otgz = true;
		if (converter.FileExists()) {
			backup_otbm = map_path + nstr(converter.GetName()) + ".otgz~";
			std::remove(backup_otbm.c_str());
			std::rename(savefile.c_str(), backup_otbm.c_str());
		}
	} else {
		if (converter.FileExists()) {
			backup_otbm = map_path + nstr(converter.GetName()) + ".otbm~";
			std::remove(backup_otbm.c_str());
			std::rename(savefile.c_str(), backup_otbm.c_str());
		}

		converter.SetFullName(wxstr(map.housefile));
		if (converter.FileExists()) {
			backup_house = map_path + nstr(converter.GetName()) + ".xml~";
			std::remove(backup_house.c_str());
			std::rename((map_path + map.housefile).c_str(), backup_house.c_str());
		}

		converter.SetFullName(wxstr(map.spawnmonsterfile));
		if (converter.FileExists()) {
			backup_spawn = map_path + nstr(converter.GetName()) + ".xml~";
			std::remove(backup_spawn.c_str());
			std::rename((map_path + map.spawnmonsterfile).c_str(), backup_spawn.c_str());
		}

		converter.SetFullName(wxstr(map.spawnnpcfile));
		if (converter.FileExists()) {
			backup_spawn_npc = map_path + nstr(converter.GetName()) + ".xml~";
			std::remove(backup_spawn_npc.c_str());
			std::rename((map_path + map.spawnnpcfile).c_str(), backup_spawn_npc.c_str());
		}

		converter.SetFullName(wxstr(map.zonefile));
		if (converter.FileExists()) {
			backup_zones = map_path + nstr(converter.GetName()) + ".xml~";
			std::remove(backup_zones.c_str());
			std::rename((map_path + map.zonefile).c_str(), backup_zones.c_str());
		}
	}

	// Save the map
	{
		std::string n = nstr(g_gui.GetLocalDataDirectory()) + ".saving.txt";
		std::ofstream f(n.c_str(), std::ios::trunc | std::ios::out);
		f << backup_otbm << std::endl
		  << backup_house << std::endl
		  << backup_spawn << std::endl
		  << backup_spawn_npc << std::endl;
	}

	{

		// Set up the Map paths
		wxFileName fn = wxstr(savefile);
		map.filename = fn.GetFullPath().mb_str(wxConvUTF8);
		map.name = fn.GetFullName().mb_str(wxConvUTF8);

		if (showdialog) {
			g_gui.CreateLoadBar("Saving OTBM map...");
		}

		// Perform the actual save
		IOMapOTBM mapsaver(map.getVersion());
		bool success = mapsaver.saveMap(map, fn);

		if (showdialog) {
			g_gui.DestroyLoadBar();
		}

		// Check for errors...
		if (!success) {
			// Rename the temporary backup files back to their previous names
			if (!backup_otbm.empty()) {
				converter.SetFullName(wxstr(savefile));
				std::string otbm_filename = map_path + nstr(converter.GetName());
				std::rename(backup_otbm.c_str(), std::string(otbm_filename + (save_otgz ? ".otgz" : ".otbm")).c_str());
			}

			if (!backup_house.empty()) {
				converter.SetFullName(wxstr(map.housefile));
				std::string house_filename = map_path + nstr(converter.GetName());
				std::rename(backup_house.c_str(), std::string(house_filename + ".xml").c_str());
			}

			if (!backup_spawn.empty()) {
				converter.SetFullName(wxstr(map.spawnmonsterfile));
				std::string spawn_filename = map_path + nstr(converter.GetName());
				std::rename(backup_spawn.c_str(), std::string(spawn_filename + ".xml").c_str());
			}

			if (!backup_spawn_npc.empty()) {
				converter.SetFullName(wxstr(map.spawnnpcfile));
				std::string spawnnpc_filename = map_path + nstr(converter.GetName());
				std::rename(backup_spawn_npc.c_str(), std::string(spawnnpc_filename + ".xml").c_str());
			}

			if (!backup_zones.empty()) {
				converter.SetFullName(wxstr(map.zonefile));
				std::string zones_filename = map_path + nstr(converter.GetName());
				std::rename(backup_zones.c_str(), std::string(zones_filename + ".xml").c_str());
			}

			// Display the error
			g_gui.PopupDialog("Error", "Could not save, unable to open target for writing.", wxOK);
		}

		// Remove temporary save runfile
		{
			std::string n = nstr(g_gui.GetLocalDataDirectory()) + ".saving.txt";
			std::remove(n.c_str());
		}

		// If failure, don't run the rest of the function
		if (!success) {
			return;
		}
	}

	// Move to permanent backup
	if (!save_as && g_settings.getInteger(Config::ALWAYS_MAKE_BACKUP)) {
		std::string backup_path = map_path + "backups/";
		ensureBackupDirectoryExists(backup_path);
		// Move temporary backups to their proper files
		time_t t = time(nullptr);
		auto date = fmt::format("{:%Y-%m-%d-%H-%M-%S}", fmt::localtime(std::time(nullptr)));

		if (!backup_otbm.empty()) {
			converter.SetFullName(wxstr(savefile));
			std::string otbm_filename = backup_path + nstr(converter.GetName());
			std::rename(backup_otbm.c_str(), (otbm_filename + "." + date + (save_otgz ? ".otgz" : ".otbm")).c_str());
		}

		if (!backup_house.empty()) {
			converter.SetFullName(wxstr(map.housefile));
			std::string house_filename = backup_path + nstr(converter.GetName());
			std::rename(backup_house.c_str(), (house_filename + "." + date + ".xml").c_str());
		}

		if (!backup_spawn.empty()) {
			converter.SetFullName(wxstr(map.spawnmonsterfile));
			std::string spawn_filename = backup_path + nstr(converter.GetName());
			std::rename(backup_spawn.c_str(), (spawn_filename + "." + date + ".xml").c_str());
		}

		if (!backup_spawn_npc.empty()) {
			converter.SetFullName(wxstr(map.spawnnpcfile));
			std::string spawnnpc_filename = backup_path + nstr(converter.GetName());
			std::rename(backup_spawn_npc.c_str(), (spawnnpc_filename + "." + date + ".xml").c_str());
		}

		if (!backup_zones.empty()) {
			converter.SetFullName(wxstr(map.zonefile));
			std::string zones_filename = backup_path + nstr(converter.GetName());
			std::rename(backup_zones.c_str(), (zones_filename + "." + date + ".xml").c_str());
		}
	} else {
		// Delete the temporary files
		std::remove(backup_otbm.c_str());
		std::remove(backup_house.c_str());
		std::remove(backup_spawn.c_str());
		std::remove(backup_spawn_npc.c_str());
		std::remove(backup_zones.c_str());
	}

	deleteOldBackups(map_path + "backups/");

	clearChanges();
}

void Editor::deleteOldBackups(const std::string &backup_path) {
	int days_to_delete = g_settings.getInteger(Config::DELETE_BACKUP_DAYS);
	if (days_to_delete <= 0) {
		return; // Se o valor é zero ou negativo, não deletar backups
	}

	try {
		auto now = std::chrono::system_clock::now();
		for (const auto &entry : fs::directory_iterator(backup_path)) {
			if (fs::is_regular_file(entry.status())) {
				auto file_time = fs::last_write_time(entry);
				auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(file_time - fs::file_time_type::clock::now() + now);
				auto file_age = std::chrono::duration_cast<std::chrono::hours>(now - sctp).count() / 24;

				if (file_age > days_to_delete) {
					fs::remove(entry);
					std::cout << "Deleted old backup: " << entry.path() << std::endl;
				}
			}
		}
	} catch (const fs::filesystem_error &e) {
		std::cerr << "Error: " << e.what() << std::endl;
	}
}
