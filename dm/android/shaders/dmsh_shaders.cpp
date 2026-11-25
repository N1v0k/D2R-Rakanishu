









#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <condition_variable>
#include <deque>
#include <dlfcn.h>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>
#include "spirv-tools/libspirv.hpp"
#include "spirv-tools/optimizer.hpp"

#define DMSH_API extern "C" __attribute__((visibility("default")))

enum { DMSH_OK = 0, DMSH_TRANSLATE = 1, DMSH_REWRITE = 2, DMSH_TRIM = 3, DMSH_CAPS = 4, DMSH_VALIDATE = 5, DMSH_INPUT = 6 };

struct dmsh_result {
  int64_t tag;
  int32_t status;
  uint8_t *spv;
  size_t spv_len;
  char *res;
  size_t res_len;
  char *err;
};

namespace {
using xlate_one_t = int (*)(const char *, size_t, const uint8_t *, size_t, uint8_t **, size_t *, char **, size_t *, char **);
using xlate_free_t = void (*)(void *);

char *dup_str(const std::string &s) {
  char *p = static_cast<char *>(std::malloc(s.size() + 1));
  if (p) std::memcpy(p, s.c_str(), s.size() + 1);
  return p;
}


template <class F> bool walk(const std::vector<uint32_t> &w, F f) {
  if (w.size() < 5) return false;
  for (size_t i = 5; i < w.size();) {
    uint32_t n = w[i] >> 16;
    if (n == 0 || n > w.size() - i) return false;
    f(i, w[i] & 0xffff, n);
    i += n;
  }
  return true;
}

bool rewrite(std::vector<uint32_t> &w, int stage) {
  return walk(w, [&](size_t i, uint32_t op, uint32_t n) {
    if (op == 71 && n >= 4 && w[i + 2] == 33) w[i + 3] = w[i + 3] * 6 + static_cast<uint32_t>(stage);
  });
}

bool add_format_caps(std::vector<uint32_t> &w) {
  std::map<uint32_t, uint32_t> fmt, ty;
  std::set<uint32_t> have;
  std::vector<uint32_t> need;
  bool ok = walk(w, [&](size_t i, uint32_t op, uint32_t n) {
    const uint32_t *a = &w[i + 1];
    if (op == 17 && n >= 2) have.insert(a[0]);
    else if (op == 25 && n >= 9) fmt[a[0]] = a[7];
    else if (op == 61 && n >= 3) ty[a[1]] = a[0];
  });
  if (!ok) return false;
  walk(w, [&](size_t i, uint32_t op, uint32_t n) {
    const uint32_t *a = &w[i + 1];
    if (!((op == 98 && n >= 4) || (op == 99 && n >= 2))) return;
    uint32_t img = op == 98 ? a[2] : a[0], cap = op == 98 ? 55 : 56;
    auto t = ty.find(img);
    if (t == ty.end()) return;
    auto f = fmt.find(t->second);
    if (f != fmt.end() && f->second == 0 && !have.count(cap) && std::find(need.begin(), need.end(), cap) == need.end()) need.push_back(cap);
  });
  for (uint32_t cap : need) w.insert(w.begin() + 5, {(2u << 16) | 17u, cap});
  return true;
}

struct Messages {
  std::string text;
  void add(spv_message_level_t level, const spv_position_t &pos, const char *msg) {
    if (level > SPV_MSG_ERROR) return;
    if (!text.empty()) text += "\n";
    text += "error: " + std::to_string(pos.index) + ": " + (msg ? msg : "");
  }
};

int trim(std::vector<uint32_t> &w, std::string &err) {
  Messages m;
  spvtools::Optimizer opt(SPV_ENV_UNIVERSAL_1_6);
  opt.SetMessageConsumer([&m](spv_message_level_t l, const char *, const spv_position_t &p, const char *s) { m.add(l, p, s); });
  opt.RegisterPass(spvtools::CreateTrimCapabilitiesPass());
  spvtools::OptimizerOptions o;
  o.set_run_validator(false);
  std::vector<uint32_t> out;
  if (!opt.Run(w.data(), w.size(), &out, o)) { err = m.text.empty() ? "trimming failed" : m.text; return DMSH_TRIM; }
  w.swap(out);
  return DMSH_OK;
}

int validate(const std::vector<uint32_t> &w, std::string &err) {
  Messages m;
  spvtools::SpirvTools t(SPV_ENV_VULKAN_1_1);
  t.SetMessageConsumer([&m](spv_message_level_t l, const char *, const spv_position_t &p, const char *s) { m.add(l, p, s); });
  if (!t.Validate(w.data(), w.size(), spvtools::ValidatorOptions())) { err = m.text.empty() ? "validation failed" : m.text; return DMSH_VALIDATE; }
  return DMSH_OK;
}

int post(std::vector<uint32_t> &w, int stage, std::string &err) {
  if (!rewrite(w, stage)) { err = "malformed SPIR-V"; return DMSH_REWRITE; }
  int r = trim(w, err);
  if (r) return r;
  if (!add_format_caps(w)) { err = "malformed SPIR-V"; return DMSH_CAPS; }
  return validate(w, err);
}

struct Job {
  int64_t tag;
  int stage;
  std::string json;
  std::vector<uint8_t> code;
};
}

