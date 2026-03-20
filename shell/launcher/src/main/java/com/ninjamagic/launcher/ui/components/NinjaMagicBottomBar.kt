package com.ninjamagic.launcher.ui.components

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Apps
import androidx.compose.material.icons.filled.Home
import androidx.compose.material.icons.outlined.Apps
import androidx.compose.material.icons.outlined.Home
import androidx.compose.material3.*
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.ninjamagic.launcher.ui.theme.NinjaCyan
import com.ninjamagic.launcher.ui.theme.NinjaDeepBlue
import com.ninjamagic.launcher.ui.theme.NinjaPurple

@Composable
fun NinjaMagicBottomBar(
    currentRoute: String,
    onNavigate: (String) -> Unit
) {
    Surface(
        modifier = Modifier.fillMaxWidth(),
        color = NinjaDeepBlue.copy(alpha = 0.95f),
        tonalElevation = 8.dp,
    ) {
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .padding(horizontal = 24.dp, vertical = 8.dp)
                .navigationBarsPadding(),
            horizontalArrangement = Arrangement.SpaceEvenly,
            verticalAlignment = Alignment.CenterVertically
        ) {
            // Home tab
            BottomBarItem(
                icon = if (currentRoute == "home") Icons.Filled.Home else Icons.Outlined.Home,
                label = "Home",
                selected = currentRoute == "home",
                onClick = { onNavigate("home") }
            )

            // Agent button (center, prominent)
            AgentButton(
                selected = currentRoute == "agent",
                onClick = { onNavigate("agent") }
            )

            // Apps tab
            BottomBarItem(
                icon = if (currentRoute == "apps") Icons.Filled.Apps else Icons.Outlined.Apps,
                label = "Apps",
                selected = currentRoute == "apps",
                onClick = { onNavigate("apps") }
            )
        }
    }
}

@Composable
private fun BottomBarItem(
    icon: ImageVector,
    label: String,
    selected: Boolean,
    onClick: () -> Unit
) {
    Column(
        modifier = Modifier
            .clip(RoundedCornerShape(12.dp))
            .clickable(onClick = onClick)
            .padding(horizontal = 20.dp, vertical = 8.dp),
        horizontalAlignment = Alignment.CenterHorizontally
    ) {
        Icon(
            imageVector = icon,
            contentDescription = label,
            tint = if (selected) NinjaCyan else Color.White.copy(alpha = 0.5f),
            modifier = Modifier.size(24.dp)
        )
        Spacer(modifier = Modifier.height(4.dp))
        Text(
            text = label,
            fontSize = 11.sp,
            fontWeight = if (selected) FontWeight.Medium else FontWeight.Normal,
            color = if (selected) NinjaCyan else Color.White.copy(alpha = 0.5f)
        )
    }
}

@Composable
private fun AgentButton(
    selected: Boolean,
    onClick: () -> Unit
) {
    Box(
        modifier = Modifier
            .size(56.dp)
            .clip(CircleShape)
            .background(
                brush = Brush.linearGradient(
                    colors = if (selected) {
                        listOf(NinjaCyan, NinjaPurple)
                    } else {
                        listOf(NinjaCyan.copy(alpha = 0.7f), NinjaPurple.copy(alpha = 0.7f))
                    }
                )
            )
            .clickable(onClick = onClick),
        contentAlignment = Alignment.Center
    ) {
        Text(
            text = "NM",
            color = Color.White,
            fontWeight = FontWeight.Bold,
            fontSize = 18.sp
        )
    }
}
