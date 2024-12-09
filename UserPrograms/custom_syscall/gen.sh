# gen all files used in test1.sh test2.sh test3. test4.sh

mkdir ~/bench
mkdir ~/bench/test2

dd if=/dev/random of=~/bench/bigfile bs=1GB count=1

dd if=/dev/random of=~/bench/test2/metafile bs=100MB count=1
dd if=/dev/random of=~/bench/test2/metafile2 bs=100MB count=1
dd if=/dev/random of=~/bench/test2/file1 bs=1000MB count=1
dd if=/dev/random of=~/bench/test2/file2 bs=1000MB count=1
dd if=/dev/random of=~/bench/test2/file3 bs=1000MB count=1
dd if=/dev/random of=~/bench/test2/file4 bs=1000MB count=1
dd if=/dev/random of=~/bench/test2/file5 bs=1000MB count=1
