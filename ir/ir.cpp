#include <cassert>
#include <cmath>
#include <vector>
#include <iostream>
#include <fstream>
#include <stdio.h>

using namespace std;

static const float PCM_QUANT = 32767.5;

double* GenerateSignal(bool *mls, long P) {
  long i;
  double *input = new double[P];
  for (i = 0; i < P; i++) { // Change 0 to 1 and 1 to -1
    input[i] = -2 * mls[i] + 1;
  }
  return input;

}

void encodePcm(double *signal, char *buf, long P) {
  for (int i = 0; i < P; i++) {
    int index = i<<1;

    buf[index+1] = signal[i] > 0 ? 0x80 : 0x7f;
    buf[index] = signal[i] > 0 ? 0 : 0xff;
  }
}



void GenerateMls(bool *mls, long P, long N)
{
  const long maxNoTaps = 18;
  const bool tapsTab[16][18] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0,
    0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0,
    0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 1, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    bool taps[maxNoTaps];
    long i, j;
    bool *delayLine = new bool[maxNoTaps];
    long sum;
    for (i = 0; i < N; i++) // copy the N'th taps table
    {
      taps[i] = tapsTab[maxNoTaps - N][i];
      delayLine[i] = 1;
    }
    for (i = 0; i < P; i++) // Generate an MLS by summing the taps mod 2
    {
      sum = 0;
      for (j = 0; j < N; j++)
      {
        sum += taps[j] * delayLine[j];
      }sum &= 1; // mod 2
      mls[i] = delayLine[N - 1];
      for (j = N - 2; j >= 0; j--)
      {
        delayLine[j + 1] = delayLine[j];
      }
      delayLine[0] = *(bool*)&sum;
    }
    delete []delayLine;
}
void FastHadamard(double *x, long P1, long N)
{
  long i, i1, j, k, k1, k2;
  double temp;
  k1 = P1;
  for (k = 0; k < N; k++)
  {
    k2 = k1 >> 1;
    for (j = 0; j < k2; j++)
    {
      for (i = j; i < P1; i = i + k1)
      {
        i1 = i + k2;
        temp = x[i] + x[i1];
        x[i1] = x[i] - x[i1];
        x[i] = temp;
      }
    }
    k1 = k1 >> 1;
  }
}
void PermuteSignal(float *sig, double *perm, long *tagS, long P)
{
  long i;
  double dc = 0;
  for (i = 0; i < P; i++)
    dc += sig[i];
    perm[0] = -dc;
    for (i = 0; i < P; i++) // Just a permutation of the measured signal
      perm[tagS[i]] = sig[i];
}void PermuteResponse(double *perm, double *resp, long *tagL, long P)
{
  long i;
  const double fact = 1 / double(P + 1);
  for (i = 0; i < P; i++) // Just a permutation of the impulse response
  {
    resp[i] = perm[tagL[i]] * fact;
  }
  resp[P] = 0;
}
void GeneratetagL(bool *mls, long *tagL, long P, long N)
{
  long i, j;long *colSum = new long[P];
  long *index = new long[N];
  for (i = 0; i < P; i++) // Run through all the columns in the autocorr matrix
  {
    colSum[i] = 0;
    for (j = 0; j < N; j++) // Find colSum as the value of the first N elements regarded as a binary number
    {
      colSum[i] += mls[(P + i - j) % P] << (N - 1 - j);
    }
    for ( j = 0; j < N; j++) // Figure out if colSum is a 2^j number and store the column as the j'th index
    {
      if (colSum[i] == (1 << j))
        index[j] = i;
    }
  }
  for (i = 0; i < P; i++) // For each row in the L matrix
  {
    tagL[i] = 0;
    for ( j = 0; j < N; j++) // Find the tagL as the value of the rows in the L matrix regarded as a binary number
    {
      tagL[i] += mls[(P + index[j] - i) % P] * (1 << j);
    }
  }
  delete []colSum;
  delete []index;
}
void GeneratetagS(bool *mls, long *tagS, long P, long N)
{
  long i, j;
  for (i = 0; i < P; i++) // For each column in the S matrix
  {
    tagS[i] = 0;
    for (j = 0; j < N; j++) // Find the tagS as the value of the columns in the S matrix regarded as a binary number
    {
      tagS[i] += mls[(P + i - j) % P] * (1 << (N - 1 - j));
    }
  }
}

