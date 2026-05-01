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

const wxEventType EVT_UPDATE_MENUS = wxNewEventType();
const wxEventType EVT_UPDATE_ACTIONS = wxNewEventType();

// Global GUI instance
GUI g_gui;

// GUI class implementation
GUI::GUI() :
	aui_manager(nullptr),
	root(nullptr),
	minimap(nullptr),
	gem(nullptr),
	search_result_window(nullptr),
	actions_history_window(nullptr),
	script_manager_window(nullptr),
	secondary_map(nullptr),
	doodad_buffer_map(nullptr),

	house_brush(nullptr),
	house_exit_brush(nullptr),
	waypoint_brush(nullptr),
	optional_brush(nullptr),
	eraser(nullptr),
	normal_door_brush(nullptr),
	locked_door_brush(nullptr),
	magic_door_brush(nullptr),
	quest_door_brush(nullptr),
	hatch_door_brush(nullptr),
	window_door_brush(nullptr),

	OGLContext(nullptr),
	mode(SELECTION_MODE),
	pasting(false),

	current_brush(nullptr),
	previous_brush(nullptr),
	brush_shape(BRUSHSHAPE_SQUARE),
	brush_size(0),
	brush_variation(0),

	monster_spawntime(0),
	npc_spawntime(0),
	use_custom_thickness(false),
	custom_thickness_mod(0.0),
	progressBar(nullptr),
	disabled_counter(0) {
	doodad_buffer_map = newd BaseMap();
}

GUI::~GUI() {
	delete doodad_buffer_map;
	delete g_gui.aui_manager;
	delete OGLContext;
}

wxGLContext* GUI::GetGLContext(wxGLCanvas* win) {
	if (OGLContext == nullptr) {
#ifdef __WXOSX__
		/*
		wxGLContext(AGLPixelFormat fmt, wxGLCanvas *win,
					const wxPalette& WXUNUSED(palette),
					const wxGLContext *other
					);
		*/
		OGLContext = new wxGLContext(win, nullptr);
#else
		OGLContext = newd wxGLContext(win);
#endif
	}

	return OGLContext;
}

wxString GUI::GetDataDirectory() {
	std::string cfg_str = g_settings.getString(Config::DATA_DIRECTORY);
	if (!cfg_str.empty()) {
		FileName dir;
		dir.Assign(wxstr(cfg_str));
		wxString path;
		if (dir.DirExists()) {
			path = dir.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR);
			return path;
		}
	}

	// Silently reset directory
	FileName exec_directory;
	try {
		exec_directory = dynamic_cast<wxStandardPaths &>(wxStandardPaths::Get()).GetExecutablePath();
	} catch (const std::bad_cast &) {
		throw; // Crash application (this should never happend anyways...)
	}

	exec_directory.AppendDir("data");
	return exec_directory.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR);
}

wxString GUI::GetExecDirectory() {
	// Silently reset directory
	FileName exec_directory;
	try {
		exec_directory = dynamic_cast<wxStandardPaths &>(wxStandardPaths::Get()).GetExecutablePath();
	} catch (const std::bad_cast &) {
		wxLogError("Could not fetch executable directory.");
	}
	return exec_directory.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR);
}

wxString GUI::GetLocalDataDirectory() {
	if (g_settings.getInteger(Config::INDIRECTORY_INSTALLATION)) {
		FileName dir = GetDataDirectory();
		dir.AppendDir("user");
		dir.AppendDir("data");
		dir.Mkdir(0755, wxPATH_MKDIR_FULL);
		return dir.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR);
		;
	} else {
		FileName dir = dynamic_cast<wxStandardPaths &>(wxStandardPaths::Get()).GetUserDataDir();
#ifdef __WINDOWS__
		dir.AppendDir("Remere's Map Editor");
#else
		dir.AppendDir(".rme");
#endif
		dir.AppendDir("data");
		dir.Mkdir(0755, wxPATH_MKDIR_FULL);
		return dir.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR);
	}
}

