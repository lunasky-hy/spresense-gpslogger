
#include <nuttx/config.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <sched.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <nuttx/i2c/i2c_master.h>

#include <nuttx/arch.h>
#include <arch/board/board.h>
#include <arch/chip/pin.h>

#include "bmi270lib/i2c_common.h"
#include "bmi270lib/i2c_bmi270.h"

int bmi270_ctrl(void)
{
  int fd;
  int ret;
  int total = 512;
  struct timespec waittime;
  axis_t acc_data;
  axis_t gyr_data;
  i2c_bmi270_t bmi270 = {0};

  /* I2C confiuguration */

  bmi270.i2c.fd = -1;
  bmi270.i2c.i2c_addr = BMI270_I2C_ADDRESS;
  bmi270.i2c.speed = I2C_SPEED;

  /** open I2C bus */

  fd = open(I2C_DEVNAME_FOR_BMI270, O_WRONLY);
  if (fd < 0)
  {
    printf("ERROR: Failed to open %s: %d\n",
           I2C_DEVNAME_FOR_BMI270, errno);
    return -1;
  }

  bmi270.i2c.fd = fd;

  /** init bmi270 */

  ret = init_bmi270(&bmi270);
  if (ret < 0)
  {
    printf("ERROR: Failed to initialize: %d\n", ret);
    goto error_on_using_bmi270;
  }

  /* -- WAIT 250ms -- */

  waittime.tv_sec = 0;
  waittime.tv_nsec = 250 * 1000 * 1000;
  nanosleep(&waittime, NULL);

  /** mesurement */

  for (int i = 0; i < total; i++)
  {
    ret = exec_dequeue_fifo(&bmi270);
    if (ret < 0)
    {
      printf("ERROR: Failed to Dequeue: %d\n", ret);
      goto error_on_using_bmi270;
    }

    ret = get_latest_acc(&acc_data, &bmi270);
    ret = get_latest_gyr(&gyr_data, &bmi270);
    printf("(%3d/%3d) ACC|%6f,%6f,%6f || GYR|%6f,%6f,%6f\n",
           i, total - 1,
           (acc_data.x * ACC_RANGE_G * CONST_G) / RESOLUTION,
           (acc_data.y * ACC_RANGE_G * CONST_G) / RESOLUTION,
           (acc_data.z * ACC_RANGE_G * CONST_G) / RESOLUTION,
           (gyr_data.x * GYR_RANGE_DPS) / RESOLUTION,
           (gyr_data.y * GYR_RANGE_DPS) / RESOLUTION,
           (gyr_data.z * GYR_RANGE_DPS) / RESOLUTION);

    /* -- WAIT 50ms -- */

    waittime.tv_sec = 0;
    waittime.tv_nsec = 50 * 1000 * 1000;
    nanosleep(&waittime, NULL);
  }

error_on_using_bmi270:

  /** close I2C bus */

  close(fd);

  /** init bmi270 */

  fini_bmi270(&bmi270);

  return ret;
}