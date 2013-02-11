#include "packeter.h"

using namespace std;

Packeter::Packeter(Encoder &encoder, Assigner &assigner) :
    encoder(&encoder),
    assigner(&assigner) {
}

void Packeter::encodeMessage(vector<char> &message, vector<vector<bool> > &target) {
}

int Packeter::decodeMessage(vector<vector<float> > &chips, vector<char> &target) {

  return 0;
}

