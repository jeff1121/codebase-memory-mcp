//! `pass_compile_commands.c` 的 command string tokenizer。

const TOKEN_BUF_SIZE: usize = 4096;

#[must_use]
pub fn split_command(command: Option<&[u8]>, max_output: usize) -> Vec<Vec<u8>> {
    let Some(command) = command else {
        return Vec::new();
    };
    if max_output == 0 {
        return Vec::new();
    }

    let mut output = Vec::new();
    let mut current = Vec::with_capacity(TOKEN_BUF_SIZE);
    let mut quote = None;
    for byte in command {
        if let Some(active_quote) = quote {
            if *byte == active_quote {
                quote = None;
            } else if current.len() < TOKEN_BUF_SIZE - 1 {
                current.push(*byte);
            }
        } else if *byte == b'"' || *byte == b'\'' {
            quote = Some(*byte);
        } else if *byte == b' ' || *byte == b'\t' {
            emit_token(&mut output, &mut current, max_output);
        } else if current.len() < TOKEN_BUF_SIZE - 1 {
            current.push(*byte);
        }
    }
    emit_token(&mut output, &mut current, max_output);
    output
}

#[must_use]
pub fn resolve_path(path: Option<&[u8]>, directory: Option<&[u8]>) -> Option<Vec<u8>> {
    let path = path?;
    let mut output = if path.first() == Some(&b'/') || directory.map_or(true, |dir| dir.is_empty())
    {
        path.to_vec()
    } else {
        let directory = directory.unwrap_or_default();
        let mut joined = Vec::with_capacity(directory.len() + path.len() + 1);
        joined.extend_from_slice(directory);
        joined.push(b'/');
        joined.extend_from_slice(path);
        joined
    };
    output.truncate(4095);
    Some(output)
}

fn emit_token(output: &mut Vec<Vec<u8>>, current: &mut Vec<u8>, max_output: usize) {
    if !current.is_empty() && output.len() < max_output {
        output.push(std::mem::take(current));
    }
}

#[cfg(test)]
mod split_command_contract_tests {
    use super::split_command;

    #[test]
    fn split_command_handles_none_empty_and_zero_maximum() {
        assert_eq!(split_command(None, 4), Vec::<Vec<u8>>::new());
        assert_eq!(split_command(Some(b""), 4), Vec::<Vec<u8>>::new());
        assert_eq!(split_command(Some(b"value"), 0), Vec::<Vec<u8>>::new());
    }

    #[test]
    fn split_command_separates_spaces_tabs_but_keeps_newlines() {
        assert_eq!(
            split_command(Some(b" \talpha\tbeta gamma\nline "), 8),
            vec![b"alpha".to_vec(), b"beta".to_vec(), b"gamma\nline".to_vec()]
        );
    }

    #[test]
    fn split_command_handles_single_double_and_mixed_quotes() {
        assert_eq!(
            split_command(
                Some(b"'single value' \"double value\" mixed' quote' \"'inner'\""),
                8,
            ),
            vec![
                b"single value".to_vec(),
                b"double value".to_vec(),
                b"mixed quote".to_vec(),
                b"'inner'".to_vec(),
            ]
        );
    }

    #[test]
    fn split_command_handles_empty_unclosed_quotes_and_backslashes() {
        assert_eq!(
            split_command(Some(b"'' \"\" 'unclosed value"), 8),
            vec![b"unclosed value".to_vec()]
        );
        assert_eq!(
            split_command(Some(b"plain\\ value"), 8),
            vec![b"plain\\".to_vec(), b"value".to_vec()]
        );
    }

    #[test]
    fn split_command_preserves_raw_nul_bytes_in_the_core_parser() {
        assert_eq!(
            split_command(Some(b"left\0right"), 1),
            vec![b"left\0right".to_vec()]
        );
    }

    #[test]
    fn split_command_honors_maximum_output_count() {
        assert_eq!(
            split_command(Some(b"one two three"), 2),
            vec![b"one".to_vec(), b"two".to_vec()]
        );
    }

    #[test]
    fn split_command_caps_tokens_at_4095_bytes() {
        let short = vec![b'a'; 4094];
        let exact = vec![b'b'; 4095];
        let long = vec![b'c'; 4133];

        assert_eq!(split_command(Some(&short), 1), vec![short]);
        assert_eq!(split_command(Some(&exact), 1), vec![exact]);

        let truncated = split_command(Some(&long), 1);
        assert_eq!(truncated.len(), 1);
        assert_eq!(truncated[0].len(), 4095);
        assert!(truncated[0].iter().all(|byte| *byte == b'c'));
    }

    #[test]
    fn split_command_preserves_high_bit_bytes() {
        assert_eq!(
            split_command(Some(&[0x80, 0xff, b' ', 0xc3, 0xa9]), 4),
            vec![vec![0x80, 0xff], vec![0xc3, 0xa9]]
        );
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn splits_whitespace_and_removes_quotes() {
        assert_eq!(
            split_command(Some(br#"cc -I"include path" 'file name.c'"#), 8),
            vec![
                b"cc".to_vec(),
                b"-Iinclude path".to_vec(),
                b"file name.c".to_vec()
            ]
        );
    }

    #[test]
    fn respects_output_limit_and_null_input() {
        assert_eq!(
            split_command(Some(b"a b c"), 2),
            vec![b"a".to_vec(), b"b".to_vec()]
        );
        assert!(split_command(None, 4).is_empty());
        assert_eq!(
            resolve_path(Some(b"src/a.c"), Some(b"/repo")),
            Some(b"/repo/src/a.c".to_vec())
        );
        assert_eq!(
            resolve_path(Some(b"/tmp/a.c"), Some(b"/repo")),
            Some(b"/tmp/a.c".to_vec())
        );
    }
}
