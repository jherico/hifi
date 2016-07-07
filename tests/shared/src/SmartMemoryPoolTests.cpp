//
// TransformTests.cpp
// tests/shared/src
//
// Copyright 2013-2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <shared/SmartMemoryPool.h>

#include <QtCore/QObject>
#include <QtTest/QtTest>
#include <QRegularExpression>
#include <QDebug>
#include <iostream>

QStringList readLines(const QString& file) {
    QStringList result;
    QFile textFile(file);
    textFile.open(QIODevice::Text | QIODevice::ReadOnly);
    QTextStream textStream(&textFile);
    while (true) {
        QString line = textStream.readLine();
        if (line.isNull()) {
            break;
        }
        result.append(line);
    }
    return result;
}

struct Allocation {
    uint64_t id;
    size_t size;
    time_t begin;
    time_t end;

    bool operator <(const Allocation& o) const {
        return begin < o.begin;
    }

    time_t lifetime() const {
        return end - begin;
    }

    static QVector<Allocation> parseAllocations() {
        QStringList lines = readLines("c:/Users/bdavis/tracker.txt");
        qDebug() << "Found " << lines.size() << " allocation records";
        // [06/30 21:45:12] [DEBUG] GLTracker buffer:  0x225bd0a8850   4096
        const QRegularExpression re("\\[06/30 21:(\\d\\d):(\\d\\d)\\] \\[DEBUG\\] GLTracker (~)?buffer:  0x([0-9a-f]+)\\s*(\\d+)?");
        QVector<Allocation> allocations;
        QHash<uint64_t, Allocation> map;
        size_t lineCount { 0 };
        time_t lastTime;
        for (const auto& line : lines) {
            ++lineCount;
            auto match = re.match(line);
            if (!match.hasMatch()) {
                continue;
            }

            int time = match.captured(1).toInt() * 60;
            time += match.captured(2).toInt();
            lastTime = time + 1;
            bool destructor = !match.captured(3).isEmpty();
            uint64_t id = match.captured(4).toLongLong(0, 16);
            if (!destructor) {
                size_t size = match.captured(5).toLongLong();
                Allocation a { id, size, time, 0 };
                assert(!map.contains(id));
                map[id] = a;
            } else {
                assert(map.contains(id));
                auto a = map[id];
                a.end = time;
                map.remove(id);
                allocations.push_back(a);
            }
        }
        for (auto a : map.values()) {
            assert(a.end == 0);
            a.end = lastTime;
            allocations.push_back(a);
        }
        map.clear();

        qDebug() << map.size();
        qDebug() << allocations.size();
        qStableSort(allocations);
        QSet<uint64_t> seenIds;
        for (auto& a : allocations) {
            while (seenIds.contains(a.id)) {
                ++a.id;
            }
            seenIds.insert(a.id);
        }
        for (const auto& a : allocations) {
            assert(a.begin <= a.end);
        }

        return allocations;
    }

    static QVector<Allocation> toAllocationQueue(QVector<Allocation> allocations) {
        time_t now = 0;
        QVector<Allocation> result;
        for (const auto& a : allocations) {
            if (!a.size) {
                continue;
            }
            assert(a.begin >= now);
            now = a.begin;
            result.push_back(a);
        }

        // Reorder by deallocation time;
        for (auto& a : allocations) {
            a.begin = a.end;
        }
        std::sort(allocations.begin(), allocations.end(), [](const Allocation& a, const Allocation& b)->bool {
            return a.begin < b.begin;
        });

        now = 0;
        for (Allocation a : allocations) {
            if (!a.size) {
                continue;
            }
            assert(a.begin >= now);
            now = a.begin;
            a.size = 0;
            auto itr = std::upper_bound(result.begin(), result.end(), a, [](const Allocation& a, const Allocation& b)->bool {
                return a.begin < b.begin;
            });
            result.insert(itr, a);
        }
        return result;
    }
};

class SmartMemoryPoolTests : public QObject {
    Q_OBJECT

private:
    hifi::memory::impl::Null m;
    QVector<Allocation> allocations;

private slots:

    void initTestCase() {
        m.reallocate(1024);
        allocations = Allocation::parseAllocations();
        size_t totalSize = 0;
        size_t concurrentAllocation = 0;
        size_t maxConcurrentAllocation = 0;
        {
            std::map<time_t, std::list<size_t>> deallocations;
            for (const auto& a : allocations) {
                if (a.lifetime() <= 1) {
                    continue;
                }
                assert(a.end);

                totalSize += a.size;
                concurrentAllocation += a.size;
                deallocations[a.end].push_back(a.size);
                while (deallocations.begin()->first < a.begin) {
                    auto sizes = deallocations.begin()->second;
                    for (auto size : sizes) {
                        assert(size <= concurrentAllocation);
                        concurrentAllocation -= size;
                    }
                    deallocations.erase(deallocations.begin()->first);
                }
                maxConcurrentAllocation = std::max(maxConcurrentAllocation, concurrentAllocation);
            }
            qDebug() << "Total size " << totalSize;
            qDebug() << "Max concurrent allocation " << maxConcurrentAllocation;
        }
    }

