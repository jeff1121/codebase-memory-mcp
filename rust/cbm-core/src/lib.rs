//! codebase-memory-mcp 階段性 Rust 重構的 foundation code。
//!
//! 這個 crate 先作為低風險 C foundation helpers 的 parity layer；預設不接入
//! production C binary。

pub mod ffi;
pub mod foundation;
pub mod pipeline;
