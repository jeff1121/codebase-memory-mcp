//! Pipeline language module-directory classifier parity。
//!
//! Go 與 Java 的 module QN 取自 containing directory；其他語言仍沿用檔名 stem。

pub const CBM_LANG_GO: i32 = 0;
pub const CBM_LANG_JAVA: i32 = 6;

#[must_use]
pub fn is_module_dir(language: i32) -> bool {
    language == CBM_LANG_GO || language == CBM_LANG_JAVA
}

#[cfg(test)]
mod tests {
    use super::is_module_dir;

    #[test]
    fn matches_go_and_java_contract() {
        assert!(is_module_dir(0));
        assert!(is_module_dir(6));
        assert!(!is_module_dir(1));
        assert!(!is_module_dir(999));
    }
}
