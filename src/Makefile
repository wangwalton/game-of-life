#
# Include platform-dependent settings, and definitions.
#
include Makefile.include

LDFLAGS += -lm -pthread -g
CPPFLAGS += "-DCOMPILER=\"$(CC)\"" "-DFLAGS=\"$(COPTFLAGS)\""

#
# Include the realtime clock library, if required.
#
ifeq ($(TIMER_TYPE),USE_CLOCK_GETTIME_REALTIME)
	LDFLAGS += -lrt
else
	ifeq ($(TIMER_TYPE),USE_CLOCK_GETTIME_MONOTONIC)
		LDFLAGS += -lrt
	endif
endif

SDIR=src
ODIR=build
TDIR=tst

GOL_EXE = gol
GOL_OBJS = $(ODIR)/gol.o $(ODIR)/life.o $(ODIR)/lifeseq.o $(ODIR)/load.o $(ODIR)/save.o 

GOL_VERIFY_EXE = gol_verify
GOL_VERIFY_OBJS = $(ODIR)/gol.verify.o $(ODIR)/life.o $(ODIR)/lifeseq.o $(ODIR)/load.o $(ODIR)/save.o 

TEST_EXE = life_test
TEST_OBJS = $(ODIR)/life_test.o $(ODIR)/life.o $(ODIR)/lifeseq.o $(ODIR)/load.o $(ODIR)/save.o

BITBOARD_EXE = initboard
BITBOARD_OBJS = $(ODIR)/bitboard.o $(ODIR)/random_bit.o

EXES = $(GOL_EXE) $(BITBOARD_EXE) $(TEST_EXE) 
OBJS = $(GOL_OBJS) $(BITBOARD_OBJS) $(TEST_EXE)

all: $(GOL_EXE) $(BITBOARD_EXE) $(GOL_VERIFY_EXE)

$(GOL_EXE): $(GOL_OBJS) 
	$(LINKER) $(CFLAGS) $(LDFLAGS) $^ -o $@ 

$(GOL_VERIFY_EXE): $(GOL_VERIFY_OBJS) 
	$(LINKER) $(CFLAGS) $(LDFLAGS) $^ -o $@ 

$(BITBOARD_EXE): $(BITBOARD_OBJS)
	$(LINKER) $(CFLAGS) $(LDFLAGS) $^ -o $@ 

$(TEST_EXE): $(TEST_OBJS) 
	$(LINKER) $(CFLAGS) $(LDFLAGS) $^ -o $@ 

$(ODIR)/%.o: $(SDIR)/%.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

$(ODIR)/%.o: $(TDIR)/%.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@


$(ODIR)/%.verify.o: $(SDIR)/%.c
	$(CC) -c $(CFLAGS) -DVERIFY_FLAG $(CPPFLAGS) $< -o $@

$(ODIR)/lifeseq.o: $(SDIR)/lifeseq.c $(SDIR)/life.h $(SDIR)/util.h

$(ODIR)/life.o: $(SDIR)/life.c $(SDIR)/life.h $(SDIR)/util.h $(SDIR)/work_queue.h $(SDIR)/board.h

$(ODIR)/load.o: $(SDIR)/load.c $(SDIR)/load.h

$(ODIR)/save.o: $(SDIR)/save.c $(SDIR)/save.h

$(ODIR)/life_test.o: $(TDIR)/life_test.c $(SDIR)/life.h $(SDIR)/load.h $(SDIR)/save.h

$(ODIR)/gol.o: $(SDIR)/gol.c $(SDIR)/life.h $(SDIR)/load.h $(SDIR)/save.h 

$(ODIR)/bitboard.o: $(SDIR)/bitboard.c $(SDIR)/random_bit.h

$(ODIR)/random_bit.o: $(SDIR)/random_bit.c $(SDIR)/random_bit.h

clean:
	rm -f $(GOL_OBJS) $(GOL_VERIFY_OBJS) $(GOL_EXE) $(GOL_VERIFY_EXE) \
		$(BITBOARD_OBJS) $(BITBOARD_EXE) $(TEST_OBJS) $(TEST_EXE)