gcc main.c pagepatrol.c -o microemu

iter=1

sudo sync    
echo 3 | sudo tee /proc/sys/vm/drop_caches

echo "=================================================================="

vmtouch -e ~/bench/bigfile
time ./microemu 4 ~/bench/bigfile 1 $iter 1 &
sleep 1
rm -rf mrulog
./pf_monitor.sh > test4_mrulog
vmtouch -v ~/bench/bigfile

echo "=================================================================="
echo ""
echo "=================================================================="

# vmtouch -e ~/bench/bigfile
# time ./microemu 1 ~/bench/bigfile 1 $iter 0 &
# sleep 1
# rm -rf lrulog
# ./pf_monitor.sh > test1_lrulog
# vmtouch -v ~/bench/bigfile

echo "=================================================================="
