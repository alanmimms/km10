#include "km10.hpp"


struct DWordGroup: KM10 {

  IResult doDADD() {
    W36 aHi{acGetN(iw.ac+0)};
    W36 aLo{acGetN(iw.ac+1)};
    W36 bHi{memGetN(ea.u+0)};
    W36 bLo{memGetN(ea.u+1)};

    int64_t lo64 = (int64_t) aLo.mag + (int64_t) bLo.mag;
    int64_t hi64 = (int64_t) aHi.u + (int64_t) bHi.u + (lo64 >> 35);
    IResult result = iNormal;

#if 0
    cout << "DADD " << iw.ac << "," << ea.fmt18() << logger.endl
	 << "     ac" << (iw.ac+0) << "=" << acGetN(iw.ac+0).fmt36()
	 << "     ac" << (iw.ac+1) << "=" << acGetN(iw.ac+1).fmt36() << logger.endl
	 << " c(ea+0)=" << memGetN(ea.u+0).fmt36()
	 << " c(ea+1)=" << memGetN(ea.u+1).fmt36() << logger.endl
	 << " aLo.mag=" << W36(aLo.mag).fmt36() << logger.endl
	 << " bLo.mag=" << W36(bLo.mag).fmt36() << logger.endl
	 << " low sum sign=" << (lo64 >> 35) << logger.endl
	 << " aHi=" << aHi.fmt36() << logger.endl
	 << " bHi=" << bHi.fmt36() << logger.endl
	 << " a=" << aHi.fmt36() << " " << aLo.fmt36() << logger.endl
	 << " b=" << bHi.fmt36() << " " << bLo.fmt36() << logger.endl
	 << " lo64=" << oct << setw(13) << setfill('0') << lo64 << logger.endl
	 << " hi64=" << oct << setw(13) << setfill('0') << hi64 << logger.endl;
#endif

    W36 rHi{hi64};
    W36 rLo{lo64};
    result = setADDFlags(aHi, bHi, rHi);
    rLo.sign = rHi.sign;
    acPutN(rHi, iw.ac+0);
    acPutN(rLo, iw.ac+1);
#if 0
    cout << " sum=" << rHi.fmt36() << " " << rLo.fmt36() << logger.endl;
    cout << " result=" << result << logger.endl;
#endif
    return result;
  }


  IResult doDSUB() {
    W36 aHi{acGetN(iw.ac+0)};
    W36 aLo{acGetN(iw.ac+1)};
    W36 bHi{memGetN(ea.u+0)};
    W36 bLo{memGetN(ea.u+1)};

    int64_t lo64 = (int64_t) aLo.mag - (int64_t) bLo.mag;
    int64_t hi64 = (int64_t) aHi.u - (int64_t) bHi.u - (lo64 < 0);
    IResult result = iNormal;

    W36 rHi{hi64};
    W36 rLo{lo64};
    result = setSUBFlags(aHi, bHi, rHi);
    rLo.sign = rHi.sign;
    acPutN(rHi, iw.ac+0);
    acPutN(rLo, iw.ac+1);
    return result;
  }


  void acPutQuad(W36::tQuadWord &q) {
    auto [r0, r1, r2, r3] = q;
    acPutN(r0, iw.ac+0);
    acPutN(r1, iw.ac+1);
    acPutN(r2, iw.ac+2);
    acPutN(r3, iw.ac+3);
  }


  IResult doDMUL() {
    auto a = W72{acGetN(iw.ac+0), acGetN(iw.ac+1)};
    auto b = W72{memGetN(ea.u+0), memGetN(ea.u+1)};

    cout << "DMUL"
	 << " a=" << W36(a.hi).fmt36()
	 << " " << W36(a.lo).fmt36()
	 << " b=" << W36(b.hi).fmt36()
	 << " " << W36(b.lo).fmt36()
	 << logger.endl;

    if (a.isMaxNeg() && b.isMaxNeg()) {
      const W36 big1 = W36::fromMag(0, 1);
      if (!flags.usr) flags.tr1 = flags.ov = 1;
      acPutN(big1, iw.ac+0);
      acPutN(big1, iw.ac+1);
      acPutN(big1, iw.ac+2);
      acPutN(big1, iw.ac+3);
      return flags.usr ? iTrap : iNormal;
    } else if ((a.lo35 == 0 && a.hi35 == 0) || (b.lo35 == 0 && b.hi35 ==0)) {
      acPutN(0, iw.ac+0);
      acPutN(0, iw.ac+1);
      acPutN(0, iw.ac+2);
      acPutN(0, iw.ac+3);
      return iNormal;
    } else {
      // Normalize signs, but remember...
      int aSign = a.hiSign;
      int bSign = b.hiSign;
      int sign = aSign ^ bSign;

      if (aSign) a = a.negate();
      if (bSign) b = b.negate();

      W144 prod = W144::product(a, b);
      cout << "DMUL aSign=" << aSign << " bSign=" << bSign << " sign=" << sign << logger.endl;
      cout << "     prod="
	   << W36(prod.u0).fmt36() << " "
	   << W36(prod.u1).fmt36() << " "
	   << W36(prod.u2).fmt36() << " "
	   << W36(prod.u3).fmt36() << logger.endl;

      prod.setSign(sign);
      acPutN(prod.u0, iw.ac+0);
      acPutN(prod.u1, iw.ac+1);
      acPutN(prod.u2, iw.ac+2);
      acPutN(prod.u3, iw.ac+3);
      return iNormal;
    }
  }


  IResult doDDIV() {
    const W144 den{
      acGetN(iw.ac+0),
      acGetN(iw.ac+1),
      acGetN(iw.ac+2),
      acGetN(iw.ac+3)};
    const W72 div72{memGetN(ea.u+0), memGetN(ea.u+1)};
    auto const div = div72.toMag();

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
    uint128_t v70 = v.toMag();

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
    uint128_t v70 = v.toMag();

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
