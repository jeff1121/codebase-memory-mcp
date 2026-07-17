#[must_use]
pub fn copy_path_without_query(path: Option<&[u8]>, out_size: usize) -> Vec<u8> {
    let path = crate::foundation::log::http_path_without_query(path);
    let copy_len = path.len().min(out_size.saturating_sub(1));
    path[..copy_len].to_vec()
}

#[cfg(test)]
mod tests {
    use super::copy_path_without_query;

    #[test]
    fn removes_query_or_fragment_and_respects_output_capacity() {
        assert_eq!(
            copy_path_without_query(Some(b"/api/items?q=1"), 64),
            b"/api/items"
        );
        assert_eq!(
            copy_path_without_query(Some(b"/api/items"), 64),
            b"/api/items"
        );
        assert_eq!(
            copy_path_without_query(Some(b"/api/items#section"), 64),
            b"/api/items"
        );
        assert_eq!(copy_path_without_query(Some(b"/api/items?q=1"), 5), b"/api");
        assert_eq!(copy_path_without_query(None, 64), b"");
        assert_eq!(copy_path_without_query(Some(b"/api"), 0), b"");
    }
}
