#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>

#define BMP280_DEVICE_PATH "/dev/bmp280"

// 压力补偿计算函数
float compensate_pressure(uint32_t adc_press, int32_t t_fine,
                         uint16_t dig_P1, int16_t dig_P2, int16_t dig_P3,
                         int16_t dig_P4, int16_t dig_P5, int16_t dig_P6,
                         int16_t dig_P7, int16_t dig_P8, int16_t dig_P9) {
    int64_t var1, var2, p;

    var1 = (int64_t)t_fine - 128000;
    var2 = var1 * var1 * (int64_t)dig_P6;
    var2 = var2 + ((var1 * (int64_t)dig_P5) << 17);
    var2 = var2 + ((int64_t)dig_P4 << 35);
    var1 = ((var1 * var1 * (int64_t)dig_P3) >> 8) + ((var1 * (int64_t)dig_P2) << 12);
    var1 = ((((int64_t)1 << 47) + var1) * (int64_t)dig_P1) >> 33;

    if (var1 == 0) {
        return 0.0f; // 避免除零异常
    }

    p = 1048576 - adc_press;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = ((int64_t)dig_P9 * (p >> 13) * (p >> 13)) >> 25;
    var2 = ((int64_t)dig_P8 * p) >> 19;
    p = ((p + var1 + var2) >> 8) + ((int64_t)dig_P7 << 4);

    return (float)p / 256.0f;
}

int main() {
    int fd;
    uint8_t buffer[30]; 
    int32_t temperature;
    uint32_t adc_press;  
    int32_t t_fine;      // 温度微调值（用于压力补偿）
    
    // 压力校准参数
    uint16_t dig_P1;
    int16_t dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9;

    fd = open(BMP280_DEVICE_PATH, O_RDONLY);
    if (fd == -1) {
        perror("Failed to open device");
        return -1;
    }

    while (1) {
        ssize_t bytes_read = read(fd, buffer, sizeof(buffer));
        if (bytes_read != sizeof(buffer)) {
            perror("Failed to read data");
            close(fd);
            return -1;
        }

        temperature = (buffer[3] << 24) | (buffer[2] << 16) | (buffer[1] << 8) | buffer[0];
        
        //adc_press = (buffer[7] << 24) | (buffer[6] << 16) | (buffer[5] << 8) | buffer[4];
        adc_press = ((uint32_t)buffer[6] << 16) | ((uint32_t)buffer[5] << 8) | buffer[4];
        adc_press &= 0x00FFFFF; 

        t_fine = (buffer[11] << 24) | (buffer[10] << 16) | (buffer[9] << 8) | buffer[8];
        
        // 解析压力校准参数
        dig_P1 = (buffer[13] << 8) | buffer[12];
        dig_P2 = (buffer[15] << 8) | buffer[14];
        dig_P3 = (buffer[17] << 8) | buffer[16];
        dig_P4 = (buffer[19] << 8) | buffer[18];
        dig_P5 = (buffer[21] << 8) | buffer[20];
        dig_P6 = (buffer[23] << 8) | buffer[22];
        dig_P7 = (buffer[25] << 8) | buffer[24];
        dig_P8 = (buffer[27] << 8) | buffer[26];
        dig_P9 = (buffer[29] << 8) | buffer[28];

        // 计算补偿后的压力值
        double pressure = compensate_pressure(adc_press, t_fine,
                                              dig_P1, dig_P2, dig_P3,
                                              dig_P4, dig_P5, dig_P6,
                                              dig_P7, dig_P8, dig_P9);

        printf("Temperature: %d\n", temperature);
        printf("Pressure: %.2f Pa\n", pressure);

        sleep(1);
    }

    close(fd);
    return 0;
}