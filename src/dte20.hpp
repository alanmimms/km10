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
#include <signal.h>

using namespace std;

#include "word.hpp"
#include "device.hpp"
#include "logger.hpp"
#include "tsqueue.hpp"


struct DTE20: Device {

  DTE20(unsigned anAddr, KM10 &cpu);
  ~DTE20();

  void connect();
  void disconnect();


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
    endOfDiagRun = 003,
    endOfDiagPass = 004,
    ctyInput = 007,
    ctyOutput,
    enterSecondaryProtocol,
    enterPrimaryProtocol,
    setDateTimeInfo,
  };

  union MonitorCommand {

    struct ATTRPACKED {
      unsigned data: 8;
      unsigned fn: 8;
    };

    unsigned u;

    MonitorCommand(unsigned v) : u(v) {}
  };

  thread consoleIOThread;
  bool consoleIOThreadDone;
  bool isConnected;
  inline static int toIOLoopFD;
  inline static int fromIOLoopFD;

  inline static bool isRaw{false};
  inline static int ttyFD{-1};
  inline static struct termios origTermios{};

  ofstream console;
  string endl;

  inline static TSQueue<char> ctyQ;


  // I/O instruction handlers
  virtual void clearIO() override;
  virtual IResult doCONO(W36 iw, W36 ea) override;
  virtual IResult doCONI(W36 iw, W36 ea) override;

  // TTY handlers and stuff
  static void consoleIOLoop();

  // reset tty - useful also for restoring the terminal when this
  // process wishes to temporarily relinquish the tty.
  static void resetTTY();
  static void setNormal();

// Put terminal in Raw mode - see termios(3) for modes.
  static void setRaw();
};
