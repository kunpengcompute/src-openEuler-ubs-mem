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
#ifndef OCK_COMMON_UTIL_REF_H
#define OCK_COMMON_UTIL_REF_H

#include <atomic>

namespace ock {
namespace common {
// base class of the smart pointer
class Referable {
public:
    Referable() : mRefCount(0) {}
    virtual ~Referable() {}

    inline long IncRefCount()
    {
        return ++mRefCount;
    }
    inline long DecRefCount()
    {
        return --mRefCount;
    }
    inline long GetRefCount() const
    {
        return mRefCount;
    }
    inline void SetRefCount(long count)
    {
        mRefCount = count;
    }

private:
    std::atomic<long> mRefCount;
};

// smart pointer class
template <class T>
class Ref {
public:
    Ref() : mObj(nullptr) {}

    Ref(const Ref<T> &other) : mObj(nullptr)
    {
        this->Set(other.mObj);
    }

    // fix: can't be explicit
    Ref(T *newObj) : mObj(newObj)
    {
        if (mObj != nullptr) {
            mObj->IncRefCount();
        }
    }

    virtual ~Ref()
    {
        this->Set(nullptr);
    }

    inline T *operator->() const
    {
        return mObj;
    }

    inline Ref<T> &operator=(const Ref<T> &other)
    {
        if (this != &other) {
            this->Set(other.mObj);
        }
        return *this;
    }

    inline Ref<T> &operator=(T *newObj)
    {
        this->Set(newObj);
        return *this;
    }

    inline bool operator==(const Ref<T> &other) const
    {
        return mObj == other.mObj;
    }

    inline bool operator==(T *other) const
    {
        return mObj == other;
    }

    inline bool operator<(const Ref<T> &other) const
    {
        return mObj < other.mObj;
    }

    inline bool operator<(T *other) const
    {
        return mObj < other;
    }

    inline bool operator!=(const Ref<T> &other) const
    {
        return mObj != other.mObj;
    }

    inline bool operator!=(T *other) const
    {
        return mObj != other;
    }

    // important: cannot assign to c++ origin pointer, only to be ref pointer object
    inline T *Get() const
    {
        return mObj;
    }

    inline void Set(T *newObj)
    {
        // do nothing if the new object is equal to the current one
        if (mObj == newObj) {
            return;
        }

        // increase the reference count of the new object
        if (newObj != nullptr) {
            newObj->IncRefCount();
        }

        // decrease the reference count the current object, if the reference count
        // equals 0, delete it
        if (mObj != nullptr && mObj->DecRefCount() == 0) {
            delete mObj;
        }

        // switch current object
        mObj = newObj;
    }

    // construction for std::move
    Ref(Ref<T> &&other) noexcept : mObj(nullptr)
    {
        this->Set(other.mObj);
        other.Set(nullptr);
    }

    Ref<T> &operator=(Ref<T> &&other)
    {
        if (this != &other) {
            this->Set(other.mObj);
            other.Set(nullptr);
        }
        return *this;
    }

private:
    T *mObj;
};
} // namespace common
} // namespace ock

#endif