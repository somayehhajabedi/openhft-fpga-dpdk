# Project Command Reference

This document collects the most useful commands used while developing,
testing, profiling, benchmarking, and running the OpenHFT-FPGA-DPDK project.

The goal is to keep a practical project-specific command reference instead of
relying on scattered shell history.

---

# Git

## Show working-tree status

```bash
git status --short
```

Shows modified, staged, and untracked files in compact form.

Useful before staging files to avoid committing unrelated changes.

---

## Show current branch

```bash
git branch --show-current
```

Shows the currently checked-out branch.

---

## Create and switch to a feature branch

```bash
git switch -c feature/my-feature
```

Creates a new branch from the current commit and switches to it.

Example:

```bash
git switch -c feature/tls-round-trip-benchmark
```

---

## Switch branches

```bash
git switch main
```

Switches to the existing `main` branch.

---

## Stage a specific file

```bash
git add README.md
```

Stages only the selected file.

Prefer staging specific files when the working tree contains unrelated changes.

---

## Stage several specific files

```bash
git add \
    benchmarks/CMakeLists.txt \
    benchmarks/ouch_tls_round_trip_benchmark.cpp \
    docs/performance/ouch_tls_round_trip_benchmark.md
```

Useful when multiple files belong to the same feature.

Avoid:

```bash
git add .
```

when unrelated files exist in the working tree.

---

## Show staged changes

```bash
git diff --cached
```

Shows exactly what will be included in the next commit.

---

## Show changes in one file

```bash
git diff -- README.md
```

Useful for reviewing documentation or source changes before staging.

---

## Check whitespace problems

```bash
git diff --check
```

Reports problems such as:

- trailing whitespace
- whitespace errors
- blank lines at end of file

For a specific file:

```bash
git diff --check -- README.md
```

---

## Remove trailing whitespace

```bash
sed -i 's/[[:space:]]*$//' README.md
```

Removes trailing spaces and tabs from every line in the file.

Use carefully on source files where whitespace may be meaningful.

---

## Commit staged changes

```bash
git commit -m "docs: update README with current trading architecture"
```

Creates a commit using the staged files.

Examples:

```bash
git commit -m "perf: add OUCH TLS round-trip latency benchmark"
```

```bash
git commit -m "feat: wire TLS OUCH round-trip runtime"
```

---

## Show latest commit

```bash
git log -1 --oneline
```

Displays the most recent commit in compact form.

---

## Show recent commit history

```bash
git log --oneline --decorate -10
```

Shows the last ten commits together with branch and tag information.

---

## Push a new branch

```bash
git push -u origin feature/my-feature
```

Pushes the branch to GitHub and configures upstream tracking.

Example:

```bash
git push -u origin feature/tls-round-trip-benchmark
```

---

## Push main

```bash
git push origin main
```

Pushes the local `main` branch to GitHub.

---

## Fast-forward merge a feature branch

```bash
git switch main
git merge --ff-only feature/my-feature
```

Performs a fast-forward merge only.

This keeps the history linear when `main` has not diverged.

Example:

```bash
git switch main
git merge --ff-only feature/tls-round-trip-benchmark
```

---

# File and Source Inspection

## Print a file

```bash
cat README.md
```

Prints the complete contents of a file.

Best for small files.

---

## Print part of a file

```bash
sed -n '60,100p' orderbook/software/array_order_book.hpp
```

Prints lines 60 through 100.

Very useful for reviewing a specific section without opening an editor.

---

## Search recursively with ripgrep

```bash
rg "Benchmark round trip failed"
```

Searches recursively through the project.

Useful for finding:

- error messages
- function names
- class names
- constants
- duplicate code

Example:

```bash
rg "AcceptedSize"
```

---

## Search only selected file types

```bash
rg "ExchangeTcpServer" -g '*.cpp' -g '*.hpp'
```

Restricts the search to C++ source and header files.

---

## Find a file by name

```bash
find . -name event_dispatcher.hpp
```

Useful when an include path is unknown.

Example output:

```text
./dispatcher/event_dispatcher.hpp
```

---

## List project files

```bash
find . -maxdepth 2 -type f | sort
```

Shows files near the top of the repository hierarchy.

---

# CMake

## Configure a default build

```bash
cmake -S . -B build
```

Configures the project into the `build/` directory.

---

