/**
 * @file api.hpp
 * @author 陈林锴
 */

#ifndef SPIRATION_API_HPP
#define SPIRATION_API_HPP

#include <spiration/api.h>

#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace spiration {
namespace capi {

/** @brief 拓展描述符别名。 */
using descriptor = spiration_extension_desc;

// ---------------------------------------------------------------------------
// 字符串
// ---------------------------------------------------------------------------

/**
 * @brief 由 `std::string` 取得字符串视图。
 * @warning 视图借用入参内存，入参必须比视图活得久。
 */
SPIRATION_INLINE spiration_str_view view(const std::string& text) {
    spiration_str_view out;
    out.data = text.empty() ? NULL : text.data();
    out.length = text.size();
    return out;
}

/** @brief 由 C 字符串取得字符串视图。 */
SPIRATION_INLINE spiration_str_view view(const char* text) {
    return spiration_str_view_cstr(text);
}

/** @brief 由字符串视图拷贝出 `std::string`。 */
SPIRATION_INLINE std::string str(spiration_str_view text) {
    return text.data ? std::string(text.data, text.length) : std::string();
}

/** @brief 由字符串视图拷贝出 `std::string`（空指针安全）。 */
SPIRATION_INLINE std::string str(const char* text) {
    return text ? std::string(text) : std::string();
}

// ---------------------------------------------------------------------------
// 内存
// ---------------------------------------------------------------------------

/** @brief 释放由宿主分配的内存。 */
SPIRATION_INLINE void dealloc(const descriptor* self, void* block) {
    if (self && self->host && self->host->dealloc) self->host->dealloc(self, block);
}

// ---------------------------------------------------------------------------
// 诊断
// ---------------------------------------------------------------------------

/** @brief 记录日志。 */
SPIRATION_INLINE void log(const descriptor* self, spiration_log_level level,
                          const std::string& message,
                          const std::string& tag = std::string()) {
    if (!self || !self->host || !self->host->log) return;
    self->host->log(self, level, view(message), view(tag));
}

/** @brief 记录追溯日志。 */
SPIRATION_INLINE void log_trace(const descriptor* self, const std::string& message,
                                const std::string& tag = std::string()) {
    log(self, SPIRATION_LOG_TRACE, message, tag);
}

/** @brief 记录调试日志。 */
SPIRATION_INLINE void log_debug(const descriptor* self, const std::string& message,
                                const std::string& tag = std::string()) {
    log(self, SPIRATION_LOG_DEBUG, message, tag);
}

/** @brief 记录信息日志。 */
SPIRATION_INLINE void log_info(const descriptor* self, const std::string& message,
                               const std::string& tag = std::string()) {
    log(self, SPIRATION_LOG_INFO, message, tag);
}

/** @brief 记录警告日志。 */
SPIRATION_INLINE void log_warning(const descriptor* self,
                                  const std::string& message,
                                  const std::string& tag = std::string()) {
    log(self, SPIRATION_LOG_WARNING, message, tag);
}

/** @brief 记录错误日志。 */
SPIRATION_INLINE void log_error(const descriptor* self, const std::string& message,
                                const std::string& tag = std::string()) {
    log(self, SPIRATION_LOG_ERROR, message, tag);
}

/** @brief 取最近一次错误文本。 */
SPIRATION_INLINE std::string last_error(const descriptor* self) {
    if (!self || !self->host || !self->host->last_error) return std::string();
    return str(self->host->last_error(self));
}

// ---------------------------------------------------------------------------
// 路径
// ---------------------------------------------------------------------------

/** @brief 拓展安装目录下的绝对路径。 */
SPIRATION_INLINE std::string extension_path(const descriptor* self,
                                            const std::string& relative = std::string()) {
    if (!self || !self->host || !self->host->extension_path) return std::string();
    return str(self->host->extension_path(self, view(relative)));
}

/** @brief 拓展私有数据目录下的绝对路径。 */
SPIRATION_INLINE std::string data_path(const descriptor* self,
                                       const std::string& relative = std::string()) {
    if (!self || !self->host || !self->host->data_path) return std::string();
    return str(self->host->data_path(self, view(relative)));
}

// ---------------------------------------------------------------------------
// 国际化
// ---------------------------------------------------------------------------

/** @brief 翻译文本。 */
SPIRATION_INLINE std::string tr(const descriptor* self, const std::string& key) {
    if (!self || !self->host || !self->host->translate) return key;
    return str(self->host->translate(self, view(key), NULL, 0));
}

/** @brief 带参数翻译文本。 */
SPIRATION_INLINE std::string tr(const descriptor* self, const std::string& key,
                                const std::vector<std::string>& args) {
    if (!self || !self->host || !self->host->translate) return key;
    std::vector<spiration_str_view> views;
    views.reserve(args.size());
    for (const std::string& arg : args) views.push_back(view(arg));
    return str(self->host->translate(
        self, view(key), views.empty() ? NULL : views.data(), views.size()));
}

/** @brief 载入翻译文件。 */
SPIRATION_INLINE bool load_translations(const descriptor* self,
                                        const std::string& path) {
    if (!self || !self->host || !self->host->load_translations)
        return false;
    return self->host->load_translations(self, view(path)) == SPIRATION_OK;
}

// ---------------------------------------------------------------------------
// 事件
// ---------------------------------------------------------------------------

/**
 * @brief 事件订阅句柄，析构时自动退订。
 */
class subscription {
public:
    subscription() = default;

