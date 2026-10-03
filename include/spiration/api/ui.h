/**
 * @file ui.h
 * @brief 可选接口：界面（标签页、控件、菜单、重绘）。
 * @author 陈林锴
 *
 * @par 获取方式
 * @code
 * const spiration_ui_api* ui = (const spiration_ui_api*)host->query_interface(
 *     self, SPIRATION_STR(SPIRATION_UI_INTERFACE_NAME), SPIRATION_UI_VERSION);
 * if (ui == NULL) { 宿主不支持界面能力 }
 * @endcode
 *
 * @par 控件模型
 * 界面采用 **保留模式控件树**：`widget_create` 创建控件，`widget_append`
 * 组装层级，`widget_set` / `widget_get` 以字符串键读写属性，
 * `widget_listen` 订阅交互事件。宿主只保证约定键的语义，未知键返回
 * `SPIRATION_ERR_UNSUPPORTED`，因此后续版本可以在不破坏 ABI 的前提下
 * 增加新键与新控件种类。
 */

#ifndef SPIRATION_API_UI_H
#define SPIRATION_API_UI_H

#include <spiration/api/base.h>

SPIRATION_EXTERN_C_BEGIN

/**
 * @brief 界面接口名，传给 `host->query_interface`。
 */
#define SPIRATION_UI_INTERFACE_NAME "spiration.ui"

/**
 * @brief 界面接口版本。
 */
#define SPIRATION_UI_VERSION \
    SPIRATION_API_VERSION_ENCODE(1, 0, 0)

/**
 * @brief 控件种类。
 */
typedef enum spiration_widget_kind {
    SPIRATION_WIDGET_CONTAINER = 1,     /**< 通用容器，可承载子控件。 */
    SPIRATION_WIDGET_LABEL = 2,         /**< 文本标签。 */
    SPIRATION_WIDGET_BUTTON = 3,        /**< 按钮。 */
    SPIRATION_WIDGET_CHECKBOX = 4,      /**< 复选框。 */
    SPIRATION_WIDGET_RADIO_BUTTON = 5,  /**< 单选框。 */
    SPIRATION_WIDGET_TOGGLE_SWITCH = 6, /**< 开关。 */
    SPIRATION_WIDGET_SLIDER = 7,        /**< 滑块。 */
    SPIRATION_WIDGET_PROGRESS_BAR = 8,  /**< 进度条。 */
    SPIRATION_WIDGET_COMBO_BOX = 9,     /**< 下拉框。 */
    SPIRATION_WIDGET_LIST_VIEW = 10,    /**< 列表。 */
    SPIRATION_WIDGET_TEXT_FIELD = 11,   /**< 单行输入框。 */
    SPIRATION_WIDGET_TEXT_AREA = 12,    /**< 多行输入框。 */
    SPIRATION_WIDGET_SCROLL_ROW = 13,   /**< 可滚动行容器。 */
    SPIRATION_WIDGET_SPLIT_PANE = 14,   /**< 分栏容器。 */
    SPIRATION_WIDGET_COLLAPSIBLE = 15,  /**< 折叠面板。 */
    SPIRATION_WIDGET_SEPARATOR = 16,    /**< 分隔线。 */
    SPIRATION_WIDGET_SPACER = 17,       /**< 弹性空白。 */
    SPIRATION_WIDGET_MARKDOWN = 18      /**< Markdown 视图。 */
} spiration_widget_kind;

/**
 * @brief 界面函数表。
 *
 * @par 约定属性键
 * | 键 | 取值类型 | 适用控件 |
 * |---|---|---|
 * | `x`、`y`、`width`、`height` | FLOAT | 全部 |
 * | `visible`、`enabled` | BOOL | 全部 |
 * | `tooltip` | STRING | 全部 |
 * | `layout` | STRING（`"none"` / `"row"` / `"column"` / `"flex"`） | 容器 |
 * | `text` | STRING | 标签、按钮、勾选控件、输入框、Markdown |
 * | `selectable`、`readonly`、`password` | BOOL | 文本类控件 |
 * | `placeholder` | STRING | 输入框 |
 * | `checked` | BOOL | 复选框、单选框、开关 |
 * | `value` | FLOAT | 滑块、进度条 |
 * | `min`、`max` | FLOAT | 滑块 |
 * | `indeterminate` | BOOL | 进度条 |
 * | `items` | STRINGS | 下拉框、列表 |
 * | `selected` | INT | 下拉框、列表 |
 *
 * @par 约定事件键
 * `"click"`、`"changed"`、`"submit"`、`"activated"`、`"focus"`、`"blur"`、
 * `"closed"`（控件被销毁，宿主保证此后再不回调）。
 */
