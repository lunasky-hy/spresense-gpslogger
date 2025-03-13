/***************************************************************************/
/*!
 * @file bmi270_spresense.c
 * @brief Bosch BMI270 を Spresense SDK (NuttX) で利用するためのラッパ実装サンプル
 *
 ****************************************************************************/
#include "bmi270.h"
#include "bmi270_spresense.h"

#include <nuttx/irq.h> /* 割り込み制御など */
#include <sys/time.h>
#include <unistd.h> /* usleepなど */

#include <nuttx/i2c/i2c_master.h>
#include <nuttx/i2c/i2c_slave.h>

/***************************************************************************/
/*     前方宣言(コールバック関数・内部ユーティリティ関数)
/***************************************************************************/

/*!
 * @brief I2C読み出しコールバック
 */
static int8_t bmi2_i2c_read(uint8_t reg_addr, uint8_t *data, uint16_t len, void *intf_ptr);

/*!
 * @brief I2C書き込みコールバック
 */
static int8_t bmi2_i2c_write(uint8_t reg_addr, const uint8_t *data, uint16_t len, void *intf_ptr);

/*!
 * @brief SPI読み出しコールバック
 */
static int8_t bmi2_spi_read(uint8_t reg_addr, uint8_t *data, uint16_t len, void *intf_ptr);

/*!
 * @brief SPI書き込みコールバック
 */
static int8_t bmi2_spi_write(uint8_t reg_addr, const uint8_t *data, uint16_t len, void *intf_ptr);

/*!
 * @brief マイクロ秒ウェイト関数
 */
static void bmi2_delay_us(uint32_t period);

/*!
 * @brief LSB値を m/s^2 に変換するユーティリティ
 */
static float lsb_to_mps2(bmi270_dev_t *dev, int16_t val);

/*!
 * @brief LSB値を deg/s に変換するユーティリティ
 */
static float lsb_to_dps(bmi270_dev_t *dev, int16_t val);

/***************************************************************************/
/*     メインAPI実装
/***************************************************************************/

int8_t bmi270_begin(bmi270_dev_t *dev, BMI270Mode mode, void *bus, uint8_t addr_or_cs)
{
  int8_t rslt;

  if (!dev || !bus)
  {
    return BMI2_E_NULL_PTR;
  }

  /* デバイス構造体の初期化 */
  dev->mode = mode;
  dev->addr = addr_or_cs;
  dev->acc_range = 2;    /* とりあえず仮の初期値 */
  dev->gyr_range = 2000; /* とりあえず仮の初期値 */

  /* bmi2_dev 構造体の設定 */
  dev->dev.intf_ptr = dev; /* コールバックで参照するユーザコンテキスト */
  dev->dev.delay_us = bmi2_delay_us;
  dev->dev.config_file_ptr = NULL; /* 既定のコンフィグファイルを使用 */
  dev->dev.read_write_len = 32;    /* I2Cトランザクション最大長などの目安 */

  if (mode == BMI270_I2C)
  {
    /* I2C 使用 */
    dev->bus.i2c = (struct i2c_master_s *)bus;
    dev->dev.intf = BMI2_I2C_INTF;
    dev->dev.read = bmi2_i2c_read;
    dev->dev.write = bmi2_i2c_write;
  }
  else
  {
    /* SPI 使用 */
    dev->bus.spi = (struct spi_dev_s *)bus;
    dev->dev.intf = BMI2_SPI_INTF;
    dev->dev.read = bmi2_spi_read;
    dev->dev.write = bmi2_spi_write;
  }

  /* Boschライブラリの初期化関数呼び出し */
  rslt = bmi270_init(&dev->dev);
  return rslt;
}

/* ボード固有のI2C初期化関数サンプル */
struct i2c_master_s *spresense_board_i2c_initialize(int bus)
{
  /* busは使用したいI2Cのバス番号 (0や1など) */
  struct i2c_master_s *i2c = up_i2cinitialize(bus);
  if (!i2c)
  {
    _err("ERROR: Failed to get I2C bus %d\n", bus);
    return NULL;
  }

  /* 必要に応じて、周波数やアドレスの初期設定を行う */
  I2C_SETFREQUENCY(i2c, 400000);

  return i2c;
}

int8_t bmi270_get_sensor_float(bmi270_dev_t *dev,
                               struct bmi2_sens_float *data)
{
  if (!dev || !data)
  {
    return BMI2_E_NULL_PTR;
  }

  /* Boschライブラリの標準データ取得API */
  struct bmi2_sens_data sensor_data;
  int8_t rslt = bmi2_get_sensor_data(&sensor_data, &dev->dev);
  if (rslt != BMI2_OK)
  {
    return rslt;
  }

  /* LSB->float変換 */
  data->acc.x = lsb_to_mps2(dev, sensor_data.acc.x);
  data->acc.y = lsb_to_mps2(dev, sensor_data.acc.y);
  data->acc.z = lsb_to_mps2(dev, sensor_data.acc.z);

  data->gyr.x = lsb_to_dps(dev, sensor_data.gyr.x);
  data->gyr.y = lsb_to_dps(dev, sensor_data.gyr.y);
  data->gyr.z = lsb_to_dps(dev, sensor_data.gyr.z);

  return rslt;
}

