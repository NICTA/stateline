#include "server.hpp"
#include "csv.hpp"

#include <iostream>

namespace sl = stateline;

int main(int ac, char *av[])
{
  // Use the default settings with one dimension and 4 chains
  auto settings = sl::StatelineSettings::fromDefault(1, { "job" }, 4);

  // Initialise a server on port 5555
  sl::Server server{5555, settings};

  std::cout << "Collecting 1000 samples from each chain" << std::endl;
  auto samples = server.step(1000);

  // Save the samples from each chain to a CSV file in a folder called output
  sl::saveToCSV(samples, "output");

  return 0;
}
