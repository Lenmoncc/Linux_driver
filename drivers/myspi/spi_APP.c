#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define DEVICE_PATH "/dev/myspidev"
#define BUF_SIZE 16

int main() {
    int fd;
    char tx_buf[BUF_SIZE] = "Hello SPI!";
    char rx_buf[BUF_SIZE] = {0};

    // 打开设备
    fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        perror("Failed to open device");
        return -1;
    }

    // 写入数据（触发SPI传输）
    if (write(fd, tx_buf, strlen(tx_buf)) < 0) {
        perror("Write failed");
        close(fd);
        return -1;
    }

    // 读取数据（从SPI设备）
    if (read(fd, rx_buf, BUF_SIZE) < 0) {
        perror("Read failed");
        close(fd);
        return -1;
    }

    printf("Received: %s\n", rx_buf);
    close(fd);
    return 0;
}