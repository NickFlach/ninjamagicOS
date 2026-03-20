# NinjaMagic MCP — Model Context Protocol for Phone OS

The NinjaMagic Agent exposes phone capabilities as MCP (Model Context Protocol) tools and resources, enabling both local agent skills and external AI systems to interact with the phone.

## MCP Server (Phone → External)

Exposes the phone as an MCP server that external agents/tools can connect to:

### Tools

| Tool | Description | MSI Events |
|------|-------------|------------|
| `phone.call` | Make a phone call | `phone/call/dial` |
| `phone.sms.send` | Send an SMS | `phone/sms/send` |
| `phone.sms.read` | Read recent messages | `phone/sms/inbox` |
| `phone.contacts.search` | Search contacts | State: contacts |
| `camera.capture` | Take a photo | `camera/capture` |
| `camera.scan` | Scan QR/barcode | `camera/scan` |
| `audio.record` | Record audio | `audio/record` |
| `audio.play` | Play audio | `audio/play` |
| `settings.wifi` | Toggle WiFi | `system/wifi` |
| `settings.bluetooth` | Toggle Bluetooth | `system/bluetooth` |
| `settings.brightness` | Set brightness | `system/display` |
| `files.read` | Read a file | State: filesystem |
| `files.write` | Write a file | State: filesystem |
| `files.list` | List directory | State: filesystem |
| `browser.open` | Open URL | `system/browser` |
| `browser.search` | Web search | `system/browser` |
| `notifications.list` | Get notifications | `system/notifications` |
| `location.get` | Get current location | `sensor/gps` |
| `alarm.set` | Set alarm/timer | `system/alarm` |

### Resources

| Resource | Description | Update Frequency |
|----------|-------------|-----------------|
| `phone://battery` | Battery level and charging state | Real-time |
| `phone://signal` | Signal strength and network type | Real-time |
| `phone://location` | Current GPS coordinates | On-demand |
| `phone://connectivity` | WiFi/cellular/bluetooth status | Real-time |
| `phone://notifications` | Recent notifications | Event-driven |
| `phone://contacts` | Contact list | On-demand |
| `phone://calendar` | Calendar events | On-demand |
| `phone://sensors` | Sensor readings (accel, gyro) | Real-time |
| `phone://biofield` | Biofield state (if wearable connected) | Real-time |

## MCP Client (Phone → External Servers)

The agent can connect to external MCP servers to extend its capabilities:

- Connect to home automation MCP servers
- Connect to work tool MCP servers (email, calendar, CRM)
- Connect to Space Child ecosystem MCP servers
- User-configurable MCP server list

## Transport

- **Local**: Unix domain socket for on-device skill communication
- **Remote**: WebSocket over TLS for external MCP connections
- **Discovery**: mDNS for local network MCP server discovery
