/**
 * @file loader.h
 * @brief 拓展加载器接口 —— 让脚本语言（Python / Node.js / ...）编写拓展。
 * @author 陈林锴
 *
 * @par 为什么需要加载器
 * Spiration 的拓展系统以“动态库 + 入口符号”为唯一装载原语。要让 Python、
 * Node.js 等语言编写拓展，只需让一个**原生拓展**扮演加载器：它自行启动
 * 解释器，并把脚本里的函数映射为 `spiration_extension_desc`。
 *
 * @par 协议
 * 1. 加载器本身是普通原生拓展，在 `SPIRATION_PHASE_EARLY` 初始化，
 *    并调用 `host->register_loader(self, &my_loader_api)`；
 * 2. 拓展清单中声明 `"loader": "<加载器拓展 ID>"`；
 * 3. 宿主解析清单后，把托管拓展的目录、入口文件与清单原文交给加载器的
 *    `instantiate`，取回一个描述符；
 * 4. 该描述符的生命周期由加载器负责；宿主在调用其 `on_shutdown` 之后
 *    调用 `release` 归还。
 */

#ifndef SPIRATION_API_LOADER_H
#define SPIRATION_API_LOADER_H

#include <spiration/api/base.h>

SPIRATION_EXTERN_C_BEGIN

/**
 * @brief 清单中声明加载器的键名。
 */
#define SPIRATION_MANIFEST_LOADER_KEY "loader"

/**
 * @brief 加载器函数表。
 *
 * @note 表首两个字段与所有接口表一致：`version`（`SPIRATION_API_VERSION`）
 *       与 `size`（本结构体字节数）。
 */
typedef struct spiration_loader_api {
    /** 加载器接口版本。 */
    uint32_t version;
    /** 本结构体字节数。 */
    uint32_t size;

    /**
     * @brief 判断本加载器能否处理指定拓展。
     * @param entry_path    清单 `main` 字段指向的入口文件绝对路径。
     * @param manifest_json 拓展清单原文。
     * @return 能处理返回 `SPIRATION_TRUE`。
     * @note 可选回调，可为 `NULL`。当清单未声明 `loader` 时，宿主会依次询问
     *       所有已注册的加载器。
     */
    spiration_bool(SPIRATION_CALL* probe)(spiration_str_view entry_path,
                                          spiration_str_view manifest_json);

    /**
     * @brief 为托管拓展构造描述符（相当于脚本世界的“入口函数”）。
     * @param host               宿主函数表，加载器应转交给脚本侧使用。
     * @param extension_id       拓展 ID（取自清单）。
     * @param extension_directory 拓展安装目录绝对路径。
     * @param entry_path         入口文件绝对路径。
     * @param manifest_json      拓展清单原文。
     * @return 描述符指针；失败返回 `NULL`。
     * @note 描述符通常由加载器动态分配，并填充好身份字段与回调，
     *       同时把描述符的 `host` 字段设为传入的 `host`。
     */
    spiration_extension_desc*(SPIRATION_CALL* instantiate)(
        const spiration_host_api* host, spiration_str_view extension_id,
        spiration_str_view extension_directory, spiration_str_view entry_path,
        spiration_str_view manifest_json);

    /**
     * @brief 归还由 `instantiate` 创建的描述符。
     * @param desc 描述符。
     * @note 宿主保证在调用 `desc->on_shutdown` 之后、且不再引用该描述符时调用。
     *       可为 `NULL`，表示描述符为静态存储、无需释放。
     */
    void(SPIRATION_CALL* release)(spiration_extension_desc* desc);

    /** 预留槽位，必须全部为 `NULL`。 */
    void* reserved[8];
} spiration_loader_api;

SPIRATION_EXTERN_C_END

#endif /* SPIRATION_API_LOADER_H */
