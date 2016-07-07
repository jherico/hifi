//
//  Created by Bradley Austin Davis on 2016/07/01
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_shared_smartmemorypool
#define hifi_shared_smartmemorypool

#include <assert.h>
#include <stdint.h>

#include <algorithm>
#include <unordered_map>

class SmartMemoryPoolTests;

namespace hifi {
    namespace memory {
        using Handle = uint64_t;

        static const size_t INVALID_SIZE = ~(size_t)0;
        static const size_t INVALID_OFFSET = ~(size_t)0;
        static const Handle INVALID_HANDLE = 0;

        // A structure representing a block of memory inside a larger allocation
        // It could be free memory or consumed memory
        struct MemoryBlock {
            size_t offset;
            size_t size;

            inline size_t end() const {
                return size + offset;
            }

            bool operator<(const MemoryBlock& o) {
                return offset < o.offset;
            }
        };

        using MemoryBlockVector = std::vector<MemoryBlock>;

        // A map of memory blocks by handles
        using HandleMap = std::unordered_map<Handle, MemoryBlock>;

        inline bool operator<(const MemoryBlock& a, const MemoryBlock& b) {
            return a.offset < b.offset;
        }

        class Provider {
        protected:
            size_t _size { 0 };
        public:
            static bool overlap(size_t size, size_t offsetA, size_t offsetB) {
                if (offsetA > offsetB) {
                    std::swap(offsetA, offsetB);
                }
                return offsetA + size > offsetB;
            }

            const size_t& size() const { return _size; }
            virtual ~Provider() {}
            virtual size_t reallocate(size_t newSize) = 0;
            virtual void copy(size_t dstOffset, size_t srcOffset, size_t size) const = 0;
            virtual void update(size_t dstOffset, size_t size, const uint8_t* data) const = 0;
            virtual void compact(HandleMap& usedMemory, MemoryBlockVector& freeMemory) {
                // FIXME implement a compact routine and heuristics for when to use it
                // Possible triggers....
                //  * 51% or more unused memory in the pool
                //  * free block count exceeds used block count
                //  * free memory exceed fixed limit
            }
            bool validRange(size_t offset, size_t size) const {
                return size > 0 && size + offset <= _size;
            }
        };

        namespace impl {

            class Null : public Provider {
            public:
                size_t reallocate(size_t newSize) override {
                    _size = newSize;
                    return _size;
                }

                void copy(size_t dstOffset, size_t srcOffset, size_t size) const override {};
                void update(size_t dstOffset, size_t size, const uint8_t* data) const override {};
            };

            class System : public Provider {
                uint8_t* _data { nullptr };

            public:
                ~System() {
                    delete[] _data;
                    _data = nullptr;
                    _size = 0;
                }

                size_t reallocate(size_t newSize) override {
                    assert(newSize != 0);
                    size_t powerSize = 2;
                    while (powerSize < newSize) {
                        powerSize <<= 1;
                    }
                    newSize = powerSize;
                    uint8_t* newData = new uint8_t[newSize];
                    if (_data) {
                        memcpy(newData, _data, _size);
                    }
                    std::swap(newData, _data);
                    std::swap(newSize, _size);
                    delete[] newData;
                    return _size;
                }

                void copy(size_t dstOffset, size_t srcOffset, size_t size) const override {
                    assert(validRange(dstOffset, size));
                    assert(validRange(srcOffset, size));
                    assert(!overlap(size, dstOffset, srcOffset));
                    memcpy(_data + dstOffset, _data + srcOffset, size);
                }

                void update(size_t dstOffset, size_t size, const uint8_t* data) const override {
                    assert(validRange(dstOffset, size));
                    memcpy(_data + dstOffset, data, size);
                }
            };

        }

        // Helper class to manage blocks of free memory
        class FreeBlockList {
            friend class SmartMemoryPoolTests;
        public:
            static const size_t INVALID_BLOCK_INDEX = ~(size_t)0;

            using Block = MemoryBlock;
            using Index = size_t;

            // Return the number of blocks
            size_t count() const {
                return _blocks.size();
            }

            // Return the total free size
            size_t size() const {
                size_t result = 0;
                for (const auto& b : _blocks) {
                    result += b.size;
                }
                return result;
            }

            // Return the total free size
            size_t largestBlock() const {
                size_t result = 0;
                for (const auto& b : _blocks) {
                    result = std::max(result, b.size);
                }
                return result;
            }

