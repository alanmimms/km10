#include <string>

#include "logger.hpp"
#include "km10.hpp"

using namespace std;


////////////////////////////////////////////////////////////////
void Logger::nyi(KM10 *cpuP, const string &context) {
  s << " [not yet implemented: " << context << "]";
  cerr << "Not yet implemented at " << cpuP->pc.fmtVMA() << endl;
}


void Logger::nsd(KM10 *cpuP, const string &context) {
  s << " [no such device: " << context << "]";
  cerr << "No such device at " << cpuP->pc.fmtVMA() << endl;
}
