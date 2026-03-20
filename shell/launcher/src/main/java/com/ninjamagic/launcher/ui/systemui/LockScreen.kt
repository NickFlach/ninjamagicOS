package com.ninjamagic.launcher.ui.systemui

import androidx.compose.animation.core.*
import androidx.compose.foundation.background
import androidx.compose.foundation.gestures.detectVerticalDragGestures
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.*
import androidx.compose.material.icons.outlined.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.ninjamagic.launcher.biofield.BiofieldSnapshot
import com.ninjamagic.launcher.ui.theme.*
import java.time.LocalDateTime
import java.time.format.DateTimeFormatter

/**
 * NinjaMagic Lock Screen — agent-accessible lock screen with biofield awareness.
 * Swipe up to unlock. Tap agent icon to ask agent without unlocking.
 */
@Composable
fun NinjaMagicLockScreen(
    biofield: BiofieldSnapshot = BiofieldSnapshot(),
    notifications: List<LockScreenNotification> = emptyList(),
    onUnlock: () -> Unit = {},
    onAgentTap: () -> Unit = {},
) {
    val currentTime = remember { mutableStateOf(LocalDateTime.now()) }
    val timeFormatter = remember { DateTimeFormatter.ofPattern("h:mm") }
    val dateFormatter = remember { DateTimeFormatter.ofPattern("EEEE, MMMM d") }

    LaunchedEffect(Unit) {
        while (true) {
            currentTime.value = LocalDateTime.now()
            kotlinx.coroutines.delay(1000)
        }
    }

    // Background gradient adapts to biofield state
    val bgGradient = Brush.verticalGradient(
        colors = listOf(
            NinjaDeepBlue,
            if (biofield.isFresh) {
                biofield.accentColor.copy(alpha = 0.08f)
            } else {
                NinjaDeepBlue
            },
            NinjaDeepBlue.copy(alpha = 0.95f)
        )
    )

    Box(
        modifier = Modifier
            .fillMaxSize()
            .background(bgGradient)
            .pointerInput(Unit) {
                detectVerticalDragGestures { _, dragAmount ->
                    if (dragAmount < -100) onUnlock()
                }
            }
    ) {
        Column(
            modifier = Modifier
                .fillMaxSize()
                .statusBarsPadding()
                .padding(horizontal = 24.dp),
            horizontalAlignment = Alignment.CenterHorizontally
        ) {
            Spacer(modifier = Modifier.height(80.dp))

            // Large clock
            Text(
                text = currentTime.value.format(timeFormatter),
                fontSize = 96.sp,
                fontWeight = FontWeight.Thin,
                color = Color.White,
                letterSpacing = (-2).sp
            )
            Text(
                text = currentTime.value.format(dateFormatter),
                style = MaterialTheme.typography.titleMedium,
                color = NinjaOnSurfaceVariant
            )

            Spacer(modifier = Modifier.height(32.dp))

            // Biofield status (if wearable connected)
            if (biofield.isFresh) {
                BiofieldLockWidget(biofield)
                Spacer(modifier = Modifier.height(24.dp))
            }

            // Notification previews
            if (notifications.isNotEmpty()) {
                NotificationStack(notifications.take(3))
            }

            Spacer(modifier = Modifier.weight(1f))

            // Agent quick-access button
            AgentLockButton(onClick = onAgentTap, biofield = biofield)

            Spacer(modifier = Modifier.height(16.dp))

            // Swipe hint
            Text(
                text = "Swipe up to unlock",
                style = MaterialTheme.typography.bodySmall,
                color = NinjaOnSurfaceVariant.copy(alpha = 0.4f)
            )

            Spacer(modifier = Modifier.height(32.dp))
        }
    }
}

