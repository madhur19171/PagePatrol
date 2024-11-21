"""
This script analyses data coming from the command:
$ sudo biosnoop-bpfcc -d nvme0n1 > output.txt

! place the output.txt file in the same directory as this script
! to only visualise the accesses of the microbenchmark, set the variables below to the correct values
"""

import pandas as pd
import matplotlib.pyplot as plt

nameProcess = "microdirect"
pidProcess = None

# Read file (where delimiter is any amount of spaces)
df = pd.read_csv('output.txt', sep='\s+', on_bad_lines="skip")

# here we only case about a special proces
if pidProcess:
    df = df[df["PID"] == pidProcess]
if nameProcess:
    df = df[df["COMM"] == nameProcess]
    
df = df[df["T"] == "R"]

# create the plot
plt.scatter(df["TIME(s)"], df["SECTOR"])
plt.xlabel("time (s)")
plt.ylabel("Disk sector")
plt.show()

