#ifndef BMI270_SPRENSE_H_
#define BMI270_SPRENSE_H_

/***************************************************************************/
/*        必要なヘッダファイルをインクルード        */
/****************************************************************************/
#include <nuttx/config.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>
#include <nuttx/i2c/i2c_master.h>
#include <nuttx/spi/spi.h>

/* Bosch BMI2 系ライブラリのヘッダ */
#include "bmi270.h"

/***************************************************************************/
/*        マクロ定義        */
/****************************************************************************/
/*! Earth's gravity in m/s^2 */
#define GRAVITY_EARTH (9.80665f)

/*! Access mode */
typedef enum
{
  BMI270_I2C = 0,
  BMI270_SPI,
} BMI270Mode;

/*! @name Structure to define accelerometer and gyroscope sensor axes and
 * sensor time for virtual frames
 */
struct bmi2_sens_axes_float
{
  float x;
  float y;
  float z;
  uint32_t virt_sens_time;
};

/*! @name Structure to define BMI2 sensor data */
struct bmi2_sens_float
{
  struct bmi2_sens_axes_float acc;
  struct bmi2_sens_axes_float gyr;
  uint8_t aux_data[BMI2_AUX_NUM_BYTES];
  uint32_t sens_time;
};

/*!
 * @brief デバイス構造体(ユーザが保持するワーク領域)
 *
 * Bosch 提供の `struct bmi2_dev` を内包し、I2C/SPI インターフェースや設定値などを
 * 拡張メンバに格納します。
 */
typedef struct
{
  /* Boschライブラリで使用するメインのデバイス構造体 */
  struct bmi2_dev dev;

  /* I2C/SPI のどちらを使うかを示す */
  BMI270Mode mode;

  /* 実際に使用するバス (I2CかSPIか) のハンドル */
  union
  {
    struct i2c_master_s *i2c; /*!< I2Cバスハンドル */
    struct spi_dev_s *spi;    /*!< SPIバスハンドル */
  } bus;

  /* I2C利用時のスレーブアドレス、または SPI 利用時のCS制御に使うIDなど */
  uint8_t addr;

  /* レンジ情報(加速度/ジャイロ) */
  uint8_t acc_range;
  uint16_t gyr_range;

} bmi270_dev_t;

#ifdef __cplusplus
extern "C"
{
#endif

  /***************************************************************************/
  /*        関数群 (Arduinoクラスのpublicメソッド相当)        */
  /****************************************************************************/

  /*!
   * @brief BMI270の初期化を行う C インターフェース
   *
   * @param[in,out] dev     : BMI270デバイス構造体
   * @param[in] mode        : BMI270_I2C または BMI270_SPI
   * @param[in] bus         : I2C の場合は `struct i2c_master_s*`、SPI の場合は `struct spi_dev_s*`
   * @param[in] addr_or_cs  : I2Cアドレス または SPIデバイスID/CS情報(運用方法により適宜)
   *
   * @return Result of API execution status
   *         0 -> Success
   *         < 0 -> Fail
   */
  int8_t bmi270_begin(bmi270_dev_t *dev,
                      BMI270Mode mode,
                      void *bus,
                      uint8_t addr_or_cs);

  struct i2c_master_s *spresense_board_i2c_initialize(int bus);

  /*!
   * @brief 加速度・ジャイロの生データを float (m/s^2, dps) で取得する
   */
  int8_t bmi270_get_sensor_float(bmi270_dev_t *dev,
                                 struct bmi2_sens_float *data);

#ifdef __cplusplus
}
#endif

#endif /* BMI270_SPRENSE_H_ */
