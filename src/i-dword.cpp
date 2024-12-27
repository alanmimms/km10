#include "km10.hpp"


struct DWordGroup: KM10 {

  IResult doDADD() {
    auto a1 = W72{memGetN(ea.u+0), memGetN(ea.u+1)};
    auto a2 = W72{acGetN(iw.ac+0), acGetN(iw.ac+1)};

    int128_t s1 = a1.toS70();
    int128_t s2 = a2.toS70();
    uint128_t u1 = a1.toU70();
    uint128_t u2 = a2.toU70();
    auto isNeg1 = s1 < 0;
    auto isNeg2 = s2 < 0;
    int128_t sum128 = s1 + s2;
    IResult result = iNormal;

    if (sum128 >= W72::sBit1) {
      flags.cy1 = flags.tr1 = flags.ov = 1;
      result = iTrap;
    } else if (sum128 < -W72::sBit1) {
      flags.cy0 = flags.tr1 = flags.ov = 1;
      result = iTrap;
    } else if ((s1 < 0 && s2 < 0) ||
	       (isNeg1 != isNeg2 &&
		(u1 == u2 || ((!isNeg1 && u1 > u2) || (!isNeg2 && u2 > u1)))))
      {
	flags.cy0 = flags.cy1 = flags.tr1 = flags.ov = 1;
	result = iTrap;
      }

    auto [hi36, lo36] = W72::toDW(sum128);
    acPutN(hi36, iw.ac+0);
    acPutN(lo36, iw.ac+1);
    return result;
  }


  IResult doDSUB() {
    auto a1 = W72{memGetN(ea.u+0), memGetN(ea.u+1)};
    auto a2 = W72{acGetN(iw.ac+0), acGetN(iw.ac+1)};

    int128_t s1 = a1.toS70();
    int128_t s2 = a2.toS70();
    uint128_t u1 = a1.toU70();
    uint128_t u2 = a2.toU70();
    auto isNeg1 = s1 < 0;
    auto isNeg2 = s2 < 0;
    int128_t diff128 = s1 - s2;
    IResult result = iNormal;

    if (diff128 >= W72::sBit1) {
      flags.cy1 = flags.tr1 = flags.ov = 1;
      result = iTrap;
    } else if (diff128 < -W72::sBit1) {
      flags.cy0 = flags.tr1 = flags.ov = 1;
      result = iTrap;
    } else if ((isNeg1 && isNeg2 && u2 >= u1) ||
	       (isNeg1 != isNeg2 && s2 < 0))
      {
	flags.cy0 = flags.cy1 = flags.tr1 = flags.ov = 1;
	result = iTrap;
      }

    auto [hi36, lo36] = W72::toDW(diff128);
    acPutN(hi36, iw.ac+0);
    acPutN(lo36, iw.ac+1);
    return result;
  }


  IResult doDMUL() {
    auto a = W72{memGetN(ea.u+0), memGetN(ea.u+1)};
    auto b = W72{acGetN(iw.ac+0), acGetN(iw.ac+1)};
    const uint128_t a70 = a.toU70();
    const uint128_t b70 = b.toU70();

    if (a.isMaxNeg() && b.isMaxNeg()) {
      const W36 big1{0400000,0};
      flags.tr1 = flags.ov = 1;
      acPutN(big1, iw.ac+0);
      acPutN(big1, iw.ac+1);
      acPutN(big1, iw.ac+2);
      acPutN(big1, iw.ac+3);
      return iTrap;
    }

    W144 prod{W144::product(a70, b70, (a.s < 0) ^ (b.s < 0))};
    auto [r0, r1, r2, r3] = prod.toQuadWord();
    acPutN(r0, iw.ac+0);
    acPutN(r1, iw.ac+1);
    acPutN(r2, iw.ac+2);
    acPutN(r3, iw.ac+3);
    return iNormal;
  }


