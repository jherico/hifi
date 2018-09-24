//
//  Created by Bradley Austin Davis on 2018/09/23
//  Copyright 2013-2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef GPU_MANAGER_H
#define GPU_MANAGER_H

#include <cstdint>

#include <atomic>
#include <exception>
#include <unordered_map>

#include "../ArrayProxy.h"
#include "../Buffer.h"

namespace gpu { namespace manager {

template <typename T>
class Manager {
public:
    struct Handle {
        using Type = uint64_t;
        static const Type INVALID = ~(Type)0;
        operator bool() const { return _handle != INVALID; }
    private:
        Type _handle{ INVALID };
    };

    static constexpr size_t ENTRY_SIZE = sizeof(T);

protected:
    Manager() {}

public:
    const Handle& acquire(size_t count = 1) {
        // try to find an existing entry
        {
            auto itr = _unused.begin();
            while (itr != _unused.end()) {
                if (unused.count >= count && unused <= result) {
                }
            }

            if (itr != _unused.end()) {
                Entry result = *itr;
                if (count < entry.count) {
                    itr->count -= count;
                    itr->offset += count;
                    result->count = count;
                }
                auto resultHandle = generateHandle();
                _entries[resultHandle] = result;
                return resultHandle;
            }
        }

        // No entry found, need to re-allocate
        throw std::runtime_error("not implemented yet");

        // If we didn't find anything, we need to reallocate
        if (!result) {
            _buffer.resize(_bufer.size() * 2)
        }
    }

    void update(Handle& handle, const ArrayProxy<const T>& t, size_t index = 0) {
        auto updateCount = t.size();
        // auto-allocate
        if (!handle) {
            handle = acquire(updateCount + index);
        }
        
        // auto-resize
        auto entry = getEntry(handle);
        if (entry.count < updateCount) {
            resize(handle, updateCount + index);
            entry = getEntry(handle);
        }

        // Find the buffer offset
        size_t destOffset = (entry.index + index) * sizeof(T);
        // Find the update size in bytes
        size_t destSize = updateCount * sizeof(T);
        _buffer->setSubData(destOffset, destSize, t.data());
    }

    void release(const Handle& handle) {
        auto newUnused = _entries.erase(handle);
        // FIXME find the offset
        _unused.push_back(newUnused);
    }

private:
    struct Entry {
        static const size_t INVALID = ~0UL;
        size_t count{ INVALID };
        size_t index{ INVALID };
        operator bool() const { return count != INVALID; }
        bool operator <(const Entry& other) { return count < other.count; }
    };

    const Entry& getEntry(Handle handle) const {
        auto itr = _entries.find(handle);
        assert(itr != entries.end())
        return itr->second;
    }

    size_t getOffset(Handle handle, size_t index = 0) {
        const auto& entry = getEntry(handle);
        return (entry.index + index) * sizeof(T);
    }
    const T* getData(Handle handle, size_t index) const {
        const auto& entry = getEntry(handle);
        auto result = &(_buffer.get());
        result += index + entry.index;
        return result;
    }

    typename Handle::Type generateHandle() const {
        return _nextHandle.incrment();
    }


    mutable std::atomic<typename Handle::Type> _nextHandle{ 1 };
    BufferPointer _buffer;
    std::unordered_map<Handle, Entry> _entries;
    std::list<Entry> _unused;
};


template <typename M, typename T>
class MutatingManager : public Manager<T> {
protected:
    virtual void transform(const M& source, T& dest) const = 0;

    void update(Handle handle, const ArrayProxy<const M>& values, size_t index = 0) {
        std::vector<M> transformed;
        auto count = values.size();
        transformed.resize(t.size());
        for (size_t i = 0; i < count; ++i) {
            transform(values.data()[i], transformed[i]);
        }
        update(handle, transformed, index);
    }
};


}}
#endif