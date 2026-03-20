//! Shinobi.Substrate (SP) Compiler
//!
//! Compiles SP cognitive programs into MSI runtime calls.
//! The compiler pipeline:
//!
//! 1. **Lexer** — tokenizes SP source text
//! 2. **Parser** — builds an AST from tokens
//! 3. **TypeChecker** — validates types and domain grants
//! 4. **Lowering** — transforms AST into MSI IR (intermediate representation)
//! 5. **CodeGen** — emits Rust source or MSI bytecode
//!
//! SP syntax example:
//! ```sp
//! domain NinjaMagicAgent {
//!     grant events "phone/"
//!     grant assoc "working" rw
//!     grant accel "npu"
//!     grant clock
//!     sealed
//! }
//!
//! lane main in NinjaMagicAgent {
//!     priority high
//!     energy balanced
//!     affinity big
//!
//!     let ctx = assoc.get("working", "context")
//!     event.publish("agent/status", "ready")
//!     loop {
//!         let ev = event.wait("phone/", 1s)
//!         assoc.put("working", ev.topic, ev.payload)
//!     }
//! }
//! ```

pub mod lexer;
pub mod parser;
pub mod ast;
pub mod lowering;
pub mod error;
