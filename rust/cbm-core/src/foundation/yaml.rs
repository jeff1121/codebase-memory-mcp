//! 對齊 `src/foundation/yaml.c` 的 minimal YAML parser contract。
//!
//! 這個模組只供 test-only FFI parity fixture 使用，不接入產品預設 C 路徑。

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
enum NodeKind {
    Map,
    List,
    Scalar,
}

#[derive(Debug)]
pub struct Node {
    kind: NodeKind,
    key: Option<Vec<u8>>,
    value: Option<Vec<u8>>,
    children: Vec<Node>,
}

#[derive(Clone, Copy)]
struct StackEntry {
    path: [usize; 32],
    depth: usize,
    indent: i32,
}

impl Node {
    #[must_use]
    pub fn empty_map() -> Self {
        Self {
            kind: NodeKind::Map,
            key: None,
            value: None,
            children: Vec::new(),
        }
    }

    fn new(kind: NodeKind, key: Option<Vec<u8>>, value: Option<Vec<u8>>) -> Self {
        Self {
            kind,
            key,
            value,
            children: Vec::new(),
        }
    }

    #[must_use]
    pub fn parse(text: Option<&[u8]>, len: i32) -> Self {
        let Some(text) = text else {
            return Self::empty_map();
        };
        if len <= 0 {
            return Self::empty_map();
        }

        let len = (len as usize).min(text.len());
        let text = &text[..len];
        let mut root = Self::empty_map();
        let mut stack = vec![StackEntry {
            path: [0; 32],
            depth: 0,
            indent: -1,
        }];

        let mut offset = 0;
        while offset < text.len() {
            let rest = &text[offset..];
            let rel_eol = rest.iter().position(|&b| b == b'\n').unwrap_or(rest.len());
            let mut line = &rest[..rel_eol];
            if line.last() == Some(&b'\r') {
                line = &line[..line.len() - 1];
            }

            let indent = leading_spaces(line) as i32;
            let content = &line[indent as usize..];
            if !content.is_empty() && content[0] != b'#' {
                while stack.len() > 1 && stack.last().is_some_and(|entry| entry.indent >= indent) {
                    stack.pop();
                }
                let parent = *stack.last().unwrap_or(&StackEntry {
                    path: [0; 32],
                    depth: 0,
                    indent: -1,
                });

                if content.len() >= 2 && content[0] == b'-' && content[1] == b' ' {
                    add_list_item(&mut root, &parent, content);
                } else if let Some(new_entry) =
                    add_key_line(&mut root, &parent, content, indent, rest, rel_eol)
                {
                    stack.push(new_entry);
                }
            }

            offset += rel_eol;
            if offset < text.len() && text[offset] == b'\n' {
                offset += 1;
            }
        }

        root
    }

    #[must_use]
    pub fn get_str(&self, path: &[u8]) -> Option<&[u8]> {
        let node = self.navigate(path)?;
        if node.kind != NodeKind::Scalar {
            return None;
        }
        node.value.as_deref().map(trim_nul)
    }

    #[must_use]
    pub fn get_float(&self, path: &[u8], default_val: f64) -> f64 {
        let Some(value) = self.get_str(path) else {
            return default_val;
        };
        parse_float_prefix(value).unwrap_or(default_val)
    }

    #[must_use]
    pub fn get_bool(&self, path: &[u8], default_val: bool) -> bool {
        let Some(value) = self.get_str(path) else {
            return default_val;
        };
        if eq_ignore_ascii_case(value, b"true")
            || eq_ignore_ascii_case(value, b"yes")
            || eq_ignore_ascii_case(value, b"on")
            || value == b"1"
        {
            return true;
        }
        if eq_ignore_ascii_case(value, b"false")
            || eq_ignore_ascii_case(value, b"no")
            || eq_ignore_ascii_case(value, b"off")
            || value == b"0"
        {
            return false;
        }
        default_val
    }

    #[must_use]
    pub fn has(&self, path: &[u8]) -> bool {
        self.navigate(path).is_some()
    }

