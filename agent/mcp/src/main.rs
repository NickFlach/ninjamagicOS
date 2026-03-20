//! NinjaMagic MCP Server entry point
//!
//! Runs the MCP server on stdio transport, exposing phone capabilities
//! as MCP tools and phone state as MCP resources.
//!
//! Usage:
//!   ninjamagic-mcp-server          # stdio transport (default)
//!   ninjamagic-mcp-server --help   # show help

use ninjamagic_mcp::server::McpServer;
use ninjamagic_mcp::transport;

fn main() {
    env_logger::init();

    let args: Vec<String> = std::env::args().collect();

    if args.contains(&"--help".to_string()) || args.contains(&"-h".to_string()) {
        eprintln!("NinjaMagic MCP Server v0.1.0");
        eprintln!("Exposes phone capabilities as MCP tools and resources.");
        eprintln!();
        eprintln!("Usage: ninjamagic-mcp-server");
        eprintln!("  Runs on stdio transport (newline-delimited JSON-RPC).");
        eprintln!();
        eprintln!("Tools: phone_dial, phone_answer, phone_hangup, sms_send, sms_read,");
        eprintln!("       settings_wifi, settings_bluetooth, camera_capture, app_launch,");
        eprintln!("       alarm_set, web_search, notification_send, clipboard_get/set");
        eprintln!();
        eprintln!("Resources: phone://state/battery, phone://state/network, phone://state/calls,");
        eprintln!("           phone://state/sms/unread, phone://state/location, phone://state/biofield");
        std::process::exit(0);
    }

    let server = McpServer::new();
    if let Err(e) = transport::run_stdio(server) {
        eprintln!("MCP server error: {}", e);
        std::process::exit(1);
    }
}
