
readme_content = """# pktgen 网卡压力测试脚本

## 简介

这是一个基于 Linux `pktgen` 内核模块的网络性能测试脚本，用于生成高速网络流量以测试网卡性能和吞吐量。

## 功能特性

- **内核级发包**：利用 Linux pktgen 内核模块，绕过协议栈，实现高性能数据包生成
- **可配置参数**：支持自定义包大小、发包数量、MAC 地址、速率限制等
- **自动加载模块**：自动检测并加载 pktgen 内核模块
- **设备自动添加**：自动将指定网卡添加到 pktgen 测试线程

## 环境要求

| 项目 | 要求 |
|------|------|
| 操作系统 | Linux（内核需支持 pktgen 模块） |
| 权限 | root 权限（需要操作 /proc/net/pktgen/） |
| 内核模块 | pktgen |
| 网卡 | 物理网卡或支持 pktgen 的虚拟网卡 |

## 快速开始

### 1. 修改配置

编辑脚本中的以下变量：

```bash
ifname="enp17s0"          # 修改为要测试的网卡名
thread_file="kpktgend_0"   # pktgen 线程文件名（通常保持默认）
```

### 2. 调整测试参数

```bash
pkt_size_num="1514"              # 数据包大小（字节），默认以太网帧最大长度
src_mac_num="aa:bb:cc:dd:00:00"  # 源 MAC 地址
dst_mac_num="66:26:62:00:00:00"  # 目的 MAC 地址
count_num="1000000"              # 发送数据包总数
rate_num="1000"                  # 发送速率限制（Mbps）
```

### 3. 执行测试

```bash
sudo bash pkt_test.sh
```

## 参数说明

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `ifname` | `enp17s0` | 绑定测试的网卡接口名 |
| `thread_file` | `kpktgend_0` | pktgen 线程控制文件 |
| `pkt_size` | 1514 | 数据包大小（含以太网头），范围 64-1514 |
| `count` | 1000000 | 发送数据包数量，0 表示无限发送 |
| `src_mac` | aa:bb:cc:dd:00:00 | 源 MAC 地址 |
| `dst_mac` | 66:26:62:00:00:00 | 目的 MAC 地址 |
| `rate` | 1000 | 速率限制，单位 Mbps，0 表示无限制 |

## 工作原理

```
┌─────────────────────────────────────────┐
│           用户空间脚本                   │
│         (pkt_test.sh)                   │
└─────────────────┬───────────────────────┘
                  │ 写入 /proc/net/pktgen/
                  ▼
┌─────────────────────────────────────────┐
│           pktgen 内核模块              │
│    ┌─────────┐    ┌─────────┐          │
│    │ kpktgend│───▶│ 网卡驱动 │          │
│    │ 线程 0  │    │ enp17s0 │          │
│    └─────────┘    └─────────┘          │
└─────────────────────────────────────────┘
```

## 输出示例

脚本执行后会显示 pktgen 统计信息：

```
Params: count 1000000  min_pkt_size: 60  max_pkt_size: 60
     frags: 0  delay: 0  clone_skb: 0  ifname: enp17s0
     flows: 0  flowlen: 0
     queue_map_min: 0  queue_map_max: 0
     dst_min: 0.0.0.0  dst_max: 0.0.0.0
     src_min: 0.0.0.0  src_max: 0.0.0.0
     src_mac: aa:bb:cc:dd:00:00 dst_mac: 66:26:62:00:00:00
     udp_src_min: 9  udp_src_max: 9  udp_dst_min: 9  udp_dst_max: 9
     src_mac_count: 0  dst_mac_count: 0
     Flags: 
Current:
     pkts-sofar: 1000000  errors: 0
     started: 1234567890us  stopped: 1234567990us idle: 0us
     seq_num: 1000000  cur_dst_mac_offset: 0  cur_src_mac_offset: 0
     cur_saddr: 0.0.0.0  cur_daddr: 0.0.0.0
     cur_udp_dst: 0  cur_udp_src: 0
     cur_queue_map: 0
     flows: 0
Result: OK: 1000000(c1000000+d0) nsec, 1000000 (1000000pps) 480Mbps (480000000bps) 1000000 packets
```

## 常见问题

### Q: 提示 "No such file or directory: /proc/net/pktgen/"

**A**: 内核未编译 pktgen 模块或模块未加载。尝试手动加载：
```bash
sudo modprobe pktgen
```

### Q: 速率达不到预期

**A**: 
- 检查网卡实际带宽能力
- 尝试调整 `rate` 参数或设为 0 取消限制
- 确认 CPU 性能是否成为瓶颈

### Q: 如何测试对端设备？

**A**: 
- 设置正确的目的 MAC 地址（对端网卡 MAC）
- 确保对端设备已连接并处于同一网络
- 在对端使用 `tcpdump` 或网卡统计查看收包情况

### Q: 支持多队列吗？

**A**: 支持。可通过添加多个设备到不同线程实现多队列测试：
```bash
echo "add_device enp17s0@0" > /proc/net/pktgen/kpktgend_0  # 队列0
echo "add_device enp17s0@1" > /proc/net/pktgen/kpktgend_1  # 队列1
```

## 扩展功能

### 添加 IP 层配置（修改脚本）

如需测试三层转发，可添加：
```bash
pgset "dst_min 192.168.1.1"     # 目的 IP
pgset "src_min 192.168.1.2"     # 源 IP
pgset "udp_dst_min 5001"        # 目的端口
pgset "udp_src_min 5002"        # 源端口
```

### 多线程测试

创建多个线程文件实现并行发包：
```bash
# 线程 0
echo "add_device enp17s0" > /proc/net/pktgen/kpktgend_0
# 线程 1  
echo "add_device enp17s0" > /proc/net/pktgen/kpktgend_1
# 同时启动
echo "start" > /proc/net/pktgen/pgctrl
```

## 参考文档

- [Linux pktgen 官方文档](https://www.kernel.org/doc/Documentation/networking/pktgen.txt)
- `man 8 pktgen`（如已安装 pktgen 工具包）

## 注意事项

⚠️ **警告**：
1. 该脚本会生成大量网络流量，请确保在测试环境中使用
2. 高速发包可能导致网卡或交换机过载
3. 建议在隔离的测试网络中运行，避免影响生产环境
4. 使用前确认对端设备可承受相应流量负载

## License

MIT License - 自由使用和修改
"""

print(readme_content)
