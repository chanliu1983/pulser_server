#include "db.h"

Database::Database(const std::string& dbPath) {
    leveldb::Options options;
    options.create_if_missing = true;
    leveldb::Status status = leveldb::DB::Open(options, dbPath, &db_);
    if (!status.ok()) {
        std::cerr << "Failed to open database: " << status.ToString() << std::endl;
    }
}

Database::~Database() {
    delete db_;
}

bool Database::Put(const std::string& key, const std::string& value) {
    leveldb::Status status = db_->Put(leveldb::WriteOptions(), key, value);
    if (!status.ok()) {
        std::cerr << "Failed to put data into database: " << status.ToString() << std::endl;
        return false;
    }
    return true;
}

std::string Database::Get(const std::string& key) {
    std::string value;
    leveldb::Status status = db_->Get(leveldb::ReadOptions(), key, &value);
    if (!status.ok()) {
        std::cerr << "Failed to get data from database: " << status.ToString() << std::endl;
    }
    return value;    
}