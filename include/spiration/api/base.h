/**
 * @file base.h
 * @brief Spiration C API 基础数据类型。
 * @author 陈林锴
 *
 * @note 本文件中所有类型均为 **平凡可复制** 的 POD 数据类型，不携带 C++ 语义，
 *       可被任意语言（C、C++、C#、Node.js、Python 等）按内存布局直接读写。
 */

#ifndef SPIRATION_API_BASE_H
#define SPIRATION_API_BASE_H

#include <spiration/api/version.h>
#include <spiration/export.h>

#include <stddef.h>
#include <stdint.h>
#include <string.h>

SPIRATION_EXTERN_C_BEGIN

/* -------------------------------------------------------------------------
 * 前置声明（不透明句柄）
 * ---------------------------------------------------------------------- */

/** @brief 宿主函数表，见 `spiration/api/host.h`。 */
typedef struct spiration_host_api spiration_host_api;
/** @brief 拓展加载器函数表，见 `spiration/api/loader.h`。 */
typedef struct spiration_loader_api spiration_loader_api;
/** @brief 拓展描述符，见 `spiration/api/extension.h`。 */
typedef struct spiration_extension_desc spiration_extension_desc;
/** @brief 可选的界面函数表，见 `spiration/api/ui.h`。 */
typedef struct spiration_ui_api spiration_ui_api;
/** @brief 标签页句柄（不透明）。 */
typedef struct spiration_tab spiration_tab;
/** @brief 控件句柄（不透明）。 */
typedef struct spiration_widget spiration_widget;
/** @brief 窗口句柄（不透明）。 */
typedef struct spiration_window spiration_window;
/** @brief 渲染器句柄（不透明）。 */
typedef struct spiration_renderer spiration_renderer;

/* -------------------------------------------------------------------------
 * 标量
 * ---------------------------------------------------------------------- */

/**
 * @brief 跨边界布尔值。
 * @note 固定为 1 字节，避免不同语言 / 编译器对 `bool` 宽度理解的差异。
 */
typedef uint8_t spiration_bool;

/** @brief 真。 */
#define SPIRATION_TRUE ((spiration_bool)1)
/** @brief 假。 */
#define SPIRATION_FALSE ((spiration_bool)0)

/**
 * @brief 调用结果状态码。
 * @note 新增状态码只会追加，既有取值永不复用。
 */
typedef enum spiration_status {
    SPIRATION_OK = 0,                   /**< 成功。 */
    SPIRATION_ERR_INVALID_ARGUMENT = 1, /**< 参数非法（含空指针、非法 UTF-8 等）。 */
    SPIRATION_ERR_NOT_FOUND = 2,        /**< 目标不存在。 */
    SPIRATION_ERR_UNSUPPORTED = 3,      /**< 当前宿主不支持该能力。 */
    SPIRATION_ERR_NOT_READY = 4,        /**< 前置条件未满足（如尚未初始化）。 */
    SPIRATION_ERR_ALREADY_EXISTS = 5,   /**< 目标已存在。 */
    SPIRATION_ERR_IO = 6,               /**< 输入输出失败。 */
    SPIRATION_ERR_INTERNAL = 7          /**< 宿主内部错误。 */
} spiration_status;

/**
 * @brief 日志等级，与宿主 `spiration::logging::level` 一一对应。
 */
typedef enum spiration_log_level {
    SPIRATION_LOG_TRACE = 0,   /**< 追溯。 */
    SPIRATION_LOG_DEBUG = 1,   /**< 调试。 */
    SPIRATION_LOG_INFO = 2,    /**< 信息。 */
    SPIRATION_LOG_WARNING = 3, /**< 警告。 */
    SPIRATION_LOG_ERROR = 4    /**< 错误。 */
} spiration_log_level;

/* -------------------------------------------------------------------------
 * 字符串
 * ---------------------------------------------------------------------- */

/**
 * @brief UTF-8 字符串视图（借用语义，不拥有内存）。
 *
 * - `data` 为 UTF-8 字节序列，**不要求**以 `\0` 结尾；
 * - `length` 为字节数，不含结尾 `\0`（若有）；
 * - `data == NULL` 当且仅当 `length == 0` 时合法，表示空串。
 *
 * @warning 视图所指内存的生命周期由提供方负责，详见 `spiration/api/host.h`
 *          中的“字符串生命周期”一节。
 */
typedef struct spiration_str_view {
    const char* data; /**< UTF-8 字节序列。 */
    size_t length;    /**< 字节数。 */
} spiration_str_view;

