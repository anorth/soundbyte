CC=g++ -g
ODIR=obj
LIB=third_party
FFT=$(LIB)/kiss_fft130
INC=-I$(FFT) -I$(FFT)/tools
BIN=bin


#_OBJ = hellomake.o hellofunc.o 
#OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

OBJS=assigners.o audio.o codecs.o packeter.o receiver.o scom.o \
     sender.o spectrum.o sync.o test.o util.o

OBJS2=$(patsubst %,$(ODIR)/%,$(OBJS))

LIBS=-lm

stuff: bin/stuff

bin/stuff: $(OBJS2) $(ODIR)/kiss_fftr.o $(ODIR)/kiss_fft.o
	$(CC) -o $(BIN)/stuff $(OBJS2) $(ODIR)/kiss_fft.o $(ODIR)/kiss_fftr.o $(LIBS) $(INC)

$(ODIR)/%.o: %.cpp
	$(CC) -c $< -o $@ $(INC) 

$(ODIR)/kiss_fft.o: $(FFT)/kiss_fft.c $(FFT)/kiss_fft.h
	$(CC) -c -o $(ODIR)/kiss_fft.o $(FFT)/kiss_fft.c $(INC)

$(ODIR)/kiss_fftr.o: $(FFT)/tools/kiss_fftr.c $(FFT)/tools/kiss_fftr.h
	$(CC) -c -o $(ODIR)/kiss_fftr.o $(FFT)/tools/kiss_fftr.c $(INC)

dirs:
	mkdir -p $(ODIR)
	mkdir -p $(BIN)

clean:
	rm -rf $(ODIR)/*
	rm -rf $(BIN)/*
	
