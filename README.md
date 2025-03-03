To compile Shredder:
1. Run ./setup_v8.sh to fetch V8 code and compile V8.
   This takes about 2-3 hours.
2. Run ./setup_seastar.sh to fetch Seastar code and compile Seastar.
   This takes about 1 hour.
3. Run make

To run and test Shredder:
1. Run "./shredder -c 16", in which 16 is the number of cores and can be modified.
