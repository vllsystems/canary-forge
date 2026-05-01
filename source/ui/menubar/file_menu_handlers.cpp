//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
// File menu event handlers for MainMenuBar
//////////////////////////////////////////////////////////////////////

#include "app/main.h"

#include "ui/menubar/main_menubar.h"
#include "ui/gui.h"
#include "app/application.h"
#include "ui/dialogs/preferences.h"
#include "ui/dialogs/about_window.h"
#include "ui/windows/minimap_window.h"
#include "ui/windows/bitmap_to_map_window.h"
#include "ui/windows/dat_debug_view.h"
#include "client_assets.h"
#include "game/monsters.h"
#include "game/npcs.h"

void MainMenuBar::OnNew(wxCommandEvent &WXUNUSED(event)) {
	g_gui.NewMap();
}

void MainMenuBar::OnGenerateMap(wxCommandEvent &WXUNUSED(event)) {
	/*
	if(!DoQuerySave()) return;

	std::ostringstream os;
	os << "Untitled-" << untitled_counter << ".otbm";
	++untitled_counter;

	editor.generateMap(wxstr(os.str()));

	g_gui.SetStatusText("Generated newd map");

	g_gui.UpdateTitle();
	g_gui.RefreshPalettes();
	g_gui.UpdateMinimap();
	g_gui.FitViewToMap();
	UpdateMenubar();
	Refresh();
	*/
}

void MainMenuBar::OnOpenRecent(wxCommandEvent &event) {
	FileName fn(recentFiles.GetHistoryFile(event.GetId() - recentFiles.GetBaseId()));
	frame->LoadMap(fn);
}

void MainMenuBar::OnOpen(wxCommandEvent &WXUNUSED(event)) {
	g_gui.OpenMap();
}

void MainMenuBar::OnClose(wxCommandEvent &WXUNUSED(event)) {
	frame->DoQuerySave(true); // It closes the editor too
}

void MainMenuBar::OnSave(wxCommandEvent &WXUNUSED(event)) {
	g_gui.SaveMap();
}

void MainMenuBar::OnSaveAs(wxCommandEvent &WXUNUSED(event)) {
	g_gui.SaveMapAs();
}

void MainMenuBar::OnPreferences(wxCommandEvent &WXUNUSED(event)) {
	PreferencesWindow dialog(frame);
	dialog.ShowModal();
	dialog.Destroy();
}

void MainMenuBar::OnQuit(wxCommandEvent &WXUNUSED(event)) {
	/*
	while(g_gui.IsEditorOpen())
		if(!frame->DoQuerySave(true))
			return;
			*/
	//((Application*)wxTheApp)->Unload();
	g_gui.root->Close();
}

void MainMenuBar::OnImportMap(wxCommandEvent &WXUNUSED(event)) {
	ASSERT(g_gui.GetCurrentEditor());
	wxDialog* importmap = newd ImportMapWindow(frame, *g_gui.GetCurrentEditor());
	importmap->ShowModal();
}

void MainMenuBar::OnImportMonsterData(wxCommandEvent &WXUNUSED(event)) {
	wxFileDialog dlg(g_gui.root, "Import monster file", "", "", "*.xml", wxFD_OPEN | wxFD_MULTIPLE | wxFD_FILE_MUST_EXIST);
	if (dlg.ShowModal() == wxID_OK) {
		wxArrayString paths;
		dlg.GetPaths(paths);
		for (uint32_t i = 0; i < paths.GetCount(); ++i) {
			wxString error;
			wxArrayString warnings;
			bool ok = g_monsters.importXMLFromOT(FileName(paths[i]), error, warnings);
			if (ok) {
				g_gui.ListDialog("Monster loader errors", warnings);
			} else {
				wxMessageBox("Error OT data file \"" + paths[i] + "\".\n" + error, "Error", wxOK | wxICON_INFORMATION, g_gui.root);
			}
		}
	}
}

void MainMenuBar::OnImportNpcData(wxCommandEvent &WXUNUSED(event)) {
	wxFileDialog dlg(g_gui.root, "Import npc file", "", "", "*.xml", wxFD_OPEN | wxFD_MULTIPLE | wxFD_FILE_MUST_EXIST);
	if (dlg.ShowModal() == wxID_OK) {
		wxArrayString paths;
		dlg.GetPaths(paths);
		for (uint32_t i = 0; i < paths.GetCount(); ++i) {
			wxString error;
			wxArrayString warnings;
			bool ok = g_npcs.importXMLFromOT(FileName(paths[i]), error, warnings);
			if (ok) {
				g_gui.ListDialog("Monster loader errors", warnings);
			} else {
				wxMessageBox("Error OT data file \"" + paths[i] + "\".\n" + error, "Error", wxOK | wxICON_INFORMATION, g_gui.root);
			}
		}
	}
}

void MainMenuBar::OnImportMinimap(wxCommandEvent &WXUNUSED(event)) {
	ASSERT(g_gui.IsEditorOpen());
	// wxDialog* importmap = newd ImportMapWindow();
	// importmap->ShowModal();
}

void MainMenuBar::OnImportBitmapToMap(wxCommandEvent &WXUNUSED(event)) {
	if (!g_gui.IsEditorOpen()) {
		return;
	}
	BitmapToMapWindow dlg(g_gui.root, *g_gui.GetCurrentEditor());
	dlg.ShowModal();
}

void MainMenuBar::OnExportMinimap(wxCommandEvent &WXUNUSED(event)) {
	if (!g_gui.IsEditorOpen()) {
		return;
	}

	ExportMiniMapWindow dialog(frame, *g_gui.GetCurrentEditor());
	dialog.ShowModal();
}

void MainMenuBar::OnExportTilesets(wxCommandEvent &WXUNUSED(event)) {
	if (g_gui.GetCurrentEditor()) {
		ExportTilesetsWindow dlg(frame, *g_gui.GetCurrentEditor());
		dlg.ShowModal();
		dlg.Destroy();
	}
}

void MainMenuBar::OnDebugViewDat(wxCommandEvent &WXUNUSED(event)) {
	wxDialog dlg(frame, wxID_ANY, "Debug .dat file", wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
	new DatDebugView(&dlg);
	dlg.ShowModal();
}

void MainMenuBar::OnReloadDataFiles(wxCommandEvent &WXUNUSED(event)) {
	wxString error;
	wxArrayString warnings;
	g_gui.loadMapWindow(error, warnings, true);
	g_gui.PopupDialog("Error", error, wxOK);
	g_gui.ListDialog("Warnings", warnings);
	auto clientDirectory = ClientAssets::getPath().ToStdString() + "/";
	if (clientDirectory.empty() || !wxDirExists(wxString(clientDirectory))) {
		PreferencesWindow dialog(nullptr);
		dialog.getBookCtrl().SetSelection(4);
		dialog.ShowModal();
		dialog.Destroy();
	}
}

void MainMenuBar::OnGotoWebsite(wxCommandEvent &WXUNUSED(event)) {
	::wxLaunchDefaultBrowser("http://www.remeresmapeditor.com/", wxBROWSER_NEW_WINDOW);
}

void MainMenuBar::OnAbout(wxCommandEvent &WXUNUSED(event)) {
	AboutWindow about(frame);
	about.ShowModal();
}
