#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>

#define MPU6050_DEV_PATH "/dev/mpu6050"

// 定义数据结构体，与驱动中保持一致
typedef struct {
    short accel_x;
    short accel_y;
    short accel_z;
    short temp;
    short gyro_x;
    short gyro_y;
    short gyro_z;
} mpu6050_data_t;

volatile sig_atomic_t stop = 0;

// 信号处理函数，用于捕获 Ctrl+C 信号
void signal_handler(int signum) {
    if (signum == SIGINT) {
        stop = 1;
    }
}

int main() {
    int fd;
    mpu6050_data_t data;

    // 注册信号处理函数
    signal(SIGINT, signal_handler);

    // 打开设备文件
    fd = open(MPU6050_DEV_PATH, O_RDONLY);
    if (fd < 0) {
        perror("Failed to open device file");
        return -1;
    }

    // 循环读取并打印数据
    while (!stop) {
        // 读取数据
        ssize_t ret = read(fd, &data, sizeof(data));
        if (ret < 0) {
            perror("Failed to read data from device");
            break;
        }

        // 打印读取到的数据
        printf("\033[2J\033[H"); // 清屏
        printf("Accelerometer:\n");
        printf("  X: %d\n", data.accel_x);
        printf("  Y: %d\n", data.accel_y);
        printf("  Z: %d\n", data.accel_z);

        printf("Temperature: %d\n", data.temp);

        printf("Gyroscope:\n");
        printf("  X: %d\n", data.gyro_x);
        printf("  Y: %d\n", data.gyro_y);
        printf("  Z: %d\n", data.gyro_z);

        fflush(stdout); // 刷新输出缓冲区

        // 延时一段时间，避免过于频繁地读取数据
        usleep(100000); // 延时 100 毫秒
    }

    // 关闭设备文件
    close(fd);

    return 0;
}