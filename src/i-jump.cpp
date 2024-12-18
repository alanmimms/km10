#include "km10.hpp"

struct JumpGroup: KM10 {
  InstructionResult doJUMP()   { return iNormal; }
  InstructionResult doJUMPL()  { return acGet().s  < 0 ? iJump : iNormal; }
  InstructionResult doJUMPE()  { return acGet().s == 0 ? iJump : iNormal; }
  InstructionResult doJUMPLE() { return acGet().s <= 0 ? iJump : iNormal; }
  InstructionResult doJUMPA()  { return iJump; }
  InstructionResult doJUMPGE() { return acGet().s >= 0 ? iJump : iNormal; }
  InstructionResult doJUMPN()  { return acGet().s != 0 ? iJump : iNormal; }
  InstructionResult doJUMPG()  { return acGet().s >  0 ? iJump : iNormal; }

  InstructionResult doSKIP()   { return iNormal; }
  InstructionResult doSKIPL()  { return acGet().s  < 0 ? iSkip : iNormal; }
  InstructionResult doSKIPLE() { return acGet().s <= 0 ? iSkip : iNormal; }
  InstructionResult doSKIPA()  { return iSkip; }
  InstructionResult doSKIPG()  { return acGet().s  > 0 ? iSkip : iNormal; }
  InstructionResult doSKIPGE() { return acGet().s >= 0 ? iSkip : iNormal; }
  InstructionResult doSKIPE()  { return acGet().s == 0 ? iSkip : iNormal; }
  InstructionResult doSKIPN()  { return acGet().s != 0 ? iSkip : iNormal; }
};


void InstallJumpGroup(KM10 &km10) {
  km10.defOp(0320, "JUMP",   static_cast<KM10::OpcodeHandler>(&JumpGroup::doJUMP));
  km10.defOp(0321, "JUMPL",  static_cast<KM10::OpcodeHandler>(&JumpGroup::doJUMPL));
  km10.defOp(0322, "JUMPE",  static_cast<KM10::OpcodeHandler>(&JumpGroup::doJUMPE));
  km10.defOp(0323, "JUMPLE", static_cast<KM10::OpcodeHandler>(&JumpGroup::doJUMPLE));
  km10.defOp(0324, "JUMPA",  static_cast<KM10::OpcodeHandler>(&JumpGroup::doJUMPA));
  km10.defOp(0325, "JUMPGE", static_cast<KM10::OpcodeHandler>(&JumpGroup::doJUMPGE));
  km10.defOp(0326, "JUMPN",  static_cast<KM10::OpcodeHandler>(&JumpGroup::doJUMPN));
  km10.defOp(0327, "JUMPG",  static_cast<KM10::OpcodeHandler>(&JumpGroup::doJUMPG));

  km10.defOp(0330, "SKIP",   static_cast<KM10::OpcodeHandler>(&JumpGroup::doSKIP));
  km10.defOp(0331, "SKIPL",  static_cast<KM10::OpcodeHandler>(&JumpGroup::doSKIPL));
  km10.defOp(0332, "SKIPE",  static_cast<KM10::OpcodeHandler>(&JumpGroup::doSKIPE));
  km10.defOp(0333, "SKIPLE", static_cast<KM10::OpcodeHandler>(&JumpGroup::doSKIPLE));
  km10.defOp(0334, "SKIPA",  static_cast<KM10::OpcodeHandler>(&JumpGroup::doSKIPA));
  km10.defOp(0335, "SKIPGE", static_cast<KM10::OpcodeHandler>(&JumpGroup::doSKIPGE));
  km10.defOp(0336, "SKIPN",  static_cast<KM10::OpcodeHandler>(&JumpGroup::doSKIPN));
  km10.defOp(0337, "SKIPG",  static_cast<KM10::OpcodeHandler>(&JumpGroup::doSKIPG));
}
