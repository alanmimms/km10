'use strict'

const predicates = {
  isLT0T: '< 0',
  isLE0T: '<= 0',
  isGT0T: '> 0',
  isGE0T: '>= 0',
  isNE0T: '!= 0',
  isEQ0T: '== 0',
};


const ops = `
  defOp(0600, "TRN",    [this]() { return iNormal; });
  defOp(0601, "TLN",    [this]() { return iNormal; });
  defOp(0602, "TRNE",   [this]() { return doTXXXX(acGetRH, getE, noMod2,    isEQ0T,  noStore); });
  defOp(0603, "TLNE",   [this]() { return doTXXXX(acGetLH, getE, noMod2,    isEQ0T,  noStore); });
  defOp(0604, "TRNA",   [this]() { return iSkip; });
  defOp(0605, "TLNA",   [this]() { return iSkip; });
  defOp(0606, "TRNN",   [this]() { return doTXXXX(acGetRH, getE, noMod2,    isNE0T,  noStore); });
  defOp(0607, "TLNN",   [this]() { return doTXXXX(acGetLH, getE, noMod2,    isNE0T,  noStore); });
  defOp(0610, "TRZ",    [this]() { return doTXXXX(acGetRH, getE, zeroMaskR, neverT,  acPutRH); });
  defOp(0611, "TLZ",    [this]() { return doTXXXX(acGetLH, getE, zeroMaskR, neverT,  acPutLH); });
  defOp(0612, "TRZE",   [this]() { return doTXXXX(acGetRH, getE, zeroMaskR, isEQ0T,  acPutRH); });
  defOp(0613, "TLZE",   [this]() { return doTXXXX(acGetLH, getE, zeroMaskR, isEQ0T,  acPutLH); });
  defOp(0614, "TRZA",   [this]() { return doTXXXX(acGetRH, getE, zeroMaskR, alwaysT, acPutRH); });
  defOp(0615, "TLZA",   [this]() { return doTXXXX(acGetLH, getE, zeroMaskR, alwaysT, acPutLH); });
  defOp(0616, "TRZN",   [this]() { return doTXXXX(acGetRH, getE, zeroMaskR, isNE0T,  acPutRH); });
  defOp(0617, "TLZN",   [this]() { return doTXXXX(acGetLH, getE, zeroMaskR, isNE0T,  acPutLH); });
  defOp(0620, "TRC",    [this]() { return doTXXXX(acGetRH, getE, compMaskR, neverT,  acPutRH); });
  defOp(0621, "TLC",    [this]() { return doTXXXX(acGetLH, getE, compMaskR, neverT,  acPutLH); });
  defOp(0622, "TRCE",   [this]() { return doTXXXX(acGetRH, getE, compMaskR, isEQ0T,  acPutRH); });
  defOp(0623, "TLCE",   [this]() { return doTXXXX(acGetLH, getE, compMaskR, isEQ0T,  acPutLH); });
  defOp(0624, "TRCA",   [this]() { return doTXXXX(acGetRH, getE, compMaskR, alwaysT, acPutRH); });
  defOp(0625, "TLCA",   [this]() { return doTXXXX(acGetLH, getE, compMaskR, alwaysT, acPutLH); });
  defOp(0626, "TRCN",   [this]() { return doTXXXX(acGetRH, getE, compMaskR, isNE0T,  acPutRH); });
  defOp(0627, "TLCN",   [this]() { return doTXXXX(acGetLH, getE, compMaskR, isNE0T,  acPutLH); });
  defOp(0630, "TRO",    [this]() { return doTXXXX(acGetRH, getE, onesMaskR, neverT,  acPutRH); });
  defOp(0631, "TLO",    [this]() { return doTXXXX(acGetLH, getE, onesMaskR, neverT,  acPutLH); });
  defOp(0632, "TROE",   [this]() { return doTXXXX(acGetRH, getE, onesMaskR, isEQ0T,  acPutRH); });
  defOp(0633, "TLOE",   [this]() { return doTXXXX(acGetLH, getE, onesMaskR, isEQ0T,  acPutLH); });
  defOp(0634, "TROA",   [this]() { return doTXXXX(acGetRH, getE, onesMaskR, alwaysT, acPutRH); });
  defOp(0635, "TLOA",   [this]() { return doTXXXX(acGetLH, getE, onesMaskR, alwaysT, acPutLH); });
  defOp(0636, "TRON",   [this]() { return doTXXXX(acGetRH, getE, onesMaskR, isNE0T,  acPutRH); });
  defOp(0637, "TLON",   [this]() { return doTXXXX(acGetLH, getE, onesMaskR, isNE0T,  acPutLH); });
  defOp(0640, "TDN",    [this]() { return iNormal; });
  defOp(0641, "TSN",    [this]() { return iNormal; });
  defOp(0642, "TDNE",   [this]() { return doTXXXX(acGet, memGet,        noMod2, isEQ0T, noStore); });
  defOp(0643, "TSNE",   [this]() { return doTXXXX(acGet, memGetSwapped, noMod2, isEQ0T, noStore); });
  defOp(0644, "TDNA",   [this]() { return iSkip; });
  defOp(0645, "TSNA",   [this]() { return iSkip; });
  defOp(0646, "TDNN",   [this]() { return doTXXXX(acGet, memGet,        noMod2,   isNE0T, noStore); });
  defOp(0647, "TSNN",   [this]() { return doTXXXX(acGet, memGetSwapped, noMod2,   isNE0T, noStore); });
  defOp(0650, "TDZ",    [this]() { return doTXXXX(acGet, memGet,        zeroMask, neverT, acPut); });
  defOp(0651, "TSZ",    [this]() { return doTXXXX(acGet, memGetSwapped, zeroMask, neverT, acPut); });
  defOp(0652, "TDZE",   [this]() { return doTXXXX(acGet, memGet,        zeroMask, isEQ0T, acPut); });
  defOp(0653, "TSZE",   [this]() { return doTXXXX(acGet, memGetSwapped, zeroMask, isEQ0T, acPut); });
  defOp(0654, "TDZA",   [this]() { return doTXXXX(acGet, memGet,        zeroMask, alwaysT, acPut); });
  defOp(0655, "TSZA",   [this]() { return doTXXXX(acGet, memGetSwapped, zeroMask, alwaysT, acPut); });
  defOp(0656, "TDZN",   [this]() { return doTXXXX(acGet, memGet,        zeroMask, isNE0T, acPut); });
  defOp(0657, "TSZN",   [this]() { return doTXXXX(acGet, memGetSwapped, zeroMask, isNE0T, acPut); });
  defOp(0660, "TDC",    [this]() { return doTXXXX(acGet, memGet,        compMask, neverT, acPut); });
  defOp(0661, "TSC",    [this]() { return doTXXXX(acGet, memGetSwapped, compMask, neverT, acPut); });
  defOp(0662, "TDCE",   [this]() { return doTXXXX(acGet, memGet,        compMask, isEQ0T, acPut); });
  defOp(0663, "TSCE",   [this]() { return doTXXXX(acGet, memGetSwapped, compMask, isEQ0T, acPut); });
  defOp(0664, "TDCA",   [this]() { return doTXXXX(acGet, memGet,        compMask, alwaysT, acPut); });
  defOp(0665, "TSCA",   [this]() { return doTXXXX(acGet, memGetSwapped, compMask, alwaysT, acPut); });
  defOp(0666, "TDCN",   [this]() { return doTXXXX(acGet, memGet,        compMask, isNE0T, acPut); });
  defOp(0667, "TSZCN",  [this]() { return doTXXXX(acGet, memGetSwapped, compMask, isNE0T, acPut); });
  defOp(0670, "TDO",    [this]() { return doTXXXX(acGet, memGet,        onesMask, neverT, acPut); });
  defOp(0671, "TSO",    [this]() { return doTXXXX(acGet, memGetSwapped, onesMask, neverT, acPut); });
  defOp(0672, "TDOE",   [this]() { return doTXXXX(acGet, memGet,        onesMask, isEQ0T, acPut); });
  defOp(0673, "TSOE",   [this]() { return doTXXXX(acGet, memGetSwapped, onesMask, isEQ0T, acPut); });
  defOp(0674, "TDOA",   [this]() { return doTXXXX(acGet, memGet,        onesMask, alwaysT, acPut); });
  defOp(0675, "TSOA",   [this]() { return doTXXXX(acGet, memGetSwapped, onesMask, alwaysT, acPut); });
  defOp(0676, "TDON",   [this]() { return doTXXXX(acGet, memGet,        onesMask, isNE0T, acPut); });
  defOp(0677, "TSON",   [this]() { return doTXXXX(acGet, memGetSwapped, onesMask, isNE0T, acPut); });
`;

