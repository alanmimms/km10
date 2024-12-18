// TODO:
// * Move BytePointer stuff here since it is only used here.

#include "km10.hpp"
#include "bytepointer.hpp"

struct ByteGroup: KM10 {

  InstructionResult doIBP_ADJBP() {
    BytePointer *bp = BytePointer::makeFrom(ea, *this);

    if (iw.ac == 0) {		// IBP
      bp->inc(*this);
    } else {			// ADJBP
      bp->adjust(iw.ac, *this);
    }

    return iNormal;
  };

  InstructionResult doILDB() {
    BytePointer *bp = BytePointer::makeFrom(ea, *this);
    bp->inc(*this);
    acPut(bp->getByte(*this));
    return iNormal;
  };

  InstructionResult doLDB() {
    BytePointer *bp = BytePointer::makeFrom(ea, *this);
    acPut(bp->getByte(*this));
    return iNormal;
  };

  InstructionResult doIDPB() {
    BytePointer *bp = BytePointer::makeFrom(ea, *this);
    bp->inc(*this);
    bp->putByte(acGet(), *this);
    return iNormal;
  };

  InstructionResult doDPB() {
    BytePointer *bp = BytePointer::makeFrom(ea, *this);
    bp->putByte(acGet(), *this);
    return iNormal;
  };


  void install() {
    defOp(0133, "IBP/ADJBP", static_cast<OpcodeHandler>(&ByteGroup::doIBP_ADJBP));
    defOp(0134, "ILDB",	     static_cast<OpcodeHandler>(&ByteGroup::doILDB));
    defOp(0135, "LDB",	     static_cast<OpcodeHandler>(&ByteGroup::doLDB));
    defOp(0136, "IDPB",	     static_cast<OpcodeHandler>(&ByteGroup::doIDPB));
    defOp(0137, "DPB",	     static_cast<OpcodeHandler>(&ByteGroup::doDPB));
  }
};
