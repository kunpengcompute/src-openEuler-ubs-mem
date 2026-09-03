/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2026. All rights reserved.

 * ubs-mem is licensed under the Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *      http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 */
#ifndef HYPERIO_COMMON_SERVICE_API_H
#define HYPERIO_COMMON_SERVICE_API_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 创建服务对象，并返回地址
 *
 * @return 服务对象的地址，失败时返回 NULL
 */
void *Create(void);

/**
 * @brief 进程向服务传递参数
 *
 * @param service 服务对象的地址
 * @param argc 参数个数
 * @param argv 参娄列表
 *
 * @return 0表示成功，非0表示失败
 */
int ServiceProcessArgs(void *service, int argc, char *argv[]);

/**
 * @brief 初始化此服务
 *
 * @param service 服务对象的地址
 *
 * @return 0表示成功，非0表示失败
 */
int ServiceInitialize(void *service);

/**
 * @brief 启动此服务
 *
 * @param service 服务对象的地址
 *
 * @return 0表示成功，非0表示失败
 */
int ServiceStart(void *service);

/**
 * @brief 判断服务是否运行正常
 * @param service 服务对象的地址
 * @return 0表示正常，非0表示异常
 */
int ServiceHealthy(void *service);

/**
 * @brief 停止此服务
 *
 * @param service 服务对象的地址
 *
 * @return 0表示成功，非0表示失败
 */
int ServiceShutdown(void *service);

/**
 * @brief 反初始化服务对象，释放相关资源
 *
 * @param service 服务对象的地址
 *
 * @return 0表示成功，非0表示失败
 */
int ServiceUninitialize(void *service);

/**
 * @brief 销毁服务对象，调用此函数后，对象不再被使用
 * @param service 服务对象的地址
 */
void Destroy(void *service);

#ifdef __cplusplus
}
#endif

#endif // HYPERIO_COMMON_SERVICE_API_H
