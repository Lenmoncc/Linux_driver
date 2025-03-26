#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

int main() {
    int fd;
    const char *dev = "/dev/ttyTest0"; // 设备节点路径
    char tx_buffer[] = "Trigger Virtual UART\n";
    char rx_buffer[256];

    // 打开设备（非阻塞模式）
    fd = open(dev, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) {
        perror("Failed to open device");
        return -1;
    }

    // 发送数据（触发 start_tx）
    ssize_t bytes_written = write(fd, tx_buffer, strlen(tx_buffer));
    if (bytes_written < 0) {
        perror("Write failed");
        close(fd);
        return -1;
    }

    // 触发 start_rx
    ssize_t bytes_read = read(fd, rx_buffer, sizeof(rx_buffer));
    if (bytes_read < 0) {
        if (errno != EAGAIN) { // 非阻塞模式下无数据时返回EAGAIN
            perror("Read failed");
        }
    }

    printf("Triggered TX/RX. Check dmesg for logs.\n");
    close(fd);
    return 0;
}