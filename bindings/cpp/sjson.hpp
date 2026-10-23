/* SPDX-License-Identifier: GPL-3.0-only OR MIT */
#ifndef SJSON_CPP_HPP
#define SJSON_CPP_HPP
#include <json_pal.h>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

namespace sjson {
namespace detail {
struct Storage {
    JsonArena* arena;
    std::uint64_t generation = 0;
    explicit Storage(std::size_t bytes) : arena(json_arena_create(nullptr, bytes)) {
        if (!arena) throw std::bad_alloc();
    }
    ~Storage() { json_arena_destroy(arena); }
    Storage(const Storage&) = delete;
    Storage& operator=(const Storage&) = delete;
};
}
class Arena;
class Value {
public:
    Value() noexcept = default;
    // Native handles remain borrowed. Arena-produced values share storage
    // and become empty after reset()/trim(), instead of dangling.
    explicit Value(JsonValue* value) noexcept : value_(value) {}
    JsonValue* get() const noexcept {
        return !owner_ || owner_->generation == generation_ ? value_ : nullptr;
    }
    explicit operator bool() const noexcept { return get() != nullptr; }
    JsonType type() const noexcept { return get() ? get()->type : JSON_NULL; }
    Value at(std::uint32_t index) const noexcept {
        JsonValue* out = nullptr;
        return json_arr_get(get(), index, &out) == JSON_OK ? child(out) : Value();
    }
    Value obj(std::string_view key) const noexcept {
        JsonValue* out = nullptr;
        return json_obj_get_n(get(), key.data(), key.size(), &out) == JSON_OK ? child(out) : Value();
    }
    std::size_t size() const noexcept {
        auto value = get();
        if (!value) return 0;
        switch (value->type) {
        case JSON_ARRAY: return value->v.a.len;
        case JSON_OBJECT: return value->v.o.len;
        case JSON_STRING: return value->v.s.len;
        default: return 0;
        }
    }
    bool as_bool(bool fallback = false) const noexcept {
        bool out = fallback; json_get_bool(get(), &out); return out;
    }
    std::int64_t as_i64(std::int64_t fallback = 0) const noexcept {
        std::int64_t out = fallback; json_get_int(get(), &out); return out;
    }
    double as_number(double fallback = 0.0) const noexcept {
        double out = fallback; json_get_number(get(), &out); return out;
    }
    // The view remains valid until this document is reset or trimmed.
    std::string_view as_string() const noexcept {
        const char* data = nullptr; std::uint32_t len = 0;
        return json_get_string(get(), &data, &len) == JSON_OK ? std::string_view(data, len) : std::string_view();
    }
    std::string string_copy() const { return std::string(as_string()); }
    std::string serialize(const JsonWriteOpts* options = nullptr) const {
        std::size_t size = 0, written = 0;
        auto status = json_measure(get(), &size, options);
        if (status != JSON_OK) throw std::runtime_error(json_error_str(status));
        if (size == static_cast<std::size_t>(-1)) throw std::length_error("JSON output too large");
        std::string output(size + 1, '\0');
        status = json_write(get(), output.data(), output.size(), &written, options);
        if (status != JSON_OK) throw std::runtime_error(json_error_str(status));
        output.resize(written);
        return output;
    }
private:
    friend class Arena;
    JsonValue* value_ = nullptr;
    std::shared_ptr<detail::Storage> owner_;
    std::uint64_t generation_ = 0;
    Value(JsonValue* value, std::shared_ptr<detail::Storage> owner) noexcept
        : value_(value), owner_(std::move(owner)), generation_(owner_ ? owner_->generation : 0) {}
    Value child(JsonValue* value) const noexcept { return Value(value, owner_); }
};
class Arena {
public:
    explicit Arena(std::size_t bytes = 64U * 1024U) : owner_(std::make_shared<detail::Storage>(bytes)) {}
    Arena(const Arena&) = delete;
    Arena& operator=(const Arena&) = delete;
    Arena(Arena&&) noexcept = default;
    Arena& operator=(Arena&&) noexcept = default;
    JsonArena* get() const noexcept { return owner_ ? owner_->arena : nullptr; }
    JsonError last_error() const noexcept { return last_error_; }
    const char* last_error_string() const noexcept { return json_error_str(last_error_); }
    void reset() noexcept {
        if (owner_) { ++owner_->generation; json_arena_reset(get()); }
        last_error_ = JSON_OK;
    }
    void trim() noexcept {
        if (owner_) { ++owner_->generation; json_arena_trim(get()); }
        last_error_ = JSON_OK;
    }
    Value parse(std::string_view input) noexcept {
        return Value(json_parse(get(), input.data(), input.size(), &last_error_), owner_);
    }
    Value parse_replace(std::string_view input) noexcept { reset(); return parse(input); }
    Value clone(Value value) noexcept {
        auto result = json_clone(get(), value.get());
        last_error_ = result ? JSON_OK : JSON_ERR_OOM;
        return Value(result, owner_);
    }
    JsonError finalize(Value value) noexcept {
        if (value.owner_ != owner_) return last_error_ = JSON_ERR_NULL_PARAM;
        return last_error_ = json_obj_finalize(get(), value.get());
    }
private:
    std::shared_ptr<detail::Storage> owner_;
    JsonError last_error_ = JSON_OK;
};
}
#endif
