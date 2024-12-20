#include "km10.hpp"

struct JumpGroup: KM10 {
  void logFlow(const char *msg) {
    if (logger.pc) logger.s << " [" << msg << "]";
  }

  InstructionResult doJUMP()   { return iNormal; }
  InstructionResult doJUMPL()  { return acGet().s  < 0 ? iJump : iNormal; }
  InstructionResult doJUMPE()  { return acGet().s == 0 ? iJump : iNormal; }
  InstructionResult doJUMPLE() { return acGet().s <= 0 ? iJump : iNormal; }
  InstructionResult doJUMPA()  { return iJump; }
  InstructionResult doJUMPGE() { return acGet().s >= 0 ? iJump : iNormal; }
  InstructionResult doJUMPN()  { return acGet().s != 0 ? iJump : iNormal; }
  InstructionResult doJUMPG()  { return acGet().s >  0 ? iJump : iNormal; }

  InstructionResult doSKIP()   { if (iw.ac != 0) acPut(memGet()); return iNormal; }
  InstructionResult doSKIPL()  { if (iw.ac != 0) acPut(memGet()); return memGet().s  < 0 ? iSkip : iNormal; }
  InstructionResult doSKIPLE() { if (iw.ac != 0) acPut(memGet()); return memGet().s <= 0 ? iSkip : iNormal; }
  InstructionResult doSKIPA()  { if (iw.ac != 0) acPut(memGet()); return iSkip; }
  InstructionResult doSKIPG()  { if (iw.ac != 0) acPut(memGet()); return memGet().s  > 0 ? iSkip : iNormal; }
  InstructionResult doSKIPGE() { if (iw.ac != 0) acPut(memGet()); return memGet().s >= 0 ? iSkip : iNormal; }
  InstructionResult doSKIPE()  { if (iw.ac != 0) acPut(memGet()); return memGet().s == 0 ? iSkip : iNormal; }
  InstructionResult doSKIPN()  { if (iw.ac != 0) acPut(memGet()); return memGet().s != 0 ? iSkip : iNormal; }


  InstructionResult doJRST() {

    switch (iw.ac) {
    case 000:					// JRST
      return iJump;

    case 001:					// PORTAL
      logger.nyi(*this);
      break;

    case 002:					// JRSTF
      cout << "JRSTF ea=" << ea.fmt36() << logger.endl << flush;
      restoreFlags(ea);
      return iJump;

    case 004:					// HALT
      cerr << "[HALT at " << pc.fmtVMA() << "]" << logger.endl;
      running = false;
      return iHALT;

    case 005:					// XJRSTF
      logger.nyi(*this);
      break;

    case 006:					// XJEN
      pi.dismissInterrupt();
      logger.nyi(*this);
      break;

    case 007:					// XPCW
      logger.nyi(*this);
      break;

    case 010:					// 25440 - no mnemonic
      restoreFlags(ea);
      break;

    case 012:					// JEN
      cerr << ">>>>>> JEN ea=" << ea.fmtVMA() << logger.endl << flush;
      pi.dismissInterrupt();
      restoreFlags(ea);
      return iJump;

    case 014:					// SFM
      logger.nyi(*this);
      break;

    default:
      logger.nyi(*this);
      break;
    }

    return iJump;
  }



  InstructionResult doJFCL() {
    unsigned wasFlags = flags.u;
    unsigned testFlags = (unsigned) iw.ac << 9; // Align with OV,CY0,CY1,FOV
    flags.u &= ~testFlags;
    if (wasFlags & testFlags) return iJump;	// JUMP
    return iNormal;				// NO JUMP
  }


  InstructionResult doPXCT() {

    if (userMode() || iw.ac == 0) {
      return iXCT;
    } else {					// PXCT
      logger.nyi(*this);
      running = false;
      return iHALT;		// XXX for now
    }
  }


  void push(W36 v, W36 acN) {
    W36 ac = acGetN(acN);

    if (pc.isSection0() || ac.lhs < 0 || (ac.lhu & 0007777) == 0) {
      ac = W36(ac.lhu + 1, ac.rhu + 1);

      if (ac.lhu == 0)
	flags.tr2 = 1;
      else			// Correct? Don't access memory for full stack?
	memPutN(v, ac.rhu);
    } else {
      ac = ac + 1;
      memPutN(ac.vma, v);
    }

    acPutN(ac, acN);
  }


  W36 pop(unsigned acN) {
    W36 ac = acGetN(acN);
    W36 poppedWord;

    if (pc.isSection0() || ac.lhs < 0 || (ac.lhu & 0007777) == 0) {
      poppedWord = memGetN(ac.rhu);
      ac = W36(ac.lhu - 1, ac.rhu - 1);
      if (ac.lhs == -1) flags.tr2 = 1;
    } else {
      poppedWord = memGetN(ac.vma);
      ac = ac - 1;
    }

    acPutN(ac, acN);
    return poppedWord;
  }


  InstructionResult doPUSHJ() {
    // Note this sets the flags that are cleared by PUSHJ before
    // push() since push() can set flags.tr2.
    flags.fpd = flags.afi = flags.tr1 = flags.tr2 = 0;
    push(pc.isSection0() ? flagsWord(pc.rhu + 1) : W36(pc.vma + 1), iw.ac);
    if (inInterrupt) flags.usr = flags.pub = 0;
    return iJump;
  }


