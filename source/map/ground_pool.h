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

#ifndef RME_GROUND_POOL_H
#define RME_GROUND_POOL_H

#include "game/item.h"
#include <unordered_map>
#include <memory>
#include <atomic>

class GroundPool {
private:
	static std::unordered_map<uint16_t, std::shared_ptr<Item>> pool;
	static std::atomic<size_t> memorySaved;
	static std::atomic<size_t> sharedCount;
	static std::mutex poolMutex;

public:
	static Item* getSharedGround(uint16_t groundId);
	static size_t getMemorySaved();
	static size_t getSharedCount();
	static void clear();
	static bool isEnabled();
};

#endif
