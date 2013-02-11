#include "sync.h"

#include "constants.h"

using namespace std;

static const int MAX_BUFFER_SAMPLES = SAMPLE_RATE * 2; // 2s

Sync::Sync() {
}

void Sync::generateSync(vector<float> &target) {
}

vector<float>::iterator Sync::receiveAudioAndSync(vector<float> &samples) {
  // Append samples to buffer
  buffer.resize(buffer.size() + samples.size());
  std::copy(samples.begin(), samples.end(), buffer.end());

  // Look for sync...

  // Discard head of buffer if no longer needed
  if (buffer.size() > MAX_BUFFER_SAMPLES) {
    deque<float>::iterator head = buffer.begin();
    deque<float>::iterator newHead = head + (buffer.size() - MAX_BUFFER_SAMPLES);
    buffer.erase(head, newHead);
  }

  // No sync yet
  return samples.end();
}
