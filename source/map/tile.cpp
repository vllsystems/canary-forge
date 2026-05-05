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
#include <cstddef>
#include <memory_resource>
#include <ranges>

#include "brushes/brush.h"

#include "map/tile.h"
#include "map/ground_pool.h"
#include "game/monster.h"
#include "game/house.h"
#include "map/basemap.h"
#include "game/spawn_monster.h"
#include "brushes/ground_brush.h"
#include "brushes/wall_brush.h"
#include "brushes/carpet_brush.h"
#include "brushes/table_brush.h"
#include "game/npc.h"
#include "game/spawn_npc.h"

namespace {
	struct alignas(std::max_align_t) TilePoolBlockHeader {
		std::size_t size;
	};

	std::pmr::unsynchronized_pool_resource &tilePool() {
		static std::pmr::unsynchronized_pool_resource pool;
		return pool;
	}
}

void* Tile::operator new(std::size_t size) {
	const auto allocationSize = sizeof(TilePoolBlockHeader) + size;
	auto* header = static_cast<TilePoolBlockHeader*>(tilePool().allocate(allocationSize, alignof(std::max_align_t)));
	header->size = allocationSize;
	return header + 1;
}

void Tile::operator delete(void* ptr) noexcept {
	if (!ptr) {
		return;
	}
	auto* header = static_cast<TilePoolBlockHeader*>(ptr) - 1;
	tilePool().deallocate(header, header->size, alignof(std::max_align_t));
}

void* Tile::operator new(std::size_t size, const char*, int) {
	return Tile::operator new(size);
}

void Tile::operator delete(void* ptr, const char*, int) noexcept {
	Tile::operator delete(ptr);
}

const std::vector<Monster*> &Tile::getMonsters() const {
	static const std::vector<Monster*> emptyMonsters;
	return monsters ? *monsters : emptyMonsters;
}

const std::set<unsigned int> &Tile::getZones() const {
	static const std::set<unsigned int> emptyZones;
	return zones ? *zones : emptyZones;
}

Tile::Tile(int x, int y, int z) :
	location(nullptr),
	ground(nullptr),
	spawnMonster(nullptr),
	npc(nullptr),
	spawnNpc(nullptr),
	house_id(0),
	usesSharedGround(false),
	mapflags(0),
	statflags(0),
	minimapColor(INVALID_MINIMAP_COLOR) {
	////
}

Tile::Tile(TileLocation &loc) :
	location(&loc),
	ground(nullptr),
	spawnMonster(nullptr),
	npc(nullptr),
	spawnNpc(nullptr),
	house_id(0),
	usesSharedGround(false),
	mapflags(0),
	statflags(0),
	minimapColor(INVALID_MINIMAP_COLOR) {
	////
}

Tile::~Tile() {
	while (!items.empty()) {
		delete items.back();
		items.pop_back();
	}

	while (monsters && !monsters->empty()) {
		delete monsters->back();
		monsters->pop_back();
	}
	// printf("%d,%d,%d,%p\n", tilePos.x, tilePos.y, tilePos.z, ground);
	// Only delete ground if it's not shared
	if (ground && !usesSharedGround) {
		delete ground;
	}
	delete spawnMonster;
	delete npc;
	delete spawnNpc;
}

Tile* Tile::deepCopy(BaseMap &map) const {
	Tile* copy = map.allocator.allocateTile(location);
	copy->flags = flags;
	copy->house_id = house_id;
	if (spawnMonster) {
		copy->spawnMonster = spawnMonster->deepCopy();
	}
	if (spawnNpc) {
		copy->spawnNpc = spawnNpc->deepCopy();
	}
	if (npc) {
		copy->npc = npc->deepCopy();
	}
	// Spawncount & exits are not transferred on copy!
	if (ground) {
		// Always create a deep copy, even if ground is shared
		copy->ground = ground->deepCopy();
		copy->usesSharedGround = false;
	}

	for (const auto monster : getMonsters()) {
		copy->getOrCreateMonsters().emplace_back(monster->deepCopy());
	}
	for (const Item* item : items) {
		copy->items.push_back(item->deepCopy());
	}
	for (unsigned int zone : getZones()) {
		copy->addZone(zone);
	}
	return copy;
}

uint32_t Tile::memsize() const {
	uint32_t mem = sizeof(*this);
	if (ground) {
		mem += ground->memsize();
	}

	for (const Item* item : items) {
		mem += item->memsize();
	}

	mem += sizeof(Item*) * items.capacity();

	return mem;
}

