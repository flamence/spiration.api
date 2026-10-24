/**
 * @file agent.h
 * @brief 可选接口：智能体（向内置智能体注册对话补全 provider）。
 * @author 陈林锴
 *
 * @par 获取方式
 * @code
 * const spiration_agent_api* agent =
 *     (const spiration_agent_api*)host->query_interface(
 *         self, SPIRATION_STR(SPIRATION_AGENT_INTERFACE_NAME),
 *         SPIRATION_AGENT_VERSION);
 * if (agent == NULL) { 宿主不支持智能体能力 }
 * @endcode
 *
 * @par 为什么需要该接口
 * 智能体内置拓展把“对话补全”抽象为 provider：一次请求 → 一次响应，流式输出
 * 经回调增量下发。宿主不限定 provider 的实现语言与传输方式（HTTP、进程内推理…），
 * 但跨 C ABI 的 provider 无法传递 C++ 对象，因此双方以 **JSON** 交换结构化的
 * 请求与响应，思路与加载器一致：
 *
 * ```
 * 宿主（agent 内置拓展）  ── request JSON ──▶  provider（拓展实现）
 *                        ◀── response JSON ──
 * ```
 *
 * @par 注册与生命周期
 * - provider 由拓展在 `on_initialize` 阶段注册，名称全局唯一；
 * - `provider`（vtable）与 `user_data` 由拓展持有，必须在整个注册期间保持有效；
 * - 注销（`unregister_provider`）返回后，宿主保证不再访问 vtable 与 `user_data`；
 *   已经创建的 provider 实例在注销后不再执行，并返回空响应；
 * - 宿主在拓展卸载时会自动注销该拓展遗留的 provider，但**不会**释放
 *   `user_data`，因此脚本类拓展应在关闭回调中主动注销并释放自己的状态。
 *
 * @warning 注销可能阻塞等待正在进行的 `execute` 结束。因此从解释器线程
 *          （如 Python / Node.js 脚本）注销时必须先释放解释器锁 / 事件循环，
 *          否则会与正在执行的 provider 相互等待。
 *
 * @par 线程模型
 * - `register_provider` / `unregister_provider` 在宿主主线程（UI 线程）调用；
 * - `execute` 在宿主的工作线程调用，**不是** UI 线程；provider 内若需操作
 *   宿主界面，必须按 C API 的线程约定 `host->dispatch` 回主线程；
 * - `on_delta` / `on_reasoning_delta` / `should_stop` 仅在对应的 `execute`
 *   调用期间有效，且不得跨线程使用。
 *
 * @par 请求 JSON
 * @code{.json}
 * {
 *   "model": "gpt-4o-mini",
 *   "endpoint": "https://api.openai.com/v1",   // 传输端点（网络 provider 使用）
 *   "api_key": "sk-...",                        // 鉴权密钥（网络 provider 使用）
 *   "timeout_seconds": 120,
 *   "max_tokens": 0,                            // 0 表示不指定
 *   "temperature": 0.7,
 *   "stream": true,
 *   "reasoning": "medium",                      // none|low|medium|high|xhigh|max
 *   "messages": [
 *     {
 *       "role": "system|user|assistant|tool",
 *       "content": "文本或 null",
 *       "reasoning_content": "思考过程或 null",
 *       "tool_call_id": "工具调用 ID 或 null",
 *       "name": "工具名或 null",
 *       "tool_calls": [
 *         {"id": "call_1", "function_name": "read_file", "arguments": "{\"path\":\"a\"}"}
 *       ]
 *     }
 *   ],
 *   "tools": [
 *     {
 *       "name": "read_file",
 *       "description": "读取文件",
 *       "parameters": {"type": "object", "properties": {}}
 *     }
 *   ]
 * }
 * @endcode
 *
 * @par 响应 JSON
 * @code{.json}
 * {
 *   "content": "最终正文或 null",
 *   "reasoning_content": "完整思考过程或 null",
 *   "tool_calls": [
 *     {"id": "call_1", "function_name": "read_file", "arguments": "{\"path\":\"a\"}"}
 *   ],
 *   "finish_reason": "stop",
 *   "prompt_tokens": 123,
 *   "completion_tokens": 45
 * }
 * @endcode
 *
 * 流式 provider 在 `execute` 期间通过 `on_delta` / `on_reasoning_delta` 上报增量，
 * 同时在返回的响应中给出**完整累积**的内容：宿主以此写入对话历史。
 */

#ifndef SPIRATION_API_AGENT_H
#define SPIRATION_API_AGENT_H

#include <spiration/api/base.h>

SPIRATION_EXTERN_C_BEGIN

/**
 * @brief 智能体接口名，传给 `host->query_interface`。
 */
#define SPIRATION_AGENT_INTERFACE_NAME "spiration.agent"

/**
 * @brief 智能体接口版本。
 */
#define SPIRATION_AGENT_VERSION \
    SPIRATION_API_VERSION_ENCODE(1, 0, 0)

/**
 * @brief 思考挡位，与宿主 `spiration::agent::reasoning_level` 一一对应。
 * @note 取值顺序即 UI 显示顺序；新增挡位只能追加。
 */
typedef enum spiration_reasoning_level {
    SPIRATION_REASONING_NONE = 0,   /**< 不思考。 */
    SPIRATION_REASONING_LOW = 1,    /**< 低。 */
    SPIRATION_REASONING_MEDIUM = 2, /**< 标准。 */
    SPIRATION_REASONING_HIGH = 3,   /**< 高。 */
    SPIRATION_REASONING_XHIGH = 4,  /**< 超高。 */
    SPIRATION_REASONING_MAX = 5     /**< 最大。 */
} spiration_reasoning_level;