typedef struct spiration_ui_api {
    /** 界面接口版本，取 `SPIRATION_UI_VERSION`。 */
    uint32_t version;
    /** 本结构体字节数。 */
    uint32_t size;

    /* ---- 窗口 ---- */

    /**
     * @brief 请求重绘窗口。
     */
    void(SPIRATION_CALL* request_repaint)(
        const spiration_extension_desc* self);

    /**
     * @brief 在宿主菜单栏中添加子项。
     * @param self     拓展描述符。
     * @param menu     菜单标题或菜单键，如 `"menu.help"`。
     * @param label    子项标签。
     * @param callback 点击回调。
     * @param user_data 传给回调的用户数据。
     * @return 状态码。
     */
    spiration_status(SPIRATION_CALL* add_menu_item)(
        const spiration_extension_desc* self, spiration_str_view menu,
        spiration_str_view label, spiration_callback callback,
        void* user_data);

    /* ---- 标签页 ---- */

    /**
     * @brief 打开一个标签页。
     * @param self  拓展描述符。
     * @param title 标签标题。
     * @return 标签页句柄；失败返回 `NULL`。
     * @note 句柄在标签页被关闭后失效，可用 `tab_is_alive` 判定。
     */
    spiration_tab*(SPIRATION_CALL* tab_open)(
        const spiration_extension_desc* self, spiration_str_view title);

    /**
     * @brief 关闭标签页。
     */
    void(SPIRATION_CALL* tab_close)(spiration_tab* tab);

    /**
     * @brief 激活标签页（若该页仍存在）。
     * @return 成功返回 `SPIRATION_OK`。
     */
    spiration_status(SPIRATION_CALL* tab_activate)(spiration_tab* tab);

    /**
     * @brief 修改标签页标题。
     */
    spiration_status(SPIRATION_CALL* tab_set_title)(spiration_tab* tab,
                                                    spiration_str_view title);

    /**
     * @brief 设置标签页内容控件。
     * @param tab     标签页句柄。
     * @param content 内容控件；传入 `NULL` 表示清空内容。
     */
    spiration_status(SPIRATION_CALL* tab_set_content)(spiration_tab* tab,
                                                      spiration_widget* content);

    /**
     * @brief 判断标签页句柄是否仍然有效。
     */
    spiration_bool(SPIRATION_CALL* tab_is_alive)(const spiration_tab* tab);

    /* ---- 控件 ---- */

    /**
     * @brief 创建控件。
     * @param kind 控件种类。
     * @return 控件句柄，所有权归调用者；失败返回 `NULL`。
     */
    spiration_widget*(SPIRATION_CALL* widget_create)(
        spiration_widget_kind kind);

    /**
     * @brief 销毁控件及其子树。
     */
    void(SPIRATION_CALL* widget_destroy)(spiration_widget* widget);

    /**
     * @brief 将 `child` 追加为 `parent` 的子控件，控件树接管其所有权。
     */
    spiration_status(SPIRATION_CALL* widget_append)(spiration_widget* parent,
                                                    spiration_widget* child);

    /**
     * @brief 写入控件属性。
     * @param widget   控件。
     * @param property 属性键。
     * @param value    属性值。
     * @return 状态码；键不受支持时返回 `SPIRATION_ERR_UNSUPPORTED`。
     */
    spiration_status(SPIRATION_CALL* widget_set)(spiration_widget* widget,
                                                 spiration_str_view property,
                                                 spiration_value value);

    /**
     * @brief 读取控件属性。
     * @param widget   控件。
     * @param property 属性键。
     * @param out      输出值。
     * @return 状态码。
     */
    spiration_status(SPIRATION_CALL* widget_get)(
        const spiration_widget* widget, spiration_str_view property,
        spiration_value* out);

    /**
     * @brief 订阅控件事件。
     * @param widget    控件。
     * @param event     事件键。
     * @param callback  回调。
     * @param user_data 传给回调的用户数据。
     * @return 状态码。
     */
    spiration_status(SPIRATION_CALL* widget_listen)(
        spiration_widget* widget, spiration_str_view event,
        spiration_widget_callback callback, void* user_data);

    /** 预留槽位，必须全部为 `NULL`。 */
    void* reserved[16];
} spiration_ui_api;

SPIRATION_EXTERN_C_END

#endif /* SPIRATION_API_UI_H */
