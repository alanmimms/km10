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
