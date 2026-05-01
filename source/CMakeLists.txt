
# *****************************************************************************
# Project remeres
# *****************************************************************************
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    project(canary-map-editor-debug LANGUAGES CXX)
else()
    project(canary-map-editor LANGUAGES CXX)
endif()

find_package(asio CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_path(STB_INCLUDE_DIRS "stb_truetype.h")
find_package(nlohmann_json CONFIG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(Threads REQUIRED)
find_package(wxWidgets COMPONENTS html aui gl adv core net base CONFIG REQUIRED)
find_package(ZLIB REQUIRED)
find_package(pugixml CONFIG REQUIRED)
find_package(liblzma CONFIG REQUIRED)
find_package(Protobuf REQUIRED)
find_package(cpr CONFIG REQUIRED)

include(FetchContent)
FetchContent_Declare(
    FastNoiseLite
    GIT_REPOSITORY https://github.com/Auburn/FastNoiseLite.git
    GIT_TAG        v1.1.1
)
FetchContent_MakeAvailable(FastNoiseLite)

# *****************************************************************************
# Options
# *****************************************************************************
option(TOGGLE_BIN_FOLDER "Use build/bin folder for generate compilation files" OFF)
option(OPTIONS_ENABLE_OPENMP "Enable Open Multi-Processing support." ON)
option(DEBUG_LOG "Enable Debug Log" OFF)
option(SPEED_UP_BUILD_UNITY "Compile using build unity for speed up build" ON)

# LibArchive disabled in compilation level by default, see "#define OTGZ_SUPPORT" in the "definitions.h" file
#if(APPLE)
#	set(CMAKE_PREFIX_PATH /usr/local/opt/libarchive)
#endif()
# If you need use, enable this:
#find_package(LibArchive REQUIRED)
#${LibArchive_INCLUDE_DIRS} ${LibArchive_LIBRARIES}

# Build static libs
if(BUILD_STATIC_LIBRARY)
	log_option_enabled("STATIC_LIBRARY")

	if(UNIX AND NOT APPLE)
		set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
	elseif(APPLE)
		set(CMAKE_FIND_LIBRARY_SUFFIXES ".a" ".dylib")
	endif()
else()
	log_option_disabled("STATIC_LIBRARY")
endif()

# === DEBUG LOG ===
# cmake -DDEBUG_LOG=ON ..
if(DEBUG_LOG)
	add_definitions(-DDEBUG_LOG=ON)
	log_option_enabled("DEBUG LOG")
else()
	log_option_disabled("DEBUG LOG")
endif(DEBUG_LOG)

if (MSVC)
	add_executable(${PROJECT_NAME} "" ../cmake/remeres.rc)

	if(BUILD_STATIC_LIBRARY)
		set_property(TARGET ${PROJECT_NAME} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
	endif()

	target_compile_options(${PROJECT_NAME} PUBLIC /MP /FS /Zf /EHsc /bigobj)
else()
	add_executable(${PROJECT_NAME} "")
endif()

# === OpenMP ===
if(OPTIONS_ENABLE_OPENMP)
	log_option_enabled("openmp")
	find_package(OpenMP)
	if(OpenMP_CXX_FOUND)
		target_link_libraries(${PROJECT_NAME} PUBLIC OpenMP::OpenMP_CXX)
	endif()
else()
	log_option_disabled("openmp")
endif()

# === IPO ===
check_ipo_supported(RESULT result OUTPUT output)
if(result)
	set_property(TARGET ${PROJECT_NAME} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
else()
	message(WARNING "IPO is not supported: ${output}")
endif()

# Lua includes (target-specific)
if(TARGET lua::lua)
	set(LUA_LIB_TARGET lua::lua)
elseif(TARGET lua)
	set(LUA_LIB_TARGET lua)
endif()

if(LUA_LIB_TARGET)
	get_target_property(VCPKG_LUA_INCLUDES ${LUA_LIB_TARGET} INTERFACE_INCLUDE_DIRECTORIES)
	if(VCPKG_LUA_INCLUDES)
		target_include_directories(${PROJECT_NAME} PRIVATE ${VCPKG_LUA_INCLUDES})
	endif()
	target_compile_definitions(${PROJECT_NAME} PRIVATE SOL_USING_CXX_LUA_HPP=1 SOL_ALL_SAFETIES_ON=1)
endif()

# === UNITY BUILD (compile time reducer) ===
if(SPEED_UP_BUILD_UNITY)
	set_target_properties(${PROJECT_NAME} PROPERTIES UNITY_BUILD ON)
	set_source_files_properties(rendering/gl_renderer.cpp PROPERTIES SKIP_UNITY_BUILD_INCLUSION ON)
	set_source_files_properties(util/mt_rand.cpp PROPERTIES SKIP_UNITY_BUILD_INCLUSION ON)
	log_option_enabled("Build unity for speed up compilation")
else()
	# === PRECOMPILED HEADER ===
	target_precompile_headers(${PROJECT_NAME} PRIVATE app/main.h)
endif()


target_sources(${PROJECT_NAME}
	PRIVATE
	ui/dialogs/about_window.cpp
	editor/action.cpp
	ui/windows/actions_history_window.cpp
	ui/windows/add_item_window.cpp
	ui/windows/add_tileset_window.cpp
	app/application.cpp
	app/artprovider.cpp
	map/basemap.cpp
	ui/windows/bitmap_to_map_window.cpp
	io/bitmap_to_map/bitmap_to_map_converter.cpp
	brushes/brush.cpp
	brushes/brush_tables.cpp
	ui/windows/browse_tile_window.cpp
	ui/positionctrl.cpp
	brushes/carpet_brush.cpp
	client_assets.cpp
	util/common.cpp
	ui/dialogs/common_windows.cpp
	game/complexitem.cpp
	ui/dialogs/container_properties_window.cpp
	editor/copybuffer.cpp
	brushes/monster_brush.cpp
	game/monster.cpp
	game/monsters.cpp
	ui/windows/dat_debug_view.cpp
	ui/dcbutton.cpp
	brushes/doodad_brush.cpp
	editor/editor.cpp
	editor/editor_save.cpp
	editor/editor_import.cpp
	editor/editor_selection_ops.cpp
	editor/editor_draw.cpp
	editor/editor_tabs.cpp
	brushes/eraser_brush.cpp
	ui/windows/find_item_window.cpp
	io/filehandle.cpp
	rendering/graphics.cpp
	rendering/gl_renderer.cpp
	brushes/ground_brush.cpp
	ui/gui.cpp
	ui/gui_loading.cpp
	ui/gui_brush_state.cpp
	ui/gui_file_ops.cpp
	ui/gui_palette.cpp
	editor/hotkey.cpp
	editor/hotkey_manager.cpp
	brushes/house_brush.cpp
	game/house.cpp
	brushes/house_exit_brush.cpp
	rendering/light_drawer.cpp
	io/iomap.cpp
	io/otbm/iomap_otbm.cpp
	io/otbm/otbm_item_serialization.cpp
	io/xml/otbm_spawns.cpp
	io/xml/otbm_houses.cpp
	io/xml/otbm_zones.cpp
	io/iominimap.cpp
	game/item_attributes.cpp
	game/item.cpp
	game/items.cpp
	live/live_action.cpp
	live/live_client.cpp
	live/live_peer.cpp
	live/live_server.cpp
	live/live_socket.cpp
	live/live_tab.cpp
	lua/lua_engine.cpp
	lua/lua_script.cpp
	lua/lua_script_manager.cpp
	lua/lua_api.cpp
	lua/lua_api_app.cpp
	lua/lua_api_position.cpp
	lua/lua_api_item.cpp
	lua/lua_api_tile.cpp
	lua/lua_api_map.cpp
	lua/lua_api_selection.cpp
	lua/lua_dialog.cpp
	lua/lua_api_color.cpp
	lua/lua_api_creature.cpp
	lua/lua_api_brush.cpp
	lua/lua_api_image.cpp
	lua/lua_api_geo.cpp
	lua/lua_api_http.cpp
	lua/lua_api_noise.cpp
	lua/lua_api_json.cpp
	lua/lua_api_algo.cpp
	lua/lua_scripts_window.cpp
	ui/menubar/main_menubar.cpp
	ui/menubar/file_menu_handlers.cpp
	ui/menubar/edit_menu_handlers.cpp
	ui/menubar/map_menu_handlers.cpp
	ui/menubar/view_menu_handlers.cpp
	ui/menubar/window_menu_handlers.cpp
	ui/menubar/menu_search_handlers.cpp
	ui/toolbar/main_toolbar.cpp
	map/map.cpp
	rendering/map_display.cpp
	rendering/canvas_mouse_handlers.cpp
	rendering/canvas_keyboard_handlers.cpp
	rendering/canvas_context_menu.cpp
	rendering/map_drawer.cpp
	rendering/draw_brush.cpp
	rendering/draw_tile_detail.cpp
	rendering/draw_tooltips.cpp
	map/map_region.cpp
	ui/map_tab.cpp
	rendering/map_window.cpp
	game/materials.cpp
	ui/windows/minimap_window.cpp
	util/mkpch.cpp
	util/mt_rand.cpp
	net/net_connection.cpp
	game/npc.cpp
	brushes/npc_brush.cpp
	game/npcs.cpp
	ui/numbertextctrl.cpp
	ui/dialogs/old_properties_window.cpp
	ui/palette/palette_brushlist.cpp
	ui/palette/palette_common.cpp
	ui/palette/palette_monster.cpp
	ui/palette/palette_house.cpp
	ui/palette/palette_npc.cpp
	ui/palette/palette_waypoints.cpp
	ui/palette/palette_zones.cpp
	ui/palette/palette_window.cpp
	io/pngfiles.cpp
	ui/dialogs/preferences.cpp
	app/process_com.cpp
	ui/dialogs/properties_window.cpp
	brushes/raw_brush.cpp
	ui/windows/replace_items_window.cpp
	ui/windows/result_window.cpp
	net/rme_net.cpp
	editor/selection.cpp
	app/settings.cpp
	brushes/spawn_monster_brush.cpp
	game/spawn_monster.cpp
	game/spawn_npc.cpp
	brushes/spawn_npc_brush.cpp
	rendering/sprite_appearances.cpp
	util/stb_truetype_impl.cpp
	brushes/table_brush.cpp
	templates/templatemap76-74.cpp
	templates/templatemap81.cpp
	templates/templatemap854.cpp
	templates/templatemapclassic.cpp
	map/tile.cpp
	map/tile_operations.cpp
	game/tileset.cpp
	ui/windows/tileset_window.cpp
	game/town.cpp
	app/updater.cpp
	brushes/wall_brush.cpp
	brushes/waypoint_brush.cpp
	game/waypoints.cpp
	ui/windows/welcome_dialog.cpp
	brushes/zone_brush.cpp
	game/zones.cpp
)

target_include_directories(${PROJECT_NAME}
	PRIVATE
	${CMAKE_SOURCE_DIR}/source
	${OPENGL_INCLUDE_DIR}
	${ZLIB_INCLUDE_DIR}
	${fastnoiselite_SOURCE_DIR}/Cpp
	${STB_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME}
	PRIVATE
	${OPENGL_LIBRARIES}
	${ZLIB_LIBRARIES}
	glad::glad
	fmt::fmt
	asio::asio
	nlohmann_json::nlohmann_json
	pugixml::pugixml
	wx::base wx::core wx::net wx::gl wx::html wx::aui wx::adv
	liblzma::liblzma
	OpenGL::GL
	protobuf
	${LUA_LIB_TARGET}
	sol2::sol2
	cpr::cpr

	$<$<PLATFORM_ID:Linux>:xcb>
	$<$<PLATFORM_ID:Windows>:psapi>
)

## Link compilation files to build/bin folder, else link to the main dir
if (TOGGLE_BIN_FOLDER)
	set_target_properties(${PROJECT_NAME}
			PROPERTIES
			RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
	)
else()
	set_target_properties(${PROJECT_NAME}
			PROPERTIES
			RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/"
	)
endif()
