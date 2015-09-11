//
//  ThreadHelpers.h
//
//  Created by Bradley Austin Davis on 2015-04-04
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once
#ifndef hifi_ThreadHelpers_h
#define hifi_ThreadHelpers_h

#include <exception>
#include <functional>

#include <QtCore/QWaitCondition>
#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtCore/QThread>
#include <QtCore/QEvent>

template <typename L, typename F>
void withLock(L lock, F function) {
    throw std::exception();
}

template <typename F>
void withLock(QMutex& lock, F function) {
    QMutexLocker locker(&lock);
    function();
}


class Condition {
    int _count{ 0 };
    QMutex _mutex;
    QWaitCondition _waitCondition;

public:
    void increment() {
        QMutexLocker locker(&_mutex);
        ++_count;
    }

    void decrement() {
        QMutexLocker locker(&_mutex);
        --_count;
    }

    void decrementAndWait() {
        QMutexLocker locker(&_mutex);
        --_count;
        _waitCondition.wait(&_mutex);
    }

    void satisfy() {
        _mutex.lock();
        // Sleep until there are no busy worker threads
        while (_count > 0) {
            _mutex.unlock();
            QThread::msleep(1);
            _mutex.lock();
        }
        _waitCondition.wakeAll();
        _mutex.unlock();
    }

    template <typename F>
    void satisfy(F f) {
        _mutex.lock();
        f();
        // Sleep until there are no busy worker threads
        while (_count > 0) {
            _mutex.unlock();
            QThread::msleep(1);
            _mutex.lock();
        }
        _waitCondition.wakeAll();
        _mutex.unlock();
    }
};

static enum CustomEventTypes {
    Lambda = QEvent::User + 1
};

class LambdaEvent : public QEvent {
    std::function<void()> _fun;
public:
    LambdaEvent(const std::function<void()> & fun) :
        QEvent(static_cast<QEvent::Type>(Lambda)), _fun(fun) {
    }
    LambdaEvent(std::function<void()> && fun) :
        QEvent(static_cast<QEvent::Type>(Lambda)), _fun(fun) {
    }
    void call() { _fun(); }
};


#endif
