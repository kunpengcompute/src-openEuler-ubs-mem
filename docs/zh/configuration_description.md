# 配置参数说明

**表 1 <a id="table003"></a>**  ubsmd.conf配置文件参数说明

|配置项|取值类型|取值范围|配置说明|
|--|--|--|--|
|`ubsm.server.log.level`|str|<ul><li>DEBUG</li><li>INFO</li><li>WARN</li><li>ERROR</li><li>CRITICAL</li></ul>|日志等级。<ul><li>DEBUG：调试</li><li>INFO：信息</li><li>WARN：警告</li><li>ERROR：错误</li><li>CRITICAL：紧急</li></ul>默认值：INFO。|
|`ubsm.server.log.path`|str|符合规范的路径|日志文件路径。默认值：/var/log/ubsm。|
|`ubsm.server.log.rotation.file.count`|int|[1, 50]|日志文件数量。默认值：10。|
|`ubsm.server.log.rotation.file.size`|int|[2, 100]|日志文件转储阈值。默认值：20，单位：MB。|
|`ubsm.server.audit.enable`|str|<ul><li>on</li><li>off</li></ul>|是否启用审计日志。<ul><li>on：启用</li><li>off：禁用</li></ul>默认值：on。|
|`ubsm.server.audit.log.path`|str|符合规范的路径|审计日志文件路径。默认值：/var/log/ubsm。|
|`ubsm.server.audit.log.rotation.file.count`|int|[1, 50]|审计日志文件数量，默认值：10。|
|`ubsm.server.audit.log.rotation.file.size`|int|[2, 100]|审计日志文件大小，默认值：20，单位：MB。|
|`ubsm.lock.enable`|str|<ul><li>on</li><li>off</li></ul>|表示是否开启CC软锁功能。<ul><li>on：开启</li><li>off：关闭</li></ul>默认值：off。|
|`ubsm.lock.expire.time`|int|[30, 86400]|加锁超时时间，默认值：300，单位：s。|
|`ubsm.lock.dev.name`|str|-|UB环境urma通信的设备名，不能为空，默认值：bonding_dev_0。|
|`ubsm.lock.dev.eid`|int|IPv6地址|UB环境urma通信的eid地址，默认值：0。优先使用eid进行通信，eid无效时会根据dev name获取第一个eid建链。|
|`ubsm.lock.ub_token.enable`|str|<ul><li>on</li><li>off</li></ul>|UB协议token访问安全配置。<ul><li>on：开启</li><li>off：关闭</li></ul>默认值：on。|
|`ubsm.lock.tls.enable`|str|<ul><li>on</li><li>off</li></ul>|是否开启CC软锁安全特性。安全配置项需要默认开启（on），关闭可能会有安全风险。<ul><li>on：启用</li><li>off：关闭</li></ul>通信使用端口：21616。|
|`ubsm.lock.tls.ca.path`|str|符合规范的路径|CA证书文件的绝对路径，不支持填写相对路径，不支持软链接。默认值：/path/cacert.pem。|
|`ubsm.lock.tls.crl.path`|str|符合规范的路径|吊销证书列表文件的绝对路径，不支持填写相对路径，不支持软链接。默认值：/path/crl.pem。|
|`ubsm.lock.tls.cert.path`|str|符合规范的路径|Server端证书文件的绝对路径，不支持填写相对路径，不支持软链接。默认值：/path/cert.pem。|
|`ubsm.lock.tls.key.path`|str|符合规范的路径|Server端证书对应的私钥文件的绝对路径，不支持填写相对路径，不支持软链接。默认值：/path/key.pem。|
|`ubsm.lock.tls.keypass.path`|str|符合规范的路径|用户使用加密工具seceasy_encrypt生成的Server端证书对应的私钥的加密密文文件的绝对路径，不支持填写相对路径，不支持软链接。默认值：/path/keypass.txt。|
|`ubsm.discovery.election.timeout`|int|[0, 2000]|选主延迟时间，用于从节点发现主节点失联后开始选主的随机推迟时间。默认值：1000，单位：ms。|
|`ubsm.discovery.min.nodes`|int|[0, 30]|选主的最小投票数。一般设置为集群总节点数的一半以上，如：(集群总节点数)/ 2 + 1，默认值：0。|
|`ubsm.server.rpc.local.ipseg`|str|IPv4地址|本地节点的IP地址及端口号。格式为**IP:Port**，**Port**取值范围为7201~7800，且必须和 `ubsm.server.rpc.remote.ipseg` 的**Port**保持一致。默认值：127.0.0.1:7201**。**|
|`ubsm.server.rpc.remote.ipseg`|str|IPv4地址|远端节点的IP地址及端口号。格式为**IP:Port**, **IP:Port**, **IP:Port**,...，**Port**取值范围为7201~7800，且必须和 `ubsm.server.rpc.local.ipseg` 的**Port**保持一致。默认值：127.0.0.1:7301。可以配置多个，例如：192.168.100.101:7301, 192.168.100.103:7301, 192.168.100.102:7301。|
|`ubsm.server.tls.enable`|str|<ul><li>on</li><li>off</li></ul>|是否开启安全特性。安全配置项需要默认开启（on），关闭可能会有安全风险。<ul><li>on：启用</li><li>off：关闭</li></ul>|
|`ubsm.server.tls.ciphersuits`|str|<ul><li>aes_gcm_128</li><li>aes_gcm_256</li><li>aes_ccm_128</li><li>chacha20_poly1305</li></ul>|通信加密方式，默认值：aes_gcm_128。配置其他加密方式程序启动会失败。|
|`ubsm.server.tls.ca.path`|str|符合规范的路径|CA证书文件的绝对路径，不支持填写相对路径，不支持软链接。默认值：/path/cacert.pem。|
|`ubsm.server.tls.crl.path`|str|符合规范的路径|吊销证书列表文件的绝对路径，不支持填写相对路径，不支持软链接。默认值：/path/crl.pem。|
|`ubsm.server.tls.cert.path`|str|符合规范的路径|Server端证书文件的绝对路径，不支持填写相对路径，不支持软链接。默认值：/path/cert.pem。|
|`ubsm.server.tls.key.path`|str|符合规范的路径|Server端证书对应的私钥文件的绝对路径，不支持填写相对路径，不支持软链接。默认值：/path/key.pem。|
|`ubsm.server.tls.keypass.path`|str|符合规范的路径|用户使用加密工具seceasy_encrypt生成的Server端证书对应的私钥的加密密文文件的绝对路径，不支持填写相对路径，不支持软链接。默认值：/path/keypass.txt。|
|`ubsm.hcom.max.connect.num`|int|(0, 256]|HCOM通信的最大连接数。默认值：128。|
|`ubsm.server.lease.cache.enable`|str|<ul><li>on</li><li>off</li></ul>|是否启用内存借用后的缓存功能。<ul><li>on：启用</li><li>off：关闭</li></ul>默认值：off。当前支持缓存5个借用内存。|
|`ubsm.performance.statistics.enable`|str|<ul><li>on</li><li>off</li></ul>|是否启用性能统计。性能统计日志默认存放在/var/log/ubsm目录。<ul><li>on：启用</li><li>off：关闭</li></ul>默认值：on。|
