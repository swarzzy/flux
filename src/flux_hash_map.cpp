#include "flux_hash_map.h"

hash_map_template_decl
hash_bucket_teamplate* FindEntry(hash_map_template* map, Key* key, bool searchForEmpty) {
    hash_bucket_teamplate* result = nullptr;
    u32 hashMask = map->size - 1;
    u32 hash = HashFunction(key);
    auto index = hash & hashMask;
    for (u32 offset = 0; offset < map->size; offset++) {
        u32 index = (index + offset) & hashMask;
        auto entry = map->table + index;
        if (searchForEmpty && (!entry->used)) {
            result = entry;
            break;
        } else if (CompareFunction(key, &entry->key)) {
            result = entry;
            break;
        }
    }
    return result;
}

hash_map_template_decl
Value* Add(hash_map_template* map, Key* key) {
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

hash_map_template_decl
Value* Get(hash_map_template* map, Key* key) {
    Value* result = nullptr;
    if (key) {
        auto entry = FindEntry(map, key, false);
        if (entry) {
            result = &entry->value;
        }
    }
    return result;
}


hash_map_template_decl
bool Delete(hash_map_template* map, Key* key) {
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
