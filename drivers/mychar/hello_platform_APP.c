#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

int main() {
    int fd = open("/dev/hello-char", O_RDWR);
    if (fd < 0) {
        perror("open failed");
        return -1;
    }

    // 测试写入
    char *test_data = "test data";
    ssize_t written = write(fd, test_data, strlen(test_data));
    printf("Write returned: %zd\n", written);

    // 测试读取
    char buf[128];
    ssize_t read_bytes = read(fd, buf, sizeof(buf));
    buf[read_bytes] = '\0'; // 添加字符串结束符
    printf("Read returned: %zd, Data: %s\n", read_bytes, buf);

    close(fd);
    return 0;
}