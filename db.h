#ifndef DB_H
#define DB_H

#include <leveldb/db.h>
#include <iostream>
#include <string>

class Database {
public:
    Database(const std::string& dbPath);
    ~Database();
    bool Put(const std::string& key, const std::string& value);
    std::string Get(const std::string& key);

private:
    leveldb::DB* db_;
};

#endif // DB_H

