#define IMU_MEASUREMENT_INTERVAL_MS 50 // 50ms in nanoseconds
#define IMU_MAX_SAVING_SECONDS 30
#define IMU_DATA_STACK_SIZE (IMU_MAX_SAVING_SECONDS * 1000 / IMU_MEASUREMENT_INTERVAL_MS)
#define IMU_CALIBRATION_SECONDS 10
#define IMU_CALIBRATION_STACK_SIZE (IMU_CALIBRATION_SECONDS * 1000 / IMU_MEASUREMENT_INTERVAL_MS)

typedef struct
{
  float ax;
  float ay;
  float az;
  float roll;
  float pitch;
  float yaw;
} IMUData;

void *thread_imu_bmi270_main(void *arg);
int read_bmi270(void);