#pragma once
#include <cstdint>
#include <map>
#include <assert.h>

using namespace std;

#include "word.hpp"
#include "logger.hpp"
#include "instruction-result.hpp"


class KM10;

struct Device {
  unsigned ioAddress;
  string name;
  KM10 &km10;			// The CPU we belong to.
  unsigned intLevel;
  bool intPending;

  // Set this in DTE20 device so it can cause interrupts on level #0.
  bool canIntLevel0;

  static inline map<unsigned, Device *> devices{};

  // Constructors
  Device(unsigned anAddr, string aName, KM10 &cpu, bool aCanIntLevel0 = false)
    : ioAddress(anAddr),
      name(aName),
      km10(cpu),
      intLevel(0),
      intPending(false),
      canIntLevel0(aCanIntLevel0)
  {
    devices[ioAddress] = this;
  }


  // Return the device's interrupt function word for its highest
  // pending interrupt. Default is to use fixed vector for the level.
  // Return zero for level to indicate no pending interrupt from this
  // device.
  virtual tuple<unsigned,W36> getIntFuncWord();


  // Clear the I/O system by calling each device's clearIO() entry
  // point.
  static void clearAll();


  // Request an interrupt at this Device's assigned level.
  virtual void requestInterrupt();


  // Handle an I/O instruction by calling the appropriate device
  // driver's I/O instruction handler method.
  static InstructionResult handleIO(W36 iw, W36 ea);


  // I/O instruction handlers
  virtual void clearIO();

  virtual InstructionResult doDATAI(W36 iw, W36 ea);
  virtual InstructionResult doDATAO(W36 iw, W36 ea);
  virtual InstructionResult doBLKI(W36 iw, W36 ea);
  virtual InstructionResult doBLKO(W36 iw, W36 ea);
  virtual InstructionResult doCONO(W36 iw, W36 ea);
  virtual InstructionResult doCONI(W36 iw, W36 ea);
  virtual InstructionResult doCONSZ(W36 iw, W36 ea);
  virtual InstructionResult doCONSO(W36 iw, W36 ea);
};
