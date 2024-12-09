gcc main.c pagepatrol.c -o microemu

iter=70

sudo sync    
echo 3 | sudo tee /proc/sys/vm/drop_caches

echo "=================================================================="

vmtouch -e ~/bench/test2/metafile
vmtouch -e ~/bench/test2/file1
vmtouch -e ~/bench/test2/file2
vmtouch -e ~/bench/test2/file3
vmtouch -e ~/bench/test2/file4
vmtouch -e ~/bench/test2/file5
(time ./microemu 2 $iter 1 ~/bench/test2/metafile ~/bench/test2/file1 ~/bench/test2/file2 ~/bench/test2/file3 ~/bench/test2/file4 ~/bench/test2/file5) &
sleep 1
rm -rf test2_mrulog
./pf_monitor.sh > test2_mrulog
vmtouch -v ~/bench/test2/metafile

echo "=================================================================="

sudo sync    
echo 3 | sudo tee /proc/sys/vm/drop_caches

echo ""
echo "=================================================================="

vmtouch -e ~/bench/test2/metafile
vmtouch -e ~/bench/test2/file1
vmtouch -e ~/bench/test2/file2
vmtouch -e ~/bench/test2/file3
vmtouch -e ~/bench/test2/file4
vmtouch -e ~/bench/test2/file5
(time ./microemu 2 $iter 0 ~/bench/test2/metafile ~/bench/test2/file1 ~/bench/test2/file2 ~/bench/test2/file3 ~/bench/test2/file4 ~/bench/test2/file5) &
sleep 1
rm -rf test2_lrulog
./pf_monitor.sh > test2_lrulog
vmtouch -v ~/bench/test2/metafile

echo "=================================================================="
