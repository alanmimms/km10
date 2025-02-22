// TODO:
// * Add command capability to change flags.
// * Add history ring buffer for PC and a way to dump it.
#include <string>
#include <iostream>
#include <string_view>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <set>
#include <map>
#include <vector>
#include <functional>

#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <signal.h>

using namespace std;


#include "word.hpp"
#include "debugger.hpp"
#include "km10.hpp"
#include "dte20.hpp"
#include "pi.hpp"
#include "device.hpp"
#include "apr.hpp"


Debugger::Debugger(KM10 &aKM10)
  : km10(aKM10),
    prevLine("help"),
    lastAddr(0),
    globalSymbols{},
    localSymbols{},
    localInvisibleSymbols{},
    valueToSymbol{},
    verboseLoad(false),
    switches(W36(02000,0))
{}


static KM10 *cpuForHandlerP;

static void sigintHandler(int signum) {
  cerr << "[SIGINT handler]" << logger.endl << flush;
  cpuForHandlerP->running = false;
}


Debugger::DebugAction Debugger::debug() {
  string line;
  vector<string> words;
  const string dumpPrefix{">>>>>>>>>>>>>>>>>>>"};


  auto COMMAND = [&](const char *s1, const char *s2, auto handler) {
    string loweredCmd(words[0]);
    for (auto& c: loweredCmd) c = tolower(c);

    if (loweredCmd == s1 || (s2 != nullptr && loweredCmd == s2)) {
      handler();
    }
  };


  auto dumpAC = [&](int k) {
    cout << dumpPrefix
	 << oct << setfill(' ') << setw(2) << k << ": "
	 << dump(km10.AC[k], W36(k), true) << logger.endl;
  };


  auto handleBPCommand = [&](unordered_set<unsigned> &s) {

    if (words.size() == 1) {	// Display breakpoints

      for (auto bp: s) {

	if (bp > 0777777u) {
	  cout << ">>>" << W36(bp).fmtVMA() << logger.endl;
	} else {
	  cout << ">>>" << oct << bp << logger.endl;
	}
      }

    } else if (words.size() > 1) { // Set or clear breakpoints

      if (words[1] == "clear") {
	s.clear();
      } else {

	try {
	  // Convert as a signed value since negative numbers are
	  // addresses of breakpoints to be cleared.
	  long a = W36(words[1]);

	  if (a < 0) {
	    s.erase((unsigned) -a);
	  } else {
	    s.insert((unsigned) a);
	  }

	} catch (exception &e) {
	  cout << "[No such breakpoint was found]" << logger.endl;
	}
      }
    }
  };


  ////////////////////////////////////////////////////////////////
  // Put console back into normal mode
  km10.dte.disconnect();

  // We have to set this every time but we only install the signal
  // handler once for this process.
  cpuForHandlerP = &km10;
  static bool firstTime{true};

  if (firstTime) {
    signal(SIGINT, sigintHandler);

    cout << "[KM-10 debugger]" << logger.endl << flush;
    firstTime = false;
  }

  const string prefix{">>>"};
  const string prompt{prefix + " "};


  // While this stays `noop` we keep looping, reading, and executing
  // debugger commands.
  DebugAction action{noop};

  do {
    // Show next instruction to execute.
    cout << km10.fetchPC.fmtVMA() << ": " << dump(km10.iw, km10.pc);
    if (km10.inInterrupt) cout << " [EXC] ";

    cout << prompt << flush;
    getline(cin, line);

    if (line.length() == 0) {
      line = prevLine;
    } else {
      prevLine = line;
    }

    words.clear();

    for (stringstream ss(line); !ss.eof(); ) {
      string s;
      ss >> s;
      if (s.length() > 0) words.push_back(s);
    }

    // Handle commands that are just abbreviations of others here
    // with reparse.
    //
    // `@ [N]` --> `m @ [N]`.
    if (words[0] == "@") {
      vector<string> expansion = {"m", "@"};
      if (words.size() > 1) expansion.push_back(words[1]);
      words = move(expansion);
    }

    COMMAND("quit", "q", [&]() {
      km10.running = false;
      km10.restart = false;
      action = quit;
    });

    COMMAND("obp", nullptr, [&]() {handleBPCommand(km10.opBPs);});
    COMMAND("gbp", nullptr, [&]() {handleBPCommand(km10.addressGBPs);});
    COMMAND("pbp", nullptr, [&]() {handleBPCommand(km10.addressPBPs);});

    COMMAND("ac", nullptr, [&]() {

      if (words.size() > 1) {

	try {
	  int n = stoi(words[1], nullptr, 8);

	  if (n > 020) {
	    cout << "ERROR: AC number too large" << logger.endl;
	    return;
	  }

	  if (words.size() > 2) {
	    km10.acPutN(W36(words[2]), n);
	  } else {
	    dumpAC(n);
	    cout << logger.endl << flush;
	  }
	} catch (exception &e) {
	}
      } else {
	for (int k=0; k < 020; ++k) dumpAC(k);
	cout << logger.endl << flush;
      }
    });

    COMMAND("bp", "b", [&]() {handleBPCommand(km10.executeBPs);});

    COMMAND("mem", "m", [&]() {

      try {
	W36 a;
	int n = words.size() > 2 ? stoi(words[2], nullptr, 8) : 1;

	if (words.size() > 1) {
	  string lowered(words[1]);
	  for (auto& c: lowered) c = tolower(c);
	  a = (lowered == "pc") ? km10.pc :
	    (lowered == "@") ? W36(km10.iw.y) :
	    W36(words[1]);
	} else {
	  a = lastAddr;
	}

	for (int k=0; k < n; ++k) {
	  W36 w(km10.memGetN(a));
	  cout << dumpPrefix << a.fmtVMA() << ": " << dump(w, a, true) << logger.endl;
	  a = a + 1;
	}

	cout << logger.endl << flush;
	lastAddr = a;
	prevLine = "m";
      } catch (exception &e) {
      }
    });

    COMMAND("mset", nullptr, [&]() {

      if (words.size() <= 2) {
	cout << "Must specify both memory address and value to set it to."
	     << logger.endl << flush;
	return;
      }

      try {
	W36 a = words[1] == "pc" ? km10.pc : W36(words[1]);
	W36 v = W36(words[2]);
	km10.memPutN(v, a);
	lastAddr = a;
      } catch (exception &e) {
      }
    });

    COMMAND("restart", nullptr, [&]() {
      km10.restart = true;
      km10.running = false;
      action = restart;
    });

    COMMAND("step", "s", [&]() {

      if (words.size() == 1) {
	km10.nSteps = 1;
      } else {

	try {
	  km10.nSteps = words.size() > 1 ? stoi(words[1], nullptr, 8) : 1;
	} catch (exception &e) {
	  km10.nSteps = 1;
	}
      }

      km10.running = true;
      action = step;
    });

    COMMAND("show", nullptr, [&]() {

      if (words.size() == 1) {
	cout << "Must specify apr or flags" << logger.endl << flush;
      } else if (words.size() == 2) {

	if (words[1] == "apr") {
	  cout << prefix << km10.apr.aprState.toString() << logger.endl;
	} else if (words[1] == "pi") {
	  cout << prefix << km10.pi.piState.toString() << logger.endl;
	} else if (words[1] == "flags") {
	  cout << prefix << km10.flags.toString() << logger.endl;
	} else if (words[1] == "devs") {

	  for (auto [ioDev, devP]: Device::devices) {
	    if (devP->ioAddress == 0777777) continue;
	    cout << setw(10) << devP->name
		 << " ioAddr=" << W36(devP->ioAddress).fmt18()
		 << " intLevel=" << devP->intLevel
		 << " intPending=" << devP->intPending
		 << logger.endl;
	  }
	} else if (words[1] == "counters") {
	  double insPerSec = (double) km10.instructionCounter / (double) km10.runNS * 1.0e9;
	  cout << "Instructions: " << setw(12) << dec << km10.instructionCounter
	       << logger.endl
	       << "              " << fixed << setw(14) << setprecision(1)
	       << insPerSec << "/sec"
	       << logger.endl;
	} else {
	  cout << "Must specify apr or flags" << logger.endl;
	}
      }

      cout << flush;
    });

    COMMAND("log", "l", [&]() {

      if (words.size() == 1) {
	cout << prefix << "Logging to " << logger.destination << ": ";
	if (logger.ac) cout << " ac";
	if (logger.io) cout << " io";
	if (logger.pc) cout << " pc";
	if (logger.dte) cout << " dte";
	if (logger.ea) cout << " ea";
	if (logger.mem) cout << " mem";
	if (logger.load) cout << " load";
	if (logger.ints) cout << " ints";
	cout << logger.endl;
      } else if (words.size() >= 2) {

	if (words[1] == "off") {
	  logger.ac = logger.io = logger.pc = logger.dte =
	    logger.mem = logger.load = logger.ints = false;
	} else if (words[1] == "file") {
	  logger.logToFile(words.size() == 3 ? words[2] : "km10.log");
	} else if (words[1] == "tty") {
	  logger.logToTTY();
	} else if (words[1] == "all") {
	  logger.ac = logger.io = logger.pc = logger.dte =
	    logger.mem = logger.load = logger.ints = true;
	} else {
	  if (words[1] == "ac") logger.ac = !logger.ac;
	  if (words[1] == "io") logger.io = !logger.io;
	  if (words[1] == "pc") logger.pc = !logger.pc;
	  if (words[1] == "dte") logger.dte = !logger.dte;
	  if (words[1] == "mem") logger.mem = !logger.mem;
	  if (words[1] == "load") logger.load = !logger.load;
	  if (words[1] == "ints") logger.ints = !logger.ints;
	}
      }

      cout << flush;
    });

    COMMAND("pc", nullptr, [&]() {

      if (words.size() == 1) {
	cout << prefix
	     << "Flags:  " << km10.flags.toString() << logger.endl
	     << "   PC: " << km10.pc.fmtVMA() << logger.endl;
      } else {

	try {
	  km10.pc = W36(words[1]);
	} catch (exception &e) {
	}
      }

      cout << flush;
      action = pcChanged;
    });

    COMMAND("pchist", nullptr, [&]() {
      size_t count;

      if (words.size() == 1) {
	count = pcHistorySize;
      } else {

	try {
	  count = stol(words[1], nullptr);
	} catch (exception &e) {
	}
      }

      vector<W36> hist = pcRing.mostRecent(count);
      static const int nPL = 8;		// # PC history elements per line of output
      size_t countDown = hist.size() - 1;

      // Iterate through the vector and print
      for (size_t i=0; i < hist.size(); i += nPL) {
	// Print the line prefix (starting count for this line)
	cout << setw(6) << countDown << ": ";

	// Print up to nPL elements for this line
	for (size_t j=0; j < nPL && i + j < hist.size(); ++j) {
	  cout << hist[i + j].fmtVMA();
	  if (j < nPL - 1 && (i + j + 1) < hist.size()) cout << " ";
	}

	// Move to the next line
	cout << logger.endl;

	// Decrement the count by nPL
	countDown = (countDown >= nPL) ? countDown - nPL : 0;
      }

      cout << logger.endl << flush;
    });

    COMMAND("continue", "c", [&]() {
      km10.nSteps = 0;
      km10.running = true;
      action = run;
    });

    COMMAND("stats", nullptr, [&]() {
      cout << prefix << "Instructions: " << km10.nInsns << logger.endl << flush;
    });

    COMMAND("switch", "sw", [&]() {

      if (words.size() == 1) {
	cout << prefix
	     << "Switches: " << switches.fmt36() << logger.endl;
      } else {

	try {
	  switches = W36(words[1]);
	} catch (exception &e) {
	}
      }

      cout << flush;
      action = pcChanged;
    });

    COMMAND("help", "?", [&]() {
      cout << R"(
  gbp [A]       Set address breakpoint after GET access to address A or list breakpoints.
                Use -A to remove an existing address breakpoint or 'clear' to clear all.
  pbp [A]       Set address breakpoint after PUT access to address A or list breakpoints.
                Use -A to remove an existing address breakpoint or 'clear' to clear all.
  obp op        Breakpoint on next instruction with 'op' as its opcode.
  ac [N [V]]    Dump all ACs or, if N specified, a single AC.
                If V is specified, set AC's value.
  b,bp [A]      Set breakpoint before execution of address A or display list of breakpoints.
                Use -A to remove existing breakpoint or 'clear' to clear all breakpoints.
  c,continue    Continue execution at current PC.
  ?,help        Display this help.
  l,log [ac|io|pc|dte|ea|mem|load|ints|off|all]
                Display logging flags, toggle one, or turn all on or off.
  l,log file [FILENAME]
                Log to FILENAME or 'km10.log' if not specified (overwriting).
  l,log tty     Log to console.
  m,mem A [N]   Dump N (octal) words of memory starting at A (octal).
                A can be 'pc' or '@' for Y. Use '@' command as equivalent to 'm @'.
  @ [N]         Dump N (octal) words starting at current instruction's Y.
  mset A V      Set memory address A to value V. A can be 'pc'.
  pc [N]        Dump PC and flags, or if N is specified set PC to N (octal).
  pchist [N]    Dump all or some (N) most recent fetch-PC value history in FIFO order.
  restart       Reset and reload as if started from scratch again.
  s,step N      Step N (octal) instructions at current PC.
  show apr|pi|flags|devs|counters
                Display APR, PI state, program flags, or device list.
  stats         Display emulator statistics.
  switch,sw [N] Display or set the console switches.
  q,quit        Quit the KM10 simulator.
)"sv.substr(1);	// "" (this helps Emacs parse the syntax properly)
    });
  } while (action == noop);

  // Restore console to raw mode for FE input/output.
  km10.dte.connect();
  return action;
}


