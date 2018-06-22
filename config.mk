VERSION = 0.1

# release flags
#CFLAGS = -fstack-protector-strong -D_FORTIFY_SOURCE=2 -Wall -fPIC -Werror -Wpedantic -O2 -pthread -flto

# debug flags
CFLAGS = -Wall -Werror -Wpedantic -O0 -g -fPIC -std=c99 -pthread -flto
LDFLAGS = -pthread -flto

# if you're feeling lucky:
#CFLAGS += -DNDEBUG

## Comment this in for profiling
#LDFLAGS += -lprofiler

## Comment this in for coverage reports
#CFLAGS += -fprofile-arcs -ftest-coverage

BENCH_JSON=bench/bench-file.json
BENCH_PLIST=bench/bench-file.plist