## Configure Release build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
```

Creates an optimized build.

Use this for meaningful latency benchmarks.

---

## Build the whole project

```bash
cmake --build build -j4
```

Builds all configured targets using four parallel jobs.

---

## Build using all available CPU cores

```bash
cmake --build build -j
```

Lets the build system use available parallelism automatically.

---

## Build one specific target

```bash
cmake --build build --target ouch_tls_round_trip_benchmark -j4
```

Useful when working on only one executable or benchmark.

---

## Clean one build tree

```bash
rm -rf build
```

Removes the complete build directory.

Then reconfigure:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
```

Use when CMake configuration becomes inconsistent.

---

# Ninja

If the project is configured with Ninja:

```bash
cmake -S . -B build -G Ninja
```

Build:

```bash
ninja -C build
```

Build one target:

```bash
ninja -C build ouch_tls_round_trip_benchmark
```

---

# Testing

## Run all tests

```bash
ctest --test-dir build
```

Runs the complete CTest test suite.

---

## Show failing test output

```bash
ctest --test-dir build --output-on-failure
```

Recommended default while debugging failing tests.

---

## Run tests using several jobs

```bash
ctest --test-dir build -j4 --output-on-failure
```

Runs tests in parallel.

---

## List tests without running them

```bash
ctest --test-dir build -N
```

Useful for finding the registered test names.

---

## Run tests matching a name

```bash
ctest --test-dir build -R ouch --output-on-failure
```

Runs only tests whose names match `ouch`.

---

# Trading Runtime

## Replay using recording transport

```bash
./build/apps/trading_runtime \
    --mode replay sample_itch_replay.bin \
    --execution recording
```

Runs the complete replay/strategy path while using the local recording
execution transport.

Useful for verifying that the strategy reaches the execution layer without
requiring an exchange server.

---

## Replay using TLS execution

```bash
./build/apps/trading_runtime \
    --mode replay sample_itch_replay.bin \
    --execution tls \
    --host 127.0.0.1 \
    --port 9000 \
    --ca certs/server.crt
```

Runs the trading runtime using TLS OUCH execution against the exchange
simulator.

---

# Exchange Runtime

## Start TLS exchange simulator

```bash
./build/apps/exchange_runtime \
    --port 9000 \
    --cert certs/server.crt \
    --key certs/server.key
```

Starts the exchange simulator using TLS.

The exchange server uses:

```text
TCP
→ epoll
→ TLS
→ ExchangeOuchHandler
→ MatchingEngine
```

---

# Replay Generation

## Generate sample ITCH replay

```bash
./build/tools/generate_sample_itch_replay
```

Generates the sample replay file used by the trading runtime.

---

# TLS Round-Trip Benchmark

## Build benchmark

```bash
cmake --build build \
    --target ouch_tls_round_trip_benchmark \
    -j4
```

---

## Run benchmark

```bash
./build/benchmarks/ouch_tls_round_trip_benchmark \
    certs/server.crt \
    certs/server.key
```

Measures steady-state OUCH application-level round-trip latency over TLS.

The TLS handshake is performed before the timed region.

---

# OpenSSL and TLS

## Show certificate contents

```bash
openssl x509 \
    -in certs/server.crt \
    -text \
    -noout
```

Displays:

- issuer
- subject
- validity period
- public key
- extensions
- SAN entries

---

## Show only certificate subject

```bash
openssl x509 \
    -in certs/server.crt \
    -noout \
    -subject
```

---

## Show certificate issuer

```bash
openssl x509 \
    -in certs/server.crt \
    -noout \
    -issuer
```

---

## Show certificate validity

```bash
openssl x509 \
    -in certs/server.crt \
    -noout \
    -dates
```

---

## Show Subject Alternative Name

```bash
openssl x509 \
    -in certs/server.crt \
    -noout \
    -ext subjectAltName
```

Useful because TLS client identity verification depends on SAN rather than only
the certificate common name.

---

## Check certificate and private key modulus

```bash
openssl x509 \
    -noout \
    -modulus \
    -in certs/server.crt
```

```bash
openssl rsa \
    -noout \
    -modulus \
    -in certs/server.key
```

The values should correspond to the same key pair.

---

## Inspect a live TLS server

```bash
openssl s_client \
    -connect 127.0.0.1:9000 \
    -CAfile certs/server.crt
```

Useful for debugging TLS connection and certificate problems.

---

# Linux Networking

## Show listening sockets

```bash
ss -ltn
```

Shows listening TCP sockets.

---

## Check whether port 9000 is listening

```bash
ss -ltn | grep 9000
```

---

## Show TCP connections

```bash
ss -tn
```

---

## Show process using a port

```bash
sudo lsof -i :9000
```

