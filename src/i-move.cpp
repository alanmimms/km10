#include "km10.hpp"

void KM10::installMoveGroup() {
  defOp(0200, "MOVE",  [this]() {acPut(memGet());		return iNormal;} );
  defOp(0201, "MOVEI", [this]() {acPut(immediate());		return iNormal;} );
  defOp(0202, "MOVEM", [this]() {memPut(acGet());		return iNormal;} );
  defOp(0203, "MOVES", [this]() {selfPut(memGet());		return iNormal;} );

  defOp(0204, "MOVS",  [this]() {acPut(swap(memGet()));		return iNormal;} );
  defOp(0205, "MOVSI", [this]() {acPut(swap(immediate()));	return iNormal;} );
  defOp(0206, "MOVSM", [this]() {memPut(swap(acGet()));		return iNormal;} );
  defOp(0207, "MOVSS", [this]() {selfPut(swap(memGet()));	return iNormal;} );

  defOp(0210, "MOVN",  [this]() {acPut(negate(memGet()));	return iNormal;} );
  defOp(0211, "MOVNI", [this]() {acPut(negate(immediate()));	return iNormal;} );
  defOp(0212, "MOVNM", [this]() {memPut(negate(acGet()));	return iNormal;} );
  defOp(0213, "MOVNS", [this]() {selfPut(negate(memGet()));	return iNormal;} );

  defOp(0214, "MOVM",  [this]() {acPut(magnitude(memGet()));	return iNormal;} );
  defOp(0215, "MOVMI", [this]() {acPut(magnitude(immediate())); return iNormal;} );
  defOp(0216, "MOVMM", [this]() {memPut(magnitude(acGet()));	return iNormal;} );
  defOp(0217, "MOVMS", [this]() {selfPut(magnitude(memGet()));	return iNormal;} );
}
