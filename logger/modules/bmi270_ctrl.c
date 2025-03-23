
#include <nuttx/config.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <sched.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <nuttx/i2c/i2c_master.h>
#include <pthread.h>

#include <nuttx/arch.h>
#include <arch/board/board.h>
#include <arch/chip/pin.h>

#include "bmi270lib/i2c_common.h"
#include "bmi270lib/i2c_bmi270.h"

#include "bmi270_ctrl.h"

pthread_mutex_t data_mutex = PTHREAD_MUTEX_INITIALIZER;
IMUData data_stack[IMU_DATA_STACK_SIZE];
IMUData imu_sensor_bias;
int data_stack_pos = 0;

void *thread_imu_bmi270_main(void *arg)
{
  int fd;
  int ret;
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
    return NULL;
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

  while (1)
  {
    ret = exec_dequeue_fifo(&bmi270);
    if (ret < 0)
    {
      printf("ERROR: Failed to Dequeue: %d\n", ret);
      goto error_on_using_bmi270;
    }

    if (data_stack_pos < IMU_DATA_STACK_SIZE)
    {

      ret = get_latest_acc(&acc_data, &bmi270);
      ret = get_latest_gyr(&gyr_data, &bmi270);

      pthread_mutex_lock(&data_mutex);
      data_stack[data_stack_pos].ax = acc_data.x;
      data_stack[data_stack_pos].ay = acc_data.y;
      data_stack[data_stack_pos].az = acc_data.z;
      data_stack[data_stack_pos].roll = gyr_data.x;
      data_stack[data_stack_pos].pitch = gyr_data.y;
      data_stack[data_stack_pos].yaw = gyr_data.z;

      data_stack_pos++;
      pthread_mutex_unlock(&data_mutex);
    }

    /* -- WAIT 50ms -- */

    waittime.tv_sec = 0;
    waittime.tv_nsec = IMU_MEASUREMENT_INTERVAL_MS * 1000 * 1000;
    nanosleep(&waittime, NULL);
  }

error_on_using_bmi270:

  /** close I2C bus */

  close(fd);

  /** init bmi270 */

  fini_bmi270(&bmi270);

  return NULL;
}

int read_bmi270(void)
{

  pthread_mutex_lock(&data_mutex);
  for (int i = 0; i < data_stack_pos; i++)
  {
    printf("IMU: ax = %.2f, ay = %.2f, az = %.2f, roll = %.2f, pitch = %.2f, yaw = %.2f\n",
           data_stack[i].ax,
           data_stack[i].ay,
           data_stack[i].az,
           data_stack[i].roll,
           data_stack[i].pitch,
           data_stack[i].yaw);
  }

  data_stack_pos = 0;
  pthread_mutex_unlock(&data_mutex);
  return 0;
}

int calibrate_bmi270(void)
{

  while (data_stack_pos < IMU_CALIBRATION_STACK_SIZE)
  {
    sleep(1);
  }

  pthread_mutex_lock(&data_mutex);
  for (int i = 0; i < data_stack_pos; i++)
  {
    imu_sensor_bias.ax += data_stack[i].ax;
    imu_sensor_bias.ay += data_stack[i].ay;
    imu_sensor_bias.az += data_stack[i].az;
    imu_sensor_bias.roll += data_stack[i].roll;
    imu_sensor_bias.pitch += data_stack[i].pitch;
    imu_sensor_bias.yaw += data_stack[i].yaw;
  }
  imu_sensor_bias.ax /= data_stack_pos;
  imu_sensor_bias.ay /= data_stack_pos;
  imu_sensor_bias.az /= data_stack_pos;
  imu_sensor_bias.roll /= data_stack_pos;
  imu_sensor_bias.pitch /= data_stack_pos;
  imu_sensor_bias.yaw /= data_stack_pos;

  data_stack_pos = 0;
  pthread_mutex_unlock(&data_mutex);
  return 0;
}

// int main_bmi270(void)
// {
//   int fd;
//   int ret;
//   int total = 512;
//   struct timespec waittime;
//   axis_t acc_data;
//   axis_t gyr_data;
//   i2c_bmi270_t bmi270 = {0};

//   /* I2C confiuguration */

//   bmi270.i2c.fd = -1;
//   bmi270.i2c.i2c_addr = BMI270_I2C_ADDRESS;
//   bmi270.i2c.speed = I2C_SPEED;

//   /** open I2C bus */

//   fd = open(I2C_DEVNAME_FOR_BMI270, O_WRONLY);
//   if (fd < 0)
//   {
//     printf("ERROR: Failed to open %s: %d\n",
//            I2C_DEVNAME_FOR_BMI270, errno);
//     return -1;
//   }

//   bmi270.i2c.fd = fd;

//   /** init bmi270 */

//   ret = init_bmi270(&bmi270);
//   if (ret < 0)
//   {
//     printf("ERROR: Failed to initialize: %d\n", ret);
//     goto error_on_using_bmi270;
//   }

//   /* -- WAIT 250ms -- */

//   waittime.tv_sec = 0;
//   waittime.tv_nsec = 250 * 1000 * 1000;
//   nanosleep(&waittime, NULL);

//   /** mesurement */

//   for (int i = 0; i < total; i++)
//   {
//     ret = exec_dequeue_fifo(&bmi270);
//     if (ret < 0)
//     {
//       printf("ERROR: Failed to Dequeue: %d\n", ret);
//       goto error_on_using_bmi270;
//     }

//     ret = get_latest_acc(&acc_data, &bmi270);
//     ret = get_latest_gyr(&gyr_data, &bmi270);
//     printf("(%3d/%3d) ACC|%6f,%6f,%6f || GYR|%6f,%6f,%6f\n",
//            i, total - 1,
//            (acc_data.x * ACC_RANGE_G * CONST_G) / RESOLUTION,
//            (acc_data.y * ACC_RANGE_G * CONST_G) / RESOLUTION,
//            (acc_data.z * ACC_RANGE_G * CONST_G) / RESOLUTION,
//            (gyr_data.x * GYR_RANGE_DPS) / RESOLUTION,
//            (gyr_data.y * GYR_RANGE_DPS) / RESOLUTION,
//            (gyr_data.z * GYR_RANGE_DPS) / RESOLUTION);

//     /* -- WAIT 50ms -- */

//     waittime.tv_sec = 0;
//     waittime.tv_nsec = 50 * 1000 * 1000;
//     nanosleep(&waittime, NULL);
//   }

// error_on_using_bmi270:

//   /** close I2C bus */

//   close(fd);

//   /** init bmi270 */

//   fini_bmi270(&bmi270);

//   return ret;
// }