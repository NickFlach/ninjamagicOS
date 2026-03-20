# ninjamagicOS User Guide

**Version 0.1.0** — Your phone, powered by AI.

---

## What is ninjamagicOS?

ninjamagicOS is an AI-native mobile operating system that puts an intelligent agent at the center of your phone experience. Instead of navigating through apps and menus, you talk to your NinjaMagic Agent — it makes calls, sends texts, controls settings, and manages your phone on your behalf.

**Key features:**
- **On-device AI** — Your agent runs entirely on your phone. No cloud required. Your data stays private.
- **Biofield-aware UI** — Connect a heart rate monitor and your phone adapts its look and feel to your physical state.
- **MSI substrate** — A capability-based security model that sandboxes every component of the OS.
- **Space Child integration** — Sync your identity across the Space Child ecosystem.

---

## Supported Devices

| Device | Codename | SoC | AI Accelerator | Secure Element |
|--------|----------|-----|----------------|----------------|
| Google Pixel 7 | panther | Tensor GS201 | Google TPU | Titan M2 |
| OnePlus Nord N30 | larry | Snapdragon 695 | Hexagon 686 DSP | Qualcomm SPU |

---

## Getting Started

### 1. Flash ninjamagicOS

**Prerequisites:**
- Unlocked bootloader on your device
- USB-C cable
- Computer with `fastboot` installed

**Steps:**
```bash
# Put device in fastboot mode
adb reboot bootloader

# Run the flash tool
./tools/flash/flash.sh --device panther --wipe
```

The flash tool will:
1. Verify all required images
2. Flash boot, system, vendor, and vbmeta partitions
3. Set the active boot slot
4. Reboot into ninjamagicOS

### 2. First-Run Setup

On first boot, the setup wizard guides you through:
1. **WiFi** — Connect to download your agent model
2. **Space Child Account** — Sign in or create an account (optional)
3. **Agent Personality** — Choose proactive, on-demand, or quiet mode
4. **Wearable Pairing** — Connect a BLE heart rate monitor (optional)
5. **Privacy Settings** — Review on-device processing, encryption, and audit options

### 3. Using the Home Screen

The NinjaMagic Launcher has three main areas:

- **Home** — Clock, agent status card, quick action tiles (Phone, Messages, Camera, Settings, Browser, Alerts)
- **Agent** — Persistent chat with your NinjaMagic Agent. Type or use voice.
- **Apps** — Searchable grid of all installed apps

Navigate between them with the bottom bar. The center button (NM) takes you to the agent.

---

## Talking to Your Agent

### Voice
Tap the microphone icon on the home screen or agent screen to speak.

### Text
Tap the agent card or navigate to the Agent tab. Type your request.

### Examples
| You say | Agent does |
|---------|-----------|
| "Call Mom" | Looks up Mom in contacts, initiates call |
| "Text Alex I'm running 10 minutes late" | Sends SMS to Alex |
| "Turn on WiFi" | Enables WiFi via settings |
| "Take a photo" | Opens camera and captures |
| "Set an alarm for 7 AM" | Creates alarm |
| "What's my battery level?" | Reports current battery percentage |
| "Play some music" | Opens music player |

### Agent Skills
The agent has built-in skills for:
- **Phone** — dial, answer, hang up
- **SMS** — send, read conversations
- **Settings** — WiFi, Bluetooth, brightness, volume
- **Camera** — capture photos
- **Alarms** — set, modify, cancel
- **Apps** — launch any installed app
- **Media** — play/pause, volume control
- **Web** — search, fetch URLs
- **Contacts** — look up contact info

---

## Biofield-Aware UI

When you connect a wearable heart rate monitor, ninjamagicOS adapts to your physical state:

| State | What it means | UI behavior |
|-------|--------------|-------------|
| **Focused** | High HRV, moderate HR | Cool cyan accents, slightly slower animations |
| **Charged** | High heart rate, active | Warm amber accents, faster animations |
| **Resting** | Low HR, high HRV | Soft green accents, slow peaceful animations |
| **Low Energy** | Low HR, low HRV | Muted grey, reduced motion |
| **Unsettled** | High HR, low HRV (stress) | Purple accents, normal speed |

### Supported Wearables
Any Bluetooth Low Energy device with the standard Heart Rate Profile:
- Polar H10, Verity Sense
- Garmin HRM-Pro, HRM-Dual
- Scosche Rhythm24
- Oura Ring Gen 3
- Any BLE chest strap or arm band

---

## Privacy & Security

### On-Device Processing
Your NinjaMagic Agent runs a local LLM (Llama 3.2 1B or 3B depending on device). No conversation data leaves your phone unless you explicitly enable cloud fallback.

### MSI Domain Sandboxing
Every component runs in a sealed MSI domain with specific capability grants. The agent cannot access data outside its grants.

### Encrypted Storage
Agent memory and MSI state regions are encrypted with AES-256-GCM. Keys are stored in the hardware secure element (Titan M2 or Qualcomm SPU) and never leave the chip.

### Verified Boot
ninjamagicOS uses Android Verified Boot (AVB) 2.0 with dm-verity to ensure the OS hasn't been tampered with.

### Network Monitoring
Every MSI domain has network policies. The privacy guard blocks unauthorized network access and logs all traffic.

---

## Settings

Access Settings from the home screen quick actions or the app drawer.

Key settings:
- **Agent** — personality mode, voice, proactive suggestions
- **Biofield** — wearable connection, UI adaptation intensity
- **Privacy** — on-device only, cloud fallback, audit log viewer
- **Network** — per-domain network policies
- **Updates** — OTA update policy, auto-reboot schedule
- **Space Child** — account, profile sync, artifact export
- **About** — OS version, security patch level, device info

---

## Updating

ninjamagicOS supports seamless A/B updates:
1. Your agent checks for updates automatically (configurable)
2. Updates download in the background to the inactive slot
3. Agent suggests rebooting when convenient (charging, idle)
4. If anything goes wrong, the bootloader automatically rolls back

You can change update policy in Settings > Updates.

---

## Troubleshooting

| Issue | Solution |
|-------|---------|
| Agent not responding | Check agent service: Settings > About > Agent Status |
| No wearable connection | Ensure Bluetooth is on, device is in pairing mode |
| Calls not working | Check SIM card, network registration in Settings |
| Boot loop | Device will auto-rollback to previous slot after 3 failed boots |
| Need to reflash | Boot to fastboot (`adb reboot bootloader`) and re-run flash tool |

---

## Support

- **GitHub**: https://github.com/NickFlach/ninjamagicOS
- **Space Child**: https://phone.spacechild.love