int Tile::size() const {
	int sz = 0;
	if (ground) {
		++sz;
	}
	sz += items.size();
	if (monsters) {
		sz += monsters->size();
	}
	if (spawnMonster) {
		++sz;
	}
	if (npc) {
		++sz;
	}
	if (spawnNpc) {
		++sz;
	}
	if (location) {
		if (location->getHouseExits()) {
			++sz;
		}
		if (location->getSpawnMonsterCount()) {
			++sz;
		}
		if (location->getSpawnNpcCount()) {
			++sz;
		}
		if (location->getWaypointCount()) {
			++sz;
		}
	}
	return sz;
}

void Tile::merge(Tile* other) {

	if (!other) {
		return;
	}

	if (other->isPZ()) {
		setPZ(true);
	}
	if (other->house_id) {
		house_id = other->house_id;
	}

	if (other->ground) {
		delete ground;
		ground = other->ground;
		other->ground = nullptr;
	}

	if (other->spawnMonster) {
		delete spawnMonster;
		spawnMonster = other->spawnMonster;
		other->spawnMonster = nullptr;
	}

	if (other->npc) {
		delete npc;
		npc = other->npc;
		other->npc = nullptr;
	}

	if (other->spawnNpc) {
		delete spawnNpc;
		spawnNpc = other->spawnNpc;
		other->spawnNpc = nullptr;
	}

	for (const auto monster : other->getMonsters()) {
		addMonster(monster);
	}
	other->monsters.reset();

	for (Item* item : other->items) {
		addItem(item);
	}
	other->items.clear();
}

bool Tile::hasProperty(enum ITEMPROPERTY prop) const {
	if (prop == PROTECTIONZONE && isPZ()) {
		return true;
	}

	if (ground && ground->hasProperty(prop)) {
		return true;
	}

	for (const Item* item : items) {
		if (item->hasProperty(prop)) {
			return true;
		}
	}

	return false;
}

uint16_t Tile::getGroundSpeed() const noexcept {
	if (ground && !ground->isMetaItem()) {
		return ground->getGroundSpeed();
	}
	return 0;
}

int Tile::getIndexOf(Item* item) const {
	if (!item) {
		return wxNOT_FOUND;
	}

	int index = 0;
	if (ground) {
		if (ground == item) {
			return index;
		}
		index++;
	}

	if (!items.empty()) {
		auto it = std::find(items.begin(), items.end(), item);
		if (it != items.end()) {
			index += (it - items.begin());
			return index;
		}
	}
	return wxNOT_FOUND;
}

Item* Tile::getTopItem() const {
	if (!items.empty() && !items.back()->isMetaItem()) {
		return items.back();
	}
	if (ground && !ground->isMetaItem()) {
		return ground;
	}
	return nullptr;
}

Item* Tile::getItemAt(int index) const {
	if (index < 0) {
		return nullptr;
	}
	if (ground) {
		if (index == 0) {
			return ground;
		}
		index--;
	}
	if (!items.empty() && index >= 0 && index < items.size()) {
		return items.at(index);
	}
	return nullptr;
}

void Tile::addMonster(Monster* monster) {
	if (!monster) {
		return;
	}

	getOrCreateMonsters().emplace_back(monster);

	if (monster->isSelected()) {
		statflags |= TILESTATE_SELECTED;
	}
}

void Tile::addItem(Item* item) {
	if (!item) {
		return;
	}
	if (item->isGroundTile()) {
		// If had shared ground, unshare it first
		if (usesSharedGround) {
			unshareGround();
		}
		delete ground;
		ground = item;
		return;
	}

	ItemVector::iterator it;

	uint16_t gid = item->getGroundEquivalent();
	if (gid != 0) {
		// If had shared ground, unshare it first
		if (usesSharedGround) {
			unshareGround();
		}
		delete ground;
		ground = Item::Create(gid);
		// At the very bottom!
		it = items.begin();
	} else {
		if (item->isAlwaysOnBottom()) {
			it = items.begin();
			while (true) {
				if (it == items.end()) {
					break;
				} else if ((*it)->isAlwaysOnBottom()) {
					if (item->getTopOrder() < (*it)->getTopOrder()) {
						break;
					}
				} else { // Always on top
					break;
				}
				++it;
			}
		} else {
			it = items.end();
		}
	}

	items.insert(it, item);

	if (item->isSelected()) {
		statflags |= TILESTATE_SELECTED;
	}
}

bool Tile::removeItem(const Item* item) {
	for (auto it = items.begin(); it != items.end(); ++it) {
		if (*it == item) {
			delete *it;
			items.erase(it);
			return true;
		}
	}
	return false;
}

void Tile::clearGround() {
	if (ground && !usesSharedGround) {
		delete ground;
	}
	ground = nullptr;
	usesSharedGround = false;
}

