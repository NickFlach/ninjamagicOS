package com.ninjamagic.launcher.ui.agent

import androidx.compose.animation.core.*
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.lazy.rememberLazyListState
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.text.BasicTextField
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.KeyboardVoice
import androidx.compose.material.icons.filled.Send
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.SolidColor
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.input.TextFieldValue
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.ninjamagic.launcher.ui.theme.*

data class ChatMessage(
    val id: String,
    val text: String,
    val isAgent: Boolean,
    val timestamp: Long = System.currentTimeMillis(),
    val skillUsed: String? = null,
)

@Composable
fun AgentScreen() {
    var messages by remember {
        mutableStateOf(
            listOf(
                ChatMessage(
                    id = "welcome",
                    text = "Hey! I'm your NinjaMagic Agent. I can make calls, send texts, control settings, search the web, and more. What can I help with?",
                    isAgent = true
                )
            )
        )
    }
    var inputText by remember { mutableStateOf(TextFieldValue("")) }
    val listState = rememberLazyListState()

    Column(
        modifier = Modifier
            .fillMaxSize()
            .statusBarsPadding()
    ) {
        // Header
        AgentHeader()

        // Messages
        LazyColumn(
            state = listState,
            modifier = Modifier
                .weight(1f)
                .fillMaxWidth()
                .padding(horizontal = 16.dp),
            verticalArrangement = Arrangement.spacedBy(12.dp),
            contentPadding = PaddingValues(vertical = 16.dp)
        ) {
            items(messages, key = { it.id }) { message ->
                ChatBubble(message)
            }
        }

        // Input bar
        InputBar(
            value = inputText,
            onValueChange = { inputText = it },
            onSend = {
                if (inputText.text.isNotBlank()) {
                    val userMsg = ChatMessage(
                        id = "user_${System.currentTimeMillis()}",
                        text = inputText.text.trim(),
                        isAgent = false
                    )
                    messages = messages + userMsg
                    inputText = TextFieldValue("")

                    // Simulate agent response
                    val agentMsg = ChatMessage(
                        id = "agent_${System.currentTimeMillis()}",
                        text = "I'll help with that. Let me process your request through the MSI event bus...",
                        isAgent = true,
                        skillUsed = "intent_router"
                    )
                    messages = messages + agentMsg
                }
            },
            onVoice = { /* TODO: Start voice input via MSI audio lane */ }
        )
    }
}

@Composable
private fun AgentHeader() {
    val infiniteTransition = rememberInfiniteTransition(label = "header_pulse")
    val glowAlpha by infiniteTransition.animateFloat(
        initialValue = 0.3f,
        targetValue = 0.7f,
        animationSpec = infiniteRepeatable(
            animation = tween(3000, easing = EaseInOutSine),
            repeatMode = RepeatMode.Reverse
        ),
        label = "glow"
    )

    Surface(
        color = NinjaDeepBlue,
        modifier = Modifier.fillMaxWidth()
    ) {
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .padding(horizontal = 20.dp, vertical = 12.dp),
            verticalAlignment = Alignment.CenterVertically
        ) {
            Box(
                modifier = Modifier
                    .size(40.dp)
                    .clip(CircleShape)
                    .background(
                        brush = Brush.linearGradient(
                            colors = listOf(
                                NinjaCyan.copy(alpha = glowAlpha),
                                NinjaPurple.copy(alpha = glowAlpha)
                            )
                        )
                    ),
                contentAlignment = Alignment.Center
            ) {
                Text("NM", color = Color.White, fontWeight = FontWeight.Bold, fontSize = 14.sp)
            }
            Spacer(modifier = Modifier.width(12.dp))
            Column {
                Text(
                    "NinjaMagic Agent",
                    style = MaterialTheme.typography.titleMedium,
                    color = Color.White
                )
                Text(
                    "On-device \u2022 Llama 3.2 \u2022 Private",
                    style = MaterialTheme.typography.bodySmall,
                    color = NinjaGreen.copy(alpha = 0.7f)
                )
            }
        }
    }
}

@Composable
private fun ChatBubble(message: ChatMessage) {
    val alignment = if (message.isAgent) Alignment.Start else Alignment.End
    val bgColor = if (message.isAgent) {
        NinjaSurfaceVariant
    } else {
        NinjaCyan.copy(alpha = 0.15f)
    }
    val textColor = if (message.isAgent) NinjaOnSurface else Color.White
    val shape = if (message.isAgent) {
        RoundedCornerShape(4.dp, 20.dp, 20.dp, 20.dp)
    } else {
        RoundedCornerShape(20.dp, 4.dp, 20.dp, 20.dp)
    }

    Column(
        modifier = Modifier.fillMaxWidth(),
        horizontalAlignment = alignment
    ) {
        Box(
            modifier = Modifier
                .widthIn(max = 300.dp)
                .clip(shape)
                .background(bgColor)
                .padding(horizontal = 16.dp, vertical = 12.dp)
        ) {
            Text(
                text = message.text,
                style = MaterialTheme.typography.bodyMedium,
                color = textColor
            )
        }

        if (message.skillUsed != null) {
            Spacer(modifier = Modifier.height(4.dp))
            Text(
                text = "\u26A1 ${message.skillUsed}",
                style = MaterialTheme.typography.labelSmall,
                color = NinjaOnSurfaceVariant.copy(alpha = 0.5f),
                modifier = Modifier.padding(horizontal = 8.dp)
            )
        }
    }
}

@Composable
private fun InputBar(
    value: TextFieldValue,
    onValueChange: (TextFieldValue) -> Unit,
    onSend: () -> Unit,
    onVoice: () -> Unit
) {
    Surface(
        color = NinjaSurface,
        tonalElevation = 4.dp,
        modifier = Modifier.fillMaxWidth()
    ) {
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .padding(horizontal = 16.dp, vertical = 8.dp)
                .navigationBarsPadding(),
            verticalAlignment = Alignment.CenterVertically
        ) {
            // Voice button
            IconButton(onClick = onVoice) {
                Icon(
                    imageVector = Icons.Filled.KeyboardVoice,
                    contentDescription = "Voice input",
                    tint = NinjaCyan
                )
            }

            // Text input
            Box(
                modifier = Modifier
                    .weight(1f)
                    .clip(RoundedCornerShape(24.dp))
                    .background(NinjaSurfaceVariant)
                    .padding(horizontal = 16.dp, vertical = 12.dp)
            ) {
                if (value.text.isEmpty()) {
                    Text(
                        "Ask NinjaMagic anything...",
                        style = MaterialTheme.typography.bodyMedium,
                        color = NinjaOnSurfaceVariant.copy(alpha = 0.5f)
                    )
                }
                BasicTextField(
                    value = value,
                    onValueChange = onValueChange,
                    textStyle = MaterialTheme.typography.bodyMedium.copy(
                        color = Color.White
                    ),
                    cursorBrush = SolidColor(NinjaCyan),
                    modifier = Modifier.fillMaxWidth(),
                    singleLine = false,
                    maxLines = 4
                )
            }

            Spacer(modifier = Modifier.width(8.dp))

            // Send button
            IconButton(
                onClick = onSend,
                enabled = value.text.isNotBlank()
            ) {
                Icon(
                    imageVector = Icons.Filled.Send,
                    contentDescription = "Send",
                    tint = if (value.text.isNotBlank()) NinjaCyan else NinjaOnSurfaceVariant.copy(alpha = 0.3f)
                )
            }
        }
    }
}