@Composable
private fun BiofieldLockWidget(biofield: BiofieldSnapshot) {
    val infiniteTransition = rememberInfiniteTransition(label = "lock_biofield")
    val pulse by infiniteTransition.animateFloat(
        initialValue = 0.7f,
        targetValue = 1.0f,
        animationSpec = infiniteRepeatable(
            animation = tween(
                durationMillis = if (biofield.heartRateBpm != null) {
                    (60000 / biofield.heartRateBpm!!).coerceIn(400, 2000)
                } else 1000,
                easing = EaseInOutSine
            ),
            repeatMode = RepeatMode.Reverse
        ),
        label = "heartbeat"
    )

    Row(
        modifier = Modifier
            .clip(RoundedCornerShape(16.dp))
            .background(NinjaSurfaceVariant.copy(alpha = 0.3f))
            .padding(horizontal = 20.dp, vertical = 12.dp),
        verticalAlignment = Alignment.CenterVertically
    ) {
        // Pulsing heart
        Box(
            modifier = Modifier
                .size((12 * pulse).dp)
                .clip(CircleShape)
                .background(biofield.accentColor.copy(alpha = pulse))
        )
        Spacer(modifier = Modifier.width(12.dp))

        Column {
            Text(
                text = biofield.classification.label,
                style = MaterialTheme.typography.labelLarge,
                color = biofield.accentColor
            )
            if (biofield.heartRateBpm != null) {
                Text(
                    text = "${biofield.heartRateBpm} bpm" +
                           if (biofield.hrvMs != null) " \u2022 HRV ${biofield.hrvMs!!.toInt()}ms" else "",
                    style = MaterialTheme.typography.bodySmall,
                    color = NinjaOnSurfaceVariant
                )
            }
        }
    }
}

data class LockScreenNotification(
    val id: String,
    val app: String,
    val title: String,
    val body: String,
    val timestamp: Long,
    val color: Color = NinjaCyan,
)

@Composable
private fun NotificationStack(notifications: List<LockScreenNotification>) {
    Column(
        modifier = Modifier.fillMaxWidth(),
        verticalArrangement = Arrangement.spacedBy(8.dp)
    ) {
        notifications.forEach { notif ->
            Surface(
                modifier = Modifier.fillMaxWidth(),
                color = NinjaSurfaceVariant.copy(alpha = 0.5f),
                shape = RoundedCornerShape(16.dp)
            ) {
                Row(
                    modifier = Modifier.padding(14.dp),
                    verticalAlignment = Alignment.Top
                ) {
                    Box(
                        modifier = Modifier
                            .size(8.dp)
                            .clip(CircleShape)
                            .background(notif.color)
                            .offset(y = 5.dp)
                    )
                    Spacer(modifier = Modifier.width(12.dp))
                    Column(modifier = Modifier.weight(1f)) {
                        Text(
                            text = "${notif.app} \u2022 ${notif.title}",
                            style = MaterialTheme.typography.labelMedium,
                            color = Color.White,
                            maxLines = 1
                        )
                        Text(
                            text = notif.body,
                            style = MaterialTheme.typography.bodySmall,
                            color = NinjaOnSurfaceVariant,
                            maxLines = 2
                        )
                    }
                }
            }
        }
    }
}

@Composable
private fun AgentLockButton(
    onClick: () -> Unit,
    biofield: BiofieldSnapshot
) {
    val infiniteTransition = rememberInfiniteTransition(label = "agent_lock")
    val glowAlpha by infiniteTransition.animateFloat(
        initialValue = 0.4f,
        targetValue = 0.8f,
        animationSpec = infiniteRepeatable(
            animation = tween(2500, easing = EaseInOutSine),
            repeatMode = RepeatMode.Reverse
        ),
        label = "agent_glow"
    )

    val accentColor = if (biofield.isFresh) biofield.accentColor else NinjaCyan

    Column(horizontalAlignment = Alignment.CenterHorizontally) {
        FloatingActionButton(
            onClick = onClick,
            containerColor = Color.Transparent,
            modifier = Modifier
                .size(64.dp)
                .background(
                    brush = Brush.radialGradient(
                        colors = listOf(
                            accentColor.copy(alpha = glowAlpha * 0.3f),
                            Color.Transparent
                        ),
                        radius = 100f
                    ),
                    shape = CircleShape
                )
        ) {
            Box(
                modifier = Modifier
                    .size(56.dp)
                    .clip(CircleShape)
                    .background(
                        brush = Brush.linearGradient(
                            colors = listOf(
                                accentColor.copy(alpha = glowAlpha),
                                NinjaPurple.copy(alpha = glowAlpha)
                            )
                        )
                    ),
                contentAlignment = Alignment.Center
            ) {
                Icon(
                    imageVector = Icons.Filled.KeyboardVoice,
                    contentDescription = "Ask Agent",
                    tint = Color.White,
                    modifier = Modifier.size(28.dp)
                )
            }
        }
        Spacer(modifier = Modifier.height(8.dp))
        Text(
            text = "Ask NinjaMagic",
            style = MaterialTheme.typography.labelMedium,
            color = NinjaOnSurfaceVariant.copy(alpha = 0.6f)
        )
    }
}
