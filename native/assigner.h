#ifndef _ASSIGNER_H_
#define _ASSIGNER_H_

#include <vector>

class Assigner {
public:
  virtual void chipify(std::vector<bool> &bits, std::vector<std::vector<bool> > &chips) = 0;

  virtual void unchipify(std::vector<std::vector<bool> > &chips, std::vector<bool> &bits) = 0;

protected:
  Assigner() {}

};

#endif
