# NeuG

## 1. 介绍

[NeuG](http://git.gniibe.org/gitweb/?p=gnuk/neug.git;a=summary) - A random number generator implementation for RT-Thread.

该组件包是 NeuG 在 RT-Thread 系统上的移植,通过使用该组件包可以很方便地获取到满足 [NISTSP800—90B](https://en.wikipedia.org/wiki/NIST_SP_800-90A) 草案中三个连续性检测(1:重复计数检测 2:自适应比例检测(64样本) 3:自适应比例检测(2048样本))的随机数,更多 NeuG 信息可查看 [NeuG README.md](http://git.gniibe.org/gitweb/?p=gnuk/neug.git;a=blob;f=README;h=7fadbfc2176b486754d4dc12f4979db95c402584;hb=HEAD) 

### 1.1 目录结构

| 名称 | 说明 |
| ---- | -----|
| src  | 源文件目录|
| inc  | 头文件目录|
| ports | 用户移植代码目录 |
| samples | 实例文件目录 |

### 1.2 许可证

NeuG 组件包遵循 GUN GPLv3 许可,详见 COPYING .

### 1.3 依赖

RT-Thread 3.0+

[tinycrypt](https://github.com/RT-Thread-packages/tinycrypt)

## 2. 获取方式

使用 NeuG package 需要在 RT-Thread 的包管理中选中它，具体路径如下：

    RT-Thread online packages
        security packages  --->
            [*] NeuG: A random number generator implementation for RT-Thread
            [*]   Enable a NeuG sample
                  NeuG version (v1.0.0)  --->

Enable a NeuG sample:使能 NeuG 示例代码.

NeuG version:配置 NeuG 组件包为 v1.0.0 版.

配置完成后让 RT-Thread 的包管理器自动更新,或者使用 pkgs --update 命令更新包到 BSP 中.

## 3. 移植

### 3.1 sys-neug.h 头文件

该头文件用来包含声明有 const uint8_t *unique_device_id (void) 函数的头文件.

在 ports 目录下提供了 sys-gnu-linux.c/h 文件,该文件作为示例文件向用户展示了 unique_device_id 的实现.用户需要根据自身硬件特性来实现相关函数.

#### 3.1.1 const uint8_t *unique_device_id (void)
获取唯一设备id,返回空间至少大于16字节

### 3.2 adc.h 头文件函数实现

在 NeuG 原方案中,使用多路 ADC 作为随机数源,需要实现
adc.h 中的相应函数来为 NeuG 提供随机数.

其中 uint32_t adc_buf[64]为存放随机数的空间.

在 ports 目录下提供了 adc-gnu-linux.c 文件,该文件作为示例文件向用户展示了 adc.h 声明函数 的实现(该实现基于伪随机数源).用户需要根据自身硬件特性来实现相关函数.

#### 3.2.1 int adc_init(void)

随机数源初始化

#### 3.2.2 void adc_start(void)

随机数源开启

#### 3.2.3 void adc_start_conversion(int offset, int count)

开启随机数填充,以 offset 为起始索引,向 adc_buf 中填充 count 次

#### 3.2.4 int adc_wait_completion (void)

等待随机数填充完毕

#### 3.2.5 void adc_stop (void)

随机数源关闭

## 4. 使用方式

### 4.1 示例列表

| 名称 | 说明 |
| ---- | ---- |
| neug_sample.c | 获取随机数 |

### 4.2 运行示例

该示例文件向 msh 导出名为 generate_random 的函数,在 msh 中输入 generate_random ,即可获取 32 字节随机数.

     \ | /
    - RT -     Thread Operating System
     / | \     3.0.4 build May 29 2018
     2006 - 2018 Copyright by rt-thread team
    Hello RT-Thread!
    File System on root initialized!
    File System on sd initialization failed!
    msh />generate_random
    -------------------------------
    66b3e2b4 1f9c2131 8bd09ff9 9eb3c64f cdc66ff9 c3117338 2227f735 22b807a2
    -------------------------------
    msh />

## 5. 注意事项

如果未在 BSP 的 ENV 中使能 RT_USING_COMPONENTS_INIT 则必须代码中添加 random_init(); 以初始化 NeuG 服务线程.

-------------------------------------------------------------------
