package com.ninjamagic.launcher.ui

import androidx.compose.foundation.layout.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Modifier
import androidx.navigation.compose.NavHost
import androidx.navigation.compose.composable
import androidx.navigation.compose.rememberNavController
import com.ninjamagic.launcher.ui.home.HomeScreen
import com.ninjamagic.launcher.ui.agent.AgentScreen
import com.ninjamagic.launcher.ui.apps.AppDrawerScreen
import com.ninjamagic.launcher.ui.components.NinjaMagicBottomBar

@Composable
fun NinjaMagicLauncher() {
    val navController = rememberNavController()
    var currentRoute by remember { mutableStateOf("home") }

    Scaffold(
        bottomBar = {
            NinjaMagicBottomBar(
                currentRoute = currentRoute,
                onNavigate = { route ->
                    currentRoute = route
                    navController.navigate(route) {
                        popUpTo("home") { saveState = true }
                        launchSingleTop = true
                        restoreState = true
                    }
                }
            )
        },
        containerColor = MaterialTheme.colorScheme.background
    ) { innerPadding ->
        NavHost(
            navController = navController,
            startDestination = "home",
            modifier = Modifier.padding(innerPadding)
        ) {
            composable("home") { HomeScreen() }
            composable("agent") { AgentScreen() }
            composable("apps") { AppDrawerScreen() }
        }
    }
}