    #[must_use]
    pub fn str_list(&self, path: &[u8]) -> Option<Vec<&[u8]>> {
        let node = self.navigate(path)?;
        if node.kind != NodeKind::List {
            return None;
        }
        Some(
            node.children
                .iter()
                .filter_map(|child| {
                    if child.kind == NodeKind::Scalar {
                        child.value.as_deref().map(trim_nul)
                    } else {
                        None
                    }
                })
                .collect(),
        )
    }

    fn navigate(&self, path: &[u8]) -> Option<&Node> {
        if path.is_empty() {
            return None;
        }

        let mut cur = self;
        let mut start = 0;
        loop {
            let end = path[start..]
                .iter()
                .position(|&b| b == b'.')
                .map_or(path.len(), |rel| start + rel);
            let segment = &path[start..end];
            if segment.is_empty() || segment.len() >= 256 {
                return None;
            }

            cur = cur.children.iter().find(|child| {
                child
                    .key
                    .as_deref()
                    .map(trim_nul)
                    .is_some_and(|key| key == segment)
            })?;

            if end == path.len() {
                return Some(cur);
            }
            start = end + 1;
        }
    }
}

fn add_list_item(root: &mut Node, parent_entry: &StackEntry, content: &[u8]) {
    let mut item = &content[2..];
    while item.first().is_some_and(|b| b.is_ascii_whitespace()) {
        item = &item[1..];
    }

    let mut list_entry = *parent_entry;
    if let Some(parent) = get_node_mut(root, parent_entry) {
        if parent.kind == NodeKind::Map
            && parent
                .children
                .last()
                .is_some_and(|child| child.kind == NodeKind::List)
            && parent_entry.depth < parent_entry.path.len()
        {
            list_entry.path[parent_entry.depth] = parent.children.len() - 1;
            list_entry.depth = parent_entry.depth + 1;
        }
    }

    if let Some(list_parent) = get_node_mut(root, &list_entry) {
        list_parent
            .children
            .push(Node::new(NodeKind::Scalar, None, Some(trim_dup(item))));
    }
}

fn add_key_line(
    root: &mut Node,
    parent_entry: &StackEntry,
    content: &[u8],
    indent: i32,
    rest: &[u8],
    rel_eol: usize,
) -> Option<StackEntry> {
    let colon = content.iter().position(|&b| b == b':')?;
    let key = trim_dup(&content[..colon]);

    let mut after = &content[colon + 1..];
    while after.first().is_some_and(|b| b.is_ascii_whitespace()) {
        after = &after[1..];
    }
    after = strip_inline_comment(after);

    let kind;
    let value;
    let push_stack;
    if after.is_empty() {
        kind = if peek_is_list(rest, rel_eol) {
            NodeKind::List
        } else {
            NodeKind::Map
        };
        value = None;
        push_stack = true;
    } else {
        kind = NodeKind::Scalar;
        value = Some(trim_dup(after));
        push_stack = false;
    }

    let parent = get_node_mut(root, parent_entry)?;
    parent.children.push(Node::new(kind, Some(key), value));
    if !push_stack || parent_entry.depth >= parent_entry.path.len() {
        return None;
    }
    let mut child_entry = *parent_entry;
    child_entry.path[parent_entry.depth] = parent.children.len() - 1;
    child_entry.depth = parent_entry.depth + 1;
    child_entry.indent = indent;
    Some(child_entry)
}

fn get_node_mut<'a>(root: &'a mut Node, entry: &StackEntry) -> Option<&'a mut Node> {
    let mut cur = root;
    for idx in entry.path.iter().take(entry.depth) {
        cur = cur.children.get_mut(*idx)?;
    }
    Some(cur)
}

fn leading_spaces(line: &[u8]) -> usize {
    line.iter().take_while(|&&b| b == b' ').count()
}

fn trim_dup(bytes: &[u8]) -> Vec<u8> {
    let mut start = 0;
    let mut end = bytes.len();
    while end > start && bytes[end - 1].is_ascii_whitespace() {
        end -= 1;
    }
    while start < end && bytes[start].is_ascii_whitespace() {
        start += 1;
    }
    let mut out = bytes[start..end].to_vec();
    out.push(0);
    out
}

fn trim_nul(bytes: &[u8]) -> &[u8] {
    bytes.strip_suffix(&[0]).unwrap_or(bytes)
}

