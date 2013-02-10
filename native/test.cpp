#include <iostream>
#include "kiss_fftr.h"
#include "math.h"
#include <vector>
using namespace std;

static const int SAMPLE_RATE = 44100;
static const float PCM_QUANT = 32767.5;

float* decodePcm16(char *buffer, int buflen, float* out) {
  for (int i = 0; i < buflen; i += 2) {
    out[i/2] = ((float)(*(short*)(buffer + i)) + 0.5f) / PCM_QUANT;
  }
  return out;

}

float abs(kiss_fft_cpx c) {
  return sqrt(c.r * c.r + c.i * c.i);
}

int main(void) {
  int size = SAMPLE_RATE / 10;
  int size_bytes = size * 2;

  char sample_bytes[size_bytes];
  float sample_floats[size];
  kiss_fft_cpx out_cpx[size]; //,out[size],*cpx_buf;

  cout << size;
  kiss_fftr_cfg fft = kiss_fftr_alloc(size, false, 0, 0);

  while (true) {
    cin.read(sample_bytes, size_bytes);
    decodePcm16(sample_bytes, size_bytes, sample_floats);
    kiss_fftr(fft, sample_floats, out_cpx);
    cout << abs(out_cpx[44]) << '\n';
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