void Tile::replaceGround(Item* newGround) {
	if (ground && !usesSharedGround) {
		delete ground;
	}
	ground = newGround;
	usesSharedGround = false;
}

void Tile::clearMonsters() {
	if (!monsters) {
		return;
	}
	for (Monster* m : *monsters) {
		delete m;
	}
	monsters.reset();
}

void Tile::clearSpawnMonster() {
	delete spawnMonster;
	spawnMonster = nullptr;
}

void Tile::select() {
	if (size() == 0) {
		return;
	}
	if (ground) {
		ground->select();
	}
	if (spawnMonster) {
		spawnMonster->select();
	}
	if (spawnNpc) {
		spawnNpc->select();
	}
	if (npc) {
		npc->select();
	}

	for (const auto monster : getMonsters()) {
		monster->select();
	}
	for (Item* item : items) {
		item->select();
	}

	statflags |= TILESTATE_SELECTED;
}

void Tile::deselect() {
	if (ground) {
		ground->deselect();
	}
	if (spawnMonster) {
		spawnMonster->deselect();
	}
	if (spawnNpc) {
		spawnNpc->deselect();
	}
	if (npc) {
		npc->deselect();
	}

	for (const auto monster : getMonsters()) {
		monster->deselect();
	}

	for (Item* item : items) {
		item->deselect();
	}

	statflags &= ~TILESTATE_SELECTED;
}

Monster* Tile::getTopMonster() const {
	return hasMonsters() ? monsters->back() : nullptr;
}

std::vector<Monster*> Tile::popSelectedMonsters() {
	std::vector<Monster*> popMonsters;
	if (!monsters) {
		return popMonsters;
	}

	std::erase_if(*monsters, [&](const auto monster) {
		if (monster->isSelected()) {
			popMonsters.emplace_back(monster);
			return true;
		}

		return false;
	});

	if (monsters->empty()) {
		monsters.reset();
	}
	statflags &= ~TILESTATE_SELECTED;
	return popMonsters;
}

std::vector<Monster*> Tile::getSelectedMonsters() {
	std::vector<Monster*> selectedMonters;
	std::copy_if(getMonsters().begin(), getMonsters().end(), std::back_inserter(selectedMonters), [](const auto monster) {
		return monster->isSelected();
	});

	return selectedMonters;
}

bool Tile::isMonsterRepeated(const std::string &searchMonster) const {
	const auto &tileMonsters = getMonsters();
	return std::ranges::find_if(tileMonsters, [&](const auto monster) {
			   return monster->getTypeName() == searchMonster;
		   })
		!= tileMonsters.end();
}

Item* Tile::getTopSelectedItem() {
	for (auto it = items.rbegin(); it != items.rend(); ++it) {
		if ((*it)->isSelected() && !(*it)->isMetaItem()) {
			return *it;
		}
	}
	if (ground && ground->isSelected() && !ground->isMetaItem()) {
		return ground;
	}
	return nullptr;
}

ItemVector Tile::popSelectedItems(bool ignoreTileSelected) {
	ItemVector pop_items;

	if (!ignoreTileSelected && !isSelected()) {
		return pop_items;
	}

	if (ground && ground->isSelected()) {
		pop_items.push_back(ground);
		ground = nullptr;
	}

	for (auto it = items.begin(); it != items.end();) {
		Item* item = (*it);
		if (item->isSelected()) {
			pop_items.push_back(item);
			it = items.erase(it);
		} else {
			++it;
		}
	}

	statflags &= ~TILESTATE_SELECTED;
	return pop_items;
}

ItemVector Tile::getSelectedItems() {
	ItemVector selected_items;

	if (!isSelected()) {
		return selected_items;
	}

	if (ground && ground->isSelected()) {
		selected_items.push_back(ground);
	}

	for (Item* item : items) {
		if (item->isSelected()) {
			selected_items.push_back(item);
		}
	}

	return selected_items;
}

uint8_t Tile::getMiniMapColor() const {
	if (minimapColor != INVALID_MINIMAP_COLOR) {
		return minimapColor;
	}

	for (auto it = items.rbegin(); it != items.rend(); ++it) {
		uint8_t color = (*it)->getMiniMapColor();
		if (color != 0) {
			return color;
		}
	}

	// check ground too
	if (hasGround()) {
		return ground->getMiniMapColor();
	}

	return 0;
}

