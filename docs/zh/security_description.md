# 安全管理与加固

## 信任模型

UBS Memory (ubsmem) 运行于超节点集群内部，其安全设计基于以下信任假设：

- **节点间互信**：ubsmd 节点之间通过 TLS 证书互相认证，RPC 通道仅对受信节点开放。RPC 消息中携带的 `uid`、`gid`、`pid` 由 IPC 入口端通过 Unix Domain Socket 从内核获取（`SO_PEERCRED`），经由已认证的 ubsmd 节点转发至服务端，链路各环节具备身份保证。
- **集群边界防护**：服务仅监听超节点内部网络，不暴露于外部不可信网络。集群外的非法访问应由网络层防火墙及访问控制策略阻断。

## 安全管理

### 防病毒软件例行检查

定期开展对集群的防病毒扫描，防病毒例行检查会帮助集群免受病毒、恶意代码、间谍软件以及程序侵害，降低系统瘫痪、信息安全问题等风险。可以使用业界主流防病毒软件进行防病毒检查。

### 日志管理

在日志管理中请关注如下两点：

- 检查系统是否可以限制单个日志文件的大小。单个日志文件支持2MB\~100MB，生成1\~50个文件。
- 检查日志空间占满后，是否存在机制进行清理。超过配置的日志文件个数后，会根据生成时间，覆盖最早的日志文件。

### 漏洞/功能问题修复

为保证生产环境的安全，降低被攻击的风险，请定期查看开源社区修复以下漏洞/功能问题：

- 操作系统漏洞/功能问题。
- 其他相关组件漏洞/功能问题。

## 安全加固

### 加固须知

本文中列出的安全加固措施为基本的加固建议项。用户应根据自身业务，重新审视整个系统的网络安全加固措施，必要时可参考业界优秀加固方案和安全专家的建议。

### 操作系统安全加固

#### 防火墙配置

操作系统安装后，若配置普通用户，可以通过在“/etc/login.defs”文件中新增“ALWAYS\_SET\_PATH=yes”配置，防止越权操作。此外，为了防止使用 `su` 命令切换用户时，将当前用户环境变量带入其他环境造成提权，请使用 `su - [user]` 命令进行用户切换，同时在服务器配置文件“/etc/default/su”中增加配置参数“ALWAYS\_SET\_PATH=yes”防止提权。

#### 设置umask

建议用户将服务器的umask设置为027\~777，限制文件权限。

以设置umask为027为例，具体操作如下所示。

1. 以root用户登录服务器，编辑“/etc/profile”文件。

    ```bash
    vim /etc/profile
    ```

2. 在“/etc/profile”文件末尾加上 `umask 027` ，保存并退出。
3. 执行如下命令使配置生效。

    ```bash
    source /etc/profile
    ```

#### 无属主文件安全加固

用户可以执行 `find / -nouser -nogroup` 命令，查找容器内或物理机上的无属主文件。根据文件的UID和GID创建相应的用户和用户组，或者修改已有用户的UID、用户组的GID来适配，赋予文件属主，避免无属主文件给系统带来安全隐患。

#### 端口扫描

用户需要关注全网侦听的端口和非必要端口，如有非必要端口请及时关闭。建议用户关闭不安全的服务，如Telnet、FTP等，以提升系统安全性。具体操作方法可参考所使用操作系统的官方文档。

#### 防DoS攻击

用户可以按IP地址限制与服务器的连接速率对系统进行防DoS攻击，方法包括但不限于利用Linux系统自带iptables防火墙进行预防、优化sysctl参数等。具体使用方法，用户可自行查阅相关资料。

#### SSH加固

由于root用户拥有最高权限，出于安全目的，建议取消root用户SSH远程登录服务器的权限，以提升系统安全性。具体操作步骤如下：

1. 登录安装UBS Memory的节点。
2. 打开“/etc/ssh/sshd\_config”文件。

    ```bash
    vim /etc/ssh/sshd_config
    ```

3. 按“i”进入编辑模式，找到“PermitRootLogin”配置项并将其值设置为“no”。

    ```bash
    PermitRootLogin no
    ```

4. 按“Esc”键，输入**:wq!**，按“Enter”保存并退出编辑。
5. 执行命令使配置生效。

    ```bash
    systemctl restart sshd
    ```

#### 缓冲区溢出安全保护

为阻止缓冲区溢出攻击，建议使用 ASLR（Address Space Layout Randomization，内存地址随机化机制）技术，通过对堆、栈、共享库映射等线性区布局的随机化，增加攻击者预测目的地址的难度，防止攻击者直接定位攻击代码位置。该技术可作用于堆、栈、内存映射区（mmap基址、shared libraries、vDSO页）。

