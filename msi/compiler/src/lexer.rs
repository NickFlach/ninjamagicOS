//! SP Lexer — tokenizes Shinobi.Substrate source text

use crate::error::{SpError, Result};

/// Token types for SP language.
#[derive(Debug, Clone, PartialEq)]
pub enum TokenKind {
    // Keywords
    Domain, Lane, In, Grant, Sealed,
    Events, State, Assoc, Clock, Accel,
    Priority, Energy, Affinity,
    Low, Normal, High, Realtime, Balanced, Unbounded,
    Any, Little, Big, Npu, Gpu, Dsp,
    Let, Loop, If, Else, Return, Rw, Ro,

    // Literals
    StringLit(String),
    IntLit(i64),
    FloatLit(f64),
    DurationLit(u64), // nanoseconds
    BoolTrue,
    BoolFalse,

    // Identifiers
    Ident(String),

    // Symbols
    LBrace, RBrace, LParen, RParen, LBracket, RBracket,
    Dot, Comma, Colon, Semicolon, Eq, Arrow,
    Plus, Minus, Star, Slash, Percent,
    EqEq, BangEq, Lt, Gt, LtEq, GtEq,
    AmpAmp, PipePipe, Bang,

    // Special
    Newline,
    Eof,
}

/// A token with its source location.
#[derive(Debug, Clone)]
pub struct Token {
    pub kind: TokenKind,
    pub line: usize,
    pub col: usize,
}

/// Lexer state.
pub struct Lexer {
    source: Vec<char>,
    pos: usize,
    line: usize,
    col: usize,
}

impl Lexer {
    pub fn new(source: &str) -> Self {
        Lexer {
            source: source.chars().collect(),
            pos: 0,
            line: 1,
            col: 1,
        }
    }

    /// Tokenize the entire source.
    pub fn tokenize(&mut self) -> Result<Vec<Token>> {
        let mut tokens = Vec::new();
        loop {
            let tok = self.next_token()?;
            let is_eof = tok.kind == TokenKind::Eof;
            tokens.push(tok);
            if is_eof { break; }
        }
        Ok(tokens)
    }

    fn peek(&self) -> Option<char> {
        self.source.get(self.pos).copied()
    }

    fn advance(&mut self) -> Option<char> {
        let ch = self.source.get(self.pos).copied();
        if let Some(c) = ch {
            self.pos += 1;
            if c == '\n' {
                self.line += 1;
                self.col = 1;
            } else {
                self.col += 1;
            }
        }
        ch
    }

    fn skip_whitespace(&mut self) {
        while let Some(c) = self.peek() {
            if c == ' ' || c == '\t' || c == '\r' {
                self.advance();
            } else if c == '/' && self.source.get(self.pos + 1) == Some(&'/') {
                // Line comment
                while let Some(c) = self.peek() {
                    if c == '\n' { break; }
                    self.advance();
                }
            } else {
                break;
            }
        }
    }

