#ifndef _PACKETER_H_
#define _PACKETER_H_

#include "assigner.h"
#include "codec.h"
#include "config.h"

#include <vector>

class Packeter {
public:
  Packeter(Config *cfg, Codec *codec, Assigner *assigner);

  /**
   * Encodes a message into a sequence of chips, each the same length.
   */
  void encodeMessage(std::vector<char> &message, std::vector<std::vector<bool> > &target);

  /**
   * Decodes a sequence of chips into a message.
   * 
   * Chip elements are bit likelihoods, [-1.0..1.0].
   * Returns 0 on success, -1 on decode failure.
   */
  int decodeMessage(std::vector<std::vector<float> > &chips, std::vector<char> &target);

  /**
   * Returns the number of chips required to encode some bytes.
   */
  int numSymbolsForBytes(int nbytes);

private:
  Config *cfg;
  Codec *codec;
  Assigner *assigner;
};

#endif