            // Make sure a given block doesn't overlap with any existing blocks in the free list
            bool isConsumed(const Block& b) const {
                assert(isValid());
                if (_blocks.empty()) {
                    return true;
                }
                auto itr = std::upper_bound(_blocks.begin(), _blocks.end(), b);
                // If the found block is after the last block in the free list, it's consumed
                if (itr == _blocks.end()) {
                    return true;
                }
                const auto& next = *itr;
                auto end = b.end();
                // End of the specified block overlaps with the next block in the list
                if (end > next.offset) {
                    return false;
                }
                if (itr != _blocks.begin()) {
                    const auto& prev = *--itr;
                    auto prevEnd = prev.end();
                    // Block overlaps with the previous item in the list
                    if (prevEnd > b.offset) {
                        return false;
                    }
                }
                return true;
            }

            // Verify there are no 0 sized blocks or contiguous blocks in the list
            bool isClean() const {
                if (_blocks.empty()) {
                    return true;
                }

                // Clean is a subset of valid
                if (!isValid()) {
                    return false;
                }

                Block p = _blocks[0];
                for (size_t i = 1; i < _blocks.size(); ++i) {
                    const Block& b = _blocks[i];
                    // Empty blocks do not appear in a clean list
                    if (!b.size) {
                        return false;
                    }
                    // Contiguous blocks do not appear in a clean list
                    if (p.end() == b.offset) {
                        return false;
                    }
                    p = b;
                }
                return true;
            }

            // Verify blocks are ordered by their offset and that no blocks overlap
            bool isValid() const {
                if (_blocks.empty()) {
                    return true;
                }

                Block p = _blocks[0];
                for (size_t i = 1; i < _blocks.size(); ++i) {
                    const Block& b = _blocks[i];
                    // Offsets must be strictly ordered and unique
                    if (p.offset >= b.offset) {
                        return false;
                    }
                    // Overlapping blocks do not appear in a valid list
                    if (p.end() > b.offset) {
                        return false;
                    }
                    p = b;
                }
                return true;
            }

            // Find a block of the requested size, or return an invalid index 
            // if no such block is available
            Index find(size_t requiredSize) const {
                assert(isValid());
                size_t foundBlockSize = INVALID_SIZE;
                Index foundBlockIndex = INVALID_BLOCK_INDEX;
                for (Index i = 0; i < _blocks.size(); ++i) {
                    const auto& b = _blocks[i];
                    if (b.size < requiredSize) {
                        continue;
                    }

                    if ((INVALID_BLOCK_INDEX == foundBlockIndex) || (b.size < foundBlockSize)) {
                        foundBlockIndex = i;
                        foundBlockSize = b.size;
                    }
                }

                return foundBlockIndex;
            }

            // Eliminate any 0 sized blocks and make two blocks that are contiguous into a single block
            void cleanup() {
                if (!_blocks.size()) {
                    return;
                }

                assert(isValid());
                if (isClean()) {
                    return;
                }

                MemoryBlockVector newBlocks;
                newBlocks.reserve(_blocks.size());
                for (const auto& b : _blocks) {
                    if (!b.size) {
                        continue;
                    }
                    if (newBlocks.empty()) {
                        newBlocks.push_back(b);
                        continue;
                    }

                    auto& previous = newBlocks.back();
                    if (previous.end() == b.offset) {
                        previous.size += b.size;
                    } else {
                        newBlocks.push_back(b);
                    }
                }
                std::swap(newBlocks, _blocks);
            }

