//! MCP Transport — stdio-based JSON-RPC transport
//!
//! The MCP server communicates over stdin/stdout using newline-delimited
//! JSON-RPC messages. This is the standard MCP transport for local servers.

use std::io::{self, BufRead, Write};
use log::{info, error, debug};

use crate::protocol::JsonRpcRequest;
use crate::server::McpServer;

/// Run the MCP server on stdio transport.
/// Reads JSON-RPC requests from stdin, dispatches to server, writes responses to stdout.
pub fn run_stdio(mut server: McpServer) -> io::Result<()> {
    let stdin = io::stdin();
    let stdout = io::stdout();
    let mut stdout_lock = stdout.lock();

    info!("NinjaMagic MCP server listening on stdio");

    for line in stdin.lock().lines() {
        let line = match line {
            Ok(l) => l,
            Err(e) => {
                error!("Failed to read stdin: {}", e);
                break;
            }
        };

        let trimmed = line.trim();
        if trimmed.is_empty() {
            continue;
        }

        debug!("MCP recv: {}", trimmed);

        let request: JsonRpcRequest = match serde_json::from_str(trimmed) {
            Ok(r) => r,
            Err(e) => {
                error!("Invalid JSON-RPC: {} — input: {}", e, trimmed);
                let err_response = serde_json::json!({
                    "jsonrpc": "2.0",
                    "id": null,
                    "error": {
                        "code": -32700,
                        "message": format!("Parse error: {}", e)
                    }
                });
                writeln!(stdout_lock, "{}", err_response)?;
                stdout_lock.flush()?;
                continue;
            }
        };

        let response = server.handle_request(&request);
        let response_json = serde_json::to_string(&response).unwrap_or_default();

        debug!("MCP send: {}", response_json);
        writeln!(stdout_lock, "{}", response_json)?;
        stdout_lock.flush()?;
    }

    info!("MCP server shutting down");
    Ok(())
}
