#include "app/main.h"

#include "ui/menubar/main_menubar.h"
#include "ui/gui.h"
#include "map/map.h"
#include "map/tile.h"
#include "game/item.h"
#include "ui/windows/result_window.h"

namespace OnSearchForStuff {
	struct Searcher {
		Searcher() :
			search_unique(false),
			search_action(false),
			search_container(false),
			search_writeable(false) { }

		bool search_unique;
		bool search_action;
		bool search_container;
		bool search_writeable;
		std::vector<std::pair<Tile*, Item*>> found;

		void operator()(Map &map, Tile* tile, Item* item, long long done) {
			if (done % 0x8000 == 0) {
				g_gui.SetLoadDone((unsigned int)(100 * done / map.getTileCount()));
			}
			auto hasContainer = [&]() {
				Container* container = dynamic_cast<Container*>(item);
				return container && container->getItemCount();
			};
			if ((search_unique && item->getUniqueID() > 0) || (search_action && item->getActionID() > 0) || (search_container && hasContainer()) || (search_writeable && item && item->getText().length() > 0)) {
				found.push_back(std::make_pair(tile, item));
			}
		}

		wxString desc(Item* item) {
			wxString label;
			if (item->getUniqueID() > 0) {
				label << "UID:" << item->getUniqueID() << " ";
			}

			if (item->getActionID() > 0) {
				label << "AID:" << item->getActionID() << " ";
			}

			label << wxstr(item->getName());

			if (dynamic_cast<Container*>(item)) {
				label << " (Container) ";
			}

			if (item->getText().length() > 0) {
				label << " (Text: " << wxstr(item->getText()) << ") ";
			}

			return label;
		}

		void sort() {
			if (search_unique || search_action) {
				std::sort(found.begin(), found.end(), Searcher::compare);
			}
		}

		static bool compare(const std::pair<Tile*, Item*> &pair1, const std::pair<Tile*, Item*> &pair2) {
			const Item* item1 = pair1.second;
			const Item* item2 = pair2.second;

			if (item1->getActionID() != 0 || item2->getActionID() != 0) {
				return item1->getActionID() < item2->getActionID();
			} else if (item1->getUniqueID() != 0 || item2->getUniqueID() != 0) {
				return item1->getUniqueID() < item2->getUniqueID();
			}

			return false;
		}
	};
}

void MainMenuBar::SearchItems(bool unique, bool action, bool container, bool writable, bool onSelection /* = false*/) {
	if (!unique && !action && !container && !writable) {
		return;
	}

	if (!g_gui.IsEditorOpen()) {
		return;
	}

	const auto searchType = onSelection ? "selected area" : "map";

	g_gui.CreateLoadBar(wxString::Format("Searching on %s...", searchType));

	OnSearchForStuff::Searcher searcher;
	searcher.search_unique = unique;
	searcher.search_action = action;
	searcher.search_container = container;
	searcher.search_writeable = writable;

	foreach_ItemOnMap(g_gui.GetCurrentMap(), searcher, onSelection);
	searcher.sort();
	std::vector<std::pair<Tile*, Item*>> &found = searcher.found;

	g_gui.DestroyLoadBar();

	SearchResultWindow* result = g_gui.ShowSearchWindow();
	result->Clear();

	for (std::vector<std::pair<Tile*, Item*>>::iterator iter = found.begin(); iter != found.end(); ++iter) {
		result->AddPosition(searcher.desc(iter->second), iter->first->getPosition());
	}
}

namespace SearchDuplicatedItems {
	struct condition {
		std::unordered_set<Tile*> foundTiles;

		void operator()(Map &map, Tile* tile, Item* item, long long done) {
			if (done % 0x8000 == 0) {
				g_gui.SetLoadDone((unsigned int)(100 * done / map.getTileCount()));
			}

			if (!tile) {
				return;
			}

			if (!item) {
				return;
			}

			if (item->isGroundTile()) {
				return;
			}

			if (foundTiles.count(tile) == 0) {
				std::unordered_set<int> itemIDs;
				for (Item* existingItem : tile->items) {
					if (itemIDs.count(existingItem->getID()) > 0 && !existingItem->hasElevation()) {
						foundTiles.insert(tile);
						break;
					}
					itemIDs.insert(existingItem->getID());
				}
			}
		}
	};
}

