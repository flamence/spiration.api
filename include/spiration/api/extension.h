/**
 * @file extension.h
 * @brief 拓展描述符与入口符号约定。
 * @author 陈林锴
 *
 * @par 生命周期
 * 拓展动态库必须导出符号 `spiration_extension_entry`（见
 * `SPIRATION_EXTENSION_ENTRY_SYMBOL`）。宿主装载动态库后：
 *
 * 1. 查找符号并以 `(host, manifest_json)` 调用之；
 * 2. 校验返回的描述符（`version` / `size` / `id` / `api_version_required`）；
 * 3. 写入 `desc->host`；
 * 4. 在合适阶段调用 `desc->on_initialize(self)`；
 * 5. 卸载前调用 `desc->on_shutdown(self)`，此后不再触碰该描述符。
 *
 * @par 描述符内存
 * 描述符由拓展自身持有（静态存储或由加载器动态分配），其内容与
 * `id` / `name` / `version` / `description` 所指字符串必须在
 * `on_shutdown` 返回之后仍然有效，直至动态库被卸载为止。
 */

#ifndef SPIRATION_API_EXTENSION_H
#define SPIRATION_API_EXTENSION_H

#include <spiration/api/base.h>

SPIRATION_EXTERN_C_BEGIN

/**
 * @brief 拓展入口符号名。
 */
#define SPIRATION_EXTENSION_ENTRY_SYMBOL "spiration_extension_entry"

/**
 * @brief 初始化阶段，与宿主 `spiration::init_phase` 一一对应。
 */
typedef enum spiration_init_phase {
    SPIRATION_PHASE_EARLY = 0,  /**< 早期：窗口与控件尚未创建。 */
    SPIRATION_PHASE_NORMAL = 1, /**< 中期：窗口与控件创建中，窗口显示前。 */
    SPIRATION_PHASE_LATE = 2    /**< 晚期：窗口已显示。 */
} spiration_init_phase;

/**
 * @brief 拓展描述符。
 *
 * @par 字段归属
 * - **[拓展]**：`version`、`size`、`phase`、`api_version_required`、身份四元组、
 *   `user_data` 与全部回调，须在 `spiration_extension_entry` 返回前填好。
 * - **[宿主]**：`host`，由宿主在调用 `on_initialize` 之前写入。
 *
 * @note 类型别名 `spiration_extension_desc` 由 `spiration/api/base.h` 前置声明。
 */
struct spiration_extension_desc {
    /** [拓展] 结构体版本，取 `SPIRATION_STRUCT_VERSION`。 */
    uint32_t version;
    /** [拓展] 结构体字节数，取 `sizeof(spiration_extension_desc)`。 */
    uint32_t size;

    /** [拓展] 初始化阶段。 */
    spiration_init_phase phase;
    /** [拓展] 所需的最低 C API 版本，取 `SPIRATION_API_VERSION` 或更低值。 */
    uint32_t api_version_required;

    /** [拓展] 唯一标识，形如 `com.example.hello-world`。必须非空。 */
    spiration_str_view id;
    /** [拓展] 显示名称，可为空（此时回退到 `id`）。可以是翻译键。 */
    spiration_str_view name;
    /** [拓展] 语义化版本号字符串，如 `1.2.3`。可为空。 */
    spiration_str_view version_string;
    /** [拓展] 描述文本，可为空。可以是翻译键。 */
    spiration_str_view description;

    /** [宿主] 宿主函数表，仅在 `on_initialize` 及其后有效。 */
    const spiration_host_api* host;
    /** [拓展] 供拓展自由使用的上下文指针，宿主不解释其含义。 */
    void* user_data;

    /**
     * [拓展] 初始化回调。
     * @param self 本描述符。
     * @return 成功返回 `SPIRATION_TRUE`；返回 `SPIRATION_FALSE` 则宿主放弃装载。
     */
    spiration_bool(SPIRATION_CALL* on_initialize)(
        spiration_extension_desc* self);

    /**
     * [拓展] 关闭回调。
     * @param self 本描述符。
     * @note 返回后拓展不得再调用任何宿主函数；订阅、服务与句柄均已失效。
     */
    void(SPIRATION_CALL* on_shutdown)(spiration_extension_desc* self);

    /**
     * [拓展] 事件回调，用于接收通过 `host->subscribe` 订阅的事件。
     * @param self 本描述符。
     * @param event 事件名。
     * @param data 事件负载。
     */
    void(SPIRATION_CALL* on_event)(spiration_extension_desc* self,
                                   spiration_str_view event,
                                   spiration_str_view data);

    /** 预留槽位，必须全部为 `NULL`。 */
    void* reserved[8];
};

/**
 * @brief 拓展入口函数原型。
 * @param host          宿主函数表。
 * @param manifest_json 拓展清单 `extension.json` 的原始文本（UTF-8）。
 * @return 描述符指针；返回 `NULL` 表示拒绝装载。
 *
 * @note 入口函数**不得**做任何重量级工作，仅构造并返回描述符即可，
 *       真正的初始化应放在 `on_initialize` 中。
 */
typedef const spiration_extension_desc*(SPIRATION_CALL*
                                            spiration_extension_entry_fn)(
    const spiration_host_api* host, spiration_str_view manifest_json);

/**
 * @brief 构造一个零值描述符，并填好 `version` 与 `size`。
 * @note C++14 不支持指定初始化器，故提供该辅助函数。
 */
