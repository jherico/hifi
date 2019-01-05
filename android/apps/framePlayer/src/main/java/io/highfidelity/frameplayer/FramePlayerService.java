//
//  Created by Bradley Austin Davis on 2018/11/20
//  Copyright 2013-2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
package io.highfidelity.frameplayer;

import android.content.Intent;
import android.os.IBinder;
import android.util.Log;

import org.qtproject.qt5.android.bindings.QtService;

public class FramePlayerService extends QtService {
    private static final String TAG =FramePlayerService.class.getSimpleName();

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        Log.w(TAG, "QQQ_LIFE Received start id " + startId + ": " + intent);
        return START_STICKY;
    }

    @Override
    public void onCreate() {
        Log.w(TAG, "QQQ_LIFE onCreate");
        super.onCreate();
    }

    @Override
    public void onDestroy() {
        Log.w(TAG, "QQQ_LIFE onDestroy");
        super.onDestroy();
    }

    @Override
    public IBinder onBind(Intent intent) {
        Log.w(TAG, "QQQ_LIFE onBind");
        return super.onBind(intent);
    }
}
