# Architecture  
  
This document describes the modular source layout of Remere's Map Editor after the refactoring.  
  
## Directory Structure  
  
```  
source/  
├── main.h  
├── CMakeLists.txt  
│  
├── app/  
│   ├── application.cpp / .h  
│   ├── definitions.h  
│   ├── process_com.cpp / .h  
│   └── updater.cpp / .h  
│  
├── brushes/  
│   ├── brush.cpp / .h  
│   ├── brush_enums.h  
│   ├── brush_tables.cpp  
│   ├── carpet_brush.cpp / .h  
│   ├── doodad_brush.cpp / .h  
│   ├── eraser_brush.cpp  
│   ├── ground_brush.cpp / .h  
│   ├── house_brush.cpp / .h  
│   ├── house_exit_brush.cpp / .h  
│   ├── monster_brush.cpp / .h  
│   ├── npc_brush.cpp / .h  
│   ├── raw_brush.cpp / .h  
│   ├── spawn_monster_brush.cpp / .h  
│   ├── spawn_npc_brush.cpp / .h  
│   ├── table_brush.cpp / .h  
│   ├── wall_brush.cpp / .h  
│   ├── waypoint_brush.cpp / .h  
│   └── zone_brush.cpp / .h  
│  
├── editor/  
│   ├── action.cpp / .h  
│   ├── copybuffer.cpp / .h  
│   ├── editor.cpp / .h  
│   ├── selection.cpp / .h  
│   └── settings.cpp / .h  
│  
├── game/  
│   ├── complexitem.cpp / .h  
│   ├── house.cpp / .h  
│   ├── item.cpp / .h  
│   ├── item_attributes.cpp / .h  
│   ├── items.cpp / .h  
│   ├── materials.cpp / .h  
│   ├── monster.cpp / .h  
│   ├── monsters.cpp / .h  
│   ├── npc.cpp / .h  
│   ├── npcs.cpp / .h  
│   ├── outfit.h  
│   ├── spawn_monster.cpp / .h  
│   ├── spawn_npc.cpp / .h  
│   ├── tileset.cpp / .h  
│   ├── town.cpp / .h  
│   ├── waypoints.cpp / .h  
│   └── zones.cpp / .h  
│  
├── io/  
│   ├── client_assets.cpp / .h  
│   ├── filehandle.cpp / .h  
│   ├── iomap.cpp / .h  
│   ├── iomap_otbm.cpp / .h  
│   ├── iomap_otmm.cpp / .h  
│   ├── iominimap.cpp / .h  
│   ├── otbm_houses.cpp  
│   ├── otbm_spawns.cpp  
│   ├── otbm_types.h  
│   ├── otbm_zones.cpp  
│   └── sprite_appearances.cpp / .h  
│  
├── live/  
│   ├── live_action.cpp / .h  
│   ├── live_client.cpp / .h  
│   ├── live_packets.h  
│   ├── live_peer.cpp / .h  
│   ├── live_server.cpp / .h  
│   ├── live_socket.cpp / .h  
│   └── live_tab.cpp / .h  
│  
├── lua/  
│   ├── lua_api.cpp / .h  
│   ├── lua_api_algo.cpp / .h  
│   ├── lua_api_app.cpp / .h  
│   ├── lua_api_brush.cpp / .h  
│   ├── lua_api_color.cpp / .h  
│   ├── lua_api_creature.cpp / .h  
│   ├── lua_api_geo.cpp / .h  
│   ├── lua_api_http.cpp / .h  
│   ├── lua_api_image.cpp / .h  
│   ├── lua_api_item.cpp / .h  
│   ├── lua_api_json.cpp / .h  
│   ├── lua_api_map.cpp / .h  
│   ├── lua_api_noise.cpp / .h  
│   ├── lua_api_position.cpp / .h  
│   ├── lua_api_selection.cpp / .h  
│   ├── lua_api_tile.cpp / .h  
│   ├── lua_dialog.cpp / .h  
│   ├── lua_engine.cpp / .h  
│   ├── lua_overlay_manager.h  
│   ├── lua_script.cpp / .h  
│   ├── lua_script_manager.cpp / .h  
│   └── lua_scripts_window.cpp / .h  
│  
├── map/  
│   ├── basemap.cpp / .h  
│   ├── map.cpp / .h  
│   ├── map_allocator.h  
│   ├── map_overlay.h  
│   ├── map_region.cpp / .h  
│   └── tile.cpp / .h  
│  
├── net/  
│   ├── net_connection.cpp / .h  
│   └── rme_net.cpp / .h  
│  
├── protobuf/  
│   ├── CMakeLists.txt  
│   └── appearances.proto  
│  
├── rendering/  
│   ├── drawing_options.h  
│   ├── gl_compat.h  
│   ├── gl_renderer.cpp / .h  
│   ├── graphics.cpp / .h  
│   ├── light_drawer.cpp / .h  
│   ├── map_display.cpp / .h  
│   ├── map_drawer.cpp / .h  
│   ├── map_window.cpp / .h  
│   └── sprites.h  
│  
├── templates/  
│   ├── templatemap76-74.cpp  
│   ├── templatemap81.cpp  
│   ├── templatemap854.cpp  
│   ├── templatemapclassic.cpp  
│   └── templates.h  
│  
├── ui/  
│   ├── about_window.cpp / .h  
│   ├── actions_history_window.cpp / .h  
│   ├── add_item_window.cpp / .h  
│   ├── add_tileset_window.cpp / .h  
│   ├── artprovider.cpp / .h  
│   ├── bitmap_to_map_converter.cpp / .h  
│   ├── bitmap_to_map_window.cpp / .h  
│   ├── browse_tile_window.cpp / .h  
│   ├── common_windows.cpp / .h  
│   ├── container_properties_window.cpp / .h  
│   ├── dat_debug_view.cpp / .h  
│   ├── dcbutton.cpp / .h  
│   ├── editor_tabs.cpp / .h  
│   ├── find_item_window.cpp / .h  
│   ├── gui.cpp / .h  
│   ├── gui_ids.h  
│   ├── hotkey.cpp / .h  
│   ├── main_menubar.cpp / .h  
│   ├── main_toolbar.cpp / .h  
│   ├── map_tab.cpp / .h  
│   ├── menu_action_ids.h  
│   ├── menu_search_handlers.cpp  
│   ├── minimap_window.cpp / .h  
│   ├── numbertextctrl.cpp / .h  
│   ├── old_properties_window.cpp / .h  
│   ├── palette_brushlist.cpp / .h  
│   ├── palette_common.cpp / .h  
│   ├── palette_house.cpp / .h  
│   ├── palette_monster.cpp / .h  
│   ├── palette_npc.cpp / .h  
│   ├── palette_waypoints.cpp / .h  
│   ├── palette_window.cpp / .h  
│   ├── palette_zones.cpp / .h  
│   ├── pngfiles.cpp / .h  
│   ├── positionctrl.cpp / .h  
│   ├── preferences.cpp / .h  
│   ├── properties_window.cpp / .h  
│   ├── replace_items_window.cpp / .h  
│   ├── result_window.cpp / .h  
│   ├── tileset_window.cpp / .h  
│   └── welcome_dialog.cpp / .h  
│  
└── util/  
    ├── common.cpp / .h  
    ├── con_vector.h  
    ├── const.h  
    ├── enums.h  
    ├── lua_parser.h  
    ├── mkpch.cpp  
    ├── mt_rand.cpp / .h  
    ├── otml.h  
    ├── position.h  
    ├── rme_forward_declarations.h  
    ├── sprite_types.h  
    ├── stb_truetype_impl.cpp  
    └── threads.h  
```  
  
