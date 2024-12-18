#include "km10.hpp"

struct CmpAndGroup: KM10 {

  inline W36 copyHRR(W36 src, W36 dst) {return W36(dst.lhu, src.rhu);}
  inline W36 copyHRL(W36 src, W36 dst) {return W36(src.rhu, dst.rhu);}
  inline W36 copyHLL(W36 src, W36 dst) {return W36(src.lhu, dst.rhu);}
  inline W36 copyHLR(W36 src, W36 dst) {return W36(dst.lhu, src.lhu);}
};


void InstallCmpAndGroup(KM10 &km10) {
}
