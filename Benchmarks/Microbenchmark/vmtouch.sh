#!/bin/bash

while true; do
  clear
  vmtouch -f -v /mnt/NVMe/bigfile1
  sleep 0.5s
done
