#ifndef _ASSIGNERS_H_
#define _ASSIGNERS_H_

#include "assigner.h"

class CombinadicAssigner : Assigner {
public:
  CombinadicAssigner(int nchans);
  void chipify(std::vector<bool> &bits, std::vector<std::vector<bool> > &chips);
  void unchipify(std::vector<std::vector<float> > &chips, std::vector<float> &bits);
private:
  int nchans;
  int width;
};

#endif
