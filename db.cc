#include <iostream>
#include "include/seastarkv.hh"
#include "include/db.hh"
#include "include/reply_builder.hh"

using namespace std;

unordered_map<string, database*> db_map;

db_val** database::get_table_direct(void)
{
    return ht.table;
}