            // Release a block of memory for use
            void free(Block block) {
                assert(isValid());
                if (0 == block.size) {
                    return;
                }

                if (_blocks.empty()) {
                    // If the free block list is empty, we're done, just add the block
                    _blocks.push_back(block);
                } else {
                    auto& last = _blocks.back();
                    auto& first = _blocks.front();
                    // If the free block list is not empty, there are three possibilities...
                    // this block is in at the beginning, middle or end.
                    // For the beginning and end we need to check one other block to see if
                    // we find a contiguous segment and either extend it, or insert into the
                    // vector if we can't
                    if (block.offset < first.offset) {
                        // If contiguous, update the first block
                        if (block.end() == first.offset) {
                            first.offset = block.offset;
                            first.size += block.size;
                        } else {
                            // Ugh... O(N) insertion into vector
                            _blocks.insert(_blocks.begin(), block);
                        }
                    } else if (block.offset >= last.offset) {
                        if (last.end() == block.offset) {
                            last.size += block.size;
                        } else {
                            _blocks.push_back(block);
                        }
                    } else {
                        // For the middle we need to check two other blocks for continuity.
                        // If one or the other is continuous, we extend it.  If both are contiuous,
                        // we combine the new block and the two ends into one new block and shrink
                        // the array
                        MemoryBlockVector::iterator nextItr = std::upper_bound(_blocks.begin(), _blocks.end(), block);
                        assert(nextItr != _blocks.end());
                        auto& next = *nextItr;
                        auto& prev = *(nextItr - 1);
                        bool prevContiguous = prev.offset + prev.size == block.offset;
                        bool nextContiguous = block.offset + block.size == next.offset;
                        if (!prevContiguous && !nextContiguous) {
                            // Neither blocks contiguous, insert this block
                            _blocks.insert(nextItr, block);
                        } else if (prevContiguous && nextContiguous) {
                            // Both blocks contiguous, update prev and shrink the array
                            prev.size += block.size + next.size;
                            _blocks.erase(nextItr);
                        } else if (prevContiguous) {
                            // Previous block contiguous, update prev
                            prev.size += block.size;
                        } else if (nextContiguous) {
                            // Next block contiguous, update prev
                            next.offset = block.offset;
                            next.size += block.size;
                        } else {
                            assert(false);
                        }
                    }
                }
                assert(isValid());
            }
            void free(size_t size, size_t offset) {
                free(Block { offset, size });
            }

            // Mark (part of) a block as being in use, either reducing it's size to 0 or 
            // reducing it's size and increasing it's offset by the same amount
            void consume(Index index, size_t size) {
                assert(isValid());
                assert(index != INVALID_BLOCK_INDEX);
                assert(index < _blocks.size());
                auto& b = _blocks[index];
                assert(b.size >= size);
                b.size -= size;
                b.offset += size;
                assert(isValid());
            }

            // Determine the current offset of a specific block.
            size_t offset(Index index) const {
                assert(isValid());
                assert(index != INVALID_BLOCK_INDEX);
                assert(index < _blocks.size());
                return _blocks[index].offset;
            }

            MemoryBlockVector _blocks;
        private:
        };

        template <typename ProviderType>
        class SmartMemoryPool {
            friend class SmartMemoryPoolTests;

            using Block = MemoryBlock;

            // A pending allocation of data with a handle, size and pointer to the actual data 
            // in a host readable void*
            struct Allocation {
                Handle handle;
                size_t size;
                void* data;
            };
            using AllocationMap = std::unordered_map<Handle, Allocation>;

            AllocationMap _pendingAllocations;
            ProviderType _provider;
            FreeBlockList _freeBlocks;
            HandleMap _usedBlocks;
            Handle _nextHandle { 1 };
            bool _autoCommit { false };

            const MemoryBlock& usedBlock(Handle handle) const {
                assert(_usedBlocks.count(handle));
                auto itr = _usedBlocks.find(handle);
                return itr->second;
            }

        public:
            const ProviderType& getProvider() const { return _provider; }

            bool isValid() {
                return true;

                {
                    size_t s = size();
                    size_t f = freeSize();
                    size_t u = usedSize();
                    if (f > s) {
                        return false;
                    }
                    if (u > s) {
                        return false;
                    }
                    if (f + u != s) {
                        return false;
                    }
                }
                if (usedSize() != size() - freeSize()) {
                    return false;
                }
                if (!_freeBlocks.isValid()) {
                    return false;
                }
                for (const auto& e : _usedBlocks) {
                    if (!_freeBlocks.isConsumed(e.second)) {
                        return false;
                    }
                }
                return true;
            }

            bool isAutoCommit() const {
                return _autoCommit;
            }

            void setAutoCommit(bool autoCommit) {
                _autoCommit = autoCommit;
                if (_autoCommit) {
                    sync();
                }
            }

            Handle allocate(size_t size, void* data = nullptr) {
                Handle handle = _nextHandle++;
                if (!size) {
                    _usedBlocks[handle] = { 0, 0 };
                    return handle;
                }
                _pendingAllocations[handle] = { handle, size, data };
                if (_autoCommit) {
                    sync();
                }
                return handle;
            }

