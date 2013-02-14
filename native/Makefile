CC=g++ -g
#CC=g++ -Ofast

ODIR=obj
THIRD=third_party
FFT=$(THIRD)/kiss_fft130
RS=$(THIRD)/rs
INC=-I$(FFT) -I$(FFT)/tools -I$(RS)
BIN=bin


#_OBJ = hellomake.o hellofunc.o 
#OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

OBJS=assigners.o audio.o codecs.o log.o packeter.o receiver.o scom.o \
     sender.o spectrum.o sync.o test.o util.o
THIRDS=$(ODIR)/kiss_fftr.o $(ODIR)/kiss_fft.o \
     $(ODIR)/init_rs_int.o $(ODIR)/encode_rs_int.o $(ODIR)/decode_rs_int.o

OBJS2=$(patsubst %,$(ODIR)/%,$(OBJS))

LIBS=-lm

stuff: bin/stuff

bin/stuff: $(OBJS2) $(THIRDS)
	$(CC) -o $(BIN)/stuff $(OBJS2) $(THIRDS) $(LIBS) $(INC)

# TODO: header files...
$(ODIR)/%.o: %.cpp Makefile
	$(CC) -c $< -o $@ $(INC) 


$(ODIR)/%_rs_int.o: $(RS)/%_rs_int.c Makefile
	$(CC) -c $< -o $@ $(INC)

$(ODIR)/kiss_fft.o: $(FFT)/kiss_fft.c $(FFT)/kiss_fft.h Makefile
	$(CC) -c $< -o $@ $(INC)

$(ODIR)/kiss_fftr.o: $(FFT)/tools/kiss_fftr.c $(FFT)/tools/kiss_fftr.h
	$(CC) -c -o $(ODIR)/kiss_fftr.o $(FFT)/tools/kiss_fftr.c $(INC)

dirs:
	mkdir -p $(ODIR)
	mkdir -p $(BIN)

clean:
	rm -rf $(ODIR)/*
	rm -rf $(BIN)/*
	
