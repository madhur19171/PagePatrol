#!/bin/bash

sudo blockdev --setra 0 /dev/nvme0n1

sudo sync    # Write back data to disk
echo 3 | sudo tee /proc/sys/vm/drop_caches      # Clear the Page Cache

sleep 3

sudo ./randomWalk > output.txt
