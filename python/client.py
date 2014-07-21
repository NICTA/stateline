import stateline as sl
import time

settings = sl.WorkerSettings()
settings.poll_rate = 100
settings.address = "localhost:5555"
settings.heartbeat = sl.HeartbeatSettings()
settings.heartbeat.rate = 1000
settings.heartbeat.poll_rate = 500
settings.heartbeat.timeout = 3000

jobList = [0]
worker = sl.Worker(jobList, settings)

minion = sl.Minion(worker, 0)

while True:
  job = minion.next_job()
  print 'Got:', job.job_data

  # Fake some computation
  print 'Computing...'

  job_num = int(job.job_data.split()[1])

  result = sl.ResultData()
  result.type = job.type
  result.data = "pong " + str(job_num)

  time.sleep(1)

  minion.submit_result(result)
  print 'Submitted reply'

worker.stop()
