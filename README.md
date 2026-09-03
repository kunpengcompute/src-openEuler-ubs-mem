# ubs-mem

### 介绍
UBS Memory(Unified Bus Service Core Memory)在超节点上基于UB硬件能力提供Memory高阶服务能力，实现超节点上的内存借用、共享、缓存等能力。

### 环境要求

操作系统：推荐 openEuler 24.03 LTS SP3或更高版本

### 软件要求

**构建工具：**

- `rpm-build`：用于生成 RPM 安装包
- `cmake` (≥ 3.13)：跨平台构建系统
- `ninja-build`：高速构建工具，替代 make

**编译环境：**

- `gcc`(≥ 10.3.1)：C 语言编译器
- `gcc-c++`(≥ 10.3.1)：C++ 语言编译器

**依赖库：**

- `numactl-devel`: NUMA (Non-Uniform Memory Access) 支持库
- `systemd-devel`: systemd 服务管理支持库
- `openssl-devel`: OpenSSL 开发库
- `libboundscheck`: 安全函数库
- `ubs-comm-lib`: UBS 通信库

### 获取源码
```shell
git clone https://gitcode.com/openeuler/ubs-mem.git

cd ubs-mem
```

### 构建项目
代码仓中提供了统一的编译构建脚本（即build.sh），可以直接执行该脚本编译构建。-p 参数表示打rpm包，-t 表示编译方式如debug、release，示例如下。

```shell
sh build.sh -t release -p
```

构建产物位于 build/release/output\* 目录下，RPM 包输出至 build/release/output。

### 项目结构
```text
.
├── build     // 存放项目中使用的脚本文件
├── doc       // 存放项目文档，例如《代码架构设计》
├── src       // 存放项目的功能实现源码，仅该目录参与构建出包
├── test      // 存放项目的ut和dtfuzz等
└── build.sh  // 统一的构建入口       
```

### 开发者测试
```shell
cd test

# 运行UT测试用例并生成覆盖率报告
sh run_dt.sh
```
执行成功后，控制台将打印对应的覆盖率总结信息。详细的覆盖率报告位于“build/gcovr_report/”目录，可打开该目录下的index.html文件查看。

### 使用说明
- **安装部署**
    安装部署相关内容请参见 [安装部署](docs/zh/installation_deployment.md)。
- **API接口**
    API相关内容请参见 [接口说明](docs/zh/api_description.md)。

### License
ubs-mem 采用 Mulan V2 License.

### 贡献指南
请阅读 贡献指南 CONTRIBUTING.md 以了解如何贡献项目。