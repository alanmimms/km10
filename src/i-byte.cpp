#include "km10.hpp"
#include "bytepointer.hpp"

void installByteGroup(KM10 &c) {

  c.defOp(0133, "IBP/ADJBP", [&]() {
    BytePointer *bp = BytePointer::makeFrom(c.ea, c);

    if (c.iw.ac == 0) {		// IBP
      bp->inc(c);
    } else {			// ADJBP
      bp->adjust(c.iw.ac, c);
    }

    return iNormal;
  });

  c.defOp(0134, "ILDB", [&]() {
    BytePointer *bp = BytePointer::makeFrom(c.ea, c);
    bp->inc(c);
    c.acPut(bp->getByte(c));
    return iNormal;
  });

  c.defOp(0135, "LDB", [&]() {
    BytePointer *bp = BytePointer::makeFrom(c.ea, c);
    c.acPut(bp->getByte(c));
    return iNormal;
  });

  c.defOp(0136, "IDPB", [&]() {
    BytePointer *bp = BytePointer::makeFrom(c.ea, c);
    bp->inc(c);
    bp->putByte(c.acGet(), c);
    return iNormal;
  });

  c.defOp(0137, "DPB", [&]() {
    BytePointer *bp = BytePointer::makeFrom(c.ea, c);
    bp->putByte(c.acGet(), c);
    return iNormal;
  });
}
