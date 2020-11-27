import launch
import pickle
import copy
import sys

with open(sys.argv[1], "rb") as f:
    brokens = pickle.load(f)

repaired = [ brokens[0] ]

for a,b in zip(brokens[0:-1], brokens[1:]):
    print(a.run_name(), b.run_name())
    cc = copy.deepcopy(b)
    cc.stats.user_time = b.stats.user_time - a.stats.user_time
    cc.stats.system_time = b.stats.system_time - a.stats.system_time
    repaired.append(cc)

print([x.stats.user_time for x in repaired])

with open(sys.argv[2], "wb") as f:
    pickle.dump(repaired,f, -1)