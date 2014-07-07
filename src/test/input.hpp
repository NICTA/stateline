/**
 * Contains common testing utility functions
 *
 * @file common.hpp
 * @author Nahid Akbar
 * @date 2014-06-10
 * @license General Public License version 3 or later
 * @copyright (c) 2013, NICTA
 */

#pragma once

#include "common.hpp"
#include "input/input.hpp"

namespace obsidian
{
  class InputTest: public ::testing::Test
  {
  };

  template<ForwardModel f>
  class testInitOptions
  {
  public:
    testInitOptions()
    {
      po::options_description options;
      initSensorInputFileOptions<f>(options);
      std::string enabled_flag_name = configHeading<f>() + ".enabled";
      std::string found_flag_name = options.find(enabled_flag_name, true, true, true).long_name();
      EXPECT_EQ(found_flag_name, enabled_flag_name);
    }
  };

  TEST_F(InputTest, testInitOptions)
  {
    applyToSensors<testInitOptions>();
  }

  template<typename T>
  inline void testCSV(const T & original, std::function<void(po::options_description&)> initOptions,
                      std::function<T(const po::variables_map&, const std::set<ForwardModel>&)> read_, std::string configHeading)
  {
    std::set<ForwardModel> s;
    s.insert(ForwardModel::GRAVITY);
    s.insert(ForwardModel::MAGNETICS);
    s.insert(ForwardModel::MTANISO);
    s.insert(ForwardModel::SEISMIC1D);
    s.insert(ForwardModel::CONTACTPOINT);
    s.insert(ForwardModel::THERMAL);
    po::options_description options;
    initOptions(options);
    std::string prefix = "test_" + configHeading;
    po::variables_map vm = write<T>(prefix, original, options);
    T read = read_(vm, s);
    EXPECT_EQ(original, read);
  }

  template<ForwardModel f> void testSpecCSV(const typename Types<f>::Spec & original)
  {
    testCSV<typename Types<f>::Spec>(original, initSensorInputFileOptions<f>, parseSpec<typename Types<f>::Spec>,
                                     configHeading<f>());
  }

  template<ForwardModel f> void testResultsCSV(typename Types<f>::Results original)
  {
    original.likelihood = 0;
    testCSV<typename Types<f>::Results>(original, initSensorInputFileOptions<f>,
                                        parseSensorReadings<typename Types<f>::Results>, configHeading<f>());
  }
  template<ForwardModel f> void testParamsCSV(typename Types<f>::Params original)
  {
    testCSV<typename Types<f>::Params>(original, initSensorInputFileOptions<f>,
                                       parseSimulationParams<typename Types<f>::Params>, configHeading<f>());
  }

}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