/**
 * @brief 编译期字符串长度（仅接受字符串字面量）。
 */
#if defined(__cplusplus)
SPIRATION_INLINE constexpr size_t spiration_static_strlen(const char* text,
                                                          size_t count = 0) {
    return *text == '\0' ? count : spiration_static_strlen(text + 1, count + 1);
}
#else
SPIRATION_INLINE size_t spiration_static_strlen(const char* text) {
    return text ? strlen(text) : 0u;
}
#endif

/**
 * @brief 由字符串字面量构造 `spiration_str_view` **初始化器**。
 * @note 仅可用于初始化式（聚合初始化、变量初始化），C 与 C++ 皆可。
 * @code
 * static spiration_str_view key = SPIRATION_STR_INIT("extension.hello.name");
 * @endcode
 */
#define SPIRATION_STR_INIT(literal) {(literal), sizeof(literal) - 1u}

/**
 * @brief 由字符串字面量构造 `spiration_str_view` **表达式**。
 * @note 可用于函数实参、赋值、`return` 等场合；参数必须是字符串字面量。
 * @code
 * host->log(self, SPIRATION_LOG_INFO, SPIRATION_STR("hello"), SPIRATION_STR_EMPTY);
 * @endcode
 */
#if defined(__cplusplus)
#define SPIRATION_STR(literal) \
    spiration_str_view{(literal), spiration_static_strlen(literal)}
#else
#define SPIRATION_STR(literal) ((spiration_str_view){(literal), sizeof(literal) - 1u})
#endif

/**
 * @brief 由 `const char*` 构造 `spiration_str_view`（运行时求长度）。
 * @note 字面量请使用 `SPIRATION_STR`，可避免运行期求长。
 */
SPIRATION_INLINE spiration_str_view spiration_str_view_cstr(const char* text) {
    spiration_str_view out;
    out.data = text;
    out.length = text ? spiration_static_strlen(text) : 0u;
    return out;
}

/**
 * @brief 由指针与长度构造 `spiration_str_view`。
 */
SPIRATION_INLINE spiration_str_view spiration_str_view_of(const char* data,
                                                          size_t length) {
    spiration_str_view out;
    out.data = data;
    out.length = (data ? length : 0u);
    return out;
}

/** @brief 空字符串视图。 */
#define SPIRATION_STR_EMPTY (spiration_str_view_cstr(NULL))

/**
 * @brief 判断两个字符串视图是否为同一内容。
 */
SPIRATION_INLINE spiration_bool spiration_str_view_equals(spiration_str_view a,
                                                          spiration_str_view b) {
    if (a.length != b.length) return SPIRATION_FALSE;
    if (a.length == 0u) return SPIRATION_TRUE;
    return memcmp(a.data, b.data, a.length) == 0u ? SPIRATION_TRUE
                                                  : SPIRATION_FALSE;
}

/**
 * @brief 判断字符串视图是否以 `\0` 结尾。
 * @note 仅用于判断能否安全地当作 C 字符串使用，不越界读取时结果为
 *       “未知”，此时返回 `SPIRATION_FALSE`。
 */
SPIRATION_INLINE spiration_bool spiration_str_view_is_cstr(
    spiration_str_view text) {
    return (text.data != NULL && text.length > 0u &&
            text.data[text.length] == '\0')
               ? SPIRATION_TRUE
               : SPIRATION_FALSE;
}

/* -------------------------------------------------------------------------
 * 复合数据
 * ---------------------------------------------------------------------- */

/**
 * @brief 线性颜色，各分量取值 `[0, 1]`。
 */
typedef struct spiration_color {
    float r; /**< 红。 */
    float g; /**< 绿。 */
    float b; /**< 蓝。 */
    float a; /**< 透明度。 */
} spiration_color;

/**
 * @brief 由 RGB 分量构造不透明颜色。
 */
SPIRATION_INLINE spiration_color spiration_color_rgb(float r, float g, float b) {
    spiration_color out;
    out.r = r;
    out.g = g;
    out.b = b;
    out.a = 1.0f;
    return out;
}

/**
 * @brief 语义化版本号。
 */
typedef struct spiration_sem_version {
    uint32_t major; /**< 主版本。 */
    uint32_t minor; /**< 次版本。 */
    uint32_t patch; /**< 修订版本。 */
} spiration_sem_version;

/**
 * @brief 字符串视图数组（借用语义）。
 */
