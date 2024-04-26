#ifndef COSICOIN_DATABASE_H
#define COSICOIN_DATABASE_H

#include <string>

//TODO: include the static library of levelDB

namespace DB {

    const std::string db_name = "../mydb";

    leveldb::Options get_options() {
        leveldb::Options options;
        options.create_if_missing = true;
        return options;
    }

    // a simple ensapulation of a Leveldb, handy for usage
    leveldb::DB* init_db(leveldb::Options&& options) {
        leveldb::DB* db;
        leveldb::Status status = leveldb::DB::Open(options, db_name, &db);
        assert(status.ok());
        return db;
    }

}

#endif