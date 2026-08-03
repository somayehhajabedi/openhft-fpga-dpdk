

$ numactl --hardware 


available: 1 nodes (0)
node 0 cpus: 0 1 2 3 4 5 6 7
node 0 size: 15133 MB
node 0 free: 1345 MB
node distances:
node     0 
   0:   10 

////////////////////////////////////////////

$numastat

                           node0
numa_hit              4461008844
numa_miss                      0
numa_foreign                   0
interleave_hit              2193
local_node            4461008849
other_node                     0

/////////////////////////////////////////////

$ lscpu | grep -E "NUMA|Socket|Core|Thread"


Model name:                              Intel(R) Core(TM) i7-10510U CPU @ 1.80GHz
Thread(s) per core:                      2
Core(s) per socket:                      4
Socket(s):                               1
NUMA node(s):                            1
NUMA node0 CPU(s):                       0-7

////////////////////////////////////////////////

$ numastat -m


Per-node system memory usage (in MBs):
                          Node 0           Total
                 --------------- ---------------
MemTotal                15133.94        15133.94
MemFree                   902.97          902.97
MemUsed                 14230.97        14230.97
SwapCached                149.65          149.65
Active                   8820.59         8820.59
Inactive                 3100.35         3100.35
Active(anon)             7663.46         7663.46
Inactive(anon)            680.41          680.41
Active(file)             1157.14         1157.14
Inactive(file)           2419.95         2419.95
Unevictable               580.14          580.14
Mlocked                     0.05            0.05
Dirty                       0.46            0.46
Writeback                   0.00            0.00
FilePages                6471.86         6471.86
Mapped                   1071.77         1071.77
AnonPages                6122.80         6122.80
Shmem                    2745.38         2745.38
KernelStack                25.20           25.20
PageTables                113.73          113.73
SecPageTables               3.20            3.20
NFS_Unstable                0.00            0.00
Bounce                      0.00            0.00
WritebackTmp                0.00            0.00
KReclaimable              501.38          501.38
Slab                      835.77          835.77
SReclaimable              501.38          501.38
SUnreclaim                334.40          334.40
AnonHugePages               0.00            0.00
ShmemHugePages              0.00            0.00
ShmemPmdMapped              0.00            0.00
FileHugePages             126.00          126.00
FilePmdMapped               0.00            0.00
Unaccepted                  0.00            0.00
HugePages_Total           256.00          256.00
HugePages_Free            256.00          256.00
HugePages_Surp              0.00            0.00

///////////////////////////////////////

ldconfig -p | grep libnuma
dpkg -l | grep libnuma-dev
	libnuma.so.1 (libc6,x86-64) => /usr/lib/x86_64-linux-gnu/libnuma.so.1
	libnuma.so (libc6,x86-64) => /usr/lib/x86_64-linux-gnu/libnuma.so
ii  libnuma-dev:amd64                          2.0.19-1build1                             amd64        Development files for libnuma


//////////////////////////////////////


$ ./build-profile/benchmarks/numa_benchmark \
    --benchmark_min_time=1s

    -----------------------------------------------------------------------------
Benchmark                   Time             CPU   Iterations UserCounters...
-----------------------------------------------------------------------------
BM_StandardMemory       0.688 ms        0.688 ms         2104 bytes_per_second=363.516Gi/s
BM_NumaNode0Memory      0.714 ms        0.714 ms         1883 bytes_per_second=350.106Gi/s

////////////////////////////////////////


$ ./build-profile/benchmarks/numa_benchmark \
    --benchmark_repetitions=5 \
    --benchmark_report_aggregates_only=true \
    --benchmark_min_time=1s








