//
//  Created by Bradley Austin Davis on 2018/11/20
//  Copyright 2013-2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
package io.highfidelity.frameplayer;

import android.app.Service;
import android.content.ComponentName;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.IBinder;
import android.util.Log;

import org.qtproject.qt5.android.bindings.QtActivity;

import io.highfidelity.oculus.OculusMobileActivity;


public class QuestOculusActivity extends OculusMobileActivity implements ServiceConnection {
    private static final String TAG =QuestOculusActivity.class.getSimpleName();
    private boolean launchedQuestMode = false;
    private IBinder service = null;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        Log.w(TAG, "QQQ_LIFE onCreate");
        bindService(new Intent(this, FramePlayerService.class), this, BIND_AUTO_CREATE);
        super.onCreate(savedInstanceState);
   }

    @Override
    public void onServiceConnected(ComponentName name, IBinder service) {
        Log.w(TAG, "QQQ_LIFE onServiceConnected");
        this.service = service;
    }

    @Override
    public void onServiceDisconnected(ComponentName name) {
        Log.w(TAG, "QQQ_LIFE onServiceDisconnected");

    }

    @Override
    public void onBindingDied(ComponentName name) {
        Log.w(TAG, "QQQ_LIFE onBindingDied");

    }

    @Override
    public void onNullBinding(ComponentName name) {
        Log.w(TAG, "QQQ_LIFE onNullBinding");

    }
}
