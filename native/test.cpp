
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>
#include <csignal>
#include "getopt.h"
#include "kiss_fftr.h"

#include "debug.h"
#include "audio.h"
#include "constants.h"
#include "scom.h"
#include "util.h"
#include "codecs.h"

#include "stream.h"

using namespace std;

static const int CHUNK_SAMPLES = SAMPLE_RATE / 50;
static const int CHUNK_BYTES = CHUNK_SAMPLES * 2;
static char* MESSAGE = "http://www.helixta.com.au/";
char blank_chunk[CHUNK_BYTES];

static struct option longopts[] = {
  { "listen",             no_argument,          NULL,          'l' },
  { "send",               no_argument,          NULL,          's' },
  { "tests",              no_argument,          NULL,          't' },
  { NULL,                 0,                    NULL,           0 }
};

// TODO: use macro instead. or an actual unit test framework.
template<typename T>
void assertEquals(T expected, T actual) {
  if (expected != actual) {
    cout << "Expected: " << expected << " but got: " << actual << endl;
    assert(false);
  }
}

void testBuffer() {
  Buffer<int> test;
  test.push_back(5);
  test.push_back(6);
  assert(test.raw()[0] == 5);
  test.consume(1);
  assert(test.raw()[0] == 6);
  test.push_back(7);
  test.push_back(8);
  test.push_back(1);
  test.push_back(2);
  test.consume(4);
  assert(test.raw()[0] == 2);
}

void testBitPacking() {
  unsigned int val = 06712534;
  unsigned char *ptr = (unsigned char *) &val;
  assertEquals(04u, unpackBits(ptr, 0, 3));
  assertEquals(03u, unpackBits(ptr, 3, 3));
  assertEquals(01u, unpackBits(ptr, 6, 2));
  assertEquals(05u, unpackBits(ptr, 6, 3));
  assertEquals(02u, unpackBits(ptr, 9, 3));
  assertEquals(01u, unpackBits(ptr, 12, 3));
  assertEquals(07u, unpackBits(ptr, 15, 3));
  assertEquals(06u, unpackBits(ptr, 18, 3));
  assertEquals(06712534u, unpackBits(ptr, 0, 21));
  assertEquals(0125u, unpackBits(ptr, 6, 9));
  assertEquals(0125u>>1, unpackBits(ptr, 7, 8));
  assertEquals(031253u, unpackBits(ptr, 3, 14));

  val = 0;
  packBits(ptr, 0, 3, 02u);
  assertEquals(02u, val);
  packBits(ptr, 3, 3, 06u);
  assertEquals(062u, val);
  packBits(ptr, 6, 3, 07u);
  assertEquals(0762u, val);
  packBits(ptr, 3, 3, 05u);
  assertEquals(0752u, val);
  packBits(ptr, 0, 1, 01u);
  assertEquals(0753u, val);
  packBits(ptr, 1, 6, 00u);
  assertEquals(0601u, val);
  packBits(ptr, 6, 12, 05403u);
  assertEquals(0540301u, val);
  packBits(ptr, 6, 11, 0u);
  assertEquals(0400001u, val);
  packBits(ptr, 18, 1, 1u);
  assertEquals(01400001u, val);
}

vector<char> fromString(char *str) {
  vector<char> ret;
  while (*str) {
    ret.push_back(*str);
    str++;
  }
  return ret;
}

vector<bool> strToBits(string& str) {
  vector<char> data(str.begin(), str.end());
  vector<bool> bits;
  toBitSequence(data, bits);
  return bits;
}

string bitsToStr(vector<bool> bits) {
  vector<char> chars;
  toByteSequence(bits, chars);
  string str(chars.begin(), chars.end());
  return str;
}

vector<float> toFloatBits(vector<bool> bits) {
  vector<float> floats;
  cout << "tfb 1" << endl;
  for (vector<bool>::iterator it = bits.begin(); it != bits.end(); ++it) {
    floats.push_back( (*it) ? 1.0f : -1.0f );
  }
  cout << "tfb 9 " << endl;
  return floats;
}

void testReedSolomonMsg(RsCodec &codec, string msg, int blockBits, int expectedBlocks) {
  vector<bool> encoded, decoded;

  encoded.clear();
  codec.encode(strToBits(msg), encoded);
  assertEquals(blockBits * expectedBlocks, (int) encoded.size());

  decoded.clear();
  vector<float> floatBits = toFloatBits(encoded);
  for (int i = 0; i < expectedBlocks; ++i) {
    vector<float> section(&floatBits[i * blockBits], &floatBits[i * blockBits] + blockBits);
    codec.decode(section, decoded); 
  }
  assertEquals(msg, bitsToStr(decoded).substr(0, msg.length()));
}

