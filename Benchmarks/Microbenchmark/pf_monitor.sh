#!/bin/bash

# Find the PID of the process named "microbenchmark"
pid=$(pgrep -f "microbenchmark")

# Check if the process was found
if [ -z "$pid" ]; then
  echo "Process 'microbenchmark' not found."
  exit 1
fi

echo "Monitoring page faults for process 'microbenchmark' with PID: $pid"
echo "Time(s)   Minor Page Faults   Major Page Faults"

# Start a counter for elapsed time
counter=0

# Monitor every 500 milliseconds
while kill -0 "$pid" 2> /dev/null; do
  # Get the page faults stats
  stats=$(ps -o min_flt,maj_flt= -p "$pid")

  # Print the time, minor page faults, and major page faults
  echo "$counter    $stats"

  # Wait for 500 milliseconds
  sleep 0.5

  # Increment the counter
  counter=$((counter + 1))
done

echo "Process 'microbenchmark' has terminated."
