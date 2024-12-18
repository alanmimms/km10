#include "km10.hpp"

struct MoveGroup: KM10 {
  InstructionResult doMOVE()  {acPut(memGet());			return iNormal;}
  InstructionResult doMOVEI() {acPut(immediate());		return iNormal;}
  InstructionResult doMOVEM() {memPut(acGet());			return iNormal;}
  InstructionResult doMOVES() {selfPut(memGet());		return iNormal;}

  InstructionResult doMOVS()  {acPut(swap(memGet()));		return iNormal;}
  InstructionResult doMOVSI() {acPut(swap(immediate()));	return iNormal;}
  InstructionResult doMOVSM() {memPut(swap(acGet()));		return iNormal;}
  InstructionResult doMOVSS() {selfPut(swap(memGet()));		return iNormal;}

  InstructionResult doMOVN()  {acPut(negate(memGet()));		return iNormal;}
  InstructionResult doMOVNI() {acPut(negate(immediate()));	return iNormal;}
  InstructionResult doMOVNM() {memPut(negate(acGet()));		return iNormal;}
  InstructionResult doMOVNS() {selfPut(negate(memGet()));	return iNormal;}

  InstructionResult doMOVM()  {acPut(magnitude(memGet()));	return iNormal;}
  InstructionResult doMOVMI() {acPut(magnitude(immediate()));	return iNormal;}
  InstructionResult doMOVMM() {memPut(magnitude(acGet()));	return iNormal;}
  InstructionResult doMOVMS() {selfPut(magnitude(memGet()));	return iNormal;}

  inline void install() {
    defOp(0200, "MOVE",  static_cast<OpcodeHandler>(&MoveGroup::doMOVE));
    defOp(0201, "MOVEI", static_cast<OpcodeHandler>(&MoveGroup::doMOVEI));
    defOp(0202, "MOVEM", static_cast<OpcodeHandler>(&MoveGroup::doMOVEM));
    defOp(0203, "MOVES", static_cast<OpcodeHandler>(&MoveGroup::doMOVES));

    defOp(0204, "MOVS",  static_cast<OpcodeHandler>(&MoveGroup::doMOVS));
    defOp(0205, "MOVSI", static_cast<OpcodeHandler>(&MoveGroup::doMOVSI));
    defOp(0206, "MOVSM", static_cast<OpcodeHandler>(&MoveGroup::doMOVSM));
    defOp(0207, "MOVSS", static_cast<OpcodeHandler>(&MoveGroup::doMOVSS));

    defOp(0210, "MOVN",  static_cast<OpcodeHandler>(&MoveGroup::doMOVN));
    defOp(0211, "MOVNI", static_cast<OpcodeHandler>(&MoveGroup::doMOVNI));
    defOp(0212, "MOVNM", static_cast<OpcodeHandler>(&MoveGroup::doMOVNM));
    defOp(0213, "MOVNS", static_cast<OpcodeHandler>(&MoveGroup::doMOVNS));

    defOp(0214, "MOVM",  static_cast<OpcodeHandler>(&MoveGroup::doMOVM));
    defOp(0215, "MOVMI", static_cast<OpcodeHandler>(&MoveGroup::doMOVMI));
    defOp(0216, "MOVMM", static_cast<OpcodeHandler>(&MoveGroup::doMOVMM));
    defOp(0217, "MOVMS", static_cast<OpcodeHandler>(&MoveGroup::doMOVMS));
  }
};
