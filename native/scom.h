

/**
 * Encodes a message payload into a waveform.
 *
 * @return the number of waveform bytes written, or -1 on failure.
 */
extern int encodeMessage(char *payload, int payloadLength, char *waveform, int waveformCapacity);

extern const char *STRING;
