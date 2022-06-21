# Makefile for gTag
#

# defaults
PROGRAM = gTag
CC = g++
PROF_OPTS = -pg
CC_OPTS = -O2 -Wall `pkg-config --cflags gtk4`
LK_OPTS = -lpng -lz -lboost_filesystem -lboost_system `pkg-config --libs gtk+-3.0 pangoft2`
RM = rm -f

SOURCE_CPP = file.cpp gui.cpp
OBJ = $(SOURCE_CPP:.cpp=.o)
all: $(OBJ)
	$(CC) -o $(PROGRAM) $(OBJ) $(LK_OPTS)
%.o: %.c
	$(CC) $(CC_OPTS) -c -o $@ $<
%.o: %.cpp
	$(CC) $(CC_OPTS) -c -o $@ $<
prof:
	$(CC) $(CC_OPTS) $(LK_OPTS) $(PROF_OPTS) -o $(PROGRAM) $(SOURCE_CPP)
clean:
	$(RM) $(OBJ) $(PROGRAM)
rebuild:
	make clean
	make
