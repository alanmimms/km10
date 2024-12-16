// TODO:
// * Move BytePointer stuff here since it is only used here.

#include "km10.hpp"
#include "bytepointer.hpp"

void KM10::installByteGroup() {

  defOp(0133, "IBP/ADJBP", [&]() {
    BytePointer *bp = BytePointer::makeFrom(ea, *this);

    if (iw.ac == 0) {		// IBP
      bp->inc(c);
    } else {			// ADJBP
      bp->adjust(iw.ac, *this);
    }

    return iNormal;
  });

  defOp(0134, "ILDB", [&]() {
    BytePointer *bp = BytePointer::makeFrom(ea, *this);
    bp->inc(c);
    acPut(bp->getByte(c));
    return iNormal;
  });

  defOp(0135, "LDB", [&]() {
    BytePointer *bp = BytePointer::makeFrom(ea, *this);
    acPut(bp->getByte(c));
    return iNormal;
  });

  defOp(0136, "IDPB", [&]() {
    BytePointer *bp = BytePointer::makeFrom(ea, *this);
    bp->inc(c);
    bp->putByte(acGet(), *this);
    return iNormal;
  });

  defOp(0137, "DPB", [&]() {
    BytePointer *bp = BytePointer::makeFrom(ea, *this);
    bp->putByte(acGet(), *this);
    return iNormal;
  });
}
