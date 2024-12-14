#include "km10.hpp"

void installMoveGroup(KM10 &c) {
  c.defOp(0200, "MOVE",  [&]() {c.acPut(c.memGet()); return iNormal;} );
  c.defOp(0201, "MOVEI", [&]() {c.acPut(c.immediate()); return iNormal;} );
  c.defOp(0202, "MOVEM", [&]() {c.memPut(c.acGet()); return iNormal;} );
  c.defOp(0203, "MOVES", [&]() {c.selfPut(c.memGet()); return iNormal;} );

  c.defOp(0204, "MOVS",  [&]() {c.acPut(c.swap(c.memGet())); return iNormal;} );
  c.defOp(0205, "MOVSI", [&]() {c.acPut(c.swap(c.immediate())); return iNormal;} );
  c.defOp(0206, "MOVSM", [&]() {c.memPut(c.swap(c.acGet())); return iNormal;} );
  c.defOp(0207, "MOVSS", [&]() {c.selfPut(c.swap(c.memGet())); return iNormal;} );

  c.defOp(0210, "MOVN",  [&]() {c.acPut(c.negate(c.memGet())); return iNormal;} );
  c.defOp(0211, "MOVNI", [&]() {c.acPut(c.negate(c.immediate())); return iNormal;} );
  c.defOp(0212, "MOVNM", [&]() {c.memPut(c.swap(c.acGet())); return iNormal;} );
  c.defOp(0213, "MOVNS", [&]() {c.selfPut(c.swap(c.memGet())); return iNormal;} );

  c.defOp(0214, "MOVM",  [&]() {c.acPut(c.magnitude(c.memGet())); return iNormal;} );
  c.defOp(0215, "MOVMI", [&]() {c.acPut(c.magnitude(c.immediate())); return iNormal;} );
  c.defOp(0216, "MOVMM", [&]() {c.memPut(c.magnitude(c.acGet())); return iNormal;} );
  c.defOp(0217, "MOVMS", [&]() {c.selfPut(c.magnitude(c.memGet())); return iNormal;} );
}
