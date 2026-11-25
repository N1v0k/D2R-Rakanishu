



#include "bitknit.h"
#include "bitknit_model.h"

#include <cstring>

namespace monpic {
namespace {

using bitknit::kProbBits;
using bitknit::kProbTotal;
using bitknit::Model;
constexpr uint32_t kStateLow = 1u << 16;
constexpr size_t kQuantum = 1u << 16;

class Decoder {
public:
    Decoder(const uint8_t *src, size_t src_len, uint8_t *dst, size_t dst_len)
        : src_(src), src_len_(src_len), dst_(dst), dst_len_(dst_len) {
        for (auto &m : command_) m.init(300, 36);
        for (auto &m : cache_ref_) m.init(40, 0);
        offset_bits_.init(21, 0);
        for (auto &o : cache_) o = 1;
    }

    BitknitError run() {
        if (src_len_ < 2 || src_[0] != 0xB1 || src_[1] != 0x75) return BitknitError::magic;
        pos_ = 2;
        while (out_ < dst_len_) {
            if (pos_ >= src_len_) return BitknitError::truncated;
            const BitknitError e = quantum();
            if (e != BitknitError::none) return e;
        }
        return BitknitError::none;
    }

private:

    uint32_t word() {
        if (src_len_ - pos_ < 2 || pos_ > src_len_) {
            truncated_ = true;
            return 0;
        }
        const uint32_t w = src_[pos_] | static_cast<uint32_t>(src_[pos_ + 1]) << 8;
        pos_ += 2;
        return w;
    }

    void renormalise() {
        if (state_[0] < kStateLow) state_[0] = state_[0] << 16 | word();
        const uint32_t t = state_[0];
        state_[0] = state_[1];
        state_[1] = t;
    }

    uint32_t bits(uint32_t n) {
        const uint32_t v = state_[0] & ((1u << n) - 1);
        state_[0] >>= n;
        renormalise();
        return v;
    }

    int symbol(Model &m) {
        const uint32_t code = state_[0] & (kProbTotal - 1);
        uint32_t start, width;
        const int s = m.find(code, &start, &width);
        state_[0] = width * (state_[0] >> kProbBits) + code - start;
        renormalise();
        m.update(s);
        return s;
    }



    void start_streams() {
        const uint32_t w0 = word(), w1 = word();
        uint32_t init = (w0 << 16 | w1) >> 4;
        const uint32_t split = w1 & 15;
        if (init < kStateLow) init = init << 16 | word();
        state_[0] = init >> split;
        if (state_[0] < kStateLow) state_[0] = state_[0] << 16 | word();
        const uint64_t low = (static_cast<uint64_t>(init) << 16 | word()) & ((uint64_t{1} << (16 + split)) - 1);
        state_[1] = static_cast<uint32_t>((uint64_t{1} << (16 + split)) | low);
    }

    BitknitError quantum() {
        const size_t end = dst_len_ - out_ < kQuantum - out_ % kQuantum ? dst_len_ : out_ + kQuantum - out_ % kQuantum;
        if (src_len_ - pos_ >= 2 && src_[pos_] == 0 && src_[pos_ + 1] == 0) {
            pos_ += 2;
            const size_t n = end - out_;
            if (src_len_ - pos_ < n) return BitknitError::truncated;
            std::memcpy(dst_ + out_, src_ + pos_, n);
            pos_ += n;
            out_ = end;
            return BitknitError::none;
        }
        start_streams();
        while (out_ < end && !truncated_) {
            if (out_ == 0) {
                dst_[out_++] = static_cast<uint8_t>(bits(8));
                continue;
            }
            const BitknitError e = command();
            if (e != BitknitError::none) return e;
        }
        if (truncated_) return BitknitError::truncated;
        if (state_[0] != kStateLow || state_[1] != kStateLow) return BitknitError::final_state;
        return BitknitError::none;
    }

    BitknitError command() {
        const int c = symbol(command_[out_ % 4]);
        if (c < 256) {
            if (last_offset_ > out_) return BitknitError::command;
            dst_[out_] = static_cast<uint8_t>(c + dst_[out_ - last_offset_]);
            out_++;
            return BitknitError::none;
        }
        size_t length;
        if (c < 288) {
            length = static_cast<size_t>(c - 254);
        } else {
            const uint32_t n = static_cast<uint32_t>(c - 287);
            length = (size_t{1} << n) + bits(n) + 32;
        }
        const int r = symbol(cache_ref_[out_ % 4]);
        uint64_t offset;
        if (r < 8) {
            offset = cache_[r];
            for (int i = r; i > 0; i--) cache_[i] = cache_[i - 1];
            cache_[0] = offset;
        } else {
            const int b = symbol(offset_bits_);
            uint64_t low = bits(static_cast<uint32_t>(b & 15));
            if (b >= 16) low = low << 16 | word();
            offset = (uint64_t{1} << (5 + b)) + (low << 5) + static_cast<uint64_t>(r) - 39;
            cache_[7] = cache_[6];
            cache_[6] = offset;
        }
        if (truncated_) return BitknitError::truncated;
        if (offset == 0 || offset > out_ || length > dst_len_ - out_) return BitknitError::command;
        const uint8_t *from = dst_ + out_ - offset;
        uint8_t *to = dst_ + out_;
        for (size_t i = 0; i < length; i++) to[i] = from[i];
        out_ += length;
        last_offset_ = offset;
        return BitknitError::none;
    }

    const uint8_t *src_;
    size_t src_len_, pos_ = 0;
    uint8_t *dst_;
    size_t dst_len_, out_ = 0;
    bool truncated_ = false;
    uint32_t state_[2] = {kStateLow, kStateLow};
    Model command_[4], cache_ref_[4], offset_bits_;
    uint64_t cache_[8];
    uint64_t last_offset_ = 1;
};

}

BitknitError bitknit_decode(const uint8_t *src, size_t src_len, uint8_t *dst, size_t dst_len) {
    Decoder d(src, src_len, dst, dst_len);
    return d.run();
}

const char *bitknit_error_text(BitknitError e) {
    switch (e) {
    case BitknitError::none: return "no error";
    case BitknitError::magic: return "the stream does not start with its magic";
    case BitknitError::truncated: return "the stream ends before its output is complete";
    case BitknitError::command: return "a copy reaches outside the output";
    case BitknitError::final_state: return "a quantum does not end in its initial state";
    }
    return "unknown error";
}

}
