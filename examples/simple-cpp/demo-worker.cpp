#include "worker.hpp"

#include <iostream>

double gaussianNLL(const std::string& jobType, const std::vector<double>& x)
{
  std::cout << "got job " << jobType << ", x = " << x[0] << std::endl;
  double squaredNorm = 0.0;
  for (auto i : x)
  {
    squaredNorm += i * i;
  }
  return 0.5 * squaredNorm;
}

int main(int argc, char *argv[])
{
  // Only 1 job type for this demo
  std::vector<std::string> jobTypes = { "job" };

  // Initialise a worker with the our likelihood function and use 4 threads.
  // Blocks until the worker is no longer needed.
  stateline::runWorkers(gaussianNLL, "localhost:5555", jobTypes, 4);

  return 0;
}
