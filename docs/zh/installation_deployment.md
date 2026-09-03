# 安装部署

## 安装前准备

**硬件环境**

安装前，需要检查以下硬件配置，如[表1](#table001)所示。

**表 1 <a id="table001"></a>**  硬件环境

| 类型 | 配置参考|
|-----|-----|
| 服务器 | <ul><li>TaiShan 500 2280</li> <li>其他配备支持UB的CPU的服务器</li></ul> |

## 安装UBS Memory

**操作步骤**

1. 使用root用户登录服务器。
2. 安装UBS Memory。

    - 在线安装
    
        ```bash
        dnf install -y ubs-mem-shmem
        ```

    - 离线安装
    
        ```bash
        dnf install -y ubs-mem-shmem-x.x.x-x.x.*.rpm
        ```

    >[!NOTE]说明
    >- 内存服务以ubsmd用户的身份运行，在使用RPM包安装时，若系统中不存在ubsmd用户，安装脚本将自动创建该用户。
    >- 安装成功后，so会默认安装到“/usr/local/ubs\_mem/lib”目录，使用时需要export该路径。
    >- 安装成功后，.h头文件会默认安装到“/usr/local/ubs\_mem/include”目录。
    >- 配置环境变量 **UBSM\_SDK\_TRACE\_ENABLE = 1**，开启性能打点统计，会在默认的日志路径（/var/log/ubsm）生成对应的打点数据。
    >- 配置环境变量 **MXM\_CHANNEL\_TIMEOUT= xx**，控制IPC通信的channel超时时间（单位s），当大块内存操作耗时较久时，可以配置较长时间，默认为60s。

3. 启动UBS Engine服务。

    ```bash
    systemctl start ubse.service
    ```

4. （可选）修改ubsmd.conf配置文件。

    a. 打开“/usr/local/ubs\_mem/config/ubsmd.conf”配置文件。

    ```bash
    vim /usr/local/ubs_mem/config/ubsmd.conf
    ```

    b. 按“i”进入编辑模式，根据实际情况对相关参数进行配置，参数详情请参见附录的[表1 ubsmd.conf配置文件参数说明](configuration_description.md#配置参数说明)。

    ```yaml
    # the log level of ubsm server, (DEBUG, INFO, WARN, ERROR, CRITICAL)
    ubsm.server.log.level = INFO
    # the log file path, must be canonical path
    ubsm.server.log.path = /var/log/ubsm
    # log file count, min is 1, max is 50
    ubsm.server.log.rotation.file.count = 10
    # log file size(MB), min is 2, max is 100
    ubsm.server.log.rotation.file.size = 20
    # enable or disable audit log, (on, off)
    ubsm.server.audit.enable = on
    # audit log, the configuration item value range is the same as 'ubsm.server.log.*'
    ubsm.server.audit.log.path = /var/log/ubsm
    ubsm.server.audit.log.rotation.file.count = 10
    ubsm.server.audit.log.rotation.file.size = 20
    # CC lock
    ubsm.lock.enable = off
    ubsm.lock.expire.time = 300
    ubsm.lock.dev.name = bonding_dev_0
    ubsm.lock.dev.eid = 0
    # CC lock tls options
    ubsm.lock.tls.enable = on
    ubsm.lock.tls.ca.path = /path/cacert.pem
    ubsm.lock.tls.crl.path = /path/crl.pem
    ubsm.lock.tls.cert.path = /path/cert.pem
    ubsm.lock.tls.key.path = /path/key.pem
    ubsm.lock.tls.keypass.path = /path/keypass.txt
    # Zen discovery
    # election timeout, min is 0, max is 2000
    ubsm.discovery.election.timeout = 1000
    # min nodes, min is 0, max is 30
    ubsm.discovery.min.nodes = 0
    ubsm.server.rpc.local.ipseg = 127.0.0.1:7201
    ubsm.server.rpc.remote.ipseg = 127.0.0.1:7301
    # tls options
    ubsm.server.tls.enable = on
    ubsm.server.tls.ciphersuits = aes_gcm_128
    ubsm.server.tls.ca.path = /path/cacert.pem
    ubsm.server.tls.crl.path = /path/crl.pem
    ubsm.server.tls.cert.path = /path/cert.pem
    ubsm.server.tls.key.path = /path/key.pem
    ubsm.server.tls.keypass.path = /path/keypass.txt
    # max is 8192, default 256
    ubsm.hcom.max.connect.num = 256
    # enable or disable memory lease cache, (on, off)
    ubsm.server.lease.cache.enable = off
    # enable performance statistics, (on off)
    ubsm.performance.statistics.enable = off
    ```

    >[!NOTE]说明
    >
    >- 使用共享内存的分布式锁功能时，需要在配置文件中主动设置当前节点的IP地址和端口号以及集群中其他节点的节点信息，启动当前节点的ubsmd进程，会同步启动其他节点。
    >- 开启TLS（Transport Layer Security，安全传输层协议）认证功能操作详情可参见[开启TLS认证](security_description.md#开启tls认证)，如果不使用该功能，将配置项 `ubsm.server.tls.enable` 设为 `off` 即可。

    c. 按“Esc”键，输入**:wq!**，按“Enter”保存并退出编辑。

5. 启动ubsmd。

    ```bash
    systemctl start ubsmd
    ```

    >[!NOTE]说明
    >ubsmd进程启动依赖UBSE，该服务启动成功方可加载成功。

6. 查看ubsmd状态。

    ```bash
    systemctl status ubsmd
    ```

    回显如下表明启动成功：

    ```bash
    ● ubsmd.service - UBS memory daemon
         Loaded: loaded (/etc/systemd/system/ubsmd.service; enabled; preset: disabled)
         Active: active (running) since xx YYYY-mm-dd HH:MM:SS CST; xxs ago
       Main PID: xxx (ubsmd)
         Status: "available"
          Tasks: 31 (limit: 822900)
       FD Store: 1 (limit: 3)
         Memory: xxG ()
         CGroup: /system.slice/ubsmd.service
                 └─xxx /usr/local/ubs_mem/bin/ubsmd -binpath=/usr/local/ubs_mem
    ```

## 卸载UBS Memory

1. 使用root用户登录服务器。
2. 卸载UBS Memory。

    ```bash
    dnf remove ubs-mem-shmem
    ```

    >[!CAUTION]注意
    >
    >- 卸载会自动停止ubsmd并释放占用的远端内存，因此在卸载ubsmd时，应确保UBSE正常运行且无业务正在进行。
    >- 为了避免权限问题，卸载后用户和用户组ubsmd将会保留。
    >- 如需卸载UBS Engine，请参见[UBS Engine 部署说明](https://atomgit.com/openeuler/ubs-engine/blob/master/docs/build_install/%E9%83%A8%E7%BD%B2%E8%AF%B4%E6%98%8E.md)文档。
