#ifndef LITE_COMMON_ENUM_C_H_
#define LITE_COMMON_ENUM_C_H_

/*!
 * \brief The log level.
 */
typedef enum {
    DEBUG = 0, /*!< The lowest level and most verbose */
    INFO = 1,  /*!< The lowest level and most verbose */
    WARN = 2,  /*!< Print only warning and errors */
    ERROR = 3, /*!< Print only errors */
} LiteLogLevel;

/*!
 * \brief The Error Code
 */
typedef enum {
    OK = 0,
    LITE_INTERNAL_ERROR = 1,
    LITE_UNKNOWN_ERROR = 2,
} ErrorCode;

typedef enum {
    LITE_DEFAULT = 0,  //! default backend is mge
} LiteBackend;

typedef enum {
    LITE_CPU = 0,
    LITE_CUDA = 1,
    LITE_ATLAS = 3,
    LITE_NPU = 4,
    LITE_CAMBRICON = 5,
    //! when the device information is set in model, so set LITE_DEVICE_DEFAULT
    //! in lite, which equal to xpu in megengine
    LITE_DEVICE_DEFAULT = 7,
} LiteDeviceType;

typedef enum {
    LITE_FLOAT = 0,
    LITE_HALF = 1,
    LITE_INT = 2,
    LITE_INT16 = 3,
    LITE_INT8 = 4,
    LITE_UINT8 = 5,
    LITE_UINT = 6,
    LITE_UINT16 = 7,
    LITE_INT64 = 8,
} LiteDataType;

typedef enum {
    //! Tensor maybe input or output
    LITE_IO = 0,
    //! Tensor is input
    LITE_INPUT = 1,
    //! Tensor is output
    LITE_OUTPUT = 2,
} LiteTensorPhase;

/*!
 * \brief the input and output type, include SHAPE and VALUE
 * sometimes user only need the shape of the output tensor
 */
typedef enum {
    LITE_IO_VALUE = 0,
    LITE_IO_SHAPE = 1,
} LiteIOType;

/*!
 * \brief operation algorithm seletion strategy type, some operations have
 * multi algorithms, different algorithm has different attribute, according to
 * the strategy, the best algorithm will be selected.
 *
 * Note: These strategies can be combined
 *
 * 1. LITE_ALGO_HEURISTIC | LITE_ALGO_PROFILE means: if profile cache not valid,
 * use heuristic instead
 *
 * 2. LITE_ALGO_HEURISTIC | LITE_ALGO_REPRODUCIBLE means: heuristic choice the
 * reproducible algo
 *
 * 3. LITE_ALGO_PROFILE | LITE_ALGO_REPRODUCIBLE means: profile the best
 * algorithm from the reproducible algorithms set
 *
 * 4. LITE_ALGO_PROFILE | LITE_ALGO_OPTIMIZED means: profile the best
 * algorithm form the optimzed algorithms, thus profile will process fast
 *
 * 5. LITE_ALGO_PROFILE | LITE_ALGO_OPTIMIZED | LITE_ALGO_REPRODUCIBLE means:
 * profile the best algorithm form the optimzed and reproducible algorithms
 */
typedef enum {
    LITE_ALGO_HEURISTIC = 1 << 0,
    LITE_ALGO_PROFILE = 1 << 1,
    LITE_ALGO_REPRODUCIBLE = 1 << 2,
    LITE_ALGO_OPTIMIZED = 1 << 3,
} LiteAlgoSelectStrategy;

/*!
 * \brief enum for cache compat level, for example: adreno 630 cache may be apply to
 * adreno 640, if you do not want search cache for adreno 640, just config SERIES_COMPAT
 * or VENDOR_COMPAT, adreno 506 cache may be apply to adreno 630, if you do not want
 * search cache for adreno 630, just config VENDOR_COMPAT
 * WARN: this config just let program_cache_io try `use a old cache` for `device compile
 * the cache` do not means MegEngine will insure the compile will be ok! it`s a device
 * CL driver behavior, if compile failed!, MegEngine will try build from source, What`s
 * more, even though compile from binary success, this cross-use-cache may affect
 * performance, VENDOR_COMPAT will contain SERIES_COMPAT
 */
typedef enum {
    LITE_NOT_COMPAT = 0,     //! default not compat for series and vendor
    LITE_SERIES_COMPAT = 1,  //! for scene adreno 640 use adreno 630 cache
    LITE_VENDOR_COMPAT = 2,  //! for scene adreno 630 use adreno 506 cache
    LITE_CACHE_COMPAT_LEVEL_CNT = 3
} LiteOpenCLCacheCompatLevel;

#endif
// vim: syntax=cpp.doxygen foldmethod=marker foldmarker=f{{{,f}}}