// Dump a string represenation of an instruction word in numeric and
// disassembled, symbolic form, label derived from the specified PC.
string Debugger::dump(W36 iw, W36 pc, bool showCharForm) {
  ostringstream s;

  s << iw.fmt36();

  s << oct << setfill('0')
    << " " << setw(3) << iw.op
    << " " << setw(2) << iw.ac
    << " " << setw(1) << iw.i
    << " " << setw(2) << iw.x
    << " " << setw(6) << iw.y;

  s << "  " << left << setfill(' ') << setw(10);

  if (auto it = valueToSymbol.find(pc); it != valueToSymbol.end()) {
    s << (it->second + ":");
  } else {
    s << " ";
  }

  s << iw.disasm(this);

  if (showCharForm) {
    s << "  " << "/" << iw.sixbit() << "/";

    if (iw.u < 0400 && iw.u > ' ')
      s << "  " << "'" << (char) iw.u << "'";
    else
      s << "  " << "'" << iw.ascii() << "'";
  }

  return s.str();
}


// Open and mmap a file full of 9-byte (72-bit) chunks. Slurp two
// successive 36-bit words from each and deposit them into our 36-bit
// data array, which we return.
static vector<W36> readFileW36s(const char* filename) {
  // Open the file.
  int fd = open(filename, O_RDONLY);
  if (fd < 0) {
    throw runtime_error("Failed to open file");
  }

  // Get the file size.
  struct stat st;
  if (fstat(fd, &st) < 0) {
    close(fd);
    throw runtime_error("Failed to get file size");
  }
  size_t fileSize = static_cast<size_t>(st.st_size);

  // Ensure file size is divisible by 9.
  if (fileSize % 9 != 0) {
    close(fd);
    throw runtime_error("File size is not divisible by 9");
  }

  // Map the file into memory.
  void* fileP = mmap(nullptr, fileSize, PROT_READ, MAP_PRIVATE, fd, 0);
  close(fd);	    // We can close the file descriptor after we mmap.
  if (fileP == MAP_FAILED) {
    throw runtime_error("Failed to mmap file");
  }

  // Calculate the number of 36-bit values.
  size_t nWords = (fileSize / 9) * 2;

  // Create the output vector and pre-allocate capacity.
  vector<W36> output;
  output.reserve(nWords);

  const uint8_t* inputBytes = static_cast<const uint8_t*>(fileP);

  for (size_t i=0; i < fileSize; i += 9) {
    // Extract 9 bytes.
    const uint8_t *wordP = inputBytes + i;

    // Build up the first 36-bit value.
    uint64_t value1 = static_cast<uint64_t>(wordP[0]) << 28 |
                      static_cast<uint64_t>(wordP[1]) << 20 |
                      static_cast<uint64_t>(wordP[2]) << 12 |
                      static_cast<uint64_t>(wordP[3]) << 4 |
                      static_cast<uint64_t>(wordP[4]) >> 4;

    // Build up the second 36-bit value.
    uint64_t value2 = static_cast<uint64_t>(wordP[4] & 0x0F) << 32 |
                      static_cast<uint64_t>(wordP[5]) << 24 |
                      static_cast<uint64_t>(wordP[6]) << 16 |
                      static_cast<uint64_t>(wordP[7]) << 8 |
                      static_cast<uint64_t>(wordP[8]);

    // Add them to the output vector.
    output.emplace_back(value1);
    output.emplace_back(value2);
  }

  // Unmap the file, effectively closing it finally as well.
  if (munmap(fileP, fileSize) < 0) {
    throw runtime_error("Failed to unmap file");
  }

  return output;
}


