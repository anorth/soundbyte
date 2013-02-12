#ifndef _SCOM_H_
#define _SCOM_H_

// For testing
extern const char *HELLO;

/**
 * Initialises system.
 */
void init();

/**
 * Encodes a message payload into a waveform.
 *
 * @return the number of waveform bytes written, or -1 on failure.
 */
int encodeMessage(char *payload, int payloadLength, char *waveform, int waveformCapacity);

/**
 * Decodes the next segment of audio.
 *
 * After this method, messageAvailable indicates whether a complete message has been received.
 */
void decodeAudio(char *buffer, int buflen);

/**
 * Whether a message is available.
 */
bool messageAvailable();

/**
 * Writes the next message contents into buffer then discards it.
 *
 * @return number of bytes written, 0 if the buffer is too small, -1 on error.
 */
int takeMessage(char *buffer, int bufferCapacity);

#endif
