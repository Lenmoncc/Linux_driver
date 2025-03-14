#include <common.h>
#include <command.h>

static int do_mycmd(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
    puts("Hello, this is my custom command!\n");
    return 0;
}

U_BOOT_CMD(
    mycmd,    // 命令名称
    1,        // 最大参数个数
    0,        // 是否可重复执行
    do_mycmd, // 命令处理函数
    "Print a message using puts", // 命令描述
    "mycmd - Print a custom greeting message\n"
    "    This command does not accept any arguments.\n"
    "    It simply prints a predefined message to the console.\n"
    "    Example:\n"
    "      mycmd\n"     // 命令用法信息
);