  IResult doDDIV() {
    const W144 den{
      acGetN(iw.ac+0),
      acGetN(iw.ac+1),
      acGetN(iw.ac+2),
      acGetN(iw.ac+3)};
    const W72 div72{memGetN(ea.u+0), memGetN(ea.u+1)};
    auto const div = div72.toU70();

    if (den >= div) {
      flags.tr1 = flags.ov = flags.ndv = 1;
      return iTrap;
    }

    int denNeg = den.sign;
    int divNeg = div72.hiSign;

    /*
      Divide 192 bit n2||n1||n0 by d, returning remainder in rem.
      performs : (n2||n1||0) = ((n2||n1||n0) / d)
      d : a 128bit unsigned integer
    */
    const uint128_t lo70 = den.lowerU70();
    const uint128_t hi70 = den.upperU70();
    uint64_t n0 = lo70;
    uint64_t n1 = hi70 | (lo70 >> 64);
    uint64_t n2 = hi70 >> 6;

    uint128_t remainder = n2 % div;
    n2 = n2 / div;
    uint128_t partial = (remainder << 64) | n1;
    n1 = partial / div;
    remainder = partial % div;
    partial = (remainder << 64) | n0;
    n0 = partial / div;

    const auto quo72 = W72::fromMag(((uint128_t) n1 << 64) | n0, denNeg ^ divNeg);
    const auto rem72 = W72::fromMag(remainder, denNeg);

    acPutN(quo72.hi, iw.ac+0);
    acPutN(quo72.lo, iw.ac+1);
    acPutN(rem72.hi, iw.ac+2);
    acPutN(rem72.lo, iw.ac+3);
    return iNormal;
  }


  IResult doDMOVE() {
    acPutN(memGetN(ea.vma+0), iw.ac+0);
    acPutN(memGetN(ea.vma+1), iw.ac+1);
    return iNormal;
  }


  IResult doDMOVN() {
    W72 v{memGetN(ea.vma+0), memGetN(ea.vma+1)};
    v.loSign = 0;
    uint128_t v70 = v.toU70();

    acPutN(v.lo, iw.ac+0);
    acPutN(v.hi, iw.ac+1);

    if (v70 == W72::bit0) {
      flags.tr1 = flags.ov = flags.cy1 = 1;
      return iTrap;
    } else if (v70 == 0) {
      flags.cy0 = flags.cy1 = 1;
      return iNormal;
    } else {
      return iNormal;
    }
  }


  IResult doDMOVNM() {
    W72 v{acGetN(iw.ac+0), acGetN(iw.ac+1)};
    v.loSign = 0;
    uint128_t v70 = v.toU70();

    memPutN(v.lo, ea.vma+0);
    memPutN(v.hi, ea.vma+1);

    if (v70 == W72::bit0) {
      flags.tr1 = flags.ov = flags.cy1 = 1;
      return iTrap;
    } else if (v.u == 0) {
      flags.cy0 = flags.cy1 = 1;
      return iNormal;
    } else {
      return iNormal;
    }
  }


  IResult doDMOVEM() {
    memPutN(acGetN(iw.ac+0), ea.vma+0);
    memPutN(acGetN(iw.ac+1), ea.vma+1);
    return iNormal;
  }

};


void InstallDWordGroup(KM10 &km10) {
  km10.defOp(0114, "DADD",   static_cast<KM10::OpcodeHandler>(&DWordGroup::doDADD));
  km10.defOp(0115, "DSUB",   static_cast<KM10::OpcodeHandler>(&DWordGroup::doDSUB));
  km10.defOp(0116, "DMUL",   static_cast<KM10::OpcodeHandler>(&DWordGroup::doDMUL));
  km10.defOp(0117, "DDIV",   static_cast<KM10::OpcodeHandler>(&DWordGroup::doDDIV));
  km10.defOp(0120, "DMOVE",  static_cast<KM10::OpcodeHandler>(&DWordGroup::doDMOVE));
  km10.defOp(0121, "DMOVN",  static_cast<KM10::OpcodeHandler>(&DWordGroup::doDMOVN));
  km10.defOp(0124, "DMOVEM", static_cast<KM10::OpcodeHandler>(&DWordGroup::doDMOVEM));
  km10.defOp(0125, "DMOVNM", static_cast<KM10::OpcodeHandler>(&DWordGroup::doDMOVNM));
}
