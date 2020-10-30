#include <iostream>
#include "include/seastarkv.hh"
#include "include/db.hh"
#include "include/reply_builder.hh"

using namespace std;

unordered_map<string, database*> db_map;

void database::hashtable_init(uint32_t size) {
    ht.table = (db_val**)calloc(size, sizeof(void*));
    assert(ht.table != NULL);
    ht.size = size;
    return;
}

void database::ht_set(db_val_t* val) {
    boost::upgrade_lock<boost::shared_mutex> lock(ht._access);
    val->next = NULL;
    uint32_t hash = get_hash(val->key, 0) % ht.size;
    db_val_t* next = ht.table[hash];
    db_val_t* before = NULL;
    while (next != NULL && next->key != val->key) {
        before = next;
        next = before->next;
    }

    boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);
    if (next != NULL) {
        free(next->data);
        next->data = val->data;
        next->length = val->length;
        free(val);
    } else if (before != NULL){
        before->next = val;
    } else
        ht.table[hash] = val;
}

db_val_t* database::ht_get(uint32_t key) {
    boost::shared_lock<boost::shared_mutex> lock(ht._access);
    uint32_t hash = get_hash(key, 0) % ht.size;
    db_val_t* p = ht.table[hash];
    while (p && p->key != key)
        p = p->next;
    
    db_val_t* ret = NULL;
    if (p) {
        ret = (db_val_t*) malloc(sizeof(db_val_t));
        ret->length = p->length;
        ret->data = malloc(ret->length);
        memcpy(ret->data, p->data, ret->length);
    }
    return ret;
}

db_val** database::get_table_direct(void)
{
    return ht.table;
}
