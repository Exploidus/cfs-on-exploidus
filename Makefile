CC ?= gcc
CFLAGS ?= -Wall -Wextra -O2 -Iinclude -D_GNU_SOURCE
LDFLAGS ?= -lpthread

SRC = src/os-impl-common.c \
      src/os-impl-mutex.c \
      src/os-impl-queues.c \
      src/os-impl-tasks.c \
      src/os-impl-timebase.c \
      src/os-impl-capability.c \
      src/os-impl-effector.c

DEMO_SRC = demo/cfs_flight_demo.c $(SRC)
BIN = cfs_exploidus_demo

all: $(BIN)

$(BIN): $(DEMO_SRC)
	$(CC) $(CFLAGS) -o $@ $(DEMO_SRC) $(LDFLAGS)

test: $(BIN)
	./$(BIN)

clean:
	rm -f $(BIN) *.o

.PHONY: all test clean
