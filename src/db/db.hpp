//!
//! Contains the interface used for persistent storage of the MCMC data.
//!
//! \file db/db.hpp
//! \author Lachlan McCalman
//! \date 2014
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <cassert>

#include "db/settings.hpp"
#include "infer/datatypes.hpp"

#include <iostream>

namespace stateline
{
  namespace db
  {
    std::ostream& operator<<(std::ostream& os, const mcmc::State& s);

    // Database writer interface
    class CSVChainArrayWriter
    {
      public:
        CSVChainArrayWriter(const std::string& directory, uint numChains)
          : chainFiles_(numChains), lastLinePos_(numChains)
        {
          for (uint i = 0; i < numChains; i++) {
            chainFiles_[i].open(directory + "/" + std::to_string(i) + ".csv",
                std::fstream::in | std::fstream::out | std::fstream::trunc);
            assert(chainFiles_[i].is_open());
          }
        }

        void append(int id, const std::vector<mcmc::State>& states)
        {
          // TODO: needs to be transactional
          assert(id >= 0 && id < chainFiles_.size());

          for (const auto& state : states)
          {
            lastLinePos_[id] = chainFiles_[id].tellg();
            chainFiles_[id] << state << "\n";
          }

          chainFiles_[id] << std::flush; // Flush to disk
        }

        void replaceLast(int id, const mcmc::State& state)
        {
          // PRECONDITION: just flushed
          chainFiles_[id].seekg(lastLinePos_[id]);
          chainFiles_[id] << state << "\n";
        }

      private:
        std::vector<std::fstream> chainFiles_;
        std::vector<std::streampos> lastLinePos_;
    };

    /*
    class Database
    {
    public:
      //! Create a new database connection.
      //!
      //! \param s The database settings.
      //!
      Database(const DBSettings& s, bool recover);

      // Move constructor only
      Database(Database&& other);

      //! Clean up and free resources used by the database connection.
      //!
      ~Database();

      //! Get the settings for this database.
      //!
      //! \return The settings passed to the database when it was created.
      //!
      DBSettings settings() const;

      //! Get the cache size.
      //!
      //! \return The cache size in bytes.
      //!
      uint cacheSize() const;

      //! Get the database entry for a particular key.
      //!
      //! \param key The key for the entry.
      //! \return String containing the value of the entry.
      //!
      std::string get(const leveldb::Slice& key) const;

      //! Write an entry to the database.
      //!
      //! \param key The key for the entry.
      //! \param value The value of the entry.
      //!
      void put(const leveldb::Slice& key, const leveldb::Slice& value);

      //! Write to the database batched.
      //!
      //! \param batch The batched keys and values to write.
      //!
      void put(leveldb::WriteBatch& batch);

      //! Remove the entry for a particular key.
      //!
      //! \param key The key for the entry.
      //!
      void remove(const leveldb::Slice& key);

      //! Swap the values for a set of keys with another.
      //!
      //! \param keys1 The first set of keys to swap.
      //! \param keys2 The second set of keys to swap. This must have the same length
      //!              as keys1.
      //!
      //void swap(const std::vector<std::string>& keys1, const std::vector<std::string>& keys2);

    private:
      DBSettings settings_;
      uint cacheNumBytes_;
      leveldb::DB* db_;
      leveldb::Options options_;
      leveldb::WriteOptions writeOptions_;
      leveldb::ReadOptions readOptions_;

    };*/
  } // namespace db
} // namespace stateline
