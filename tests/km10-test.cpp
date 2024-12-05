#include <assert.h>
#include <functional>

using namespace std;

#include <gtest/gtest.h>

#include "word.hpp"
#include "km10.hpp"
#include "km10-test.hpp"

#include "logger.hpp"


Logger logger{};


int main(int argc, char *argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}


void Logger::nyi(KM10 &km10, const string &context) {
  s << " [not yet implemented: " << context << "]";
  cerr << "Not yet implemented at " << km10.pc.fmtVMA() << endl;
  ADD_FAILURE();
}


void Logger::nsd(KM10 &km10, const string &context) {
  s << " [no such device]: " << context << "]";
  cerr << "No such device at " << km10.pc.fmtVMA() << endl;
  ADD_FAILURE();
}