struct Radix50Word {

  union {

    struct ATTRPACKED {
      unsigned rad50: 32;
      unsigned format: 4;
    };
    
    uint64_t u;
  };


  Radix50Word(unsigned aFormat, unsigned aRad50)
    : rad50(aRad50),
      format(aFormat)
  { }


  Radix50Word(W36 w)
    : rad50(w.u & W36::rMask(32)),
      format(w.u >> 32)
  { }


  // Human octal readable version of bits 0..3
  inline unsigned flavor() const {return (unsigned) format << 2; }

  static inline const char RADIX50[] = " 0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ$.%";


  string toString() const {
    unsigned v = rad50;
    vector<char> s;
    s.reserve(16);

    for (int k=7; k >= 0 && v != 0; --k) {
      s.insert(s.begin(), RADIX50[v % 40]);
      v /= 40;
    }

    string str(s.begin(), s.end());
    return str;
  }


  // For googletest stringification
  friend void PrintTo(const Radix50Word& w, ostream* os) {
    *os << w.toString();
  }
};


void Debugger::loadWord(unsigned addr, W36 value)  {
  if (verboseLoad) cout << W36(addr).fmtVMA() << ": " << value.fmt36() << endl;
}


// Load a listing file (*.REL) to get symbol definitions for symbolic
// debugging.
void Debugger::loadREL(const char *fileNameP) {
  vector<W36> rel = readFileW36s(fileNameP);
  size_t relSize = rel.size();
  unsigned rw = 0;     // Relocation word we're working with right now
  unsigned blockType;
  unsigned sc;
  unsigned x;
  unsigned relX;

  map<unsigned, function<void()>> blockTypeHandler{};


  // 000: Ignored
  blockTypeHandler[000] = [&]() {
    x += sc;
  };


  // 001 Code
  blockTypeHandler[001] = [&]() {
    // If "address word" has binary 1100 in top four bits, it's a
    // symbol we have to relocate address by. Otherwise, I guess we
    // don't relocate.
    W36 symOrAddr{rel[x++]};
    Radix50Word codeSymbol{symOrAddr};
    unsigned loadAddr;

    if (verboseLoad) {

      if ((symOrAddr >> 18) != 0) {
	cout << "Symbol " << codeSymbol.toString()
	     << " flavor=" << codeSymbol.flavor()
	     << endl;
      }
    }

    // If it's a symbol, we use its value plus the word following as
    // the load address.
    if ((symOrAddr >> 32) == 014) {
      string symbol = codeSymbol.toString();
      loadAddr = globalSymbols[symbol];
      if (verboseLoad) cout << "Offset by " << symbol
			    << "=" << W36(loadAddr).fmtVMA() << endl;
      loadAddr += rel[x++].u;	// Add the offset
    } else {
      loadAddr = symOrAddr;	// Get the load addres
      ++x;
    }

    if (verboseLoad) cout << "loadAddr=" << W36(loadAddr).fmtVMA() << endl;

    for (unsigned k=x; k - relX < sc; ++k, ++loadAddr, ++x) {
      loadWord(loadAddr, rel[k]);
    }
  };


  blockTypeHandler[002] = [&]() {

    while (x - relX < sc) {
      unsigned startX = x;
      Radix50Word sym{rel[x++]};
      W36 value = W36{rel[x++]};

      switch (sym.flavor()) {
      case 000:			// ILLEGAL
      default:
	cout << "[" << oct << startX << "] ILLEGAL Radix50Word symbol flavor "
	     << oct << sym.flavor() << endl;
	break;

      case 004:			// Global symbol
	if (verboseLoad) {
	  cout << "[" << oct << startX << "] Define global " << sym.toString()
	       << " as " << value.fmt36() << endl;
	}

	globalSymbols[sym.toString()] = value;
	break;

      case 010:			// Local symbol
      case 014:			// Block name
	if (verboseLoad) {
	  cout << "[" << oct << startX << "] Define local " << sym.toString()
	       << " as " << value.fmt36() << endl;
	}

	localSymbols[sym.toString()] = value;
	break;

      case 050:			// DDT invisible local symbol
	if (verboseLoad) {
	  cout << "[" << oct << startX << "] Define invisible local " << sym.toString()
	       << " as " << value.fmt36() << endl;
	}

	localInvisibleSymbols[sym.toString()] = value;
	break;

      case 024:			// Partially defined global symbol
	break;
      }
    }
  };
  

  // HISEG
  blockTypeHandler[003] = [&]() {

    for (unsigned k=0; k <= sc; ++k) {
      Radix50Word sym{rel[x++]};
      cout << "Entry " << sym.toString() << endl;
    }
  };


  // Entry symbols
  blockTypeHandler[004] = [&]() {

    for (unsigned k=0; k < sc; ++k) {
      Radix50Word sym{rel[x++]};
      if (verboseLoad) cout << "Entry " << sym.toString() << endl;
    }
  };


  // End of program
  blockTypeHandler[005] = [&]() {
    // we ignore this
  };


  // Program name
  blockTypeHandler[006] = [&]() {
    Radix50Word sym{rel[x++]};
    if (verboseLoad) cout << "Program " << sym.toString();
    uint64_t w = rel[x++].u;
    unsigned cpu = w >> 30;
    unsigned compiler = (w >> 18) & 07777ul;
    unsigned lengthOfBlankCommon = w & W36::halfOnes;

    if (verboseLoad) {
      if (cpu & 010) cout << " KS10";
      if (cpu & 004) cout << " KL10";
      if (cpu & 002) cout << " KI10";
      if (cpu & 001) cout << " KA10";

      if (compiler == 000) cout << " Unknown";
      if (compiler == 001) cout << " (Not used)";
      if (compiler == 002) cout << " COBOL-68";
      if (compiler == 003) cout << " ALGOL";
      if (compiler == 004) cout << " NELIAC";
      if (compiler == 005) cout << " PL/I";
      if (compiler == 006) cout << " BLISS";
      if (compiler == 007) cout << " SAIL";

      if (compiler == 010) cout << " FORTRAN";
      if (compiler == 011) cout << " MACRO";
      if (compiler == 012) cout << " FAIL";
      if (compiler == 013) cout << " BCPL";
      if (compiler == 014) cout << " MIDAS";
      if (compiler == 015) cout << " SIMULA";
      if (compiler == 016) cout << " COBOL-7";
      if (compiler == 017) cout << " COBOL";

      if (compiler == 020) cout << " BLISS-36";
      if (compiler == 021) cout << " BASIC";
      if (compiler == 022) cout << " SITGO";
      if (compiler == 023) cout << " (Reserved)";
      if (compiler == 024) cout << " PASCAL";
      if (compiler == 025) cout << " JOVIAL";
      if (compiler == 026) cout << " ADA";

      cout << " common=" << right << oct << lengthOfBlankCommon << endl;
    }
  };


  // Start
  blockTypeHandler[007] = [&]() {
    W36 startAddr{rel[x++]};

    if (verboseLoad) cout << "[" << oct << x << "] Start: " << startAddr.fmtVMA();

    if (sc >= 2) {
      Radix50Word sym{rel[x++]};

      if (verboseLoad) {
	if (sym.flavor() == 060) cout << " offset by " << sym.toString();
      }
    }

    if (verboseLoad) cout << endl;
  };


  cout << "[loading " << fileNameP << "  "
       << right << oct << relSize << " words]"
       << endl;

  for (x=0; x < relSize; ) {
    unsigned startX = x;

    blockType = rel[x].lhu;

    // Length of the block excluding header word and relocation word.
    sc = rel[x].rhu;

    ++x;			// Consume block header

    // Relocation word.
    rw = rel[x++].u;

    // Offset of where our data words start.
    relX = x;

    if (verboseLoad) {
      cout << "[" << right << oct << startX << "] Block " << blockType
	   << " shortCount=" << sc
	   << " relocationWord=" << W36(rw).fmt36() << endl;
    }

    auto it = blockTypeHandler.find(blockType);

    if (it != blockTypeHandler.end()) {
      it->second();		// Invoke the handler
    } else {
      if (verboseLoad) cout << "blockType " << right << oct << blockType
			    << " not defined" << endl;
      return;			// FOR NOW
      x += sc;			// Try to skip the block
    }
  }

  // Build our value-to-symbol reverse lookup table from all symbols
  // we know about.
  for (const auto &pair: globalSymbols) {
    valueToSymbol[pair.second] = pair.first;
  }

  for (const auto &pair: localSymbols) {
    valueToSymbol[pair.second] = pair.first;
  }

#if 0
  for (const auto &pair: localInvisibleSymbols) {
    valueToSymbol[pair.second] = pair.first;
  }
#endif

  // Dump our results proudly to a log file.
  static const string bannerDash(30, '-');
  static const string symLogFileName{"rel-symbols.log"};
  ofstream symLog(symLogFileName);

  if (globalSymbols.size() != 0) {
    symLog << endl << bannerDash << "GLOBAL SYMBOLS" << bannerDash << endl;

    for (const auto &pair: globalSymbols) {
      symLog << pair.first << ": " << pair.second.fmt36() << endl;
    }
  }

  if (localSymbols.size() != 0) {
    symLog << endl << bannerDash << "LOCAL SYMBOLS" << bannerDash << endl;

    for (const auto &pair: localSymbols) {
      symLog << pair.first << ": " << pair.second.fmt36() << endl;
    }
  }

  if (localInvisibleSymbols.size() != 0) {
    symLog << endl << bannerDash << "LOCAL INVISIBLE SYMBOLS" << bannerDash << endl;

    for (const auto &pair: localInvisibleSymbols) {
      symLog << pair.first << ": " << pair.second.fmt36() << endl;
    }
  }

  if (valueToSymbol.size() != 0) {
    symLog << endl << bannerDash << "VALUES TO SYMBOLS" << bannerDash << endl;

    for (const auto &pair: valueToSymbol) {
      symLog << pair.first.fmt36() << "=" << pair.second << endl;
    }
  }

  symLog.close();
  cout << "[symbols dumped to " << symLogFileName << "]" << endl;

  cout << "[done]" << endl << flush;
}