    fn next_token(&mut self) -> Result<Token> {
        self.skip_whitespace();

        let line = self.line;
        let col = self.col;

        let ch = match self.peek() {
            Some(c) => c,
            None => return Ok(Token { kind: TokenKind::Eof, line, col }),
        };

        // Newline
        if ch == '\n' {
            self.advance();
            return Ok(Token { kind: TokenKind::Newline, line, col });
        }

        // String literal
        if ch == '"' {
            return self.lex_string(line, col);
        }

        // Number or duration literal
        if ch.is_ascii_digit() {
            return self.lex_number(line, col);
        }

        // Identifier or keyword
        if ch.is_alphabetic() || ch == '_' {
            return self.lex_ident(line, col);
        }

        // Symbols
        self.advance();
        let kind = match ch {
            '{' => TokenKind::LBrace,
            '}' => TokenKind::RBrace,
            '(' => TokenKind::LParen,
            ')' => TokenKind::RParen,
            '[' => TokenKind::LBracket,
            ']' => TokenKind::RBracket,
            '.' => TokenKind::Dot,
            ',' => TokenKind::Comma,
            ':' => TokenKind::Colon,
            ';' => TokenKind::Semicolon,
            '+' => TokenKind::Plus,
            '-' => {
                if self.peek() == Some('>') {
                    self.advance();
                    TokenKind::Arrow
                } else {
                    TokenKind::Minus
                }
            }
            '*' => TokenKind::Star,
            '/' => TokenKind::Slash,
            '%' => TokenKind::Percent,
            '=' => {
                if self.peek() == Some('=') {
                    self.advance();
                    TokenKind::EqEq
                } else {
                    TokenKind::Eq
                }
            }
            '!' => {
                if self.peek() == Some('=') {
                    self.advance();
                    TokenKind::BangEq
                } else {
                    TokenKind::Bang
                }
            }
            '<' => {
                if self.peek() == Some('=') {
                    self.advance();
                    TokenKind::LtEq
                } else {
                    TokenKind::Lt
                }
            }
            '>' => {
                if self.peek() == Some('=') {
                    self.advance();
                    TokenKind::GtEq
                } else {
                    TokenKind::Gt
                }
            }
            '&' => {
                if self.peek() == Some('&') {
                    self.advance();
                    TokenKind::AmpAmp
                } else {
                    return Err(SpError::Lexer { line, col, msg: "expected '&&'".into() });
                }
            }
            '|' => {
                if self.peek() == Some('|') {
                    self.advance();
                    TokenKind::PipePipe
                } else {
                    return Err(SpError::Lexer { line, col, msg: "expected '||'".into() });
                }
            }
            _ => {
                return Err(SpError::Lexer {
                    line, col,
                    msg: format!("unexpected character: '{}'", ch),
                });
            }
        };

        Ok(Token { kind, line, col })
    }

    fn lex_string(&mut self, line: usize, col: usize) -> Result<Token> {
        self.advance(); // consume opening "
        let mut s = String::new();
        loop {
            match self.advance() {
                Some('"') => break,
                Some('\\') => {
                    match self.advance() {
                        Some('n') => s.push('\n'),
                        Some('t') => s.push('\t'),
                        Some('\\') => s.push('\\'),
                        Some('"') => s.push('"'),
                        Some(c) => s.push(c),
                        None => return Err(SpError::Lexer { line, col, msg: "unterminated escape".into() }),
                    }
                }
                Some(c) => s.push(c),
                None => return Err(SpError::Lexer { line, col, msg: "unterminated string".into() }),
            }
        }
        Ok(Token { kind: TokenKind::StringLit(s), line, col })
    }

    fn lex_number(&mut self, line: usize, col: usize) -> Result<Token> {
        let mut num_str = String::new();
        let mut is_float = false;

        while let Some(c) = self.peek() {
            if c.is_ascii_digit() {
                num_str.push(c);
                self.advance();
            } else if c == '.' && !is_float {
                is_float = true;
                num_str.push(c);
                self.advance();
            } else {
                break;
            }
        }

        // Check for duration suffix
        if let Some(c) = self.peek() {
            if c == 's' || c == 'm' || c == 'u' || c == 'n' {
                let base: f64 = num_str.parse().unwrap_or(0.0);
                let mut suffix = String::new();
                while let Some(c) = self.peek() {
                    if c.is_alphabetic() {
                        suffix.push(c);
                        self.advance();
                    } else {
                        break;
                    }
                }
                let nanos = match suffix.as_str() {
                    "s" => (base * 1_000_000_000.0) as u64,
                    "ms" => (base * 1_000_000.0) as u64,
                    "us" => (base * 1_000.0) as u64,
                    "ns" => base as u64,
                    _ => return Err(SpError::Lexer { line, col, msg: format!("unknown duration suffix: {}", suffix) }),
                };
                return Ok(Token { kind: TokenKind::DurationLit(nanos), line, col });
            }
        }

        if is_float {
            let val: f64 = num_str.parse().map_err(|_| SpError::Lexer { line, col, msg: "invalid float".into() })?;
            Ok(Token { kind: TokenKind::FloatLit(val), line, col })
        } else {
            let val: i64 = num_str.parse().map_err(|_| SpError::Lexer { line, col, msg: "invalid integer".into() })?;
            Ok(Token { kind: TokenKind::IntLit(val), line, col })
        }
    }

