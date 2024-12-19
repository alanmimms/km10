#include "km10.hpp"

struct MoveGroup: KM10 {

  W36 negate(W36 src) {
    W36 v(-src.s);
    if (src.u == W36::bit0) flags.tr1 = flags.ov = flags.cy1 = 1;
    if (src.u == 0) flags.cy0 = flags.cy1 = 1;
    return v;
  }


  W36 magnitude(W36 src) {
    W36 v(src.s < 0 ? -src.s : src.s);
    if (src.u == W36::bit0) flags.tr1 = flags.ov = flags.cy1 = 1;
    return v;
  }


  inline W36 swap(W36 src) {
    return W36{src.rhu, src.lhu};
  }


  void selfPut(W36 v) {
    memPut(v);
    if (iw.ac != 0) acPut(v);
  }


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
};


void InstallMoveGroup(KM10 &km10) {
  km10.defOp(0200, "MOVE",  static_cast<KM10::OpcodeHandler>(&MoveGroup::doMOVE));
  km10.defOp(0201, "MOVEI", static_cast<KM10::OpcodeHandler>(&MoveGroup::doMOVEI));
  km10.defOp(0202, "MOVEM", static_cast<KM10::OpcodeHandler>(&MoveGroup::doMOVEM));
  km10.defOp(0203, "MOVES", static_cast<KM10::OpcodeHandler>(&MoveGroup::doMOVES));

  km10.defOp(0204, "MOVS",  static_cast<KM10::OpcodeHandler>(&MoveGroup::doMOVS));
  km10.defOp(0205, "MOVSI", static_cast<KM10::OpcodeHandler>(&MoveGroup::doMOVSI));
  km10.defOp(0206, "MOVSM", static_cast<KM10::OpcodeHandler>(&MoveGroup::doMOVSM));
  km10.defOp(0207, "MOVSS", static_cast<KM10::OpcodeHandler>(&MoveGroup::doMOVSS));

  km10.defOp(0210, "MOVN",  static_cast<KM10::OpcodeHandler>(&MoveGroup::doMOVN));
  km10.defOp(0211, "MOVNI", static_cast<KM10::OpcodeHandler>(&MoveGroup::doMOVNI));
  km10.defOp(0212, "MOVNM", static_cast<KM10::OpcodeHandler>(&MoveGroup::doMOVNM));
  km10.defOp(0213, "MOVNS", static_cast<KM10::OpcodeHandler>(&MoveGroup::doMOVNS));

  km10.defOp(0214, "MOVM",  static_cast<KM10::OpcodeHandler>(&MoveGroup::doMOVM));
  km10.defOp(0215, "MOVMI", static_cast<KM10::OpcodeHandler>(&MoveGroup::doMOVMI));
  km10.defOp(0216, "MOVMM", static_cast<KM10::OpcodeHandler>(&MoveGroup::doMOVMM));
  km10.defOp(0217, "MOVMS", static_cast<KM10::OpcodeHandler>(&MoveGroup::doMOVMS));
}

