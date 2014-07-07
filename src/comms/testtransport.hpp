//!
//! \file comms/testtransport.cpp
//! \author Lachlan McCalman
//! \date 2014
//! \license Affero General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include <glog/logging.h>
#include "gtest/gtest.h"

#include "comms/messages.hpp"
#include "comms/transport.hpp"
#include "serial/serial.hpp"

#include <Eigen/Core>

namespace stateline
{
  namespace comms
  {

    class Transport: public testing::Test
    {
    public:
      zmq::context_t context_;
      zmq::socket_t requestSocket_;
      zmq::socket_t replySocket_;

      Transport()
          : context_(1), requestSocket_(context_, ZMQ_REQ), replySocket_(context_, ZMQ_REP)
      {
        replySocket_.bind("tcp://*:5555");
        requestSocket_.connect("tcp://localhost:5555");
      }
      ;

      virtual void SetUp()
      {
      }

      virtual void TearDown()
      {
      }
    };

  TEST_F(Transport, sendReceiveString)
  {
    std::string s = "I am a test string.";

    sendString(requestSocket_, s);
    std::string r = receiveString(replySocket_);
    EXPECT_EQ(r, s);
  }

  TEST_F(Transport, sendReceiveMultipartString)
  {
    std::string s1 = "first test string.";
    std::string s2 = "second test string.";
    std::string s3 = "third test string.";

    sendStringPart(requestSocket_, s1);
    sendStringPart(requestSocket_, s2);
    sendString(requestSocket_, s3);
    std::string r1 = receiveString(replySocket_);
    std::string r2 = receiveString(replySocket_);
    std::string r3 = receiveString(replySocket_);
    EXPECT_EQ(r1, s1);
    EXPECT_EQ(r2, s2);
    EXPECT_EQ(r3, s3);
  }

  TEST_F(Transport, sendReceiveMessageAddrSubjData)
  {
    Message m(
        { "1234-1223", "0000-1111"}, stateline::comms::HELLO,
        { "data1", "data2"});
    send(requestSocket_, m);
    auto r = receive(replySocket_);
    EXPECT_EQ(m,r);
  }

  TEST_F(Transport, sendReceiveMessageSubjData)
  {
    Message m(stateline::comms::HELLO,
        { "data1", "data2"});
    send(requestSocket_, m);
    auto r = receive(replySocket_);
    EXPECT_EQ(m,r);
  }

  TEST_F(Transport, sendReceiveMessageAddrSubj)
  {
    Message m(
        { "1234-1223", "0000-1111"}, stateline::comms::HELLO);
    send(requestSocket_, m);
    auto r = receive(replySocket_);
    EXPECT_EQ(m,r);
  }

  TEST_F(Transport, sendReceiveMessageSubj)
  {
    Message m(stateline::comms::HELLO);
    send(requestSocket_, m);
    auto r = receive(replySocket_);
    EXPECT_EQ(m,r);
  }

  TEST_F(Transport, randomSocketIDs)
  {
    auto i1 = randomSocketID();
    auto i2 = randomSocketID();
    EXPECT_NE(i1,i2);
  }

  TEST_F(Transport, setSocketID)
  {
    size_t size=255;
    char r[size];
    std::string id = randomSocketID();
    setSocketID(id, requestSocket_);
    requestSocket_.getsockopt(ZMQ_IDENTITY, (void*)r, &size);
    EXPECT_EQ(std::string(r, size),id);
  }

}
 // namespace comms
}// namespace obsidian
