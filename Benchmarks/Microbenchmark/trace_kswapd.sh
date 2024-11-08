#!/bin/bash

# Define output file
TRACE_OUTPUT="kswapd_trace.log"

# Enable function tracing
echo "function" > /sys/kernel/debug/tracing/current_tracer

# Set filter to only trace kswapd functions
echo "kswapd" > /sys/kernel/debug/tracing/set_ftrace_filter

# Start tracing
echo 1 > /sys/kernel/debug/tracing/tracing_on
echo "Tracing kswapd... Press Enter to stop and dump the trace."

# Wait for user input to stop tracing
read -r -p ""

# Stop tracing
echo 0 > /sys/kernel/debug/tracing/tracing_on

# Dump trace to file
cat /sys/kernel/debug/tracing/trace > "$TRACE_OUTPUT"
echo "Trace saved to $TRACE_OUTPUT"

# Clear the trace to free up buffer
echo > /sys/kernel/debug/tracing/trace
echo "Trace cleared from the tracing buffer."

# Reset ftrace settings
echo "nop" > /sys/kernel/debug/tracing/current_tracer
echo > /sys/kernel/debug/tracing/set_ftrace_filter

echo "Tracing complete."
