#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

// サンプル用に更新周期を秒単位で定義（実際は適切な周波数に調整）
#define IMU_INTERVAL 1  // 例：毎秒1回
#define GNSS_INTERVAL 1 // 例：毎秒1回

// IMUデータの構造体（実際には必要な項目に応じて拡張する）
typedef struct
{
    double ax, ay, az; // 加速度（例）
    int valid;
} IMUData;

// GNSSデータの構造体
typedef struct
{
    double lat, lon; // 緯度・経度
    int valid;
} GNSSData;

// グローバルなデータとミューテックス
IMUData imuData;
GNSSData gnssData;
pthread_mutex_t data_mutex = PTHREAD_MUTEX_INITIALIZER;

// ダミー関数（実際は各センサからデータを取得する処理を記述）
void readIMU(double *ax, double *ay, double *az)
{
    // ここではダミーの値を返す
    *ax = 1.0;
    *ay = 2.0;
    *az = 3.0;
}

void readGNSS(double *lat, double *lon)
{
    // ここではダミーの値を返す
    *lat = 35.0;
    *lon = 139.0;
}

// IMU取得用スレッド
void *thread_imu(void *arg)
{
    while (1)
    {
        double ax, ay, az;
        readIMU(&ax, &ay, &az);

        // 取得した値を共有変数に格納（排他制御）
        pthread_mutex_lock(&data_mutex);
        imuData.ax = ax;
        imuData.ay = ay;
        imuData.az = az;
        imuData.valid = 1;
        pthread_mutex_unlock(&data_mutex);

        // 次の取得まで待機
        sleep(IMU_INTERVAL);
    }
    return NULL;
}

// GNSS取得用スレッド
void *thread_gnss(void *arg)
{
    while (1)
    {
        double lat, lon;
        readGNSS(&lat, &lon);

        pthread_mutex_lock(&data_mutex);
        gnssData.lat = lat;
        gnssData.lon = lon;
        gnssData.valid = 1;
        pthread_mutex_unlock(&data_mutex);

        sleep(GNSS_INTERVAL);
    }
    return NULL;
}

int main()
{
    pthread_t imu_thread, gnss_thread;

    // 初期化
    imuData.valid = 0;
    gnssData.valid = 0;

    // 各センサ取得用スレッドの作成
    if (pthread_create(&imu_thread, NULL, thread_imu, NULL) != 0)
    {
        perror("IMUスレッド作成失敗");
        exit(EXIT_FAILURE);
    }
    if (pthread_create(&gnss_thread, NULL, thread_gnss, NULL) != 0)
    {
        perror("GNSSスレッド作成失敗");
        exit(EXIT_FAILURE);
    }

    // メインスレッドでのデータ融合ループ
    while (1)
    {
        pthread_mutex_lock(&data_mutex);
        if (imuData.valid && gnssData.valid)
        {
            // ここで両センサの最新データを用いてより正確な位置推定を行う
            // 例：IMUの加速度値からの移動量積分とGNSS値の補正の組み合わせ
            printf("GNSS: lat = %.6f, lon = %.6f | IMU: ax = %.2f, ay = %.2f, az = %.2f\n",
                   gnssData.lat, gnssData.lon, imuData.ax, imuData.ay, imuData.az);
        }
        pthread_mutex_unlock(&data_mutex);

        // 融合処理の周期（適宜調整）
        sleep(1);
    }

    // ※ 通常はスレッド終了処理が必要ですが、この例では無限ループのため省略
    return 0;
}
