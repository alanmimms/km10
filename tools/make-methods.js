'use strict'

const ops = `
-  defOp(0500, "HLL",   [this]() { doHXXXX(memGet,    acGet,  copyHLL, noMod1, acPut);  return iNormal; });
-  defOp(0501, "HLLI",  [this]() { doHXXXX(immediate, acGet,  copyHLL, noMod1, acPut);  return iNormal; });
-  defOp(0502, "HLLM",  [this]() { doHXXXX(acGet,     memGet, copyHLL, noMod1, memPut); return iNormal; });
-  defOp(0503, "HLLS",  [this]() { doHXXXX(memGet,    memGet, copyHLL, noMod1, selfPut);        return iNormal; });
-  defOp(0504, "HRL",   [this]() { doHXXXX(memGet,    acGet,  copyHRL, noMod1, acPut);  return iNormal; });
-  defOp(0505, "HRLI",  [this]() { doHXXXX(immediate, acGet,  copyHRL, noMod1, acPut);  return iNormal; });
-  defOp(0506, "HRLM",  [this]() { doHXXXX(acGet,     memGet, copyHRL, noMod1, memPut); return iNormal; });
-  defOp(0507, "HRLS",  [this]() { doHXXXX(memGet,    memGet, copyHRL, noMod1, selfPut);        return iNormal; });
-  defOp(0510, "HLLZ",  [this]() { doHXXXX(memGet,    acGet,  copyHLL, zeroR,  acPut);  return iNormal; });
-  defOp(0511, "HLLZI", [this]() { doHXXXX(immediate, acGet,  copyHLL, zeroR,  acPut);  return iNormal; });
-  defOp(0512, "HLLZM", [this]() { doHXXXX(acGet,     memGet, copyHLL, zeroR,  memPut); return iNormal; });
-  defOp(0513, "HLLZS", [this]() { doHXXXX(memGet,    memGet, copyHLL, zeroR,  selfPut);        return iNormal; });
-  defOp(0514, "HRLZ",  [this]() { doHXXXX(memGet,    acGet,  copyHRL, zeroR,  acPut);  return iNormal; });
-  defOp(0515, "HRLZI", [this]() { doHXXXX(immediate, acGet,  copyHRL, zeroR,  acPut);  return iNormal; });
-  defOp(0516, "HRLZM", [this]() { doHXXXX(acGet,     memGet, copyHRL, zeroR,  memPut); return iNormal; });
-  defOp(0517, "HRLZS", [this]() { doHXXXX(memGet,    memGet, copyHRL, zeroR,  selfPut);        return iNormal; });
-  defOp(0520, "HLLO",  [this]() { doHXXXX(memGet,    acGet,  copyHLL, onesR,  acPut);  return iNormal; });
-  defOp(0521, "HLLOI", [this]() { doHXXXX(immediate, acGet,  copyHLL, onesR,  acPut);  return iNormal; });
-  defOp(0522, "HLLOM", [this]() { doHXXXX(acGet,     memGet, copyHLL, onesR,  memPut); return iNormal; });
-  defOp(0523, "HLLOS", [this]() { doHXXXX(memGet,    memGet, copyHLL, onesR,  selfPut);        return iNormal; });
-  defOp(0524, "HRLO",  [this]() { doHXXXX(memGet,    acGet,  copyHRL, onesR,  acPut);  return iNormal; });
-  defOp(0525, "HRLOI", [this]() { doHXXXX(immediate, acGet,  copyHRL, onesR,  acPut);  return iNormal; });
-  defOp(0526, "HRLOM", [this]() { doHXXXX(acGet,     memGet, copyHRL, onesR,  memPut); return iNormal; });
-  defOp(0527, "HRLOS", [this]() { doHXXXX(memGet,    memGet, copyHRL, onesR,  selfPut);        return iNormal; });
-  defOp(0530, "HLLE",  [this]() { doHXXXX(memGet,    acGet,  copyHLL, extnL,  acPut);  return iNormal; });
-  defOp(0531, "HLLEI", [this]() { doHXXXX(immediate, acGet,  copyHLL, extnL,  acPut);  return iNormal; });
-  defOp(0532, "HLLEM", [this]() { doHXXXX(acGet,     memGet, copyHLL, extnL,  memPut); return iNormal; });
-  defOp(0533, "HLLES", [this]() { doHXXXX(memGet,    memGet, copyHLL, extnL,  selfPut);        return iNormal; });
-  defOp(0534, "HRLE",  [this]() { doHXXXX(memGet,    acGet,  copyHRL, extnL,  acPut);  return iNormal; });
-  defOp(0535, "HRLEI", [this]() { doHXXXX(immediate, acGet,  copyHRL, extnL,  acPut);  return iNormal; });
-  defOp(0536, "HRLEM", [this]() { doHXXXX(acGet,     memGet, copyHRL, extnL,  memPut); return iNormal; });
-  defOp(0537, "HRLES", [this]() { doHXXXX(memGet,    memGet, copyHRL, extnL,  selfPut);        return iNormal; });
-  defOp(0540, "HRR",   [this]() { doHXXXX(memGet,    acGet,  copyHRR, noMod1, acPut);  return iNormal; });
-  defOp(0541, "HRRI",  [this]() { doHXXXX(immediate, acGet,  copyHRR, noMod1, acPut);  return iNormal; });
-  defOp(0542, "HRRM",  [this]() { doHXXXX(acGet,     memGet, copyHRR, noMod1, memPut); return iNormal; });
-  defOp(0543, "HRRS",  [this]() { doHXXXX(memGet,    memGet, copyHRR, noMod1, selfPut);        return iNormal; });
-  defOp(0544, "HLR",   [this]() { doHXXXX(memGet,    acGet,  copyHLR, noMod1, acPut);  return iNormal; });
-  defOp(0545, "HLRI",  [this]() { doHXXXX(immediate, acGet,  copyHLR, noMod1, acPut);  return iNormal; });
-  defOp(0546, "HLRM",  [this]() { doHXXXX(acGet,     memGet, copyHLR, noMod1, memPut); return iNormal; });
-  defOp(0547, "HLRS",  [this]() { doHXXXX(memGet,    memGet, copyHLR, noMod1, selfPut);        return iNormal; });
-  defOp(0550, "HRRZ",  [this]() { doHXXXX(memGet,    acGet,  copyHRR, zeroL,  acPut);  return iNormal; });
-  defOp(0551, "HRRZI", [this]() { doHXXXX(immediate, acGet,  copyHRR, zeroL,  acPut);  return iNormal; });
-  defOp(0552, "HRRZM", [this]() { doHXXXX(acGet,     memGet, copyHRR, zeroL,  memPut); return iNormal; });
-  defOp(0553, "HRRZS", [this]() { doHXXXX(memGet,    memGet, copyHRR, zeroL,  selfPut);        return iNormal; });
-  defOp(0554, "HLRZ",  [this]() { doHXXXX(memGet,    acGet,  copyHLR, zeroL,  acPut);  return iNormal; });
-  defOp(0555, "HLRZI", [this]() { doHXXXX(immediate, acGet,  copyHLR, zeroL,  acPut);  return iNormal; });
-  defOp(0556, "HLRZM", [this]() { doHXXXX(acGet,     memGet, copyHLR, zeroL,  memPut); return iNormal; });
-  defOp(0557, "HLRZS", [this]() { doHXXXX(memGet,    memGet, copyHLR, zeroL,  selfPut);        return iNormal; });
-  defOp(0560, "HRRO",  [this]() { doHXXXX(memGet,    acGet,  copyHRR, onesL,  acPut);  return iNormal; });
-  defOp(0561, "HRROI", [this]() { doHXXXX(immediate, acGet,  copyHRR, onesL,  acPut);  return iNormal; });
-  defOp(0562, "HRROM", [this]() { doHXXXX(acGet,     memGet, copyHRR, onesL,  memPut); return iNormal; });
-  defOp(0563, "HRROS", [this]() { doHXXXX(memGet,    memGet, copyHRR, onesL,  selfPut);        return iNormal; });
-  defOp(0564, "HLRO",  [this]() { doHXXXX(memGet,    acGet,  copyHLR, onesL,  acPut);  return iNormal; });
-  defOp(0565, "HLROI", [this]() { doHXXXX(immediate, acGet,  copyHLR, onesL,  acPut);  return iNormal; });
-  defOp(0566, "HLROM", [this]() { doHXXXX(acGet,     memGet, copyHLR, onesL,  memPut); return iNormal; });
-  defOp(0567, "HLROS", [this]() { doHXXXX(memGet,    memGet, copyHLR, onesL,  selfPut);        return iNormal; });
-  defOp(0570, "HRRE",  [this]() { doHXXXX(memGet,    acGet,  copyHRR, extnR,  acPut);  return iNormal; });
-  defOp(0571, "HRREI", [this]() { doHXXXX(immediate, acGet,  copyHRR, extnR,  acPut);  return iNormal; });
-  defOp(0572, "HRREM", [this]() { doHXXXX(acGet,     memGet, copyHRR, extnR,  memPut); return iNormal; });
-  defOp(0573, "HRRES", [this]() { doHXXXX(memGet,    memGet, copyHRR, extnR,  selfPut);        return iNormal; });
-  defOp(0574, "HLRE",  [this]() { doHXXXX(memGet,    acGet,  copyHLR, extnR,  acPut);  return iNormal; });
-  defOp(0575, "HLREI", [this]() { doHXXXX(immediate, acGet,  copyHLR, extnR,  acPut);  return iNormal; });
-  defOp(0576, "HLREM", [this]() { doHXXXX(acGet,     memGet, copyHLR, extnR,  memPut); return iNormal; });
-  defOp(0577, "HLRES", [this]() { doHXXXX(memGet,    memGet, copyHLR, extnR,  selfPut);        return iNormal; });
`;

