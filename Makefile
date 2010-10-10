CC	= gcc

include makefile.dir
INC	= -I$(ATISDK)/include -IACCL_CAL
LIBS	= -LACCL_CAL -laccl -laticalcl -laticalrt -lblas

OPTIMIZE= -Wall -O4 -std=c99 -msse3
CFLAGS	= $(OPTIMIZE) $(INC)

OBJS	= main.o
PROGRAM	= run

$(PROGRAM) : $(OBJS) kernel_TN.h kernel_NN.h libaccl.a
	$(CC) $(OBJS) $(LIBS) -o $@

clean:
	rm -rf *.o *~ $(PROGRAM) *.img *dis.txt kernel*.h
	rm -rf pack pack.tar.gz
	cd ACCL_CAL; make clean

.c.o:
	$(CC) $(CFLAGS) -c $<

main.o : main.c kernel_TN.h kernel_NN.h kernel_NT.h kernel_TT.h


kernel_TN.h : m44_gemm_TN.il
	./utils/template-converter d_TN $< >| $@

kernel_NN.h : m44_gemm_NN.il
	./utils/template-converter d_NN $< >| $@

kernel_NT.h : m44_gemm_NT.il
	./utils/template-converter d_NT $< >| $@

kernel_TT.h : m44_gemm_TT.il
	./utils/template-converter d_TT $< >| $@

pack:
	./pack.sh

libaccl.a:
	cd ACCL_CAL; make
