{
  let unroll = options.util.makeUnroll(location, options);

//  let DBG = console.log;
  let DBG = function() {}

  let ast = function(a) {
    DBG("makeAST('" + a + "')");
    return options.util.makeAST(location, options)(a);
  }
}

cpuDef "CPU definition" = defs:insnList	{ DBG("cpu:", text());
					  let a = ast("cpu");
					  defs.forEach(d => a.add(d));
					  return a;
					}

insnList = insns:insnDef+		{ DBG("insnList:", text());
					  return insns;
					}

insnDef "instruction definition"
=	_ EOL				{ DBG("EOL"); return null; }
/	m:insnName _ op:opcode _ ':' _ action:action _ actions:( EOL _ action _ )* EOL
					{ DBG("Insn:", text());
					  return ast("Instruction")
					      .set(m).set(op)
					      .add(unroll(action, actions, 2));
					}

insnName = mnemonic:mnemonic		{
					  DBG("Mnemonic:", text());
					  return {mnemonic};
					}

mnemonic = mnemonic:$[A-Z]+

opcode
=       _ '7dd' op:$([0-7]+)		{ DBG("opcode:", text());
					  let p = {
					    op: '700' + op,
					    dd: true,
					  };
					  return p;
					}

/	_ op:$([0-7]+) nn:$('nn')?	{ DBG("opcode:", text());
					  let p = {op};
					  p.nn = (nn.length != 0);
					  if (p.nn) p.op = p.op + '00';
					  return p;
					}

action = cond:ifCondition? _ act:condAction
					{ DBG("action:", text());
					  return cond ? cond.add(act) : act;
					}

condAction
=	rtl:rtl				{ DBG("rtl:", text());
					  return rtl;
					}
/	'{' code:$(!'}'.)* '}'		{ DBG("code:", text());
					  return ast("Code").set({code});
					}
/	'skip'				{ DBG("skip:", text());
					  return ast("Skip");
					}

lhs
=	a:operand _ op:$(operator) _ b:operand
					{ DBG("lhs binary:", text());
					  return ast('binOp').set({op}).add([a, b]);
					}
/	a:operand			{ DBG("lhs:", text());
					  return a;
					}

rtl =	lhs:lhs _
	rtlOp:$( ('->' / '<->') { return ast(text()); })
	dsts:( _ operand )+
					{ DBG("rtl:", text());
					  return ast(rtlOp).add(unroll(lhs, dsts, 1));
					}
operator
=	'f<<'
/	'fu+'
/	( 'fr' / 'fl' / 'f' / 'd' )? ( '+' / '-' / '*' / '/' )
/	'<<' [aru]
/	'|'
/	'&'
/	'^'
/	'><'		// EQV

singletonOperand
=	'(' op:operand ')' mod:modifier?
					{ DBG("()");
					  let a = ast('()').add(op);
					  if (mod) a.set({mod});
					  return a;
					}

/	'|' op:operand '|' mod:modifier?
					{ DBG("||");
					  let a = ast("||").add(op);
					  if (mod) a.set({mod});
					  return a;
					}
/	'[' op:operand ']' mod:modifier?
					{ DBG("[]:", text());
					  let a = ast("[]").add(op);
					  if (mod) a.set({mod});
					  return a;
					}
/	ref:regRef mod:modifier?	{ DBG("RegOp:");
					  if (mod) ref.set({mod});
					  return ref;
					}
/	uop:uop op:operand		{ DBG("uop:", text());
					  return ast("unOp").set("op", uop).add(op);
					}
/	n:$([0-7_]+)			{ DBG("n:", text());
					  return ast("Number").set({n});
					}

uop = '-D'
	/ '~D'
	/ '-'
	/ '~'
	/ 'D'
	/ 'DG2F'
	/ 'Dfix'
	/ 'DfixD'
	/ 'DfixR'
	/ 'DfixRD'
	/ 'Difr'
	/ 'DifrD'
	/ 'F2G'
	/ 'G2F'
	/ 'jffo'
	/ 'Q'
	/ 'dble'
	/ 'fix'
	/ 'fixR'
	/ 'fixRD'
	/ 'ifr'
	/ 'ifrD'
	/ 'sngl'

operand
=	lh:singletonOperand ',,' rh:singletonOperand
					{ DBG("pair:", text());
					  return ast("Pair").add(lh, rh);
					}
/	s:singletonOperand mod:modifier?
					{ DBG("singleton:", text());
					  if (mod) s.set({mod});
					  return s;
					}

modifier = '.' modName			{ DBG("modifier:", text());
					  return text();
					}

regRef = name:regName range:range?	{ DBG("Reg:", text());
					  let a = ast("Reg").set({name});
					  return range ? a.add(range) : a;
					}

range = ',' op2:regName '+' end:$[0-7]+
					{ DBG("range:", text());
					  return ast("Range").set({op2, end});
					}

regName = (! 'if') [a-z]+ ( '+' [0-9]+)?
					{ return text(); }

modName = mod:$([a-zA-Z]+ [a-zA-Z0-9]*)	{ DBG("Mod:", text()); return mod; }

ifCondition
=	'if' _ a:operand mask:( _ '&' _ m:operand { return m; } )? _ op:condOp _ b:operand _ ':'
					{ DBG("if:", text());
					  if (mask) a = ast('binOp').set('op', '&').add([a, mask]);
					  return ast("If").set({op}).add([a, b]);
					}

condOp = cond: $( '===' / '!==' / '<=' / '<'/ '>=' / '>' )
					{ DBG("cond:", text());
					  return text();
					}

EOL "end of line" = '\r\n' / '\r' / '\n'

_ "whitespace or comments"
=	(   [ \t]+   /   '//' (!EOL .)*   /   '/*' (!'*/' .)* '*/'   )*