/***************************************************************************/
/*   以下、内部コールバック・ユーティリティ実装
/***************************************************************************/
static int8_t bmi2_i2c_read(uint8_t reg_addr, uint8_t *data, uint16_t len, void *intf_ptr)
{
  bmi270_dev_t *priv = (bmi270_dev_t *)intf_ptr;
  if (!priv || !data || len == 0)
  {
    return BMI2_E_NULL_PTR;
  }

  /* I2C レジスタ読み込み: (1)書き込み(レジスタ指定), (2)読み出し */
  struct i2c_config_s config;
  int ret;

  config.frequency = 400000; /* 適宜設定 */
  config.address = priv->addr;
  config.addrlen = 7;

  /* レジスタアドレスを送信 */
  ret = i2c_write(priv->bus.i2c, &config, &reg_addr, 1);
  if (ret < 0)
  {
    return (int8_t)BMI2_E_COM_FAIL;
  }

  /* 連続受信 */
  ret = i2c_read(priv->bus.i2c, &config, data, len);
  if (ret < 0)
  {
    return (int8_t)BMI2_E_COM_FAIL;
  }

  return BMI2_OK;
}

static int8_t bmi2_i2c_write(uint8_t reg_addr, const uint8_t *data, uint16_t len, void *intf_ptr)
{
  bmi270_dev_t *priv = (bmi270_dev_t *)intf_ptr;
  if (!priv || !data || len == 0)
  {
    return BMI2_E_NULL_PTR;
  }

  /* reg_addr + data(len) をまとめて送る */
  uint8_t buffer[64];
  if (len + 1 > sizeof(buffer))
  {
    return BMI2_E_COM_FAIL; /* バッファ不足など */
  }

  buffer[0] = reg_addr;
  for (uint16_t i = 0; i < len; i++)
  {
    buffer[i + 1] = data[i];
  }

  struct i2c_config_s config;
  config.frequency = 400000;
  config.address = priv->addr;
  config.addrlen = 7;

  if (i2c_write(priv->bus.i2c, &config, buffer, len + 1) < 0)
  {
    return BMI2_E_COM_FAIL;
  }

  return BMI2_OK;
}

static int8_t bmi2_spi_read(uint8_t reg_addr, uint8_t *data, uint16_t len, void *intf_ptr)
{
  bmi270_dev_t *priv = (bmi270_dev_t *)intf_ptr;
  if (!priv || !data || len == 0)
  {
    return BMI2_E_NULL_PTR;
  }

  /* SPI トランザクション開始 */
  /* SPI_LOCK() / SPI_SELECT() の呼び出しなどが必要(環境による) */
  /* 以下は簡略例 */

  /* BMI270 のSPIは、読み出し時にアドレスのMSBを 1 にする(7bit目にセット) が必要かつ
   * Bosch推奨手順あり。ここではラフに reg_addr | 0x80 とする例を示す。
   */
  uint8_t cmd = reg_addr | 0x80;

  /* 送受信バッファ(コマンド送信+ダミー送信で受信する) */
  /* 1バイト目にコマンド、続く len バイトをダミー送信しながら data を受け取る */
  uint8_t send[1] = {cmd};

  /* Lock */
  SPI_LOCK(priv->bus.spi, true);
  SPI_SELECT(priv->bus.spi, priv->addr, true);

  /* レジスタアドレスを送信 */
  SPI_SNDBLOCK(priv->bus.spi, send, 1);
  /* 連続受信 */
  SPI_RECVBLOCK(priv->bus.spi, data, len);

  /* Unlock */
  SPI_SELECT(priv->bus.spi, priv->addr, false);
  SPI_LOCK(priv->bus.spi, false);

  return BMI2_OK;
}

static int8_t bmi2_spi_write(uint8_t reg_addr, const uint8_t *data, uint16_t len, void *intf_ptr)
{
  bmi270_dev_t *priv = (bmi270_dev_t *)intf_ptr;
  if (!priv || !data || len == 0)
  {
    return BMI2_E_NULL_PTR;
  }

  /* SPI トランザクション開始 */
  /* 書き込み時は reg_addr の MSB=0(7bit目=0) */
  uint8_t cmd = reg_addr & 0x7F;

  SPI_LOCK(priv->bus.spi, true);
  SPI_SELECT(priv->bus.spi, priv->addr, true);

  /* レジスタアドレス1バイト送信 */
  SPI_SNDBLOCK(priv->bus.spi, &cmd, 1);
  /* 続いてデータを送信 */
  SPI_SNDBLOCK(priv->bus.spi, data, len);

  SPI_SELECT(priv->bus.spi, priv->addr, false);
  SPI_LOCK(priv->bus.spi, false);

  return BMI2_OK;
}

static void bmi2_delay_us(uint32_t period)
{
  /* 短いスリープの場合、us単位でsleepできる nxsig_usleep などを利用 */
  /* periodが大きいときには注意(簡略実装) */
  usleep(period);
}

static float lsb_to_mps2(bmi270_dev_t *dev, int16_t val)
{
  /* BMI270 の分解能: デフォルト 16bit (dev->dev.resolution=16) */
  float half_scale = ((float)(1 << dev->dev.resolution) / 2.0f);

  /* range は dev->acc_range に 2,4,8,16 が入っている想定 */
  /* 例えば ±2G の場合 -> 2 * GRAVITY_EARTH を full scale として換算 */
  float scale = (GRAVITY_EARTH * (float)dev->acc_range) / half_scale;

  return (val * scale);
}

static float lsb_to_dps(bmi270_dev_t *dev, int16_t val)
{
  /* ±gyr_range (2000,1000,500,250,125) のうちいずれかが入っている想定 */
  /* ±2000 dps = 32768 カウント, 1 LSB = 2000/32768 dps */
  float dps_per_count = (float)dev->gyr_range / 32768.0f;
  return val * dps_per_count;
}
