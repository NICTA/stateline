#include "../comms/minion.hpp"
#include "../comms/worker.hpp"


namespace stateline
{
typedef std::function<double(const std::string&, const std::vector<double>&)> LikelihoodFn;

class WorkerWrapper
{

  public:
    WorkerWrapper(const LikelihoodFn& f, const std::string& address, const std::vector<std::string>& jobTypes, uint nThreads);
    ~WorkerWrapper();
    void start();
    void stop();

  private:
    const LikelihoodFn& f_;
    std::string address_;
    std::vector<std::string> jobTypes_;
    uint nThreads_;
    bool running_;
    zmq::context_t* context_;
    std::future<void> clientThread_;
    std::vector<std::future<void>> wthreads_;
};
}