void Tile::update() {
	statflags &= TILESTATE_MODIFIED;

	if (spawnMonster && spawnMonster->isSelected()) {
		statflags |= TILESTATE_SELECTED;
	}
	if (spawnNpc && spawnNpc->isSelected()) {
		statflags |= TILESTATE_SELECTED;
	}
	if (monsters) {
		for (const auto monster : *monsters) {
			if (monster->isSelected()) {
				statflags |= TILESTATE_SELECTED;
				break;
			}
		}
	}
	if (npc && npc->isSelected()) {
		statflags |= TILESTATE_SELECTED;
	}

	if (ground) {
		if (ground->isSelected()) {
			statflags |= TILESTATE_SELECTED;
		}
		if (ground->isBlocking()) {
			statflags |= TILESTATE_BLOCKING;
		}
		if (ground->getUniqueID() != 0) {
			statflags |= TILESTATE_UNIQUE;
		}
		if (ground->getMiniMapColor() != 0) {
			minimapColor = ground->getMiniMapColor();
		}
	}

	for (const Item* item : items) {
		if (item->isSelected()) {
			statflags |= TILESTATE_SELECTED;
		}
		if (item->getUniqueID() != 0) {
			statflags |= TILESTATE_UNIQUE;
		}
		if (item->getMiniMapColor() != 0) {
			minimapColor = item->getMiniMapColor();
		}

		const ItemType &type = g_items.getItemType(item->getID());

		if (type.unpassable) {
			statflags |= TILESTATE_BLOCKING;
		}
		if (type.isOptionalBorder) {
			statflags |= TILESTATE_OP_BORDER;
		}
		if (type.isTable) {
			statflags |= TILESTATE_HAS_TABLE;
		}
		if (type.isCarpet) {
			statflags |= TILESTATE_HAS_CARPET;
		}
	}

	if ((statflags & TILESTATE_BLOCKING) == 0) {
		if (!ground && items.empty()) {
			statflags |= TILESTATE_BLOCKING;
		}
	}
}

GroundBrush* Tile::getGroundBrush() const {
	if (ground && ground->getGroundBrush()) {
		return ground->getGroundBrush();
	}
	return nullptr;
}

Item* Tile::getWall() const {
	for (Item* item : items) {
		if (item->isWall()) {
			return item;
		}
	}
	return nullptr;
}

Item* Tile::getCarpet() const {
	for (Item* item : items) {
		if (item->isCarpet()) {
			return item;
		}
	}
	return nullptr;
}

Item* Tile::getTable() const {
	for (Item* item : items) {
		if (item->isTable()) {
			return item;
		}
	}
	return nullptr;
}

void Tile::selectGround() {
	bool selected = false;
	if (ground) {
		ground->select();
		selected = true;
	}
	ItemVector::iterator it;

	for (Item* item : items) {
		if (!item->isBorder()) {
			break;
		}
		item->select();
		selected = true;
	}

	if (selected) {
		statflags |= TILESTATE_SELECTED;
	}
}

void Tile::deselectGround() {
	if (ground) {
		ground->deselect();
	}
	for (Item* item : items) {
		if (!item->isBorder()) {
			break;
		}

		item->deselect();
	}
}

void Tile::setHouse(House* house) {
	house_id = (house ? house->id : 0);
}

void Tile::addHouseExit(House* house) {
	if (!house) {
		return;
	}

	HouseExitList* exits = location->createHouseExits();
	exits->push_back(house->id);
}

void Tile::removeHouseExit(House* house) {
	if (!house) {
		return;
	}

	HouseExitList* exits = location->getHouseExits();
	if (!exits || exits->empty()) {
		return;
	}

	auto it = std::find(exits->begin(), exits->end(), house->id);
	if (it != exits->end()) {
		exits->erase(it);
	}
}

bool Tile::hasHouseExit(uint32_t houseId) const {
	const HouseExitList* exits = getHouseExits();
	if (!exits || exits->empty()) {
		return false;
	}

	auto it = std::find(exits->begin(), exits->end(), houseId);
	return it != exits->end();
}

bool Tile::canUseSharedGround() const {
	return hasGround() && items.empty() && !spawnMonster && !spawnNpc && house_id == 0 && !hasUniqueItem() && ground->getActionID() == 0 && ground->getUniqueID() == 0 && !isSelected();
}

void Tile::optimizeGround() {
	if (!canUseSharedGround()) {
		return;
	}

	Item* sharedGround = GroundPool::getSharedGround(ground->getID());
	if (sharedGround) {
		// Delete the original ground if it's not already shared
		if (ground && !usesSharedGround) {
			delete ground;
		}
		ground = sharedGround;
		usesSharedGround = true;
	}
}

void Tile::unshareGround() {
	if (usesSharedGround && ground) {
		ground = ground->deepCopy();
		usesSharedGround = false;
	}
}
