/**
 * @file export.h
 * @brief 符号导出、调用约定与语言链接宏。
 * @author 陈林锴
 */

#ifndef SPIRATION_EXPORT_H
#define SPIRATION_EXPORT_H

/**
 * @brief 宿主导出，拓展导入。
 * @note 拓展构建时应定义 `SPIRATION_EXTENSION`。
 */
#if defined(_WIN32) || defined(__CYGWIN__)
    #if defined(SPIRATION_EXTENSION)
        #define SPIRATION_EXPORT __declspec(dllimport)
    #else
        #define SPIRATION_EXPORT __declspec(dllexport)
    #endif
    #define SPIRATION_ENTRY __declspec(dllexport)
#elif defined(__GNUC__) || defined(__clang__)
    #define SPIRATION_EXPORT __attribute__((visibility("default")))
    #define SPIRATION_ENTRY __attribute__((visibility("default")))
#else
    #define SPIRATION_EXPORT
    #define SPIRATION_ENTRY
#endif

/**
 * @brief 静态库场景下无导出语义。
 */
#define SPIRATION_LOCAL

/**
 * @brief ABI 调用约定。
 * @note 所有跨边界函数（宿主函数表与拓展回调）必须使用该约定。
 */
#if defined(_WIN32) && !defined(_WIN64) && !defined(__clang__) && !defined(__GNUC__)
    #define SPIRATION_CALL __cdecl
#else
    #define SPIRATION_CALL
#endif

/**
 * @brief 头文件内联函数。
 */
#if defined(__cplusplus)
    #define SPIRATION_INLINE static inline
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
    #define SPIRATION_INLINE static inline
#elif defined(_MSC_VER)
    #define SPIRATION_INLINE static __inline
#else
    #define SPIRATION_INLINE static
#endif

/**
 * @brief 语言链接包裹，使头文件可同时被 C 与 C++ 使用。
 */
#ifdef __cplusplus
    #define SPIRATION_EXTERN_C_BEGIN extern "C" {
    #define SPIRATION_EXTERN_C_END }
#else
    #define SPIRATION_EXTERN_C_BEGIN
    #define SPIRATION_EXTERN_C_END
#endif

/**
 * @brief 标记不使用 / 暂未实现的参数。
 */
#define SPIRATION_UNUSED(x) (void)(x)

#endif /* SPIRATION_EXPORT_H */