wxString GUI::GetLocalDirectory() {
	if (g_settings.getInteger(Config::INDIRECTORY_INSTALLATION)) {
		FileName dir = GetDataDirectory();
		dir.AppendDir("user");
		dir.Mkdir(0755, wxPATH_MKDIR_FULL);
		return dir.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR);
		;
	} else {
		FileName dir = dynamic_cast<wxStandardPaths &>(wxStandardPaths::Get()).GetUserDataDir();
#ifdef __WINDOWS__
		dir.AppendDir("Remere's Map Editor");
#else
		dir.AppendDir(".rme");
#endif
		dir.Mkdir(0755, wxPATH_MKDIR_FULL);
		return dir.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR);
	}
}

void GUI::discoverDataDirectory(const wxString &existentFile) {
	wxString currentDir = wxGetCwd();
	wxString execDir = GetExecDirectory();

	wxString possiblePaths[] = {
		execDir,
		currentDir + "/",

		// these are used usually when running from build directories
		execDir + "/../",
		execDir + "/../../",
		execDir + "/../../../",
		currentDir + "/../",
	};

	bool found = false;
	for (const wxString &path : possiblePaths) {
		if (wxFileName(path + "data/" + existentFile).FileExists()) {
			m_dataDirectory = path + "data/";
			found = true;
			break;
		}
	}

	if (!found) {
		wxLogError(wxString() + "Could not find data directory.\n");
	}
}

void GUI::CycleTab(bool forward) {
	tabbook->CycleTab(forward);
}

bool GUI::IsEditorOpen() const {
	return tabbook != nullptr && GetCurrentMapTab();
}

double GUI::GetCurrentZoom() {
	MapTab* tab = GetCurrentMapTab();
	if (tab) {
		return tab->GetCanvas()->GetZoom();
	}
	return 1.0;
}

void GUI::SetCurrentZoom(double zoom) {
	MapTab* tab = GetCurrentMapTab();
	if (tab) {
		tab->GetCanvas()->SetZoom(zoom);
	}
}

void GUI::FitViewToMap() {
	for (int index = 0; index < tabbook->GetTabCount(); ++index) {
		if (auto* tab = dynamic_cast<MapTab*>(tabbook->GetTab(index))) {
			tab->GetView()->FitToMap();
		}
	}
}

void GUI::FitViewToMap(MapTab* mt) {
	for (int index = 0; index < tabbook->GetTabCount(); ++index) {
		if (auto* tab = dynamic_cast<MapTab*>(tabbook->GetTab(index))) {
			if (tab->HasSameReference(mt)) {
				tab->GetView()->FitToMap();
			}
		}
	}
}

Editor* GUI::GetCurrentEditor() {
	MapTab* mapTab = GetCurrentMapTab();
	if (mapTab) {
		return mapTab->GetEditor();
	}
	return nullptr;
}

EditorTab* GUI::GetTab(int idx) {
	return tabbook->GetTab(idx);
}

int GUI::GetTabCount() const {
	return tabbook->GetTabCount();
}

EditorTab* GUI::GetCurrentTab() {
	return tabbook->GetCurrentTab();
}

MapTab* GUI::GetCurrentMapTab() const {
	if (tabbook && tabbook->GetTabCount() > 0) {
		EditorTab* editorTab = tabbook->GetCurrentTab();
		auto* mapTab = dynamic_cast<MapTab*>(editorTab);
		return mapTab;
	}
	return nullptr;
}

Map &GUI::GetCurrentMap() {
	Editor* editor = GetCurrentEditor();
	ASSERT(editor);
	return editor->getMap();
}

int GUI::GetOpenMapCount() {
	std::set<Map*> open_maps;

	for (int i = 0; i < tabbook->GetTabCount(); ++i) {
		auto* tab = dynamic_cast<MapTab*>(tabbook->GetTab(i));
		if (tab) {
			open_maps.insert(open_maps.begin(), tab->GetMap());
		}
	}

	return static_cast<int>(open_maps.size());
}

bool GUI::ShouldSave() {
	Editor* editor = GetCurrentEditor();
	ASSERT(editor);
	return editor->hasChanges();
}

void GUI::AddPendingCanvasEvent(wxEvent &event) {
	MapTab* mapTab = GetCurrentMapTab();
	if (mapTab) {
		mapTab->GetCanvas()->GetEventHandler()->AddPendingEvent(event);
	}
}