struct dmsh_pool {
  xlate_one_t one = nullptr;
  xlate_free_t xfree = nullptr;
  std::mutex mu;
  std::condition_variable work, done;
  std::deque<Job> queue;
  std::deque<dmsh_result> results;
  int running = 0;
  bool stop = false;
  std::vector<std::thread> threads;

  void run_one(Job &j, dmsh_result &r) {
    uint8_t *spv = nullptr; size_t spv_len = 0; char *res = nullptr; size_t res_len = 0; char *e = nullptr;
    int rc = one(j.json.data(), j.json.size(), j.code.data(), j.code.size(), &spv, &spv_len, &res, &res_len, &e);
    if (rc != 0) {
      r.status = rc == 1 ? DMSH_TRANSLATE : DMSH_INPUT;
      r.err = dup_str(e ? e : "the translator failed without a message");
      if (e) xfree(e);
      return;
    }
    std::string text;
    if (spv_len % 4 != 0) { r.status = DMSH_TRANSLATE; r.err = dup_str("the translator returned a partial word"); }
    else {
      std::vector<uint32_t> w(spv_len / 4);
      if (spv_len) std::memcpy(w.data(), spv, spv_len);
      std::string err;
      try { r.status = post(w, j.stage, err); }
      catch (const std::exception &x) { r.status = DMSH_TRIM; err = x.what(); }
      if (r.status != DMSH_OK) r.err = dup_str(err);
      else {
        r.spv = static_cast<uint8_t *>(std::malloc(w.empty() ? 1 : w.size() * 4));
        r.res = static_cast<char *>(std::malloc(res_len + 1));
        if (!r.spv || !r.res) { std::free(r.spv); std::free(r.res); r.spv = nullptr; r.res = nullptr; r.status = DMSH_INPUT; r.err = dup_str("out of memory"); }
        else {
          std::memcpy(r.spv, w.data(), w.size() * 4); r.spv_len = w.size() * 4;
          std::memcpy(r.res, res, res_len); r.res[res_len] = 0; r.res_len = res_len;
        }
      }
    }
    xfree(spv);
    xfree(res);
  }

  void worker() {
    for (;;) {
      Job j;
      {
        std::unique_lock<std::mutex> l(mu);
        work.wait(l, [&] { return stop || !queue.empty(); });
        if (stop) return;
        j = std::move(queue.front());
        queue.pop_front();
        running++;
      }
      dmsh_result r{};
      r.tag = j.tag;
      try { run_one(j, r); }
      catch (...) { std::free(r.spv); std::free(r.res); std::free(r.err); r = dmsh_result{}; r.tag = j.tag; r.status = DMSH_INPUT; r.err = dup_str("out of memory"); }
      std::lock_guard<std::mutex> l(mu);
      running--;
      if (stop) { std::free(r.spv); std::free(r.res); std::free(r.err); }
      else results.push_back(r);
      done.notify_all();
    }
  }
};


