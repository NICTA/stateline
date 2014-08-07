import stateline.comms as sl
import random

def monte_carlo(n):
    count = 0
    for _ in range(n):
        x, y = random.random(), random.random()
        if x * x + y * y < 1:
            count += 1
    return count

worker = sl.Worker("localhost:5555")
num_samples = worker.global_spec

minion = sl.Minion(worker)

for job in minion.jobs():
    minion.submit(monte_carlo(num_samples))