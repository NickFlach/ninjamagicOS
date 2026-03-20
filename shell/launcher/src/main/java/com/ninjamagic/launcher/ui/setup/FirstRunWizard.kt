package com.ninjamagic.launcher.ui.setup

import androidx.compose.animation.*
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
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
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.ninjamagic.launcher.ui.theme.*

/**
 * NinjaMagic First-Run Setup Wizard
 *
 * Guides the user through initial setup after flashing ninjamagicOS:
 * 1. Welcome & language selection
 * 2. WiFi connection
 * 3. Space Child account (login or create)
 * 4. Agent personality setup
 * 5. Wearable pairing (optional)
 * 6. Privacy settings
 * 7. Ready to go
 */
@Composable
fun FirstRunWizard(
    onComplete: () -> Unit = {}
) {
    var currentStep by remember { mutableIntStateOf(0) }

    val steps = listOf(
        WizardStep.Welcome,
        WizardStep.WiFi,
        WizardStep.SpaceChildAccount,
        WizardStep.AgentSetup,
        WizardStep.WearablePairing,
        WizardStep.PrivacySettings,
        WizardStep.AllSet,
    )

    Box(
        modifier = Modifier
            .fillMaxSize()
            .background(
                brush = Brush.verticalGradient(
                    colors = listOf(NinjaDeepBlue, NinjaSurface)
                )
            )
    ) {
        Column(
            modifier = Modifier
                .fillMaxSize()
                .statusBarsPadding()
                .navigationBarsPadding()
                .padding(24.dp),
            horizontalAlignment = Alignment.CenterHorizontally
        ) {
            // Progress indicator
            if (currentStep > 0 && currentStep < steps.size - 1) {
                StepProgressBar(
                    currentStep = currentStep,
                    totalSteps = steps.size - 2 // exclude welcome and allset
                )
                Spacer(modifier = Modifier.height(24.dp))
            } else {
                Spacer(modifier = Modifier.height(48.dp))
            }

            // Step content
            AnimatedContent(
                targetState = currentStep,
                transitionSpec = {
                    slideInHorizontally { it } + fadeIn() togetherWith
                    slideOutHorizontally { -it } + fadeOut()
                },
                modifier = Modifier.weight(1f),
                label = "wizard_step"
            ) { step ->
                when (steps[step]) {
                    WizardStep.Welcome -> WelcomeStep()
                    WizardStep.WiFi -> WiFiStep()
                    WizardStep.SpaceChildAccount -> AccountStep()
                    WizardStep.AgentSetup -> AgentSetupStep()
                    WizardStep.WearablePairing -> WearableStep()
                    WizardStep.PrivacySettings -> PrivacyStep()
                    WizardStep.AllSet -> AllSetStep()
                }
            }

            // Navigation buttons
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.CenterVertically
            ) {
                if (currentStep > 0 && currentStep < steps.size - 1) {
                    TextButton(onClick = { currentStep-- }) {
                        Text("Back", color = NinjaOnSurfaceVariant)
                    }
                } else {
                    Spacer(modifier = Modifier.width(1.dp))
                }

                if (currentStep < steps.size - 1) {
                    Button(
                        onClick = { currentStep++ },
                        colors = ButtonDefaults.buttonColors(
                            containerColor = NinjaCyan,
                            contentColor = NinjaDeepBlue
                        ),
                        shape = RoundedCornerShape(16.dp),
                        modifier = Modifier.height(52.dp)
                    ) {
                        Text(
                            text = if (currentStep == 0) "Get Started" else "Continue",
                            fontWeight = FontWeight.SemiBold,
                            modifier = Modifier.padding(horizontal = 16.dp)
                        )
                    }
                } else {
                    Button(
                        onClick = onComplete,
                        colors = ButtonDefaults.buttonColors(
                            containerColor = NinjaCyan,
                            contentColor = NinjaDeepBlue
                        ),
                        shape = RoundedCornerShape(16.dp),
                        modifier = Modifier.height(52.dp)
                    ) {
                        Text(
                            text = "Enter NinjaMagic",
                            fontWeight = FontWeight.Bold,
                            modifier = Modifier.padding(horizontal = 16.dp)
                        )
                    }
                }
            }
        }
    }
}

