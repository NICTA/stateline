import stateline.comms as sl
import matplotlib.pyplot as plt
import math

num_trials = 1000
num_samples = 100000

delegator = sl.Delegator(5555, global_spec=num_samples)
delegator.start()

requester = sl.Requester(delegator)

for i in range(num_trials):
    requester.submit(i, [(0, None)])

count = 0
estimates = []
for _, result in requester.retrieve_all():
    count += result[0][1]
    estimates.append((4.0 * count) / ((len(estimates) + 1) * num_samples))

print 'Final estimate:', estimates[-1]

fig, axes = plt.subplots()
axes.axhline(y=math.pi, color='r')
axes.plot(range(len(estimates)), estimates)
plt.show()