    subscription(const descriptor* self, int64_t id) : self_(self), id_(id) {}

    subscription(subscription&& other) noexcept
        : self_(other.self_), id_(other.id_) {
        other.self_ = NULL;
        other.id_ = 0;
    }

    subscription& operator=(subscription&& other) noexcept {
        if (this != &other) {
            release();
            self_ = other.self_;
            id_ = other.id_;
            other.self_ = NULL;
            other.id_ = 0;
        }
        return *this;
    }

    subscription(const subscription&) = delete;
    subscription& operator=(const subscription&) = delete;

    ~subscription() { release(); }

    /** @brief 订阅是否有效。 */
    bool valid() const { return id_ != 0; }

    /** @brief 主动退订。 */
    void release() {
        if (self_ && id_ != 0 && self_->host && self_->host->unsubscribe) {
            self_->host->unsubscribe(self_, id_);
        }
        self_ = NULL;
        id_ = 0;
    }

private:
    const descriptor* self_ = NULL;
    int64_t id_ = 0;
};

namespace detail {

/** @brief `std::function` 与 C 回调之间的转发载体。 */
struct event_forwarder {
    std::function<void(const std::string&, const std::string&)> callback;
};

/** @brief C 回调实现。 */
SPIRATION_INLINE void SPIRATION_CALL event_trampoline(
    void* user_data, spiration_str_view event, spiration_str_view data) {
    event_forwarder* forwarder = static_cast<event_forwarder*>(user_data);
    if (forwarder && forwarder->callback) {
        forwarder->callback(str(event), str(data));
    }
}

} // namespace detail

/**
 * @brief 订阅事件。
 * @param self     拓展描述符。
 * @param event    事件名。
 * @param callback 回调，签名为 `void(const std::string& event, const std::string& data)`。
 * @return 订阅句柄；失败时 `valid()` 为 `false`。
 */
SPIRATION_INLINE subscription subscribe(
    const descriptor* self, const std::string& event,
    std::function<void(const std::string&, const std::string&)> callback) {
    if (!self || !self->host || !self->host->subscribe) return subscription();
    detail::event_forwarder* forwarder = new detail::event_forwarder();
    forwarder->callback = std::move(callback);
    int64_t id = self->host->subscribe(self, view(event),
                                       detail::event_trampoline, forwarder);
    if (id == 0) {
        delete forwarder;
        return subscription();
    }
    return subscription(self, id);
}

/** @brief 发布事件。 */
SPIRATION_INLINE bool publish(const descriptor* self, const std::string& event,
                              const std::string& data = std::string()) {
    if (!self || !self->host || !self->host->publish) return false;
    return self->host->publish(self, view(event), view(data)) == SPIRATION_OK;
}

// ---------------------------------------------------------------------------
// 服务与接口
// ---------------------------------------------------------------------------

/** @brief 注册命名服务。 */
SPIRATION_INLINE bool register_service(const descriptor* self,
                                       const std::string& name, void* service) {
    if (!self || !self->host || !self->host->register_service) return false;
    return self->host->register_service(self, view(name), service) == SPIRATION_OK;
}

/** @brief 查询命名服务。 */
SPIRATION_INLINE void* query_service(const descriptor* self,
                                     const std::string& extension_id,
                                     const std::string& name) {
    if (!self || !self->host || !self->host->query_service) return NULL;
    return self->host->query_service(self, view(extension_id), view(name));
}

/** @brief 查询宿主可选接口并转换为指定类型。 */
template <typename Interface>
SPIRATION_INLINE const Interface* query_interface(const descriptor* self,
                                                  const std::string& name,
                                                  uint32_t required_version) {
    if (!self || !self->host || !self->host->query_interface) return NULL;
    return static_cast<const Interface*>(
        self->host->query_interface(self, view(name), required_version));
}

/** @brief 在宿主主线程上执行回调。 */
SPIRATION_INLINE bool dispatch(const descriptor* self,
                               spiration_callback callback, void* user_data) {
    if (!self || !self->host || !self->host->dispatch) return false;
    return self->host->dispatch(self, callback, user_data) == SPIRATION_OK;
}

} // namespace capi
} // namespace spiration

#endif /* SPIRATION_API_HPP */
