//! Native sJson documents with RAII ownership and lifetime-bound values.
//!
//! ```
//! let doc = sjson_native::Document::parse(br#"{"x":[17,true]}"#)?;
//! assert_eq!(doc.root().get("x").unwrap().at(0).unwrap().as_i64(), Some(17));
//! # Ok::<(), sjson_native::Error>(())
//! ```
use std::{
    ffi::{c_char, c_int, c_void, CStr},
    fmt,
    marker::PhantomData,
    ptr::NonNull,
};

mod ffi {
    use super::*;
    extern "C" {
        pub fn sjson_document_parse(
            data: *const c_char,
            len: usize,
            error: *mut c_int,
        ) -> *mut c_void;
        pub fn sjson_document_free(doc: *mut c_void);
        pub fn sjson_document_root(doc: *const c_void) -> *mut c_void;
        pub fn sjson_value_member(
            value: *const c_void,
            key: *const c_char,
            len: usize,
        ) -> *mut c_void;
        pub fn sjson_value_at(value: *const c_void, index: u32) -> *mut c_void;
        pub fn sjson_value_kind(value: *const c_void) -> c_int;
        pub fn sjson_value_size(value: *const c_void) -> usize;
        pub fn sjson_value_bool(value: *const c_void, output: *mut c_int) -> c_int;
        pub fn sjson_value_int(value: *const c_void, output: *mut i64) -> c_int;
        pub fn sjson_value_number(value: *const c_void, output: *mut f64) -> c_int;
        pub fn sjson_value_string(
            value: *const c_void,
            output: *mut *const c_char,
            len: *mut usize,
        ) -> c_int;
        pub fn sjson_value_write(
            value: *const c_void,
            output: *mut c_char,
            cap: usize,
            len: *mut usize,
        ) -> c_int;
        pub fn sjson_error_message(error: c_int) -> *const c_char;
    }
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Error(pub i32);
impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        // The C ABI returns a static, NUL-terminated message for every code.
        let text = unsafe { CStr::from_ptr(ffi::sjson_error_message(self.0)) };
        write!(f, "{}", text.to_string_lossy())
    }
}
impl std::error::Error for Error {}