DMSH_API dmsh_pool *dmsh_pool_open(const char *xlate_path, int workers, char *err, size_t err_len) {
  auto fail = [&](const std::string &s) -> dmsh_pool * {
    if (err && err_len) { std::strncpy(err, s.c_str(), err_len - 1); err[err_len - 1] = 0; }
    return nullptr;
  };
  if (!xlate_path || workers < 1 || workers > 64) return fail("bad arguments");

  void *h = dlopen(xlate_path, RTLD_NOW | RTLD_LOCAL);
  if (!h) { const char *e = dlerror(); return fail(std::string("cannot load the translator: ") + (e ? e : "?")); }
  auto p = std::make_unique<dmsh_pool>();
  p->one = reinterpret_cast<xlate_one_t>(dlsym(h, "dmsh_xlate_one"));
  p->xfree = reinterpret_cast<xlate_free_t>(dlsym(h, "dmsh_xlate_free"));
  if (!p->one || !p->xfree) return fail("the translator lacks dmsh_xlate_one / dmsh_xlate_free");
  try {
    for (int i = 0; i < workers; i++) p->threads.emplace_back([q = p.get()] { q->worker(); });
  } catch (const std::exception &x) {
    { std::lock_guard<std::mutex> l(p->mu); p->stop = true; }
    p->work.notify_all();
    for (auto &t : p->threads) t.join();
    return fail(std::string("cannot start the workers: ") + x.what());
  }
  return p.release();
}



DMSH_API int dmsh_pool_submit(dmsh_pool *p, int64_t tag, int stage, const char *json, size_t json_len, const uint8_t *code, size_t code_len) {
  if (!p || !json || (!code && code_len) || stage < 0 || stage > 5) return -1;
  try {
    Job j{tag, stage, std::string(json, json_len), std::vector<uint8_t>(code, code + code_len)};
    std::lock_guard<std::mutex> l(p->mu);
    if (p->stop) return -1;
    p->queue.push_back(std::move(j));
  } catch (const std::exception &) { return -1; }
  p->work.notify_one();
  return 0;
}


DMSH_API int dmsh_pool_next(dmsh_pool *p, dmsh_result *r) {
  if (!p || !r) return -1;
  std::unique_lock<std::mutex> l(p->mu);
  p->done.wait(l, [&] { return p->stop || !p->results.empty() || (p->queue.empty() && p->running == 0); });
  if (p->stop) return -1;
  if (p->results.empty()) return 0;
  *r = p->results.front();
  p->results.pop_front();
  return 1;
}



DMSH_API void dmsh_pool_close(dmsh_pool *p) {
  if (!p) return;
  {
    std::lock_guard<std::mutex> l(p->mu);
    p->stop = true;
    p->queue.clear();
  }
  p->work.notify_all();
  p->done.notify_all();
  for (auto &t : p->threads) t.join();
  for (auto &r : p->results) { std::free(r.spv); std::free(r.res); std::free(r.err); }
  delete p;
}

DMSH_API void dmsh_result_free(dmsh_result *r) {
  if (!r) return;
  std::free(r->spv); std::free(r->res); std::free(r->err);
  r->spv = nullptr; r->res = nullptr; r->err = nullptr;
}



DMSH_API int dmsh_post(const uint8_t *in, size_t len, int stage, uint8_t **out, size_t *out_len, char **err) {
  if (out) *out = nullptr;
  if (err) *err = nullptr;
  if (!in || !out || !out_len || len % 4 || stage < 0 || stage > 5) return DMSH_INPUT;
  std::vector<uint32_t> w(len / 4);
  if (len) std::memcpy(w.data(), in, len);
  std::string e;
  int r;
  try { r = post(w, stage, e); }
  catch (const std::exception &x) { r = DMSH_TRIM; e = x.what(); }
  if (r != DMSH_OK) { if (err) *err = dup_str(e); return r; }
  *out = static_cast<uint8_t *>(std::malloc(w.empty() ? 1 : w.size() * 4));
  if (!*out) return DMSH_INPUT;
  std::memcpy(*out, w.data(), w.size() * 4);
  *out_len = w.size() * 4;
  return DMSH_OK;
}

DMSH_API void dmsh_free(void *p) { std::free(p); }
