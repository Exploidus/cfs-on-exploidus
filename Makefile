CC ?= gcc
CFLAGS ?= -Wall -Wextra -O2 -Iinclude -Ipsp/inc -Icfe/inc -D_GNU_SOURCE
LDFLAGS ?= -lpthread -lm

OSAL_SRC = src/os-impl-common.c \
           src/os-impl-mutex.c \
           src/os-impl-queues.c \
           src/os-impl-tasks.c \
           src/os-impl-timebase.c \
           src/os-impl-capability.c \
           src/os-impl-effector.c

PSP_SRC  = psp/src/cfe_psp_start.c \
           psp/src/cfe_psp_support.c \
           psp/src/cfe_psp_memory.c \
           psp/src/cfe_psp_watchdog.c

CFE_SRC  = cfe/src/cfe_es.c \
           cfe/src/cfe_sb.c \
           cfe/src/cfe_evs.c

# Milestone 2 additions
CFE_M2_SRC = cfe/src/cfe_time.c \
             cfe/src/cfe_tbl.c \
             cfe/src/cfe_sch.c

DEMO_SRC   = demo/cfs_flight_demo.c $(OSAL_SRC) $(PSP_SRC) $(CFE_SRC) $(CFE_M2_SRC)
DEMO_M2_SRC = demo/cfs_m2_demo.c $(OSAL_SRC) $(PSP_SRC) $(CFE_SRC) $(CFE_M2_SRC)

BIN    = cfs_exploidus_demo
BIN_M2 = cfs_exploidus_m2

all: $(BIN) $(BIN_M2)

$(BIN): $(DEMO_SRC)
	$(CC) $(CFLAGS) -o $@ $(DEMO_SRC) $(LDFLAGS)

$(BIN_M2): $(DEMO_M2_SRC)
	$(CC) $(CFLAGS) -o $@ $(DEMO_M2_SRC) $(LDFLAGS)

test: $(BIN)
	./$(BIN)

test_m2: $(BIN_M2)
	./$(BIN_M2)

clean:
	rm -f $(BIN) $(BIN_M2) *.o

.PHONY: all test test_m2 clean
