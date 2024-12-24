#include "km10.hpp"

struct HalfGroup: KM10 {
  inline W36 copyHRR(W36 src, W36 dst) {return W36(dst.lhu, src.rhu);};
  inline W36 copyHRL(W36 src, W36 dst) {return W36(src.rhu, dst.rhu);};
  inline W36 copyHLL(W36 src, W36 dst) {return W36(src.lhu, dst.rhu);};
  inline W36 copyHLR(W36 src, W36 dst) {return W36(dst.lhu, src.lhu);};

  inline W36 extnOf(W36 v) { return (v & 0400'000) ? W36::halfOnes : 0u; }

  inline W36 zeroR(W36 v) {return W36(v.lhu, 0);};
  inline W36 onesR(W36 v) {return W36(v.lhu, W36::halfOnes);};
  inline W36 extnR(W36 v) {return W36(extnOf(v.rhu), v.rhu);};
  inline W36 zeroL(W36 v) {return W36(0, v.rhu);};
  inline W36 onesL(W36 v) {return W36(W36::halfOnes, v.rhu);};
  inline W36 extnL(W36 v) {return W36(v.lhu, extnOf(v.lhu));};

  void selfPut(W36 v) {
    memPut(v);
    if (iw.ac != 0) acPut(v);
  }


  IResult doHLL()   {   acPut(      copyHLL(   memGet(),  acGet()));  return iNormal; }
  IResult doHLLI()  {   acPut(      copyHLL(immediate(),  acGet()));  return iNormal; }
  IResult doHLLM()  {  memPut(      copyHLL(    acGet(), memGet()));  return iNormal; }
  IResult doHLLS()  { selfPut(      copyHLL(   memGet(), memGet()));  return iNormal; }
  IResult doHRL()   {   acPut(      copyHRL(   memGet(),  acGet()));  return iNormal; }
  IResult doHRLI()  {   acPut(      copyHRL(immediate(),  acGet()));  return iNormal; }
  IResult doHRLM()  {  memPut(      copyHRL(    acGet(), memGet()));  return iNormal; }
  IResult doHRLS()  { selfPut(      copyHRL(   memGet(), memGet()));  return iNormal; }
  IResult doHLLZ()  {   acPut(zeroR(copyHLL(   memGet(),  acGet()))); return iNormal; }
  IResult doHLLZI() {   acPut(zeroR(copyHLL(immediate(),  acGet()))); return iNormal; }
  IResult doHLLZM() {  memPut(zeroR(copyHLL(    acGet(), memGet()))); return iNormal; }
  IResult doHLLZS() { selfPut(zeroR(copyHLL(   memGet(), memGet()))); return iNormal; }
  IResult doHRLZ()  {   acPut(zeroR(copyHRL(   memGet(),  acGet()))); return iNormal; }
  IResult doHRLZI() {   acPut(zeroR(copyHRL(immediate(),  acGet()))); return iNormal; }
  IResult doHRLZM() {  memPut(zeroR(copyHRL(    acGet(), memGet()))); return iNormal; }
  IResult doHRLZS() { selfPut(zeroR(copyHRL(   memGet(), memGet()))); return iNormal; }
  IResult doHLLO()  {   acPut(onesR(copyHLL(   memGet(),  acGet()))); return iNormal; }
  IResult doHLLOI() {   acPut(onesR(copyHLL(immediate(),  acGet()))); return iNormal; }
  IResult doHLLOM() {  memPut(onesR(copyHLL(    acGet(), memGet()))); return iNormal; }
  IResult doHLLOS() { selfPut(onesR(copyHLL(   memGet(), memGet()))); return iNormal; }
  IResult doHRLO()  {   acPut(onesR(copyHRL(   memGet(),  acGet()))); return iNormal; }
  IResult doHRLOI() {   acPut(onesR(copyHRL(immediate(),  acGet()))); return iNormal; }
  IResult doHRLOM() {  memPut(onesR(copyHRL(    acGet(), memGet()))); return iNormal; }
  IResult doHRLOS() { selfPut(onesR(copyHRL(   memGet(), memGet()))); return iNormal; }
  IResult doHLLE()  {   acPut(extnL(copyHLL(   memGet(),  acGet()))); return iNormal; }
  IResult doHLLEI() {   acPut(extnL(copyHLL(immediate(),  acGet()))); return iNormal; }
  IResult doHLLEM() {  memPut(extnL(copyHLL(    acGet(), memGet()))); return iNormal; }
  IResult doHLLES() { selfPut(extnL(copyHLL(   memGet(), memGet()))); return iNormal; }
  IResult doHRLE()  {   acPut(extnL(copyHRL(   memGet(),  acGet()))); return iNormal; }
  IResult doHRLEI() {   acPut(extnL(copyHRL(immediate(),  acGet()))); return iNormal; }
  IResult doHRLEM() {  memPut(extnL(copyHRL(    acGet(), memGet()))); return iNormal; }
  IResult doHRLES() { selfPut(extnL(copyHRL(   memGet(), memGet()))); return iNormal; }
  IResult doHRR()   {   acPut(      copyHRR(   memGet(),  acGet()));  return iNormal; }
  IResult doHRRI()  {   acPut(      copyHRR(immediate(),  acGet()));  return iNormal; }
  IResult doHRRM()  {  memPut(      copyHRR(    acGet(), memGet()));  return iNormal; }
  IResult doHRRS()  { selfPut(      copyHRR(   memGet(), memGet()));  return iNormal; }
  IResult doHLR()   {   acPut(      copyHLR(   memGet(),  acGet()));  return iNormal; }
  IResult doHLRI()  {   acPut(      copyHLR(immediate(),  acGet()));  return iNormal; }
  IResult doHLRM()  {  memPut(      copyHLR(    acGet(), memGet()));  return iNormal; }
  IResult doHLRS()  { selfPut(      copyHLR(   memGet(), memGet()));  return iNormal; }
  IResult doHRRZ()  {   acPut(zeroL(copyHRR(   memGet(),  acGet()))); return iNormal; }
  IResult doHRRZI() {   acPut(zeroL(copyHRR(immediate(),  acGet()))); return iNormal; }
  IResult doHRRZM() {  memPut(zeroL(copyHRR(    acGet(), memGet()))); return iNormal; }
  IResult doHRRZS() { selfPut(zeroL(copyHRR(   memGet(), memGet()))); return iNormal; }
  IResult doHLRZ()  {   acPut(zeroL(copyHLR(   memGet(),  acGet()))); return iNormal; }
  IResult doHLRZI() {   acPut(zeroL(copyHLR(immediate(),  acGet()))); return iNormal; }
  IResult doHLRZM() {  memPut(zeroL(copyHLR(    acGet(), memGet()))); return iNormal; }
  IResult doHLRZS() { selfPut(zeroL(copyHLR(   memGet(), memGet()))); return iNormal; }
  IResult doHRRO()  {   acPut(onesL(copyHRR(   memGet(),  acGet()))); return iNormal; }
  IResult doHRROI() {   acPut(onesL(copyHRR(immediate(),  acGet()))); return iNormal; }
  IResult doHRROM() {  memPut(onesL(copyHRR(    acGet(), memGet()))); return iNormal; }
  IResult doHRROS() { selfPut(onesL(copyHRR(   memGet(), memGet()))); return iNormal; }
  IResult doHLRO()  {   acPut(onesL(copyHLR(   memGet(),  acGet()))); return iNormal; }
  IResult doHLROI() {   acPut(onesL(copyHLR(immediate(),  acGet()))); return iNormal; }
  IResult doHLROM() {  memPut(onesL(copyHLR(    acGet(), memGet()))); return iNormal; }
  IResult doHLROS() { selfPut(onesL(copyHLR(   memGet(), memGet()))); return iNormal; }
  IResult doHRRE()  {   acPut(extnR(copyHRR(   memGet(),  acGet()))); return iNormal; }
  IResult doHRREI() {   acPut(extnR(copyHRR(immediate(),  acGet()))); return iNormal; }
  IResult doHRREM() {  memPut(extnR(copyHRR(    acGet(), memGet()))); return iNormal; }
  IResult doHRRES() { selfPut(extnR(copyHRR(   memGet(), memGet()))); return iNormal; }
  IResult doHLRE()  {   acPut(extnR(copyHLR(   memGet(),  acGet()))); return iNormal; }
  IResult doHLREI() {   acPut(extnR(copyHLR(immediate(),  acGet()))); return iNormal; }
  IResult doHLREM() {  memPut(extnR(copyHLR(    acGet(), memGet()))); return iNormal; }
  IResult doHLRES() { selfPut(extnR(copyHLR(   memGet(), memGet()))); return iNormal; }
};


void InstallHalfGroup(KM10 &km10) {
  km10.defOp(0500, "HLL",   static_cast<KM10::OpcodeHandler>(&HalfGroup::doHLL));
  km10.defOp(0501, "HLLI",  static_cast<KM10::OpcodeHandler>(&HalfGroup::doHLLI));
  km10.defOp(0502, "HLLM",  static_cast<KM10::OpcodeHandler>(&HalfGroup::doHLLM));
  km10.defOp(0503, "HLLS",  static_cast<KM10::OpcodeHandler>(&HalfGroup::doHLLS));
  km10.defOp(0504, "HRL",   static_cast<KM10::OpcodeHandler>(&HalfGroup::doHRL));
  km10.defOp(0505, "HRLI",  static_cast<KM10::OpcodeHandler>(&HalfGroup::doHRLI));
  km10.defOp(0506, "HRLM",  static_cast<KM10::OpcodeHandler>(&HalfGroup::doHRLM));
  km10.defOp(0507, "HRLS",  static_cast<KM10::OpcodeHandler>(&HalfGroup::doHRLS));
  km10.defOp(0510, "HLLZ",  static_cast<KM10::OpcodeHandler>(&HalfGroup::doHLLZ));
  km10.defOp(0511, "HLLZI", static_cast<KM10::OpcodeHandler>(&HalfGroup::doHLLZI));
  km10.defOp(0512, "HLLZM", static_cast<KM10::OpcodeHandler>(&HalfGroup::doHLLZM));
  km10.defOp(0513, "HLLZS", static_cast<KM10::OpcodeHandler>(&HalfGroup::doHLLZS));
  km10.defOp(0514, "HRLZ",  static_cast<KM10::OpcodeHandler>(&HalfGroup::doHRLZ));
  km10.defOp(0515, "HRLZI", static_cast<KM10::OpcodeHandler>(&HalfGroup::doHRLZI));
  km10.defOp(0516, "HRLZM", static_cast<KM10::OpcodeHandler>(&HalfGroup::doHRLZM));
  km10.defOp(0517, "HRLZS", static_cast<KM10::OpcodeHandler>(&HalfGroup::doHRLZS));
  km10.defOp(0520, "HLLO",  static_cast<KM10::OpcodeHandler>(&HalfGroup::doHLLO));
  km10.defOp(0521, "HLLOI", static_cast<KM10::OpcodeHandler>(&HalfGroup::doHLLOI));
  km10.defOp(0522, "HLLOM", static_cast<KM10::OpcodeHandler>(&HalfGroup::doHLLOM));
  km10.defOp(0523, "HLLOS", static_cast<KM10::OpcodeHandler>(&HalfGroup::doHLLOS));
  km10.defOp(0524, "HRLO",  static_cast<KM10::OpcodeHandler>(&HalfGroup::doHRLO));
  km10.defOp(0525, "HRLOI", static_cast<KM10::OpcodeHandler>(&HalfGroup::doHRLOI));
  km10.defOp(0526, "HRLOM", static_cast<KM10::OpcodeHandler>(&HalfGroup::doHRLOM));
  km10.defOp(0527, "HRLOS", static_cast<KM10::OpcodeHandler>(&HalfGroup::doHRLOS));
  km10.defOp(0530, "HLLE",  static_cast<KM10::OpcodeHandler>(&HalfGroup::doHLLE));
  km10.defOp(0531, "HLLEI", static_cast<KM10::OpcodeHandler>(&HalfGroup::doHLLEI));
  km10.defOp(0532, "HLLEM", static_cast<KM10::OpcodeHandler>(&HalfGroup::doHLLEM));
  km10.defOp(0533, "HLLES", static_cast<KM10::OpcodeHandler>(&HalfGroup::doHLLES));
  km10.defOp(0534, "HRLE",  static_cast<KM10::OpcodeHandler>(&HalfGroup::doHRLE));
  km10.defOp(0535, "HRLEI", static_cast<KM10::OpcodeHandler>(&HalfGroup::doHRLEI));
  km10.defOp(0536, "HRLEM", static_cast<KM10::OpcodeHandler>(&HalfGroup::doHRLEM));
  km10.defOp(0537, "HRLES", static_cast<KM10::OpcodeHandler>(&HalfGroup::doHRLES));
  km10.defOp(0540, "HRR",   static_cast<KM10::OpcodeHandler>(&HalfGroup::doHRR));
  km10.defOp(0541, "HRRI",  static_cast<KM10::OpcodeHandler>(&HalfGroup::doHRRI));
  km10.defOp(0542, "HRRM",  static_cast<KM10::OpcodeHandler>(&HalfGroup::doHRRM));
  km10.defOp(0543, "HRRS",  static_cast<KM10::OpcodeHandler>(&HalfGroup::doHRRS));
  km10.defOp(0544, "HLR",   static_cast<KM10::OpcodeHandler>(&HalfGroup::doHLR));
  km10.defOp(0545, "HLRI",  static_cast<KM10::OpcodeHandler>(&HalfGroup::doHLRI));
  km10.defOp(0546, "HLRM",  static_cast<KM10::OpcodeHandler>(&HalfGroup::doHLRM));
  km10.defOp(0547, "HLRS",  static_cast<KM10::OpcodeHandler>(&HalfGroup::doHLRS));
  km10.defOp(0550, "HRRZ",  static_cast<KM10::OpcodeHandler>(&HalfGroup::doHRRZ));
  km10.defOp(0551, "HRRZI", static_cast<KM10::OpcodeHandler>(&HalfGroup::doHRRZI));
  km10.defOp(0552, "HRRZM", static_cast<KM10::OpcodeHandler>(&HalfGroup::doHRRZM));
  km10.defOp(0553, "HRRZS", static_cast<KM10::OpcodeHandler>(&HalfGroup::doHRRZS));
  km10.defOp(0554, "HLRZ",  static_cast<KM10::OpcodeHandler>(&HalfGroup::doHLRZ));
  km10.defOp(0555, "HLRZI", static_cast<KM10::OpcodeHandler>(&HalfGroup::doHLRZI));
  km10.defOp(0556, "HLRZM", static_cast<KM10::OpcodeHandler>(&HalfGroup::doHLRZM));
  km10.defOp(0557, "HLRZS", static_cast<KM10::OpcodeHandler>(&HalfGroup::doHLRZS));
  km10.defOp(0560, "HRRO",  static_cast<KM10::OpcodeHandler>(&HalfGroup::doHRRO));
  km10.defOp(0561, "HRROI", static_cast<KM10::OpcodeHandler>(&HalfGroup::doHRROI));
  km10.defOp(0562, "HRROM", static_cast<KM10::OpcodeHandler>(&HalfGroup::doHRROM));
  km10.defOp(0563, "HRROS", static_cast<KM10::OpcodeHandler>(&HalfGroup::doHRROS));
  km10.defOp(0564, "HLRO",  static_cast<KM10::OpcodeHandler>(&HalfGroup::doHLRO));
  km10.defOp(0565, "HLROI", static_cast<KM10::OpcodeHandler>(&HalfGroup::doHLROI));
  km10.defOp(0566, "HLROM", static_cast<KM10::OpcodeHandler>(&HalfGroup::doHLROM));
  km10.defOp(0567, "HLROS", static_cast<KM10::OpcodeHandler>(&HalfGroup::doHLROS));
  km10.defOp(0570, "HRRE",  static_cast<KM10::OpcodeHandler>(&HalfGroup::doHRRE));
  km10.defOp(0571, "HRREI", static_cast<KM10::OpcodeHandler>(&HalfGroup::doHRREI));
  km10.defOp(0572, "HRREM", static_cast<KM10::OpcodeHandler>(&HalfGroup::doHRREM));
  km10.defOp(0573, "HRRES", static_cast<KM10::OpcodeHandler>(&HalfGroup::doHRRES));
  km10.defOp(0574, "HLRE",  static_cast<KM10::OpcodeHandler>(&HalfGroup::doHLRE));
  km10.defOp(0575, "HLREI", static_cast<KM10::OpcodeHandler>(&HalfGroup::doHLREI));
  km10.defOp(0576, "HLREM", static_cast<KM10::OpcodeHandler>(&HalfGroup::doHLREM));
  km10.defOp(0577, "HLRES", static_cast<KM10::OpcodeHandler>(&HalfGroup::doHLRES));
}
