//!
//! \file comms/tests/message.cpp
//! \author Lachlan McCalman
//! \author Darren Shen
//! \date 2015
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include <gtest/gtest.h>

#include "comms/messages.hpp"

using namespace stateline::comms;

TEST(Message, canConstructMessageWithNoData)
{
  std::vector<std::string> address = { "A", "B", "C" };
  Message m{address, HELLO};

  ASSERT_EQ(3U, m.address.size());
  EXPECT_EQ("A", m.address[0]);
  EXPECT_EQ("B", m.address[1]);
  EXPECT_EQ("C", m.address[2]);

  EXPECT_EQ(HELLO, m.subject);
  EXPECT_TRUE(m.data.empty());
}

TEST(Message, canConstructMessageWithData)
{
  std::vector<std::string> address = { "A", "B", "C" };
  std::vector<std::string> data = { "Data1", "Data2", "Data3" };
  Message m{address, HELLO, data};

  ASSERT_EQ(3U, m.address.size());
  EXPECT_EQ("A", m.address[0]);
  EXPECT_EQ("B", m.address[1]);
  EXPECT_EQ("C", m.address[2]);

  EXPECT_EQ(HELLO, m.subject);

  ASSERT_EQ(3U, m.data.size());
  EXPECT_EQ("Data1", m.data[0]);
  EXPECT_EQ("Data2", m.data[1]);
  EXPECT_EQ("Data3", m.data[2]);
}

TEST(Message, canConstructMessageWithNoAddress)
{
  std::vector<std::string> data = { "Data1", "Data2", "Data3" };
  Message m{HELLO, data};

  EXPECT_TRUE(m.address.empty());

  EXPECT_EQ(HELLO, m.subject);

  ASSERT_EQ(3U, m.data.size());
  EXPECT_EQ("Data1", m.data[0]);
  EXPECT_EQ("Data2", m.data[1]);
  EXPECT_EQ("Data3", m.data[2]);
}

TEST(Message, emptyAddressConvertsToStringCorrectly)
{
  std::vector<std::string> address = {};
  EXPECT_EQ("", addressAsString(address));
}

TEST(Message, singleAddressConvertsToStringCorrectly)
{
  std::vector<std::string> address = { "address" };
  EXPECT_EQ("address", addressAsString(address));
}

TEST(Message, addressContainingEmptyEntriesConvertsToStringCorrectly)
{
  std::vector<std::string> address = { "", "A", "", "B1", "" };
  EXPECT_EQ(":B1::A:", addressAsString(address));
}
