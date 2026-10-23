"""Python ownership wrapper for the optional sJson C ABI."""
from __future__ import annotations

import ctypes as C
from ctypes.util import find_library
from functools import lru_cache
import os
from pathlib import Path
from threading import RLock
from typing import Any

__all__ = ["Document", "loads", "normalize"]


@lru_cache(maxsize=1)
def _library() -> C.CDLL:
    location = os.environ.get("SJSON_LIBRARY") or find_library("sjson_ffi")
    if not location:
        raise OSError("Build sjson_ffi with BUILD_SHARED_LIBS=ON and set SJSON_LIBRARY to its path")
    if os.name == "nt" and Path(location).is_file():
        with os.add_dll_directory(str(Path(location).resolve().parent)):
            lib = C.CDLL(location)
    else:
        lib = C.CDLL(location)
    lib.sjson_abi_version.argtypes, lib.sjson_abi_version.restype = [], C.c_uint
    if lib.sjson_abi_version() != 1:
        raise OSError("unsupported sJson native ABI")
    pointer = C.c_void_p
    signatures = {
        "sjson_document_parse": ([C.c_char_p, C.c_size_t, C.POINTER(C.c_int)], pointer),
        "sjson_document_free": ([pointer], None),
        "sjson_document_root": ([pointer], pointer),
        "sjson_value_kind": ([pointer], C.c_int),
        "sjson_value_size": ([pointer], C.c_size_t),
        "sjson_value_at": ([pointer, C.c_uint32], pointer),
        "sjson_value_bool": ([pointer, C.POINTER(C.c_int)], C.c_int),
        "sjson_value_int": ([pointer, C.POINTER(C.c_int64)], C.c_int),
        "sjson_value_number": ([pointer, C.POINTER(C.c_double)], C.c_int),
        "sjson_value_string": ([pointer, C.POINTER(pointer), C.POINTER(C.c_size_t)], C.c_int),
        "sjson_value_entry": ([pointer, C.c_uint32, C.POINTER(pointer), C.POINTER(C.c_size_t), C.POINTER(pointer)], C.c_int),
        "sjson_value_write": ([pointer, pointer, C.c_size_t, C.POINTER(C.c_size_t)], C.c_int),
        "sjson_error_message": ([C.c_int], C.c_char_p),
    }
    for name, (arguments, result) in signatures.items():
        function = getattr(lib, name)
        function.argtypes, function.restype = arguments, result
    return lib


class Document:
    """Own a native document. Use a context manager or close() to release it.

    Returned Python values are copies. Native pointers never escape this class.
    Operations and close() are serialized for callers sharing a document.
    """

    def __init__(self, data: str | bytes) -> None:
        self._handle = None
        self._lock = RLock()
        if isinstance(data, str):
            data = data.encode("utf-8")
        if not isinstance(data, bytes):
            raise TypeError("expected str or bytes")
        self._lib = _library()
        error = C.c_int()
        self._handle = self._lib.sjson_document_parse(data, len(data), C.byref(error))
        if not self._handle:
            self._check(error.value)
            raise ValueError("sJson returned an empty document")

    def _check(self, status: int) -> None:
        if status:
            message = self._lib.sjson_error_message(status).decode("utf-8")
            if status == -2:
                raise MemoryError(message)
            raise ValueError(message)

    def _root(self) -> int:
        if self._handle is None:
            raise ValueError("document is closed")
        return self._lib.sjson_document_root(self._handle)

    def close(self) -> None:
        with self._lock:
            if self._handle is not None:
                self._lib.sjson_document_free(self._handle)
                self._handle = None

    def __enter__(self) -> Document:
        with self._lock:
            self._root()
        return self

    def __exit__(self, *_: object) -> None:
        self.close()

    def __del__(self) -> None:
        if hasattr(self, "_lock"):
            self.close()

    def to_python(self) -> Any:
        with self._lock:
            return self._convert(self._root())

    def _convert(self, value: int) -> Any:
        kind = self._lib.sjson_value_kind(value)
        if kind == 0:
            return None
        if kind in (1, 2, 3):
            scalar = {1: C.c_int, 2: C.c_int64, 3: C.c_double}[kind]()
            getter = {1: "sjson_value_bool", 2: "sjson_value_int", 3: "sjson_value_number"}[kind]
            self._check(getattr(self._lib, getter)(value, C.byref(scalar)))
            return bool(scalar.value) if kind == 1 else scalar.value
        if kind == 4:
            data, size = C.c_void_p(), C.c_size_t()
            self._check(self._lib.sjson_value_string(value, C.byref(data), C.byref(size)))
            return C.string_at(data, size.value).decode("utf-8")
        size = self._lib.sjson_value_size(value)
        if kind == 5:
            return [self._convert(self._lib.sjson_value_at(value, i)) for i in range(size)]
        if kind == 6:
            result = {}
            for i in range(size):
                key, length, child = C.c_void_p(), C.c_size_t(), C.c_void_p()
                self._check(self._lib.sjson_value_entry(value, i, C.byref(key), C.byref(length), C.byref(child)))
                result[C.string_at(key, length.value).decode("utf-8")] = self._convert(child)
            return result
        raise ValueError("invalid native value")

    def serialize(self) -> str:
        with self._lock:
            root, size = self._root(), C.c_size_t()
            self._check(self._lib.sjson_value_write(root, None, 0, C.byref(size)))
            output = C.create_string_buffer(size.value + 1)
            self._check(self._lib.sjson_value_write(root, output, len(output), C.byref(size)))
            return output.raw[:size.value].decode("utf-8")


def loads(data: str | bytes) -> Any:
    with Document(data) as document:
        return document.to_python()


def normalize(data: str | bytes) -> str:
    with Document(data) as document:
        return document.serialize()