    fn lex_ident(&mut self, line: usize, col: usize) -> Result<Token> {
        let mut ident = String::new();
        while let Some(c) = self.peek() {
            if c.is_alphanumeric() || c == '_' {
                ident.push(c);
                self.advance();
            } else {
                break;
            }
        }

        let kind = match ident.as_str() {
            "domain" => TokenKind::Domain,
            "lane" => TokenKind::Lane,
            "in" => TokenKind::In,
            "grant" => TokenKind::Grant,
            "sealed" => TokenKind::Sealed,
            "events" => TokenKind::Events,
            "state" => TokenKind::State,
            "assoc" => TokenKind::Assoc,
            "clock" => TokenKind::Clock,
            "accel" => TokenKind::Accel,
            "priority" => TokenKind::Priority,
            "energy" => TokenKind::Energy,
            "affinity" => TokenKind::Affinity,
            "low" => TokenKind::Low,
            "normal" => TokenKind::Normal,
            "high" => TokenKind::High,
            "realtime" => TokenKind::Realtime,
            "balanced" => TokenKind::Balanced,
            "unbounded" => TokenKind::Unbounded,
            "any" => TokenKind::Any,
            "little" => TokenKind::Little,
            "big" => TokenKind::Big,
            "npu" => TokenKind::Npu,
            "gpu" => TokenKind::Gpu,
            "dsp" => TokenKind::Dsp,
            "let" => TokenKind::Let,
            "loop" => TokenKind::Loop,
            "if" => TokenKind::If,
            "else" => TokenKind::Else,
            "return" => TokenKind::Return,
            "rw" => TokenKind::Rw,
            "ro" => TokenKind::Ro,
            "true" => TokenKind::BoolTrue,
            "false" => TokenKind::BoolFalse,
            _ => TokenKind::Ident(ident),
        };

        Ok(Token { kind, line, col })
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_lex_domain() {
        let mut lexer = Lexer::new("domain MyDomain { sealed }");
        let tokens = lexer.tokenize().unwrap();
        assert!(matches!(tokens[0].kind, TokenKind::Domain));
        assert!(matches!(tokens[1].kind, TokenKind::Ident(ref s) if s == "MyDomain"));
        assert!(matches!(tokens[2].kind, TokenKind::LBrace));
        assert!(matches!(tokens[3].kind, TokenKind::Sealed));
        assert!(matches!(tokens[4].kind, TokenKind::RBrace));
    }

    #[test]
    fn test_lex_duration() {
        let mut lexer = Lexer::new("1s 500ms 100us 50ns");
        let tokens = lexer.tokenize().unwrap();
        assert!(matches!(tokens[0].kind, TokenKind::DurationLit(1_000_000_000)));
        assert!(matches!(tokens[1].kind, TokenKind::DurationLit(500_000_000)));
        assert!(matches!(tokens[2].kind, TokenKind::DurationLit(100_000)));
        assert!(matches!(tokens[3].kind, TokenKind::DurationLit(50)));
    }

    #[test]
    fn test_lex_string() {
        let mut lexer = Lexer::new(r#""hello world""#);
        let tokens = lexer.tokenize().unwrap();
        assert!(matches!(tokens[0].kind, TokenKind::StringLit(ref s) if s == "hello world"));
    }

    #[test]
    fn test_lex_msi_call() {
        let mut lexer = Lexer::new(r#"event.publish("phone/status", "ready")"#);
        let tokens = lexer.tokenize().unwrap();
        assert!(matches!(tokens[0].kind, TokenKind::Ident(ref s) if s == "event"));
        assert!(matches!(tokens[1].kind, TokenKind::Dot));
        assert!(matches!(tokens[2].kind, TokenKind::Ident(ref s) if s == "publish"));
        assert!(matches!(tokens[3].kind, TokenKind::LParen));
    }
}
