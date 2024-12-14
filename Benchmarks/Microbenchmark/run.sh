#!/bin/bash

sudo sync    # Write back data to disk
echo 3 | sudo tee /proc/sys/vm/drop_caches      # Clear the Page Cache

sudo ./microbenchmark config1.cfg 
