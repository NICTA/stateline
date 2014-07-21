import stateline as sl

settings = sl.DelegatorSettings()
settings.poll_rate = 100
settings.port = 5555
settings.heartbeat = sl.HeartbeatSettings()
settings.heartbeat.rate = 1000
settings.heartbeat.poll_rate = 500
settings.heartbeat.timeout = 10000

globalSpec = ""
jobSpec = { 0: "" }

delegator = sl.Delegator(globalSpec, jobSpec, settings)
delegator.start()

requester = sl.Requester(delegator)

for i in range(0, 100):
  print 'Sending ping...'
  job = sl.JobData()
  job.type = 0
  job.global_data = ""
  job.job_data = "ping " + str(i)

  requester.submit(0, job)
  _, result = requester.retrieve()
  print 'Got: ', result.data
