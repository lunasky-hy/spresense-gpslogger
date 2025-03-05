#include <nuttx/config.h>
#include <stdio.h>
#include "modules/connection.h"
#include "modules/gnss.h"

int main(int argc, FAR char *argv[])
{
  char imsi[16];

  // Connect LTE
  lte_staprocess(STATE_UNINITIALIZED, STATE_CONNECTED_PDN);
  get_imsi(imsi);
  printf("%s\n", imsi);

  gnss_start();

  lte_finprocess(STATE_CONNECTED_PDN, STATE_POWER_ON);

  return 0;
}