void testReedSolomon() {
  {
    RsCodec codec(
        20, // encoded size (symbols)
        10, // message size (symbols)
        8   // symbols size (bits)
        );

    int blockSize = 20 * 8;
    testReedSolomonMsg(codec, "hello", blockSize, 1);
    testReedSolomonMsg(codec, "hello67890", blockSize, 1);
    testReedSolomonMsg(codec, "hello678901", blockSize, 2);
  }

  {
    RsCodec codec(
        20, // encoded size (symbols)
        10, // message size (symbols)
        6   // symbols size (bits)
        );

    int blockSize = 20 * 6;
    testReedSolomonMsg(codec, "hello", blockSize, 1);
    testReedSolomonMsg(codec, "hello67890", blockSize, 2);
    testReedSolomonMsg(codec, "hello678901", blockSize, 2);
    testReedSolomonMsg(codec, "hello6789012", blockSize, 2);
    testReedSolomonMsg(codec, "hello67890123", blockSize, 2);
    testReedSolomonMsg(codec, "hello678901234", blockSize, 2);
    testReedSolomonMsg(codec, "hello6789012345", blockSize, 2);
    testReedSolomonMsg(codec, "hello67890123456", blockSize, 3);

    cout << " OK -------" << endl;
  }

  {
    RsCodec codec(
        15, // encoded size (symbols)
        8,  // message size (symbols)
        4   // symbols size (bits)
        );

    int blockSize = 15 * 4;
    testReedSolomonMsg(codec, "hi", blockSize, 1);
    testReedSolomonMsg(codec, "hello", blockSize, 2);
    testReedSolomonMsg(codec, "hello678", blockSize, 2);
    testReedSolomonMsg(codec, "hello6789", blockSize, 3);
  }

}

int doTests() {
  testBuffer();
  testBitPacking();
  testReedSolomon();

  cout << "TESTS PASSED" << endl;
  return 0;
}

void doListen(int expectedMessages) {
  char chunkBytes[CHUNK_BYTES];
  char messageBuffer[256];
  int numReceived = 0;
  while (cin.good()) {
    cin.read(chunkBytes, CHUNK_BYTES);
    if (cin.good()) {
      decodeAudio(chunkBytes, sizeof(chunkBytes));
      while (messageAvailable()) {
        int bytes = takeMessage(messageBuffer, sizeof(messageBuffer));

        cerr << "MESSAGE: ";
        cerr.write(messageBuffer, bytes);
        cerr << endl;

        cout.write(messageBuffer, bytes);
        cout << endl;
        if (strncmp(messageBuffer, MESSAGE, strlen(MESSAGE))) {
          cout << "Received " << numReceived << " messages ok, then" << endl;
          cout << "GOT BAD MESSAGE: ";
          cout.write(messageBuffer, bytes);
          cout << endl;
          assert(false);
        } 
        numReceived++;
      }
    }
  }
  if (numReceived != expectedMessages) {
    cout << "Received " << numReceived << " messages but expected " << expectedMessages << endl;
    assert(false);
  }
  cout << "OK" << endl;
}

void doSend(int iters) {
  int bufSize = SAMPLE_RATE * 10;
  char *waveform = new char[bufSize];
  while (iters-- > 0) {
    int waveformBytes = encodeMessage(MESSAGE, strlen(MESSAGE), waveform, bufSize);
    assert(waveformBytes <= bufSize);
    //cerr << "Message '" << MESSAGE << "', waveform " << waveformBytes << " bytes" << endl;
    assert(waveformBytes > 0);
    cout.write(waveform, waveformBytes);

    // Spit out a blank chunk to ensure any half-filled buffer on the
    // receiver gets filled before the sender stops sending.
    cout.write(blank_chunk, CHUNK_BYTES);
  }
  delete waveform;
}

int main(int argc, char **argv) {
  signal(SIGSEGV, signal_handler);
  signal(SIGKILL, signal_handler);
  signal(6, signal_handler); // TODO: figure out what the constant is for assert

  bool optListen = false, optSend = false;
  for (int i = 0; i < CHUNK_BYTES; i++) {
    blank_chunk[i] = 0;
  }

  int ch;
  while ((ch = getopt_long(argc, argv, "lst", longopts, NULL)) != -1) {
    switch (ch) {
      case 'l':
        optListen = true;
        break;
      case 's':
        optSend = true;
        break;
      case 't':
        return doTests();
      default:
        assert(false);
        //usage();
    }
  }
  argc -= optind;
  argv += optind;
  
  scomInit();
  int iters = 30;

  if (optListen) {
    doListen(iters);
  } else if (optSend) {
    doSend(iters);
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

