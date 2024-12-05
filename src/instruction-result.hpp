#pragma once

// Each KM10 instruction method returns this to indicate what type of
// instruction it was. This affects how PC is updated, whether a trap
// is to be executed, etc.
//
// At the end of an instruction, iNormal, iSkip, iNoSuchDevice, and
// iNYI all use pc + pcOffset as the next fetchPC address.
enum InstructionResult {
  iNormal,	 // Normal execution with no PC modification or traps.
  iSkip,	 // Instruction caused a skip condition.
  iJump,	 // Instruction changed the PC.
  iMUUO,	 // Instruction is a MUUO.
  iLUUO,	 // Instruction is a LUUO.
  iTrap,	 // Instruction caused a trap condition.
  iHALT,	 // Instruction halted the CPU.
  iXCT,		 // Instruction is an XCT.
  iNoSuchDevice, // Instruction is I/O operation on a non-existent device.
  iNYI,		 // Instruction is not yet implemented.
};
