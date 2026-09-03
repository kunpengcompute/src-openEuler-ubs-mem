# API接口

## 说明

### 头文件列表

开发代码时所需的头文件如[表1](#table004)所示。

**表 1 <a name="table004"></a>**  头文件列表

|头文件名称|用途|
|--|--|
|ubs_mem_def.h|定义公共的错误码和查询类型。|
|ubs_mem.h|定义了UBS Memory对外接口。|

### 设置进程运行环境

- 应用进程依赖UBS Memory SDK动态库，需要设置环境变量用于查找动态库路径。

    ```bash
    export LD_LIBRARY_PATH="/usr/local/ubs_mem/lib/:$LD_LIBRARY_PATH"
    ```

- 当前版本锁的有效期默认为30s，故障恢复流程受UBS Comm建链重试的次数影响，为保证ubsmd故障恢复功能稳定，需设置如下环境变量。

    ```bash
    export HCOM_CONNECTION_RETRY_TIMES=2
    ```

- 为了保证正常使用UBS Memory SDK功能，需要将用户添加到ubsmd用户组中。

    ```bash
    usermod -aG ubsmd {UBSM-install-user}
    ```

## 接口

### 初始化

#### ubsmem\_init\_attributes

**接口功能**

构造UBSM SDK初始化函数参数，将参数设置为默认值。

**接口格式**

```C++
int ubsmem_init_attributes(ubsmem_options_t *ubsm_shmem_opts);
```

**参数说明**

|参数名|数据类型|参数类型|描述|
|--|--|--|--|
|ubsm_shmem_opts|ubsmem_options_t *|出参|初始化函数ubsmem_initialize的参数。|

**返回值**

|返回值|描述|
|--|--|
|0|操作成功。|
|非0|操作失败。具体错误码根据返回值不同参考[错误码](#错误码)。|

#### ubsmem\_initialize

**接口功能**

UBSM SDK初始化函数，使用UBSM功能前需要调用此函数，依赖UBSM服务。

**接口格式**

```C++
int ubsmem_initialize(const ubsmem_options_t *ubsm_shmem_opts);
```

**参数说明**

|参数名|数据类型|参数类型|描述|
|--|--|--|--|
|ubsm_shmem_opts|ubsmem_options_t *|入参|初始化函数入参，需要先通过ubsmem_init_attributes函数对参数进行初始化。|

**返回值**

|返回值|描述|
|--|--|
|0|操作成功。|
|非0|操作失败。具体错误码根据返回值不同参考[错误码](#错误码)。|

#### ubsmem\_finalize

**接口功能**

用于退出UBSM SDK功能的函数，回收SDK使用的资源。

**接口格式**

```C++
int ubsmem_finalize(void);
```

**参数说明**

无

**返回值**

|返回值|描述|
|--|--|
|0|操作成功。|
|非0|操作失败。具体错误码根据返回值不同参考[错误码](#错误码)。|

#### ubsmem\_set\_logger\_level

**接口功能**

设置UBSM打印日志级别。

**接口格式**

```C++
int ubsmem_set_logger_level(int level);
```

**参数说明**

|参数名|数据类型|参数类型|描述|
|--|--|--|--|
|level|int|入参|打印日志最高级别，对应级别如下：<ul><li>0：debug（默认）</li><li>1：info</li><li>2：warning</li><li>3：error</li><li>4：critical</li></ul>|

**返回值**

|返回值|描述|
|--|--|
|0|操作成功。|
|非0|操作失败。具体错误码根据返回值不同参考[错误码](#错误码)。|

#### ubsmem\_set\_extern\_logger

**接口功能**

自定义UBSM日志打印函数。

**接口格式**

```C++
int ubsmem_set_extern_logger(void (*func)(int level, const char *msg));
```

**参数说明**

|参数名|数据类型|参数类型|描述|
|--|--|--|--|
|func|`void (*)(int level, const char *msg)`|入参|UBSM组件日志定向打印函数。|

**返回值**

|返回值|描述|
|--|--|
|0|操作成功。|
|非0|操作失败。具体错误码根据返回值不同参考[错误码](#错误码)。|

### 共享域

共享域（region）是一组节点的组合，内存共享和借用都限制在一个指定的共享域内。

#### ubsmem\_lookup\_regions

**接口功能**

查询当前节点的相关拓扑信息，如互联节点数以及对应节点的hostname。

**接口格式**

```C++
int ubsmem_lookup_regions(ubsmem_regions_t *regions);
```

**参数说明<a id="parameter01"></a>**

|参数名|数据类型|参数类型|描述|
|--|--|--|--|
|regions|ubsmem_regions_t *|出参|互联节点数以及对应节点的hostname。|

相关结构体类型和常量定义。

```C++
#define MAX_HOST_NAME_DESC_LENGTH 64
#define MAX_REGION_NODE_NUM 16
#define MAX_REGIONS_NUM 6

typedef struct {
  char host_name[MAX_HOST_NAME_DESC_LENGTH];  // 需包含 '\0'
  bool affinity;  // 节点亲和，设置为true时则指定从该节点申请共享或借用内存
} ubsmem_region_node_desc_t;

typedef struct {
  int host_num;
  ubsmem_region_node_desc_t hosts[MAX_REGION_NODE_NUM];
} ubsmem_region_attributes_t;

typedef struct {
  int num;
  ubsmem_region_attributes_t region[MAX_REGIONS_NUM];
} ubsmem_regions_t;
```

**返回值**

|返回值|描述|
|--|--|
|0|操作成功。|
|非0|操作失败。具体错误码根据返回值不同参考[错误码](#错误码)。|

#### ubsmem\_create\_region

**接口功能**

创建共享域。

>[!NOTE]说明
>region仅在其创建的节点上有效，并且只能在该节点上使用。

**接口格式**

```C++
int ubsmem_create_region(const char *region_name, size_t size, const ubsmem_region_attributes_t *reg_attr);
```

**参数说明**

|参数名|数据类型|参数类型|描述|
|--|--|--|--|
|region_name|char *|入参|共享域名称，节点内唯一标识。最大有效长度为47字符（不包括“\0”）。|
|size|size_t|入参|控制资源使用，当前不支持quota，必须填0。|
|reg_attr|ubsmem_region_attributes_t *|入参|指定属于该共享域的节点以及亲和节点，亲和节点用于创建指定节点的共享内存。相关结构体类型和常量定义请参见ubsmem_lookup_regions章节的[参数说明](#parameter01)。|

**返回值**

|返回值|描述|
|--|--|
|0|操作成功。|
|非0|操作失败。具体错误码根据返回值不同参考[错误码](#错误码)。|

#### ubsmem\_lookup\_region

**接口功能**

查询共享域信息。

>[!NOTE]说明
>region仅在创建它的节点上有效，只能在该节点上查询，其他节点无法查询。

**接口格式**

```C++
int ubsmem_lookup_region(const char *region_name, ubsmem_region_desc_t *region_desc);
```

**参数说明**

|参数名|数据类型|参数类型|描述|
|--|--|--|--|
|region_name|const char *|入参|共享域名称，节点内唯一标识。最大有效长度为47字符（不包括“\0”）。|
|region_desc|ubsmem_region_desc_t *|出参|共享域信息，如域内节点数、对应节点hostname以及亲和性。|

相关结构体类型和常量定义如下：

```C++
#define MAX_REGION_NAME_DESC_LENGTH 48

typedef struct {
    char region_name[MAX_REGION_NAME_DESC_LENGTH];
    size_t size;
    ubsmem_region_attributes_t region_attr;
} ubsmem_region_desc_t;
```

**返回值**

|返回值|描述|
|--|--|
|0|操作成功。|
|非0|操作失败。具体错误码根据返回值不同参考[错误码](#错误码)。|

#### ubsmem\_destroy\_region

**接口功能**

销毁共享域。

>[!NOTE]说明
>
>- region仅在创建它的节点上有效，只能在该节点上进行删除操作，其他节点无法删除。
>- region删除后，无法通过该region创建共享内存或借用内存，但之前已创建的共享内存或借用内存仍可继续使用。

**接口格式**

```C++
int ubsmem_destroy_region(const char *region_name);
```

**参数说明**

|参数名|数据类型|参数类型|描述|
|--|--|--|--|
|region_name|const char *|入参|共享域名称，节点内唯一标识。最大有效长度为47字符（不包括“\0”）。|

**返回值**

|返回值|描述|
|--|--|
|0|操作成功。|
|非0|操作失败。具体错误码根据返回值不同参考[错误码](#错误码)。|

### 内存共享  

#### ubsmem\_shmem\_allocate

**接口功能**

创建共享内存对象。

>[!NOTE]说明
>
>- 当用户的进程绑定了指定的NUMA节点时，系统会从具有NUMA亲和性的节点申请共享内存。
>- 如果在指定的NUMA节点上申请内存失败，系统会尝试从其他节点申请内存。若希望通过绑定特定NUMA节点来提升性能，需及时判断内存是否成功从目标节点分配。

**接口格式**

```C++
int ubsmem_shmem_allocate(const char *region_name, const char *name, size_t size, mode_t mode, uint64_t flags);
```

**参数说明**

|参数名|数据类型|参数类型|描述|
|--|--|--|--|
|region_name|const char *|入参|内存域名称（默认域为“default”，包含与当前节点全互联的节点）。|
|name|const char *|入参|共享内存名称。全局唯一标识，最大有效长度为47字符（不包括“\0”），仅允许使用大小写字母、数字、“.”、“:”、“-”和“_”。|
|size|size_t|入参|共享内存size，最小4MB，需为4MB整数倍，单位为字节。<br>该参数的最小值与南向依赖UBSE中的配置项 [obmm.memory.block.size](https://atomgit.com/openeuler/ubs-engine/blob/master/docs/zh/ubse_configration_instructions.md) 有关。共享内存以FD的形式进行管理，每个FD管理 `obmm.memory.block.size` 大小的内存。共享内存在导出导入时会以该配置项向上取整对齐。<br>例如：该配置项为128MB，创建共享内存时传入size是4MB，实际上会创建出128MB的共享内存。|
|mode|mode_t|入参|访问权限，Unix文件权限位的 *rwx* 权限控制（ *x* 权限暂不支持，请忽略）。用于控制不同用户对该共享内存的访问权限，若无权限则映射失败。建议值： `S_IRUSR \| S_IWUSR` （即仅创建该共享内存的用户可以访问该共享内存）。|
|flags|uint64_t|入参|创建共享内存的标志信息。flag有效比特位含义请参见[表1 共享内存的flags](#table005)，各有效比特位组合关系参见[表2 共享内存的flags可组合关系](#table006)。|

**表 1 <a id="table005"></a>**  共享内存的flags

|flag|描述|说明|
|--|--|-|
|UBSM_FLAG_CACHE|表示是否支持cache模式。|该模式需要配合ubsmem_shmem_set_ownership接口对共享内存进行读写操作，同时，在BIOS的启动参数中不能配置snoop参数。|
|UBSM_FLAG_NONCACHE|表示是否开启NonCache模式。|该模式需要配合非接力，否则不保证数据一致性。|
|UBSM_FLAG_WR_DELAY_COMP|表示在提供共享内存时，开启非接力模式。|-|
|UBSM_FLAG_ONLY_IMPORT_NONCACHE|表示是否开启半NonCache模式，该模式下共享的内存导入方使用NonCache模式，导出方使用Cacheable模式。|该模式下，需要在BIOS的启动参数中配置snoop参数。|
|UBSM_FLAG_MEM_ANONYMOUS|带有该flag的共享内存，在一段时间内（最少5min）没有任何使用方时，UBSE会自动将其回收。|-|
|UBSM_FLAG_MMAP_HUGETLB_PMD|带有该flag的共享内存，映射时会以2MB大页为粒度进行。|-|

**表 2 <a id="table006"></a>**  共享内存的flags可组合关系

|flag|UBSM_FLAG_CACHE|UBSM_FLAG_NONCACHE|UBSM_FLAG_WR_DELAY_COMP|UBSM_FLAG_ONLY_IMPORT_NONCACHE|UBSM_FLAG_MEM_ANONYMOUS|UBSM_FLAG_MMAP_HUGETLB_PMD|
|--|--|--|--|--|--|--|
|**UBSM_FLAG_CACHE**|-|-|-|-|**√**|**√**|
|**UBSM_FLAG_NONCACHE**|-|-|**√**|-|**√**|**√**|
|**UBSM_FLAG_WR_DELAY_COMP**|-|**√**|-|**√**|**√**|**√**|
|**UBSM_FLAG_ONLY_IMPORT_NONCACHE**|-|-|**√**|-|**√**|**√**|
|**UBSM_FLAG_MEM_ANONYMOUS**|**√**|**√**|**√**|**√**|-|**√**|
|**UBSM_FLAG_MMAP_HUGETLB_PMD**|**√**|**√**|**√**|**√**|**√**|-|

**返回值**

|返回值|描述|
|--|--|
|0|操作成功。|
|非0|操作失败。具体错误码根据返回值不同参考[错误码](#错误码)。|

#### ubsmem\_shmem\_allocate\_with\_provider

**接口功能**

指定节点创建共享内存对象。

**接口格式**

```C++
int ubsmem_shmem_allocate_with_provider(const ubs_mem_provider_t *src_loc, const char *name, size_t size, mode_t mode, uint64_t flags);
```

**参数说明**

|参数名|数据类型|参数类型|描述|
|--|--|--|--|
|src_loc|const ubs_mem_provider_t *|入参|指定内存创建的节点信息，包括：<ul><li>host_name：节点名称，必填。</li><li>socket_id：指定内存导出的socket id，选填（UINT32_MAX）。</li><li>numa_id：指定内存导出的numa id，选填（UINT32_MAX）。</li><li>port_id：指定内存导出的port id，选填（UINT32_MAX）。</li></ul>|
|name|const char *|入参|共享内存名称。全局唯一标识，最大有效长度为47字符（不包括“\0”），仅允许使用大小写字母、数字、“.”、“:”、“-”和“_”。|
|size|size_t|入参|共享内存size，最小4MB，需为4MB整数倍，单位为字节。<br>该参数的最小值与南向依赖UBSE中的配置项 [obmm.memory.block.size](https://atomgit.com/openeuler/ubs-engine/blob/master/docs/zh/ubse_configration_instructions.md) 有关。共享内存以FD的形式进行管理，每个FD管理 `obmm.memory.block.size` 大小的内存。共享内存在导出导入时会以该配置项向上取整对齐。<br>例如：该配置项为128MB，创建共享内存时传入size是4MB，实际上会创建出128MB的共享内存。|
|mode|mode_t|入参|访问权限，Unix文件权限位的 *rwx* 权限控制（ *x* 权限暂不支持，忽略）。|
|flags|uint64_t|入参|创建共享内存的标志信息。flag有效比特位含义请参见[表1 共享内存的flags](#table005)，各有效比特位组合关系参见[表2 共享内存的flags可组合关系](#table006)。|

**返回值**

|返回值|描述|
|--|--|
|0|操作成功。|
|非0|操作失败。具体错误码根据返回值不同参考[错误码](#错误码)。|

#### ubsmem\_shmem\_deallocate

**接口功能**

删除共享内存对象。

**接口格式**

```C++
int ubsmem_shmem_deallocate(const char *name);
```

**参数说明**

|参数名|数据类型|参数类型|描述|
|--|--|--|--|
|name|const char *|入参|共享内存名称。全局唯一标识，最大有效长度为47字符（不包括“\0”）。|

**返回值**

|返回值|描述|
|--|--|
|0|操作成功。|
|非0|操作失败。具体错误码根据返回值不同参考[错误码](#错误码)。|

#### ubsmem\_shmem\_map

**接口功能**

映射共享内存对象到本进程地址空间。

>[!NOTE]说明
>
>- 同一节点可以映射的共享内存的个数上限为18432，该值为硬件限制下的理论上限，实际还会受限于物理拓扑、物理资源及分配策略。
>- 应用进程在每次映射操作中会占用一定数量的文件描述符（FD，File Descriptor），系统默认的FD上限为1024，因此在频繁映射场景下，存在触及FD限制而导致系统映射失败的风险，需要按需修改系统配置。

**接口格式**

```C++
int ubsmem_shmem_map(void *addr, size_t length, int prot, int flags, const char *name, off_t offset, void **local_ptr);
```

**参数说明**

|参数名|数据类型|参数类型|描述|
|--|--|--|--|
|addr|void *|入参|指定期望的地址。|
|length|size_t|入参|映射长度。当前仅支持整个共享内存映射，此值必须为共享内存size。|
|prot|int|入参|映射内存权限。当前支持的组合值包含：<ul><li>PROT_NONE</li><li>PROT_READ</li><li>PROT_READ\|PROT_WRITE</li></ul>|
|flags|int|入参|引用系统mmap的取值，可选如下参数：<ul><li>MAP_SHARED</li><li>MAP_FIXED</li><li>MAP_FIXED_NOREPLACE</li></ul>|
|name|const char *|入参|通过 ubsmem_shmem_allocate 创建的共享内存的名称。|
|offset|off_t|入参|映射的起始偏移。当前仅支持整个共享内存映射，此值必须为0。|
|local_ptr|void **|出参|映射成功时得到的本地地址。|

**返回值**

|返回值|描述|
|--|--|
|0|操作成功。|
|非0|操作失败。具体错误码根据返回值不同参考[错误码](#错误码)。|

#### ubsmem\_shmem\_unmap

**接口功能**

解除映射共享内存对象。

>[!NOTE]说明
>
>- 解除失败时，用户可以根据[错误码](#错误码)进行重试。
>- 解除失败后，该虚拟地址将禁止访问。

**接口格式**

```C++
int ubsmem_shmem_unmap(void *local_ptr, size_t length);
```

**参数说明**

|参数名|数据类型|参数类型|描述|
|--|--|--|--|
|local_ptr|void *|入参|由 ubsmem_shmem_map 得到的地址。当前仅支持整个共享内存映射，不能增加偏移量，此值必须严格等于 ubsmem_shmem_map 得到的地址。|
|length|size_t|入参|共享内存的大小。当前仅支持整个共享内存映射，不支持部分unmap。|

**返回值**

|返回值|描述|
|--|--|
|0|操作成功。|
|非0|操作失败。具体错误码根据返回值不同参考[错误码](#错误码)。|

#### ubsmem\_lookup\_cluster\_statistic

**接口功能**

查询集群节点内存信息。

**接口格式**

```C++
int ubsmem_lookup_cluster_statistic(ubsmem_cluster_info_t* info);
```

**参数说明**

|参数名|数据类型|参数类型|描述|
|--|--|--|--|
|info|ubsmem_cluster_info_t*|出参|集群的节点内存信息。相关结构体和常量定义如下。|

结构体和常量定义：

```C++
#define MAX_HOST_NUM 16
#define MAX_NUMA_NUM 32
#define MAX_NUMA_RESV_LEN 16
#define MAX_HOST_NAME_DESC_LENGTH 64

typedef struct {
  uint32_t slot_id;    // 节点唯一标识，采用slot id，与UBS Eengine一致
  uint32_t socket_id;
  uint32_t numa_id;    
  uint32_t mem_lend_ratio;  // 池化内存借出比例上限
  uint64_t mem_total;       // 节点内存总量，单位字节
  uint64_t mem_free;        // 内存空闲量，单位字节
  uint64_t mem_borrow;      // 借用的内存量，单位字节
  uint64_t mem_lend;        // 借出的内存量，单位字节
  uint8_t resv[MAX_NUMA_RESV_LEN];
} ubsmem_numa_mem_t;

typedef struct {
  char host_name[MAX_HOST_NAME_DESC_LENGTH];
  int numa_num;
  ubsmem_numa_mem_t numa[MAX_NUMA_NUM];
} ubsmem_host_info_t;

typedef struct {
  int host_num;   // Number of available nodes in the cluster
  ubsmem_host_info_t host[MAX_HOST_NUM];
} ubsmem_cluster_info_t;
```

**返回值**

|返回值|描述|
|--|--|
|0|操作成功。|
|非0|操作失败。具体错误码根据返回值不同参考[错误码](#错误码)。|

#### ubsmem\_shmem\_faults\_register

**接口功能**

订阅共享内存故障事件，当共享内存发生故障时，会调用注册的事件通知函数。

>[!NOTE]说明
>用户在调用该接口前，需加入ubse属组，并具备ubse的mem.shm类接口权限，具体配置请参见[UBSE 配置说明](https://atomgit.com/openeuler/ubs-engine/blob/master/docs/zh/ubse_configration_instructions.md)文档。

**接口格式**

```C++
int ubsmem_shmem_faults_register(shmem_faults_func registerFunc);
```

**参数说明**

|参数名|数据类型|参数类型|描述|
|--|--|--|--|
|registerFunc|shmem_faults_func|入参|共享内存故障事件响应处理函数，类型定义及故障类型枚举定义如下：|

相关类型定义：

```C++
typedef int32_t (*shmem_faults_func)(const char *shm_name, ubsmem_fault_type_t fault_type);
```

ubsmem_fault_type_t枚举定义：

```C++
typedef enum {
    UBMEM_ATOMIC_DATA_ERR = 0,
    UBMEM_READ_DATA_ERR,
    UBMEM_FLOW_POISON,
    UBMEM_FLOW_READ_AUTH_POISON,
    UBMEM_FLOW_READ_AUTH_RESPERR,
    UBMEM_TIMEOUT_POISON,
    UBMEM_TIMEOUT_RESPERR,
    UBMEM_READ_DATA_POISON,
    UBMEM_READ_DATA_RESPERR,
    UBMEM_MAR_NOPORT_VLD_INT_ERR,
    UBMEM_MAR_FLUX_INT_ERR,
    UBMEM_MAR_WITHOUT_CXT_ERR,
    UBMEM_RSP_BKPRE_OVER_TIMEOUT_ERR,
    UBMEM_MAR_NEAR_AUTH_FAIL_ERR,
    UBMEM_MAR_FAR_AUTH_FAIL_ERR,
    UBMEM_MAR_TIMEOUT_ERR,
    UBMEM_MAR_ILLEGAL_ACCESS_ERR,
    UBMEM_REMOTE_READ_DATA_ERR_OR_WRITE_RESPONSE_ERR,
    UBMEM_HEALTHY = 1000,
} ubsmem_fault_type_t;
```

**返回值**

|返回值|描述|
|--|--|
|0|操作成功。|
|非0|操作失败。具体错误码根据返回值不同参考[错误码](#错误码)。|

#### ubsmem\_local\_nid\_query

**接口功能**

查询该节点在超节点域中的节点ID。

>[!NOTE]说明
>用户在调用该接口前，需加入ubse属组，并具备ubse的topo类接口权限，具体配置请参见[UBSE 配置说明](https://atomgit.com/openeuler/ubs-engine/blob/master/docs/zh/ubse_configration_instructions.md)文档。

**接口格式**

```C++
int ubsmem_local_nid_query(uint32_t* nid);
```

**参数说明**

|参数名|数据类型|参数类型|描述|
|--|--|--|--|
|nid|uint32_t*|出参|该节点在超节点域中的节点ID。|

**返回值**

|返回值|描述|
|--|--|
|0|操作成功。|
|非0|操作失败。具体错误码根据返回值不同参考[错误码](#错误码)。|

#### ubsmem\_shmem\_set\_ownership

**接口功能**

修改共享内存在本地的权限信息，同时刷新cache。

>[!NOTE]说明
>
>- 只有已经映射到本地的共享内存才可以调用此接口。
>- 使用NonCache模式的时候，不支持调用修改内存权限接口。

**接口格式**

```C++
int ubsmem_shmem_set_ownership(const char *name, void *start, size_t length, int prot);
```

**参数说明**

|参数名|数据类型|参数类型|描述|
|--|--|--|--|
|name|const char *|入参|通过 ubsmem_shmem_allocate 创建的共享内存的名称。|
|start|void *|入参|由 ubsmem_shmem_map 得到的地址。支持刷新某一段地址的共享内存。|
|length|size_t|入参|共享内存的大小。最小值为内核页的大小，且内存地址需与mmap分配的起始地址保持内核页的大小对齐。|
|prot|int|入参|内存权限。当前支持的组合值包含：<ul><li>PROT_NONE</li><li>PROT_READ</li><li>PROT_READ\|PROT_WRITE</li></ul>|

**返回值**

|返回值|描述|
|--|--|
|0|操作成功。|
|非0|操作失败。具体错误码根据返回值不同参考[错误码](#错误码)。|

#### ubsmem\_shmem\_lookup

**接口功能**

根据名称查询指定共享的信息。

**接口格式**

```C++
int ubsmem_shmem_lookup(const char *name, ubsmem_shmem_info_t *shm_info);
```

**参数说明**

|参数名|数据类型|参数类型|描述|
|--|--|--|--|
|name|const char*|入参|查询的共享名。最大有效长度为47字符（不包括“\0”），仅允许使用大小写字母、数字、“-”和“_”。|
|shm_info|ubsmem_shmem_info_t *|出参|对应共享的信息。相关结构体和常量定义如下：<pre class="screen">#define MAX_SHM_NAME_LENGTH 48<br>#define MAX_MEMID_NUM 2048<br><br>typedef struct {<br>    char name[MAX_SHM_NAME_LENGTH + 1];<br>    size_t size;<br>    uint32_t mem_num;<br>    uint64_t mem_unit_size;<br>    uint64_t mem_id_list[MAX_MEMID_NUM];<br>} ubsmem_shmem_info_t;</pre>|

**返回值**

|返回值|描述|
|--|--|
|0|操作成功。|
|非0|操作失败。具体错误码根据返回值不同参考[错误码](#错误码)。|

#### ubsmem\_shmem\_list\_lookup

**接口功能**

查询指定前缀的共享内存信息。

**接口格式**

```C++
int ubsmem_shmem_list_lookup(const char *prefix, ubsmem_shmem_desc_t shm_list[], uint32_t *shm_cnt);
```

**参数说明**

|参数名|数据类型|参数类型|描述|
|--|--|--|--|
|prefix|const char *|入参|查询的共享前缀名。|
|shm_list|ubsmem_shmem_desc_t []|出参|查到的共享信息，调用方需提前准备一个ubsmem_shmem_desc_t类型的数组。相关结构体和常量定义如下：<pre class="screen">#define MAX_SHM_NAME_LENGTH 48<br><br>typedef struct {<br>char name[MAX_SHM_NAME_LENGTH + 1];<br>size_t size;<br>} ubsmem_shmem_desc_t;</pre>|
|shm_cnt|uint32_t *|出/入参|<ul><li>作为入参，表示shm_list数组的大小。</li><li>作为出参，表示查到的共享内存个数。</li></ul>|

**返回值**

|返回值|描述|
|--|--|
|0|操作成功。|
|非0|操作失败。具体错误码根据返回值不同参考[错误码](#错误码)。|

### 内存借用

#### ubsmem\_lease\_malloc

**接口功能**

申请内存。

>[!NOTE]说明
>同一节点可以借用内存的个数上限为16384。该值为硬件限制下的理论上限，实际还会受限于物理拓扑、物理资源及分配策略。

**接口格式**

```C++
int ubsmem_lease_malloc(const char *region_name, size_t size, ubsmem_distance_t mem_distance, uint64_t flags, void **local_ptr);
```

**参数说明**

|参数名|数据类型|参数类型|描述|
|--|--|--|--|
|region_name|const char *|入参|内存域名称（默认域为“default”，包含与当前节点全互联的节点）。节点内唯一标识，最大有效长度为47字符（不包括“\0”），仅允许使用大小写字母、数字、“-”和“_”。|
|size|size_t|入参|申请的内存size。最小4MB，需为4MB整数倍，单位为字节。<br>该参数的最小值与南向依赖UBSE中的配置项 [obmm.memory.block.size](https://atomgit.com/openeuler/ubs-engine/blob/master/docs/zh/ubse_configration_instructions.md) 有关。共享内存以FD的形式进行管理，每个FD管理 `obmm.memory.block.size` 大小的内存。共享内存在导出导入时会以该配置项向上取整对齐。<br>例如：该配置项为128MB，创建共享内存时传入size是4MB，实际上会创建出128MB的共享内存。|
|mem_distance|ubsmem_distance_t|入参|节点连接模式，当前仅支持直连借用，即 `DISTANCE_DIRECT_NODE` 取值为0。|
|flags|uint64_t|入参|借用的标志信息。如果为0，默认进行FD借用。当前支持以下flags：<ul><li>UBSM_FLAG_MMAP_HUGETLB_PMD：表示以2MB大页粒度进行映射。</li><li>UBSM_FLAG_MALLOC_WITH_NUMA：表示借用以远端NUMA呈现。与UBSM_FLAG_MMAP_HUGETLB_PMD不可同时指定，设置该flag借用最小值为128MB。</li></ul>|
|local_ptr|void **|出参|申请后得到的本地地址。|

**返回值**

|返回值|描述|
|--|--|
|0|操作成功。|
|非0|操作失败。具体错误码根据返回值不同参考[错误码](#错误码)。|

#### ubsmem\_lease\_malloc\_with\_location

**接口功能**

申请指定节点的内存。

**接口格式**

```C++
int ubsmem_lease_malloc_with_location(const ubs_mem_location_t *src_loc, size_t size, uint64_t flags, void **local_ptr);
```

**参数说明**

|参数名|数据类型|参数类型|描述|
|--|--|--|--|
|src_loc|const ubs_mem_location_t *|入参|指定借出内存节点的信息，包括slot_id、socket_id、numa_id和port_id。|
|size|size_t|入参|申请的内存size。最小4MB，需为4MB整数倍，单位为字节。<br>该参数的最小值与南向依赖UBSE中的配置项 [obmm.memory.block.size](https://atomgit.com/openeuler/ubs-engine/blob/master/docs/zh/ubse_configration_instructions.md) 有关。共享内存以FD的形式进行管理，每个FD管理 `obmm.memory.block.size` 大小的内存。共享内存在导出导入时会以该配置项向上取整对齐。<br>例如：该配置项为128MB，创建共享内存时传入size是4MB，实际上会创建出128MB的共享内存。|
|flags|uint64_t|入参|借用的标志信息。如果为0，默认进行FD借用。当前支持以下flags：<ul><li>UBSM_FLAG_MMAP_HUGETLB_PMD：表示以2MB大页粒度进行映射。</li><li>UBSM_FLAG_MALLOC_WITH_NUMA：表示借用以远端NUMA呈现。与UBSM_FLAG_MMAP_HUGETLB_PMD不可同时指定，设置该flag借用最小值为128MB。</li></ul>|
|local_ptr|void **|出参|申请后得到的本地地址。|

**返回值**

|返回值|描述|
|--|--|
|0|操作成功。|
|非0|操作失败。具体错误码根据返回值不同参考[错误码](#错误码)。|

#### ubsmem\_lease\_free

**接口功能**

释放通过ubsmem\_lease\_malloc、ubsmem\_lease\_malloc\_with\_location申请的内存。

**接口格式**

```C++
int ubsmem_lease_free(void *local_ptr);
```

**参数说明**

|参数名|数据类型|参数类型|描述|
|--|--|--|--|
|local_ptr|void *|入参|ubsmem_lease_malloc申请的内存指针。|

>[!NOTE]说明
>
>- 若传入的参数不是通过 ubsmem\_lease\_malloc 申请的内存（即为无效内存地址），系统将检测并打印告警日志，避免引发系统异常。
>- 若传入的参数是通过 ubsmem\_lease\_malloc 申请的内存，无论内存是否成功释放，该虚拟地址都将无法访问。
>- 释放该段内存时，UBSM不会主动清除其中的数据，需用户自行处理以确保安全性。
>- 若传入NULL，则忽略，并返回相应错误码。

**返回值**

|返回值|描述|
|--|--|
|0|操作成功。|
|非0|操作失败。具体错误码根据返回值不同参考[错误码](#错误码)。|

## 使用示例

典型代码示例如下所示。

### 初始化

```C++
#include <iostream>
#include "ubs_mem_def.h"
#include "ubs_mem.h"


int ubs_mem_initialize_demo()
{
    /* Initialize ubs-mem library. */
    ubsmem_options_t ubsm_shmem_opts;
    int ret = ubsmem_init_attributes(&ubsm_shmem_opts);
    if (ret != UBSM_OK) {
        std::cerr << "Failed to initialize attributes. ret: " << ret << std::endl;
        return -1;
    }
    ret = ubsmem_initialize(&ubsm_shmem_opts);
    if (ret != UBSM_OK) {
        std::cerr << "Failed to initialize ubs-mem. ret: " << ret << std::endl;
        return -1;
    }
    /* Initialize ubs-mem library succeeded. */


    /* Do your work here... */


    /* finalize ubs-mem library. */
    ret = ubsmem_finalize();
    if (ret != UBSM_OK) {
        std::cerr << "Failed to finalize  ubs-mem. ret: " << ret << std::endl;
        return -1;
    }
    return 0;
}
```

### 内存借用

大数据和数据库场景使用本接口借入远端内存，对远端内存进行读写。

```C++
#include <iostream>
#include "ubs_mem_def.h"
#include "ubs_mem.h"

int ubs_mem_lease_demo()
{
    /* Initialize ubs-mem library... */

    /* Allocate 4M memory from other host in the default region. */
    std::string region_name = "default";
    size_t size = 0x400000UL;
    ubsmem_distance_t mem_distance = DISTANCE_DIRECT_NODE;
    uint64_t flags = 0;
    void *addr = nullptr;
    auto ret = ubsmem_lease_malloc(region_name.c_str(), size, mem_distance, flags, &addr);
    if (ret != UBSM_OK) {
        std::cerr << "Failed to allocate remote memory. ret: " << ret << std::endl;
        return -1;
    }
    /* Allocate memory succeeded. */

    /* Do your work here... */

    /* Free memory. */
    ret = ubsmem_lease_free(addr);
    if (ret != UBSM_OK) {
        std::cerr << "Failed to free remote memory. ret: " << ret << std::endl;
        return -1;
    }
    return 0;
}
```

### 创建共享内存

大数据和数据库场景使用共享借用一写多读的功能，在多个节点共享数据。

```C++
#include <iostream>
#include "ubs_mem_def.h"
#include "ubs_mem.h"
#include <cstring>

int ubs_mem_create_share_memory_demo()
{
    /* Initialize ubs-mem library... */

    /* Construct region attributes. */
    ubsmem_region_attributes_t attr{};
    attr.host_num = 2;
    (void)strncpy(attr.hosts[0].host_name, "host01", MAX_HOST_NAME_DESC_LENGTH);
    attr.hosts[0].affinity = true;
    (void)strncpy(attr.hosts[1].host_name, "host02", MAX_HOST_NAME_DESC_LENGTH);
    attr.hosts[1].affinity = true;

    /* Create a region. */
    std::string region_name = "region";
    auto ret = ubsmem_create_region(region_name.c_str(), 0, &attr);
    if (ret != UBSM_OK) {
        std::cerr << "Failed to create region. ret: " << ret << std::endl;
        return -1;
    }

    /* Create a 4M shared memory. */
    std::string shared_name = "shared_mem";
    size_t size = 0x400000UL;
    uint64_t flags = UBSM_FLAG_ONLY_IMPORT_NONCACHE | UBSM_FLAG_WR_DELAY_COMP;
    ret = ubsmem_shmem_allocate(region_name.c_str(), shared_name.c_str(), size, 0600, flags);
    if (ret != UBSM_OK) {
        std::cerr << "Failed to create shared memory. ret: " << ret << std::endl;
        return -1;
    }
    /* Create the shared memory succeeded. */

    /* Do your work here... */

    /* Delete the shared memory. */
    ret = ubsmem_shmem_deallocate(shared_name.c_str());
    if (ret != UBSM_OK) {
        std::cerr << "Failed to deallocate shared memory. ret: " << ret << std::endl;
        return -1;
    }
    return 0;
}
```

### 映射共享内存

```C++
#include <iostream>
#include "ubs_mem_def.h"
#include "ubs_mem.h"
#include <sys/mman.h>

int ubs_mem_share_memory_map_demo()
{
    /* Create shared memory... */

    /* Map a shared memory. */
    std::string shared_name = "shared_mem";
    size_t length = 0x400000UL;
    void *address = nullptr;
    auto ret = ubsmem_shmem_map(nullptr, length, PROT_WRITE | PROT_READ, MAP_SHARED, shared_name.c_str(), 0, &address);
    if (ret != UBSM_OK) {
        std::cerr << "Failed to map shared memory. ret: " << ret << std::endl;
        return -1;
    }
    /* Map the shared memory succeeded. */

    /* Do your work here... */

    /* Flush and invalidate the data cache after accessing shared memory to ensure coherency. */
    /* Note: Cache flush/invalidate is invalid for non-cacheable shared memory — skip this step. */
    ret = ubsmem_shmem_set_ownership(shared_name.c_str(), address, length, PROT_NONE);
    if (ret != UBSM_OK) {
        std::cerr << "Failed to set shared memory. ret: " << ret << std::endl;
        return -1;
    }

    /* Unmap the shared memory. */
    ret = ubsmem_shmem_unmap(address, length);
    if (ret != UBSM_OK) {
        std::cerr << "Failed to unmap shared memory. ret: " << ret << std::endl;
        return -1;
    }
    return 0;
}
```

## 错误码

|错误码|标识符|含义|
|--|--|--|
|0|UBSM_OK|成功。|
|6010|UBSM_ERR_PARAM_INVALID|传入参数无效。|
|6011|UBSM_ERR_NOPERM|没有对应权限。|
|6012|UBSM_ERR_MEMORY|内存操作失败，如拷贝等。|
|6013|UBSM_ERR_UNIMPL|功能未实现。|
|6014|UBSM_CHECK_RESOURCE_ERROR|资源检查失败。|
|6015|UBSM_ERR_MEMLIB|MEM LIB失败。|
|6016|UBSM_ERR_NO_NEEDED|默认共享域，无需创建。|
|6017|UBSM_ERR_BUSY|资源忙碌。|
|6020|UBSM_ERR_NOT_FOUND|资源不存在。|
|6021|UBSM_ERR_ALREADY_EXIST|资源已存在。|
|6022|UBSM_ERR_MALLOC_FAIL|申请内存失败。|
|6023|UBSM_ERR_RECORD|资源记录失败。|
|6024|UBSM_ERR_IN_USING|共享正在被使用，不能删除。|
|6025|UBSM_ERR_NOT_SUPPORTED|操作不支持。|
|6040|UBSM_ERR_NET|网络错误。|
|6050|UBSM_ERR_UBSE|UBSE接口报错。|
|6051|UBSM_ERR_OBMM|OBMM接口报错。|
|6060|UBSM_ERR_LOCK_NOT_SUPPORTED|锁操作不支持。|
|6061|UBSM_ERR_LOCK_ALREADY_LOCKED|锁已被锁定。|
|6062|UBSM_ERR_DLOCK|dlock内部错误。|
|6099|UBSM_ERR_BUFF|未知错误。|
