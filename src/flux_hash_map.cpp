#include "flux_hash_map.h"

template<typename Key, typename Value>
HashBucket<Key, Value>* FindEntry(HashMap<Key, Value>* map, Key* key, bool searchForEmpty) {
    HashBucket<Key, Value>* result = nullptr;
    u32 hashMask = array_count(map->table) - 1;
    u32 hash = map->HashFunction(key);
    auto index = hash & hashMask;
    for (u32 offset = 0; offset < array_count(map->table); offset++) {
        u32 index = (index + offset) & hashMask;
        auto entry = map->table + index;
        if (searchForEmpty && (!entry->used)) {
            result = entry;
            break;
        } else if (map->CompareFunction(key, &entry->key)) {
            result = entry;
            break;
        }
    }
    return result;
}

template<typename Key, typename Value>
Value* Add(HashMap<Key, Value>* map, Key* key) {
    Value* result = nullptr;
    auto entry = FindEntry(map, key, true);
    if (entry) {
        entry->used = true;
        entry->key = *key;
        map->entryCount++;
        result = &entry->value;
    }
    return result;
}

template<typename Key, typename Value>
Value* Get(HashMap<Key, Value>* map, Key* key) {
    Value* result = nullptr;
    if (key) {
        auto entry = FindEntry(map, key, false);
        if (entry) {
            result = &entry->value;
        }
    }
    return result;
}


template<typename Key, typename Value>
bool Delete(HashMap<Key, Value>* map, Key* key) {
    bool result = false;
    if (key) {
        auto entry = FindEntry(map, key, false);
        if (entry) {
            assert(map->entryCount);
            entry->used = false;
            map->entryCount--;
            result = true;
        }
    }
    return result;
}
