#include "km10.hpp"

KM10::MOVE::MOVE() {

  auto doMOVXX = [](auto getSrcF, auto modifyF, auto putDstF) {

    return []() {
      putDstF(modifyF(getSrcF()));
    };
  };
  
  KM10::ops[0200] = doMOVXX(memGet, noMod1, acPut);	  // MOVE
  KM10::ops[0201] = doMOVXX(immediate, noMod1, acPut);    // MOVEI
  KM10::ops[0202] = doMOVXX(acGet, noMod1, memPut);	  // MOVEM
  KM10::ops[0203] = doMOVXX(memGet, noMod1, selfPut);	  // MOVES
  KM10::ops[0204] = doMOVXX(memGet, swap, acPut);	  // MOVS
  KM10::ops[0205] = doMOVXX(immediate, swap, acPut);	  // MOVSI
  KM10::ops[0206] = doMOVXX(acGet, swap, memPut);	  // MOVSM
  KM10::ops[0207] = doMOVXX(memGet, swap, selfPut);	  // MOVSS
  KM10::ops[0210] = doMOVXX(memGet, negate, acPut);	  // MOVN
  KM10::ops[0211] = doMOVXX(immediate, negate, acPut);    // MOVNI
  KM10::ops[0212] = doMOVXX(acGet, negate, memPut);	  // MOVNM
  KM10::ops[0213] = doMOVXX(memGet, negate, selfPut);	  // MOVNS
  KM10::ops[0214] = doMOVXX(memGet, magnitude, acPut);    // MOVM
  KM10::ops[0215] = doMOVXX(immediate, magnitude, acPut); // MOVMI
  KM10::ops[0216] = doMOVXX(acGet, magnitude, memPut);    // MOVMM
  KM10::ops[0217] = doMOVXX(memGet, magnitude, selfPut);  // MOVMS
}
