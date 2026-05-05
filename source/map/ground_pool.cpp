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

#include "ground_pool.h"
#include "app/settings.h"
#include <mutex>

std::unordered_map<uint16_t, std::shared_ptr<Item>> GroundPool::pool;
std::atomic<size_t> GroundPool::memorySaved(0);
std::atomic<size_t> GroundPool::sharedCount(0);
std::mutex GroundPool::poolMutex;

Item* GroundPool::getSharedGround(uint16_t groundId) {
	if (!isEnabled()) {
		return nullptr;
	}

	std::lock_guard<std::mutex> lock(poolMutex);

	auto it = pool.find(groundId);
	if (it != pool.end()) {
		sharedCount.fetch_add(1);
		return it->second.get();
	}

	Item* ground = Item::Create(groundId);
	if (ground) {
		pool[groundId] = std::shared_ptr<Item>(ground);
		sharedCount.fetch_add(1);
		return ground;
	}

	return nullptr;
}

void GroundPool::clear() {
	std::lock_guard<std::mutex> lock(poolMutex);
	pool.clear();
	memorySaved.store(0);
	sharedCount.store(0);
}

bool GroundPool::isEnabled() {
	return g_settings.getBoolean(Config::GROUND_COMPRESSION_ENABLED);
}
