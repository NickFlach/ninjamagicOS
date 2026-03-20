//! spc — Shinobi.Substrate Compiler CLI
//!
//! Usage: spc <input.sp> [-o output.rs]
//! Compiles SP cognitive programs to Rust source using msi-runtime.

use std::fs;
use std::path::PathBuf;

use sp_compiler::lexer::Lexer;
use sp_compiler::parser::Parser;
use sp_compiler::lowering;

fn main() {
    env_logger::init();

    let args: Vec<String> = std::env::args().collect();

    if args.len() < 2 {
        eprintln!("Usage: spc <input.sp> [-o output.rs]");
        eprintln!("       spc --ast <input.sp>     (dump AST as JSON)");
        std::process::exit(1);
    }

    let dump_ast = args.contains(&"--ast".to_string());
    let input_path = &args[1];

    // Find output path
    let output_path = args.iter()
        .position(|a| a == "-o")
        .and_then(|i| args.get(i + 1))
        .map(|s| PathBuf::from(s))
        .unwrap_or_else(|| {
            let mut p = PathBuf::from(input_path);
            p.set_extension("rs");
            p
        });

    // Read source
    let source = match fs::read_to_string(input_path) {
        Ok(s) => s,
        Err(e) => {
            eprintln!("Error reading {}: {}", input_path, e);
            std::process::exit(1);
        }
    };

    // Lex
    let tokens = match Lexer::new(&source).tokenize() {
        Ok(t) => t,
        Err(e) => {
            eprintln!("Lexer error: {}", e);
            std::process::exit(1);
        }
    };

    // Parse
    let program = match Parser::new(tokens).parse() {
        Ok(p) => p,
        Err(e) => {
            eprintln!("Parse error: {}", e);
            std::process::exit(1);
        }
    };

    // Dump AST if requested
    if dump_ast {
        let json = serde_json::to_string_pretty(&program).unwrap();
        println!("{}", json);
        return;
    }

    // Lower to Rust
    let lowered = match lowering::lower(&program) {
        Ok(l) => l,
        Err(e) => {
            eprintln!("Lowering error: {}", e);
            std::process::exit(1);
        }
    };

    // Write output
    match fs::write(&output_path, &lowered.rust_source) {
        Ok(_) => {
            println!("Compiled {} -> {}", input_path, output_path.display());
            println!("  Domains: {}", lowered.domains.join(", "));
            println!("  Lanes:   {}", lowered.lanes.join(", "));
        }
        Err(e) => {
            eprintln!("Error writing {}: {}", output_path.display(), e);
            std::process::exit(1);
        }
    }
}