enum class WizardStep {
    Welcome, WiFi, SpaceChildAccount, AgentSetup,
    WearablePairing, PrivacySettings, AllSet
}

@Composable
private fun StepProgressBar(currentStep: Int, totalSteps: Int) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .padding(horizontal = 32.dp),
        horizontalArrangement = Arrangement.spacedBy(6.dp)
    ) {
        for (i in 1..totalSteps) {
            Box(
                modifier = Modifier
                    .weight(1f)
                    .height(3.dp)
                    .clip(RoundedCornerShape(2.dp))
                    .background(
                        if (i <= currentStep) NinjaCyan
                        else NinjaSurfaceVariant
                    )
            )
        }
    }
}

@Composable
private fun WelcomeStep() {
    Column(
        modifier = Modifier.fillMaxSize(),
        horizontalAlignment = Alignment.CenterHorizontally,
        verticalArrangement = Arrangement.Center
    ) {
        // NinjaMagic logo
        Box(
            modifier = Modifier
                .size(96.dp)
                .clip(CircleShape)
                .background(
                    brush = Brush.linearGradient(
                        colors = listOf(NinjaCyan, NinjaPurple)
                    )
                ),
            contentAlignment = Alignment.Center
        ) {
            Text("NM", color = Color.White, fontWeight = FontWeight.Bold, fontSize = 36.sp)
        }

        Spacer(modifier = Modifier.height(32.dp))

        Text(
            text = "Welcome to\nninjamagicOS",
            style = MaterialTheme.typography.displaySmall,
            color = Color.White,
            fontWeight = FontWeight.Light,
            textAlign = TextAlign.Center
        )

        Spacer(modifier = Modifier.height(16.dp))

        Text(
            text = "Your phone, powered by AI.\nPrivate. On-device. Yours.",
            style = MaterialTheme.typography.bodyLarge,
            color = NinjaOnSurfaceVariant,
            textAlign = TextAlign.Center
        )
    }
}

@Composable
private fun WiFiStep() {
    WizardStepLayout(
        icon = Icons.Outlined.Wifi,
        iconColor = NinjaCyan,
        title = "Connect to WiFi",
        subtitle = "Connect to download your agent model and sync your account."
    ) {
        // Placeholder WiFi list
        val networks = listOf("Home WiFi", "Office-5G", "CoffeeShop_Free", "NinjaMagic-Guest")
        Column(verticalArrangement = Arrangement.spacedBy(8.dp)) {
            networks.forEach { network ->
                Surface(
                    modifier = Modifier
                        .fillMaxWidth()
                        .clickable { /* TODO: connect to WiFi */ },
                    color = NinjaSurfaceVariant,
                    shape = RoundedCornerShape(12.dp)
                ) {
                    Row(
                        modifier = Modifier.padding(16.dp),
                        verticalAlignment = Alignment.CenterVertically
                    ) {
                        Icon(
                            imageVector = Icons.Filled.Wifi,
                            contentDescription = null,
                            tint = NinjaCyan,
                            modifier = Modifier.size(20.dp)
                        )
                        Spacer(modifier = Modifier.width(12.dp))
                        Text(network, color = Color.White)
                        Spacer(modifier = Modifier.weight(1f))
                        Icon(
                            imageVector = Icons.Filled.Lock,
                            contentDescription = null,
                            tint = NinjaOnSurfaceVariant,
                            modifier = Modifier.size(16.dp)
                        )
                    }
                }
            }
        }
    }
}

