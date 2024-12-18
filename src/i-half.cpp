#include "km10.hpp"

struct HalfGroup: KM10 {
  InstructionResult doHLL()   {   acPut(      copyHLL(   memGet(),  acGet()));  return iNormal; }
  InstructionResult doHLLI()  {   acPut(      copyHLL(immediate(),  acGet()));  return iNormal; }
  InstructionResult doHLLM()  {  memPut(      copyHLL(    acGet(), memGet()));  return iNormal; }
  InstructionResult doHLLS()  { selfPut(      copyHLL(   memGet(), memGet()));  return iNormal; }
  InstructionResult doHRL()   {   acPut(      copyHRL(   memGet(),  acGet()));  return iNormal; }
  InstructionResult doHRLI()  {   acPut(      copyHRL(immediate(),  acGet()));  return iNormal; }
  InstructionResult doHRLM()  {  memPut(      copyHRL(    acGet(), memGet()));  return iNormal; }
  InstructionResult doHRLS()  { selfPut(      copyHRL(   memGet(), memGet()));  return iNormal; }
  InstructionResult doHLLZ()  {   acPut(zeroR(copyHLL(   memGet(),  acGet()))); return iNormal; }
  InstructionResult doHLLZI() {   acPut(zeroR(copyHLL(immediate(),  acGet()))); return iNormal; }
  InstructionResult doHLLZM() {  memPut(zeroR(copyHLL(    acGet(), memGet()))); return iNormal; }
  InstructionResult doHLLZS() { selfPut(zeroR(copyHLL(   memGet(), memGet()))); return iNormal; }
  InstructionResult doHRLZ()  {   acPut(zeroR(copyHRL(   memGet(),  acGet()))); return iNormal; }
  InstructionResult doHRLZI() {   acPut(zeroR(copyHRL(immediate(),  acGet()))); return iNormal; }
  InstructionResult doHRLZM() {  memPut(zeroR(copyHRL(    acGet(), memGet()))); return iNormal; }
  InstructionResult doHRLZS() { selfPut(zeroR(copyHRL(   memGet(), memGet()))); return iNormal; }
  InstructionResult doHLLO()  {   acPut(onesR(copyHLL(   memGet(),  acGet()))); return iNormal; }
  InstructionResult doHLLOI() {   acPut(onesR(copyHLL(immediate(),  acGet()))); return iNormal; }
  InstructionResult doHLLOM() {  memPut(onesR(copyHLL(    acGet(), memGet()))); return iNormal; }
  InstructionResult doHLLOS() { selfPut(onesR(copyHLL(   memGet(), memGet()))); return iNormal; }
  InstructionResult doHRLO()  {   acPut(onesR(copyHRL(   memGet(),  acGet()))); return iNormal; }
  InstructionResult doHRLOI() {   acPut(onesR(copyHRL(immediate(),  acGet()))); return iNormal; }
  InstructionResult doHRLOM() {  memPut(onesR(copyHRL(    acGet(), memGet()))); return iNormal; }
  InstructionResult doHRLOS() { selfPut(onesR(copyHRL(   memGet(), memGet()))); return iNormal; }
  InstructionResult doHLLE()  {   acPut(extnL(copyHLL(   memGet(),  acGet()))); return iNormal; }
  InstructionResult doHLLEI() {   acPut(extnL(copyHLL(immediate(),  acGet()))); return iNormal; }
  InstructionResult doHLLEM() {  memPut(extnL(copyHLL(    acGet(), memGet()))); return iNormal; }
  InstructionResult doHLLES() { selfPut(extnL(copyHLL(   memGet(), memGet()))); return iNormal; }
  InstructionResult doHRLE()  {   acPut(extnL(copyHRL(   memGet(),  acGet()))); return iNormal; }
  InstructionResult doHRLEI() {   acPut(extnL(copyHRL(immediate(),  acGet()))); return iNormal; }
  InstructionResult doHRLEM() {  memPut(extnL(copyHRL(    acGet(), memGet()))); return iNormal; }
  InstructionResult doHRLES() { selfPut(extnL(copyHRL(   memGet(), memGet()))); return iNormal; }
  InstructionResult doHRR()   {   acPut(      copyHRR(   memGet(),  acGet()));  return iNormal; }
  InstructionResult doHRRI()  {   acPut(      copyHRR(immediate(),  acGet()));  return iNormal; }
  InstructionResult doHRRM()  {  memPut(      copyHRR(    acGet(), memGet()));  return iNormal; }
  InstructionResult doHRRS()  { selfPut(      copyHRR(   memGet(), memGet()));  return iNormal; }
  InstructionResult doHLR()   {   acPut(      copyHLR(   memGet(),  acGet()));  return iNormal; }
  InstructionResult doHLRI()  {   acPut(      copyHLR(immediate(),  acGet()));  return iNormal; }
  InstructionResult doHLRM()  {  memPut(      copyHLR(    acGet(), memGet()));  return iNormal; }
  InstructionResult doHLRS()  { selfPut(      copyHLR(   memGet(), memGet()));  return iNormal; }
  InstructionResult doHRRZ()  {   acPut(zeroL(copyHRR(   memGet(),  acGet()))); return iNormal; }
  InstructionResult doHRRZI() {   acPut(zeroL(copyHRR(immediate(),  acGet()))); return iNormal; }
  InstructionResult doHRRZM() {  memPut(zeroL(copyHRR(    acGet(), memGet()))); return iNormal; }
  InstructionResult doHRRZS() { selfPut(zeroL(copyHRR(   memGet(), memGet()))); return iNormal; }
  InstructionResult doHLRZ()  {   acPut(zeroL(copyHLR(   memGet(),  acGet()))); return iNormal; }
  InstructionResult doHLRZI() {   acPut(zeroL(copyHLR(immediate(),  acGet()))); return iNormal; }
  InstructionResult doHLRZM() {  memPut(zeroL(copyHLR(    acGet(), memGet()))); return iNormal; }
  InstructionResult doHLRZS() { selfPut(zeroL(copyHLR(   memGet(), memGet()))); return iNormal; }
  InstructionResult doHRRO()  {   acPut(onesL(copyHRR(   memGet(),  acGet()))); return iNormal; }
  InstructionResult doHRROI() {   acPut(onesL(copyHRR(immediate(),  acGet()))); return iNormal; }
  InstructionResult doHRROM() {  memPut(onesL(copyHRR(    acGet(), memGet()))); return iNormal; }
  InstructionResult doHRROS() { selfPut(onesL(copyHRR(   memGet(), memGet()))); return iNormal; }
  InstructionResult doHLRO()  {   acPut(onesL(copyHLR(   memGet(),  acGet()))); return iNormal; }
  InstructionResult doHLROI() {   acPut(onesL(copyHLR(immediate(),  acGet()))); return iNormal; }
  InstructionResult doHLROM() {  memPut(onesL(copyHLR(    acGet(), memGet()))); return iNormal; }
  InstructionResult doHLROS() { selfPut(onesL(copyHLR(   memGet(), memGet()))); return iNormal; }
  InstructionResult doHRRE()  {   acPut(extnR(copyHRR(   memGet(),  acGet()))); return iNormal; }
  InstructionResult doHRREI() {   acPut(extnR(copyHRR(immediate(),  acGet()))); return iNormal; }
  InstructionResult doHRREM() {  memPut(extnR(copyHRR(    acGet(), memGet()))); return iNormal; }
  InstructionResult doHRRES() { selfPut(extnR(copyHRR(   memGet(), memGet()))); return iNormal; }
  InstructionResult doHLRE()  {   acPut(extnR(copyHLR(   memGet(),  acGet()))); return iNormal; }
  InstructionResult doHLREI() {   acPut(extnR(copyHLR(immediate(),  acGet()))); return iNormal; }
  InstructionResult doHLREM() {  memPut(extnR(copyHLR(    acGet(), memGet()))); return iNormal; }
  InstructionResult doHLRES() { selfPut(extnR(copyHLR(   memGet(), memGet()))); return iNormal; }