const lineRE = / +defOp\((?<op>[0-7]+), +"(?<mne>[^"]+)", +\[this\]\(\) +\{ return +(?:doTXXXX\((?<get1>[a-zA-Z0-9]+), *(?<get2>[a-zA-Z0-9]+), *(?<mod>[a-zA-Z0-9]+), *(?<cond>[a-zA-Z0-9]+), *(?<store>[a-zA-Z0-9]+)\))?(?<ret>iNormal|iSkip)?;.*/;

console.log(`\
// Methods` + ops.split(/\n/).
	    map(line=>{
	      const m = line.match(lineRE);
	      if (!m) return '';

	      if (m.groups.store) {

		return `\

InstructionResult do${m.groups.mne}() {
    W36 a1 = ${m.groups.get1}();
    W36 a2 = ${m.groups.get2}();
    const bool doSkip = ${doCond(m)};
    ${doModStore(m)}
    return doSkip ? iSkip : iNormal;
  }`;
	      } else {
		return `\

InstructionResult do${m.groups.mne}() { return ${m.groups.ret}; }`;
	      }
	    }).join('\n'));

console.log(`
// Install` + ops.split(/\n/).
	    map(line=>{
	      const m = line.match(lineRE);
	      return m ? `\
  defOp(${m.groups.op}, "${m.groups.mne}", static_cast<OpcodeHandler>(&TestGroup::do${m.groups.mne}));` : ``;
	    }).join('\n'));


function doModStore(m) {
  return (m.groups.store == 'noStore') ? `\
/* No store */;` : `\
${m.groups.store}(${m.groups.mod}(a1, a2));`;
}


function doCond(m) {
  const pred = predicates[m.groups.cond];

  if (pred) {
    return `(a1.u & a2.u) ${pred}`;
  } else {

    if (m.groups.cond == 'alwaysT')
      return `true`;
    else if (m.groups.cond == 'neverT')
      return `false`;
    else
      return `!!!! "${m.groups.cond}" is an unmatched cond`;
  }
}
