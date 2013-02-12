
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

static const int CHUNK_SAMPLES = SAMPLE_RATE / 10;
static const int CHUNK_BYTES = CHUNK_SAMPLES * 2;

static struct option longopts[] = {
  { "listen",             no_argument,          NULL,          'l' },
  { "send",               no_argument,          NULL,          's' },
  { NULL,                 0,                    NULL,           0 }
};

kiss_fftr_cfg fftConfig;
kiss_fft_cpx fftResult[CHUNK_SAMPLES];

float abs(kiss_fft_cpx c) {
  return sqrt(c.r * c.r + c.i * c.i);
}

void doFft(char chunkBytes[]) {
  vector<float> samples(CHUNK_SAMPLES);
  decodePcm16(chunkBytes, CHUNK_BYTES, samples);
  kiss_fftr(fftConfig, samples.data(), fftResult);
  cerr << abs(fftResult[44]) << '\n';
}

void doListen() {
  char chunkBytes[CHUNK_BYTES];
  char messageBuffer[100];
  while (cin.good()) {
    cin.read(chunkBytes, CHUNK_BYTES);
    if (cin.good()) {
      //doFft(chunkBytes);

      decodeAudio(chunkBytes, sizeof(chunkBytes));
      while (messageAvailable()) {
        takeMessage(messageBuffer, sizeof(messageBuffer));
        cout << messageBuffer << endl;
      }
    }
  }
}

void doSend() {
  char message[TEST_MESSAGE_SIZE] = "123456789";
  char waveform[SAMPLE_RATE * 10];
  while (true) {
    int waveformBytes = encodeMessage(message, sizeof(message), waveform, sizeof(waveform));
    cerr << "Generated " << waveformBytes << " bytes" << endl;
    assert(waveformBytes > 0);
    cout.write(waveform, waveformBytes);
  }
}

int main(int argc, char **argv) {
  fftConfig = kiss_fftr_alloc(CHUNK_SAMPLES, false, 0, 0);

  bool optListen, optSend;
  int ch;
  while ((ch = getopt_long(argc, argv, "ls", longopts, NULL)) != -1) {
    switch (ch) {
      case 'l':
        optListen = 1;
        break;
      case 's':
        optSend = 1;
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

  kiss_fft_cleanup();   
  free(fft);
  free(ifft);
  return 0;
}

