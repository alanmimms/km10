#include "km10.hpp"

struct IOGroup: KM10 {

  IResult doIO() {
    if (logger.io) logger.s << "; ioDev=" << oct << iw.ioDev << " ioOp=" << oct << iw.ioOp;
    return Device::handleIO(iw, ea);
  }
};


void InstallIOGroup(KM10 &km10) {

  for (unsigned op=0700; op <= 0777; ++op) {
    km10.defOp(op, "", static_cast<KM10::OpcodeHandler>(&IOGroup::doIO));
  }
}
