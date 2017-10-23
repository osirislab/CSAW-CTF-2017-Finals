TARGET=connectXor

IDIR=include
CC=gcc
CFLAGS= -I$(IDIR) -Wall -pthread -no-pie

ODIR=obj
LDIR=/lib
CDIR=src

LIBS=

_DEPS = $(shell ls $(IDIR))
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = $(patsubst %.c, %.o, $(shell ls $(CDIR)))
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))


$(ODIR)/%.o: $(CDIR)/%.c $(DEPS)
		$(CC) $(CFLAGS) -g -c -o $@ $<

all: release

.PHONY: release 
release: $(OBJ)
		$(CC) $(CFLAGS) -o $(TARGET) $^ $(LIBS) -O3
		strip $(TARGET)

.PHONY: debug
debug: $(OBJ)
		$(CC) $(CFLAGS) -g -o $(TARGET) $^ $(LIBS) -O0

.PHONY: clean
clean:
		rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~ $(TARGET) 
