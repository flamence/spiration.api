/**
 * @file host.h
 * @brief 宿主函数表 —— 拓展调用宿主能力的唯一入口。
 * @author 陈林锴
 *
 * @par 使用方式
 * 拓展在 `on_initialize` 中通过 `self->host` 取得函数表并长期保存：
 * @code
 * static const spiration_host_api* g_host = NULL;
 *
 * static spiration_bool SPIRATION_CALL app_on_initialize(spiration_extension_desc* self) {
 *     g_host = self->host;
 *     g_host->log(self, SPIRATION_LOG_INFO, SPIRATION_STR("hello"), SPIRATION_STR_EMPTY);
 *     return SPIRATION_TRUE;
 * }
 * @endcode
 *
 * @par 版本协商
 * 宿主函数表的首两个字段为 `version`（`SPIRATION_API_VERSION`）与 `size`
 * （宿主侧结构体字节数）。拓展必须：
 * 1. 以 `SPIRATION_API_VERSION_COMPATIBLE(host->version, 所需版本)` 判断兼容性；
 * 2. 只调用下标小于 `size` 的函数（即只调用自身 `sizeof` 与 `host->size`
 *    共同覆盖的字段）；
 * 3. 不读取 `reserved` 数组。
 *
 * @par 函数表扩张规则
 * 新函数一律追加在 `reserved` 之前，并同步缩小 `reserved`，由此保证既有
 * 函数的偏移量永久不变。
 *
 * @par 字符串生命周期
 * 宿主返回的 `spiration_str_view` 均指向 **宿主持有的结果区**：
 * - 该结果区在线程内按调用批次累积；
 * - 一次拓展回调（`on_initialize` / `on_event` 等）返回时，宿主自动清空结果区；
 * - 拓展亦可通过 `reset_results` 主动清空；
 * - 拓展卸载后全部失效。
 * 因此拓展若需跨回调长期持有文本，**必须自行拷贝**。
 *
 * @par 线程模型
 * 除 `dispatch` 外，所有宿主函数都只能在宿主主线程（UI 线程）调用。
 * 可用 `is_host_thread` 判定；在其他线程取得的回调应先 `dispatch` 回主线程。
 *
 * @par 错误处理
 * 返回 `spiration_status` 的函数以状态码报告错误；返回指针或其他类型的函数
 * 以 `NULL` / `0` / `SPIRATION_FALSE` 表示失败。最近一次失败的细节可通过
 * `last_error` 获取。宿主不会让异常穿过 ABI 边界。
 */

#ifndef SPIRATION_API_HOST_H
#define SPIRATION_API_HOST_H

#include <spiration/api/base.h>

SPIRATION_EXTERN_C_BEGIN

/**
 * @brief 宿主函数表。
 * @note 类型别名 `spiration_host_api` 由 `spiration/api/base.h` 前置声明。
 */
struct spiration_host_api {
    /** 宿主 C API 版本，编码值，见 `SPIRATION_API_VERSION`。 */
    uint32_t version;
    /** 宿主侧结构体字节数，可用于字段可用性判断。 */
    uint32_t size;

    /* ---------------------------------------------------------------------
     * 一、内存（1 - 3）
     * ------------------------------------------------------------------ */

    /**
     * @brief 从宿主堆分配内存。
     * @param self 拓展描述符。
     * @param size 字节数。
     * @return 内存指针；失败返回 `NULL`。
     * @note 该内存必须由 `dealloc` 释放。
     */
    void*(SPIRATION_CALL* alloc)(const spiration_extension_desc* self,
                                 size_t size);

    /**
     * @brief 调整 `alloc` 所分配内存的大小。
     * @param self 拓展描述符。
     * @param block 原内存指针，可为 `NULL`。
     * @param size 新字节数。
     * @return 内存指针；失败返回 `NULL`（原内存保持有效）。
     */
    void*(SPIRATION_CALL* realloc)(const spiration_extension_desc* self,
                                   void* block, size_t size);

    /**
     * @brief 释放 `alloc` / `realloc` 分配的内存。
     * @param self 拓展描述符。
     * @param block 内存指针，可为 `NULL`。
     */
    void(SPIRATION_CALL* dealloc)(const spiration_extension_desc* self,
                                  void* block);

    /* ---------------------------------------------------------------------
     * 二、诊断（4 - 6）
     * ------------------------------------------------------------------ */

    /**
     * @brief 记录一条日志。
     * @param self    拓展描述符。
     * @param level   日志等级。
     * @param message 日志正文，UTF-8。
     * @param tag     可选的附加标签，空串表示无标签。
     * @note 宿主会自动以 `extension/<短名>[/<tag>]` 作为日志频道。
     */
    void(SPIRATION_CALL* log)(const spiration_extension_desc* self,
                              spiration_log_level level,
                              spiration_str_view message,
                              spiration_str_view tag);

