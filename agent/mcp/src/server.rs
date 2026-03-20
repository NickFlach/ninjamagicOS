//! MCP Server — handles JSON-RPC requests and dispatches to tools/resources

use serde_json::{json, Value};
use log::{info, warn};

use crate::protocol::*;
use crate::tools;
use crate::resources;

/// MCP Server state.
pub struct McpServer {
    initialized: bool,
}

impl McpServer {
    pub fn new() -> Self {
        McpServer { initialized: false }
    }

    /// Handle an incoming JSON-RPC request and return a response.
    pub fn handle_request(&mut self, request: &JsonRpcRequest) -> JsonRpcResponse {
        info!("MCP request: method={} id={}", request.method, request.id);

        match request.method.as_str() {
            "initialize" => self.handle_initialize(&request.id),
            "initialized" => JsonRpcResponse::success(request.id.clone(), json!({})),
            "tools/list" => self.handle_tools_list(&request.id),
            "tools/call" => self.handle_tools_call(&request.id, &request.params),
            "resources/list" => self.handle_resources_list(&request.id),
            "resources/read" => self.handle_resources_read(&request.id, &request.params),
            "ping" => JsonRpcResponse::success(request.id.clone(), json!({})),

            _ => {
                warn!("Unknown MCP method: {}", request.method);
                JsonRpcResponse::error(
                    request.id.clone(),
                    -32601,
                    &format!("Method not found: {}", request.method),
                )
            }
        }
    }

    fn handle_initialize(&mut self, id: &Value) -> JsonRpcResponse {
        self.initialized = true;
        info!("MCP server initialized");

        JsonRpcResponse::success(id.clone(), json!({
            "protocolVersion": "2024-11-05",
            "serverInfo": {
                "name": "ninjamagic-phone",
                "version": "0.1.0"
            },
            "capabilities": {
                "tools": { "listChanged": false },
                "resources": { "subscribe": false, "listChanged": false }
            }
        }))
    }

    fn handle_tools_list(&self, id: &Value) -> JsonRpcResponse {
        let tool_list = tools::list_tools();
        let tools_json: Vec<Value> = tool_list.iter().map(|t| {
            json!({
                "name": t.name,
                "description": t.description,
                "inputSchema": t.input_schema
            })
        }).collect();

        JsonRpcResponse::success(id.clone(), json!({ "tools": tools_json }))
    }

    fn handle_tools_call(&self, id: &Value, params: &Option<Value>) -> JsonRpcResponse {
        let params = match params {
            Some(p) => p,
            None => return JsonRpcResponse::error(id.clone(), -32602, "Missing params"),
        };

        let tool_name = match params["name"].as_str() {
            Some(n) => n,
            None => return JsonRpcResponse::error(id.clone(), -32602, "Missing tool name"),
        };

        let args = params.get("arguments").cloned().unwrap_or(json!({}));

        info!("Tool call: {} args={}", tool_name, args);
        let result = tools::execute_tool(tool_name, &args);

        let content_json: Vec<Value> = result.content.iter().map(|c| {
            match c {
                ContentBlock::Text { text } => json!({ "type": "text", "text": text }),
                ContentBlock::Image { data, mime_type } => json!({
                    "type": "image", "data": data, "mimeType": mime_type
                }),
                ContentBlock::Resource { resource } => json!({
                    "type": "resource",
                    "resource": {
                        "uri": resource.uri,
                        "mimeType": resource.mime_type,
                        "text": resource.text
                    }
                }),
            }
        }).collect();

        JsonRpcResponse::success(id.clone(), json!({
            "content": content_json,
            "isError": result.is_error
        }))
    }

    fn handle_resources_list(&self, id: &Value) -> JsonRpcResponse {
        let resource_list = resources::list_resources();
        let resources_json: Vec<Value> = resource_list.iter().map(|r| {
            json!({
                "uri": r.uri,
                "name": r.name,
                "description": r.description,
                "mimeType": r.mime_type
            })
        }).collect();

        JsonRpcResponse::success(id.clone(), json!({ "resources": resources_json }))
    }

    fn handle_resources_read(&self, id: &Value, params: &Option<Value>) -> JsonRpcResponse {
        let params = match params {
            Some(p) => p,
            None => return JsonRpcResponse::error(id.clone(), -32602, "Missing params"),
        };

        let uri = match params["uri"].as_str() {
            Some(u) => u,
            None => return JsonRpcResponse::error(id.clone(), -32602, "Missing resource URI"),
        };

        info!("Resource read: {}", uri);

        match resources::read_resource(uri) {
            Some(contents) => {
                JsonRpcResponse::success(id.clone(), json!({
                    "contents": [{
                        "uri": contents.uri,
                        "mimeType": contents.mime_type,
                        "text": contents.text
                    }]
                }))
            }
            None => {
                JsonRpcResponse::error(id.clone(), -32002, &format!("Resource not found: {}", uri))
            }
        }
    }
}
