#pragma once
#include "km10.hpp" // Ensure KM10 base class is included

#define DEFOP(O,M,H)	(c.ops[O] = H)

struct MoveGroup {
  MoveGroup(KM10 &cpu) : c(cpu) {}

  KM10 &c;

  inline void install() {
    DEFOP(0200, MOVE,  [&]() {c.acPut(c.memGet()); return iNormal;} );
    DEFOP(0201, MOVEI, [&]() {c.acPut(c.immediate()); return iNormal;} );
    DEFOP(0202, MOVEM, [&]() {c.memPut(c.acGet()); return iNormal;} );
    DEFOP(0203, MOVES, [&]() {c.selfPut(c.memGet()); return iNormal;} );

    DEFOP(0204, MOVS,  [&]() {c.acPut(c.swap(c.memGet())); return iNormal;} );
    DEFOP(0205, MOVSI, [&]() {c.acPut(c.swap(c.immediate())); return iNormal;} );
    DEFOP(0206, MOVSM, [&]() {c.memPut(c.swap(c.acGet())); return iNormal;} );
    DEFOP(0207, MOVSS, [&]() {c.selfPut(c.swap(c.memGet())); return iNormal;} );

    DEFOP(0210, MOVN,  [&]() {c.acPut(c.negate(c.memGet())); return iNormal;} );
    DEFOP(0211, MOVNI, [&]() {c.acPut(c.negate(c.immediate())); return iNormal;} );
    DEFOP(0212, MOVNM, [&]() {c.memPut(c.swap(c.acGet())); return iNormal;} );
    DEFOP(0213, MOVNS, [&]() {c.selfPut(c.swap(c.memGet())); return iNormal;} );

    DEFOP(0214, MOVM,  [&]() {c.acPut(c.magnitude(c.memGet())); return iNormal;} );
    DEFOP(0215, MOVMI, [&]() {c.acPut(c.magnitude(c.immediate())); return iNormal;} );
    DEFOP(0216, MOVMM, [&]() {c.memPut(c.magnitude(c.acGet())); return iNormal;} );
    DEFOP(0217, MOVMS, [&]() {c.selfPut(c.magnitude(c.memGet())); return iNormal;} );
  }
};
