//!
//! Contains implementation of the database structure used for persistent
//! storage of the MCMC data.
//!
//! \file db/db.cpp
//! \author Lachlan McCalman
//! \date 2014
//! \license Affero General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include "db/db.hpp"

#include <glog/logging.h>

#include "leveldb/write_batch.h"
#include "leveldb/cache.h"
#include "leveldb/options.h"
#include "leveldb/filter_policy.h"

namespace stateline
{
  namespace db
  {
    Database::Database(const DBSettings& s, bool recover)
        : cacheNumBytes_(s.cacheSizeMB * 1024 * 1024)
    {
      options_.block_cache = leveldb::NewLRUCache(cacheNumBytes_); // Cache in MB
      options_.filter_policy = leveldb::NewBloomFilterPolicy(10); // smart filtering -- bits per key?
      if (!recover)
      {
        options_.error_if_exists = true;
        options_.create_if_missing = true;
      } else
      {
        options_.error_if_exists = false;
        options_.create_if_missing = false;
      }

      leveldb::Status status = leveldb::DB::Open(options_, s.directory, &db_);
      if (!status.ok())
      {
        if (recover)
        {
          LOG(ERROR)<< "Could not recover database. Check the path in the config file and disk write permissions";
        }
        else
        {
          LOG(ERROR) << "Could not create database. Database may already exist and would be overwritten";
        }
        exit(EXIT_FAILURE);
      }

      writeOptions_.sync = false;
    }

    Database::Database(Database&& other)
      : settings_(other.settings_), cacheNumBytes_(other.cacheNumBytes_),
        db_(other.db_), options_(other.options_),
        writeOptions_(other.writeOptions_),
        readOptions_(other.readOptions_)
    {
      // Set the other's DB pointer to null to steal ownership
      other.db_ = NULL;
    }

    DBSettings Database::settings() const
    {
      return settings_;
    }

    uint Database::cacheSize() const
    {
      return cacheNumBytes_;
    }

    std::string Database::get(const leveldb::Slice& key) const
    {
      std::string s;
      leveldb::Status status = db_->Get(readOptions_, key, &s);
      CHECK(status.ok()) << "key is " << key.ToString();
      return s;
    }

    void Database::put(const leveldb::Slice& key, const leveldb::Slice& value)
    {
      leveldb::Status status = db_->Put(writeOptions_, key, value);
      CHECK(status.ok());
    }

    void Database::put(leveldb::WriteBatch& batch)
    {
      db_->Write(writeOptions_, &batch);
    }

    void Database::remove(const leveldb::Slice& key)
    {
      leveldb::Status status = db_->Delete(writeOptions_, key);
      CHECK(status.ok());
    }

    void Database::swap(const std::vector<std::string>& keys1, const std::vector<std::string>& keys2)
    {
      CHECK_EQ(keys1.size(), keys2.size());
      uint size = keys1.size();
      leveldb::WriteBatch batch;
      std::vector<std::string> values1(size);
      std::vector<std::string> values2(size);

      for (uint i = 0; i < keys1.size(); i++)
      {
        std::string value1;
        std::string value2;
        leveldb::Status s1 = db_->Get(readOptions_, keys1[i], &value1);
        leveldb::Status s2 = db_->Get(readOptions_, keys2[i], &value2);
        CHECK(s1.ok() && s2.ok()) << s1.ToString() << " " << s2.ToString();
        values1[i] = value2;
        values2[i] = value1;
        batch.Put(keys1[i], values1[i]);
        batch.Put(keys2[i], values2[i]);
      }

      db_->Write(writeOptions_, &batch);
    }

    Database::~Database()
    {
      if (db_ != NULL)
      {
        delete db_;
        delete options_.block_cache;
      }
    }

  } // namespace db
} // namespace obsidian

