#pragma once
#include "include/seastarkv.hh"
#include "include/reply_builder.hh"
#include <cstdio>
#include <fstream>
#include <chrono>
#include <boost/thread/shared_mutex.hpp>

using namespace redis;
using namespace std;

// Thomas Wang, Integer Hash Functions.
// http://www.concentric.net/~Ttwang/tech/inthash.htm
inline uint32_t get_hash(uint32_t key, uint32_t seed) {
    uint32_t hash = key;
    hash = hash ^ seed;
    hash = ~hash + (hash << 15);  // hash = (hash << 15) - hash - 1;
    hash = hash ^ (hash >> 12);
    hash = hash + (hash << 2);
    hash = hash ^ (hash >> 4);
    hash = hash * 2057;  // hash = (hash + (hash << 3)) + (hash << 11);
    hash = hash ^ (hash >> 16);
    return hash & 0x3fffffff;
}

class database;

struct db_val {
    uint32_t key;
    struct db_val *next;
    void* data;
    uint32_t length;
}__attribute__((__packed__));

typedef struct db_val db_val_t;

struct node
{
    void* data;
    node *next;
};

class database {
public:
    typedef std::chrono::time_point<std::chrono::high_resolution_clock> ts;
    typedef std::chrono::duration<size_t, std::nano> ns;

    struct hashtable {
        boost::shared_mutex _access;
        uint32_t size;
        struct db_val **table;
    };

    typedef struct hashtable hashtable_t;

    void hashtable_init(uint32_t size);

    void ht_set(db_val_t* val);

    db_val_t* ht_get(uint32_t key);

    database()
    {
        ht.table = NULL;
	hashtable_init(1000*1000);
    }

    ~database() {
    }; 

    db_val** get_table_direct(void);

private:
    hashtable_t ht;
};

extern unordered_map<string, database*> db_map;

inline database* get_db(std::string s) {
    if (db_map.find(s) == db_map.end())
        return NULL;
    return db_map[s];
}

inline bool create_db(std::string s) {
    auto db = new database;
    if (!db)
        return false;
    db_map[s] = db;
    return true;
}