    /**
     * @brief 取最近一次失败的描述文本。
     * @param self 拓展描述符。
     * @return 错误文本，UTF-8；无错误时返回空串。
     */
    spiration_str_view(SPIRATION_CALL* last_error)(
        const spiration_extension_desc* self);

    /**
     * @brief 清空本线程的结果区，使此前取得的字符串视图全部失效。
     * @param self 拓展描述符。
     * @note 长循环中批量取用文本时应主动调用，避免结果区无界增长。
     */
    void(SPIRATION_CALL* reset_results)(const spiration_extension_desc* self);

    /* ---------------------------------------------------------------------
     * 三、线程（7 - 8）
     * ------------------------------------------------------------------ */

    /**
     * @brief 判断当前线程是否为宿主主线程。
     * @param self 拓展描述符。
     */
    spiration_bool(SPIRATION_CALL* is_host_thread)(
        const spiration_extension_desc* self);

    /**
     * @brief 将回调投递到宿主主线程执行。
     * @param self     拓展描述符。
     * @param callback 回调。
     * @param user_data 传给回调的用户数据。
     * @return 状态码。
     * @note 回调保证在 `on_shutdown` 之前或之后之一被调用；`on_shutdown`
     *       返回后尚未执行的回调会被丢弃。
     */
    spiration_status(SPIRATION_CALL* dispatch)(
        const spiration_extension_desc* self, spiration_callback callback,
        void* user_data);

    /* ---------------------------------------------------------------------
     * 四、路径（9 - 10）
     * ------------------------------------------------------------------ */

    /**
     * @brief 取得拓展安装目录下的路径。
     * @param self     拓展描述符。
     * @param relative 相对路径；空串表示拓展根目录本身。
     * @return 绝对路径，UTF-8；失败返回空串。
     * @note 返回路径仅做拼接与规范化，不保证目标存在。
     */
    spiration_str_view(SPIRATION_CALL* extension_path)(
        const spiration_extension_desc* self, spiration_str_view relative);

    /**
     * @brief 取得拓展私有数据目录下的路径（可读写、跨版本保留）。
     * @param self     拓展描述符。
     * @param relative 相对路径；空串表示数据目录本身。
     * @return 绝对路径，UTF-8；失败返回空串。
     */
    spiration_str_view(SPIRATION_CALL* data_path)(
        const spiration_extension_desc* self, spiration_str_view relative);

    /* ---------------------------------------------------------------------
     * 五、国际化（11 - 12）
     * ------------------------------------------------------------------ */

    /**
     * @brief 翻译文本。
     * @param self      拓展描述符。
     * @param key       翻译键。
     * @param args      参数数组，可为 `NULL`。
     * @param arg_count 参数个数。
     * @return 翻译结果；缺失键时返回 `key` 本身。
     */
    spiration_str_view(SPIRATION_CALL* translate)(
        const spiration_extension_desc* self, spiration_str_view key,
        const spiration_str_view* args, size_t arg_count);

    /**
     * @brief 载入本拓展的翻译文件（Properties 格式），键会以拓展 ID 为前缀。
     * @param self 拓展描述符。
     * @param path 语言文件路径，可为绝对路径；相对路径以拓展目录为基准。
     * @return 状态码。
     */
    spiration_status(SPIRATION_CALL* load_translations)(
        const spiration_extension_desc* self, spiration_str_view path);

    /* ---------------------------------------------------------------------
     * 六、事件（13 - 15）
     * ------------------------------------------------------------------ */

    /**
     * @brief 订阅事件。
     * @param self      拓展描述符。
     * @param event     事件名，如 `"tab:opened"`。
     * @param callback  回调。
     * @param user_data 传给回调的用户数据。
     * @return 订阅 ID，大于 0；失败返回 0。
     * @note 订阅者即本拓展，宿主在拓展卸载时自动退订。
     */
    int64_t(SPIRATION_CALL* subscribe)(const spiration_extension_desc* self,
                                       spiration_str_view event,
                                       spiration_event_callback callback,
                                       void* user_data);

    /**
     * @brief 取消订阅。
     * @param self         拓展描述符。
     * @param subscription `subscribe` 返回的 ID。
     */
    void(SPIRATION_CALL* unsubscribe)(const spiration_extension_desc* self,
                                      int64_t subscription);

    /**
     * @brief 发布事件，同步分发给所有订阅者。
     * @param self  拓展描述符。
     * @param event 事件名。
     * @param data  事件负载，UTF-8。
     * @return 状态码。
     */
    spiration_status(SPIRATION_CALL* publish)(
        const spiration_extension_desc* self, spiration_str_view event,
        spiration_str_view data);