            void copy(Handle dst, size_t dstOffset, Handle src, size_t srcOffset, size_t size) const {
                if (!size) {
                    return;
                }
                const auto& dstBlock = usedBlock(dst);
                const auto& srcBlock = usedBlock(src);
                assert((size + dstOffset) <= dstBlock.size);
                assert((size + srcOffset) <= srcBlock.size);
                _provider.copy(dstOffset + dstBlock.offset, srcOffset + srcBlock.offset, size);
            }

            void update(Handle dst, size_t dstOffset, size_t size, const uint8_t* data) const {
                if (!size) {
                    return;
                }
                const auto& dstBlock = usedBlock(dst);
                assert((size + dstOffset) <= dstBlock.size);
                _provider.update(dstOffset + dstBlock.offset, size, data);
            }

            // Return the current offset of the specified memory handle.
            size_t offset(Handle handle) const {
                assert(_usedBlocks.count(handle) == 1 || _pendingAllocations.count(handle) == 1);
                if (_pendingAllocations.count(handle)) {
                    return INVALID_OFFSET;
                }
                auto itr = _usedBlocks.find(handle);
                return itr->second.offset;
            }

            void free(Handle handle) {
                assert(isValid());
                assert(_usedBlocks.count(handle) == 1 || _pendingAllocations.count(handle) == 1);
                if (_pendingAllocations.count(handle)) {
                    _pendingAllocations.erase(handle);
                    return;
                }

                assert(_usedBlocks.count(handle) == 1);
                auto block = _usedBlocks[handle];
                _freeBlocks.free(block);
                size_t erasedCount = _usedBlocks.erase(handle);
                assert(erasedCount);
                assert(isValid());
            }

            // Commit all the pending allocations
            void sync() {
                if (_pendingAllocations.empty()) {
                    return;
                }

                assert(isValid());

                using AllocationVector = std::vector<Allocation>;
                AllocationVector allocations, deferredAllocations;
                allocations.reserve(_pendingAllocations.size());
                deferredAllocations.reserve(allocations.size());
                for (const auto& p : _pendingAllocations) {
                    allocations.push_back(p.second);
                }
                _pendingAllocations.clear();

                std::sort(allocations.begin(), allocations.end(), [&](const Allocation& a, const Allocation& b) {
                    return a.size < b.size;
                });

                // Handle the allocations that fit into the free blocks
                {
                    // Brute force approch
                    for (const auto& a : allocations) {
                        size_t blockIndex = _freeBlocks.find(a.size);
                        if (blockIndex == FreeBlockList::INVALID_BLOCK_INDEX) {
                            deferredAllocations.push_back(a);
                            continue;
                        }
                        Block newBlock = {
                            _freeBlocks.offset(blockIndex),
                            a.size
                        };
                        if (a.data) {
                            _provider.update(a.size, newBlock.offset, (const uint8_t*)a.data);
                        }
                        _freeBlocks.consume(blockIndex, a.size);
                        assert(_freeBlocks.isConsumed(newBlock));
                        assert(!_usedBlocks.count(a.handle));
                        _usedBlocks[a.handle] = newBlock;
                        assert(isValid());
                    }
                }

                // Handle the allocations that didn't fit by re-allocating
                if (!deferredAllocations.empty()) {
                    size_t required = 0;
                    for (const auto& a : deferredAllocations) {
                        required += a.size;
                    }
                    size_t curSize = _provider.size();
                    size_t newSize = _provider.reallocate(curSize + required);
                    MemoryBlock allocationBlock { curSize, newSize - curSize };
                    for (const auto& a : deferredAllocations) {
                        MemoryBlock newBlock { allocationBlock.offset, a.size };
                        if (a.data) {
                            _provider.update(newBlock.size, newBlock.offset, (const uint8_t*)a.data);
                        }
                        allocationBlock.offset += newBlock.size;
                        allocationBlock.size -= newBlock.size;
                        _usedBlocks[a.handle] = newBlock;
                    }
                    _freeBlocks.free(allocationBlock);
                }
                assert(isValid());
                _freeBlocks.cleanup();
                _provider.compact(_usedBlocks, _freeBlocks._blocks);
            }

            size_t size() const {
                return _provider.size();
            }

            size_t freeSize() const {
                return _freeBlocks.size();
            }

            size_t usedSize() const {
                size_t usedSize = 0;
                for (const auto& e : _usedBlocks) {
                    usedSize += e.second.size;
                }
                return usedSize;
            }

            size_t freeBlocksCount() const {
                return _freeBlocks.count();
            }
        };

        using SystemMemoryPool = SmartMemoryPool<impl::System>;
    }
}


#endif
