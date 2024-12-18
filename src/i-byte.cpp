// TODO:
// * Move BytePointer stuff here since it is only used here?

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
};



void InstallByteGroup(KM10 &km10) {
    km10.defOp(0133, "IBP/ADJBP", static_cast<KM10::OpcodeHandler>(&ByteGroup::doIBP_ADJBP));
    km10.defOp(0134, "ILDB",	  static_cast<KM10::OpcodeHandler>(&ByteGroup::doILDB));
    km10.defOp(0135, "LDB",	  static_cast<KM10::OpcodeHandler>(&ByteGroup::doLDB));
    km10.defOp(0136, "IDPB",	  static_cast<KM10::OpcodeHandler>(&ByteGroup::doIDPB));
    km10.defOp(0137, "DPB",	  static_cast<KM10::OpcodeHandler>(&ByteGroup::doDPB));
}
