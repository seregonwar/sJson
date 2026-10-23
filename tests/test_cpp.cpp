/* SPDX-License-Identifier: GPL-3.0-only OR MIT */
#define JSON_IMPLEMENTATION
#include "sjson.hpp"
#include <stdexcept>
static void check(bool value) { if (!value) throw std::runtime_error("C++ binding regression"); }
int main() {
    sjson::Value held;
    {
        sjson::Arena arena(4096);
        auto root = arena.parse("{\"s\":\"a\\u0000b\",\"a\":[17,true]}");
        check(root && root.size() == 2);
        held = root.obj("s");
        check(held.as_string() == std::string_view("a\0b", 3));
        check(root.obj("a").at(0).as_i64() == 17 && root.obj("a").at(1).as_bool());
        auto moved = std::move(arena);
        check(moved.get() && !arena.get());
    }
    check(held.string_copy() == std::string("a\0b", 3));
    sjson::Arena arena;
    auto stale = arena.parse("{\"x\":1}");
    auto current = arena.parse_replace("{\"x\":2}");
    check(!stale && current.obj("x").as_i64() == 2);
    auto clone = arena.clone(held);
    check(clone && clone.string_copy() == std::string("a\0b", 3));
    arena.trim();
    check(!current && !clone);
    check(arena.parse("0").as_i64(-1) == 0);
    sjson::Arena target;
    auto foreign = arena.parse("{}");
    check(target.finalize(foreign) == JSON_ERR_NULL_PARAM);
    auto sortable = arena.parse("{\"z\":1,\"q\":2,\"a\":3,\"b\":4}");
    JsonWriteOpts options = {false, 2, true, false, false};
    check(sortable.serialize(&options) == "{\"a\":3,\"b\":4,\"q\":2,\"z\":1}");
}
