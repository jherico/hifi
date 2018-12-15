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

import org.qtproject.qt5.android.bindings.QtActivity;

import java.util.Timer;
import java.util.TimerTask;

public class FramePlayerActivity extends QtActivity {
    private native void nativeOnCreate();

    @Override
    public void onCreate(Bundle savedInstanceState) {
        System.loadLibrary("framePlayer");
        super.onCreate(savedInstanceState);
        nativeOnCreate();
        new Timer().schedule(new TimerTask() {
            @Override
            public void run() {
                runOnUiThread(()->{
                    startActivity(new Intent(FramePlayerActivity.this, RenderActivity.class));
                });
            }
        }, 1000);
    }
}