## Module Dependency Rules  
  
```  
                         ┌─────┐  
                         │ app │  
                         └──┬──┘  
                            │  
                         ┌──┴──┐  
                         │ ui  │  
                         └──┬──┘  
                   ┌────────┼────────┐  
                   │        │        │  
              ┌────┴────┐ ┌─┴──┐ ┌──┴───┐  
              │rendering│ │live│ │editor│  
              └────┬────┘ └─┬──┘ └──┬───┘  
                   │      ┌─┘    ┌──┼──────────┐  
                   │      │      │  │          │  
                   │   ┌──┴─┐ ┌─┴──┴──┐  ┌────┴───┐  
                   │   │net │ │brushes│  │   io   │  
                   │   └──┬─┘ └───┬───┘  └───┬────┘  
                   │      │       │           │  
                   │      │    ┌──┴──┐        │  
                   │      │    │ map │────────┘  
                   │      │    └──┬──┘  
                   │      │       │  
                   │      │    ┌──┴──┐  
                   └──────┴────│game │  
                               └──┬──┘  
                                  │  
                               ┌──┴──┐  
                               │util │  
                               └─────┘  
```  
  
General rules:  
- **util/** has zero internal dependencies (only stdlib and third-party)  
- **game/** depends on util/  
- **map/** depends on util/, game/  
- **brushes/** depends on util/, game/, map/  
- **io/** depends on util/, game/, map/  
- **net/** depends on util/  
- **editor/** depends on util/, game/, map/, brushes/, io/  
- **rendering/** depends on util/, game/, map/, editor/  
- **live/** depends on util/, net/, editor/  
- **ui/** depends on most modules (top of the dependency graph)  
- **app/** depends on ui/, editor/ (application bootstrap)  
  
## Key Design Decisions  
  
### main.h (Precompiled Header)  
`main.h` is the precompiled header. It contains only universally-needed includes:  
- Core wx headers (`wx/wx.h`, `wx/defs.h`, `wx/event.h`, etc.)  
- Standard library containers and streams  
- `fmt`, `pugixml`, `spdlog`  
- Shared utilities (`util/common.h`, `util/const.h`, `util/threads.h`)  
  
Specialized headers (`wx/glcanvas.h`, `wx/grid.h`, `wx/notebook.h`, `nlohmann/json.hpp`, `asio.hpp`, etc.) are included only in the files that use them.  
  
### Unity Build  
The project uses Unity Build by default (`SPEED_UP_BUILD_UNITY=ON`) for faster compilation. All source files must also compile individually (Unity Build OFF) to ensure correct include hygiene.  
  
### Include What You Use (IWYU)  
Every header and source file should include what it directly uses. Do not rely on transitive includes from `main.h` or other headers.  
  
## Tests  
  
Unit tests use GoogleTest. Enable with `-DBUILD_TESTS=ON` (off by default). Test sources live in `tests/` at the project root.  
  
Current test coverage:  
- `Position` — default/parameterized construction, equality/inequality, addition, subtraction, `+=`, less-than ordering (z > y > x priority), `isValid` boundary checks, stream `<<`/`>>` round-trip, `abs()`  
- `contiguous_vector` — default and custom-size construction, `at()` returns null for empty/out-of-bounds slots, `set()`/`at()` store-and-retrieve, `locate()` auto-resize, `operator[]`, `resize()` with null-initialized new elements  
  
## Adding New Files  
  
1. Place the file in the appropriate module directory under `source/`  
2. Add it to the `SOURCES` list in `source/CMakeLists.txt`  
3. Include only what the file directly uses — do not rely on transitive includes from `main.h`  
4. Verify the file compiles with Unity Build OFF: `cmake -DSPEED_UP_BUILD_UNITY=OFF`  
5. Respect the dependency rules above — e.g., a file in `util/` must not include anything from `ui/`