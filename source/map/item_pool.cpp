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

#include "item_pool.h"
#include "game/complexitem.h"
#include "app/settings.h"

// Static member definitions
std::vector<std::shared_ptr<Item>> ItemPool::commonPool;
std::mutex ItemPool::poolMutex;
std::atomic<size_t> ItemPool::memorySaved{0};
std::atomic<size_t> ItemPool::sharedCount{0};
std::atomic<size_t> ItemPool::totalRequests{0};
std::vector<size_t> ItemPool::occurrenceCounts;
std::mutex ItemPool::countsMutex;

// Maximum item ID to consider for deduplication
// Cover all possible item IDs (uint16_t max is 65535, but items are usually < 50000)
static constexpr uint16_t MAX_COMMON_ITEM_ID = 50000;

// Check if an item can be deduplicated - STRICT criteria
bool canDeduplicateItem(Item* item) {
    if (!item) {
        return false;
    }

    // Only deduplicate items within valid range
    uint16_t itemId = item->getID();
    if (itemId == 0 || itemId >= MAX_COMMON_ITEM_ID) {
        return false;
    }

    // Items with unique IDs cannot be shared
    if (item->getUniqueID() != 0) {
        return false;
    }

    // Items with action IDs cannot be shared
    if (item->getActionID() != 0) {
        return false;
    }

    // Containers cannot be shared (they have unique contents)
    if (item->getContainer() != nullptr) {
        return false;
    }

    // Teleports cannot be shared (they have unique destinations)
    if (item->getTeleport() != nullptr) {
        return false;
    }

    // Doors cannot be shared (they have unique door IDs)
    if (item->getDoor() != nullptr) {
        return false;
    }

    // Depots cannot be shared (they have unique depot IDs)
    if (item->getDepot() != nullptr) {
        return false;
    }

    // Items with subtype other than 1 cannot be shared (stacked items, fluids, charges)
    if (item->getSubtype() != 1) {
        return false;
    }

    // Items with text cannot be shared
    if (!item->getText().empty()) {
        return false;
    }

    // Items with description cannot be shared
    if (!item->getDescription().empty()) {
        return false;
    }

    // Complex items (with additional attributes) cannot be shared
    if (item->isComplex()) {
        return false;
    }

    return true;
}

// Report an item occurrence during map loading
void ItemPool::reportOccurrence(uint16_t itemId) {
    if (itemId >= MAX_COMMON_ITEM_ID) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(countsMutex);
    if (occurrenceCounts.empty()) {
        occurrenceCounts.resize(MAX_COMMON_ITEM_ID, 0);
    }
    
    occurrenceCounts[itemId]++;
}

// Get a shared item from the pool
Item* ItemPool::getSharedItem(const Item& item) {
    totalRequests.fetch_add(1, std::memory_order_relaxed);
    
    if (!g_settings.getBoolean(Config::ITEM_DEDUPLICATION_ENABLED)) {
        return nullptr;
    }

    // Quick rejection: only handle common items
    uint16_t itemId = item.getID();
    if (itemId >= MAX_COMMON_ITEM_ID) {
        return nullptr;
    }
    
    // Check if we've seen enough occurrences to justify deduplication
    size_t count = 0;
    {
        std::lock_guard<std::mutex> lock(countsMutex);
        if (!occurrenceCounts.empty() && itemId < occurrenceCounts.size()) {
            count = occurrenceCounts[itemId];
        }
    }
    if (count < MIN_OCCURRENCES_THRESHOLD) {
        return nullptr;
    }

    // Full eligibility check
    Item* mutableItem = const_cast<Item*>(&item);
    if (!canDeduplicateItem(mutableItem)) {
        return nullptr;
    }

    std::lock_guard<std::mutex> lock(poolMutex);
    
    // Initialize pool if needed
    if (commonPool.empty()) {
        commonPool.resize(MAX_COMMON_ITEM_ID);
    }

    // Check if we already have this item in the pool
    if (commonPool[itemId]) {
        sharedCount.fetch_add(1, std::memory_order_relaxed);
        return commonPool[itemId].get();
    }

    // Create new shared item - only create once per item ID
    // Note: We create a deep copy to ensure the pool owns the item
    Item* newItem = static_cast<Item*>(item.deepCopy());
    
    if (newItem) {
        commonPool[itemId] = std::shared_ptr<Item>(newItem);
        
        // Memory saved: each subsequent use saves sizeof(Item) - sizeof(shared_ptr)
        // But we don't count the first creation
        memorySaved.fetch_add(sizeof(Item), std::memory_order_relaxed);
    }

    return newItem;
}

// Get total memory saved by using shared items
size_t ItemPool::getMemorySaved() {
    return memorySaved.load();
}

// Get total count of shared items (actual deduplications)
size_t ItemPool::getSharedCount() {
    return sharedCount.load();
}

// Get total number of requests
size_t ItemPool::getTotalRequests() {
    return totalRequests.load();
}

// Get hit rate percentage
double ItemPool::getHitRate() {
    size_t total = totalRequests.load();
    if (total == 0) return 0.0;
    return (static_cast<double>(sharedCount.load()) / total) * 100.0;
}

// Clear the pool
void ItemPool::clear() {
    std::lock_guard<std::mutex> lock(poolMutex);
    {
        std::lock_guard<std::mutex> countsLock(countsMutex);
        occurrenceCounts.clear();
    }
    commonPool.clear();
    memorySaved = 0;
    sharedCount = 0;
    totalRequests = 0;
}

// Check if item pooling is enabled
bool ItemPool::isEnabled() {
    return g_settings.getBoolean(Config::ITEM_DEDUPLICATION_ENABLED);
}
