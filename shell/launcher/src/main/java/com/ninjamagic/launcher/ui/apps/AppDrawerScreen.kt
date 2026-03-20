package com.ninjamagic.launcher.ui.apps

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.grid.GridCells
import androidx.compose.foundation.lazy.grid.LazyVerticalGrid
import androidx.compose.foundation.lazy.grid.items
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.text.BasicTextField
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.outlined.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.SolidColor
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.input.TextFieldValue
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.ninjamagic.launcher.ui.theme.*

data class AppInfo(
    val name: String,
    val packageName: String,
    val icon: ImageVector,
    val color: Color
)

@Composable
fun AppDrawerScreen() {
    var searchText by remember { mutableStateOf(TextFieldValue("")) }

    val installedApps = remember {
        listOf(
            AppInfo("Phone", "com.ninjamagic.dialer", Icons.Outlined.Phone, NinjaGreen),
            AppInfo("Messages", "com.ninjamagic.sms", Icons.Outlined.Message, NinjaCyan),
            AppInfo("Camera", "com.ninjamagic.camera", Icons.Outlined.CameraAlt, NinjaAmber),
            AppInfo("Gallery", "com.ninjamagic.gallery", Icons.Outlined.Photo, NinjaPurple),
            AppInfo("Browser", "com.ninjamagic.browser", Icons.Outlined.Public, NinjaCyan),
            AppInfo("Settings", "com.ninjamagic.settings", Icons.Outlined.Settings, NinjaOnSurfaceVariant),
            AppInfo("Files", "com.ninjamagic.files", Icons.Outlined.Folder, NinjaAmber),
            AppInfo("Clock", "com.ninjamagic.clock", Icons.Outlined.Schedule, NinjaCyan),
            AppInfo("Calculator", "com.ninjamagic.calc", Icons.Outlined.Calculate, NinjaGreen),
            AppInfo("Contacts", "com.ninjamagic.contacts", Icons.Outlined.Contacts, NinjaPurple),
            AppInfo("Maps", "com.google.android.apps.maps", Icons.Outlined.Map, NinjaGreen),
            AppInfo("YouTube", "com.google.android.youtube", Icons.Outlined.PlayCircle, NinjaRed),
            AppInfo("Gmail", "com.google.android.gm", Icons.Outlined.Email, NinjaRed),
            AppInfo("Calendar", "com.google.android.calendar", Icons.Outlined.CalendarMonth, NinjaCyan),
            AppInfo("Play Store", "com.android.vending", Icons.Outlined.ShoppingBag, NinjaGreen),
            AppInfo("Chrome", "com.android.chrome", Icons.Outlined.Language, NinjaCyan),
            AppInfo("Music", "com.ninjamagic.music", Icons.Outlined.MusicNote, NinjaPurple),
            AppInfo("Notes", "com.ninjamagic.notes", Icons.Outlined.EditNote, NinjaAmber),
            AppInfo("Weather", "com.ninjamagic.weather", Icons.Outlined.WbSunny, NinjaCyan),
            AppInfo("Recorder", "com.ninjamagic.recorder", Icons.Outlined.Mic, NinjaRed),
        )
    }

    val filteredApps = remember(searchText.text) {
        if (searchText.text.isBlank()) {
            installedApps
        } else {
            installedApps.filter {
                it.name.contains(searchText.text, ignoreCase = true) ||
                it.packageName.contains(searchText.text, ignoreCase = true)
            }
        }
    }

    Column(
        modifier = Modifier
            .fillMaxSize()
            .statusBarsPadding()
    ) {
        Spacer(modifier = Modifier.height(16.dp))

        // Search bar
        Box(
            modifier = Modifier
                .fillMaxWidth()
                .padding(horizontal = 20.dp)
                .clip(RoundedCornerShape(16.dp))
                .background(NinjaSurfaceVariant)
                .padding(horizontal = 16.dp, vertical = 14.dp)
        ) {
            Row(verticalAlignment = Alignment.CenterVertically) {
                Icon(
                    imageVector = Icons.Outlined.Search,
                    contentDescription = "Search",
                    tint = NinjaOnSurfaceVariant,
                    modifier = Modifier.size(20.dp)
                )
                Spacer(modifier = Modifier.width(12.dp))
                Box(modifier = Modifier.weight(1f)) {
                    if (searchText.text.isEmpty()) {
                        Text(
                            "Search apps or ask agent...",
                            style = MaterialTheme.typography.bodyMedium,
                            color = NinjaOnSurfaceVariant.copy(alpha = 0.5f)
                        )
                    }
                    BasicTextField(
                        value = searchText,
                        onValueChange = { searchText = it },
                        textStyle = MaterialTheme.typography.bodyMedium.copy(
                            color = Color.White
                        ),
                        cursorBrush = SolidColor(NinjaCyan),
                        modifier = Modifier.fillMaxWidth(),
                        singleLine = true,
                    )
                }
            }
        }

        Spacer(modifier = Modifier.height(8.dp))

        // App count
        Text(
            text = "${filteredApps.size} apps",
            style = MaterialTheme.typography.labelMedium,
            color = NinjaOnSurfaceVariant.copy(alpha = 0.5f),
            modifier = Modifier.padding(horizontal = 24.dp, vertical = 8.dp)
        )

        // App grid
        LazyVerticalGrid(
            columns = GridCells.Fixed(4),
            modifier = Modifier
                .fillMaxWidth()
                .padding(horizontal = 16.dp),
            horizontalArrangement = Arrangement.spacedBy(8.dp),
            verticalArrangement = Arrangement.spacedBy(16.dp),
            contentPadding = PaddingValues(vertical = 8.dp)
        ) {
            items(filteredApps, key = { it.packageName }) { app ->
                AppTile(app)
            }
        }
    }
}

@Composable
private fun AppTile(app: AppInfo) {
    Column(
        modifier = Modifier
            .clip(RoundedCornerShape(12.dp))
            .clickable { /* Launch app via packageManager or MSI event */ }
            .padding(vertical = 8.dp, horizontal = 4.dp),
        horizontalAlignment = Alignment.CenterHorizontally
    ) {
        Box(
            modifier = Modifier
                .size(52.dp)
                .clip(RoundedCornerShape(14.dp))
                .background(app.color.copy(alpha = 0.12f)),
            contentAlignment = Alignment.Center
        ) {
            Icon(
                imageVector = app.icon,
                contentDescription = app.name,
                tint = app.color,
                modifier = Modifier.size(26.dp)
            )
        }
        Spacer(modifier = Modifier.height(6.dp))
        Text(
            text = app.name,
            style = MaterialTheme.typography.labelSmall,
            color = NinjaOnSurface.copy(alpha = 0.85f),
            textAlign = TextAlign.Center,
            maxLines = 1,
            overflow = TextOverflow.Ellipsis,
            modifier = Modifier.fillMaxWidth()
        )
    }
}