Useful when a server cannot bind because the port is already in use.

---

## Test local TCP connectivity

```bash
nc -vz 127.0.0.1 9000
```

Checks whether a TCP connection can be established.

---

# CPU Affinity

## Show available CPUs

```bash
nproc
```

Prints the number of processing units available to the process.

---

## Show CPU topology

```bash
lscpu
```

Useful for identifying:

- physical cores
- logical CPUs
- NUMA nodes
- cache information

---

## Run a process on one CPU

```bash
taskset -c 2 ./build/benchmarks/ouch_tls_round_trip_benchmark \
    certs/server.crt \
    certs/server.key
```

Pins the process to CPU 2.

For multi-threaded latency experiments, explicit per-thread affinity inside the
application is generally more useful.

---

## Show process affinity

```bash
taskset -cp <PID>
```

Example:

```bash
taskset -cp 12345
```

---

# NUMA

## Show NUMA topology

```bash
numactl --hardware
```

---

## Run on one NUMA node

```bash
numactl \
    --cpunodebind=0 \
    --membind=0 \
    ./build/apps/trading_runtime
```

Keeps execution and memory allocation on the same NUMA node.

---

# perf

## Basic profiling

```bash
perf stat ./build/benchmarks/ouch_tls_round_trip_benchmark \
    certs/server.crt \
    certs/server.key
```

Shows high-level hardware and software performance counters.

Typical counters include:

- cycles
- instructions
- branches
- branch misses
- context switches
- page faults

---

## Record CPU profile

```bash
perf record \
    ./build/benchmarks/ouch_tls_round_trip_benchmark \
    certs/server.crt \
    certs/server.key
```

---

## View profile

```bash
perf report
```

Displays the functions consuming CPU time.

---

## Record call graph

```bash
perf record -g \
    ./build/benchmarks/ouch_tls_round_trip_benchmark \
    certs/server.crt \
    certs/server.key
```

Then:

```bash
perf report
```

---

## Inspect context switches

```bash
perf stat \
    -e context-switches,cpu-migrations,page-faults \
    ./build/benchmarks/ouch_tls_round_trip_benchmark \
    certs/server.crt \
    certs/server.key
```

Useful when investigating tail-latency spikes.

---

# Valgrind

## Basic memory check

```bash
valgrind \
    --leak-check=full \
    ./build/apps/trading_runtime
```

Detects:

- memory leaks
- invalid reads
- invalid writes

Do not use Valgrind results as latency measurements because instrumentation
greatly slows execution.

---

# Cachegrind

## Record cache behavior

```bash
valgrind \
    --tool=cachegrind \
    ./build/benchmarks/orderbook_benchmark
```

---

## Inspect Cachegrind results

```bash
cg_annotate cachegrind.out.<PID>
```

Or open the result using KCachegrind.

---

# AddressSanitizer

Configure:

```bash
cmake \
    -S . \
    -B build-asan \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_CXX_FLAGS="-fsanitize=address -fno-omit-frame-pointer" \
    -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address"
```

Build:

```bash
cmake --build build-asan -j4
```

Run tests:

```bash
ctest \
    --test-dir build-asan \
    --output-on-failure
```

---

# UndefinedBehaviorSanitizer

Configure:

```bash
cmake \
    -S . \
    -B build-ubsan \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_CXX_FLAGS="-fsanitize=undefined -fno-omit-frame-pointer" \
    -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=undefined"
```

Build and test:

```bash
cmake --build build-ubsan -j4
ctest --test-dir build-ubsan --output-on-failure
```

---

# ThreadSanitizer

Configure:

```bash
cmake \
    -S . \
    -B build-tsan \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_CXX_FLAGS="-fsanitize=thread -fno-omit-frame-pointer" \
    -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=thread"
```

Build:

```bash
cmake --build build-tsan -j4
```

Useful for detecting data races in multi-threaded code.

---

# GDB

## Start executable under GDB

```bash
gdb ./build/apps/trading_runtime
```

---

## Run with arguments

Inside GDB:

```gdb
run --mode replay sample_itch_replay.bin --execution recording
```

---

## Set breakpoint

```gdb
break ExchangeOuchHandler::handleEnterOrder
```

---

## Continue execution

```gdb
continue
```

---

## Show backtrace

```gdb
bt
```

Useful after crashes or breakpoint stops.

---

# Core Dumps

## Enable core dumps

```bash
ulimit -c unlimited
```

---

## Open a core dump

```bash
gdb ./build/apps/trading_runtime core
```