void GUI::CloseCurrentEditor() {
	RefreshPalettes();
	tabbook->DeleteTab(tabbook->GetSelection());
	root->UpdateMenubar();
}

bool GUI::CloseLiveEditors(LiveSocket* sock) {
	for (int i = 0; i < tabbook->GetTabCount(); ++i) {
		auto* mapTab = dynamic_cast<MapTab*>(tabbook->GetTab(i));
		if (mapTab) {
			Editor* editor = mapTab->GetEditor();
			if (editor->GetLiveClient() == sock) {
				tabbook->DeleteTab(i--);
			}
		}
		auto* liveLogTab = dynamic_cast<LiveLogTab*>(tabbook->GetTab(i));
		if (liveLogTab) {
			if (liveLogTab->GetSocket() == sock) {
				liveLogTab->Disconnect();
				tabbook->DeleteTab(i--);
			}
		}
	}
	root->UpdateMenubar();
	return true;
}

bool GUI::CloseAllEditors() {
	for (int i = 0; i < tabbook->GetTabCount(); ++i) {
		auto* mapTab = dynamic_cast<MapTab*>(tabbook->GetTab(i));
		if (mapTab) {
			if (mapTab->IsUniqueReference() && mapTab->GetMap() && mapTab->GetMap()->hasChanged()) {
				tabbook->SetFocusedTab(i);
				if (!root->DoQuerySave(false)) {
					return false;
				} else {
					RefreshPalettes();
					tabbook->DeleteTab(i--);
				}
			} else {
				tabbook->DeleteTab(i--);
			}
		}
	}
	if (root) {
		root->UpdateMenubar();
	}
	return true;
}

void GUI::NewMapView() {
	MapTab* mapTab = GetCurrentMapTab();
	if (mapTab) {
		auto* newMapTab = newd MapTab(mapTab);
		newMapTab->OnSwitchEditorMode(mode);

		SetStatusText("Created new view");
		UpdateTitle();
		RefreshPalettes();
		root->UpdateMenubar();
		root->Refresh();
	}
}

void GUI::LoadPerspective() {
	if (!ClientAssets::isLoaded()) {
		if (g_settings.getInteger(Config::WINDOW_MAXIMIZED)) {
			root->Maximize();
		} else {
			root->SetSize(wxSize(
				g_settings.getInteger(Config::WINDOW_WIDTH),
				g_settings.getInteger(Config::WINDOW_HEIGHT)
			));
		}
	} else {
		std::string tmp;
		std::string layout = g_settings.getString(Config::PALETTE_LAYOUT);

		std::vector<std::string> palette_list;
		for (char c : layout) {
			if (c == '|') {
				palette_list.push_back(tmp);
				tmp.clear();
			} else {
				tmp.push_back(c);
			}
		}

		if (!tmp.empty()) {
			palette_list.push_back(tmp);
		}

		for (const std::string &name : palette_list) {
			PaletteWindow* palette = CreatePalette();

			wxAuiPaneInfo &info = aui_manager->GetPane(palette);
			aui_manager->LoadPaneInfo(wxstr(name), info);

			if (info.IsFloatable()) {
				bool offscreen = true;
				for (uint32_t index = 0; index < wxDisplay::GetCount(); ++index) {
					wxDisplay display(index);
					wxRect rect = display.GetClientArea();
					if (rect.Contains(info.floating_pos)) {
						offscreen = false;
						break;
					}
				}

				if (offscreen) {
					info.Dock();
				}
			}
		}

		if (g_settings.getInteger(Config::MINIMAP_VISIBLE)) {
			if (!minimap) {
				wxAuiPaneInfo info;

				const wxString &data = wxstr(g_settings.getString(Config::MINIMAP_LAYOUT));
				aui_manager->LoadPaneInfo(data, info);

				minimap = newd MinimapWindow(root);
				aui_manager->AddPane(minimap, info);
			} else {
				wxAuiPaneInfo &info = aui_manager->GetPane(minimap);

				const wxString &data = wxstr(g_settings.getString(Config::MINIMAP_LAYOUT));
				aui_manager->LoadPaneInfo(data, info);
			}

			wxAuiPaneInfo &info = aui_manager->GetPane(minimap);
			if (info.IsFloatable()) {
				bool offscreen = true;
				for (uint32_t index = 0; index < wxDisplay::GetCount(); ++index) {
					wxDisplay display(index);
					wxRect rect = display.GetClientArea();
					if (rect.Contains(info.floating_pos)) {
						offscreen = false;
						break;
					}
				}

				if (offscreen) {
					info.Dock();
				}
			}
		}

		if (g_settings.getInteger(Config::ACTIONS_HISTORY_VISIBLE)) {
			if (!actions_history_window) {
				wxAuiPaneInfo info;

				const wxString &data = wxstr(g_settings.getString(Config::ACTIONS_HISTORY_LAYOUT));
				aui_manager->LoadPaneInfo(data, info);

				actions_history_window = new ActionsHistoryWindow(root);
				aui_manager->AddPane(actions_history_window, info);
			} else {
				wxAuiPaneInfo &info = aui_manager->GetPane(actions_history_window);
				const wxString &data = wxstr(g_settings.getString(Config::ACTIONS_HISTORY_LAYOUT));
				aui_manager->LoadPaneInfo(data, info);
			}

			wxAuiPaneInfo &info = aui_manager->GetPane(actions_history_window);
			if (info.IsFloatable()) {
				bool offscreen = true;
				for (uint32_t index = 0; index < wxDisplay::GetCount(); ++index) {
					wxDisplay display(index);
					wxRect rect = display.GetClientArea();
					if (rect.Contains(info.floating_pos)) {
						offscreen = false;
						break;
					}
				}

				if (offscreen) {
					info.Dock();
				}
			}
		}

		aui_manager->Update();
		root->UpdateMenubar();
	}

	root->GetAuiToolBar()->LoadPerspective();
}