    void testValidRange() {
        // Zero size is always invalid
        QVERIFY(!m.validRange(0, 0));

        // Valid ranges mean the offset + the size does not exceed the memory size
        QVERIFY(m.validRange(1, 0));
        QVERIFY(m.validRange(1024, 0));
        QVERIFY(m.validRange(1, 512));
        QVERIFY(m.validRange(512, 512));
        QVERIFY(m.validRange(1, 1023));

        // Invalid ranges mean the offset + the size goes beyond the allocation range
        QVERIFY(!m.validRange(1025, 0));
        QVERIFY(!m.validRange(1, 1024));
        QVERIFY(!m.validRange(513, 512));
        QVERIFY(!m.validRange(1024, 1024));
    }

    void testOverlap() {
        // Zero size is always invalid
        QVERIFY(!m.overlap(1, 0, 1));
        QVERIFY(!m.overlap(1, 1, 0));
        QVERIFY(m.overlap(1, 0, 0));
        QVERIFY(m.overlap(1, 1, 1));
        QVERIFY(!m.overlap(512, 512, 0));
        QVERIFY(!m.overlap(512, 0, 512));
        QVERIFY(m.overlap(512, 0, 0));
        QVERIFY(m.overlap(512, 1, 2));
        QVERIFY(m.overlap(512, 2, 1));
    }

    void reportPool(const hifi::memory::SystemMemoryPool& pool, bool excessive) {
        qDebug() << "Total allocation " << pool.size();
        qDebug() << "Free " << pool.freeSize();
        qDebug() << "Used " << pool.usedSize();
        qDebug() << "FreeBlocks " << pool.freeBlocksCount();

        if (excessive) {
            using Pair = std::pair<size_t, size_t>;
            std::vector<Pair> blockSizeCounts;
            {
                std::map<size_t, Pair> blocksBySize;
                for (const auto& b : pool._freeBlocks._blocks) {
                    if (!blocksBySize.count(b.size)) {
                        blocksBySize[b.size] = { 1, b.size };
                    } else {
                        ++blocksBySize[b.size].first;
                    }
                }
                for (const auto& e : blocksBySize) {
                    blockSizeCounts.push_back(e.second);
                }
            }
            std::sort(blockSizeCounts.begin(), blockSizeCounts.end(), [](const Pair& a, const Pair& b) {
                return a.first < b.first;
            });
            std::reverse(blockSizeCounts.begin(), blockSizeCounts.end());
            for (const auto& p : blockSizeCounts) {
                qDebug() << "Count " << p.first << " block size " << p.second << " total size " << p.first * p.second;
            }
        }
    }

    void testSystemPool() {
        hifi::memory::SystemMemoryPool pool;
        const auto allocationQueue = Allocation::toAllocationQueue(allocations);
        time_t now = 0;
        using Handle = hifi::memory::Handle;
        std::unordered_map<uint64_t, std::pair<Handle, size_t>> handleMap;
        size_t allocations = 0;
        size_t deallocations = 0;

        for (const auto& a : allocationQueue) {
            bool dtor = a.size == 0;
            time_t t = a.begin;
            if (t > now) {
                assert(pool.isValid());
                pool.sync();
                assert(pool.isValid());
                now = t;
                qDebug() << "Time " << t;
                qDebug() << "Allocations " << allocations;
                qDebug() << "Deallocations " << deallocations;
                allocations = deallocations = 0;
                reportPool(pool, false);
            }
            if (!dtor) {
                Handle h = pool.allocate(a.size);
                handleMap[a.id] = { h, a.size };
                ++allocations;
            } else {
                assert(handleMap.count(a.id));
                const auto& e = handleMap[a.id];
                Handle h = e.first;
                size_t size = e.second;
                handleMap.erase(a.id);
                ++deallocations;
                pool.free(h);
            }
        }
        pool.sync();
        reportPool(pool, true);
    }

    void cleanupTestCase() {
        qDebug() << "Finished";
        char c;
        std::cin >> c;
    }
};


QTEST_MAIN(SmartMemoryPoolTests)

#include "SmartMemoryPoolTests.moc"
