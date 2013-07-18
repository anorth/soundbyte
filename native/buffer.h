#ifndef _BUFFER_H_
#define _BUFFER_H_

#include <vector>
#include <cassert>

template<class T>
class Buffer {
public:
  typedef typename std::vector<T>::iterator iterator;

  Buffer() {
    start = 0;
    ratio = 5;
    assert(size() == 0);
  }

  virtual ~Buffer() {}

  Buffer(std::vector<T>& other) : data(other) {
    start = 0;
    ratio = 5;
  }

  iterator begin() {
    return data.begin() + start;
  }

  iterator end() {
    return data.end();
  }

  int size() {
    return data.size() - start;
  }

  T* raw() {
    return data.data() + start;
  }

  template <class InputIterator>
  void append (InputIterator first, InputIterator last) {
    assert(first <= last);
    data.insert(data.end(), first, last);
  }

  void push_back (const T& val) {
    data.push_back(val);
  }

  void consumeAll() {
    consume(size());
  }

  void consume(int count) {
    assert(count <= size());
    start += count;

    maybeCompact();
  }

  void consumeUntil(iterator excluding) {
    assert(excluding >= begin() && excluding <= end());
    consume(excluding - begin());
  }

  void maybeCompact() {
    if (size() > 0 && data.size() / size() > ratio) {
      data.erase(data.begin(), data.begin() + start);
      start = 0;
    }
  }

private:
  // config
  int ratio;

  // state
  int start;
  std::vector<T> data;
};

#endif

