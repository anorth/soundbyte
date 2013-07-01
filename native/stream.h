#ifndef _STREAM_H_
#define _STREAM_H_

#include <cassert>

#include "buffer.h"

template<class T>
class Stream : public Buffer<T> {
public:

  virtual ~Stream() {}
  virtual void pull() {}
};

template<class T, class S>
class ChainedStream : public Stream<T> {
public:

  ChainedStream(Stream<S>& source) : source(source) { }
  virtual ~ChainedStream() {}

  virtual void pull() {
    source.pull();
    doPull();
  }

protected:
  virtual void doPull() = 0;
  Stream<S>& source;
};


#endif

