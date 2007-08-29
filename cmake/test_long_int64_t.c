// This file is used to test if long and int64_t are the same type.
// It will fail to compile if long and int64_t are not the same type.
#include <stdint.h>
int main() {
  long f(long a);
  int64_t f(int64_t a);
  return 0;
}
