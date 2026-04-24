# STM32MP2 U-Boot

[English](README) | 中文

## 简介

本项目是基于 STMicroelectronics STM32MP2 系列处理器的 U-Boot 引导加载程序。U-Boot 是一个广泛使用的开源引导加载程序，支持多种处理器架构，用于初始化硬件并启动操作系统。

基于版本: `v2023.10-stm32mp-r2`

## 开发历史

| 提交 | 说明 |
|------|------|
| `c469dc2e` | 基于 ST 官方 v2023.10-stm32mp-r2 版本 |
| `b84d8ae6` | 修复以太网驱动，网络功能正常工作 |
| `fa374843` | 添加 Alientek MIPI 屏幕支持 |
| `a8159832` | 添加 W280BF036I DSI 屏幕支持 (1 lane) |
| `3e26059b` | 添加 Makefile.sdk.stm32mp2 构建脚本和 fiptool 工具 |
| `c2180fd2` | 添加中文 README 文档 |

## 支持的开发板

| SoC | 开发板 |
|-----|--------|
| STM32MP21 | STM32MP215F-DK |
| STM32MP23 | STM32MP235F-DK |
| STM32MP25 | STM32MP257F-DK, STM32MP257F-EV1 |

## 依赖项

在构建之前，请确保安装以下工具:

```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install -y build-essential gcc-aarch64-linux-gnu bison flex \
    libssl-dev python3 python3-pyelftools libgnutls28-dev
```

## 构建方法

### 方法一: 使用 SDK Makefile (推荐)

```bash
# 设置工具链路径
export ARCH=arm
export CROSS_COMPILE=aarch64-linux-gnu-

# 构建默认配置 (STM32MP257F-DK)
make -f Makefile.sdk.stm32mp2

# 构建特定 defconfig
make -f Makefile.sdk.stm32mp2 UBOOT_DEFCONFIG=stm32mp25_defconfig
```

### 方法二: 传统构建方式

```bash
export ARCH=arm
export CROSS_COMPILE=aarch64-linux-gnu-

# 配置 (以 STM32MP257F-DK 为例)
make stm32mp25_defconfig

# 编译
make -j$(nproc)
```

### 构建输出

构建完成后，以下文件将生成:

- `u-boot` - ELF 格式的 U-Boot 镜像
- `u-boot.bin` - 二进制格式的 U-Boot 镜像
- `u-boot.dtb` - U-Boot 设备树
- `u-boot-nodtb.bin` - 不含设备树的 U-Boot 镜像

## FIP 镜像生成

STM32MP2 使用 FIP (Firmware Image Package) 格式打包固件:

```bash
# 使用 fiptool 生成 FIP 镜像
./fiptool-stm32mp.stm32mp2 create \
    --hw-config u-boot.dtb \
    --tos-fw tee.bin \
    --nt-fw bl33.bin \
    fip.bin
```

## 刷写到 SD 卡

```bash
# 查找 SD 卡设备
lsblk

# 写入 (请将 sdX 替换为实际设备)
sudo dd if=u-boot-sdcard.bin of=/dev/sdX bs=1M seek=17
sync
```

## 项目结构

```
├── arch/           # 架构相关代码
├── board/          # 开发板相关代码
├── cmd/            # U-Boot 命令实现
├── common/         # 通用代码
├── configs/        # 板级默认配置文件
├── doc/            # 文档
├── drivers/        # 设备驱动
├── dts/            # 设备树源文件
├── env/            # 环境变量
├── fs/             # 文件系统支持
├── include/        # 头文件
├── lib/            # 库文件
├── net/            # 网络协议栈
├── scripts/        # 构建脚本
└── tools/          # 工具
```

## 调试

### 串口调试

连接串口调试工具 (如 minicom, picocom):

```bash
# 使用 picocom
picocom -b 115200 /dev/ttyUSB0

# 使用 minicom
minicom -D /dev/ttyUSB0 -b 115200
```

### U-Boot 常用命令

```
# 查看环境变量
printenv

# 设置启动参数
setenv bootargs console=ttySTM0,115200 root=/dev/mmcblk0p2 rootwait

# 保存环境变量
saveenv

# 从 SD 卡加载内核
load mmc 0:1 ${kernel_addr_r} uImage
load mmc 0:1 ${fdt_addr_r} stm32mp257f-dk.dtb

# 启动内核
bootm ${kernel_addr_r} - ${fdt_addr_r}
```

## 相关资源

- [U-Boot 官方文档](https://docs.u-boot.org/)
- [ST 官方 Wiki](https://wiki.st.com/stm32mpu/)
- [STM32MP2 参考手册](https://www.st.com/en/microcontrollers-microprocessors/stm32mp2-series.html)
- [U-Boot 邮件列表](https://lists.denx.de/listinfo/u-boot)

## 许可证

本项目基于 GPL-2.0+ 许可证发布。详情请参阅 [Licenses](Licenses/) 目录。
