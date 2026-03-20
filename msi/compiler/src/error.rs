//! SP compiler error types

use thiserror::Error;

#[derive(Error, Debug)]
pub enum SpError {
    #[error("Lexer error at line {line}, col {col}: {msg}")]
    Lexer { line: usize, col: usize, msg: String },

    #[error("Parse error at line {line}: {msg}")]
    Parse { line: usize, msg: String },

    #[error("Type error: {0}")]
    Type(String),

    #[error("Undefined symbol: '{0}'")]
    Undefined(String),

    #[error("Domain '{domain}' missing grant for {resource}")]
    MissingGrant { domain: String, resource: String },

    #[error("Lowering error: {0}")]
    Lowering(String),

    #[error("IO error: {0}")]
    Io(#[from] std::io::Error),
}

pub type Result<T> = std::result::Result<T, SpError>;
