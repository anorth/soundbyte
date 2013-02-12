CC=g++ -g
ODIR=obj
LIB=third_party
FFT=$(LIB)/kiss_fft130
INC=-I$(FFT) -I$(FFT)/tools
BIN=bin


#_OBJ = hellomake.o hellofunc.o 
#OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

SRCS=assigners.cpp audio.cpp decoder.cpp codecs.cpp packeter.cpp scom.cpp spectrum.cpp sync.cpp test.cpp util.cpp

LIBS=-lm

stuff: $(SRCS) fft fftr dirs
	$(CC) -o $(BIN)/stuff $(SRCS) $(ODIR)/kiss_fft.o $(ODIR)/kiss_fftr.o ${LIBS} $(INC)

fft: $(FFT)/kiss_fft.c $(FFT)/kiss_fft.h dirs
	$(CC) -c -o $(ODIR)/kiss_fft.o $(FFT)/kiss_fft.c -I $(INC)

fftr: $(FFT)/tools/kiss_fftr.c $(FFT)/tools/kiss_fftr.h dirs
	$(CC) -c -o $(ODIR)/kiss_fftr.o $(FFT)/tools/kiss_fftr.c $(INC)

dirs:
	mkdir -p $(ODIR)
	mkdir -p $(BIN)

clean:
	rm -rf $(ODIR)
	rm -rf $(BIN)
	
