#include <nuttx/config.h>
#include <stdio.h>
#include "modules/connection.h"
#include "modules/gnss.h"
#include "modules/bmi270/bmi270_spresense.h"

int imu_task()
{
  bmi270_dev_t g_bmi270;
  struct i2c_master_s *i2c = spresense_board_i2c_initialize(0); // 例: I2C0を初期化

  bmi270_begin(&g_bmi270, BMI270_I2C, i2c, 0x24);
}

int main(int argc, FAR char *argv[])
{
  char imsi[16];
  sigset_t mask;
  int gnss_fd;
  int count = 0;

  // Connect LTE
  lte_staprocess(STATE_UNINITIALIZED, STATE_CONNECTED_PDN);
  get_imsi(imsi);
  printf("%s\n", imsi);

  imu_task();

  // Start GNSS
  gnss_fd = gnss_initialize(&mask);
  if (gnss_fd < 0)
  {
    printf("gnss_initialize failed. %d\n", gnss_fd);
    lte_finprocess(STATE_CONNECTED_PDN, STATE_POWER_ON);
    return -1;
  }
  do
  {
    gnss_get(gnss_fd, &mask);
  } while (count++ < 100);
  gnss_stop(gnss_fd);
  gnss_finalize(gnss_fd, &mask);

  lte_finprocess(STATE_CONNECTED_PDN, STATE_POWER_ON);

  return 0;
}