  InstructionResult doPUSH() {
    push(memGet(), iw.ac);
    return iNormal;
  }

  InstructionResult doPOP() {
    memPut(pop(iw.ac));
    return iNormal;
  }

  InstructionResult doPOPJ() {
    ea.rhu = pop(iw.ac).rhu;
    return iJump;
  }

  InstructionResult doJSR() {
    W36 tmp = ea.isSection0() ? flagsWord(pc.rhu + 1) : W36(pc.vma + 1);
    cerr << ">>>>>> JSR saved PC=" << tmp.fmt36() << "  ea=" << ea.fmt36()
	 << logger.endl << flush;
    memPut(tmp);
    ++ea.rhu;			// Advance past flags word to target instruction
    flags.fpd = flags.afi = flags.tr2 = flags.tr1 = 0;
    if (inInterrupt) flags.usr = flags.pub = 0;
    return iJump;
  }

  InstructionResult doJSP() {
    W36 tmp = ea.isSection0() ? flagsWord(tmp.rhu) : W36(tmp.vma);
    cerr << ">>>>>> JSP set ac=" << tmp.fmt36() << "  ea=" << ea.fmt36()
	 << logger.endl << flush;
    acPut(tmp);
    flags.fpd = flags.afi = flags.tr2 = flags.tr1 = 0;
    if (inInterrupt) flags.usr = flags.pub = 0;
    return iJump;
  }

  InstructionResult doJSA() {
    memPut(acGet());
    acPut(W36(ea.rhu, pc.rhu + 1));
    ++ea.rhu;
    if (inInterrupt) flags.usr = 0;
    return iJump;
  }

  InstructionResult doJRA() {
    acPut(memGetN(acGet().lhu));
    return iJump;
  }

  InstructionResult doXCT() {
    if (userMode() || iw.ac == 0) {
      return iXCT;
    } else {					// PXCT
      logger.nyi(*this);
      return iXCT;
    }
  }

  InstructionResult doMAP() {
    logger.nyi(*this);
    return iNormal;
  }


  InstructionResult doAOBJP() {
    W36 tmp = acGet();
    tmp = W36(tmp.lhu + 1, tmp.rhu + 1);
    acPut(tmp);

    if (tmp.ext64() >= 0) {
      return iJump;
    } else {
      return iNormal;
    }
  }

  InstructionResult doAOBJN() {
    W36 tmp = acGet();
    tmp = W36(tmp.lhu + 1, tmp.rhu + 1);
    acPut(tmp);

    if (tmp.ext64() < 0) {
      return iJump;
    } else {
      return iNormal;
    }
  }


  InstructionResult doADJSP() {
    W36 a = acGet();

    if (pc.isSection0() || a.lhs < 0) {
      int32_t previousLH = a.lhs;

      a.lhs += ea.rhs;
      a.rhs += ea.rhs;
      acPut(a);

      if (ea.rhs < 0) {

	if (previousLH >= 0 && a.lhs < 0) {
	  flags.tr2 = 1;
	  return iTrap;
	}
      } else if (ea.rhs > 0) {

	if (previousLH < 0 && a.lhs >= 0) {
	  flags.tr2 = 1;
	  return iTrap;
	}
      }
    } else if (!pc.isSection0() && (a.lhu & W36::bit0) == 0 && a.extract(6, 17) != 0) {
      a.s += ea.rhs;
      acPut(a);
    }

    return iNormal;
  }
};


void InstallJumpGroup(KM10 &km10) {
  km10.defOp(0105, "ADJSP",  static_cast<KM10::OpcodeHandler>(&JumpGroup::doADJSP));

  km10.defOp(0252, "AOBJP",  static_cast<KM10::OpcodeHandler>(&JumpGroup::doAOBJP));
  km10.defOp(0253, "AOBJN",  static_cast<KM10::OpcodeHandler>(&JumpGroup::doAOBJN));
  km10.defOp(0254, "JRST",   static_cast<KM10::OpcodeHandler>(&JumpGroup::doJRST));
  km10.defOp(0255, "JFCL",   static_cast<KM10::OpcodeHandler>(&JumpGroup::doJFCL));
  km10.defOp(0256, "XCT",    static_cast<KM10::OpcodeHandler>(&JumpGroup::doXCT));
  km10.defOp(0257, "MAP",    static_cast<KM10::OpcodeHandler>(&JumpGroup::doMAP));
  km10.defOp(0260, "PUSHJ",  static_cast<KM10::OpcodeHandler>(&JumpGroup::doPUSHJ));
  km10.defOp(0261, "PUSH",   static_cast<KM10::OpcodeHandler>(&JumpGroup::doPUSH));
  km10.defOp(0262, "POP",    static_cast<KM10::OpcodeHandler>(&JumpGroup::doPOP));
  km10.defOp(0263, "POPJ",   static_cast<KM10::OpcodeHandler>(&JumpGroup::doPOPJ));
  km10.defOp(0264, "JSR",    static_cast<KM10::OpcodeHandler>(&JumpGroup::doJSR));
  km10.defOp(0265, "JSP",    static_cast<KM10::OpcodeHandler>(&JumpGroup::doJSP));
  km10.defOp(0266, "JSA",    static_cast<KM10::OpcodeHandler>(&JumpGroup::doJSA));
  km10.defOp(0267, "JRA",    static_cast<KM10::OpcodeHandler>(&JumpGroup::doJRA));

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