void GUI::SavePerspective() {
	g_settings.setInteger(Config::WINDOW_MAXIMIZED, root->IsMaximized());
	g_settings.setInteger(Config::WINDOW_WIDTH, root->GetSize().GetWidth());
	g_settings.setInteger(Config::WINDOW_HEIGHT, root->GetSize().GetHeight());
	g_settings.setInteger(Config::MINIMAP_VISIBLE, minimap ? 1 : 0);
	g_settings.setInteger(Config::ACTIONS_HISTORY_VISIBLE, actions_history_window ? 1 : 0);

	wxString pinfo;
	for (auto &palette : palettes) {
		if (aui_manager->GetPane(palette).IsShown()) {
			pinfo << aui_manager->SavePaneInfo(aui_manager->GetPane(palette)) << "|";
		}
	}
	g_settings.setString(Config::PALETTE_LAYOUT, nstr(pinfo));

	if (minimap) {
		wxString s = aui_manager->SavePaneInfo(aui_manager->GetPane(minimap));
		g_settings.setString(Config::MINIMAP_LAYOUT, nstr(s));
	}

	if (actions_history_window) {
		wxString info = aui_manager->SavePaneInfo(aui_manager->GetPane(actions_history_window));
		g_settings.setString(Config::ACTIONS_HISTORY_LAYOUT, nstr(info));
	}

	root->GetAuiToolBar()->SavePerspective();
}

void GUI::HideSearchWindow() {
	if (search_result_window) {
		aui_manager->GetPane(search_result_window).Show(false);
		aui_manager->Update();
	}
}

SearchResultWindow* GUI::ShowSearchWindow() {
	if (search_result_window == nullptr) {
		search_result_window = newd SearchResultWindow(root);
		aui_manager->AddPane(search_result_window, wxAuiPaneInfo().Caption("Search Results"));
	} else {
		aui_manager->GetPane(search_result_window).Show();
	}
	aui_manager->Update();
	return search_result_window;
}

ActionsHistoryWindow* GUI::ShowActionsWindow() {
	if (!actions_history_window) {
		actions_history_window = new ActionsHistoryWindow(root);
		aui_manager->AddPane(actions_history_window, wxAuiPaneInfo().Caption("Actions History"));
	} else {
		aui_manager->GetPane(actions_history_window).Show();
	}

	aui_manager->Update();
	actions_history_window->RefreshActions();
	return actions_history_window;
}

