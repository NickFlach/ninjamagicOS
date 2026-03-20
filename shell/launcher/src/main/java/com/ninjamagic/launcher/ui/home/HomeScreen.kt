package com.ninjamagic.launcher.ui.home

import androidx.compose.animation.core.*
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.grid.GridCells
import androidx.compose.foundation.lazy.grid.LazyVerticalGrid
import androidx.compose.foundation.lazy.grid.items
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
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.ninjamagic.launcher.ui.theme.*
import java.time.LocalDateTime
import java.time.format.DateTimeFormatter

@Composable
fun HomeScreen() {
    val currentTime = remember { mutableStateOf(LocalDateTime.now()) }

    // Update clock every second
    LaunchedEffect(Unit) {
        while (true) {
            currentTime.value = LocalDateTime.now()
            kotlinx.coroutines.delay(1000)
        }
    }

    Column(
        modifier = Modifier
            .fillMaxSize()
            .statusBarsPadding()
            .padding(horizontal = 20.dp),
    ) {
        Spacer(modifier = Modifier.height(48.dp))

        // Clock
        ClockWidget(currentTime.value)

        Spacer(modifier = Modifier.height(32.dp))

        // Agent status card
        AgentStatusCard()

        Spacer(modifier = Modifier.height(24.dp))

        // Quick actions grid
        QuickActionsGrid()

        Spacer(modifier = Modifier.weight(1f))

        // Biofield indicator
        BiofieldIndicator()

        Spacer(modifier = Modifier.height(16.dp))
    }
}

@Composable
private fun ClockWidget(time: LocalDateTime) {
    val timeFormatter = remember { DateTimeFormatter.ofPattern("h:mm") }
    val amPmFormatter = remember { DateTimeFormatter.ofPattern("a") }
    val dateFormatter = remember { DateTimeFormatter.ofPattern("EEEE, MMMM d") }

    Column(
        modifier = Modifier.fillMaxWidth(),
        horizontalAlignment = Alignment.CenterHorizontally
    ) {
        Row(
            verticalAlignment = Alignment.Bottom
        ) {
            Text(
                text = time.format(timeFormatter),
                style = MaterialTheme.typography.displayLarge,
                color = Color.White,
                fontWeight = FontWeight.Thin,
                fontSize = 72.sp
            )
            Spacer(modifier = Modifier.width(8.dp))
            Text(
                text = time.format(amPmFormatter),
                style = MaterialTheme.typography.headlineSmall,
                color = NinjaCyan.copy(alpha = 0.7f),
                modifier = Modifier.padding(bottom = 12.dp)
            )
        }
        Text(
            text = time.format(dateFormatter),
            style = MaterialTheme.typography.bodyLarge,
            color = NinjaOnSurfaceVariant
        )
    }
}

@Composable
private fun AgentStatusCard() {
    // Subtle breathing animation for the agent status
    val infiniteTransition = rememberInfiniteTransition(label = "agent_pulse")
    val alpha by infiniteTransition.animateFloat(
        initialValue = 0.6f,
        targetValue = 1.0f,
        animationSpec = infiniteRepeatable(
            animation = tween(2000, easing = EaseInOutSine),
            repeatMode = RepeatMode.Reverse
        ),
        label = "pulse_alpha"
    )

    Card(
        modifier = Modifier
            .fillMaxWidth()
            .clickable { /* Navigate to agent conversation */ },
        colors = CardDefaults.cardColors(
            containerColor = NinjaSurfaceVariant.copy(alpha = 0.7f)
        ),
        shape = RoundedCornerShape(20.dp)
    ) {
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .padding(16.dp),
            verticalAlignment = Alignment.CenterVertically
        ) {
            // Agent avatar with pulse
            Box(
                modifier = Modifier
                    .size(48.dp)
                    .clip(CircleShape)
                    .background(
                        brush = Brush.linearGradient(
                            colors = listOf(
                                NinjaCyan.copy(alpha = alpha),
                                NinjaPurple.copy(alpha = alpha)
                            )
                        )
                    ),
                contentAlignment = Alignment.Center
            ) {
                Text(
                    text = "NM",
                    color = Color.White,
                    fontWeight = FontWeight.Bold,
                    fontSize = 16.sp
                )
            }

            Spacer(modifier = Modifier.width(16.dp))

            Column(modifier = Modifier.weight(1f)) {
                Text(
                    text = "NinjaMagic Agent",
                    style = MaterialTheme.typography.titleMedium,
                    color = Color.White
                )
                Spacer(modifier = Modifier.height(2.dp))
                Text(
                    text = "Ready \u2022 On-device \u2022 Tap to chat",
                    style = MaterialTheme.typography.bodySmall,
                    color = NinjaGreen.copy(alpha = 0.8f)
                )
            }

            Icon(
                imageVector = Icons.Filled.KeyboardVoice,
                contentDescription = "Voice",
                tint = NinjaCyan,
                modifier = Modifier
                    .size(32.dp)
                    .clip(CircleShape)
                    .background(NinjaCyan.copy(alpha = 0.1f))
                    .padding(6.dp)
            )
        }
    }
}

