
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>
#include "getopt.h"
#include "kiss_fftr.h"

#include "audio.h"
#include "constants.h"
#include "scom.h"

using namespace std;

static const int CHUNK_SAMPLES = SAMPLE_RATE / 100;
static const int CHUNK_BYTES = CHUNK_SAMPLES * 2;
char blank_chunk[CHUNK_BYTES];

static struct option longopts[] = {
  { "listen",             no_argument,          NULL,          'l' },
  { "send",               no_argument,          NULL,          's' },
  { NULL,                 0,                    NULL,           0 }
};

void doListen() {
  char chunkBytes[CHUNK_BYTES];
  char messageBuffer[256];
  while (cin.good()) {
    cin.read(chunkBytes, CHUNK_BYTES);
    if (cin.good()) {
      decodeAudio(chunkBytes, sizeof(chunkBytes));
      while (messageAvailable()) {
        int bytes = takeMessage(messageBuffer, sizeof(messageBuffer));
        cout.write(messageBuffer, bytes);
        cout << endl;

        cerr << "MESSAGE: ";
        cerr.write(messageBuffer, bytes);
        cerr << endl;
      }
    }
  }
}

void doSend() {
  char message[] = "http://www.helixta.com.au/";
  char waveform[SAMPLE_RATE * 10];
  //while (true) {
  for (int rpt = 0; rpt < 10; rpt++) {
    int waveformBytes = encodeMessage(message, strlen(message), waveform, sizeof(waveform));
    //cerr << "Message '" << message << "', waveform " << waveformBytes << " bytes" << endl;
    assert(waveformBytes > 0);
    cout.write(waveform, waveformBytes);

    // Spit out a blank chunk to ensure any half-filled buffer on the
    // receiver gets filled before the sender stops sending.
    cout.write(blank_chunk, CHUNK_BYTES);
    cout.write(blank_chunk, CHUNK_BYTES);
    cout.write(blank_chunk, CHUNK_BYTES);
    cout.write(blank_chunk, CHUNK_BYTES);
    cout.write(blank_chunk, CHUNK_BYTES);
    cout.write(blank_chunk, CHUNK_BYTES);
    cout.write(blank_chunk, CHUNK_BYTES);
    cout.write(blank_chunk, CHUNK_BYTES);
    cout.write(blank_chunk, CHUNK_BYTES);
    cout.write(blank_chunk, CHUNK_BYTES);
    cout.write(blank_chunk, CHUNK_BYTES);
    cout.write(blank_chunk, CHUNK_BYTES);
    cout.write(blank_chunk, CHUNK_BYTES);
  }
}

int main(int argc, char **argv) {
  bool optListen = false, optSend = false;
  for (int i = 0; i < CHUNK_BYTES; i++) {
    blank_chunk[i] = 0;
  }

  int ch;
  while ((ch = getopt_long(argc, argv, "ls", longopts, NULL)) != -1) {
    switch (ch) {
      case 'l':
        optListen = true;
        break;
      case 's':
        optSend = true;
        break;
      default:
        assert(false);
        //usage();
    }
  }
  argc -= optind;
  argv += optind;
  
  scomInit();

  if (optListen) {
    doListen();
  } else if (optSend) {
    doSend();
  }

  return 0;
}


/*** EXAMPLE JUNK FROM INTERNET ***/

kiss_fft_cpx* copycpx(float *mat, int nframe) {
  int i;
  kiss_fft_cpx *mat2;
  mat2=(kiss_fft_cpx*)KISS_FFT_MALLOC(sizeof(kiss_fft_cpx)*nframe);
  kiss_fft_scalar zero;
  memset(&zero,0,sizeof(zero) );
  for(i=0; i<nframe ; i++)
  {
    mat2[i].r = mat[i];
    mat2[i].i = zero;
  }
  return mat2;
}

int main2(void) {
  int i,size = 12;
  int isinverse = 1;
  float buf[size];
  float array[] = {0.1, 0.6, 0.1, 0.4, 0.5, 0, 0.8, 0.7, 0.8, 0.6, 0.1,0};  

  kiss_fft_cpx out_cpx[size],out[size],*cpx_buf;

  kiss_fftr_cfg fft = kiss_fftr_alloc(size*2 ,0 ,0,0);
  kiss_fftr_cfg ifft = kiss_fftr_alloc(size*2,isinverse,0,0);

  cpx_buf = copycpx(array,size);
  kiss_fftr(fft,(kiss_fft_scalar*)cpx_buf, out_cpx);
  kiss_fftri(ifft,out_cpx,(kiss_fft_scalar*)out );

  printf("Input:    \tOutput:\n");
  for(i=0;i<size;i++)
  {
    buf[i] = (out[i].r)/(size*2);
    printf("%f\t%f  %f,%f\n",array[i],buf[i],out_cpx[i].r, out_cpx[i].i);
  }

  free(fft);
  free(ifft);
  kiss_fft_cleanup();   
  return 0;
}

