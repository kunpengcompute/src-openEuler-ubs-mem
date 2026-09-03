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
#ifndef MXM_MSG_PACKER_H
#define MXM_MSG_PACKER_H

#include <map>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include "rpc_config.h"

namespace ock {
namespace mxmd {
constexpr size_t MAX_ALLOWED_SIZE = 1024 * 1024; // 1MB
class NetMsgPacker {
public:
    /**
     * @brief Append a POD(Plain Old Data) struct
     *
     * @tparam T           [in] type of POD
     * @param val          [in] value of POD
     */
    template <typename T>
    void Serialize(const T &val, typename std::enable_if<std::is_trivially_copyable<T>::value, int>::type = 0)
    {
        outStream_.write(reinterpret_cast<const char *>(&val), sizeof(T));
    }

    /**
     * @brief Append a string
     *
     * @param val          [in] value of string
     */
    void Serialize(const std::string &val)
    {
        const uint32_t size = val.size();
        outStream_.write(reinterpret_cast<const char *>(&size), sizeof(size));
        outStream_.write(val.data(), size);
    }

    void Serialize(const rpc::ClusterNode &val)
    {
        Serialize(val.id);
        Serialize(val.ip);
        Serialize(val.port);
        Serialize(val.isMaster);
        Serialize(val.lastSeen);
        Serialize(val.isActive);
        Serialize(val.type);
    }

    /**
     * @brief Append a pair
     *
     * @tparam K           [in] type of K of pair
     * @tparam V           [in] type of V of pair
     * @param val          [in] value of pair
     */
    template <typename K, typename V>
    void Serialize(const std::pair<K, V> &val)
    {
        Serialize(val.first);
        Serialize(val.second);
    }

    /**
     * @brief Append a vector
     *
     * @tparam V           [in] type of vector element
     * @param container    [in] vector to be appended
     */
    template <typename V>
    void Serialize(const std::vector<V> &container)
    {
        const std::size_t size = container.size();
        outStream_.write(reinterpret_cast<const char *>(&size), sizeof(size));
        for (const auto &item : container) {
            Serialize(item);
        }
    }

    /**
     * @brief Append a map
     *
     * @tparam K           [in] type of map key
     * @tparam V           [in] type of map value
     * @param container    [in] map to be appended
     */
    template <typename K, typename V>
    void Serialize(const std::map<K, V> &container)
    {
        const std::size_t size = container.size();
        outStream_.write(reinterpret_cast<const char *>(&size), sizeof(size));
        for (const auto &item : container) {
            Serialize(item);
        }
    }

    /**
     * @brief Get the serialized result
     *
     * @return String value of serialized
     */
    std::string String() const
    {
        return outStream_.str();
    }

private:
    std::ostringstream outStream_;
};

class NetMsgUnpacker {
public:
    /**
     * @brief Create unpacker with serialized data
     *
     * @param value        [in] serialized data with string type
     */
    explicit NetMsgUnpacker(const std::string &value) : inStream_(value) {}

    /**
     * @brief Take data and deserialize to POD data
     *
     * @tparam T           [in] type of POD
     * @param val          [in/out] result data of POD
     */
    template <typename T>
    void Deserialize(T &val, typename std::enable_if<std::is_trivially_copyable<T>::value, int>::type = 0)
    {
        inStream_.read(reinterpret_cast<char *>(&val), sizeof(T));
    }

    /**
     * @brief Take data and deserialize to string
     *
     * @param val          [in/out] result data of string
     */
    void Deserialize(std::string &val)
    {
        uint32_t size = 0;
        inStream_.read(reinterpret_cast<char *>(&size), sizeof(size));
        if (size > MAX_ALLOWED_SIZE) {
            DBG_LOGERROR("size(" << size << ") is over limited MAX_ALLOWED_SIZE(" << MAX_ALLOWED_SIZE << ").");
            return; // 超出范围，直接返回
        }
        val.resize(size);
        inStream_.read(&val[0], size);
    }

    void Deserialize(rpc::ClusterNode &val)
    {
        std::string id;
        std::string ip;
        uint16_t port;
        bool isMaster;
        long lastSeen;
        bool isActive;
        rpc::NodeType nodetype;

        Deserialize(id);
        Deserialize(ip);
        Deserialize(port);
        Deserialize(isMaster);
        Deserialize(lastSeen);
        Deserialize(isActive);
        Deserialize(nodetype);

        val.id = id;
        val.ip = ip;
        val.port = port;
        val.isMaster = isMaster;
        val.lastSeen = lastSeen;
        val.isActive = isActive;
        val.type = nodetype;
    }

    /**
     * @brief Take data and deserialize to vector
     *
     * @tparam V           [in] type of vector element
     * @param container    [in/out] result data of vector
     */
    template <typename V>
    void Deserialize(std::vector<V> &container)
    {
        size_t size = 0;
        inStream_.read(reinterpret_cast<char *>(&size), sizeof(size));
        if (size > MAX_ALLOWED_SIZE) {
            DBG_LOGERROR("size(" << size << ") is over limited MAX_ALLOWED_SIZE(" << MAX_ALLOWED_SIZE << ").");
            return; // 超出范围，直接返回
        }
        container.clear();
        container.reserve(size);
        for (size_t i = 0; i < size; ++i) {
            V item;
            Deserialize(item);
            container.emplace_back(std::move(item));
        }
    }

    /**
     * @brief Take data and deserialize to vector
     *
     * @tparam K           [in] type of map key
     * @tparam V           [in] type of map value
     * @param container    [in/out] result data of map
     */
    template <typename K, typename V>
    void Deserialize(std::map<K, V> &container)
    {
        size_t size = 0;
        inStream_.read(reinterpret_cast<char *>(&size), sizeof(size));
        if (size > MAX_ALLOWED_SIZE) {
            DBG_LOGERROR("size(" << size << ") is over limited MAX_ALLOWED_SIZE(" << MAX_ALLOWED_SIZE << ").");
            return; // 超出范围，直接返回
        }
        container.clear();
        for (size_t i = 0; i < size; ++i) {
            K key;
            V value;
            Deserialize(key);
            Deserialize(value);
            container.emplace(std::move(key), std::move(value));
        }
    }

private:
    std::istringstream inStream_;
};
} // namespace mxmd
} // namespace ock

#endif // MXMD_MSG_PACKER_H