@Composable
private fun AccountStep() {
    WizardStepLayout(
        icon = Icons.Outlined.AccountCircle,
        iconColor = NinjaPurple,
        title = "Space Child Account",
        subtitle = "Sign in to sync your profile, preferences, and biofield data across all Space Child apps."
    ) {
        Column(
            modifier = Modifier.fillMaxWidth(),
            verticalArrangement = Arrangement.spacedBy(12.dp)
        ) {
            Button(
                onClick = { /* TODO: launch SSO flow */ },
                modifier = Modifier.fillMaxWidth().height(52.dp),
                colors = ButtonDefaults.buttonColors(
                    containerColor = NinjaPurple,
                    contentColor = Color.White
                ),
                shape = RoundedCornerShape(16.dp)
            ) {
                Text("Sign in with Space Child", fontWeight = FontWeight.SemiBold)
            }

            OutlinedButton(
                onClick = { /* TODO: launch registration */ },
                modifier = Modifier.fillMaxWidth().height(52.dp),
                shape = RoundedCornerShape(16.dp)
            ) {
                Text("Create Account", color = NinjaCyan)
            }

            TextButton(
                onClick = { /* Skip — use without account */ },
                modifier = Modifier.fillMaxWidth()
            ) {
                Text("Skip for now", color = NinjaOnSurfaceVariant)
            }
        }
    }
}

@Composable
private fun AgentSetupStep() {
    WizardStepLayout(
        icon = Icons.Outlined.SmartToy,
        iconColor = NinjaCyan,
        title = "Meet Your Agent",
        subtitle = "NinjaMagic Agent runs entirely on your device. Choose how it helps you."
    ) {
        Column(verticalArrangement = Arrangement.spacedBy(12.dp)) {
            AgentOption(
                title = "Proactive",
                description = "Agent suggests actions based on context (calls, messages, calendar)",
                selected = true
            )
            AgentOption(
                title = "On-demand",
                description = "Agent only responds when you ask",
                selected = false
            )
            AgentOption(
                title = "Quiet",
                description = "Minimal agent interaction, manual phone controls",
                selected = false
            )
        }
    }
}

@Composable
private fun AgentOption(title: String, description: String, selected: Boolean) {
    Surface(
        modifier = Modifier
            .fillMaxWidth()
            .clickable { /* TODO: toggle selection */ },
        color = if (selected) NinjaCyan.copy(alpha = 0.1f) else NinjaSurfaceVariant,
        shape = RoundedCornerShape(16.dp),
        border = if (selected) {
            androidx.compose.foundation.BorderStroke(1.dp, NinjaCyan)
        } else null
    ) {
        Row(
            modifier = Modifier.padding(16.dp),
            verticalAlignment = Alignment.CenterVertically
        ) {
            Column(modifier = Modifier.weight(1f)) {
                Text(title, color = Color.White, fontWeight = FontWeight.SemiBold)
                Spacer(modifier = Modifier.height(2.dp))
                Text(description, color = NinjaOnSurfaceVariant, style = MaterialTheme.typography.bodySmall)
            }
            if (selected) {
                Icon(
                    imageVector = Icons.Filled.CheckCircle,
                    contentDescription = null,
                    tint = NinjaCyan,
                    modifier = Modifier.size(24.dp)
                )
            }
        }
    }
}

@Composable
private fun WearableStep() {
    WizardStepLayout(
        icon = Icons.Outlined.Watch,
        iconColor = NinjaGreen,
        title = "Wearable Device",
        subtitle = "Connect a heart rate monitor for biofield-aware UI that adapts to your state."
    ) {
        Column(
            modifier = Modifier.fillMaxWidth(),
            verticalArrangement = Arrangement.spacedBy(12.dp)
        ) {
            Button(
                onClick = { /* TODO: start BLE scan */ },
                modifier = Modifier.fillMaxWidth().height(52.dp),
                colors = ButtonDefaults.buttonColors(
                    containerColor = NinjaGreen,
                    contentColor = NinjaDeepBlue
                ),
                shape = RoundedCornerShape(16.dp)
            ) {
                Icon(Icons.Filled.BluetoothSearching, contentDescription = null)
                Spacer(modifier = Modifier.width(8.dp))
                Text("Scan for Devices", fontWeight = FontWeight.SemiBold)
            }

            Text(
                text = "Supports: Polar, Garmin, Oura, Scosche, or any Bluetooth HR monitor",
                style = MaterialTheme.typography.bodySmall,
                color = NinjaOnSurfaceVariant,
                textAlign = TextAlign.Center,
                modifier = Modifier.fillMaxWidth()
            )

            TextButton(
                onClick = { /* Skip */ },
                modifier = Modifier.fillMaxWidth()
            ) {
                Text("Skip for now", color = NinjaOnSurfaceVariant)
            }
        }
    }
}