/**
 * @brief 内容增量回调。
 * @param user_data `spiration_agent_stream::user_data`。
 * @param delta     增量文本（UTF-8，宿主持有，回调返回后失效）。
 */
typedef void(SPIRATION_CALL* spiration_agent_delta_fn)(void* user_data,
                                                       spiration_str_view delta);

/**
 * @brief 中止检查回调。
 * @param user_data `spiration_agent_stream::user_data`。
 * @return 用户已请求中止时返回 `SPIRATION_TRUE`。
 */
typedef spiration_bool(SPIRATION_CALL* spiration_agent_stop_fn)(void* user_data);

/**
 * @brief 一次 `execute` 期间可用的流式回调集合。
 * @note 各回调均可为 `NULL`（表示宿主不关心对应事件）；结构体本身仅在
 *       对应的 `execute` 调用期间有效。
 */
typedef struct spiration_agent_stream {
    /** 正文增量回调，可为 `NULL`。 */
    spiration_agent_delta_fn on_delta;
    /** 思考过程增量回调，可为 `NULL`。 */
    spiration_agent_delta_fn on_reasoning_delta;
    /** 中止检查回调，可为 `NULL`。 */
    spiration_agent_stop_fn should_stop;
    /** 传给上述回调的用户数据。 */
    void* user_data;
} spiration_agent_stream;

/**
 * @brief 查询 provider 支持的思考挡位。
 * @param user_data 注册时传入的 `user_data`。
 * @param levels    输出数组，可为 `NULL`（此时只查询数量）。
 * @param capacity  `levels` 的容量。
 * @return 支持的挡位总数；`levels` 非空时写入前 `min(总数, capacity)` 个。
 * @note 宿主注册时调用一次并缓存结果，调用发生在宿主主线程。
 */
typedef size_t(SPIRATION_CALL* spiration_agent_levels_fn)(
    void* user_data, spiration_reasoning_level* levels, size_t capacity);

/**
 * @brief provider 函数表（由拓展实现）。
 *
 * @note 表首两个字段与所有接口表一致：`version`（`SPIRATION_STRUCT_VERSION`）
 *       与 `size`（本结构体字节数），用于追加式演进。
 */
typedef struct spiration_agent_provider {
    /** 结构体版本，取 `SPIRATION_STRUCT_VERSION`。 */
    uint32_t version;
    /** 本结构体字节数。 */
    uint32_t size;

    /**
     * @brief 查询支持的思考挡位，可为 `NULL`（表示支持全部挡位）。
     */
    spiration_agent_levels_fn supported_reasoning_levels;

    /**
     * @brief 执行一次补全。
     * @param user_data 注册时传入的 `user_data`。
     * @param request   请求 JSON，UTF-8。
     * @param stream    流式回调集合（不为 `NULL`，但成员可为 `NULL`）。
     * @param response  输出响应 JSON；由 provider 持有，宿主在本次调用返回后
     *                  立即消费，因此无需跨调用保持有效。
     * @return 状态码；非 `SPIRATION_OK` 时宿主按失败处理。
     */
    spiration_status(SPIRATION_CALL* execute)(
        void* user_data, spiration_str_view request,
        const spiration_agent_stream* stream, spiration_str_view* response);

    /** 预留槽位，必须全部为 `NULL`。 */
    void* reserved[4];
} spiration_agent_provider;

/**
 * @brief 智能体函数表。
 */
typedef struct spiration_agent_api {
    /** 智能体接口版本，取 `SPIRATION_AGENT_VERSION`。 */
    uint32_t version;
    /** 本结构体字节数。 */
    uint32_t size;

    /**
     * @brief 注册 provider。
     * @param self     拓展描述符（用于归属与卸载清理）。
     * @param name     provider 名称，如 `"openai"`；全局唯一，空名非法。
     * @param provider provider 函数表；vtable 与 `user_data` 由拓展持有。
     * @param user_data 传给 provider 回调的用户数据。
     * @return 成功返回 `SPIRATION_OK`；名称已被占用返回
     *         `SPIRATION_ERR_ALREADY_EXISTS`；参数非法返回
     *         `SPIRATION_ERR_INVALID_ARGUMENT`。
     * @note 必须在拓展的 `on_initialize` 阶段调用。
     */
    spiration_status(SPIRATION_CALL* register_provider)(
        const spiration_extension_desc* self, spiration_str_view name,
        const spiration_agent_provider* provider, void* user_data);

    /**
     * @brief 注销 provider。
     * @param self 拓展描述符。
     * @param name provider 名称。
     * @return 成功或该名称不存在时返回 `SPIRATION_OK`（幂等）。
     * @note 返回后宿主保证不再访问对应 vtable 与 `user_data`；若正在执行
     *       `execute`，本函数会等待其结束再返回（见文件头的线程模型说明）。
     */
    spiration_status(SPIRATION_CALL* unregister_provider)(
        const spiration_extension_desc* self, spiration_str_view name);

    /** 预留槽位，必须全部为 `NULL`。 */
    void* reserved[8];
} spiration_agent_api;

SPIRATION_EXTERN_C_END

#endif /* SPIRATION_API_AGENT_H */