void MainMenuBar::SearchDuplicatedItems(bool onSelection /* = false*/) {
	if (!g_gui.IsEditorOpen()) {
		return;
	}

	const auto searchType = onSelection ? "selected area" : "map";

	g_gui.CreateLoadBar(wxString::Format("Searching on %s...", searchType));

	SearchDuplicatedItems::condition finder;

	foreach_ItemOnMap(g_gui.GetCurrentMap(), finder, onSelection);
	std::unordered_set<Tile*> &foundTiles = finder.foundTiles;

	g_gui.DestroyLoadBar();

	const auto tilesFoundAmount = foundTiles.size();

	g_gui.PopupDialog("Search completed", wxString::Format("%zu tiles with duplicated items founded.", tilesFoundAmount), wxOK);

	SearchResultWindow* result = g_gui.ShowSearchWindow();
	result->Clear();
	for (const Tile* tile : foundTiles) {
		result->AddPosition("Duplicated items", tile->getPosition());
	}
}

namespace RemoveDuplicatesItems {
	struct condition {
		bool operator()(Map &map, Tile* tile, Item* item, long long removed, long long done) {
			if (done % 0x8000 == 0) {
				g_gui.SetLoadDone((unsigned int)(100 * done / map.getTileCount()));
			}

			if (!tile) {
				return false;
			}

			if (!item) {
				return false;
			}

			if (item->isGroundTile()) {
				return false;
			}

			if (item->isMoveable() && item->hasElevation()) {
				return false;
			}

			if (item->getActionID() > 0 || item->getUniqueID() > 0) {
				return false;
			}

			std::unordered_set<int> itemIDsDuplicates;
			for (const auto &itemInTile : tile->items) {
				if (itemInTile && itemInTile->getID() == item->getID()) {
					if (itemIDsDuplicates.count(itemInTile->getID()) > 0) {
						itemIDsDuplicates.clear();
						return true;
					}
					itemIDsDuplicates.insert(itemInTile->getID());
				}
			}

			itemIDsDuplicates.clear();
			return false;
		}
	};
}

void MainMenuBar::RemoveDuplicatesItems(bool onSelection /* = false*/) {
	if (!g_gui.IsEditorOpen()) {
		return;
	}

	const auto removalType = onSelection ? "selected area" : "map";

	const auto dialogResult = g_gui.PopupDialog("Remove Duplicated Items", wxString::Format("Do you want to remove all duplicated items from the %s?", removalType), wxYES | wxNO);

	if (dialogResult == wxID_YES) {
		g_gui.GetCurrentEditor()->clearActions();

		RemoveDuplicatesItems::condition func;

		g_gui.CreateLoadBar(wxString::Format("Searching on %s for items to remove...", removalType));

		const auto removedAmount = RemoveItemDuplicateOnMap(g_gui.GetCurrentMap(), func, onSelection);

		g_gui.DestroyLoadBar();

		g_gui.PopupDialog("Search completed", wxString::Format("%lld duplicated items removed.", removedAmount), wxOK);

		g_gui.GetCurrentMap().doChange();
	}
}

namespace SearchWallsUponWalls {
	struct condition {
		std::unordered_set<Tile*> foundTiles;

		void operator()(const Map &map, Tile* tile, const Item* item, long long done) {
			if (done % 0x8000 == 0) {
				g_gui.SetLoadDone(static_cast<unsigned int>(100 * done / map.getTileCount()));
			}

			if (!tile) {
				return;
			}

			if (!item) {
				return;
			}

			if (!item->isBlockMissiles()) {
				return;
			}

			if (!item->isWall() && !item->isDoor()) {
				return;
			}

			std::unordered_set<int> itemIDs;
			for (const Item* itemInTile : tile->items) {
				if (!itemInTile || (!itemInTile->isWall() && !itemInTile->isDoor())) {
					continue;
				}

				if (item->getID() != itemInTile->getID()) {
					itemIDs.insert(itemInTile->getID());
				}
			}

			if (!itemIDs.empty()) {
				foundTiles.insert(tile);
			}

			itemIDs.clear();
		}
	};
}

void MainMenuBar::SearchWallsUponWalls(bool onSelection /* = false*/) {
	if (!g_gui.IsEditorOpen()) {
		return;
	}

	const auto searchType = onSelection ? "selected area" : "map";

	g_gui.CreateLoadBar(wxString::Format("Searching on %s...", searchType));

	SearchWallsUponWalls::condition finder;

	foreach_ItemOnMap(g_gui.GetCurrentMap(), finder, onSelection);

	const std::unordered_set<Tile*> &foundTiles = finder.foundTiles;

	g_gui.DestroyLoadBar();

	const auto tilesFoundAmount = foundTiles.size();

	g_gui.PopupDialog("Search completed", wxString::Format("%zu items under walls and doors founded.", tilesFoundAmount), wxOK);

	SearchResultWindow* result = g_gui.ShowSearchWindow();
	result->Clear();
	for (const Tile* tile : foundTiles) {
		result->AddPosition("Item Under", tile->getPosition());
	}
}