void GUI::HideActionsWindow() {
	if (actions_history_window) {
		aui_manager->GetPane(actions_history_window).Show(false);
		aui_manager->Update();
	}
}

LuaScriptsWindow* GUI::ShowScriptManagerWindow() {
	if (!script_manager_window) {
		script_manager_window = new LuaScriptsWindow(root);
		LuaScriptsWindow::SetInstance(script_manager_window);
		aui_manager->AddPane(script_manager_window, wxAuiPaneInfo().Caption("Script Manager").Right().Layer(1).CloseButton(true).MinSize(300, 200).BestSize(400, 300));
	} else {
		aui_manager->GetPane(script_manager_window).Show();
	}
	aui_manager->Update();
	script_manager_window->RefreshScriptList();
	return script_manager_window;
}

//=============================================================================
// Palette Window Interface implementation

//=============================================================================
// Minimap Window Interface Implementation

//=============================================================================

void GUI::RefreshView() {
	EditorTab* editorTab = GetCurrentTab();
	if (!editorTab) {
		return;
	}

	if (!dynamic_cast<MapTab*>(editorTab)) {
		editorTab->GetWindow()->Refresh();
		return;
	}

	std::vector<EditorTab*> editorTabs;
	for (int32_t index = 0; index < tabbook->GetTabCount(); ++index) {
		auto* mapTab = dynamic_cast<MapTab*>(tabbook->GetTab(index));
		if (mapTab) {
			editorTabs.push_back(mapTab);
		}
	}

	for (EditorTab* editorTab : editorTabs) {
		auto* mapTab = static_cast<MapTab*>(editorTab);
		mapTab->GetCanvas()->Refresh(); // MapCanvas::Refresh() → markDirty() + wxGLCanvas::Refresh()
		editorTab->GetWindow()->Update();
	}
}

void GUI::ShowWelcomeDialog(const wxBitmap &icon) {
	std::vector<wxString> recent_files = root->GetRecentFiles();
	welcomeDialog = newd WelcomeDialog(__W_RME_APPLICATION_NAME__, "Version " + __W_RME_VERSION__, FROM_DIP(root, wxSize(800, 480)), icon, recent_files);
	welcomeDialog->Bind(wxEVT_CLOSE_WINDOW, &GUI::OnWelcomeDialogClosed, this);
	welcomeDialog->Bind(WELCOME_DIALOG_ACTION, &GUI::OnWelcomeDialogAction, this);
	welcomeDialog->Show();
	UpdateMenubar();
}

void GUI::FinishWelcomeDialog() {
	if (welcomeDialog != nullptr) {
		welcomeDialog->Hide();
		root->Show();
		welcomeDialog->Destroy();
		welcomeDialog = nullptr;
	}
}

bool GUI::IsWelcomeDialogShown() {
	return welcomeDialog != nullptr && welcomeDialog->IsShown();
}

void GUI::OnWelcomeDialogClosed(wxCloseEvent &event) {
	welcomeDialog->Destroy();
	root->Close();
}

void GUI::OnWelcomeDialogAction(wxCommandEvent &event) {
	if (event.GetId() == wxID_NEW) {
		NewMap();
	} else if (event.GetId() == wxID_OPEN) {
		LoadMap(FileName(event.GetString()));
	}
}

void GUI::UpdateMenubar() {
	root->UpdateMenubar();
}

void GUI::SetScreenCenterPosition(const Position &position, bool showIndicator) {
	MapTab* mapTab = GetCurrentMapTab();
	if (mapTab) {
		mapTab->SetScreenCenterPosition(position, showIndicator);
	}
}

void GUI::DoCut() {
	if (!IsSelectionMode()) {
		return;
	}

	Editor* editor = GetCurrentEditor();
	if (!editor) {
		return;
	}

	editor->copybuffer.cut(*editor, GetCurrentFloor());
	RefreshView();
	root->UpdateMenubar();
}

void GUI::DoCopy() {
	if (!IsSelectionMode()) {
		return;
	}

	Editor* editor = GetCurrentEditor();
	if (!editor) {
		return;
	}

	editor->copybuffer.copy(*editor, GetCurrentFloor());
	RefreshView();
	root->UpdateMenubar();
}