开启方式：

```bash
echo 2 >/proc/sys/kernel/randomize_va_space
```

## 开启TLS认证

>[!CAUTION]注意
>如果关闭TLS认证，会存在较高的网络安全风险。

### 准备TLS证书

内存池组件不提供数字证书、CA证书、密钥、吊销证书列表文件，需自行准备所列文件，文件权限应为600。

>[!CAUTION]注意
>证书安全要求：
>
>- 需使用业界公认安全可信的非对称加密算法、密钥长度、Hash算法、证书格式等。
>- 应处于有效期内。

### 使用说明

将ubsmd.conf文件中通信加密和认证选项的`ubsm.server.tls.enable`和`ubsm.lock.tls.enable`参数配置为`on`，再将对应文件的绝对路径填入通信加密和认证选项`ubsm\.server\.tls\.xxx` 和`ubsm\.lock\.tls\.xxx`，即可开启TLS认证。

`ubsm.server.tls.enable`是CC软锁模块选主的通信TLS开关，`ubsm.lock.tls.enable`是锁操作的TLS通信，支持分别配置。

通信加密和认证选项的详情请参见[表 ubsmd.conf配置文件参数说明](./configuration_description.md#table003)。

### 加密与解密

为支持灵活的密码解密策略，系统允许用户通过提供自定义动态链接库（\.so文件）的方式，注入其专属的解密实现。

#### 接口功能

解密私钥口令。用户需在自行编译的动态库（libdecrypt\.so）中，实现以下C++函数。

#### 接口格式

```C++
extern "C" char* DecryptPassword(const char* encrypted_data, size_t encrypted_len, size_t* p_out_len);
```

#### 参数说明

|参数名|数据类型|参数类型|描述|
|--|--|--|--|
|encrypted_data|const char*|入参|指向输入密文数据的指针，内容为原始二进制或编码后的字符串（由用户与调用方约定格式）。|
|encrypted_len|size_t|入参|密文数据的字节长度。|
|p_out_len|size_t*|出参|用于返回解密后明文的字节长度。用户必须在此函数内将实际明文长度写入 \*p_out_len。解密失败时 \*p_out_len 返回0。|

#### 返回值

|返回值|描述|
|--|--|
|char*|解密成功。指向通过new char[]分配的明文数据的指针（类型为char*）。该内存将由调用方使用delete[]释放，请勿使用malloc、calloc或栈/静态内存。|
|NULL|解密失败（如校验错误、密钥不匹配等）。|

>[!NOTE]说明
>
>- 尽管函数逻辑可以用C++编写，但必须使用extern "C"链接规范，以避免C++名称修饰（name mangling），确保动态库能正确解析符号。
>- 必须使用 new char[plain\_len] 分配返回缓冲区，因为调用方将使用 delete[] 回收内存。
>- 混用 malloc/free 与 new/delete[] 会导致未定义行为（如程序崩溃、内存泄漏等）。

#### 动态库要求

动态库文件名应为 libdecrypt\.so，并放置于如下加载路径。

```bash
/usr/local/ubs_mem/lib/libdecrypt.so
```

需要修改其属主为ubsmd，并设置权限为500。

```bash
chmod ubsmd /usr/local/ubs_mem/lib/libdecrypt.so
chmod 500 /usr/local/ubs_mem/lib/libdecrypt.so
```

通过遵循上述规范，用户可安全、可靠地集成自定义密码解密能力，满足不同安全策略或合规性要求。

## 账户一览表

|用户|描述|初始密码|密码修改方法|
|--|--|--|--|
|ubsmd|UBS Memory服务所属用户。|系统用户，无密码，不可登录。|-|

## 通信矩阵

| 通道 | 源 | 目标 | 协议 | 端口/路径 | TLS | 用途 |
|------|----|------|------|----------|-----|------|
| IPC通信 | 应用进程 (UBSM SDK) | ubsmd | UDS (Unix Domain Socket) | `/run/matrix/memory/MxmAgentIpcServer` | 否 | 内存借用、共享内存管理、锁/解锁、集群信息查询 |
| RPC通信 | ubsmd | ubsmd | TCP | `ubsm.server.rpc.local.ipseg` / `ubsm.server.rpc.remote.ipseg` | 可选（`ubsm.server.tls.enable`，默认 `on`） | 主节点选举 (ZenDiscovery)、节点信息查询 |
| DLock通信 | ubsmd | ubsmd | UB | `ubsm.lock.dev.name` / `ubsm.lock.dev.eid` | 可选（`ubsm.lock.tls.enable`，默认 `on`） | 分布式排他锁/共享锁/解锁/心跳 |
