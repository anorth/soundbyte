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
   * Returns:
   *   negative number  -> decode failure.
   *   zero-or-positive -> number of chunks remaining to complete the message.
   *
   * WARNING: STATEFUL (only decode 1 at a time per packeter).
   */
  int decodePartial(std::vector<std::vector<float> > &chips, std::vector<char> &target);

  /**
   * Returns the number of chips the packeter will accept at a time for decoding.
   */
  int chunkChips();

private:
  Config *cfg;
  Codec *codec;
  Assigner *assigner;

  // state
  int remaining;
  int successful;
  int total;
};

#endif

