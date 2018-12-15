package io.highfidelity.frameplayer;


import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.MotionEvent;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.WindowManager;

public class RenderActivity extends Activity implements SurfaceHolder.Callback {
    static {
        System.loadLibrary("framePlayer");
    }
    private static final String TAG = RenderActivity.class.getName();
    private native long nativeOnCreate();
    private native void nativeOnTouch(long action, float x, float y);
    private native void nativeOnSurfaceCreated(long handle, Surface s);
    private native void nativeOnSurfaceChanged(long handle, Surface s);
    private native void nativeOnSurfaceDestroyed(long handle);

    private SurfaceView mView;
    private SurfaceHolder mSurfaceHolder;
    private long mNativeHandle;

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        //Log.w(TAG, String.format("QQQ touchEvent %d", event.getAction()));
        int h = getWindow().getDecorView().getHeight();
        int w = getWindow().getDecorView().getWidth();
        float x = event.getX() / (float)w;
        float y = event.getY() / (float)h;
        nativeOnTouch(event.getAction(), x, y);;
        return super.onTouchEvent(event);
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        Log.w(TAG, "QQQ onCreate");
        super.onCreate(savedInstanceState);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        // Create a native surface for VR rendering (Qt GL surfaces are not suitable
        // because of the lack of fine control over the surface callbacks)
        mView = new SurfaceView(this);
        setContentView(mView);
        mView.getHolder().addCallback(this);
        // Forward the create message to the display plugin
        mNativeHandle = nativeOnCreate();
    }

    @Override
    protected void onDestroy() {
        Log.w(TAG, "QQQ onDestroy");
        if (mSurfaceHolder != null) {
            nativeOnSurfaceDestroyed(mNativeHandle);
        }
        super.onDestroy();
        mNativeHandle = 0;
    }

    @Override
    protected void onResume() {
        Log.w(TAG, "QQQ onResume");
        super.onResume();
    }

    @Override
    protected void onPause() {
        Log.w(TAG, "QQQ onPause");
        super.onPause();
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        Log.w(TAG, "QQQ surfaceCreated");
        nativeOnSurfaceCreated(mNativeHandle, holder.getSurface());
        mSurfaceHolder = holder;
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        Log.w(TAG, "QQQ surfaceChanged");
        nativeOnSurfaceChanged(mNativeHandle, holder.getSurface());
        mSurfaceHolder = holder;
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        Log.w(TAG, "QQQ surfaceDestroyed");
        nativeOnSurfaceDestroyed(mNativeHandle);
        mSurfaceHolder = null;
    }
}

