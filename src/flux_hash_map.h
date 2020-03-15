#pragma once

//
// TODO: Itartors
//

template<typename Key, typename Value>
struct HashBucket {
    b32 used;
    Key key;
    Value value;
};

template<typename Key, typename Value>
struct HashMap {
    typedef u32(HashFunctionFn)(Key*);
    typedef bool(CompareFunctionFn)(Key*, Key*);

    HashFunctionFn* HashFunction;
    CompareFunctionFn* CompareFunction;
    u32 entryCount;
    HashBucket<Key, Value> table[128];

    static HashMap Make(HashFunctionFn* hash, CompareFunctionFn* comp) {
        return HashMap { hash, comp };
    }
};

template<typename Key, typename Value>
struct HashMapIter
{
    HashMap<Key, Value>* map;
    u32 at;

    inline HashMapIter<Key, Value>& operator++() {
        do {
            at++;
        } while (!(map->table[at].used));

        return *this;
    }

    bool operator!=(HashMapIter<Key, Value> const& other) const {
        assert(this->map == other.map);
        return at != other.at;
    }

    Value& operator*() {
        return map->table[at].value;
    }
};

template<typename Key, typename Value>
    inline HashMapIter<Key, Value> begin(HashMap<Key, Value>& map) {
    HashMapIter<Key, Value> iter = {};
    u32 at = 0;
    do {
        at++;
    } while (!(map.table[at].used));
    iter.map = &map;
    iter.at = at;
    return iter;
}

template<typename Key, typename Value>
inline HashMapIter<Key, Value> end(HashMap<Key, Value>& map) {
    HashMapIter<Key, Value> iter = {};
    iter.map = &map;
    iter.at = array_count(map.table);
    return iter;
}

template<typename Key, typename Value>
    Value* Add(HashMap<Key, Value>* map, Key* key);

template<typename Key, typename Value>
Value* Get(HashMap<Key, Value>* map, Key* key);

template<typename Key, typename Value>
bool Delete(HashMap<Key, Value>* map, Key* key);