pub struct Document {
    raw: NonNull<c_void>,
}
// Documents expose immutable reads only; each document owns an independent
// arena and Drop requires exclusive ownership. No parser-global state is used.
unsafe impl Send for Document {}
unsafe impl Sync for Document {}
impl Document {
    pub fn parse(input: impl AsRef<[u8]>) -> Result<Self, Error> {
        let bytes = input.as_ref();
        let mut code = 0;
        let raw =
            unsafe { ffi::sjson_document_parse(bytes.as_ptr().cast(), bytes.len(), &mut code) };
        NonNull::new(raw)
            .map(|raw| Self { raw })
            .ok_or(Error(if code == 0 { -2 } else { code }))
    }
    pub fn root(&self) -> Value<'_> {
        let raw = unsafe { ffi::sjson_document_root(self.raw.as_ptr()) };
        Value {
            raw: NonNull::new(raw).expect("native document must have a root"),
            owner: PhantomData,
        }
    }
}
impl Drop for Document {
    fn drop(&mut self) {
        unsafe { ffi::sjson_document_free(self.raw.as_ptr()) }
    }
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Kind {
    Null,
    Bool,
    Integer,
    Float,
    String,
    Array,
    Object,
}

/// A read-only value that cannot outlive its document.
///
/// ```compile_fail
/// let value;
/// { let doc = sjson_native::Document::parse(b"17").unwrap(); value = doc.root(); }
/// let _ = value.as_i64();
/// ```
#[derive(Clone, Copy)]
pub struct Value<'doc> {
    raw: NonNull<c_void>,
    owner: PhantomData<&'doc Document>,
}
unsafe impl Send for Value<'_> {}
unsafe impl Sync for Value<'_> {}
impl<'doc> Value<'doc> {
    fn from_raw(raw: *mut c_void) -> Option<Self> {
        NonNull::new(raw).map(|raw| Self {
            raw,
            owner: PhantomData,
        })
    }
    pub fn kind(self) -> Kind {
        match unsafe { ffi::sjson_value_kind(self.raw.as_ptr()) } {
            0 => Kind::Null,
            1 => Kind::Bool,
            2 => Kind::Integer,
            3 => Kind::Float,
            4 => Kind::String,
            5 => Kind::Array,
            6 => Kind::Object,
            _ => unreachable!("invalid native value kind"),
        }
    }
    pub fn len(self) -> usize {
        unsafe { ffi::sjson_value_size(self.raw.as_ptr()) }
    }
    pub fn is_empty(self) -> bool {
        self.len() == 0
    }
    pub fn get(self, key: &str) -> Option<Self> {
        Self::from_raw(unsafe {
            ffi::sjson_value_member(self.raw.as_ptr(), key.as_ptr().cast(), key.len())
        })
    }
    pub fn at(self, index: u32) -> Option<Self> {
        Self::from_raw(unsafe { ffi::sjson_value_at(self.raw.as_ptr(), index) })
    }
    pub fn as_bool(self) -> Option<bool> {
        let mut result = 0;
        (unsafe { ffi::sjson_value_bool(self.raw.as_ptr(), &mut result) } == 0)
            .then_some(result != 0)
    }
    pub fn as_i64(self) -> Option<i64> {
        let mut result = 0;
        (unsafe { ffi::sjson_value_int(self.raw.as_ptr(), &mut result) } == 0).then_some(result)
    }
    pub fn as_f64(self) -> Option<f64> {
        let mut result = 0.0;
        (unsafe { ffi::sjson_value_number(self.raw.as_ptr(), &mut result) } == 0).then_some(result)
    }
    pub fn as_str(self) -> Option<&'doc str> {
        let (mut data, mut len) = (std::ptr::null(), 0);
        if unsafe { ffi::sjson_value_string(self.raw.as_ptr(), &mut data, &mut len) } != 0 {
            return None;
        }
        if len == 0 {
            return Some("");
        }
        if data.is_null() {
            return None;
        }
        // The document owns these validated UTF-8 bytes for the full 'doc lifetime.
        std::str::from_utf8(unsafe { std::slice::from_raw_parts(data.cast(), len) }).ok()
    }
    pub fn to_json(self) -> Result<String, Error> {
        let mut len = 0;
        let code =
            unsafe { ffi::sjson_value_write(self.raw.as_ptr(), std::ptr::null_mut(), 0, &mut len) };
        if code != 0 {
            return Err(Error(code));
        }
        let mut output = vec![0u8; len.checked_add(1).ok_or(Error(-9))?];
        let code = unsafe {
            ffi::sjson_value_write(
                self.raw.as_ptr(),
                output.as_mut_ptr().cast(),
                output.len(),
                &mut len,
            )
        };
        if code != 0 {
            return Err(Error(code));
        }
        output.truncate(len);
        String::from_utf8(output).map_err(|_| Error(-8))
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn types_and_lifetimes() {
        let doc = Document::parse(br#"{"s":"a\u0000b","a":[null,true,17,1.25]}"#).unwrap();
        assert_eq!(doc.root().get("s").unwrap().as_str(), Some("a\0b"));
        let array = doc.root().get("a").unwrap();
        assert_eq!(array.len(), 4);
        assert_eq!(array.at(0).unwrap().kind(), Kind::Null);
        assert_eq!(array.at(1).unwrap().as_bool(), Some(true));
        assert_eq!(array.at(2).unwrap().as_i64(), Some(17));
        assert_eq!(array.at(3).unwrap().as_f64(), Some(1.25));
        assert!(array.at(4).is_none());
        assert!(doc.root().get("missing").is_none());
        let text = doc.root().to_json().unwrap();
        assert!(Document::parse(text).is_ok());
    }
    #[test]
    fn strict_errors_and_boundaries() {
        for text in ["{}x", "[1,]", "NaN", "9223372036854775808", "\"\\uD800\""] {
            assert!(Document::parse(text).is_err(), "{text}");
        }
        assert!(Document::parse(b"\"\xc0\xaf\"").is_err());
        assert_eq!(
            Document::parse(i64::MAX.to_string())
                .unwrap()
                .root()
                .as_i64(),
            Some(i64::MAX)
        );
        assert_eq!(
            Document::parse(i64::MIN.to_string())
                .unwrap()
                .root()
                .as_i64(),
            Some(i64::MIN)
        );
        assert_eq!(Document::parse("1e100").unwrap().root().as_i64(), None);
        assert_eq!(Document::parse("\"\"").unwrap().root().as_str(), Some(""));
    }
    #[test]
    fn independent_arenas_and_shared_reads() {
        let document = std::sync::Arc::new(Document::parse("{\"x\":17}").unwrap());
        let threads: Vec<_> = (0..4)
            .map(|_| {
                let document = document.clone();
                std::thread::spawn(move || {
                    for _ in 0..1000 {
                        assert_eq!(document.root().get("x").unwrap().as_i64(), Some(17));
                        let temporary = Document::parse("[true]").unwrap();
                        assert_eq!(temporary.root().at(0).unwrap().as_bool(), Some(true));
                    }
                })
            })
            .collect();
        for thread in threads {
            thread.join().unwrap();
        }
    }
}
