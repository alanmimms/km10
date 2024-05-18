#pragma once

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <cstdlib>

#include <stdexcept>
#include <thread>

using namespace std;

#include "w36.hpp"
#include "device.hpp"
#include "logging.hpp"
#include "memory.hpp"


struct DTE20: Device {

  DTE20(unsigned anAddr, string aName, Memory &aMemory)
    : Device(anAddr, aName),
      protocolMode(SECONDARY)
  {
    memoryP = &aMemory;
      
    ttyFD = open("/dev/tty", O_RDWR);
    if (ttyFD < 0) throw runtime_error("can't open /dev/tty");

    if (!isatty(ttyFD)) throw runtime_error("MUST be run on a terminal without redirection");

    // Save current termios settings to restore on exit
    int st = tcgetattr(ttyFD, &origTermios);
    if (st < 0) throw runtime_error("failed to tcgetattr for tty");

    /* register the tty reset with the exit handler */
    if (atexit(resetTTY) != 0) throw runtime_error("atexit: can't register tty reset");

    setRAW();

    consoleIOThread = thread(&consoleIOLoop);
  }

  ~DTE20() {
    consoleIOThreadDone = true;
    consoleIOThread.join();
  }


  union CONOMask {

    struct ATTRPACKED {
      unsigned: 3;
      unsigned pi0Enable: 1;
      unsigned piEnable: 1;
      unsigned clearTo10: 1;
      unsigned clearTo11: 1;
      unsigned clear11PI: 1;
      unsigned setReload11: 1;
      unsigned clearReload11: 1;
      unsigned to11Doorbell: 1;

      unsigned: 14;
    };

    uint64_t u: 36;
  };

  unsigned piAssigned;
  bool pi0Enabled;
  bool restricted;

  enum {
    PRIMARY,
    SECONDARY,			// Also called MONITOR mode
  } protocolMode;

  inline static Memory *memoryP;

  thread consoleIOThread;
  bool consoleIOThreadDone;

  inline static bool isRaw{false};
  inline static int ttyFD{-1};
  inline static struct termios origTermios{};


  enum EPTOffset {
    interruptInstr = 0142,
    examineProtWord = 0144,
    examineRelocWord = 145,
    depositProtWord = 146,
    depositRelocWord = 147,
  };

  enum SecondaryProtoEPTOffset {
    opComplete = 0444,
    to10Arg = 0450,
    to11Arg = 0451,
    opInProgress = 0453,
    outputDone = 0455,
    klNotReadyForChar = 0456,
  };


  // I/O instruction handlers
  virtual void clearIO() {
    logging.s << " ; DTE CLEAR IO";
  }


  virtual void doCONO(W36 iw, W36 ea) {
    logging.s << " ; DTE CONO " << oct << ea;
    Logging::nyi();
  }


  virtual void doCONI(W36 iw, W36 ea) {
    logging.s << " ; DTE CONI";
  }


  // TTY handlers and stuff
  static void consoleIOLoop() {

    while (true) {
      char buf[64];
      int st = read(ttyFD, buf, sizeof(buf));
      if (st < 0) throw runtime_error("Error in console TTY read()");

      for (int k=0; k < st; ++k) {

	// Lamely sleep until KL grabs the previous char
	while (memoryP->eptP->DTEKLNotReadyForChar) usleep(100);
	memoryP->eptP->DTEto10Arg = buf[k];
	memoryP->eptP->DTEKLNotReadyForChar = W36::allOnes;
      }
    }
  }


  // reset tty - useful also for restoring the terminal when this
  // process wishes to temporarily relinquish the tty.
  static void resetTTY() {
    /* flush and reset */
    if (isRaw) tcsetattr(ttyFD, TCSAFLUSH, &origTermios);
  }


// Put terminal in raw mode - see termio(7I) for modes.
  static void setRAW() {
    struct termios raw;

    raw = origTermios;  // Copy original and then modify

    // Input Modes. Clear indicated ones giving: no break, no CR to
    // NL, no parity check, no strip char, no start/stop output (sic)
    // control.
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);

    // Output modes. Clear giving: no post processing such as NL to
    // CR+NL.
    raw.c_oflag &= ~(OPOST);

    // Control modes. Set 8 bit chars.
    raw.c_cflag |= CS8;

    // Local modes. Clear giving: echoing off, canonical off (no erase
    // with backspace, ^U,...), no extended functions, no signal chars
    // (^Z,^C).
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

    // Control chars. Reorder to set return condition: min number of
    // bytes and timer.
    raw.c_cc[VMIN] = 5; raw.c_cc[VTIME] = 1; // 5 bytes or .8sec after first byte
    raw.c_cc[VMIN] = 2; raw.c_cc[VTIME] = 0; // after two bytes, no timer
    raw.c_cc[VMIN] = 0; raw.c_cc[VTIME] = 8; // after a byte or .8 seconds
    raw.c_cc[VMIN] = 0; raw.c_cc[VTIME] = 0; // immediate - anything

    // Put terminal in raw mode after flushing
    if (tcsetattr(ttyFD, TCSAFLUSH, &raw) < 0) throw runtime_error("can't set TTY raw mode");

    isRaw = true;
  }
};
