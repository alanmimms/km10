'use strict'

const ops = `
    defOp(220, "IMUL",  [this]() { return doBinOp(acGet,     memGet, imulWord,    acPut); });
    defOp(221, "IMULI", [this]() { return doBinOp(acGet,  immediate, imulWord,    acPut); });
    defOp(222, "IMULM", [this]() { return doBinOp(acGet,     memGet, imulWord,   memPut); });
    defOp(223, "IMULB", [this]() { return doBinOp(acGet,     memGet, imulWord,  bothPut); });
    defOp(224, "MUL",   [this]() { return doBinOp(acGet,     memGet,  mulWord,   acPut2); });
    defOp(225, "MULI",  [this]() { return doBinOp(acGet,  immediate,  mulWord,   acPut2); });
    defOp(226, "MULM",  [this]() { return doBinOp(acGet,     memGet,  mulWord, memPutHi); });
    defOp(227, "MULB",  [this]() { return doBinOp(acGet,     memGet,  mulWord, bothPut2); });
    defOp(230, "IDIV",  [this]() { return doBinOp(acGet,     memGet, idivWord,   acPut2); });
    defOp(231, "IDIVI", [this]() { return doBinOp(acGet,  immediate, idivWord,   acPut2); });
    defOp(232, "IDIVM", [this]() { return doBinOp(acGet,     memGet, idivWord, memPutHi); });
    defOp(233, "IDIVB", [this]() { return doBinOp(acGet,     memGet, idivWord, bothPut2); });
    defOp(234, "DIV",   [this]() { return doBinOp(acGet2,    memGet,  divWord,   acPut2); });
    defOp(235, "DIVI",  [this]() { return doBinOp(acGet2, immediate,  divWord,   acPut2); });
    defOp(236, "DIVM",  [this]() { return doBinOp(acGet2,    memGet,  divWord, memPutHi); });
    defOp(237, "DIVB",  [this]() { return doBinOp(acGet2,    memGet,  divWord, bothPut2); });

    defOp(270, "ADD",   [this]() { return doBinOp(acGet,    memGet, addWord,   acPut); });
    defOp(271, "ADDI",  [this]() { return doBinOp(acGet, immediate, addWord,   acPut); });
    defOp(272, "ADDM",  [this]() { return doBinOp(acGet,    memGet, addWord,  memPut); });
    defOp(273, "ADDB",  [this]() { return doBinOp(acGet,    memGet, addWord, bothPut); });
    defOp(274, "SUB",   [this]() { return doBinOp(acGet,    memGet, subWord,   acPut); });
    defOp(275, "SUBI",  [this]() { return doBinOp(acGet, immediate, subWord,   acPut); });
    defOp(276, "SUBM",  [this]() { return doBinOp(acGet,    memGet, subWord,  memPut); });
    defOp(277, "SUBB",  [this]() { return doBinOp(acGet,    memGet, subWord, bothPut); });

    defOp(430, "XOR",     [this]() { return doBinOp(   memGet, acGet,   xorWord,   acPut); });
    defOp(431, "XORI",    [this]() { return doBinOp(immediate, acGet,   xorWord,   acPut); });
    defOp(432, "XORM",    [this]() { return doBinOp(   memGet, acGet,   xorWord,  memPut); });
    defOp(433, "XORB",    [this]() { return doBinOp(   memGet, acGet,   xorWord, bothPut); });
    defOp(434, "IOR",     [this]() { return doBinOp(   memGet, acGet,   iorWord,   acPut); });
    defOp(435, "IORI",    [this]() { return doBinOp(immediate, acGet,   iorWord,   acPut); });
    defOp(436, "IORM",    [this]() { return doBinOp(   memGet, acGet,   iorWord,  memPut); });
    defOp(437, "IORB",    [this]() { return doBinOp(   memGet, acGet,   iorWord, bothPut); });
    defOp(440, "ANDCBM",  [this]() { return doBinOp(   memGet, acGet, andCBWord,   acPut); });
    defOp(441, "ANDCBMI", [this]() { return doBinOp(immediate, acGet, andCBWord,   acPut); });
    defOp(442, "ANDCBMM", [this]() { return doBinOp(   memGet, acGet, andCBWord,  memPut); });
    defOp(443, "ANDCBMB", [this]() { return doBinOp(   memGet, acGet, andCBWord, bothPut); });
    defOp(444, "EQV",     [this]() { return doBinOp(   memGet, acGet,   eqvWord,   acPut); });
    defOp(445, "EQVI",    [this]() { return doBinOp(immediate, acGet,   eqvWord,   acPut); });
    defOp(446, "EQVM",    [this]() { return doBinOp(   memGet, acGet,   eqvWord,  memPut); });
    defOp(447, "EQVB",    [this]() { return doBinOp(   memGet, acGet,   eqvWord, bothPut); });
`;

const lineRE = / +defOp\((?<op>[0-7]+), *"(?<mne>[^"]+)", *\[this\]\(\) *\{ return +doBinOp\( *(?<get1>[a-zA-Z0-9]+), *(?<get2>[a-zA-Z0-9]+), *(?<mod>[a-zA-Z0-9]+), *(?<store>[a-zA-Z0-9]+)\);.*/;

console.log(`\
// Methods` + ops.split(/\n/).
	    map(line=>{
	      const m = line.match(lineRE);
	      if (!m) return '';

	      return `\

InstructionResult do${m.groups.mne}() {
    W36 a1 = ${m.groups.get1}();
    W36 a2 = ${m.groups.get2}();
    ${doModStore(m)}
    return iNormal;
  }`;
	    }).join('\n'));

console.log(`
  void install() {
` + ops.split(/\n/).
	    map(line=>{
	      const m = line.match(lineRE);
	      return m ? `\
  defOp(${m.groups.op}, "${m.groups.mne}", static_cast<OpcodeHandler>(&IntBinGroup::do${m.groups.mne}));` : ``;
	    }).join('\n'));
console.log(`  }
};
`);


function doModStore(m) {
  return (m.groups.store == 'noStore') ? `\
/* No store */;` : `\
${m.groups.store}(${m.groups.mod}(a1, a2));`;
}