void GUI::DoPaste() {
	MapTab* mapTab = GetCurrentMapTab();
	if (mapTab) {
		copybuffer.paste(*mapTab->GetEditor(), mapTab->GetCanvas()->GetCursorPosition());
	}
}

void GUI::PreparePaste() {
	Editor* editor = GetCurrentEditor();
	if (editor) {
		SetSelectionMode();
		Selection &selection = editor->getSelection();
		selection.start();
		selection.clear();
		selection.finish();
		StartPasting();
		RefreshView();
	}
}

void GUI::StartPasting() {
	if (GetCurrentEditor()) {
		pasting = true;
		secondary_map = &copybuffer.getBufferMap();
	}
}

void GUI::EndPasting() {
	if (pasting) {
		pasting = false;
		secondary_map = nullptr;
	}
}

bool GUI::CanUndo() {
	Editor* editor = GetCurrentEditor();
	return (editor && editor->canUndo());
}

bool GUI::CanRedo() {
	Editor* editor = GetCurrentEditor();
	return (editor && editor->canRedo());
}

bool GUI::DoUndo() {
	Editor* editor = GetCurrentEditor();
	if (editor && editor->canUndo()) {
		editor->undo();
		if (editor->hasSelection()) {
			SetSelectionMode();
		}
		SetStatusText("Undo action");
		UpdateMinimap();
		root->UpdateMenubar();
		RefreshView();
		return true;
	}
	return false;
}

bool GUI::DoRedo() {
	Editor* editor = GetCurrentEditor();
	if (editor && editor->canRedo()) {
		editor->redo();
		if (editor->hasSelection()) {
			SetSelectionMode();
		}
		SetStatusText("Redo action");
		UpdateMinimap();
		root->UpdateMenubar();
		RefreshView();
		return true;
	}
	return false;
}

int GUI::GetCurrentFloor() {
	MapTab* tab = GetCurrentMapTab();
	ASSERT(tab);
	return tab->GetCanvas()->GetFloor();
}

void GUI::ChangeFloor(int new_floor) {
	MapTab* tab = GetCurrentMapTab();
	if (tab) {
		int old_floor = GetCurrentFloor();
		if (new_floor < rme::MapMinLayer || new_floor > rme::MapMaxLayer) {
			return;
		}

		if (old_floor != new_floor) {
			tab->GetCanvas()->ChangeFloor(new_floor);
		}
	}
}

void GUI::SetStatusText(wxString text) {
	g_gui.root->SetStatusText(text, 0);
}

void GUI::SetTitle(wxString title) {
	if (g_gui.root == nullptr) {
		return;
	}

#ifdef NIGHTLY_BUILD
	#ifdef SVN_BUILD
		#define TITLE_APPEND (wxString(" (Nightly Build #") << i2ws(SVN_BUILD) << ")")
	#else
		#define TITLE_APPEND (wxString(" (Nightly Build)"))
	#endif
#else
	#ifdef SVN_BUILD
		#define TITLE_APPEND (wxString(" (Build #") << i2ws(SVN_BUILD) << ")")
	#else
		#define TITLE_APPEND (wxString(""))
	#endif
#endif
#ifdef __EXPERIMENTAL__
	if (title != "") {
		g_gui.root->SetTitle(title << " - Canary's Map Editor BETA" << TITLE_APPEND);
	} else {
		g_gui.root->SetTitle(wxString("Canary's Map Editor BETA") << TITLE_APPEND);
	}
#elif __SNAPSHOT__
	if (title != "") {
		g_gui.root->SetTitle(title << " - Canary's Map Editor - SNAPSHOT" << TITLE_APPEND);
	} else {
		g_gui.root->SetTitle(wxString("Canary's Map Editor - SNAPSHOT") << TITLE_APPEND);
	}
#else
	if (!title.empty()) {
		g_gui.root->SetTitle(title << " - Canary's Map Editor" << TITLE_APPEND);
	} else {
		g_gui.root->SetTitle(wxString("Canary's Map Editor") << TITLE_APPEND);
	}
#endif
}