  inline void install() {
    defOp(0500, "HLL",   static_cast<OpcodeHandler>(&HalfGroup::doHLL));
    defOp(0501, "HLLI",  static_cast<OpcodeHandler>(&HalfGroup::doHLLI));
    defOp(0502, "HLLM",  static_cast<OpcodeHandler>(&HalfGroup::doHLLM));
    defOp(0503, "HLLS",  static_cast<OpcodeHandler>(&HalfGroup::doHLLS));
    defOp(0504, "HRL",   static_cast<OpcodeHandler>(&HalfGroup::doHRL));
    defOp(0505, "HRLI",  static_cast<OpcodeHandler>(&HalfGroup::doHRLI));
    defOp(0506, "HRLM",  static_cast<OpcodeHandler>(&HalfGroup::doHRLM));
    defOp(0507, "HRLS",  static_cast<OpcodeHandler>(&HalfGroup::doHRLS));
    defOp(0510, "HLLZ",  static_cast<OpcodeHandler>(&HalfGroup::doHLLZ));
    defOp(0511, "HLLZI", static_cast<OpcodeHandler>(&HalfGroup::doHLLZI));
    defOp(0512, "HLLZM", static_cast<OpcodeHandler>(&HalfGroup::doHLLZM));
    defOp(0513, "HLLZS", static_cast<OpcodeHandler>(&HalfGroup::doHLLZS));
    defOp(0514, "HRLZ",  static_cast<OpcodeHandler>(&HalfGroup::doHRLZ));
    defOp(0515, "HRLZI", static_cast<OpcodeHandler>(&HalfGroup::doHRLZI));
    defOp(0516, "HRLZM", static_cast<OpcodeHandler>(&HalfGroup::doHRLZM));
    defOp(0517, "HRLZS", static_cast<OpcodeHandler>(&HalfGroup::doHRLZS));
    defOp(0520, "HLLO",  static_cast<OpcodeHandler>(&HalfGroup::doHLLO));
    defOp(0521, "HLLOI", static_cast<OpcodeHandler>(&HalfGroup::doHLLOI));
    defOp(0522, "HLLOM", static_cast<OpcodeHandler>(&HalfGroup::doHLLOM));
    defOp(0523, "HLLOS", static_cast<OpcodeHandler>(&HalfGroup::doHLLOS));
    defOp(0524, "HRLO",  static_cast<OpcodeHandler>(&HalfGroup::doHRLO));
    defOp(0525, "HRLOI", static_cast<OpcodeHandler>(&HalfGroup::doHRLOI));
    defOp(0526, "HRLOM", static_cast<OpcodeHandler>(&HalfGroup::doHRLOM));
    defOp(0527, "HRLOS", static_cast<OpcodeHandler>(&HalfGroup::doHRLOS));
    defOp(0530, "HLLE",  static_cast<OpcodeHandler>(&HalfGroup::doHLLE));
    defOp(0531, "HLLEI", static_cast<OpcodeHandler>(&HalfGroup::doHLLEI));
    defOp(0532, "HLLEM", static_cast<OpcodeHandler>(&HalfGroup::doHLLEM));
    defOp(0533, "HLLES", static_cast<OpcodeHandler>(&HalfGroup::doHLLES));
    defOp(0534, "HRLE",  static_cast<OpcodeHandler>(&HalfGroup::doHRLE));
    defOp(0535, "HRLEI", static_cast<OpcodeHandler>(&HalfGroup::doHRLEI));
    defOp(0536, "HRLEM", static_cast<OpcodeHandler>(&HalfGroup::doHRLEM));
    defOp(0537, "HRLES", static_cast<OpcodeHandler>(&HalfGroup::doHRLES));
    defOp(0540, "HRR",   static_cast<OpcodeHandler>(&HalfGroup::doHRR));
    defOp(0541, "HRRI",  static_cast<OpcodeHandler>(&HalfGroup::doHRRI));
    defOp(0542, "HRRM",  static_cast<OpcodeHandler>(&HalfGroup::doHRRM));
    defOp(0543, "HRRS",  static_cast<OpcodeHandler>(&HalfGroup::doHRRS));
    defOp(0544, "HLR",   static_cast<OpcodeHandler>(&HalfGroup::doHLR));
    defOp(0545, "HLRI",  static_cast<OpcodeHandler>(&HalfGroup::doHLRI));
    defOp(0546, "HLRM",  static_cast<OpcodeHandler>(&HalfGroup::doHLRM));
    defOp(0547, "HLRS",  static_cast<OpcodeHandler>(&HalfGroup::doHLRS));
    defOp(0550, "HRRZ",  static_cast<OpcodeHandler>(&HalfGroup::doHRRZ));
    defOp(0551, "HRRZI", static_cast<OpcodeHandler>(&HalfGroup::doHRRZI));
    defOp(0552, "HRRZM", static_cast<OpcodeHandler>(&HalfGroup::doHRRZM));
    defOp(0553, "HRRZS", static_cast<OpcodeHandler>(&HalfGroup::doHRRZS));
    defOp(0554, "HLRZ",  static_cast<OpcodeHandler>(&HalfGroup::doHLRZ));
    defOp(0555, "HLRZI", static_cast<OpcodeHandler>(&HalfGroup::doHLRZI));
    defOp(0556, "HLRZM", static_cast<OpcodeHandler>(&HalfGroup::doHLRZM));
    defOp(0557, "HLRZS", static_cast<OpcodeHandler>(&HalfGroup::doHLRZS));
    defOp(0560, "HRRO",  static_cast<OpcodeHandler>(&HalfGroup::doHRRO));
    defOp(0561, "HRROI", static_cast<OpcodeHandler>(&HalfGroup::doHRROI));
    defOp(0562, "HRROM", static_cast<OpcodeHandler>(&HalfGroup::doHRROM));
    defOp(0563, "HRROS", static_cast<OpcodeHandler>(&HalfGroup::doHRROS));
    defOp(0564, "HLRO",  static_cast<OpcodeHandler>(&HalfGroup::doHLRO));
    defOp(0565, "HLROI", static_cast<OpcodeHandler>(&HalfGroup::doHLROI));
    defOp(0566, "HLROM", static_cast<OpcodeHandler>(&HalfGroup::doHLROM));
    defOp(0567, "HLROS", static_cast<OpcodeHandler>(&HalfGroup::doHLROS));
    defOp(0570, "HRRE",  static_cast<OpcodeHandler>(&HalfGroup::doHRRE));
    defOp(0571, "HRREI", static_cast<OpcodeHandler>(&HalfGroup::doHRREI));
    defOp(0572, "HRREM", static_cast<OpcodeHandler>(&HalfGroup::doHRREM));
    defOp(0573, "HRRES", static_cast<OpcodeHandler>(&HalfGroup::doHRRES));
    defOp(0574, "HLRE",  static_cast<OpcodeHandler>(&HalfGroup::doHLRE));
    defOp(0575, "HLREI", static_cast<OpcodeHandler>(&HalfGroup::doHLREI));
    defOp(0576, "HLREM", static_cast<OpcodeHandler>(&HalfGroup::doHLREM));
    defOp(0577, "HLRES", static_cast<OpcodeHandler>(&HalfGroup::doHLRES));
  }
};
