/* SPDX-License-Identifier: GPL-3.0-only OR MIT */
#define JSON_IMPLEMENTATION
#include <sjson.hpp>
int main() {
    sjson::Arena arena;
    auto value = arena.parse_replace("{\"x\":17}");
    return value && value.obj("x").as_i64() == 17 ? 0 : 1;
}
