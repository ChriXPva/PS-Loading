#include "auxi.h"
#include <stdio.h>
#include <math.h>

void formatarTemperatura(char* buf, size_t len, float temp, bool valido) {
  if (!valido || isnan(temp)) {
    snprintf(buf, len, "FALHA");
  } else {
    snprintf(buf, len, "%.1fC", temp);
  }
}
