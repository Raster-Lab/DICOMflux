// MIT; Copyright (c) 2026 Raster Images. DF-0 models, not a DICOM writer.
#pragma once
#include <cstdint>
#include <limits>

namespace dicomflux::probe {
inline bool add(std::uint64_t a, std::uint64_t b, std::uint64_t &out) noexcept {
    out = 0;
    if (b > std::numeric_limits<std::uint64_t>::max() - a) return false;
    out = a + b;
    return true;
}
inline bool multiply(std::uint64_t a, std::uint64_t b, std::uint64_t &out) noexcept {
    out = 0;
    if (a != 0 && b > std::numeric_limits<std::uint64_t>::max() / a) return false;
    out = a * b;
    return true;
}
inline bool even_length(std::uint64_t logical, std::uint64_t &encoded) noexcept {
    return add(logical, logical & 1U, encoded);
}
class budget {
    std::uint64_t ceiling_, used_ = 0, peak_ = 0;
public:
    explicit budget(std::uint64_t ceiling) : ceiling_(ceiling) {}
    bool reserve(std::uint64_t count) noexcept {
        if (count > ceiling_ - used_) return false;
        used_ += count;
        if (used_ > peak_) peak_ = used_;
        return true;
    }
    bool release(std::uint64_t count) noexcept {
        if (count > used_) return false;
        used_ -= count;
        return true;
    }
    std::uint64_t used() const noexcept { return used_; }
    std::uint64_t peak() const noexcept { return peak_; }
};
enum class plan_state { ready, executing, succeeded, failed, cancelled };
class single_use_plan {
    plan_state state_ = plan_state::ready;
public:
    bool start(bool cancelled) noexcept {
        if (state_ != plan_state::ready) return false;
        state_ = cancelled ? plan_state::cancelled : plan_state::executing;
        return !cancelled;
    }
    bool finish(plan_state terminal) noexcept {
        if (state_ != plan_state::executing ||
            (terminal != plan_state::succeeded && terminal != plan_state::failed &&
             terminal != plan_state::cancelled)) return false;
        state_ = terminal;
        return true;
    }
    plan_state state() const noexcept { return state_; }
};
enum class io_status { ok, eof, error, would_block, cancelled };
enum class progress_status { more, done, failed, cancelled };
// Cursor changes once for valid partial progress, including partial errors.
// Caller asks for <= remaining bytes. A finished stream must not call again.
inline progress_status progress(std::uint64_t total, std::uint64_t requested,
                                std::uint64_t count, io_status status,
                                bool source, std::uint64_t &cursor) noexcept {
    if (static_cast<unsigned>(status) > static_cast<unsigned>(io_status::cancelled) ||
        cursor > total || cursor == total || requested == 0 ||
        requested > total - cursor || count > requested) return progress_status::failed;
    cursor += count;
    if (status == io_status::cancelled) return progress_status::cancelled;
    if (status == io_status::error || status == io_status::would_block)
        return progress_status::failed;
    if (status == io_status::eof && (!source || cursor != total))
        return progress_status::failed;
    if (count == 0) return progress_status::failed;
    return cursor == total ? progress_status::done : progress_status::more;
}
}
