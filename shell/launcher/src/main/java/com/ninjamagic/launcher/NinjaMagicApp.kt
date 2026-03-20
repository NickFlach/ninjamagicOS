package com.ninjamagic.launcher

import android.app.Application
import android.util.Log

class NinjaMagicApp : Application() {
    companion object {
        const val TAG = "NinjaMagic"
        lateinit var instance: NinjaMagicApp
            private set
    }

    override fun onCreate() {
        super.onCreate()
        instance = this
        Log.i(TAG, "NinjaMagic Launcher initialized — ninjamagicOS v0.1.0")
    }
}
