package com.ninjamagic.launcher.ui.systemui

import androidx.compose.animation.core.*
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.*
import androidx.compose.material.icons.outlined.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.ninjamagic.launcher.biofield.BiofieldSnapshot
import com.ninjamagic.launcher.ui.theme.*
import java.time.LocalTime
import java.time.format.DateTimeFormatter

/**
 * NinjaMagic Status Bar — replaces standard Android status bar.
 * Shows time, connectivity, battery, agent status, and biofield indicator.
 */
@Composable
fun NinjaMagicStatusBar(
    biofield: BiofieldSnapshot = BiofieldSnapshot(),
    agentActive: Boolean = true,
    wifiConnected: Boolean = true,
    cellularSignal: Int = 3, // 0-4
    batteryLevel: Int = 85,
    batteryCharging: Boolean = false,
) {
    val timeFormatter = remember { DateTimeFormatter.ofPattern("h:mm") }
    var currentTime by remember { mutableStateOf(LocalTime.now()) }

    LaunchedEffect(Unit) {
        while (true) {
            currentTime = LocalTime.now()
            kotlinx.coroutines.delay(1000)
        }
    }

    Row(
        modifier = Modifier
            .fillMaxWidth()
            .background(Color.Transparent)
            .statusBarsPadding()
            .padding(horizontal = 16.dp, vertical = 4.dp),
        verticalAlignment = Alignment.CenterVertically
    ) {
        // Time
        Text(
            text = currentTime.format(timeFormatter),
            fontSize = 14.sp,
            fontWeight = FontWeight.Medium,
            color = Color.White
        )

        Spacer(modifier = Modifier.weight(1f))

        // Agent status indicator
        AgentStatusDot(active = agentActive)
        Spacer(modifier = Modifier.width(8.dp))

        // Biofield dot (if wearable connected)
        if (biofield.wearableConnected) {
            BiofieldDot(biofield = biofield)
            Spacer(modifier = Modifier.width(8.dp))
        }

        // Connectivity icons
        if (wifiConnected) {
            Icon(
                imageVector = Icons.Filled.Wifi,
                contentDescription = "WiFi",
                tint = Color.White,
                modifier = Modifier.size(14.dp)
            )
            Spacer(modifier = Modifier.width(4.dp))
        }

        // Cellular signal
        Icon(
            imageVector = when (cellularSignal) {
                0 -> Icons.Filled.SignalCellular0Bar
                1 -> Icons.Filled.SignalCellularAlt1Bar
                2 -> Icons.Filled.SignalCellularAlt2Bar
                else -> Icons.Filled.SignalCellularAlt
            },
            contentDescription = "Signal",
            tint = Color.White,
            modifier = Modifier.size(14.dp)
        )
        Spacer(modifier = Modifier.width(6.dp))

        // Battery
        BatteryIndicator(level = batteryLevel, charging = batteryCharging)
    }
}

@Composable
private fun AgentStatusDot(active: Boolean) {
    val infiniteTransition = rememberInfiniteTransition(label = "agent_dot")
    val alpha by infiniteTransition.animateFloat(
        initialValue = 0.5f,
        targetValue = 1.0f,
        animationSpec = infiniteRepeatable(
            animation = tween(1500, easing = EaseInOutSine),
            repeatMode = RepeatMode.Reverse
        ),
        label = "agent_alpha"
    )

    Row(verticalAlignment = Alignment.CenterVertically) {
        Box(
            modifier = Modifier
                .size(6.dp)
                .clip(CircleShape)
                .background(
                    if (active) NinjaGreen.copy(alpha = alpha)
                    else NinjaOnSurfaceVariant.copy(alpha = 0.4f)
                )
        )
        Spacer(modifier = Modifier.width(3.dp))
        Text(
            text = "NM",
            fontSize = 10.sp,
            fontWeight = FontWeight.Bold,
            color = if (active) NinjaCyan.copy(alpha = 0.8f) else NinjaOnSurfaceVariant.copy(alpha = 0.4f)
        )
    }
}

@Composable
private fun BiofieldDot(biofield: BiofieldSnapshot) {
    val infiniteTransition = rememberInfiniteTransition(label = "biofield_dot")
    val scale by infiniteTransition.animateFloat(
        initialValue = 0.8f,
        targetValue = 1.2f,
        animationSpec = infiniteRepeatable(
            animation = tween(
                durationMillis = (1000 / biofield.tempoMultiplier).toInt(),
                easing = EaseInOutSine
            ),
            repeatMode = RepeatMode.Reverse
        ),
        label = "biofield_scale"
    )

    Box(
        modifier = Modifier
            .size((6 * scale).dp)
            .clip(CircleShape)
            .background(biofield.accentColor)
    )
}

@Composable
private fun BatteryIndicator(level: Int, charging: Boolean) {
    val color = when {
        charging -> NinjaGreen
        level > 20 -> Color.White
        level > 10 -> NinjaAmber
        else -> NinjaRed
    }

    Row(verticalAlignment = Alignment.CenterVertically) {
        Text(
            text = "$level%",
            fontSize = 12.sp,
            color = color
        )
        Spacer(modifier = Modifier.width(2.dp))
        Icon(
            imageVector = if (charging) Icons.Filled.BatteryChargingFull
                          else if (level > 50) Icons.Filled.BatteryFull
                          else Icons.Filled.Battery3Bar,
            contentDescription = "Battery",
            tint = color,
            modifier = Modifier.size(14.dp)
        )
    }
}
