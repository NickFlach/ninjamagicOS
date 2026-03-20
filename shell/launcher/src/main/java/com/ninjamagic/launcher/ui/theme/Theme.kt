package com.ninjamagic.launcher.ui.theme

import android.app.Activity
import androidx.compose.foundation.isSystemInDarkTheme
import androidx.compose.material3.*
import androidx.compose.runtime.Composable
import androidx.compose.runtime.SideEffect
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.toArgb
import androidx.compose.ui.platform.LocalView
import androidx.core.view.WindowCompat

// NinjaMagic brand colors
val NinjaCyan = Color(0xFF00E5FF)
val NinjaDeepBlue = Color(0xFF0A1628)
val NinjaSlate = Color(0xFF1A2742)
val NinjaPurple = Color(0xFF7C4DFF)
val NinjaGreen = Color(0xFF00E676)
val NinjaAmber = Color(0xFFFFAB00)
val NinjaRed = Color(0xFFFF1744)
val NinjaSurface = Color(0xFF0D1B2A)
val NinjaSurfaceVariant = Color(0xFF162A44)
val NinjaOnSurface = Color(0xFFE0E6ED)
val NinjaOnSurfaceVariant = Color(0xFF8899AA)

// Biofield-aware color tokens (overridden at runtime by biofield state)
val BiofieldFocused = Color(0xFF00B8D4)    // Cool cyan — focused/flow state
val BiofieldCharged = Color(0xFFFF6D00)    // Warm amber — high energy
val BiofieldRestorative = Color(0xFF69F0AE) // Soft green — resting/recovery
val BiofieldDepleted = Color(0xFF78909C)   // Muted grey — low energy
val BiofieldUnsettled = Color(0xFFE040FB)  // Vibrant purple — unsettled

private val DarkColorScheme = darkColorScheme(
    primary = NinjaCyan,
    onPrimary = NinjaDeepBlue,
    primaryContainer = NinjaSlate,
    onPrimaryContainer = NinjaCyan,
    secondary = NinjaPurple,
    onSecondary = Color.White,
    secondaryContainer = Color(0xFF3A1F78),
    onSecondaryContainer = Color(0xFFD4BBFF),
    tertiary = NinjaGreen,
    onTertiary = NinjaDeepBlue,
    error = NinjaRed,
    onError = Color.White,
    background = NinjaDeepBlue,
    onBackground = NinjaOnSurface,
    surface = NinjaSurface,
    onSurface = NinjaOnSurface,
    surfaceVariant = NinjaSurfaceVariant,
    onSurfaceVariant = NinjaOnSurfaceVariant,
    outline = Color(0xFF3A4F66),
    outlineVariant = Color(0xFF253548),
)

@Composable
fun NinjaMagicTheme(
    darkTheme: Boolean = true, // NinjaMagic is always dark-first
    content: @Composable () -> Unit
) {
    val colorScheme = DarkColorScheme

    val view = LocalView.current
    if (!view.isInEditMode) {
        SideEffect {
            val window = (view.context as Activity).window
            window.statusBarColor = NinjaDeepBlue.toArgb()
            window.navigationBarColor = NinjaDeepBlue.toArgb()
            WindowCompat.getInsetsController(window, view).apply {
                isAppearanceLightStatusBars = false
                isAppearanceLightNavigationBars = false
            }
        }
    }

    MaterialTheme(
        colorScheme = colorScheme,
        typography = NinjaMagicTypography,
        content = content
    )
}
