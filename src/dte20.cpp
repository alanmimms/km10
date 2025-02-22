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
#include "km10.hpp"
#include "logger.hpp"
#include "tsqueue.hpp"
#include "dte20.hpp"
#include "iresult.hpp"


DTE20::DTE20(unsigned anAddr, KM10 &cpu)
  : Device(anAddr,
	   string("DTE20.") + to_string(anAddr & 3),
	   cpu,
	   true),
    protocolMode(SECONDARY),
    genericConditions(0),
    isConnected(false),
    console("/dev/tty"),
    endl{"\n"}
{
  ttyFD = open("/dev/tty", O_RDWR);
  if (ttyFD < 0) throw runtime_error("can't open /dev/tty");

  if (!isatty(ttyFD)) throw runtime_error("MUST be run on a terminal without redirection");

  // Save current termios settings to restore on exit
  int st = tcgetattr(ttyFD, &origTermios);
  if (st < 0) throw runtime_error("failed to tcgetattr for tty");

  /* register the tty reset with the exit handler */
  if (atexit(resetTTY) != 0) throw runtime_error("atexit: can't register tty reset");
}

DTE20::~DTE20() {
  disconnect();
}


void DTE20::connect() {
  setRaw();
  endl = "\r\n";

  int pipeFDs[2];
  int st = pipe(pipeFDs);
  if (st < 0) perror("Error creation console I/O pipe");

  fromIOLoopFD = pipeFDs[0];
  toIOLoopFD = pipeFDs[1];

  consoleIOThread = thread(&consoleIOLoop);
  isConnected = true;
}


void DTE20::disconnect() {

  if (isConnected) {
    consoleIOThreadDone = true;
    write(toIOLoopFD, "bye", 3);
    consoleIOThread.join();
    close(fromIOLoopFD);
    close(toIOLoopFD);
    isConnected = false;
    if (isRaw) setNormal();
    endl = "\n";
  }
}


// I/O instruction handlers
void DTE20::clearIO() {
  if (logger.dte) logger.s << "; DTE CLEAR IO";
}


unsigned DTE20::getConditions() {
  return genericConditions;
}


void DTE20::putConditions(unsigned v) {
  genericConditions = v;
}



IResult DTE20::doCONO(W36 iw, W36 ea) {
  putConditions(ea.rhu);
  CONOMask req(ea);
  if (logger.dte) logger.s << "; DTE CONO " << oct << ea;

  if (req.to11Doorbell) {
    char buf;

    MonitorCommand mc{km10.eptP->DTEto11Arg.rhu};

    if (logger.dte) logger.s << " to11DoorBell arg=" << oct << km10.eptP->DTEto11Arg.rhu
			     << " data=" << mc.data << " fn=" << mc.fn << endl;

    switch (mc.fn) {
    case clockControl:		// Control/read the 60Hz clock interrupt from FE.

      switch (mc.data) {
      case 000:
	cerr << "[Clock is now OFF]" << endl;
	break;

      case 001:
	cerr << "[Clock is now ON]" << endl;
	break;

      case 003:
	cerr << "[Reading FE 60Hz clock (NYI)]" << endl;
	break;

      default:
	cerr << "[unknown clockControl " << oct << mc.data << "]" << endl;
	break;
      }

      // Acknowledge the command. (rsxt20.l20:5980)
      km10.eptP->DTEMonitorOpComplete = W36(-2 & 0xFFFF);
      break;

    case ctyInputDirect:
    case ctyInput:

      if (ctyQ.isEmpty()) {
	//	  cerr << "ctyIn [empty]" << endl;
	km10.eptP->DTEto10Arg.rhu = 0;
      } else {
	buf = ctyQ.dequeue() & 0177;
	cerr << "ctyIn [got "
	     << hex << setw(2) << setfill('0')
	     << (int) buf << "]" << endl;
	km10.eptP->DTEto10Arg.rhu = buf;
      }

      // Acknowledge the command. (rsxt20.l20:5980)
      km10.eptP->DTEMonitorOpComplete = W36(-2 & 0xFFFF);
      break;

    default:
    case ctyForcedOutput:
    case ctyOutput:
      // In RSX-20F this is an ELSE case vs function numbers 13, 12, 11 (rsxt20.l20:6017).
      buf = mc.data;
      write(1, &buf, 1);

      // Acknowledge the command. (rsxt20.l20:5924)
      km10.eptP->DTEMonitorOpComplete = W36(-2 & 0xFFFF);

      // XXX needs to set to-10 doorbell...?
      break;

    case enterSecondaryProtocol:
      cerr << "DTE20 enter secondary protocol command with data " << mc.data << " (ignored)."
	   << endl;
      break;

    case enterPrimaryProtocol:
      cerr << "DTE20 enter primary protocol command with data " << mc.data << " (ignored)."
	   << endl;
      break;

    case getSwitches:
      km10.eptP->DTEto10Arg = km10.debugger.switches;
      cerr << "[Get switches " << km10.eptP->DTEto10Arg.fmt36() << "]" << endl;

      // Acknowledge the command. (rsxt20.l20:5980)
      km10.eptP->DTEMonitorOpComplete = W36(-2 & 0xFFFF);
      cerr << flush;
      break;

    case diagNotice:

      switch (mc.data) {
      case 005:			// Get clock default word
	cerr << "[Get default clock word (NYI)]" << endl;
	break;

      case 004:			// End of PASS
	cerr << "[End of diagnostic PASS]" << endl;
	break;

      case 003:
	cerr << "[End of diagnostic RUN]" << endl;
	break;

      case 002:			// End of PASS HALT
	cerr << "[End of diagnostic FAIL HALT]" << endl;
	km10.running = false;
	break;

      case 001:			// Program error
	cerr << "[Diagnostic PROGRAM ERROR]" << endl;
	km10.running = false;
	break;

      default:
	cerr << "[Unknown diagNotice status=" << oct << mc.data << "]" << endl;
	break;
      }

      // Acknowledge the command. (rsxt20.l20:5980)
      km10.eptP->DTEMonitorOpComplete = W36(-2 & 0xFFFF);
      cerr << flush;
      break;
    }
  } else {
    logger.nyi(km10);
  }

  return IResult::iNormal;
}


// TTY handlers and stuff
void DTE20::consoleIOLoop() {
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
      char buf;
      int st = read(ttyFD, &buf, sizeof(buf));
      if (st < 0) throw runtime_error("Error in console TTY read()");

      if (buf == 0x1C) {
	cerr << "[control-\\]\r\n" << flush;
	kill(getpid(), SIGINT);
      } else {
	cerr << "[" << setw(2) << setfill('0') << hex << (int) buf << "]\r\n" << flush;
	ctyQ.enqueue(buf);
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
void DTE20::resetTTY() {
  /* flush and reset */
  if (isRaw) setNormal();
}


void DTE20::setNormal() {
  tcsetattr(ttyFD, TCSAFLUSH, &origTermios);
  isRaw = false;
}


// Put terminal in Raw mode - see termios(3) for modes.
void DTE20::setRaw() {
  struct termios raw{origTermios};
  cfmakeraw(&raw);

  // Put terminal in Raw mode after flushing
  if (tcsetattr(ttyFD, TCSAFLUSH, &raw) < 0) throw runtime_error("can't set TTY Raw mode");

  isRaw = true;
}
