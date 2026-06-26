/*
 * sJson v1.1.0 — safe, fast, single-header JSON library in C99.
 * SPDX-License-Identifier: GPL-3.0-only OR MIT
 *
 * This file is dual-licensed under GPL-3.0-only OR MIT.
 * You may choose either license.
 */

#ifndef SJSON_CPP_HPP
#define SJSON_CPP_HPP

#include <cstddef>
#include <cstdint>
#include <string_view>

extern "C" {
#include "../../src/json_pal.h"
}

namespace sjson {

class Value {
public:
    Value() noexcept : value_(nullptr) {}
    explicit Value(JsonValue* value) noexcept : value_(value) {}

    JsonValue* get() const noexcept { return value_; }
    explicit operator bool() const noexcept { return value_ != nullptr; }
    JsonType type() const noexcept { return value_ ? value_->type : JSON_NULL; }

    Value at(std::uint32_t index) const noexcept {
        JsonValue* out = nullptr;
        return json_arr_get(value_, index, &out) == JSON_OK ? Value(out) : Value();
    }

    Value obj(std::string_view key) const noexcept {
        JsonValue* out = nullptr;
        return json_obj_get_n(value_, key.data(), key.size(), &out) == JSON_OK ? Value(out) : Value();
    }

    bool as_bool(bool fallback = false) const noexcept {
        bool out = fallback;
        json_get_bool(value_, &out);
        return out;
    }

    std::int64_t as_i64(std::int64_t fallback = 0) const noexcept {
        std::int64_t out = fallback;
        json_get_int(value_, &out);
        return out;
    }

    double as_number(double fallback = 0.0) const noexcept {
        double out = fallback;
        json_get_number(value_, &out);
        return out;
    }

    std::string_view as_string() const noexcept {
        const char* data = nullptr;
        std::uint32_t len = 0;
        return json_get_string(value_, &data, &len) == JSON_OK
            ? std::string_view(data, len)
            : std::string_view();
    }

private:
    JsonValue* value_;
};

class Arena {
public:
    explicit Arena(std::size_t block_size = 64U * 1024U) noexcept
        : arena_(json_arena_create(nullptr, block_size)), last_error_(JSON_OK) {}

    ~Arena() { json_arena_destroy(arena_); }

    Arena(const Arena&) = delete;
    Arena& operator=(const Arena&) = delete;

    Arena(Arena&& other) noexcept
        : arena_(other.arena_), last_error_(other.last_error_) {
        other.arena_ = nullptr;
    }

    Arena& operator=(Arena&& other) noexcept {
        if (this != &other) {
            json_arena_destroy(arena_);
            arena_ = other.arena_;
            last_error_ = other.last_error_;
            other.arena_ = nullptr;
        }
        return *this;
    }

    JsonArena* get() const noexcept { return arena_; }
    JsonError last_error() const noexcept { return last_error_; }
    const char* last_error_string() const noexcept { return json_error_str(last_error_); }

    void reset() noexcept {
        json_arena_reset(arena_);
        last_error_ = JSON_OK;
    }

    Value parse(std::string_view json) noexcept {
        JsonValue* root = json_parse(arena_, json.data(), json.size(), &last_error_);
        return Value(root);
    }

    JsonError finalize(Value value) noexcept {
        last_error_ = json_obj_finalize(arena_, value.get());
        return last_error_;
    }

private:
    JsonArena* arena_;
    JsonError last_error_;
};

}

#endif