@Composable
private fun PrivacyStep() {
    WizardStepLayout(
        icon = Icons.Outlined.Shield,
        iconColor = NinjaAmber,
        title = "Privacy & Security",
        subtitle = "ninjamagicOS keeps your data on-device by default. Review your privacy settings."
    ) {
        Column(verticalArrangement = Arrangement.spacedBy(16.dp)) {
            PrivacyToggle("On-device AI processing", "Agent runs locally, never sends data to cloud", true)
            PrivacyToggle("Cloud fallback", "Use cloud AI when on-device model can't handle request", false)
            PrivacyToggle("MSI domain audit", "Log all capability access for security review", true)
            PrivacyToggle("Network monitoring", "Monitor per-app network traffic", true)
            PrivacyToggle("Encrypted state", "Encrypt agent memory and MSI state at rest", true)
        }
    }
}

@Composable
private fun PrivacyToggle(title: String, description: String, defaultEnabled: Boolean) {
    var enabled by remember { mutableStateOf(defaultEnabled) }

    Row(
        modifier = Modifier.fillMaxWidth(),
        verticalAlignment = Alignment.CenterVertically
    ) {
        Column(modifier = Modifier.weight(1f)) {
            Text(title, color = Color.White, style = MaterialTheme.typography.titleSmall)
            Text(description, color = NinjaOnSurfaceVariant, style = MaterialTheme.typography.bodySmall)
        }
        Spacer(modifier = Modifier.width(8.dp))
        Switch(
            checked = enabled,
            onCheckedChange = { enabled = it },
            colors = SwitchDefaults.colors(
                checkedThumbColor = NinjaDeepBlue,
                checkedTrackColor = NinjaCyan,
                uncheckedTrackColor = NinjaSurfaceVariant
            )
        )
    }
}

@Composable
private fun AllSetStep() {
    Column(
        modifier = Modifier.fillMaxSize(),
        horizontalAlignment = Alignment.CenterHorizontally,
        verticalArrangement = Arrangement.Center
    ) {
        Icon(
            imageVector = Icons.Filled.RocketLaunch,
            contentDescription = null,
            tint = NinjaCyan,
            modifier = Modifier.size(72.dp)
        )

        Spacer(modifier = Modifier.height(24.dp))

        Text(
            text = "You're all set!",
            style = MaterialTheme.typography.headlineLarge,
            color = Color.White,
            fontWeight = FontWeight.Light
        )

        Spacer(modifier = Modifier.height(12.dp))

        Text(
            text = "ninjamagicOS is ready.\nYour agent is listening.",
            style = MaterialTheme.typography.bodyLarge,
            color = NinjaOnSurfaceVariant,
            textAlign = TextAlign.Center
        )
    }
}

@Composable
private fun WizardStepLayout(
    icon: ImageVector,
    iconColor: Color,
    title: String,
    subtitle: String,
    content: @Composable ColumnScope.() -> Unit
) {
    Column(
        modifier = Modifier.fillMaxSize(),
        horizontalAlignment = Alignment.CenterHorizontally
    ) {
        Spacer(modifier = Modifier.height(16.dp))

        Box(
            modifier = Modifier
                .size(56.dp)
                .clip(CircleShape)
                .background(iconColor.copy(alpha = 0.15f)),
            contentAlignment = Alignment.Center
        ) {
            Icon(
                imageVector = icon,
                contentDescription = null,
                tint = iconColor,
                modifier = Modifier.size(28.dp)
            )
        }

        Spacer(modifier = Modifier.height(20.dp))

        Text(
            text = title,
            style = MaterialTheme.typography.headlineMedium,
            color = Color.White,
            fontWeight = FontWeight.SemiBold,
            textAlign = TextAlign.Center
        )

        Spacer(modifier = Modifier.height(8.dp))

        Text(
            text = subtitle,
            style = MaterialTheme.typography.bodyMedium,
            color = NinjaOnSurfaceVariant,
            textAlign = TextAlign.Center,
            modifier = Modifier.padding(horizontal = 16.dp)
        )

        Spacer(modifier = Modifier.height(32.dp))

        content()
    }
}
