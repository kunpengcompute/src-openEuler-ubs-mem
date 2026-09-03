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
#ifndef OCK_COMMON_COMMON_UTIL_CONF_CONSTANTS_H
#define OCK_COMMON_COMMON_UTIL_CONF_CONSTANTS_H
#include <string>
namespace ock {
namespace common {
namespace ConfConstant {
// add configuration here with default values

const auto MXMD_SERVER_LOG_PATH = std::make_pair("ubsm.server.log.path", "/var/log/ubsm/");
const auto MXMD_SERVER_LOG_LEVEL = std::make_pair("ubsm.server.log.level", "INFO");
const auto MXMD_SEVER_PERFORMANCE_STATISTICS_ENABLE = std::make_pair("ubsm.performance.statistics.enable", "off");
const auto MXMD_SERVER_LOG_ROTATION_FILE_SIZE = std::make_pair("ubsm.server.log.rotation.file.size", 20);
const auto MXMD_SERVER_LOG_ROTATION_FILE_COUNT = std::make_pair("ubsm.server.log.rotation.file.count", 10);
const auto MXMD_SERVER_AUDIT_ENABLE = std::make_pair("ubsm.server.audit.enable", "on");
const auto MXMD_SERVER_AUDIT_LOG_PATH = std::make_pair("ubsm.server.audit.log.path", "/var/log/ubsm/");
const auto MXMD_SERVER_AUDIT_LOG_ROTATION_FILE_SIZE = std::make_pair("ubsm.server.audit.log.rotation.file.size", 20);
const auto MXMD_SERVER_AUDIT_LOG_ROTATION_FILE_COUNT = std::make_pair("ubsm.server.audit.log.rotation.file.count", 10);

const auto MXMD_FEATURES_ENABLE = std::make_pair("mxmd.features.enable", "ubsmd");
const auto MXMD_DAEMON_BINPATH = std::make_pair("mxmd.daemon.binpath", "-binpath=");
const auto MXMD_LOCK_DEV_NAME = std::make_pair("ubsm.lock.dev.name", "bonding_dev_0");
const auto MXMD_LOCK_DEV_EID = std::make_pair("ubsm.lock.dev.eid", "0000:0000:0000:1000:0010:0000:df00:0404");
const auto MXMD_LOCK_EXPIRE_TIME = std::make_pair("ubsm.lock.expire.time", 300);
const auto MXMD_IS_LOCK_ENABLE = std::make_pair("ubsm.lock.enable", "on");
const auto MXMD_IS_LOCK_UB_TOKEN_ENABLE = std::make_pair("ubsm.lock.ub_token.enable", "on");
const auto MXMD_IS_LOCK_TLS_ENABLE = std::make_pair("ubsm.lock.tls.enable", "on");
const auto UBSMD_LOCK_TLS_CA_PATH = std::make_pair("ubsm.lock.tls.ca.path", "/path/cacert.pem");
const auto UBSMD_LOCK_TLS_CRL_PATH = std::make_pair("ubsm.lock.tls.crl.path", "");
const auto UBSMD_LOCK_TLS_CERT_PATH = std::make_pair("ubsm.lock.tls.cert.path", "/path/cert.pem");
const auto UBSMD_LOCK_TLS_KEY_PATH = std::make_pair("ubsm.lock.tls.key.path", "/path/key.pem");
const auto UBSMD_LOCK_TLS_KEYPASS_PATH = std::make_pair("ubsm.lock.tls.keypass.path", "/path/keypass.txt");
const auto MXMD_DISCOVERY_ELECTION_TIMEOUT = std::make_pair("ubsm.discovery.election.timeout", 1000);
const auto MXMD_DISCOVERY_MIN_NODES = std::make_pair("ubsm.discovery.min.nodes", 0);

const auto UBSMD_MAX_HCOM_CONNECT_NUM = std::make_pair("ubsm.hcom.max.connect.num", 128);

const auto MXMD_SERVER_RPC_LOCAL_IPSEG = std::make_pair("ubsm.server.rpc.local.ipseg", "127.0.0.1:7201");
const auto MXMD_SERVER_RPC_REMOTE_IPSEG = std::make_pair("ubsm.server.rpc.remote.ipseg", "127.0.0.1:7301");

const auto UBSMD_SERVER_TLS_ENABLE = std::make_pair("ubsm.server.tls.enable", "on");
const auto UBSMD_SERVER_TLS_CIPHERSUITS = std::make_pair("ubsm.server.tls.ciphersuits", "aes_gcm_128");
const auto UBSMD_SERVER_TLS_CA_PATH = std::make_pair("ubsm.server.tls.ca.path", "/path/cacert.pem");
const auto UBSMD_SERVER_TLS_CRL_PATH = std::make_pair("ubsm.server.tls.crl.path", "");
const auto UBSMD_SERVER_TLS_CERT_PATH = std::make_pair("ubsm.server.tls.cert.path", "/path/cert.pem");
const auto UBSMD_SERVER_TLS_KEY_PATH = std::make_pair("ubsm.server.tls.key.path", "/path/key.pem");
const auto UBSMD_SERVER_TLS_KEYPASS_PATH = std::make_pair("ubsm.server.tls.keypass.path", "/path/keypass.txt");

const auto MXMD_SERVER_LEASE_CACHE_ENABLE = std::make_pair("ubsm.server.lease.cache.enable", "on");

constexpr int MIN_LOG_FILE_SIZE = 2;   // MB
constexpr int MAX_LOG_FILE_SIZE = 100; // MB
constexpr int MIN_LOG_FILE_COUNT = 1;
constexpr int MAX_LOG_FILE_COUNT = 50;
constexpr int MIN_AUDIT_LOG_FILE_SIZE = 2;   // MB
constexpr int MAX_AUDIT_LOG_FILE_SIZE = 100; // MB
constexpr int MIN_AUDIT_LOG_FILE_COUNT = 1;
constexpr int MAX_AUDIT_LOG_FILE_COUNT = 50;
constexpr int MAX_HCOM_CONNECT_NUM = 256;
constexpr int MIN_HCOM_CONNECT_NUM = 0;
constexpr int MIN_DISCOVERY_ELECTION_TIMEOUT = 0;
constexpr int MAX_DISCOVERY_ELECTION_TIMEOUT = 2000;
constexpr int MIN_DISCOVERY_MIN_NODES = 0;
constexpr int MAX_DISCOVERY_MIN_NODES = 30;
constexpr int MIN_LOCK_EXPIRE_TIME = 30;
constexpr int MAX_LOCK_EXPIRE_TIME = 86400;

} // namespace ConfConstant
} // namespace common
} // namespace ock
#endif