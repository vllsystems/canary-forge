# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

---

## [4.0.0] — Canary's Map Editor

Rebranded from "Remere's Map Editor" to "Canary's Map Editor".

### Added

- Lua scripting engine with API modules and Script Manager UI (#158, #161).
- Load monsters and NPCs from Canary Lua files instead of XML; configurable directories in Preferences (#162).
- Monster palette organized by Lua folder structure with "All" option and alphabetical sorting (#174).
- Bitmap to Map terrain generator — import images and convert colors to map tiles using ground brushes, with color detection, tolerance, presets, crop, rotate, flip, scale (#166).
- Real-time performance monitor overlay (FPS, CPU, RAM) (#156).
- Zones brush/editor (#56).
- Tilesets manager (#68).
- Monsters and NPCs spawns icons (#58).
- Search tiles by flag (#84).
- Search for items under walls/windows/doors (#63).
- Option to remove duplicated items (#62).
- Show item and ground avoidables / block pathfinder (#103).
- Show light strength (#78).
- Lights visible outside in-game box (#77).
- Edit item properties from browse field (#97).
- Copy position anywhere (#93).
- Erase flags and zones (#90).
- Count monsters on selection (#114).
- Item overlay order customization in browse field (#110).
- Palette brushes pagination with Small/Large Icons (#119).
- Monsters weight display (#122).
- Export minimap improvements with pre-defined image sizes (#117, #141).
- Import and export zones (#137).
- Update monsters spawn time based on selection (#139).
- Add complement 'beds' option for houses (#45).
- Add max beds and house client id (#8).
- Organization of blank sprites in blank tileset (#42).
- Support reading assets from client 11 or higher (#70).
- Add item 2187 to invisible item rendering (#152).
- Fallback creature sprite (lookType 197) when outfit has no valid look.
- Added ARCHITECTURE.md documenting the full project structure.
- Added clang-format linter (#54).
- Added missing creatures (#41).
- Added missing Inferniarch monsters (#149).

### Fixed

- Fix outfit color cache and addon rendering for creatures (#164).
- Fix correct start_frame type mismatch from protobuf loading (#163).
- Fix NPC spawn use-after-free on map import, progress bar freeze (#167).
- Fix crash when opening new map view on Linux (#168).
- Instant process exit on close to avoid freeze with large maps (#172).
- Fix properties window not opening (#131).
- Fix cmake build (#130).
- Fix crash with debug build / wrong assert (#128).
- Fix selection dialog issues (#124).
- Fix loading of fluid items (#116).
- Fix monsters search (#115).
- Fix update unknown fluid and improve subtype selection (#108).
- Fix remove duplicated items feature (#101).
- Fix difference of splash type between RME and server (#104).
- Fix clipboard paste position pattern and special brushes palette (#102).
- Fix go to house with no exit (#92).
- Fix position indicator (#91).
- Fix position copy/paste clipboard (#89).
- Fix crash on place monster spawn (#75).
- Fix reload warnings (#80).
- Fix 'open with' option (#143).
- Fix enable opening multiple map files in dialog (#153).
- Fix errors dialog (#142).
- Fix NPC lookitem (#19).
- Fix map display OnMouseActionClick object selection logic (#17).
- Fix monster spawn time negative value (#29).
- Fix exception handling for reading and creating JSON objects (#34).
- Fix slightly more accurate house size estimation (#46).
- Fix macOS build (#55).
- Fix Linux build (#57).
- Fix deprecated warnings (#47, #65).
- Fix compilation error on MainMenuBar and DatDebugView (#22).
- Fix sonar issues (#76).
- Fixed build using vcxproj (#154).

### Changed

- Migrated from legacy OpenGL (GLUT/immediate mode) to modern OpenGL 3.3 with batched 2D renderer, shaders, and VAO/VBO pipeline (#165).
- Sprite atlas rendering using sheet textures with per-sprite UV coordinates (#171).
- Deferred rendering pipeline with FBO-backed render-to-texture and scene caching for faster map redraws (#173).
- Baked font atlas using stb_truetype for text rendering (replaced GLUT bitmap fonts).
- Lazy atlas texture upload with automatic garbage collection of unused GPU textures.
- Command-based draw batching with merged draw calls for fewer GPU submissions.
- Complete modular reorganization of source/ into 14 domain modules (app, brushes, editor, game, io, live, lua, map, net, protobuf, rendering, templates, ui, util).
- Slimmed down main.h precompiled header — moved specialized includes to specific modules.
- Extracted TileOperations, HotkeyManager, DrawingOptions, MenuBar::ActionID, OTBM types into dedicated files.
- Split large files: editor.cpp, gui.cpp, map_drawer.cpp, main_menubar.cpp, iomap_otbm.cpp into focused sub-files.
- Non-unity build compatibility with IWYU includes.
- Updated C++ standard from C++20 to C++23 (#159).
- Replaced legacy GLUT dependency with glad GL loader.
- Removed Boost dependencies, improved compilation and GHA support (#31).
- Integrated pugixml (#40).
- Palette window refactor (#82).
- Palette brushlist refactor (#81).
- Brush icon box rendering optimization (#79).
- Backup organization in a folder with auto-delete after X days (#73, #113).
- Select item on raw palette when using CTRL+F (#72).
- Not remove duplicate items with action id or unique id (#85).
- Rate-limited automatic cleanup of unused textures.
- Smarter scene invalidation to avoid unnecessary re-renders.
- Updated items to version 13.40 (#94).
- Updated items to version 13.32 (#87).
- Updated items.otb and items.xml (#52).
- Updated items and fixed import map window size (#27).
- Updated NPCs list and fix missing NPCs message display (#150).
- Updated vcpkg builtin-baseline (#74).
- Updated vcpkg manifest and SonarCloud GHA (#35).
- Updated Visual Studio solution (#155).

### Removed

- Removed static data/creatures/monsters.xml and npcs.xml (replaced by Lua loading).

---

## [3.5]

### Added

- Implements flood fill in Terrain Brush.
- Update wall brushes for 10.98.
- Added Show As Minimap menu.
- Make spawns visible when placing a new spawn.

### Fixed

- Fix container item crash.

---

## [3.4]

### Added

- New Find Item / Jump to Item dialog.
- Configurable copy position format.
- Add text ellipsis for tooltips.
- Show hook indicators for walls.
- Updated data for 10.98.

### Fixed

- Icon background colour; white and gray no longer work.
- Only show colors option breaks RME.

---

## [3.3]

### Added

- Support for tooltips in the map.
- Support for animations preview.
- Restore last position when opening a map.
- Export search result to a .txt file.
- Waypoint brush improvements.
- Better fullscreen support on macOS.

### Fixed

- Items larger than 64x64 are now displayed properly.
- Fixed potential crash when using waypoint brush.
- Fixed a bug where you could not open map files by clicking them while the editor is running.
- You can now open the extensions folder on macOS.
- Fixed a bug where an item search would not display any result on macOS.
- Fixed multiple issues related to editing houses on macOS.

---

## [3.2]

### Added

- Export minimap by selected area.
- Search for unique id, action id, containers or writable items on selected area.
- Go to Previous Position menu. Keyboard shortcut `P`.
- Data files for version 10.98.
- Select Raw button on the Browse Field window.

### Fixed

- Text is hidden after selecting an item from the palette. Issue #144
- Search result does not sort ids. Issue #126
- Monster direction is not saved. Issue #132

---

## [3.1]

### Added

- In-game box improvements. Now the hidden tiles, visible tiles and player position are displayed.
- New _Zoom In_, _Zoom Out_ and _Zoom Normal_ menus.
- New keyboard shortcuts:
  - `Ctrl+Alt+Click` — Select the relative brush of an item.
  - `Ctrl++` — Zoom In.
  - `Ctrl+-` — Zoom Out.
  - `Ctrl+0` — Zoom Normal (100%).
- If zoom is 100%, move one tile at a time.

### Fixed

- Some keyboard shortcuts not working on Linux.
- Main menu is not updated when the last map tab is closed.
- In-game box wrong height.
- UI tweaks for Import Map window.
