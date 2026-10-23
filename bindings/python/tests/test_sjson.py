import concurrent.futures
import json
import unittest
from sjson import Document, loads, normalize


class BindingTests(unittest.TestCase):
    def test_types_and_nul(self):
        expected = {"s": "a\0b", "unicode": "é中𝄞", "a": [None, True, -17, 1.25]}
        self.assertEqual(loads(json.dumps(expected)), expected)

    def test_integer_limits(self):
        self.assertEqual(loads("9223372036854775807"), 2**63 - 1)
        self.assertEqual(loads("-9223372036854775808"), -(2**63))
        with self.assertRaises(ValueError):
            loads("9223372036854775808")

    def test_strict_input(self):
        for value in (b'"\xc0\xaf"', b'"\xed\xa0\x80"', "{}x", "[1,]", '{"x":1,}', "NaN", "[["):
            with self.subTest(value=value), self.assertRaises(ValueError):
                loads(value)
        with self.assertRaises(TypeError):
            loads(123)

    def test_normalize(self):
        self.assertEqual(normalize(' { "x" : [ 1, true ] } '), '{"x":[1,true]}')

    def test_close_and_context(self):
        with Document('{"x":1}') as document:
            result = document.to_python()
        document.close()
        self.assertEqual(result, {"x": 1})
        with self.assertRaises(ValueError):
            document.serialize()
        with self.assertRaises(ValueError):
            document.to_python()

    def test_shared_document(self):
        with Document('{"a":[1,2,3]}') as document:
            with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
                values = list(pool.map(lambda _: document.to_python(), range(200)))
            self.assertTrue(all(value == {"a": [1, 2, 3]} for value in values))

    def test_repeated_success_and_failure(self):
        for _ in range(1000):
            self.assertEqual(loads('[1,{"x":2}]'), [1, {"x": 2}])
            with self.assertRaises(ValueError):
                loads('{"unfinished":')


if __name__ == "__main__":
    unittest.main()
