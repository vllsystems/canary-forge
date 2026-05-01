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
#include <wx/mstream.h>
#include <wx/display.h>
#include <wx/stopwatch.h>
#include <wx/clipbrd.h>
#include <wx/progdlg.h>

#include "ui/gui.h"

#include "app/application.h"
#include "client_assets.h"
#include "ui/menubar/main_menubar.h"

#include "editor/editor.h"
#include "brushes/brush.h"
#include "map/map.h"
#include "rendering/sprites.h"
#include "game/materials.h"
#include "brushes/doodad_brush.h"
#include "brushes/spawn_monster_brush.h"

#include "ui/dialogs/common_windows.h"
#include "ui/windows/result_window.h"
#include "ui/windows/minimap_window.h"
#include "ui/palette/palette_window.h"
#include "rendering/map_display.h"
#include "app/application.h"
#include "ui/windows/welcome_dialog.h"
#include "brushes/spawn_npc_brush.h"
#include "ui/windows/actions_history_window.h"
#include "lua/lua_scripts_window.h"
#include "rendering/sprite_appearances.h"
#include "ui/dialogs/preferences.h"

#include "live/live_client.h"
#include "live/live_tab.h"
#include "live/live_server.h"

#ifdef __WXOSX__
	#include <AGL/agl.h>
#endif

#include <appearances.pb.h>

#include "ui/gui.h"
#include "app/application.h"
#include "editor/editor.h"
#include "map/map.h"
#include "rendering/map_display.h"
#include "ui/palette/palette_window.h"
#include "ui/map_tab.h"
#include "brushes/brush.h"
#include "brushes/doodad_brush.h"
#include "game/materials.h"
#include "game/items.h"
#include "game/monsters.h"
#include "game/npcs.h"
#include "rendering/sprites.h"
#include "client_assets.h"
#include "rendering/sprite_appearances.h"
#include "ui/windows/minimap_window.h"
#include "ui/windows/actions_history_window.h"
#include "live/live_server.h"
#include "ui/windows/welcome_dialog.h"

void GUI::SwitchMode() {
	if (mode == DRAWING_MODE) {
		SetSelectionMode();
	} else {
		SetDrawingMode();
	}
}

void GUI::SetSelectionMode() {
	if (mode == SELECTION_MODE) {
		return;
	}

	if (current_brush && current_brush->isDoodad()) {
		secondary_map = nullptr;
	}

	tabbook->OnSwitchEditorMode(SELECTION_MODE);
	mode = SELECTION_MODE;
}

void GUI::SetDrawingMode() {
	if (mode == DRAWING_MODE) {
		return;
	}

	std::set<MapTab*> al;
	for (int idx = 0; idx < tabbook->GetTabCount(); ++idx) {
		EditorTab* editorTab = tabbook->GetTab(idx);
		if (MapTab* mapTab = dynamic_cast<MapTab*>(editorTab)) {
			if (al.find(mapTab) != al.end()) {
				continue;
			}

			Editor* editor = mapTab->GetEditor();
			Selection &selection = editor->getSelection();
			selection.start(Selection::NONE, ACTION_UNSELECT);
			selection.clear();
			selection.finish();
			selection.updateSelectionCount();
			al.insert(mapTab);
		}
	}

	if (current_brush && current_brush->isDoodad()) {
		secondary_map = doodad_buffer_map;
	} else {
		secondary_map = nullptr;
	}

	tabbook->OnSwitchEditorMode(DRAWING_MODE);
	mode = DRAWING_MODE;
}

void GUI::SetBrushSizeInternal(int nz) {
	if (nz != brush_size && current_brush && current_brush->isDoodad() && !current_brush->oneSizeFitsAll()) {
		brush_size = nz;
		FillDoodadPreviewBuffer();
		secondary_map = doodad_buffer_map;
	} else {
		brush_size = nz;
	}
}

void GUI::SetBrushSize(int nz) {
	SetBrushSizeInternal(nz);

	for (auto &palette : palettes) {
		palette->OnUpdateBrushSize(brush_shape, brush_size);
	}

	root->GetAuiToolBar()->UpdateBrushSize(brush_shape, brush_size);
}

void GUI::SetBrushVariation(int nz) {
	if (nz != brush_variation && current_brush && current_brush->isDoodad()) {
		// Monkey!
		brush_variation = nz;
		FillDoodadPreviewBuffer();
		secondary_map = doodad_buffer_map;
	}
}

void GUI::SetBrushShape(BrushShape bs) {
	if (bs != brush_shape && current_brush && current_brush->isDoodad() && !current_brush->oneSizeFitsAll()) {
		// Donkey!
		brush_shape = bs;
		FillDoodadPreviewBuffer();
		secondary_map = doodad_buffer_map;
	}
	brush_shape = bs;

	for (auto &palette : palettes) {
		palette->OnUpdateBrushSize(brush_shape, brush_size);
	}

	root->GetAuiToolBar()->UpdateBrushSize(brush_shape, brush_size);
}

