#include "km10.hpp"

struct CmpAndGroup: KM10 {
  InstructionResult doCAI()   { (void) acGet(); return iNormal; }
  InstructionResult doCAIL()  { return acGet().s  < immediate().s ? iSkip : iNormal; }
  InstructionResult doCAIE()  { return acGet().s == immediate().s ? iSkip : iNormal; }
  InstructionResult doCAILE() { return acGet().s <= immediate().s ? iSkip : iNormal; }
  InstructionResult doCAIA()  { (void) acGet(); return iSkip; }
  InstructionResult doCAIGE() { return acGet().s >= immediate().s ? iSkip : iNormal; }
  InstructionResult doCAIN()  { return acGet().s != immediate().s ? iSkip : iNormal; }
  InstructionResult doCAIG()  { return acGet().s  > immediate().s ? iSkip : iNormal; }

  InstructionResult doCAM()   { (void) acGet(); (void) memGet(); return iNormal; }
  InstructionResult doCAML()  { return acGet().s  < memGet().s ? iSkip : iNormal; }
  InstructionResult doCAME()  { return acGet().s == memGet().s ? iSkip : iNormal; }
  InstructionResult doCAMLE() { return acGet().s <= memGet().s ? iSkip : iNormal; }
  InstructionResult doCAMA()  { (void) acGet(); (void) memGet(); return iSkip; }
  InstructionResult doCAMGE() { return acGet().s >= memGet().s ? iSkip : iNormal; }
  InstructionResult doCAMN()  { return acGet().s != memGet().s ? iSkip : iNormal; }
  InstructionResult doCAMG()  { return acGet().s  > memGet().s ? iSkip : iNormal; }
};


void InstallCmpAndGroup(KM10 &km10) {
  km10.defOp(0300, "CAI",   static_cast<KM10::OpcodeHandler>(&CmpAndGroup::doCAI));
  km10.defOp(0301, "CAIL",  static_cast<KM10::OpcodeHandler>(&CmpAndGroup::doCAIL));
  km10.defOp(0302, "CAIE",  static_cast<KM10::OpcodeHandler>(&CmpAndGroup::doCAIE));
  km10.defOp(0303, "CAILE", static_cast<KM10::OpcodeHandler>(&CmpAndGroup::doCAILE));
  km10.defOp(0304, "CAIA",  static_cast<KM10::OpcodeHandler>(&CmpAndGroup::doCAIA));
  km10.defOp(0305, "CAIGE", static_cast<KM10::OpcodeHandler>(&CmpAndGroup::doCAIGE));
  km10.defOp(0306, "CAIN",  static_cast<KM10::OpcodeHandler>(&CmpAndGroup::doCAIN));
  km10.defOp(0307, "CAIG",  static_cast<KM10::OpcodeHandler>(&CmpAndGroup::doCAIG));
  km10.defOp(0310, "CAM",   static_cast<KM10::OpcodeHandler>(&CmpAndGroup::doCAM));
  km10.defOp(0311, "CAML",  static_cast<KM10::OpcodeHandler>(&CmpAndGroup::doCAML));
  km10.defOp(0312, "CAME",  static_cast<KM10::OpcodeHandler>(&CmpAndGroup::doCAME));
  km10.defOp(0313, "CAMLE", static_cast<KM10::OpcodeHandler>(&CmpAndGroup::doCAMLE));
  km10.defOp(0314, "CAMA",  static_cast<KM10::OpcodeHandler>(&CmpAndGroup::doCAMA));
  km10.defOp(0315, "CAMGE", static_cast<KM10::OpcodeHandler>(&CmpAndGroup::doCAMGE));
  km10.defOp(0316, "CAMN",  static_cast<KM10::OpcodeHandler>(&CmpAndGroup::doCAMN));
  km10.defOp(0317, "CAMG",  static_cast<KM10::OpcodeHandler>(&CmpAndGroup::doCAMG));
}