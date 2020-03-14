#include "flux_hash_map.h"

template<typename T>
HashBucket<T>* FindEntry(HashMap<T>* map, u32 key, bool searchForEmpty) {
    HashBucket<T>* result = nullptr;
    u32 cmp = searchForEmpty ? 0 : key;
    u32 hashMask = array_count(map->table) - 1;
    // TODO: Real hashing
    auto hash = key & hashMask;
    for (u32 offset = 0; offset < array_count(map->table); offset++) {
        u32 index = (hash + offset) & hashMask;
        auto entry = map->table + index;
        if (entry->key == cmp) {
            result = entry;
            break;
        }
    }
    return result;
}

template<typename T>
T* Add(HashMap<T>* map, u32 key) {
    T* result = nullptr;
    auto entry = FindEntry(map, key, true);
    if (entry) {
        entry->key = key;
        map->entryCount++;
        result = &entry->value;
    }
    return result;
}

template<typename T>
T* Get(HashMap<T>* map, u32 key) {
    T* result = nullptr;
    if (key) {
        auto entry = FindEntry(map, key, false);
        if (entry) {
            result = &entry->value;
        }
    }
    return result;
}


template<typename T>
bool Delete(HashMap<T>* map, u32 key) {
    bool result = false;
    if (key) {
        auto entry = FindEntry(map, key, false);
        if (entry) {
            assert(map->entryCount);
            map->entryCount--;
            entry->key = 0;
            result = true;
        }
    }
    return result;
}
