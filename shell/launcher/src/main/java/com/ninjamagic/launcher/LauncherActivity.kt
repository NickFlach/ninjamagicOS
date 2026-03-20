package com.ninjamagic.launcher

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import com.ninjamagic.launcher.ui.NinjaMagicLauncher
import com.ninjamagic.launcher.ui.theme.NinjaMagicTheme

class LauncherActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        setContent {
            NinjaMagicTheme {
                NinjaMagicLauncher()
            }
        }
    }
}