console.log(`\
// Methods` + ops.split(/\n/).
	    map(line=>{
	      const m = line.match(
		/- +defOp\(([0-7]+), +"([^"]+)", +\[this\]\(\) +\{ +doHXXXX\(([a-zA-Z0-9]+), *([a-zA-Z0-9]+), *([a-zA-Z0-9]+), *([a-zA-Z0-9]+), *([a-zA-Z0-9]+)\);.*/);
	      if (m && m[6] == 'noMod1') {
		return m ? `\
InstructionResult ${padOp(`do${m[2]}()`)} { ${padDst(m[7])}(      ${padCopy(m[5])}(${padSrc1(m[3])}(), ${padSrc2(m[4])}()));  return iNormal; }` : ``
	      } else {
		return m ? `\
InstructionResult ${padOp(`do${m[2]}()`)} { ${padDst(m[7])}(${padMod(m[6])}(${padCopy(m[5])}(${padSrc1(m[3])}(), ${padSrc2(m[4])}()))); return iNormal; }` : ``
	      }
	    }).join('\n'));

console.log(`
// Install` + ops.split(/\n/).
	    map(line=>{
	      const m = line.match(
		/- +defOp\(([0-7]+), +"([^"]+)", +\[this\]\(\) +\{ +doHXXXX\(([a-zA-Z0-9]+), *([a-zA-Z0-9]+), *([a-zA-Z0-9]+), *([a-zA-Z0-9]+), *([a-zA-Z0-9]+)\);.*/);
		return m ? `\
  defOp(${m[1]}, ${padMne('"' + m[2] + '",')}static_cast<OpcodeHandler>(&HalfGroup::do${m[2]}));` : ``
	    }).join('\n'));

function padMne(m) {
  return m.padEnd(9);
}

function padOp(op) {
  return op.padEnd(9);
}


function padDst(d) {
  return d.padStart(7);
}


function padSrc1(s) {
  return s.padStart(9);
}


function padSrc2(s) {
  return s.padStart(6);
}


function padCopy(c) {
  return c.padStart(7);
}


function padMod(m) {
  return m.padStart(5);
}
