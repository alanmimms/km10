#include "km10.hpp"

struct AOxSOxGroup: KM10 {

  IResult checkAOTraps(W36 a) {

    if (a.u == (1ull << 35) - 1) {
      flags.tr1 = flags.ov = flags.cy1 = 1;
      return iTrap;
    } else if (a.s == -1ll) {
      flags.cy0 = flags.cy1 = 1;
      return iNormal;
    } else {
      return iNormal;
    }
  }

  IResult doAOJ(auto condF, auto actionF) {
    W36 a = acGet();
    IResult result = checkAOTraps(a);

    ++a.u;
    acPut(a);

    if (condF(a)) {
      return actionF();
    } else {
      return result;
    }
  }

  IResult doAOS(auto condF, auto actionF) {
    W36 a = memGet();
    IResult result = checkAOTraps(a);

    ++a.u;
    memPut(a);
    if (iw.ac != 0) acPut(a);

    if (condF(a)) {
      return actionF();
    } else {
      return result;
    }
  }


  IResult checkSOTraps(W36 a) {

    if (a.u == W36::bit0) {
      flags.tr1 = flags.ov = flags.cy1 = 1;
      return iTrap;
    } else if (a.u != 0) {
      flags.cy0 = flags.cy1 = 1;
      return iNormal;
    } else {
      return iNormal;
    }
  }

  IResult doSOJ(auto condF, auto actionF) {
    W36 a = acGet();
    IResult result = checkSOTraps(a);

    --a.u;
    acPut(a);

    if (condF(a)) {
      return actionF();
    } else {
      return result;
    }
  }

  IResult doSOS(auto condF, auto actionF) {
    W36 a = memGet();
    IResult result = checkSOTraps(a);

    --a.u;
    memPut(a);
    if (iw.ac != 0) acPut(a);

    if (condF(a)) {
      return actionF();
    } else {
      return result;
    }
  }


  IResult doAOJ() {
    return doAOJ([](W36 a) {return false;},
		 [&km10 = *this]() {return iNormal;});
  }

  IResult doAOJL() {
    return doAOJ([](W36 a) {return a.s < 0;},
		 [&km10 = *this]() {return iJump;});
  }

  IResult doAOJE() {
    return doAOJ([](W36 a) {return a.s == 0;},
		 [&km10 = *this]() {return iJump;});
  }


  IResult doAOJLE() {
    return doAOJ([](W36 a) {return a.s <= 0;},
		 [&km10 = *this]() {return iJump;});
  }

  IResult doAOJA() {
    return doAOJ([](W36 a) {return true;},
		 [&km10 = *this]() {return iJump;});
  }


  IResult doAOJGE() {
    return doAOJ([](W36 a) {return a.s >= 0;},
		 [&km10 = *this]() {return iJump;});
  }

  IResult doAOJN() {
    return doAOJ([](W36 a) {return a.s != 0;},
		 [&km10 = *this]() {return iJump;});
  }


  IResult doAOJG() {
    return doAOJ([](W36 a) {return a.s > 0;},
		 [&km10 = *this]() {return iJump;});
  }

  IResult doAOS() {
    return doAOS([](W36 a) {return false;},
		 [&km10 = *this]() {return iNormal;});
  }

  IResult doAOSL() {
    return doAOS([](W36 a) {return a.s < 0;},
		 [&km10 = *this]() {return iSkip;});
  }

  IResult doAOSE() {
    return doAOS([](W36 a) {return a.s == 0;},
		 [&km10 = *this]() {return iSkip;});
  }


  IResult doAOSLE() {
    return doAOS([](W36 a) {return a.s <= 0;},
		 [&km10 = *this]() {return iSkip;});
  }

  IResult doAOSA() {
    return doAOS([](W36 a) {return true;},
		 [&km10 = *this]() {return iSkip;});
  }


  IResult doAOSGE() {
    return doAOS([](W36 a) {return a.s >= 0;},
		 [&km10 = *this]() {return iSkip;});
  }

  IResult doAOSN() {
    return doAOS([](W36 a) {return a.s != 0;},
		 [&km10 = *this]() {return iSkip;});
  }


  IResult doAOSG() {
    return doAOS([](W36 a) {return a.s > 0;},
		 [&km10 = *this]() {return iSkip;});
  }



  IResult doSOJ() {
    return doSOJ([](W36 a) {return false;},
		 [&km10 = *this]() {return iNormal;});
  }

  IResult doSOJL() {
    return doSOJ([](W36 a) {return a.s < 0;},
		 [&km10 = *this]() {return iJump;});
  }

  IResult doSOJE() {
    return doSOJ([](W36 a) {return a.s == 0;},
		 [&km10 = *this]() {return iJump;});
  }


  IResult doSOJLE() {
    return doSOJ([](W36 a) {return a.s <= 0;},
		 [&km10 = *this]() {return iJump;});
  }

  IResult doSOJA() {
    return doSOJ([](W36 a) {return true;},
		 [&km10 = *this]() {return iJump;});
  }


  IResult doSOJGE() {
    return doSOJ([](W36 a) {return a.s >= 0;},
		 [&km10 = *this]() {return iJump;});
  }

  IResult doSOJN() {
    return doSOJ([](W36 a) {return a.s != 0;},
		 [&km10 = *this]() {return iJump;});
  }


  IResult doSOJG() {
    return doSOJ([](W36 a) {return a.s > 0;},
		 [&km10 = *this]() {return iJump;});
  }




  IResult doSOS() {
    return doSOS([](W36 a) {return false;},
		 [&km10 = *this]() {return iNormal;});
  }

