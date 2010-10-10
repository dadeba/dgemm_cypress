CC              = gcc

include ../makefile.dir

INCLUDE         = -I$(ATISDK)/include
LIBS            =

CFLAGS          = -O4 -Wall $(INCLUDE) -std=c99

INCS            = accllib.h
OBJS            = accllib.o

LIBNAME         = libaccl.a

$(LIBNAME): $(OBJS)
	ld -r -o temp.o $(OBJS) $(LIBS)
	ar ruv $(LIBNAME) temp.o
	ranlib $(LIBNAME)

clean:; 
	@rm -f $(PROGRAM) $(OBJS) core *~ *.trace *.a *.o

accllib.o : accllib.c $(INCS)