string Debugger::symbolicForm(W36 w) {
  static const int64_t maxDelta = 01000;
  auto it = valueToSymbol.upper_bound(w.u);

  // Find and return the string (+ offset) for a symbol whose value is
  // no more than maxDelta greater than w. If no such entry exists,
  // fall through and return just the octal value.
  if (it != valueToSymbol.begin()) {
    --it;			// Seek to previous entry

    int64_t delta = w.u - it->first.u;

    if (delta == 0) {
      return it->second;
    } else if (delta < maxDelta) {
      stringstream ss;
      ss << it->second << "+" << oct << right << delta;
      return ss.str();
    }
  }

  if (w.lhu != 0)
    return w.fmt36();
  else
    return w.fmt18();
}


Debugger::RingBuffer::RingBuffer()
  : head{0}, full{false}
{}


void Debugger::RingBuffer::add(const W36& element) {
  buffer[head] = element;
  head = (head + 1) % pcHistorySize;
  if (head == 0) full = true;
}


std::optional<W36> Debugger::RingBuffer::peek() const {
  if (head == 0 && !full) return std::nullopt; // Buffer is empty
  size_t lastIndex = (head == 0) ? pcHistorySize - 1 : head - 1;
  return buffer[lastIndex];
}


std::vector<W36> Debugger::RingBuffer::mostRecent(size_t n) const {
  size_t count = full ? pcHistorySize : head;
  size_t nToReturn = std::min(n, count);
  size_t start = (count >= n) ? (head + pcHistorySize - nToReturn) % pcHistorySize : 0;

  std::vector<W36> result;
  for (size_t i = 0; i < nToReturn; ++i) {
    result.push_back(buffer[(start + i) % pcHistorySize]);
  }

  return result;
}

void Debugger::RingBuffer::print(std::ostream& s, size_t count) const {
  if (count > pcHistorySize) count = 0;
  if (count == 0) count = full ? pcHistorySize : head;
  size_t start = full ? head - count : 0;

  for (size_t i = 0; i < count; ++i) {
    s << buffer[(start + i) % pcHistorySize] << " ";
  }

  s << std::endl;
}
