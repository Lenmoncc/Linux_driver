#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

#define AHT10_DEVICE_FILE "/dev/aht10"
#define READ_INTERVAL_SECONDS 2

int main() {
    int fd;
    int data[2];  
    ssize_t ret;

    fd = open(AHT10_DEVICE_FILE, O_RDONLY);
    if (fd < 0) {
        perror("Failed to open device file");
        return -1;
    }

    while (1) {
        ret = read(fd, data, sizeof(data));
        if (ret < 0) {
            perror("Failed to read data from device");
            break;
        }
        
        // 转换为浮点数显示
        float humidity = data[0] / 100.0f;
        float temperature = data[1] / 100.0f;

        printf("Humidity: %.2f%%\n", humidity);
        printf("Temperature: %.2f°C\n", temperature);

        sleep(READ_INTERVAL_SECONDS);
    }

    close(fd);

    return 0;
}