void GUI::SetBrushThickness(bool on, int x, int y) {
	use_custom_thickness = on;

	if (x != -1 || y != -1) {
		custom_thickness_mod = std::max<float>(x, 1.f) / std::max<float>(y, 1.f);
	}

	if (current_brush && current_brush->isDoodad()) {
		FillDoodadPreviewBuffer();
	}

	RefreshView();
}

void GUI::SetBrushThickness(int low, int ceil) {
	custom_thickness_mod = std::max<float>(low, 1.f) / std::max<float>(ceil, 1.f);

	if (use_custom_thickness && current_brush && current_brush->isDoodad()) {
		FillDoodadPreviewBuffer();
	}

	RefreshView();
}

void GUI::DecreaseBrushSize(bool wrap) {
	switch (brush_size) {
		case 0: {
			if (wrap) {
				SetBrushSize(11);
			}
			break;
		}
		case 1: {
			SetBrushSize(0);
			break;
		}
		case 2:
		case 3: {
			SetBrushSize(1);
			break;
		}
		case 4:
		case 5: {
			SetBrushSize(2);
			break;
		}
		case 6:
		case 7: {
			SetBrushSize(4);
			break;
		}
		case 8:
		case 9:
		case 10: {
			SetBrushSize(6);
			break;
		}
		case 11:
		default: {
			SetBrushSize(8);
			break;
		}
	}
}

void GUI::IncreaseBrushSize(bool wrap) {
	switch (brush_size) {
		case 0: {
			SetBrushSize(1);
			break;
		}
		case 1: {
			SetBrushSize(2);
			break;
		}
		case 2:
		case 3: {
			SetBrushSize(4);
			break;
		}
		case 4:
		case 5: {
			SetBrushSize(6);
			break;
		}
		case 6:
		case 7: {
			SetBrushSize(8);
			break;
		}
		case 8:
		case 9:
		case 10: {
			SetBrushSize(11);
			break;
		}
		case 11:
		default: {
			if (wrap) {
				SetBrushSize(0);
			}
			break;
		}
	}
}

Brush* GUI::GetCurrentBrush() const {
	return current_brush;
}

BrushShape GUI::GetBrushShape() const {
	if (current_brush == spawn_brush) {
		return BRUSHSHAPE_SQUARE;
	}

	if (current_brush == spawn_npc_brush) {
		return BRUSHSHAPE_SQUARE;
	}

	return brush_shape;
}

int GUI::GetBrushSize() const {
	return brush_size;
}

int GUI::GetBrushVariation() const {
	return brush_variation;
}

int GUI::GetSpawnMonsterTime() const {
	return monster_spawntime;
}

int GUI::GetSpawnNpcTime() const {
	return npc_spawntime;
}

void GUI::SelectBrush() {
	if (palettes.empty()) {
		return;
	}

	SelectBrushInternal(palettes.front()->GetSelectedBrush());

	RefreshView();
}

bool GUI::SelectBrush(const Brush* whatbrush, PaletteType primary) {
	if (palettes.empty()) {
		if (!CreatePalette()) {
			return false;
		}
	}

	if (!palettes.front()->OnSelectBrush(whatbrush, primary)) {
		return false;
	}

	SelectBrushInternal(const_cast<Brush*>(whatbrush));
	root->GetAuiToolBar()->UpdateBrushButtons();
	return true;
}

void GUI::SelectBrushInternal(Brush* brush) {
	// Fear no evil don't you say no evil
	if (current_brush != brush && brush) {
		previous_brush = current_brush;
	}

	current_brush = brush;
	if (!current_brush) {
		return;
	}

	brush_variation = std::min(brush_variation, brush->getMaxVariation());
	FillDoodadPreviewBuffer();
	if (brush->isDoodad()) {
		secondary_map = doodad_buffer_map;
	}

	SetDrawingMode();
	RefreshView();
}

void GUI::SelectPreviousBrush() {
	if (previous_brush) {
		SelectBrush(previous_brush);
	}
}

