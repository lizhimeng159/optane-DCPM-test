gcc aep_test.c -o result -O0 -lpmem -lpthread


# media_type   thread_count   block_count   block_size
./result   0   8  134217728   8
sleep 1

echo "   "
./result   0   8  67108864   16
sleep 1

echo "   "
./result   0   8   16777216   64
sleep 1

echo "   "
./result   0   8   4194304   256

echo "   "
./result   0   8   1048576   1024

echo "   "
./result   0   8   262144   4096

echo "   "
./result   0   8   16384   65536

echo "   "
./result   0   8   4096   262144

echo "   "
./result   0   8   1024    1048576