SPIRATION_INLINE spiration_extension_desc spiration_extension_desc_default(
    void) {
    spiration_extension_desc desc;
    memset(&desc, 0, sizeof(desc));
    desc.version = SPIRATION_STRUCT_VERSION;
    desc.size = (uint32_t)sizeof(spiration_extension_desc);
    desc.phase = SPIRATION_PHASE_NORMAL;
    desc.api_version_required = SPIRATION_API_VERSION;
    return desc;
}

/**
 * @brief 描述符的零值初始化器，可直接用于静态描述符。
 * @code
 * static spiration_extension_desc g_desc = SPIRATION_EXTENSION_DESC_DEFAULT;
 * @endcode
 * @note 使用后仍需填写 `id` 与回调等字段。
 */
#define SPIRATION_EXTENSION_DESC_DEFAULT                                    \
    {SPIRATION_STRUCT_VERSION, (uint32_t)sizeof(spiration_extension_desc),  \
     SPIRATION_PHASE_NORMAL, SPIRATION_API_VERSION,                         \
     {NULL, 0u}, {NULL, 0u}, {NULL, 0u}, {NULL, 0u},                        \
     NULL, NULL, NULL, NULL, NULL, {NULL}}

/**
 * @brief 完善描述符：补齐 `version` / `size` / `host`。
 * @param desc          拓展自行持有（静态或由加载器分配）的描述符。
 * @param host          宿主函数表。
 * @param manifest_json 拓展清单文本（可忽略）。
 * @return `desc`；`desc` 为 `NULL` 时返回 `NULL`。
 */
SPIRATION_INLINE const spiration_extension_desc* spiration_extension_prepare(
    spiration_extension_desc* desc, const spiration_host_api* host,
    spiration_str_view manifest_json) {
    SPIRATION_UNUSED(manifest_json);
    if (desc == NULL) return NULL;
    if (desc->version == 0u) desc->version = SPIRATION_STRUCT_VERSION;
    desc->size = (uint32_t)sizeof(spiration_extension_desc);
    desc->host = host;
    return desc;
}

/**
 * @brief 定义拓展入口符号。
 * @param descriptor_expr 求值为 `spiration_extension_desc*` 的表达式。
 *
 * @code
 * static spiration_extension_desc g_desc = {
 *     SPIRATION_STRUCT_VERSION, sizeof(spiration_extension_desc),
 *     SPIRATION_PHASE_NORMAL, SPIRATION_API_VERSION,
 *     SPIRATION_STR_INIT("com.example.hello-world"),
 *     SPIRATION_STR_INIT("Hello World"),
 *     SPIRATION_STR_INIT("0.1.0"),
 *     SPIRATION_STR_INIT(""),
 *     NULL, NULL,
 *     on_initialize, on_shutdown, NULL,
 *     {NULL}
 * };
 * SPIRATION_DEFINE_EXTENSION_ENTRY(&g_desc);
 * @endcode
 *
 * @note 入口以 **C 链接** 导出，符号名即 `spiration_extension_entry`，
 *       与编译语言（C 或 C++）无关。
 */
#define SPIRATION_DEFINE_EXTENSION_ENTRY(descriptor_expr)                  \
    SPIRATION_EXTERN_C_BEGIN                                               \
    SPIRATION_ENTRY const spiration_extension_desc* SPIRATION_CALL         \
    spiration_extension_entry(const spiration_host_api* spiration_host,    \
                              spiration_str_view spiration_manifest) {     \
        return spiration_extension_prepare((descriptor_expr),              \
                                           spiration_host,                 \
                                           spiration_manifest);            \
    }                                                                      \
    SPIRATION_EXTERN_C_END

/**
 * @brief 自定义入口钩子：在返回描述符之前做装载期工作。
 * @param host          宿主函数表。
 * @param manifest_json 拓展清单原文。
 * @return 拓展描述符。
 */
typedef const spiration_extension_desc*(SPIRATION_CALL* spiration_entry_hook)(
    const spiration_host_api* host, spiration_str_view manifest_json);

/**
 * @brief 以自定义钩子定义拓展入口符号。
 *
 * 适用于需要在**装载期**（而非初始化期）与宿主交互的拓展，最典型的是
 * 加载器拓展 —— 它必须在被托管的拓展被装载之前注册自己：
 *
 * @code
 * static const spiration_extension_desc* SPIRATION_CALL my_hook(
 *     const spiration_host_api* host, spiration_str_view manifest_json) {
 *     host->register_loader(&g_desc, &g_loader);
 *     g_desc.host = host;
 *     return &g_desc;
 * }
 * SPIRATION_DEFINE_EXTENSION_ENTRY_HOOKED(&my_hook);
 * @endcode
 */
#define SPIRATION_DEFINE_EXTENSION_ENTRY_HOOKED(hook)                       \
    SPIRATION_EXTERN_C_BEGIN                                                \
    SPIRATION_ENTRY const spiration_extension_desc* SPIRATION_CALL          \
    spiration_extension_entry(const spiration_host_api* spiration_host,     \
                              spiration_str_view spiration_manifest) {      \
        return (hook)(spiration_host, spiration_manifest);                  \
    }                                                                       \
    SPIRATION_EXTERN_C_END

/**
 * @brief 取描述符的宿主函数表。
 */
SPIRATION_INLINE const spiration_host_api* spiration_extension_host(
    const spiration_extension_desc* self) {
    return self ? self->host : NULL;
}

SPIRATION_EXTERN_C_END

#endif /* SPIRATION_API_EXTENSION_H */
