gcc main.c pagepatrol.c -o microemu

testn=3
iter=30
nmetas=2
nfiles=5

sudo sync    
echo 3 | sudo tee /proc/sys/vm/drop_caches

echo "=================================================================="

vmtouch -e ~/bench/test2/metafile
vmtouch -e ~/bench/test2/metafile2
vmtouch -e ~/bench/test2/file1
vmtouch -e ~/bench/test2/file2
vmtouch -e ~/bench/test2/file3
vmtouch -e ~/bench/test2/file4
vmtouch -e ~/bench/test2/file5
(time ./microemu $testn $iter 1 $nmetas $nfiles ~/bench/test2/metafile ~/bench/test2/metafile2 ~/bench/test2/file1 ~/bench/test2/file2 ~/bench/test2/file3 ~/bench/test2/file4 ~/bench/test2/file5) &
sleep 1
rm -rf test3_mrulog
./pf_monitor.sh > test3_mrulog
vmtouch -v ~/bench/test2/metafile
vmtouch -v ~/bench/test2/metafile2

echo "=================================================================="

sudo sync    
echo 3 | sudo tee /proc/sys/vm/drop_caches

echo ""
echo "=================================================================="

vmtouch -e ~/bench/test2/metafile
vmtouch -e ~/bench/test2/metafile2
vmtouch -e ~/bench/test2/file1
vmtouch -e ~/bench/test2/file2
vmtouch -e ~/bench/test2/file3
vmtouch -e ~/bench/test2/file4
vmtouch -e ~/bench/test2/file5
(time ./microemu $testn $iter 0 $nmetas $nfiles ~/bench/test2/metafile ~/bench/test2/metafile2 ~/bench/test2/file1 ~/bench/test2/file2 ~/bench/test2/file3 ~/bench/test2/file4 ~/bench/test2/file5) &
sleep 1
rm -rf test3_lrulog
./pf_monitor.sh > test3_lrulog
vmtouch -v ~/bench/test2/metafile
vmtouch -v ~/bench/test2/metafile2

echo "=================================================================="
