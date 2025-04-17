#!/bin/bash

# 检查 ethtool 命令是否存在
if ! command -v ethtool &> /dev/null; then
    echo "错误：ethtool 未安装，请先安装（CentOS: yum install ethtool; Ubuntu: apt install ethtool）"
    exit 1
fi

# 获取网卡列表（参数或所有活动网卡）
interfaces=("$@")
if [ ${#interfaces[@]} -eq 0 ]; then
    # 获取所有存在的网卡（排除回环接口 lo）
    interfaces=($(ls /sys/class/net/ | grep -v '^lo$'))
fi

# 检查每个网卡的 Link 状态
for interface in "${interfaces[@]}"; do
    echo "------------------------"
    echo "网卡：$interface"
    
    # 获取 Link 状态（ethtool 输出格式：Link detected: yes/no）
    link_status=$(ethtool "$interface" 2> /dev/null | grep "Link detected" | awk '{print $NF}')
    
    if [ -z "$link_status" ]; then
        echo "错误：网卡 $interface 不存在或无权限访问"
    elif [ "$link_status" == "yes" ]; then
        echo "Link 状态：UP（已连接）"
    else
        echo "Link 状态：DOWN（未连接）"
    fi
done
