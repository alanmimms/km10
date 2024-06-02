#pragma once

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <poll.h>
#include <cstdlib>
#include <cstdio>

#include <stdexcept>
#include <thread>

using namespace std;

#include "w36.hpp"
#include "device.hpp"
#include "logger.hpp"
#include "kmstate.hpp"


struct DTE20: Device {

  DTE20(unsigned anAddr, string aName, KMState &aState)
    : Device(anAddr, aName),
      protocolMode(SECONDARY),
      isConnected(false)
  {
    stateP = &aState;
      
    ttyFD = open("/dev/tty", O_RDWR);
    if (ttyFD < 0) throw runtime_error("can't open /dev/tty");

    if (!isatty(ttyFD)) throw runtime_error("MUST be run on a terminal without redirection");

    // Save current termios settings to restore on exit
    int st = tcgetattr(ttyFD, &origTermios);
    if (st < 0) throw runtime_error("failed to tcgetattr for tty");

    /* register the tty reset with the exit handler */
    if (atexit(resetTTY) != 0) throw runtime_error("atexit: can't register tty reset");
  }

  ~DTE20() {
    disconnect();
  }


  void connect() {
    setRaw();
    logger.endl = "\r\n";

    int pipeFDs[2];
    int st = pipe(pipeFDs);
    if (st < 0) perror("Error creation console I/O pipe");

    fromIOLoopFD = pipeFDs[0];
    toIOLoopFD = pipeFDs[1];

    consoleIOThread = thread(&consoleIOLoop);
    isConnected = true;
  }


  void disconnect() {

    if (isConnected) {
      consoleIOThreadDone = true;
      write(toIOLoopFD, "bye", 3);
      consoleIOThread.join();
      close(fromIOLoopFD);
      close(toIOLoopFD);
      isConnected = false;
      if (isRaw) setNormal();
      logger.endl = "\n";
    }
  }


  union CONOMask {

    struct ATTRPACKED {
      unsigned: 3;
      unsigned pi0Enable: 1;	// 32
      unsigned piEnable: 1;
      unsigned clearTo10: 1;
      unsigned clearTo11: 1;
      unsigned: 2;
      unsigned clear11PI: 1;
      unsigned: 1;
      unsigned setReload11: 1;
      unsigned clearReload11: 1;
      unsigned to11Doorbell: 1;

      unsigned: 4;
    };

    unsigned u: 18;

    CONOMask(unsigned ea) { u = ea; }
  };

  unsigned piAssigned;
  bool pi0Enabled;
  bool restricted;

  enum {
    PRIMARY,
    SECONDARY,			// Also called MONITOR mode
  } protocolMode;

  // See klcom.mem p.50
  enum DTECMD {
    ctyOutput = 010,
    enterSecondaryProtocol,
    enterPrimaryProtocol,
  };

  union MonitorCommand {

    struct ATTRPACKED {
      unsigned data: 8;
      unsigned fn: 8;
    };

    unsigned u;
  };

  inline static KMState *stateP;

  thread consoleIOThread;
  bool consoleIOThreadDone;
  bool isConnected;
  inline static int toIOLoopFD;
  inline static int fromIOLoopFD;

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
    if (logger.dte) logger.s << " ; DTE CLEAR IO";
  }


  virtual void doCONO(W36 iw, W36 ea) {
    CONOMask req(ea);
    if (logger.dte) logger.s << " ; DTE CONO " << oct << ea;

    if (req.to11Doorbell) {
      char buf;

      MonitorCommand mc{stateP->eptP->DTEto11Arg.rhu};

      if (logger.dte) logger.s << " to11DoorBell arg=" << oct << stateP->eptP->DTEto11Arg.rhu;

      switch (mc.fn) {
      case 0:			// XXX This should not be required?!
      case ctyOutput:
	buf = mc.data;
	write(1, &buf, 1);
	stateP->eptP->DTEMonitorOpComplete = W36(W36::allOnes);
	break;

      case enterSecondaryProtocol:
	cerr << "DTE20 enter secondary protocol command with data " << mc.data << " (ignored)."
	     << logger.endl;
	break;

      case enterPrimaryProtocol:
	cerr << "DTE20 enter primary protocol command with data " << mc.data << " (ignored)."
	     << logger.endl;
	break;

      default:
	cerr << "Unimplemented DTECMD: " << oct << mc.fn << " with data " << mc.data << " (ignored)."
	     << logger.endl;
	break;
      }
    } else {
      logger.nyi();
    }
  }


  virtual void doCONI(W36 iw, W36 ea) {
    if (logger.dte) logger.s << " ; DTE CONI";
  }


  // TTY handlers and stuff
  static void consoleIOLoop() {
    struct pollfd polls[] = {
      {.fd=ttyFD, .events=POLLIN, .revents=0},
      {.fd=fromIOLoopFD, .events=POLLIN, .revents=0},
    };

    while (true) {
      int st = poll(polls, sizeof(polls) / sizeof(polls[0]), -1);

      if (st < 0) {
	perror("Error polling for console I/O");
	continue;
      }

      if ((polls[0].revents & POLLIN) != 0) {
	char buf[64];
	int st = read(ttyFD, buf, sizeof(buf));
	if (st < 0) throw runtime_error("Error in console TTY read()");

	// Check for "out of band" attempt to exit emulator (control-\).
	for (int k=0; k < st; ++k) {

	  if (buf[k] == 0x1C) {
	    cerr << "[control-\\]" << logger.endl << flush;
	    raise(SIGINT);
	  }
	}

	for (int k=0; k < st; ++k) {

	  // Don't send control-\ to KL.
	  if (buf[k] == 0x5C - 0x20) continue;

	  // Lamely sleep up to 10ms until KL grabs the previous char
	  if (stateP->eptP->DTEKLNotReadyForChar) usleep(100);

	  stateP->eptP->DTEto10Arg = buf[k];
	  stateP->eptP->DTEKLNotReadyForChar = W36::allOnes;

	  // XXX ring the 10's doorbell here
	}
      }

      // Handle messages from our parent thread (usually we're just
      // told to fuck off and die).
      if ((polls[1].revents & POLLIN) != 0) {
	return;			// XXX for now just fuck off on any pipe data
      }
    }
  }


  // reset tty - useful also for restoring the terminal when this
  // process wishes to temporarily relinquish the tty.
  static void resetTTY() {
    /* flush and reset */
    if (isRaw) setNormal();
  }


  static void setNormal() {
    tcsetattr(ttyFD, TCSAFLUSH, &origTermios);
    isRaw = false;
  }


// Put terminal in Raw mode - see termios(3) for modes.
  static void setRaw() {
    struct termios raw{origTermios};
    cfmakeraw(&raw);

    // Put terminal in Raw mode after flushing
    if (tcsetattr(ttyFD, TCSAFLUSH, &raw) < 0) throw runtime_error("can't set TTY Raw mode");

    isRaw = true;
  }
};
