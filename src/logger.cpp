#include <string>

#include "logger.hpp"
#include "km10.hpp"

using namespace std;


////////////////////////////////////////////////////////////////
void Logger::nyi(KM10 &km10, const string &context) {
  s << " [not yet implemented: " << context << "]";
  cerr << "Not yet implemented at " << km10.pc.fmtVMA() << endl;
}


void Logger::nsd(KM10 &km10, const string &context) {
  s << " [no such device: " << context << "]";
  cerr << "No such device at " << km10.pc.fmtVMA() << endl;
}