typedef struct spiration_str_list {
    const spiration_str_view* items; /**< 元素数组。 */
    size_t count;                    /**< 元素个数。 */
} spiration_str_list;

/**
 * @brief 动态数据取值类型。
 */
typedef enum spiration_value_type {
    SPIRATION_VALUE_NONE = 0,    /**< 无值。 */
    SPIRATION_VALUE_BOOL = 1,    /**< `data.boolean`。 */
    SPIRATION_VALUE_INT = 2,     /**< `data.integer`。 */
    SPIRATION_VALUE_FLOAT = 3,   /**< `data.number`。 */
    SPIRATION_VALUE_STRING = 4,  /**< `data.text`。 */
    SPIRATION_VALUE_COLOR = 5,   /**< `data.color`。 */
    SPIRATION_VALUE_STRINGS = 6, /**< `data.strings`。 */
    SPIRATION_VALUE_POINTER = 7  /**< `data.pointer`。 */
} spiration_value_type;

/**
 * @brief 标签化的动态取值，用于属性读写、通用服务调用等场景。
 *
 * @note 该结构体是 C API 中唯一的“万能值”，其布局固定，便于各语言绑定
 *       一次映射、反复使用。
 */
typedef struct spiration_value {
    spiration_value_type type; /**< 当前生效的联合体成员。 */
    union {
        spiration_bool boolean;  /**< `SPIRATION_VALUE_BOOL`。 */
        int64_t integer;         /**< `SPIRATION_VALUE_INT`。 */
        double number;           /**< `SPIRATION_VALUE_FLOAT`。 */
        spiration_str_view text; /**< `SPIRATION_VALUE_STRING`。 */
        spiration_color color;   /**< `SPIRATION_VALUE_COLOR`。 */
        spiration_str_list strings; /**< `SPIRATION_VALUE_STRINGS`。 */
        void* pointer;           /**< `SPIRATION_VALUE_POINTER`。 */
    } data;
} spiration_value;

/** @brief 构造空值。 */
SPIRATION_INLINE spiration_value spiration_value_none(void) {
    spiration_value out;
    memset(&out, 0, sizeof(out));
    out.type = SPIRATION_VALUE_NONE;
    return out;
}

/** @brief 构造布尔值。 */
SPIRATION_INLINE spiration_value spiration_value_bool(spiration_bool v) {
    spiration_value out = spiration_value_none();
    out.type = SPIRATION_VALUE_BOOL;
    out.data.boolean = v;
    return out;
}

/** @brief 构造整数值。 */
SPIRATION_INLINE spiration_value spiration_value_int(int64_t v) {
    spiration_value out = spiration_value_none();
    out.type = SPIRATION_VALUE_INT;
    out.data.integer = v;
    return out;
}

/** @brief 构造浮点值。 */
SPIRATION_INLINE spiration_value spiration_value_float(double v) {
    spiration_value out = spiration_value_none();
    out.type = SPIRATION_VALUE_FLOAT;
    out.data.number = v;
    return out;
}

/** @brief 构造字符串值。 */
SPIRATION_INLINE spiration_value spiration_value_string(spiration_str_view v) {
    spiration_value out = spiration_value_none();
    out.type = SPIRATION_VALUE_STRING;
    out.data.text = v;
    return out;
}

/* -------------------------------------------------------------------------
 * 回调
 * ---------------------------------------------------------------------- */

/**
 * @brief 无参回调（菜单项点击、UI 线程投递等）。
 */
typedef void(SPIRATION_CALL* spiration_callback)(void* user_data);

/**
 * @brief 事件回调。
 * @param user_data 订阅时传入的用户数据。
 * @param event     事件名（UTF-8，宿主持有）。
 * @param data      事件负载（UTF-8，宿主持有，可为空串）。
 */
typedef void(SPIRATION_CALL* spiration_event_callback)(void* user_data,
                                                       spiration_str_view event,
                                                       spiration_str_view data);

/**
 * @brief 控件事件回调。
 * @param user_data 注册时传入的用户数据。
 * @param widget    触发事件的控件。
 * @param event     事件名（`"click"`、`"changed"` 等）。
 * @param data      事件负载，可为空值。
 */
typedef void(SPIRATION_CALL* spiration_widget_callback)(
    void* user_data, spiration_widget* widget, spiration_str_view event,
    spiration_value data);

SPIRATION_EXTERN_C_END

#endif /* SPIRATION_API_BASE_H */
