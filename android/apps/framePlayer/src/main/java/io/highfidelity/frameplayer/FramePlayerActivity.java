//
//  Created by Bradley Austin Davis on 2018/11/20
//  Copyright 2013-2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
package io.highfidelity.frameplayer;

import android.content.Intent;
import android.os.Bundle;
import android.util.Log;

import org.qtproject.qt5.android.bindings.QtActivity;

import io.highfidelity.oculus.OculusMobileActivity;


public class FramePlayerActivity extends QtActivity {
    private static final String TAG =FramePlayerActivity.class.getSimpleName();

    private native void nativeOnCreate();
    private boolean launchedQuestMode = false;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        Log.w(TAG, "QQQ_LIFE onCreate");
        System.loadLibrary("framePlayer");
        super.onCreate(savedInstanceState);
        nativeOnCreate();
    }

    @Override
    protected void onDestroy() {
        Log.w(TAG, "QQQ_LIFE onDestroy");
        super.onDestroy();
        finishAndRemoveTask();
    }

    @Override
    protected void onResume() {
        Log.w(TAG, "QQQ_LIFE onResume");
        super.onResume();
        if (launchedQuestMode) {
            Log.w(TAG, "QQQ_Qt forwarding to OculusMobileActivity");
            startActivity(new Intent(this, OculusMobileActivity.class));
        }
    }

    @Override
    protected void onPause() {
        Log.w(TAG, "QQQ_LIFE onPause");
        super.onPause();
    }

    @SuppressWarnings("unused")
    public void launchOculusActivity() {
        startActivity(new Intent(this, OculusMobileActivity.class));
        launchedQuestMode = true;
    }

}
