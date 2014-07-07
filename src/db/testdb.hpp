//!
//!
//! \file db/testdb.hpp
//! \author Lachlan McCalman
//! \date 2014
//! \license Affero General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include <glog/logging.h>
#include "gtest/gtest.h"
#include "boost/filesystem.hpp"

#include "app/settings.hpp"
#include "db/db.hpp"

namespace stateline
{
  namespace db
  {
    class DB: public testing::Test
    {
    public:
      DBSettings settings;
      std::string path = "./AUTOGENtestDB";
      DB()
      {
        settings.directory = path;
        settings.recover = false;
        settings.cacheSizeMB = 1.0;
        boost::filesystem::remove_all(path);
      }
      ~DB()
      {
        boost::filesystem::remove_all(path);
      }
    };

  TEST_F(DB, helloWorld)
  {
    Database db(settings);
    db.put("hello", "world");
    std::string result = db.get("hello");
    EXPECT_EQ(result,"world");
  }

  TEST_F(DB, swap)
  {
    Database db(settings);
    db.put("key1", "val1");
    db.put("key2", "val2");
    db.swap(
        { "key1"},
        { "key2"});
    std::string result1 = db.get("key1");
    std::string result2 = db.get("key2");
    EXPECT_EQ(result1,"val2");
    EXPECT_EQ(result2,"val1");
  }
}
 // namespace db
}// namespace obsidian
