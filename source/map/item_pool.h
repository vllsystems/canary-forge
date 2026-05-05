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

#ifndef RME_ITEM_POOL_H
#define RME_ITEM_POOL_H

#include "game/item.h"
#include <vector>
#include <memory>
#include <mutex>
#include <atomic>

// Forward declarations
class Container;
class Teleport;
class Door;
class Depot;

// Simple item key for indexing (item ID + subtype)
struct ItemKey {
    uint16_t id;
    uint16_t subtype;
    
    bool operator==(const ItemKey& other) const {
        return id == other.id && subtype == other.subtype;
    }
};

// Check if an item can be deduplicated (shared)
// Uses stricter criteria to avoid overhead from rare items
bool canDeduplicateItem(Item* item);

// ItemPool with optimized storage using vector instead of unordered_map
// Only deduplicates very common simple items (ID < 500, subtype == 1)
class ItemPool {
private:
    // Fixed-size vector for common items (IDs 0-499)
    // Each entry holds a shared_ptr to the deduplicated item
    static std::vector<std::shared_ptr<Item>> commonPool;
    static std::mutex poolMutex;
    static std::atomic<size_t> memorySaved;
    static std::atomic<size_t> sharedCount;
    static std::atomic<size_t> totalRequests;
    
    // Threshold: minimum number of times an item type must be seen
    // before we start deduplicating it
    static constexpr size_t MIN_OCCURRENCES_THRESHOLD = 50;
    static std::vector<size_t> occurrenceCounts;
    static std::mutex countsMutex;

public:
    // Try to get a shared item. Returns nullptr if item shouldn't be deduplicated.
    static Item* getSharedItem(const Item& item);
    
    // Get statistics
    static size_t getMemorySaved();
    static size_t getSharedCount();
    static size_t getTotalRequests();
    static double getHitRate();
    
    // Reset pool
    static void clear();
    static bool isEnabled();
    
    // Report an item occurrence (call during map loading)
    static void reportOccurrence(uint16_t itemId);
};

#endif