  IResult doSOSL() {
    return doSOS([](W36 a) {return a.s < 0;},
		 [&km10 = *this]() {return iSkip;});
  }

  IResult doSOSE() {
    return doSOS([](W36 a) {return a.s == 0;},
		 [&km10 = *this]() {return iSkip;});
  }


  IResult doSOSLE() {
    return doSOS([](W36 a) {return a.s <= 0;},
		 [&km10 = *this]() {return iSkip;});
  }

  IResult doSOSA() {
    return doSOS([](W36 a) {return true;},
		 [&km10 = *this]() {return iSkip;});
  }


  IResult doSOSGE() {
    return doSOS([](W36 a) {return a.s >= 0;},
		 [&km10 = *this]() {return iSkip;});
  }

  IResult doSOSN() {
    return doSOS([](W36 a) {return a.s != 0;},
		 [&km10 = *this]() {return iSkip;});
  }


  IResult doSOSG() {
    return doSOS([](W36 a) {return a.s > 0;},
		 [&km10 = *this]() {return iSkip;});
  }
};


void InstallAOxSOxGroup(KM10 &km10) {
  km10.defOp(0340, "AOJ",   static_cast<KM10::OpcodeHandler>(&AOxSOxGroup::doAOJ));
  km10.defOp(0341, "AOJL",  static_cast<KM10::OpcodeHandler>(&AOxSOxGroup::doAOJL));
  km10.defOp(0342, "AOJE",  static_cast<KM10::OpcodeHandler>(&AOxSOxGroup::doAOJE));
  km10.defOp(0343, "AOJLE", static_cast<KM10::OpcodeHandler>(&AOxSOxGroup::doAOJLE));
  km10.defOp(0344, "AOJA",  static_cast<KM10::OpcodeHandler>(&AOxSOxGroup::doAOJA));
  km10.defOp(0345, "AOJGE", static_cast<KM10::OpcodeHandler>(&AOxSOxGroup::doAOJGE));
  km10.defOp(0346, "AOJN",  static_cast<KM10::OpcodeHandler>(&AOxSOxGroup::doAOJN));
  km10.defOp(0347, "AOJG",  static_cast<KM10::OpcodeHandler>(&AOxSOxGroup::doAOJG));
  km10.defOp(0350, "AOS",   static_cast<KM10::OpcodeHandler>(&AOxSOxGroup::doAOS));
  km10.defOp(0351, "AOSL",  static_cast<KM10::OpcodeHandler>(&AOxSOxGroup::doAOSL));
  km10.defOp(0352, "AOSE",  static_cast<KM10::OpcodeHandler>(&AOxSOxGroup::doAOSE));
  km10.defOp(0353, "AOSLE", static_cast<KM10::OpcodeHandler>(&AOxSOxGroup::doAOSLE));
  km10.defOp(0354, "AOSA",  static_cast<KM10::OpcodeHandler>(&AOxSOxGroup::doAOSA));
  km10.defOp(0355, "AOSGE", static_cast<KM10::OpcodeHandler>(&AOxSOxGroup::doAOSGE));
  km10.defOp(0356, "AOSN",  static_cast<KM10::OpcodeHandler>(&AOxSOxGroup::doAOSN));
  km10.defOp(0357, "AOSG",  static_cast<KM10::OpcodeHandler>(&AOxSOxGroup::doAOSG));
  km10.defOp(0360, "SOJ",   static_cast<KM10::OpcodeHandler>(&AOxSOxGroup::doSOJ));
  km10.defOp(0361, "SOJL",  static_cast<KM10::OpcodeHandler>(&AOxSOxGroup::doSOJL));
  km10.defOp(0362, "SOJE",  static_cast<KM10::OpcodeHandler>(&AOxSOxGroup::doSOJE));
  km10.defOp(0363, "SOJLE", static_cast<KM10::OpcodeHandler>(&AOxSOxGroup::doSOJLE));
  km10.defOp(0364, "SOJA",  static_cast<KM10::OpcodeHandler>(&AOxSOxGroup::doSOJA));
  km10.defOp(0365, "SOJGE", static_cast<KM10::OpcodeHandler>(&AOxSOxGroup::doSOJGE));
  km10.defOp(0366, "SOJN",  static_cast<KM10::OpcodeHandler>(&AOxSOxGroup::doSOJN));
  km10.defOp(0367, "SOJG",  static_cast<KM10::OpcodeHandler>(&AOxSOxGroup::doSOJG));
  km10.defOp(0370, "SOS",   static_cast<KM10::OpcodeHandler>(&AOxSOxGroup::doSOS));
  km10.defOp(0371, "SOSL",  static_cast<KM10::OpcodeHandler>(&AOxSOxGroup::doSOSL));
  km10.defOp(0372, "SOSE",  static_cast<KM10::OpcodeHandler>(&AOxSOxGroup::doSOSE));
  km10.defOp(0373, "SOSLE", static_cast<KM10::OpcodeHandler>(&AOxSOxGroup::doSOSLE));
  km10.defOp(0374, "SOSA",  static_cast<KM10::OpcodeHandler>(&AOxSOxGroup::doSOSA));
  km10.defOp(0375, "SOSGE", static_cast<KM10::OpcodeHandler>(&AOxSOxGroup::doSOSGE));
  km10.defOp(0376, "SOSN",  static_cast<KM10::OpcodeHandler>(&AOxSOxGroup::doSOSN));
  km10.defOp(0377, "SOSG",  static_cast<KM10::OpcodeHandler>(&AOxSOxGroup::doSOSG));
}
