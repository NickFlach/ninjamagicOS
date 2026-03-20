package com.ninjamagic.launcher.biofield

import androidx.compose.ui.graphics.Color
import com.ninjamagic.launcher.ui.theme.*

/**
 * Biofield state classifications derived from wearable physiological data.
 * Maps to the Space Child Biofield system signal processing pipeline:
 * NormalizedReading -> SignalSnapshot -> BiofieldStateInference
 */
enum class BiofieldClassification(
    val label: String,
    val accentColor: Color,
    val tempoMultiplier: Float,  // Animation speed modifier (1.0 = normal)
    val motionIntensity: Float,  // Particle/glow intensity (0.0 - 1.0)
) {
    FOCUSED(
        label = "Focused",
        accentColor = BiofieldFocused,
        tempoMultiplier = 0.85f,   // Slightly slower, calmer animations
        motionIntensity = 0.4f,
    ),
    CHARGED(
        label = "Charged",
        accentColor = BiofieldCharged,
        tempoMultiplier = 1.2f,    // Faster, more energetic
        motionIntensity = 0.8f,
    ),
    RESTORATIVE(
        label = "Resting",
        accentColor = BiofieldRestorative,
        tempoMultiplier = 0.6f,    // Slow, peaceful
        motionIntensity = 0.2f,
    ),
    DEPLETED(
        label = "Low Energy",
        accentColor = BiofieldDepleted,
        tempoMultiplier = 0.7f,    // Reduced to conserve
        motionIntensity = 0.1f,
    ),
    UNSETTLED(
        label = "Unsettled",
        accentColor = BiofieldUnsettled,
        tempoMultiplier = 1.0f,
        motionIntensity = 0.6f,
    ),
    NEUTRAL(
        label = "Neutral",
        accentColor = NinjaCyan,
        tempoMultiplier = 1.0f,
        motionIntensity = 0.3f,
    ),
    UNKNOWN(
        label = "No wearable",
        accentColor = NinjaOnSurfaceVariant,
        tempoMultiplier = 1.0f,
        motionIntensity = 0.3f,
    );
}

/**
 * Snapshot of the user's current biofield state from wearable data.
 */
data class BiofieldSnapshot(
    val classification: BiofieldClassification = BiofieldClassification.UNKNOWN,
    val heartRateBpm: Int? = null,
    val hrvMs: Float? = null,
    val coherence: Float = 0f,       // 0.0 - 1.0 cardiac coherence
    val fieldStrength: Float = 0f,    // 0.0 - 1.0 overall biofield strength
    val wearableConnected: Boolean = false,
    val wearableType: String? = null,
    val lastUpdateMs: Long = 0,
) {
    /** Whether the data is fresh (updated within last 30 seconds). */
    val isFresh: Boolean
        get() = wearableConnected &&
                (System.currentTimeMillis() - lastUpdateMs) < 30_000

    /** Get the appropriate accent color for the current state. */
    val accentColor: Color
        get() = if (isFresh) classification.accentColor else BiofieldClassification.UNKNOWN.accentColor

    /** Get animation tempo for the current state. */
    val tempoMultiplier: Float
        get() = if (isFresh) classification.tempoMultiplier else 1.0f
}
