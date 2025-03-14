#include <nuttx/config.h>
#include <stdio.h>
#include "modules/connection.h"
#include "modules/gnss.h"
#include "modules/bmi270_ctrl.h"

int bmi270_connection(void)
{
  bmi270_ctrl();
  return 0;
}

int main(int argc, FAR char *argv[])
{
  char imsi[16];
  char send_buffer[512];
  sigset_t mask;
  int gnss_fd;
  int gnss_status;
  struct gnss_positiondata_s position_data;

  bmi270_connection();
  return 0;

  // Start GNSS
  gnss_fd = gnss_initialize(&mask);
  if (gnss_fd < 0)
  {
    printf("gnss_initialize failed. %d\n", gnss_fd);
    return -1;
  }

  // Connect LTE
  lte_staprocess(STATE_UNINITIALIZED, STATE_CONNECTED_PDN);
  get_imsi(imsi);
  printf("%s\n", imsi);

  do
  {
    printf("GNSS get\n");

    gnss_status = gnss_get(gnss_fd, &mask, &position_data);
    if (gnss_status == 0)
    {
      sprintf(send_buffer, "{\"lat\":%f,\"lng\":%f}", position_data.latitude, position_data.longitude);
      printf("%s\n", send_buffer);

      send2harvest(send_buffer);
      sleep(30);
    }

  } while (1);
  gnss_stop(gnss_fd);
  gnss_finalize(gnss_fd, &mask);

  lte_finprocess(STATE_CONNECTED_PDN, STATE_POWER_ON);

  return 0;
}
