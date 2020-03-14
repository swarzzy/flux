#pragma once

//
// TODO: Itartors
//

template<typename T>
struct HashBucket {
    u32 key;
    T value;
};

template<typename T>
struct HashMap {
    u32 entryCount;
    HashBucket<T> table[128];
};

template<typename T>
struct HashMapIter
{
    HashMap<T>* map;
    u32 at;

    inline HashMapIter<T>& operator++() {
        do {
            at++;
        } while (!map->table[at].key);

        return *this;
    }

    bool operator!=(HashMapIter<T> const& other) const {
        assert(this->map == other.map);
        return at != other.at;
    }

    T& operator*() {
        return map->table[at].value;
    }
};

template<typename T>
inline HashMapIter<T> begin(HashMap<T>& map) {
    HashMapIter<T> iter = {};
    iter.map = &map;
    return iter;
}

template<typename T>
inline HashMapIter<T> end(HashMap<T>& map) {
    HashMapIter<T> iter = {};
    iter.map = &map;
    iter.at = array_count(map.table);
    return iter;
}

template<typename T>
T* Add(HashMap<T>* map, u32 key);

template<typename T>
T* Get(HashMap<T>* map, u32 key);

template<typename T>
bool Delete(HashMap<T>* map, u32 key);
