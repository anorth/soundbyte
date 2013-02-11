#ifndef _SYNC_H_
#define _SYNC_H_

#include <vector>
#include <deque>

class Sync {
public:
  Sync();

  /**
   * Generates a sync signal into target.
   */
  void generateSync(std::vector<float> &target);

  /**
   * Receives the next segment of audio and attempt to synchronise with this and
   * previous samples.
   * 
   * If sync is successful, returns an iterator addressing the first sample after
   * sync and resets state. If sync fails, returns samples.end() and maintains state
   * to continue processing with the next contiguous samples.
   */
  std::vector<float>::iterator receiveAudioAndSync(std::vector<float> &samples);

  /**
   * Resets internal sync state. 
   */
  void resetSync();

private:
  std::deque<float> buffer;
};

#endif