Then:

```gdb
bt
```

---

# Shared Libraries

## Show linked libraries

```bash
ldd ./build/apps/trading_runtime
```

Useful for confirming dependencies such as OpenSSL.

---

## Check OpenSSL linkage

```bash
ldd ./build/apps/trading_runtime | grep ssl
```

---

# Symbols

## Show symbols in an executable

```bash
nm -C ./build/apps/trading_runtime
```

`-C` demangles C++ symbol names.

---

## Search one symbol

```bash
nm -C ./build/apps/trading_runtime | grep TlsOuchTransport
```

---

# DPDK

## Show network interfaces and drivers

```bash
sudo dpdk-devbind.py --status
```

Depending on installation, the command may also be:

```bash
sudo dpdk-devbind --status
```

Shows which NICs are bound to:

- kernel drivers
- `vfio-pci`
- other DPDK-compatible drivers

---

## Bind a NIC to vfio-pci

Example:

```bash
sudo dpdk-devbind.py \
    --bind=vfio-pci \
    0000:01:00.0
```

The PCI address must match the target NIC.

---

## Bind NIC back to kernel driver

Example:

```bash
sudo dpdk-devbind.py \
    --bind=igb \
    0000:01:00.0
```

Use the correct kernel driver for the NIC.

---

# Huge Pages

## Show huge-page configuration

```bash
grep Huge /proc/meminfo
```

Useful fields include:

```text
HugePages_Total
HugePages_Free
Hugepagesize
```

---

## Allocate 2 MB huge pages

Example:

```bash
sudo sysctl -w vm.nr_hugepages=1024
```

---

## Confirm huge pages

```bash
grep HugePages /proc/meminfo
```

---

# PCI Devices

## Show Ethernet controllers

```bash
lspci | grep -i ethernet
```

---

## Show detailed NIC information

```bash
lspci -nnk
```

Useful for checking:

- PCI address
- device ID
- current driver
- available kernel modules

---

# Network Interfaces

## Show interfaces

```bash
ip link
```

---

## Show IP addresses

```bash
ip addr
```

---

## Show detailed NIC information

```bash
sudo ethtool enp1s0
```

---

## Show NIC driver

```bash
sudo ethtool -i enp1s0
```

---

# Process Inspection

## Show running project processes

```bash
ps aux | grep trading_runtime
```

---

## Find process ID

```bash
pgrep trading_runtime
```

---

## Kill one process

```bash
kill <PID>
```

Use force only if necessary:

```bash
kill -9 <PID>
```

---

# Useful Shell Commands

## Show current directory

```bash
pwd
```

---

## List files

```bash
ls
```

---

## Detailed listing

```bash
ls -lah
```

---

## Create directory

```bash
mkdir -p docs/performance
```

---

## Copy file

```bash
cp source destination
```

---

## Rename or move file

```bash
mv old_name new_name
```

---

## Remove file

```bash
rm file
```

Use carefully.

---

# Recommended Workflow

A typical feature-development workflow for this project is:

```bash
git switch main
git switch -c feature/my-feature
```

Make the code changes.

Then:

```bash
cmake --build build -j4
ctest --test-dir build --output-on-failure
```

Review:

```bash
git diff
git status --short
```

Stage only the files belonging to the feature:

```bash
git add path/to/file1 path/to/file2
```

Verify staged changes:

```bash
git diff --cached
```

Commit:

```bash
git commit -m "feat: describe the change"
```

Push:

```bash
git push -u origin feature/my-feature
```

After verification:

```bash
git switch main
git merge --ff-only feature/my-feature
git push origin main
```

---

# Performance Benchmark Workflow

For latency measurements:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
```

Build the target:

```bash
cmake --build build \
    --target ouch_tls_round_trip_benchmark \
    -j4
```

Run:

```bash
./build/benchmarks/ouch_tls_round_trip_benchmark \
    certs/server.crt \
    certs/server.key
```

Record at least:

```text
min
mean
p50
p95
p99
p99.9
max
throughput
```

Run benchmarks multiple times before judging an optimization.

For latency-sensitive work, compare tail latency as well as the mean.

---

# Notes

- Prefer Release builds for benchmark measurements.
- Avoid debug logging inside benchmark hot paths.
- Do not interpret loopback latency as physical network latency.
- Keep TLS handshake timing separate from steady-state order latency when
  comparing execution-path performance.
- Prefer explicit file staging over `git add .` when the working tree contains
  unrelated changes.
- Record baseline benchmark results before performing optimizations.