data class QuickAction(
    val icon: ImageVector,
    val label: String,
    val color: Color,
    val action: String
)

@Composable
private fun QuickActionsGrid() {
    val actions = remember {
        listOf(
            QuickAction(Icons.Outlined.Phone, "Phone", NinjaGreen, "phone"),
            QuickAction(Icons.Outlined.Message, "Messages", NinjaCyan, "messages"),
            QuickAction(Icons.Outlined.CameraAlt, "Camera", NinjaAmber, "camera"),
            QuickAction(Icons.Outlined.Settings, "Settings", NinjaOnSurfaceVariant, "settings"),
            QuickAction(Icons.Outlined.Public, "Browser", NinjaPurple, "browser"),
            QuickAction(Icons.Outlined.Notifications, "Alerts", NinjaRed, "notifications"),
        )
    }

    LazyVerticalGrid(
        columns = GridCells.Fixed(3),
        horizontalArrangement = Arrangement.spacedBy(16.dp),
        verticalArrangement = Arrangement.spacedBy(16.dp),
        modifier = Modifier.fillMaxWidth()
    ) {
        items(actions) { action ->
            QuickActionTile(action)
        }
    }
}

@Composable
private fun QuickActionTile(action: QuickAction) {
    Column(
        modifier = Modifier
            .clip(RoundedCornerShape(16.dp))
            .background(NinjaSurfaceVariant.copy(alpha = 0.5f))
            .clickable { /* Launch action */ }
            .padding(vertical = 16.dp, horizontal = 8.dp),
        horizontalAlignment = Alignment.CenterHorizontally
    ) {
        Box(
            modifier = Modifier
                .size(44.dp)
                .clip(CircleShape)
                .background(action.color.copy(alpha = 0.15f)),
            contentAlignment = Alignment.Center
        ) {
            Icon(
                imageVector = action.icon,
                contentDescription = action.label,
                tint = action.color,
                modifier = Modifier.size(22.dp)
            )
        }
        Spacer(modifier = Modifier.height(8.dp))
        Text(
            text = action.label,
            style = MaterialTheme.typography.labelMedium,
            color = NinjaOnSurface,
            textAlign = TextAlign.Center
        )
    }
}

@Composable
private fun BiofieldIndicator() {
    // Shows the user's current biofield state from wearable data
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .clip(RoundedCornerShape(12.dp))
            .background(NinjaSurfaceVariant.copy(alpha = 0.3f))
            .padding(horizontal = 16.dp, vertical = 10.dp),
        verticalAlignment = Alignment.CenterVertically
    ) {
        // Heartbeat dot
        Box(
            modifier = Modifier
                .size(8.dp)
                .clip(CircleShape)
                .background(NinjaOnSurfaceVariant.copy(alpha = 0.5f))
        )
        Spacer(modifier = Modifier.width(10.dp))
        Text(
            text = "Connect wearable for biofield awareness",
            style = MaterialTheme.typography.bodySmall,
            color = NinjaOnSurfaceVariant.copy(alpha = 0.6f),
            modifier = Modifier.weight(1f)
        )
        Icon(
            imageVector = Icons.Outlined.Watch,
            contentDescription = "Wearable",
            tint = NinjaOnSurfaceVariant.copy(alpha = 0.4f),
            modifier = Modifier.size(16.dp)
        )
    }
}
