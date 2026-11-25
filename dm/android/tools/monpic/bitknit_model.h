
#pragma once
#include <cstdint>

namespace monpic {
namespace bitknit {

constexpr uint32_t kProbBits = 15;
constexpr uint32_t kProbTotal = 1u << kProbBits;
constexpr uint32_t kAdaptInterval = 1024;
constexpr int kMaxSymbols = 300;

class Model {
public:

    void init(int n, int min_symbols) {
        n_ = n;
        const int equal = n - min_symbols;
        for (int i = 0; i <= n; i++)
            cdf_[i] = static_cast<uint16_t>(i < equal ? (kProbTotal - min_symbols) * static_cast<uint32_t>(i) / equal
                                                      : kProbTotal - n + i);
        for (int i = 0; i < n; i++) seen_[i] = 1;
        count_ = 0;
        step_ = static_cast<uint16_t>((kProbTotal - n) / kAdaptInterval);
        last_step_ = static_cast<uint16_t>(kProbTotal + 1 - n - kAdaptInterval * step_);
    }


    int find(uint32_t code, uint32_t *start, uint32_t *width) const {
        int lo = 0, hi = n_;
        while (hi - lo > 1) {
            const int mid = (lo + hi) / 2;
            if (cdf_[mid] <= code) lo = mid;
            else hi = mid;
        }
        *start = cdf_[lo];
        *width = static_cast<uint32_t>(cdf_[lo + 1]) - cdf_[lo];
        return lo;
    }


    void range(int symbol, uint32_t *start, uint32_t *width) const {
        *start = cdf_[symbol];
        *width = static_cast<uint32_t>(cdf_[symbol + 1]) - cdf_[symbol];
    }

    void update(int symbol) {
        seen_[symbol] = static_cast<uint16_t>(seen_[symbol] + step_);
        if (++count_ < kAdaptInterval) return;
        count_ = 0;
        seen_[symbol] = static_cast<uint16_t>(seen_[symbol] + last_step_);
        uint32_t sum = 0;
        for (int i = 0; i <= n_; i++) {

            const uint32_t old = cdf_[i];
            cdf_[i] = static_cast<uint16_t>(old + ((sum - old) >> 1));
            if (i < n_) sum += seen_[i];
        }
        for (int i = 0; i < n_; i++) seen_[i] = 1;
    }

private:
    int n_ = 0;
    uint32_t count_ = 0;
    uint16_t step_ = 0, last_step_ = 0;
    uint16_t cdf_[kMaxSymbols + 1];
    uint16_t seen_[kMaxSymbols];
};

}
}
