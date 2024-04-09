#include <stddef.h>
#include "kl10.h"
#include "acutest.h"


static void testMaskForBit(void) {
  W36 was, sb;

  for (int n = 0; n < 36; ++n) {
    sb = 0400000000000ull >> n;
    was = MaskForBit(n);
    TEST_CHECK(was == sb);
  }

  sb = 0ull;
  was = MaskForBit(999);
  TEST_CHECK(was == 0ull);

  sb = 0ull;
  was = MaskForBit(-999);
  TEST_CHECK(was == 0ull);

  sb = 0ull;
  was = MaskForBit(36);
  TEST_CHECK(was == 0ull);

  sb = 0ull;
  was = MaskForBit(-1);
  TEST_CHECK(was == 0ull);
}


static void testExtract(void) {
  W36 was, sb;

  for (int n = 0; n < 36; ++n) {
    was = Extract(MaskForBit(n), n, n);
    sb = 1ull;
    TEST_CHECK(was == sb);

    if (n > 0) {
      was = Extract(MaskForBit(n), 0, n - 1);
      sb = 0ull;
      TEST_CHECK(was == sb);
    } else if (n < 35) {
      was = Extract(MaskForBit(n), n + 1, 35);
      sb = 0ull;
      TEST_CHECK(was == sb);
    }
  }

  was = Extract(MaskForBit(0), 1, 35);
  sb = 0ull;
  TEST_CHECK(was == sb);

  was = Extract(MaskForBit(35), 35, 35);
  sb = 1ull;
  TEST_CHECK(was == sb);

  was = Extract(MaskForBit(35), 0, 34);
  sb = 0ull;
  TEST_CHECK(was == sb);
}


TEST_LIST = {
  {"MaskForBit", testMaskForBit},
  {"Extract", testExtract},
  {NULL, NULL},
};