    /* ---------------------------------------------------------------------
     * 七、拓展间服务（16 - 17）
     * ------------------------------------------------------------------ */

    /**
     * @brief 注册本拓展提供的命名服务。
     * @param self    拓展描述符。
     * @param name    服务名，建议使用 `"<拓展ID>.<服务>"` 形式。
     * @param service 服务实例指针，由拓展自行保证生命周期。
     * @return 状态码。
     * @note 服务在拓展卸载时被自动注销。
     */
    spiration_status(SPIRATION_CALL* register_service)(
        const spiration_extension_desc* self, spiration_str_view name,
        void* service);

    /**
     * @brief 查询其他拓展暴露的命名服务。
     * @param self         拓展描述符。
     * @param extension_id 目标拓展 ID。
     * @param name         服务名。
     * @return 服务指针；不存在返回 `NULL`。
     * @note 返回裸指针的语义由服务提供方约定，宿主不做任何类型检查。
     */
    void*(SPIRATION_CALL* query_service)(const spiration_extension_desc* self,
                                         spiration_str_view extension_id,
                                         spiration_str_view name);

    /* ---------------------------------------------------------------------
     * 八、拓展注册表（18 - 19）
     * ------------------------------------------------------------------ */

    /**
     * @brief 按 ID 查找已装载拓展的描述符。
     * @param self 拓展描述符。
     * @param id   目标拓展 ID。
     * @return 描述符指针；不存在返回 `NULL`。
     * @note 对于以原生 C++ API 实现的拓展，宿主会构造一个**垫片描述符**：
     *       身份字段有效，但回调为 `NULL`；此类描述符不可调用其回调。
     */
    spiration_extension_desc*(SPIRATION_CALL* find_extension)(
        const spiration_extension_desc* self, spiration_str_view id);

    /**
     * @brief 枚举所有已装载拓展。
     * @param self     拓展描述符。
     * @param out      输出数组，可为 `NULL`（此时仅查询数量）。
     * @param capacity `out` 的容量。
     * @return 拓展总数。
     */
    size_t(SPIRATION_CALL* enumerate_extensions)(
        const spiration_extension_desc* self,
        const spiration_extension_desc** out, size_t capacity);

    /* ---------------------------------------------------------------------
     * 九、可选接口（20）
     * ------------------------------------------------------------------ */

    /**
     * @brief 查询宿主可选接口。
     * @param self            拓展描述符。
     * @param name            接口名，见 `spiration/api/ui.h` 等。
     * @param required_version 所需的最低接口版本（编码值）。
     * @return 函数表指针；不支持时返回 `NULL`。
     *
     * @note 该函数是 C API 的“横向”扩展点：界面、智能体、网络等体积较大的
     *       能力均以独立接口表的形式发布，避免核心函数表无限膨胀。
     *       约定接口表同样以 `{ uint32_t version; uint32_t size; ... }` 开头。
     */
    const void*(SPIRATION_CALL* query_interface)(
        const spiration_extension_desc* self, spiration_str_view name,
        uint32_t required_version);

    /* ---------------------------------------------------------------------
     * 十、加载器（21 - 22）
     * ------------------------------------------------------------------ */

    /**
     * @brief 以 **加载器** 身份注册自身，用于装载脚本类拓展。
     * @param self   拓展描述符。
     * @param loader 加载器函数表，见 `spiration/api/loader.h`。
     * @return 状态码。
     * @note 加载器应在 `SPIRATION_PHASE_EARLY` 初始化，早于其托管的拓展。
     */
    spiration_status(SPIRATION_CALL* register_loader)(
        const spiration_extension_desc* self,
        const spiration_loader_api* loader);

    /**
     * @brief 查询已注册的加载器。
     * @param self      拓展描述符。
     * @param loader_id 加载器拓展 ID。
     * @return 函数表指针；不存在返回 `NULL`。
     */
    const spiration_loader_api*(SPIRATION_CALL* query_loader)(
        const spiration_extension_desc* self, spiration_str_view loader_id);

    /** 预留槽位，必须全部为 `NULL`。 */
    void* reserved[16];
};

/**
 * @brief 判断宿主函数表是否包含指定字段。
 * @param host  宿主函数表指针。
 * @param field 字段名，如 `query_interface`。
 * @return 该字段的开头是否落在宿主已知的 `size` 范围内。
 */
#define SPIRATION_HOST_HAS(host, field)                                     \
    ((host) != NULL &&                                                      \
     ((host)->size >=                                                       \
      (uint32_t)(offsetof(struct spiration_host_api, field) +               \
                 sizeof((host)->field))))

SPIRATION_EXTERN_C_END

#endif /* SPIRATION_API_HOST_H */