fn strip_inline_comment(mut after: &[u8]) -> &[u8] {
    if !after.is_empty() && after[0] != b'"' && after[0] != b'\'' {
        for i in 0..after.len() {
            if after[i] == b'#' && i > 0 && after[i - 1] == b' ' {
                after = &after[..i];
                while after.last().is_some_and(|b| b.is_ascii_whitespace()) {
                    after = &after[..after.len() - 1];
                }
                break;
            }
        }
    }
    after
}

fn peek_is_list(rest: &[u8], rel_eol: usize) -> bool {
    let mut offset = if rel_eol < rest.len() {
        rel_eol + 1
    } else {
        rest.len()
    };
    while offset < rest.len() {
        let line_rest = &rest[offset..];
        let line_len = line_rest
            .iter()
            .position(|&b| b == b'\n')
            .unwrap_or(line_rest.len());
        let mut line = &line_rest[..line_len];
        if line.last() == Some(&b'\r') {
            line = &line[..line.len() - 1];
        }
        let indent = leading_spaces(line);
        let content = &line[indent..];
        if !content.is_empty() && content[0] != b'#' {
            return content[0] == b'-';
        }
        offset += line_len;
        if offset < rest.len() && rest[offset] == b'\n' {
            offset += 1;
        }
    }
    false
}

fn eq_ignore_ascii_case(a: &[u8], b: &[u8]) -> bool {
    a.len() == b.len() && a.iter().zip(b).all(|(x, y)| x.eq_ignore_ascii_case(y))
}

fn parse_float_prefix(bytes: &[u8]) -> Option<f64> {
    let mut i = 0;
    if matches!(bytes.first(), Some(b'+' | b'-')) {
        i += 1;
    }

    let mut saw_digit = false;
    while i < bytes.len() && bytes[i].is_ascii_digit() {
        saw_digit = true;
        i += 1;
    }
    if i < bytes.len() && bytes[i] == b'.' {
        i += 1;
        while i < bytes.len() && bytes[i].is_ascii_digit() {
            saw_digit = true;
            i += 1;
        }
    }
    if !saw_digit {
        return None;
    }

    let exp_start = i;
    if i < bytes.len() && matches!(bytes[i], b'e' | b'E') {
        i += 1;
        if i < bytes.len() && matches!(bytes[i], b'+' | b'-') {
            i += 1;
        }
        let exp_digits_start = i;
        while i < bytes.len() && bytes[i].is_ascii_digit() {
            i += 1;
        }
        if exp_digits_start == i {
            i = exp_start;
        }
    }

    std::str::from_utf8(&bytes[..i]).ok()?.parse::<f64>().ok()
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn parses_nested_scalars_lists_and_comments() {
        let yaml = b"project:\n  name: app # comment\n  tags:\n    - web\n    - api\n";
        let root = Node::parse(Some(yaml), yaml.len() as i32);
        assert_eq!(root.get_str(b"project.name"), Some(&b"app"[..]));
        assert_eq!(
            root.str_list(b"project.tags").unwrap(),
            vec![&b"web"[..], &b"api"[..]]
        );
        assert!(root.has(b"project"));
        assert_eq!(root.get_str(b"project"), None);
    }

    #[test]
    fn matches_query_helpers_and_edge_cases() {
        let yaml = b"enabled: TRUE\nrate: -2.75\nchannel: #general\nkey: first\nkey: second\n";
        let root = Node::parse(Some(yaml), yaml.len() as i32);
        assert!(root.get_bool(b"enabled", false));
        assert_eq!(root.get_float(b"rate", 0.0), -2.75);
        assert_eq!(root.get_str(b"channel"), Some(&b"#general"[..]));
        assert_eq!(root.get_str(b"key"), Some(&b"first"[..]));
        assert!(!root.has(&[b'a'; 260]));
    }

    #[test]
    fn null_empty_and_partial_len_match_contract() {
        let empty = Node::parse(None, 0);
        assert!(!empty.has(b"anything"));

        let yaml = b"key: value\n";
        let partial = Node::parse(Some(yaml), 7);
        assert_eq!(partial.get_str(b"key"), Some(&b"va"[..]));
    }
}
