/**
 * @file version.h
 * @brief Spiration C API 版本号与兼容性判定。
 * @author 陈林锴
 */

#ifndef SPIRATION_API_VERSION_H
#define SPIRATION_API_VERSION_H

#include <stdint.h>

/**
 * @name 当前 C API 版本
 *
 * 约定：
 * - **主版本**（major）：发生不兼容变更时递增。宿主与拓展的主版本必须严格一致。
 * - **次版本**（minor）：以向后兼容的方式新增函数或结构体字段时递增。
 *   新增内容只允许追加到结构体尾部（占用 `reserved` 空槽），既有的字段、
 *   函数指针的位置与含义永不改变。
 * - **修订版本**（patch）：仅修补文档、注释或不影响 ABI 的实现错误。
 * @{
 */
#define SPIRATION_API_VERSION_MAJOR 1 /**< 主版本。 */
#define SPIRATION_API_VERSION_MINOR 0 /**< 次版本。 */
#define SPIRATION_API_VERSION_PATCH 0 /**< 修订版本。 */
/** @} */

/**
 * @brief 将三段版本号编码为单个整数，便于跨边界比较与协商。
 */
#define SPIRATION_API_VERSION_ENCODE(major, minor, patch)          \
    ((((uint32_t)(major)) << 16) | (((uint32_t)(minor)) << 8) |    \
     ((uint32_t)(patch)))

/**
 * @brief 当前 C API 版本的编码值。
 */
#define SPIRATION_API_VERSION                                              \
    SPIRATION_API_VERSION_ENCODE(SPIRATION_API_VERSION_MAJOR,              \
                                 SPIRATION_API_VERSION_MINOR,              \
                                 SPIRATION_API_VERSION_PATCH)

/**
 * @brief 当前 C API 版本的可读字符串。
 */
#define SPIRATION_API_VERSION_STRING "1.0.0"

/** @brief 取编码版本的主版本号。 */
#define SPIRATION_API_VERSION_MAJOR_OF(version) \
    ((uint32_t)(((uint32_t)(version) >> 16) & 0xFFu))
/** @brief 取编码版本的次版本号。 */
#define SPIRATION_API_VERSION_MINOR_OF(version) \
    ((uint32_t)(((uint32_t)(version) >> 8) & 0xFFu))
/** @brief 取编码版本的修订版本号。 */
#define SPIRATION_API_VERSION_PATCH_OF(version) \
    ((uint32_t)((uint32_t)(version) & 0xFFu))

/**
 * @brief 判断宿主版本是否满足拓展所需版本。
 * @param host_version     宿主提供的版本（编码值）。
 * @param required_version 拓展需要的版本（编码值）。
 * @return 主版本一致且宿主次版本不低于所需次版本时为 `1`，否则 `0`。
 *
 * @note 该宏是唯一被认可的兼容性判据。拓展应在 `on_initialize` 前自行校验，
 *       宿主亦会在装载时以同样的规则拒绝不兼容的拓展。
 */
#define SPIRATION_API_VERSION_COMPATIBLE(host_version, required_version)     \
    ((SPIRATION_API_VERSION_MAJOR_OF(host_version) ==                        \
      SPIRATION_API_VERSION_MAJOR_OF(required_version)) &&                   \
     ((uint32_t)(host_version) >= (uint32_t)(required_version)))

/**
 * @brief 结构体版本号，用于结构体自身的追加式演进。
 * @note 结构体首字段恒为 `version`（该宏取值）与 `size`（**宿主填写的**字节数）。
 *       拓展只应访问 `size` 覆盖范围内的字段，其余字节不得读取。
 */
#define SPIRATION_STRUCT_VERSION SPIRATION_API_VERSION

/**
 * @brief 兼容性别名：拓展清单中的 `api_version` 应声明为该值。
 */
#define SPIRATION_EXTENSION_API_VERSION SPIRATION_API_VERSION

#endif /* SPIRATION_API_VERSION_H */
