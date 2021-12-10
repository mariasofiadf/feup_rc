import os
import time

tries = 6
sum = 0
d = []

for i in range(0,tries):
        
    start = time.time()

    os.system("./app r /dev/ttyS11")

    end = time.time()

    diff = end-start
    d.insert(i,diff);
    sum+=diff

print(d)
print(sum/6)