void decodePcm16(char *buffer, int buflen, vector<float> &target) {                                              
  target.reserve(target.size() + buflen / 2);
  for (int i = 0; i < buflen; i += 2) {
    short *ps = (short *)(buffer + i);
    target.push_back(((float)*ps + 0.5f) / PCM_QUANT);
  }
}

void usage() {
  cerr << "usage: ir <num> (-s | -l)  # 0 < num <= 16" << endl;
  exit(1);
}

int main(int argc, char **argv) {
  if (argc != 3) {
    usage();
  }
  long N = atoi(argv[1]);
  if (N < 1 || N > 16) usage();

  //const long N = 15;
  const long P = (1 << N) - 1;
  long i;
  bool *mls = new bool[P];
  long *tagL = new long[P];
  long *tagS = new long[P];
  double *perm = new double[P + 1];
  double *resp = new double[P + 1];
  GenerateMls(mls, P, N); // Generate the Maximum length sequence

  char *buf = new char[P*2];

  if (strcmp(argv[2], "-s") == 0) {
    //GenerateSignal(mls, signal, P); // Do a simulated measurement and get the signal
    double *sig = GenerateSignal(mls, P);
    encodePcm(sig, buf, P);

    while (true) {
      cout.write(buf, P*2);
    }
  } else if (strcmp(argv[2], "-l") == 0) {
    GeneratetagL(mls, tagL, P, N); // Generate tagL for the L matrix
    GeneratetagS(mls, tagS, P, N); // Generate tagS for the S matrix
    double *avg = NULL;

    int remaining = 20;
    remaining = 200000;

    while (cin.good() && remaining--) {
      cin.read(buf, P*2);
      if (cin.good()) {
        double interval = (double)(1<<N) / (44100.0);
        double distance = 340 * interval;
        printf("computing for %2.5fs, dist %2.2fm\n", interval, distance);
        vector<float> decoded;
        decodePcm16(buf, P*2, decoded);
        assert(decoded.size() == P);

        //for (int i = 0; i < P; i++) {
        //  printf("%d ", mls[i]);
        //}
        //printf("\n");
        PermuteSignal(decoded.data(), perm, tagS, P); // Permute the signal according to tagS
        FastHadamard(perm, P + 1, N); // Do a Hadamard transform in place
        PermuteResponse(perm, resp, tagL, P); // Permute the impulseresponse according to tagL
      //  printf("Impulse response:\n");
      //  for (i = 0; i < P; i++) {
      //    //printf("%10.5f\n", resp[i]);
      //    printf("%1.4f ", resp[i]);
      //  }
      //  printf("\n");

        if (!avg) {
          avg = new double[P+1];
          for (int i = 0; i <= P; i++) {
            avg[i] = resp[i];
          }
        } else {
          double decay = 0.98;
          for (int i = 0; i <= P; ++i) {
            avg[i] *= decay;
            avg[i] += (1-decay) * resp[i];
          }
        }
        ofstream myfile;
        myfile.open ("tmp-avg.csv");

        cout << "writing... ";
        cout.flush();
        for (int i = 0; i <= P; ++i) {
          myfile << avg[i] << endl;
        }
        myfile.close();
        rename("tmp-avg.csv", "avg.csv");
        cout << "done" << endl;
      }
    }
  }

  delete []mls;
  delete []tagL;
  delete []tagS;
  delete []perm;
  delete []resp;
}
