SIMGRID_PATH = /usr/local/
CC = gcc
LIBS := -lsimgrid -lm

SOURCES = \
src/bicpa.c \
src/dag.c \
src/main.c \
src/task.c \
src/timer.c \
src/workstation.c

OBJS = \
src/bicpa.o \
src/dag.o \
src/main.o \
src/task.o \
src/timer.o \
src/workstation.o

all: biCPA

biCPA: $(OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C Linker'
	gcc -L$(SIMGRID_PATH)/lib -o biCPA $(OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

%.o: %.c
	$(CC)  -I$(SIMGRID_PATH)/include -I"./include" -O3 -Wall -c -o $@ $<


# Other Targets
clean:
	rm -rf $(OBJS) biCPA

