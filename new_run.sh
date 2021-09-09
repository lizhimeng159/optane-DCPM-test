gcc aep_test.c -o result -O0 -lpmem -lpthread


# media_type   thread_count   block_count   block_size
./result   0   1  134217728   8
sleep 1

echo "   "
./result   0   1   4194304   256
sleep 1

echo "   "
./result   0   1   262144   4096
sleep 1




./result   0   4  134217728   8
sleep 1

echo "   "
./result   0   4   4194304   256
sleep 1

echo "   "
./result   0   4   262144   4096
sleep 1




./result   0   8  134217728   8
sleep 1

echo "   "
./result   0   8   4194304   256
sleep 1

echo "   "
./result   0   8   262144   4096
sleep 1




./result   0   12  134217728   8
sleep 1

echo "   "
./result   0   12  4194304   256
sleep 1

echo "   "
./result   0   12   262144   4096
sleep 1




./result   1   16  134217728   8
sleep 1

echo "   "
./result   1   16   4194304   256
sleep 1

echo "   "
./result   1   16   262144   4096
sleep 1