void GUI::UpdateTitle() {
	if (tabbook->GetTabCount() > 0) {
		SetTitle(tabbook->GetCurrentTab()->GetTitle());
		for (int idx = 0; idx < tabbook->GetTabCount(); ++idx) {
			if (tabbook->GetTab(idx)) {
				tabbook->SetTabLabel(idx, tabbook->GetTab(idx)->GetTitle());
			}
		}
	} else {
		SetTitle("");
	}
}

void GUI::UpdateMenus() {
	wxCommandEvent evt(EVT_UPDATE_MENUS);
	g_gui.root->AddPendingEvent(evt);
}

void GUI::UpdateActions() {
	wxCommandEvent evt(EVT_UPDATE_ACTIONS);
	g_gui.root->AddPendingEvent(evt);
}

void GUI::RefreshActions() {
	if (actions_history_window) {
		actions_history_window->RefreshActions();
	}
}

void GUI::ShowToolbar(ToolBarID id, bool show) {
	if (root && root->GetAuiToolBar()) {
		root->GetAuiToolBar()->Show(id, show);
	}
}

long GUI::PopupDialog(wxWindow* parent, wxString title, wxString text, long style, wxString confisavename, uint32_t configsavevalue) {
	if (text.empty()) {
		return wxID_ANY;
	}

	wxMessageDialog dlg(parent, text, title, style);
	return dlg.ShowModal();
}

long GUI::PopupDialog(wxString title, wxString text, long style, wxString configsavename, uint32_t configsavevalue) {
	return g_gui.PopupDialog(g_gui.root, title, text, style, configsavename, configsavevalue);
}

void GUI::ListDialog(wxWindow* parent, wxString title, const wxArrayString &param_items) {
	if (param_items.empty()) {
		return;
	}

	wxArrayString list_items(param_items);

	// Create the window
	wxDialog* dlg = newd wxDialog(parent, wxID_ANY, title, wxDefaultPosition, wxDefaultSize, wxRESIZE_BORDER | wxCAPTION | wxCLOSE_BOX);

	wxSizer* sizer = newd wxBoxSizer(wxVERTICAL);
	wxListBox* item_list = newd wxListBox(dlg, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, nullptr, wxLB_SINGLE);
	item_list->SetMinSize(wxSize(500, 300));

	for (size_t i = 0; i != list_items.GetCount();) {
		wxString str = list_items[i];
		size_t pos = str.find("\n");
		if (pos != wxString::npos) {
			// Split string!
			item_list->Append(str.substr(0, pos));
			list_items[i] = str.substr(pos + 1);
			continue;
		}
		item_list->Append(list_items[i]);
		++i;
	}
	sizer->Add(item_list, 1, wxEXPAND);

	wxSizer* stdsizer = newd wxBoxSizer(wxHORIZONTAL);
	stdsizer->Add(newd wxButton(dlg, wxID_OK, "OK"), wxSizerFlags(1).Center());
	sizer->Add(stdsizer, wxSizerFlags(0).Center());

	dlg->SetSizerAndFit(sizer);

	// Show the window
	dlg->ShowModal();
	delete dlg;
}

void GUI::ShowTextBox(wxWindow* parent, wxString title, wxString content) {
	wxDialog* dlg = newd wxDialog(parent, wxID_ANY, title, wxDefaultPosition, wxDefaultSize, wxRESIZE_BORDER | wxCAPTION | wxCLOSE_BOX);
	wxSizer* topsizer = newd wxBoxSizer(wxVERTICAL);
	wxTextCtrl* text_field = newd wxTextCtrl(dlg, wxID_ANY, content, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY);
	text_field->SetMinSize(wxSize(400, 550));
	topsizer->Add(text_field, wxSizerFlags(5).Expand());

	wxSizer* choicesizer = newd wxBoxSizer(wxHORIZONTAL);
	choicesizer->Add(newd wxButton(dlg, wxID_CANCEL, "OK"), wxSizerFlags(1).Center());
	topsizer->Add(choicesizer, wxSizerFlags(0).Center());
	dlg->SetSizerAndFit(topsizer);

	dlg->ShowModal();
}

void SetWindowToolTip(wxWindow* a, const wxString &tip) {
	a->SetToolTip(tip);
}

void SetWindowToolTip(wxWindow* a, wxWindow* b, const wxString &tip) {
	a->SetToolTip(tip);
	b->SetToolTip(tip);
}