void GUI::FillDoodadPreviewBuffer() {
	if (!current_brush || !current_brush->isDoodad()) {
		return;
	}

	doodad_buffer_map->clear();

	DoodadBrush* brush = current_brush->asDoodad();
	if (brush->isEmpty(GetBrushVariation())) {
		return;
	}

	int object_count = 0;
	int area;
	if (GetBrushShape() == BRUSHSHAPE_SQUARE) {
		area = 2 * GetBrushSize();
		area = area * area + 1;
	} else {
		if (GetBrushSize() == 1) {
			// There is a huge deviation here with the other formula.
			area = 5;
		} else {
			area = int(0.5 + GetBrushSize() * GetBrushSize() * rme::PI);
		}
	}
	const int object_range = (use_custom_thickness ? int(area * custom_thickness_mod) : brush->getThickness() * area / std::max(1, brush->getThicknessCeiling()));
	const int final_object_count = std::max(1, object_range + random(object_range));

	Position center_pos(0x8000, 0x8000, 0x8);

	if (brush_size > 0 && !brush->oneSizeFitsAll()) {
		while (object_count < final_object_count) {
			int retries = 0;
			bool exit = false;

			// Try to place objects 5 times
			while (retries < 5 && !exit) {

				int pos_retries = 0;
				int xpos = 0, ypos = 0;
				bool found_pos = false;
				if (GetBrushShape() == BRUSHSHAPE_CIRCLE) {
					while (pos_retries < 5 && !found_pos) {
						xpos = random(-brush_size, brush_size);
						ypos = random(-brush_size, brush_size);
						float distance = sqrt(float(xpos * xpos) + float(ypos * ypos));
						if (distance < g_gui.GetBrushSize() + 0.005) {
							found_pos = true;
						} else {
							++pos_retries;
						}
					}
				} else {
					found_pos = true;
					xpos = random(-brush_size, brush_size);
					ypos = random(-brush_size, brush_size);
				}

				if (!found_pos) {
					++retries;
					continue;
				}

				// Decide whether the zone should have a composite or several single objects.
				bool fail = false;
				if (random(brush->getTotalChance(GetBrushVariation())) <= brush->getCompositeChance(GetBrushVariation())) {
					// Composite
					const CompositeTileList &composites = brush->getComposite(GetBrushVariation());

					// Figure out if the placement is valid
					for (const auto &composite : composites) {
						Position pos = center_pos + composite.first + Position(xpos, ypos, 0);
						if (Tile* tile = doodad_buffer_map->getTile(pos)) {
							if (!tile->empty()) {
								fail = true;
								break;
							}
						}
					}
					if (fail) {
						++retries;
						break;
					}

					// Transfer items to the stack
					for (const auto &composite : composites) {
						Position pos = center_pos + composite.first + Position(xpos, ypos, 0);
						const ItemVector &items = composite.second;
						Tile* tile = doodad_buffer_map->getTile(pos);

						if (!tile) {
							tile = doodad_buffer_map->allocator(doodad_buffer_map->createTileL(pos));
						}

						for (auto item : items) {
							tile->addItem(item->deepCopy());
						}
						doodad_buffer_map->setTile(tile->getPosition(), tile);
					}
					exit = true;
				} else if (brush->hasSingleObjects(GetBrushVariation())) {
					Position pos = center_pos + Position(xpos, ypos, 0);
					Tile* tile = doodad_buffer_map->getTile(pos);
					if (tile) {
						if (!tile->empty()) {
							fail = true;
							break;
						}
					} else {
						tile = doodad_buffer_map->allocator(doodad_buffer_map->createTileL(pos));
					}
					int variation = GetBrushVariation();
					brush->draw(doodad_buffer_map, tile, &variation);
					// std::cout << "\tpos: " << tile->getPosition() << std::endl;
					doodad_buffer_map->setTile(tile->getPosition(), tile);
					exit = true;
				}
				if (fail) {
					++retries;
					break;
				}
			}
			++object_count;
		}
	} else {
		if (brush->hasCompositeObjects(GetBrushVariation()) && random(brush->getTotalChance(GetBrushVariation())) <= brush->getCompositeChance(GetBrushVariation())) {
			// Composite
			const CompositeTileList &composites = brush->getComposite(GetBrushVariation());

			// All placement is valid...

			// Transfer items to the buffer
			for (const auto &composite : composites) {
				Position pos = center_pos + composite.first;
				const ItemVector &items = composite.second;
				Tile* tile = doodad_buffer_map->allocator(doodad_buffer_map->createTileL(pos));
				// std::cout << pos << " = " << center_pos << " + " << buffer_tile->getPosition() << std::endl;

				for (auto item : items) {
					tile->addItem(item->deepCopy());
				}
				doodad_buffer_map->setTile(tile->getPosition(), tile);
			}
		} else if (brush->hasSingleObjects(GetBrushVariation())) {
			Tile* tile = doodad_buffer_map->allocator(doodad_buffer_map->createTileL(center_pos));
			int variation = GetBrushVariation();
			brush->draw(doodad_buffer_map, tile, &variation);
			doodad_buffer_map->setTile(center_pos, tile);
		}
	}
}
