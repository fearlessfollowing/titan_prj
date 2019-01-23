#include "web_http.h"
#include "../include/log/log_wrapper.h"

#undef  TAG 
#define TAG "webHttp"

#ifndef CS_MONGOOSE_SRC_INTERNAL_H_
#define CS_MONGOOSE_SRC_INTERNAL_H_

#ifndef MBUF_REALLOC
#define MBUF_REALLOC MG_REALLOC
#endif

#ifndef MBUF_FREE
#define MBUF_FREE MG_FREE
#endif

#define MG_SET_PTRPTR(_ptr, _v) \
  do {                          \
    if (_ptr) *(_ptr) = _v;     \
  } while (0)

#ifndef MG_INTERNAL
#define MG_INTERNAL static
#endif

#ifdef PICOTCP
#define NO_LIBC
#define MG_DISABLE_PFS
#endif

/* Amalgamated: #include "common/cs_dbg.h" */
/* Amalgamated: #include "mg_http.h" */
/* Amalgamated: #include "mg_net.h" */

#define MG_CTL_MSG_MESSAGE_SIZE 8192

/* internals that need to be accessible in unit tests */
MG_INTERNAL struct mg_connection *mg_do_connect(struct mg_connection *nc,
                                                int proto,
                                                union socket_address *sa);

MG_INTERNAL int mg_parse_address(const char *str, union socket_address *sa,
                                 int *proto, char *host, size_t host_len);
MG_INTERNAL void mg_call(struct mg_connection *nc,
                         mg_event_handler_t ev_handler, void *user_data, int ev,
                         void *ev_data);
void mg_forward(struct mg_connection *from, struct mg_connection *to);
MG_INTERNAL void mg_add_conn(struct mg_mgr *mgr, struct mg_connection *c);
MG_INTERNAL void mg_remove_conn(struct mg_connection *c);
MG_INTERNAL struct mg_connection *mg_create_connection(
    struct mg_mgr *mgr, mg_event_handler_t callback,
    struct mg_add_sock_opts opts);

#ifdef _WIN32
/* Retur value is the same as for MultiByteToWideChar. */
int to_wchar(const char *path, wchar_t *wbuf, size_t wbuf_len);
#endif

struct ctl_msg {
  mg_event_handler_t callback;
  char message[MG_CTL_MSG_MESSAGE_SIZE];
};

#if MG_ENABLE_MQTT
struct mg_mqtt_message;

#define MG_MQTT_ERROR_INCOMPLETE_MSG -1
#define MG_MQTT_ERROR_MALFORMED_MSG -2

MG_INTERNAL int parse_mqtt(struct mbuf *io, struct mg_mqtt_message *mm);
#endif

/* Forward declarations for testing. */
extern void *(*test_malloc)(size_t size);
extern void *(*test_calloc)(size_t count, size_t size);

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#if MG_ENABLE_HTTP
struct mg_serve_http_opts;

/*
 * Reassemble the content of the buffer (buf, blen) which should be
 * in the HTTP chunked encoding, by collapsing data chunks to the
 * beginning of the buffer.
 *
 * If chunks get reassembled, modify hm->body to point to the reassembled
 * body and fire MG_EV_HTTP_CHUNK event. If handler sets MG_F_DELETE_CHUNK
 * in nc->flags, delete reassembled body from the mbuf.
 *
 * Return reassembled body size.
 */
MG_INTERNAL size_t mg_handle_chunked(struct mg_connection *nc,
                                     struct http_message *hm, char *buf,
                                     size_t blen);

#if MG_ENABLE_FILESYSTEM
MG_INTERNAL int mg_uri_to_local_path(struct http_message *hm,
                                     const struct mg_serve_http_opts *opts,
                                     char **local_path,
                                     struct mg_str *remainder);
MG_INTERNAL time_t mg_parse_date_string(const char *datetime);
MG_INTERNAL int mg_is_not_modified(struct http_message *hm, cs_stat_t *st);
#endif
#if MG_ENABLE_HTTP_CGI
MG_INTERNAL void mg_handle_cgi(struct mg_connection *nc, const char *prog,
                               const struct mg_str *path_info,
                               const struct http_message *hm,
                               const struct mg_serve_http_opts *opts);
struct mg_http_proto_data_cgi;
MG_INTERNAL void mg_http_free_proto_data_cgi(struct mg_http_proto_data_cgi *d);
#endif
#if MG_ENABLE_HTTP_SSI
MG_INTERNAL void mg_handle_ssi_request(struct mg_connection *nc,
                                       struct http_message *hm,
                                       const char *path,
                                       const struct mg_serve_http_opts *opts);
#endif
#if MG_ENABLE_HTTP_WEBDAV
MG_INTERNAL int mg_is_dav_request(const struct mg_str *s);
MG_INTERNAL void mg_handle_propfind(struct mg_connection *nc, const char *path,
                                    cs_stat_t *stp, struct http_message *hm,
                                    struct mg_serve_http_opts *opts);
MG_INTERNAL void mg_handle_lock(struct mg_connection *nc, const char *path);
MG_INTERNAL void mg_handle_mkcol(struct mg_connection *nc, const char *path,
                                 struct http_message *hm);
MG_INTERNAL void mg_handle_move(struct mg_connection *c,
                                const struct mg_serve_http_opts *opts,
                                const char *path, struct http_message *hm);
MG_INTERNAL void mg_handle_delete(struct mg_connection *nc,
                                  const struct mg_serve_http_opts *opts,
                                  const char *path);
MG_INTERNAL void mg_handle_put(struct mg_connection *nc, const char *path,
                               struct http_message *hm);
#endif
#if MG_ENABLE_HTTP_WEBSOCKET
MG_INTERNAL void mg_ws_handler(struct mg_connection *nc, int ev,
                               void *ev_data MG_UD_ARG(void *user_data));
MG_INTERNAL void mg_ws_handshake(struct mg_connection *nc,
                                 const struct mg_str *key,
                                 struct http_message *);
#endif
#endif /* MG_ENABLE_HTTP */

MG_INTERNAL int mg_get_errno(void);

MG_INTERNAL void mg_close_conn(struct mg_connection *conn);

#if MG_ENABLE_SNTP
MG_INTERNAL int mg_sntp_parse_reply(const char *buf, int len,
                                    struct mg_sntp_message *msg);
#endif

#endif /* CS_MONGOOSE_SRC_INTERNAL_H_ */


#ifndef CS_COMMON_MG_MEM_H_
#define CS_COMMON_MG_MEM_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MG_MALLOC
#define MG_MALLOC malloc
#endif

#ifndef MG_CALLOC
#define MG_CALLOC calloc
#endif

#ifndef MG_REALLOC
#define MG_REALLOC realloc
#endif

#ifndef MG_FREE
#define MG_FREE free
#endif

#ifdef __cplusplus
}
#endif

#endif /* CS_COMMON_MG_MEM_H_ */



#ifndef EXCLUDE_COMMON

#include <string.h>


/* ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/ */

#define NUM_UPPERCASES ('Z' - 'A' + 1)
#define NUM_LETTERS (NUM_UPPERCASES * 2)
#define NUM_DIGITS ('9' - '0' + 1)

/*
 * Emit a base64 code char.
 *
 * Doesn't use memory, thus it's safe to use to safely dump memory in crashdumps
 */
static void cs_base64_emit_code(struct cs_base64_ctx *ctx, int v) 
{
    if (v < NUM_UPPERCASES) {
        ctx->b64_putc(v + 'A', ctx->user_data);
    } else if (v < (NUM_LETTERS)) {
        ctx->b64_putc(v - NUM_UPPERCASES + 'a', ctx->user_data);
    } else if (v < (NUM_LETTERS + NUM_DIGITS)) {
        ctx->b64_putc(v - NUM_LETTERS + '0', ctx->user_data);
    } else {
        ctx->b64_putc(v - NUM_LETTERS - NUM_DIGITS == 0 ? '+' : '/', ctx->user_data);
    }
}

static void cs_base64_emit_chunk(struct cs_base64_ctx *ctx) 
{
    int a, b, c;

    a = ctx->chunk[0];
    b = ctx->chunk[1];
    c = ctx->chunk[2];

    cs_base64_emit_code(ctx, a >> 2);
    cs_base64_emit_code(ctx, ((a & 3) << 4) | (b >> 4));
    if (ctx->chunk_size > 1) {
        cs_base64_emit_code(ctx, (b & 15) << 2 | (c >> 6));
    }
    
    if (ctx->chunk_size > 2) {
        cs_base64_emit_code(ctx, c & 63);
    }
}

void cs_base64_init(struct cs_base64_ctx *ctx, cs_base64_putc_t b64_putc, void *user_data) 
{
    ctx->chunk_size = 0;
    ctx->b64_putc = b64_putc;
    ctx->user_data = user_data;
}

void cs_base64_update(struct cs_base64_ctx *ctx, const char *str, size_t len) 
{
    const unsigned char *src = (const unsigned char *) str;
    size_t i;
    for (i = 0; i < len; i++) {
        ctx->chunk[ctx->chunk_size++] = src[i];
        if (ctx->chunk_size == 3) {
            cs_base64_emit_chunk(ctx);
            ctx->chunk_size = 0;
        }
    }
}

void cs_base64_finish(struct cs_base64_ctx *ctx) 
{
    if (ctx->chunk_size > 0) {
        int i;
        memset(&ctx->chunk[ctx->chunk_size], 0, 3 - ctx->chunk_size);
        cs_base64_emit_chunk(ctx);
        for (i = 0; i < (3 - ctx->chunk_size); i++) {
            ctx->b64_putc('=', ctx->user_data);
        }
    }
}

#define BASE64_ENCODE_BODY                                                \
  static const char *b64 =                                                \
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"; \
  int i, j, a, b, c;                                                      \
                                                                          \
  for (i = j = 0; i < src_len; i += 3) {                                  \
    a = src[i];                                                           \
    b = i + 1 >= src_len ? 0 : src[i + 1];                                \
    c = i + 2 >= src_len ? 0 : src[i + 2];                                \
                                                                          \
    BASE64_OUT(b64[a >> 2]);                                              \
    BASE64_OUT(b64[((a & 3) << 4) | (b >> 4)]);                           \
    if (i + 1 < src_len) {                                                \
      BASE64_OUT(b64[(b & 15) << 2 | (c >> 6)]);                          \
    }                                                                     \
    if (i + 2 < src_len) {                                                \
      BASE64_OUT(b64[c & 63]);                                            \
    }                                                                     \
  }                                                                       \
                                                                          \
  while (j % 4 != 0) {                                                    \
    BASE64_OUT('=');                                                      \
  }                                                                       \
  BASE64_FLUSH()

#define BASE64_OUT(ch) \
  do {                 \
    dst[j++] = (ch);   \
  } while (0)

#define BASE64_FLUSH() \
  do {                 \
    dst[j++] = '\0';   \
  } while (0)

void cs_base64_encode(const unsigned char *src, int src_len, char *dst) {
  BASE64_ENCODE_BODY;
}

#undef BASE64_OUT
#undef BASE64_FLUSH

#if CS_ENABLE_STDIO
#define BASE64_OUT(ch)      \
    do {                      \
        fprintf(f, "%c", (ch)); \
        j++;                    \
    } while (0)

#define BASE64_FLUSH()

void cs_fprint_base64(FILE *f, const unsigned char *src, int src_len) 
{
    BASE64_ENCODE_BODY;
}

#undef BASE64_OUT
#undef BASE64_FLUSH
#endif /* CS_ENABLE_STDIO */

/* Convert one byte of encoded base64 input stream to 6-bit chunk */
static unsigned char from_b64(unsigned char ch) 
{
    /* Inverse lookup map */
    static const unsigned char tab[128] = {
        255, 255, 255, 255,
        255, 255, 255, 255, /*  0 */
        255, 255, 255, 255,
        255, 255, 255, 255, /*  8 */
        255, 255, 255, 255,
        255, 255, 255, 255, /*  16 */
        255, 255, 255, 255,
        255, 255, 255, 255, /*  24 */
        255, 255, 255, 255,
        255, 255, 255, 255, /*  32 */
        255, 255, 255, 62,
        255, 255, 255, 63, /*  40 */
        52,  53,  54,  55,
        56,  57,  58,  59, /*  48 */
        60,  61,  255, 255,
        255, 200, 255, 255, /*  56   '=' is 200, on index 61 */
        255, 0,   1,   2,
        3,   4,   5,   6, /*  64 */
        7,   8,   9,   10,
        11,  12,  13,  14, /*  72 */
        15,  16,  17,  18,
        19,  20,  21,  22, /*  80 */
        23,  24,  25,  255,
        255, 255, 255, 255, /*  88 */
        255, 26,  27,  28,
        29,  30,  31,  32, /*  96 */
        33,  34,  35,  36,
        37,  38,  39,  40, /*  104 */
        41,  42,  43,  44,
        45,  46,  47,  48, /*  112 */
        49,  50,  51,  255,
        255, 255, 255, 255, /*  120 */
    };
    return tab[ch & 127];
}

int cs_base64_decode(const unsigned char *s, int len, char *dst, int *dec_len) 
{
    unsigned char a, b, c, d;
    int orig_len = len;
    char *orig_dst = dst;
    while (len >= 4 && (a = from_b64(s[0])) != 255 &&
         (b = from_b64(s[1])) != 255 && (c = from_b64(s[2])) != 255 &&
         (d = from_b64(s[3])) != 255) {
    
        s += 4;
        len -= 4;
        if (a == 200 || b == 200) break; /* '=' can't be there */
        *dst++ = a << 2 | b >> 4;
        if (c == 200) break;
        *dst++ = b << 4 | c >> 2;
        if (d == 200) break;
        *dst++ = c << 6 | d;
    }
  
    *dst = 0;
    if (dec_len != NULL) *dec_len = (dst - orig_dst);
    return orig_len - len;
}

#endif /* EXCLUDE_COMMON */


#ifndef CS_COMMON_CS_DBG_H_
#define CS_COMMON_CS_DBG_H_


#if CS_ENABLE_STDIO
#include <stdio.h>
#endif

#ifndef CS_ENABLE_DEBUG
#define CS_ENABLE_DEBUG 0
#endif

#ifndef CS_LOG_ENABLE_TS_DIFF
#define CS_LOG_ENABLE_TS_DIFF 1
#endif


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
 * Log level; `LL_INFO` is the default. Use `cs_log_set_level()` to change it.
 */
enum cs_log_level {
    LL_NONE = -1,
    LL_ERROR = 0,
    LL_WARN = 1,
    LL_INFO = 2,
    LL_DEBUG = 3,
    LL_VERBOSE_DEBUG = 4,

    _LL_MIN = -2,
    _LL_MAX = 5,
};

/*
 * Set max log level to print; messages with the level above the given one will
 * not be printed.
 */
void cs_log_set_level(enum cs_log_level level);


void cs_log_set_filter(const char *pattern);

/*
 * Helper function which prints message prefix with the given `level`, function
 * name `func` and `filename`. If message should be printed (accordingly to the
 * current log level and filter), prints the prefix and returns 1, otherwise
 * returns 0.
 *
 * Clients should typically just use `LOG()` macro.
 */
int cs_log_print_prefix(enum cs_log_level level, const char *func, const char *filename);

extern enum cs_log_level cs_log_threshold;


#if CS_ENABLE_STDIO

/*
 * Set file to write logs into. If `NULL`, logs go to `stderr`.
 */
void cs_log_set_file(FILE *file);

/*
 * Prints log to the current log file, appends "\n" in the end and flushes the
 * stream.
 */
void cs_log_printf(const char *fmt, ...) PRINTF_LIKE(1, 2);



#if CS_ENABLE_STDIO

/*
 * Format and print message `x` with the given level `l`. Example:
 *
 * ```c
 * LOG(LL_INFO, ("my info message: %d", 123));
 * LOG(LL_DEBUG, ("my debug message: %d", 123));
 * ```
 */
#define LOG(l, x)                                                    \
  do {                                                               \
    if (cs_log_print_prefix(l, __func__, __FILE__)) cs_log_printf x; \
  } while (0)

#else

#define LOG(l, x) ((void) l)

#endif

#ifndef CS_NDEBUG


#define DBG(x) LOG(LL_VERBOSE_DEBUG, x)

#else /* NDEBUG */

#define DBG(x)

#endif

#else /* CS_ENABLE_STDIO */

#define LOG(l, x)
#define DBG(x)

#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* CS_COMMON_CS_DBG_H_ */


#include <stdarg.h>
#include <stdio.h>
#include <string.h>


enum cs_log_level cs_log_threshold WEAK =

#if CS_ENABLE_DEBUG
    LL_VERBOSE_DEBUG;
#else
    LL_ERROR;
#endif

#if CS_ENABLE_STDIO
static char *s_filter_pattern = NULL;
static size_t s_filter_pattern_len;

void cs_log_set_filter(const char *pattern) WEAK;

FILE *cs_log_file WEAK = NULL;

#if CS_LOG_ENABLE_TS_DIFF
double cs_log_ts WEAK;
#endif

enum cs_log_level cs_log_cur_msg_level WEAK = LL_NONE;

void cs_log_set_filter(const char *pattern) 
{
    free(s_filter_pattern);
    if (pattern != NULL) {
        s_filter_pattern = strdup(pattern);
        s_filter_pattern_len = strlen(pattern);
    } else {
        s_filter_pattern = NULL;
        s_filter_pattern_len = 0;
    }
}

int cs_log_print_prefix(enum cs_log_level, const char *, const char *) WEAK;
int cs_log_print_prefix(enum cs_log_level level, const char *func, const char *filename) 
{
    char prefix[21];

    if (level > cs_log_threshold) return 0;
    if (s_filter_pattern != NULL &&
        mg_match_prefix(s_filter_pattern, s_filter_pattern_len, func) == 0 &&
        mg_match_prefix(s_filter_pattern, s_filter_pattern_len, filename) == 0) {
      return 0;
    }

    strncpy(prefix, func, 20);
    prefix[20] = '\0';
    if (cs_log_file == NULL) cs_log_file = stderr;
    cs_log_cur_msg_level = level;
    fprintf(cs_log_file, "%-20s ", prefix);

#if CS_LOG_ENABLE_TS_DIFF
    {
        double now = cs_time();
        fprintf(cs_log_file, "%7u ", (unsigned int) ((now - cs_log_ts) * 1000000));
        cs_log_ts = now;
    }
#endif
    return 1;
}

void cs_log_printf(const char *fmt, ...) WEAK;
void cs_log_printf(const char *fmt, ...) 
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(cs_log_file, fmt, ap);
    va_end(ap);
  
    fputc('\n', cs_log_file);
    fflush(cs_log_file);
    cs_log_cur_msg_level = LL_NONE;
}

void cs_log_set_file(FILE *file) WEAK;
void cs_log_set_file(FILE *file) 
{
    cs_log_file = file;
}

#else

void cs_log_set_filter(const char *pattern) 
{
    (void) pattern;
}

#endif /* CS_ENABLE_STDIO */

void cs_log_set_level(enum cs_log_level level) WEAK;
void cs_log_set_level(enum cs_log_level level) 
{
    cs_log_threshold = level;

#if CS_LOG_ENABLE_TS_DIFF && CS_ENABLE_STDIO
    cs_log_ts = cs_time();
#endif
}



#ifndef CS_COMMON_CS_DIRENT_H_
#define CS_COMMON_CS_DIRENT_H_

#include <limits.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifdef CS_DEFINE_DIRENT
typedef struct { int dummy; } DIR;

struct dirent {
    int d_ino;
    /* TODO(rojer): Use PATH_MAX but make sure it's sane on every platform */
    char d_name[256];
};

DIR *opendir(const char *dir_name);
int closedir(DIR *dir);
struct dirent *readdir(DIR *dir);
#endif /* CS_DEFINE_DIRENT */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_COMMON_CS_DIRENT_H_ */


/* ISO C requires a translation unit to contain at least one declaration */
typedef int cs_dirent_dummy;


/* Amalgamated: #include "common/cs_time.h" */

#ifndef _WIN32
#include <stddef.h>
/*
 * There is no sys/time.h on ARMCC.
 */
#if !(defined(__ARMCC_VERSION) || defined(__ICCARM__)) && \
    !defined(__TI_COMPILER_VERSION__) &&                  \
    (!defined(CS_PLATFORM) || CS_PLATFORM != CS_P_NXP_LPC)
#include <sys/time.h>
#endif
#else
#include <windows.h>
#endif

double cs_time(void) WEAK;
double cs_time(void) 
{
    double now;
    struct timeval tv;
    if (gettimeofday(&tv, NULL /* tz */) != 0) return 0;
    now = (double) tv.tv_sec + (((double) tv.tv_usec) / 1000000.0);
    return now;
}

double cs_timegm(const struct tm *tm) 
{
    /* Month-to-day offset for non-leap-years. */
    static const int month_day[12] = {0,   31,  59,  90,  120, 151,
                                    181, 212, 243, 273, 304, 334};

    /* Most of the calculation is easy; leap years are the main difficulty. */
    int month = tm->tm_mon % 12;
    int year = tm->tm_year + tm->tm_mon / 12;
    int year_for_leap;
    int64_t rt;

    if (month < 0) { /* Negative values % 12 are still negative. */
        month += 12;
        --year;
    }

    /* This is the number of Februaries since 1900. */
    year_for_leap = (month > 1) ? year + 1 : year;

    rt = tm->tm_sec /* Seconds */
        +
        60 *
          (tm->tm_min /* Minute = 60 seconds */
           +
           60 * (tm->tm_hour /* Hour = 60 minutes */
                 +
                 24 * (month_day[month] + tm->tm_mday - 1 /* Day = 24 hours */
                       + 365 * (year - 70)                /* Year = 365 days */
                       + (year_for_leap - 69) / 4 /* Every 4 years is leap... */
                       - (year_for_leap - 1) / 100 /* Except centuries... */
                       + (year_for_leap + 299) / 400))); /* Except 400s. */
    return rt < 0 ? -1 : (double) rt;
}

#ifndef CS_COMMON_CS_ENDIAN_H_
#define CS_COMMON_CS_ENDIAN_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * clang with std=-c99 uses __LITTLE_ENDIAN, by default
 * while for ex, RTOS gcc - LITTLE_ENDIAN, by default
 * it depends on __USE_BSD, but let's have everything
 */
#if !defined(BYTE_ORDER) && defined(__BYTE_ORDER)
#define BYTE_ORDER __BYTE_ORDER
#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN __LITTLE_ENDIAN
#endif /* LITTLE_ENDIAN */
#ifndef BIG_ENDIAN
#define BIG_ENDIAN __LITTLE_ENDIAN
#endif /* BIG_ENDIAN */
#endif /* BYTE_ORDER */

#ifdef __cplusplus
}
#endif

#endif /* CS_COMMON_CS_ENDIAN_H_ */


#if !defined(EXCLUDE_COMMON)
#if !CS_DISABLE_MD5

/* Amalgamated: #include "common/cs_endian.h" */

static void byteReverse(unsigned char *buf, unsigned longs) {
/* Forrest: MD5 expect LITTLE_ENDIAN, swap if BIG_ENDIAN */
#if BYTE_ORDER == BIG_ENDIAN
    do {
        uint32_t t = (uint32_t)((unsigned) buf[3] << 8 | buf[2]) << 16 |
                 ((unsigned) buf[1] << 8 | buf[0]);
        *(uint32_t *) buf = t;
        buf += 4;
    } while (--longs);
#else
    (void) buf;
    (void) longs;
#endif
}

#define F1(x, y, z) (z ^ (x & (y ^ z)))
#define F2(x, y, z) F1(z, x, y)
#define F3(x, y, z) (x ^ y ^ z)
#define F4(x, y, z) (y ^ (x | ~z))

#define MD5STEP(f, w, x, y, z, data, s) \
  (w += f(x, y, z) + data, w = w << s | w >> (32 - s), w += x)

/*
 * Start MD5 accumulation.  Set bit count to 0 and buffer to mysterious
 * initialization constants.
 */
void cs_md5_init(cs_md5_ctx *ctx) 
{
    ctx->buf[0] = 0x67452301;
    ctx->buf[1] = 0xefcdab89;
    ctx->buf[2] = 0x98badcfe;
    ctx->buf[3] = 0x10325476;

    ctx->bits[0] = 0;
    ctx->bits[1] = 0;
}

static void cs_md5_transform(uint32_t buf[4], uint32_t const in[16]) 
{
    register uint32_t a, b, c, d;

    a = buf[0];
    b = buf[1];
    c = buf[2];
    d = buf[3];

    MD5STEP(F1, a, b, c, d, in[0] + 0xd76aa478, 7);
    MD5STEP(F1, d, a, b, c, in[1] + 0xe8c7b756, 12);
    MD5STEP(F1, c, d, a, b, in[2] + 0x242070db, 17);
    MD5STEP(F1, b, c, d, a, in[3] + 0xc1bdceee, 22);
    MD5STEP(F1, a, b, c, d, in[4] + 0xf57c0faf, 7);
    MD5STEP(F1, d, a, b, c, in[5] + 0x4787c62a, 12);
    MD5STEP(F1, c, d, a, b, in[6] + 0xa8304613, 17);
    MD5STEP(F1, b, c, d, a, in[7] + 0xfd469501, 22);
    MD5STEP(F1, a, b, c, d, in[8] + 0x698098d8, 7);
    MD5STEP(F1, d, a, b, c, in[9] + 0x8b44f7af, 12);
    MD5STEP(F1, c, d, a, b, in[10] + 0xffff5bb1, 17);
    MD5STEP(F1, b, c, d, a, in[11] + 0x895cd7be, 22);
    MD5STEP(F1, a, b, c, d, in[12] + 0x6b901122, 7);
    MD5STEP(F1, d, a, b, c, in[13] + 0xfd987193, 12);
    MD5STEP(F1, c, d, a, b, in[14] + 0xa679438e, 17);
    MD5STEP(F1, b, c, d, a, in[15] + 0x49b40821, 22);

    MD5STEP(F2, a, b, c, d, in[1] + 0xf61e2562, 5);
    MD5STEP(F2, d, a, b, c, in[6] + 0xc040b340, 9);
    MD5STEP(F2, c, d, a, b, in[11] + 0x265e5a51, 14);
    MD5STEP(F2, b, c, d, a, in[0] + 0xe9b6c7aa, 20);
    MD5STEP(F2, a, b, c, d, in[5] + 0xd62f105d, 5);
    MD5STEP(F2, d, a, b, c, in[10] + 0x02441453, 9);
    MD5STEP(F2, c, d, a, b, in[15] + 0xd8a1e681, 14);
    MD5STEP(F2, b, c, d, a, in[4] + 0xe7d3fbc8, 20);
    MD5STEP(F2, a, b, c, d, in[9] + 0x21e1cde6, 5);
    MD5STEP(F2, d, a, b, c, in[14] + 0xc33707d6, 9);
    MD5STEP(F2, c, d, a, b, in[3] + 0xf4d50d87, 14);
    MD5STEP(F2, b, c, d, a, in[8] + 0x455a14ed, 20);
    MD5STEP(F2, a, b, c, d, in[13] + 0xa9e3e905, 5);
    MD5STEP(F2, d, a, b, c, in[2] + 0xfcefa3f8, 9);
    MD5STEP(F2, c, d, a, b, in[7] + 0x676f02d9, 14);
    MD5STEP(F2, b, c, d, a, in[12] + 0x8d2a4c8a, 20);

    MD5STEP(F3, a, b, c, d, in[5] + 0xfffa3942, 4);
    MD5STEP(F3, d, a, b, c, in[8] + 0x8771f681, 11);
    MD5STEP(F3, c, d, a, b, in[11] + 0x6d9d6122, 16);
    MD5STEP(F3, b, c, d, a, in[14] + 0xfde5380c, 23);
    MD5STEP(F3, a, b, c, d, in[1] + 0xa4beea44, 4);
    MD5STEP(F3, d, a, b, c, in[4] + 0x4bdecfa9, 11);
    MD5STEP(F3, c, d, a, b, in[7] + 0xf6bb4b60, 16);
    MD5STEP(F3, b, c, d, a, in[10] + 0xbebfbc70, 23);
    MD5STEP(F3, a, b, c, d, in[13] + 0x289b7ec6, 4);
    MD5STEP(F3, d, a, b, c, in[0] + 0xeaa127fa, 11);
    MD5STEP(F3, c, d, a, b, in[3] + 0xd4ef3085, 16);
    MD5STEP(F3, b, c, d, a, in[6] + 0x04881d05, 23);
    MD5STEP(F3, a, b, c, d, in[9] + 0xd9d4d039, 4);
    MD5STEP(F3, d, a, b, c, in[12] + 0xe6db99e5, 11);
    MD5STEP(F3, c, d, a, b, in[15] + 0x1fa27cf8, 16);
    MD5STEP(F3, b, c, d, a, in[2] + 0xc4ac5665, 23);

    MD5STEP(F4, a, b, c, d, in[0] + 0xf4292244, 6);
    MD5STEP(F4, d, a, b, c, in[7] + 0x432aff97, 10);
    MD5STEP(F4, c, d, a, b, in[14] + 0xab9423a7, 15);
    MD5STEP(F4, b, c, d, a, in[5] + 0xfc93a039, 21);
    MD5STEP(F4, a, b, c, d, in[12] + 0x655b59c3, 6);
    MD5STEP(F4, d, a, b, c, in[3] + 0x8f0ccc92, 10);
    MD5STEP(F4, c, d, a, b, in[10] + 0xffeff47d, 15);
    MD5STEP(F4, b, c, d, a, in[1] + 0x85845dd1, 21);
    MD5STEP(F4, a, b, c, d, in[8] + 0x6fa87e4f, 6);
    MD5STEP(F4, d, a, b, c, in[15] + 0xfe2ce6e0, 10);
    MD5STEP(F4, c, d, a, b, in[6] + 0xa3014314, 15);
    MD5STEP(F4, b, c, d, a, in[13] + 0x4e0811a1, 21);
    MD5STEP(F4, a, b, c, d, in[4] + 0xf7537e82, 6);
    MD5STEP(F4, d, a, b, c, in[11] + 0xbd3af235, 10);
    MD5STEP(F4, c, d, a, b, in[2] + 0x2ad7d2bb, 15);
    MD5STEP(F4, b, c, d, a, in[9] + 0xeb86d391, 21);

    buf[0] += a;
    buf[1] += b;
    buf[2] += c;
    buf[3] += d;
}

void cs_md5_update(cs_md5_ctx *ctx, const unsigned char *buf, size_t len) 
{
    uint32_t t;

    t = ctx->bits[0];
    if ((ctx->bits[0] = t + ((uint32_t) len << 3)) < t) 
        ctx->bits[1]++;
    ctx->bits[1] += (uint32_t) len >> 29;

    t = (t >> 3) & 0x3f;

    if (t) {
        unsigned char *p = (unsigned char *) ctx->in + t;

        t = 64 - t;
        if (len < t) {
            memcpy(p, buf, len);
            return;
        }
        
        memcpy(p, buf, t);
        byteReverse(ctx->in, 16);
        cs_md5_transform(ctx->buf, (uint32_t *) ctx->in);
        buf += t;
        len -= t;
    }

    while (len >= 64) {
        memcpy(ctx->in, buf, 64);
        byteReverse(ctx->in, 16);
        cs_md5_transform(ctx->buf, (uint32_t *) ctx->in);
        buf += 64;
        len -= 64;
    }

    memcpy(ctx->in, buf, len);
}

void cs_md5_final(unsigned char digest[16], cs_md5_ctx *ctx) 
{
    unsigned count;
    unsigned char *p;
    uint32_t *a;

    count = (ctx->bits[0] >> 3) & 0x3F;

    p = ctx->in + count;
    *p++ = 0x80;
    count = 64 - 1 - count;
    if (count < 8) {
        memset(p, 0, count);
        byteReverse(ctx->in, 16);
        cs_md5_transform(ctx->buf, (uint32_t *) ctx->in);
        memset(ctx->in, 0, 56);
    } else {
        memset(p, 0, count - 8);
    }
    byteReverse(ctx->in, 14);

    a = (uint32_t *) ctx->in;
    a[14] = ctx->bits[0];
    a[15] = ctx->bits[1];

    cs_md5_transform(ctx->buf, (uint32_t *) ctx->in);
    byteReverse((unsigned char *) ctx->buf, 4);
    memcpy(digest, ctx->buf, 16);
    memset((char *) ctx, 0, sizeof(*ctx));
}

#endif /* CS_DISABLE_MD5 */
#endif /* EXCLUDE_COMMON */


#if !CS_DISABLE_SHA1 && !defined(EXCLUDE_COMMON)

/* Amalgamated: #include "common/cs_endian.h" */

#define SHA1HANDSOFF
#if defined(__sun)
/* Amalgamated: #include "common/solarisfixes.h" */
#endif

union char64long16 {
    unsigned char c[64];
    uint32_t l[16];
};

#define rol(value, bits) (((value) << (bits)) | ((value) >> (32 - (bits))))

static uint32_t blk0(union char64long16 *block, int i) 
{
/* Forrest: SHA expect BIG_ENDIAN, swap if LITTLE_ENDIAN */
#if BYTE_ORDER == LITTLE_ENDIAN
    block->l[i] = (rol(block->l[i], 24) & 0xFF00FF00) | (rol(block->l[i], 8) & 0x00FF00FF);
#endif
    return block->l[i];
}

/* Avoid redefine warning (ARM /usr/include/sys/ucontext.h define R0~R4) */
#undef blk
#undef R0
#undef R1
#undef R2
#undef R3
#undef R4

#define blk(i)                                                               \
  (block->l[i & 15] = rol(block->l[(i + 13) & 15] ^ block->l[(i + 8) & 15] ^ \
                              block->l[(i + 2) & 15] ^ block->l[i & 15],     \
                          1))
#define R0(v, w, x, y, z, i)                                          \
  z += ((w & (x ^ y)) ^ y) + blk0(block, i) + 0x5A827999 + rol(v, 5); \
  w = rol(w, 30);
#define R1(v, w, x, y, z, i)                                  \
  z += ((w & (x ^ y)) ^ y) + blk(i) + 0x5A827999 + rol(v, 5); \
  w = rol(w, 30);
#define R2(v, w, x, y, z, i)                          \
  z += (w ^ x ^ y) + blk(i) + 0x6ED9EBA1 + rol(v, 5); \
  w = rol(w, 30);
#define R3(v, w, x, y, z, i)                                        \
  z += (((w | x) & y) | (w & x)) + blk(i) + 0x8F1BBCDC + rol(v, 5); \
  w = rol(w, 30);
#define R4(v, w, x, y, z, i)                          \
  z += (w ^ x ^ y) + blk(i) + 0xCA62C1D6 + rol(v, 5); \
  w = rol(w, 30);

void cs_sha1_transform(uint32_t state[5], const unsigned char buffer[64]) 
{
    uint32_t a, b, c, d, e;
    union char64long16 block[1];

    memcpy(block, buffer, 64);
    a = state[0];
    b = state[1];
    c = state[2];
    d = state[3];
    e = state[4];
    R0(a, b, c, d, e, 0);
    R0(e, a, b, c, d, 1);
    R0(d, e, a, b, c, 2);
    R0(c, d, e, a, b, 3);
    R0(b, c, d, e, a, 4);
    R0(a, b, c, d, e, 5);
    R0(e, a, b, c, d, 6);
    R0(d, e, a, b, c, 7);
    R0(c, d, e, a, b, 8);
    R0(b, c, d, e, a, 9);
    R0(a, b, c, d, e, 10);
    R0(e, a, b, c, d, 11);
    R0(d, e, a, b, c, 12);
    R0(c, d, e, a, b, 13);
    R0(b, c, d, e, a, 14);
    R0(a, b, c, d, e, 15);
    R1(e, a, b, c, d, 16);
    R1(d, e, a, b, c, 17);
    R1(c, d, e, a, b, 18);
    R1(b, c, d, e, a, 19);
    R2(a, b, c, d, e, 20);
    R2(e, a, b, c, d, 21);
    R2(d, e, a, b, c, 22);
    R2(c, d, e, a, b, 23);
    R2(b, c, d, e, a, 24);
    R2(a, b, c, d, e, 25);
    R2(e, a, b, c, d, 26);
    R2(d, e, a, b, c, 27);
    R2(c, d, e, a, b, 28);
    R2(b, c, d, e, a, 29);
    R2(a, b, c, d, e, 30);
    R2(e, a, b, c, d, 31);
    R2(d, e, a, b, c, 32);
    R2(c, d, e, a, b, 33);
    R2(b, c, d, e, a, 34);
    R2(a, b, c, d, e, 35);
    R2(e, a, b, c, d, 36);
    R2(d, e, a, b, c, 37);
    R2(c, d, e, a, b, 38);
    R2(b, c, d, e, a, 39);
    R3(a, b, c, d, e, 40);
    R3(e, a, b, c, d, 41);
    R3(d, e, a, b, c, 42);
    R3(c, d, e, a, b, 43);
    R3(b, c, d, e, a, 44);
    R3(a, b, c, d, e, 45);
    R3(e, a, b, c, d, 46);
    R3(d, e, a, b, c, 47);
    R3(c, d, e, a, b, 48);
    R3(b, c, d, e, a, 49);
    R3(a, b, c, d, e, 50);
    R3(e, a, b, c, d, 51);
    R3(d, e, a, b, c, 52);
    R3(c, d, e, a, b, 53);
    R3(b, c, d, e, a, 54);
    R3(a, b, c, d, e, 55);
    R3(e, a, b, c, d, 56);
    R3(d, e, a, b, c, 57);
    R3(c, d, e, a, b, 58);
    R3(b, c, d, e, a, 59);
    R4(a, b, c, d, e, 60);
    R4(e, a, b, c, d, 61);
    R4(d, e, a, b, c, 62);
    R4(c, d, e, a, b, 63);
    R4(b, c, d, e, a, 64);
    R4(a, b, c, d, e, 65);
    R4(e, a, b, c, d, 66);
    R4(d, e, a, b, c, 67);
    R4(c, d, e, a, b, 68);
    R4(b, c, d, e, a, 69);
    R4(a, b, c, d, e, 70);
    R4(e, a, b, c, d, 71);
    R4(d, e, a, b, c, 72);
    R4(c, d, e, a, b, 73);
    R4(b, c, d, e, a, 74);
    R4(a, b, c, d, e, 75);
    R4(e, a, b, c, d, 76);
    R4(d, e, a, b, c, 77);
    R4(c, d, e, a, b, 78);
    R4(b, c, d, e, a, 79);
    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
    /* Erase working structures. The order of operations is important,
    * used to ensure that compiler doesn't optimize those out. */
    memset(block, 0, sizeof(block));
    a = b = c = d = e = 0;
    (void) a;
    (void) b;
    (void) c;
    (void) d;
    (void) e;
}

void cs_sha1_init(cs_sha1_ctx *context) 
{
    context->state[0] = 0x67452301;
    context->state[1] = 0xEFCDAB89;
    context->state[2] = 0x98BADCFE;
    context->state[3] = 0x10325476;
    context->state[4] = 0xC3D2E1F0;
    context->count[0] = context->count[1] = 0;
}

void cs_sha1_update(cs_sha1_ctx *context, const unsigned char *data, uint32_t len) 
{
    uint32_t i, j;

    j = context->count[0];
    if ((context->count[0] += len << 3) < j) context->count[1]++;
    context->count[1] += (len >> 29);
    j = (j >> 3) & 63;
    if ((j + len) > 63) {
        memcpy(&context->buffer[j], data, (i = 64 - j));
        cs_sha1_transform(context->state, context->buffer);
        for (; i + 63 < len; i += 64) {
            cs_sha1_transform(context->state, &data[i]);
        }
        j = 0;
    } else
        i = 0;
    memcpy(&context->buffer[j], &data[i], len - i);
}

void cs_sha1_final(unsigned char digest[20], cs_sha1_ctx *context) 
{
    unsigned i;
    unsigned char finalcount[8], c;

    for (i = 0; i < 8; i++) {
        finalcount[i] = (unsigned char) ((context->count[(i >= 4 ? 0 : 1)] >>
                                      ((3 - (i & 3)) * 8)) & 255);
    }
  
    c = 0200;
    cs_sha1_update(context, &c, 1);
    while ((context->count[0] & 504) != 448) {
        c = 0000;
        cs_sha1_update(context, &c, 1);
    }
    
    cs_sha1_update(context, finalcount, 8);
    for (i = 0; i < 20; i++) {
        digest[i] = (unsigned char) ((context->state[i >> 2] >> ((3 - (i & 3)) * 8)) & 255);
    }
    memset(context, '\0', sizeof(*context));
    memset(&finalcount, '\0', sizeof(finalcount));
}

void cs_hmac_sha1(const unsigned char *key, size_t keylen,
                  const unsigned char *data, size_t datalen,
                  unsigned char out[20]) 
{
    cs_sha1_ctx ctx;
    unsigned char buf1[64], buf2[64], tmp_key[20], i;

    if (keylen > sizeof(buf1)) {
        cs_sha1_init(&ctx);
        cs_sha1_update(&ctx, key, keylen);
        cs_sha1_final(tmp_key, &ctx);
        key = tmp_key;
        keylen = sizeof(tmp_key);
    }

    memset(buf1, 0, sizeof(buf1));
    memset(buf2, 0, sizeof(buf2));
    memcpy(buf1, key, keylen);
    memcpy(buf2, key, keylen);

    for (i = 0; i < sizeof(buf1); i++) {
        buf1[i] ^= 0x36;
        buf2[i] ^= 0x5c;
    }

    cs_sha1_init(&ctx);
    cs_sha1_update(&ctx, buf1, sizeof(buf1));
    cs_sha1_update(&ctx, data, datalen);
    cs_sha1_final(out, &ctx);

    cs_sha1_init(&ctx);
    cs_sha1_update(&ctx, buf2, sizeof(buf2));
    cs_sha1_update(&ctx, out, 20);
    cs_sha1_final(out, &ctx);
}

#endif /* EXCLUDE_COMMON */

#ifndef EXCLUDE_COMMON

#include <assert.h>
#include <string.h>


#ifndef MBUF_REALLOC
#define MBUF_REALLOC realloc
#endif

#ifndef MBUF_FREE
#define MBUF_FREE free
#endif

void mbuf_init(struct mbuf *mbuf, size_t initial_size) WEAK;
void mbuf_init(struct mbuf *mbuf, size_t initial_size) 
{
    mbuf->len = mbuf->size = 0;
    mbuf->buf = NULL;
    mbuf_resize(mbuf, initial_size);
}

void mbuf_free(struct mbuf *mbuf) WEAK;
void mbuf_free(struct mbuf *mbuf) 
{
    if (mbuf->buf != NULL) {
        MBUF_FREE(mbuf->buf);
        mbuf_init(mbuf, 0);
    }
}

void mbuf_resize(struct mbuf *a, size_t new_size) WEAK;
void mbuf_resize(struct mbuf *a, size_t new_size) 
{
    if (new_size > a->size || (new_size < a->size && new_size >= a->len)) {
        char *buf = (char *) MBUF_REALLOC(a->buf, new_size);
        /*
         * In case realloc fails, there's not much we can do, except keep things as
         * they are. Note that NULL is a valid return value from realloc when
         * size == 0, but that is covered too.
         */
        if (buf == NULL && new_size != 0) return;
        a->buf = buf;
        a->size = new_size;
    }
}

void mbuf_trim(struct mbuf *mbuf) WEAK;
void mbuf_trim(struct mbuf *mbuf) 
{
    mbuf_resize(mbuf, mbuf->len);
}

size_t mbuf_insert(struct mbuf *a, size_t off, const void *buf, size_t) WEAK;
size_t mbuf_insert(struct mbuf *a, size_t off, const void *buf, size_t len) 
{
    char *p = NULL;

    assert(a != NULL);
    assert(a->len <= a->size);
    assert(off <= a->len);

    /* check overflow */
    if (~(size_t) 0 - (size_t) a->buf < len) return 0;

    if (a->len + len <= a->size) {
        memmove(a->buf + off + len, a->buf + off, a->len - off);
        if (buf != NULL) {
            memcpy(a->buf + off, buf, len);
        }
        a->len += len;
    } else {
        size_t min_size = (a->len + len);
        size_t new_size = (size_t)(min_size * MBUF_SIZE_MULTIPLIER);
        if (new_size - min_size > MBUF_SIZE_MAX_HEADROOM) {
            new_size = min_size + MBUF_SIZE_MAX_HEADROOM;
        }
        
        p = (char *) MBUF_REALLOC(a->buf, new_size);
        if (p == NULL && new_size != min_size) {
            new_size = min_size;
            p = (char *) MBUF_REALLOC(a->buf, new_size);
        }
    
        if (p != NULL) {
            a->buf = p;
            if (off != a->len) {
                memmove(a->buf + off + len, a->buf + off, a->len - off);
            }
            if (buf != NULL) memcpy(a->buf + off, buf, len);
            a->len += len;
            a->size = new_size;
        } else {
            len = 0;
        }
    }

    return len;
}

size_t mbuf_append(struct mbuf *a, const void *buf, size_t len) WEAK;
size_t mbuf_append(struct mbuf *a, const void *buf, size_t len) 
{
    return mbuf_insert(a, a->len, buf, len);
}

void mbuf_remove(struct mbuf *mb, size_t n) WEAK;
void mbuf_remove(struct mbuf *mb, size_t n) 
{
    if (n > 0 && n <= mb->len) {
        memmove(mb->buf, mb->buf + n, mb->len - n);
        mb->len -= n;
    }
}

#endif /* EXCLUDE_COMMON */

#include <stdlib.h>
#include <string.h>

int mg_ncasecmp(const char *s1, const char *s2, size_t len) WEAK;

struct mg_str mg_mk_str(const char *s) WEAK;
struct mg_str mg_mk_str(const char *s) 
{
    struct mg_str ret = {s, 0};
    if (s != NULL) ret.len = strlen(s);
    return ret;
}

struct mg_str mg_mk_str_n(const char *s, size_t len) WEAK;
struct mg_str mg_mk_str_n(const char *s, size_t len) 
{
    struct mg_str ret = {s, len};
    return ret;
}

int mg_vcmp(const struct mg_str *str1, const char *str2) WEAK;
int mg_vcmp(const struct mg_str *str1, const char *str2) 
{
    size_t n2 = strlen(str2), n1 = str1->len;
    int r = strncmp(str1->p, str2, (n1 < n2) ? n1 : n2);
    if (r == 0) {
        return n1 - n2;
    }
    return r;
}

int mg_vcasecmp(const struct mg_str *str1, const char *str2) WEAK;
int mg_vcasecmp(const struct mg_str *str1, const char *str2) 
{
    size_t n2 = strlen(str2), n1 = str1->len;
    int r = mg_ncasecmp(str1->p, str2, (n1 < n2) ? n1 : n2);
    if (r == 0) {
        return n1 - n2;
    }
    return r;
}

static struct mg_str mg_strdup_common(const struct mg_str s, int nul_terminate) 
{
    struct mg_str r = {NULL, 0};
    if (s.len > 0 && s.p != NULL) {
        char *sc = (char *) MG_MALLOC(s.len + (nul_terminate ? 1 : 0));
        if (sc != NULL) {
            memcpy(sc, s.p, s.len);
            if (nul_terminate) sc[s.len] = '\0';
            r.p = sc;
            r.len = s.len;
        }
    }
    return r;
}

struct mg_str mg_strdup(const struct mg_str s) WEAK;
struct mg_str mg_strdup(const struct mg_str s) 
{
    return mg_strdup_common(s, 0 /* NUL-terminate */);
}

struct mg_str mg_strdup_nul(const struct mg_str s) WEAK;
struct mg_str mg_strdup_nul(const struct mg_str s) 
{
    return mg_strdup_common(s, 1 /* NUL-terminate */);
}

const char *mg_strchr(const struct mg_str s, int c) WEAK;
const char *mg_strchr(const struct mg_str s, int c) 
{
    size_t i;
    for (i = 0; i < s.len; i++) {
        if (s.p[i] == c) return &s.p[i];
    }
    return NULL;
}

int mg_strcmp(const struct mg_str str1, const struct mg_str str2) WEAK;
int mg_strcmp(const struct mg_str str1, const struct mg_str str2) 
{
    size_t i = 0;
    while (i < str1.len && i < str2.len) {
        if (str1.p[i] < str2.p[i]) return -1;
        if (str1.p[i] > str2.p[i]) return 1;
        i++;
    }
    if (i < str1.len) return 1;
    if (i < str2.len) return -1;
    return 0;
}

int mg_strncmp(const struct mg_str, const struct mg_str, size_t n) WEAK;
int mg_strncmp(const struct mg_str str1, const struct mg_str str2, size_t n) 
{
    struct mg_str s1 = str1;
    struct mg_str s2 = str2;

    if (s1.len > n) {
        s1.len = n;
    }
    
    if (s2.len > n) {
        s2.len = n;
    }
    return mg_strcmp(s1, s2);
}

const char *mg_strstr(const struct mg_str haystack,
                      const struct mg_str needle) WEAK;
const char *mg_strstr(const struct mg_str haystack, const struct mg_str needle) 
{
    size_t i;
    if (needle.len > haystack.len) 
        return NULL;
    for (i = 0; i <= haystack.len - needle.len; i++) {
        if (memcmp(haystack.p + i, needle.p, needle.len) == 0) {
            return haystack.p + i;
        }
    }
    return NULL;
}

struct mg_str mg_strstrip(struct mg_str s) WEAK;
struct mg_str mg_strstrip(struct mg_str s) 
{
    while (s.len > 0 && isspace((int) *s.p)) {
        s.p++;
        s.len--;
    }
    while (s.len > 0 && isspace((int) *(s.p + s.len - 1))) {
        s.len--;
    }
    return s;
}


#ifndef EXCLUDE_COMMON

#ifndef C_DISABLE_BUILTIN_SNPRINTF
#define C_DISABLE_BUILTIN_SNPRINTF 0
#endif

/* Amalgamated: #include "common/mg_mem.h" */

size_t c_strnlen(const char *s, size_t maxlen) WEAK;
size_t c_strnlen(const char *s, size_t maxlen)
{
    size_t l = 0;
    for (; l < maxlen && s[l] != '\0'; l++) {
    }
    return l;
}

#define C_SNPRINTF_APPEND_CHAR(ch)       \
  do {                                   \
    if (i < (int) buf_size) buf[i] = ch; \
    i++;                                 \
  } while (0)

#define C_SNPRINTF_FLAG_ZERO 1

#if C_DISABLE_BUILTIN_SNPRINTF
int c_vsnprintf(char *buf, size_t buf_size, const char *fmt, va_list ap) WEAK;
int c_vsnprintf(char *buf, size_t buf_size, const char *fmt, va_list ap) {
  return vsnprintf(buf, buf_size, fmt, ap);
}
#else
static int c_itoa(char *buf, size_t buf_size, int64_t num, int base, int flags, int field_width) 
{
    char tmp[40];
    int i = 0, k = 0, neg = 0;

    if (num < 0) {
        neg++;
        num = -num;
    }

    /* Print into temporary buffer - in reverse order */
    do {
        int rem = num % base;
        if (rem < 10) {
            tmp[k++] = '0' + rem;
        } else {
            tmp[k++] = 'a' + (rem - 10);
        }
        num /= base;
    } while (num > 0);

    /* Zero padding */
    if (flags && C_SNPRINTF_FLAG_ZERO) {
        while (k < field_width && k < (int) sizeof(tmp) - 1) {
            tmp[k++] = '0';
        }
    }

    /* And sign */
    if (neg) {
        tmp[k++] = '-';
    }

    /* Now output */
    while (--k >= 0) {
        C_SNPRINTF_APPEND_CHAR(tmp[k]);
    }

    return i;
}

int c_vsnprintf(char *buf, size_t buf_size, const char *fmt, va_list ap) WEAK;
int c_vsnprintf(char *buf, size_t buf_size, const char *fmt, va_list ap) 
{
    int ch, i = 0, len_mod, flags, precision, field_width;

    while ((ch = *fmt++) != '\0') {
        if (ch != '%') {
            C_SNPRINTF_APPEND_CHAR(ch);
        } else {
            /*
            * Conversion specification:
            *   zero or more flags (one of: # 0 - <space> + ')
            *   an optional minimum  field  width (digits)
            *   an  optional precision (. followed by digits, or *)
            *   an optional length modifier (one of: hh h l ll L q j z t)
            *   conversion specifier (one of: d i o u x X e E f F g G a A c s p n)
            */
            flags = field_width = precision = len_mod = 0;

            /* Flags. only zero-pad flag is supported. */
            if (*fmt == '0') {
                flags |= C_SNPRINTF_FLAG_ZERO;
            }

            /* Field width */
            while (*fmt >= '0' && *fmt <= '9') {
                field_width *= 10;
                field_width += *fmt++ - '0';
            }
      
            /* Dynamic field width */
            if (*fmt == '*') {
                field_width = va_arg(ap, int);
                fmt++;
            }

            /* Precision */
            if (*fmt == '.') {
                fmt++;
                if (*fmt == '*') {
                    precision = va_arg(ap, int);
                    fmt++;
                } else {
                    while (*fmt >= '0' && *fmt <= '9') {
                        precision *= 10;
                        precision += *fmt++ - '0';
                    }
                }
            }

            /* Length modifier */
            switch (*fmt) {
                case 'h':
                case 'l':
                case 'L':
                case 'I':
                case 'q':
                case 'j':
                case 'z':
                case 't':
                    len_mod = *fmt++;
                    if (*fmt == 'h') {
                        len_mod = 'H';
                        fmt++;
                    }
                    if (*fmt == 'l') {
                        len_mod = 'q';
                        fmt++;
                    }
                    break;
            }

            ch = *fmt++;
      if (ch == 's') {
        const char *s = va_arg(ap, const char *); /* Always fetch parameter */
        int j;
        int pad = field_width - (precision >= 0 ? c_strnlen(s, precision) : 0);
        for (j = 0; j < pad; j++) {
          C_SNPRINTF_APPEND_CHAR(' ');
        }

        /* `s` may be NULL in case of %.*s */
        if (s != NULL) {
          /* Ignore negative and 0 precisions */
          for (j = 0; (precision <= 0 || j < precision) && s[j] != '\0'; j++) {
            C_SNPRINTF_APPEND_CHAR(s[j]);
          }
        }
      } else if (ch == 'c') {
        ch = va_arg(ap, int); /* Always fetch parameter */
        C_SNPRINTF_APPEND_CHAR(ch);
      } else if (ch == 'd' && len_mod == 0) {
        i += c_itoa(buf + i, buf_size - i, va_arg(ap, int), 10, flags,
                    field_width);
      } else if (ch == 'd' && len_mod == 'l') {
        i += c_itoa(buf + i, buf_size - i, va_arg(ap, long), 10, flags,
                    field_width);
#ifdef SSIZE_MAX
      } else if (ch == 'd' && len_mod == 'z') {
        i += c_itoa(buf + i, buf_size - i, va_arg(ap, ssize_t), 10, flags,
                    field_width);
#endif
      } else if (ch == 'd' && len_mod == 'q') {
        i += c_itoa(buf + i, buf_size - i, va_arg(ap, int64_t), 10, flags,
                    field_width);
      } else if ((ch == 'x' || ch == 'u') && len_mod == 0) {
        i += c_itoa(buf + i, buf_size - i, va_arg(ap, unsigned),
                    ch == 'x' ? 16 : 10, flags, field_width);
      } else if ((ch == 'x' || ch == 'u') && len_mod == 'l') {
        i += c_itoa(buf + i, buf_size - i, va_arg(ap, unsigned long),
                    ch == 'x' ? 16 : 10, flags, field_width);
      } else if ((ch == 'x' || ch == 'u') && len_mod == 'z') {
        i += c_itoa(buf + i, buf_size - i, va_arg(ap, size_t),
                    ch == 'x' ? 16 : 10, flags, field_width);
      } else if (ch == 'p') {
        unsigned long num = (unsigned long) (uintptr_t) va_arg(ap, void *);
        C_SNPRINTF_APPEND_CHAR('0');
        C_SNPRINTF_APPEND_CHAR('x');
        i += c_itoa(buf + i, buf_size - i, num, 16, flags, 0);
      } else {
#ifndef NO_LIBC
        /*
         * TODO(lsm): abort is not nice in a library, remove it
         * Also, ESP8266 SDK doesn't have it
         */
        abort();
#endif
      }
    }
  }

    /* Zero-terminate the result */
    if (buf_size > 0) {
        buf[i < (int) buf_size ? i : (int) buf_size - 1] = '\0';
    }

    return i;
}
#endif

int c_snprintf(char *buf, size_t buf_size, const char *fmt, ...) WEAK;
int c_snprintf(char *buf, size_t buf_size, const char *fmt, ...) 
{
    int result;
    va_list ap;
    va_start(ap, fmt);
    result = c_vsnprintf(buf, buf_size, fmt, ap);
    va_end(ap);
    return result;
}


/* The simplest O(mn) algorithm. Better implementation are GPLed */
const char *c_strnstr(const char *s, const char *find, size_t slen) WEAK;
const char *c_strnstr(const char *s, const char *find, size_t slen) 
{
    size_t find_length = strlen(find);
    size_t i;

    for (i = 0; i < slen; i++) {
        if (i + find_length > slen) {
            return NULL;
        }

        if (strncmp(&s[i], find, find_length) == 0) {
            return &s[i];
        }
    }

    return NULL;
}

#if CS_ENABLE_STRDUP
char *strdup(const char *src) WEAK;
char *strdup(const char *src) 
{
    size_t len = strlen(src) + 1;
    char *ret = MG_MALLOC(len);
    if (ret != NULL) {
        strcpy(ret, src);
    }
    return ret;
}
#endif

void cs_to_hex(char *to, const unsigned char *p, size_t len) WEAK;
void cs_to_hex(char *to, const unsigned char *p, size_t len) 
{
    static const char *hex = "0123456789abcdef";

    for (; len--; p++) {
        *to++ = hex[p[0] >> 4];
        *to++ = hex[p[0] & 0x0f];
    }
    *to = '\0';
}

static int fourbit(int ch) 
{
    if (ch >= '0' && ch <= '9') {
        return ch - '0';
    } else if (ch >= 'a' && ch <= 'f') {
        return ch - 'a' + 10;
    } else if (ch >= 'A' && ch <= 'F') {
        return ch - 'A' + 10;
    }
    return 0;
}

void cs_from_hex(char *to, const char *p, size_t len) WEAK;
void cs_from_hex(char *to, const char *p, size_t len) 
{
    size_t i;

    for (i = 0; i < len; i += 2) {
        *to++ = (fourbit(p[i]) << 4) + fourbit(p[i + 1]);
    }
    *to = '\0';
}

#if CS_ENABLE_TO64
int64_t cs_to64(const char *s) WEAK;
int64_t cs_to64(const char *s) 
{
    int64_t result = 0;
    int64_t neg = 1;
    while (*s && isspace((unsigned char) *s)) s++;
    if (*s == '-') {
        neg = -1;
        s++;
    }
    while (isdigit((unsigned char) *s)) {
        result *= 10;
        result += (*s - '0');
        s++;
    }
    return result * neg;
}
#endif

static int str_util_lowercase(const char *s) 
{
    return tolower(*(const unsigned char *) s);
}

int mg_ncasecmp(const char *s1, const char *s2, size_t len) WEAK;
int mg_ncasecmp(const char *s1, const char *s2, size_t len) 
{
    int diff = 0;

    if (len > 0) 
        do {
            diff = str_util_lowercase(s1++) - str_util_lowercase(s2++);
        } while (diff == 0 && s1[-1] != '\0' && --len > 0);

    return diff;
}

int mg_casecmp(const char *s1, const char *s2) WEAK;
int mg_casecmp(const char *s1, const char *s2) 
{
    return mg_ncasecmp(s1, s2, (size_t) ~0);
}

int mg_asprintf(char **buf, size_t size, const char *fmt, ...) WEAK;
int mg_asprintf(char **buf, size_t size, const char *fmt, ...) 
{
    int ret;
    va_list ap;
    va_start(ap, fmt);
    ret = mg_avprintf(buf, size, fmt, ap);
    va_end(ap);
    return ret;
}

int mg_avprintf(char **buf, size_t size, const char *fmt, va_list ap) WEAK;
int mg_avprintf(char **buf, size_t size, const char *fmt, va_list ap) 
{
    va_list ap_copy;
    int len;

    va_copy(ap_copy, ap);
    len = vsnprintf(*buf, size, fmt, ap_copy);
    va_end(ap_copy);

    if (len < 0) {
        /* eCos and Windows are not standard-compliant and return -1 when
        * the buffer is too small. Keep allocating larger buffers until we
        * succeed or out of memory. */
        *buf = NULL; /* LCOV_EXCL_START */
        while (len < 0) {
            MG_FREE(*buf);
            if (size == 0) {
                size = 5;
            }
            size *= 2;
            if ((*buf = (char *) MG_MALLOC(size)) == NULL) {
                len = -1;
                break;
            }
      
            va_copy(ap_copy, ap);
            len = vsnprintf(*buf, size - 1, fmt, ap_copy);
            va_end(ap_copy);
        }

        /*
        * Microsoft version of vsnprintf() is not always null-terminated, so put
        * the terminator manually
        */
        (*buf)[len] = 0;
        /* LCOV_EXCL_STOP */
    } else if (len >= (int) size) {
        /* Standard-compliant code path. Allocate a buffer that is large enough. */
        if ((*buf = (char *) MG_MALLOC(len + 1)) == NULL) {
            len = -1; /* LCOV_EXCL_LINE */
        } else {    /* LCOV_EXCL_LINE */
            va_copy(ap_copy, ap);
            len = vsnprintf(*buf, len + 1, fmt, ap_copy);
            va_end(ap_copy);
        }
    }

    return len;
}

const char *mg_next_comma_list_entry(const char *, struct mg_str *,
                                     struct mg_str *) WEAK;
const char *mg_next_comma_list_entry(const char *list, 
                                     struct mg_str *val,
                                     struct mg_str *eq_val) 
{
    struct mg_str ret = mg_next_comma_list_entry_n(mg_mk_str(list), val, eq_val);
    return ret.p;
}

struct mg_str mg_next_comma_list_entry_n(struct mg_str list, struct mg_str *val,
                                         struct mg_str *eq_val) WEAK;
struct mg_str mg_next_comma_list_entry_n(struct mg_str list, 
                                         struct mg_str *val,
                                         struct mg_str *eq_val) 
{
    if (list.len == 0) {
        /* End of the list */
        list = mg_mk_str(NULL);
    } else {
        const char *chr = NULL;
        *val = list;

        if ((chr = mg_strchr(*val, ',')) != NULL) {
            /* Comma found. Store length and shift the list ptr */
            val->len = chr - val->p;
            chr++;
            list.len -= (chr - list.p);
            list.p = chr;
        } else {
            /* This value is the last one */
            list = mg_mk_str_n(list.p + list.len, 0);
        }

        if (eq_val != NULL) {
            /* Value has form "x=y", adjust pointers and lengths */
            /* so that val points to "x", and eq_val points to "y". */
            eq_val->len = 0;
            eq_val->p = (const char *) memchr(val->p, '=', val->len);
            if (eq_val->p != NULL) {
                eq_val->p++; /* Skip over '=' character */
                eq_val->len = val->p + val->len - eq_val->p;
                val->len = (eq_val->p - val->p) - 1;
            }
        }
    }

    return list;
}

size_t mg_match_prefix_n(const struct mg_str, const struct mg_str) WEAK;
size_t mg_match_prefix_n(const struct mg_str pattern, const struct mg_str str) 
{
    const char *or_str;
    size_t res = 0, len = 0, i = 0, j = 0;

    if ((or_str = (const char *) memchr(pattern.p, '|', pattern.len)) != NULL ||
        (or_str = (const char *) memchr(pattern.p, ',', pattern.len)) != NULL) {
        struct mg_str pstr = {pattern.p, (size_t)(or_str - pattern.p)};
        res = mg_match_prefix_n(pstr, str);
        if (res > 0) return res;
        pstr.p = or_str + 1;
        pstr.len = (pattern.p + pattern.len) - (or_str + 1);
        return mg_match_prefix_n(pstr, str);
    }

    for (; i < pattern.len && j < str.len; i++, j++) {
        if (pattern.p[i] == '?') {
            continue;
        } else if (pattern.p[i] == '*') {
            i++;
            if (i < pattern.len && pattern.p[i] == '*') {
                i++;
                len = str.len - j;
            } else {
                len = 0;
                while (j + len < str.len && str.p[j + len] != '/') len++;
            }
            if (i == pattern.len || (pattern.p[i] == '$' && i == pattern.len - 1))
                return j + len;
            do {
                const struct mg_str pstr = {pattern.p + i, pattern.len - i};
                const struct mg_str sstr = {str.p + j + len, str.len - j - len};
                res = mg_match_prefix_n(pstr, sstr);
            } while (res == 0 && len != 0 && len-- > 0);
            return res == 0 ? 0 : j + res + len;
        } else if (str_util_lowercase(&pattern.p[i]) != str_util_lowercase(&str.p[j])) {
            break;
        }
    }
    if (i < pattern.len && pattern.p[i] == '$') {
        return j == str.len ? str.len : 0;
    }
    return i == pattern.len ? j : 0;
}

size_t mg_match_prefix(const char *, int, const char *) WEAK;
size_t mg_match_prefix(const char *pattern, int pattern_len, const char *str) 
{
    const struct mg_str pstr = {pattern, (size_t) pattern_len};
    struct mg_str s = {str, 0};
    if (str != NULL) s.len = strlen(str);
    return mg_match_prefix_n(pstr, s);
}

#endif /* EXCLUDE_COMMON */

#define MG_MAX_HOST_LEN 200

#ifndef MG_TCP_IO_SIZE
#define MG_TCP_IO_SIZE 1460
#endif
#ifndef MG_UDP_IO_SIZE
#define MG_UDP_IO_SIZE 1460
#endif

#define MG_COPY_COMMON_CONNECTION_OPTIONS(dst, src) \
  memcpy(dst, src, sizeof(*dst));

/* Which flags can be pre-set by the user at connection creation time. */
#define _MG_ALLOWED_CONNECT_FLAGS_MASK                                   \
  (MG_F_USER_1 | MG_F_USER_2 | MG_F_USER_3 | MG_F_USER_4 | MG_F_USER_5 | \
   MG_F_USER_6 | MG_F_WEBSOCKET_NO_DEFRAG | MG_F_ENABLE_BROADCAST)
/* Which flags should be modifiable by user's callbacks. */
#define _MG_CALLBACK_MODIFIABLE_FLAGS_MASK                               \
  (MG_F_USER_1 | MG_F_USER_2 | MG_F_USER_3 | MG_F_USER_4 | MG_F_USER_5 | \
   MG_F_USER_6 | MG_F_WEBSOCKET_NO_DEFRAG | MG_F_SEND_AND_CLOSE |        \
   MG_F_CLOSE_IMMEDIATELY | MG_F_IS_WEBSOCKET | MG_F_DELETE_CHUNK)

#ifndef intptr_t
#define intptr_t long
#endif


MG_INTERNAL void mg_add_conn(struct mg_mgr *mgr, struct mg_connection *c) 
{
    LOGDBG(TAG, "--> Add connection: mgr[%p], connection[%p]", mgr, c);
    c->mgr = mgr;
    c->next = mgr->active_connections;
    mgr->active_connections = c;
    c->prev = NULL;
    
    if (c->next != NULL) 
        c->next->prev = c;
    
    if (c->sock != INVALID_SOCKET) {
        c->iface->vtable->add_conn(c);
    }
}

MG_INTERNAL void mg_remove_conn(struct mg_connection *conn) 
{
    if (conn->prev == NULL) 
        conn->mgr->active_connections = conn->next;
    
    if (conn->prev) 
        conn->prev->next = conn->next;
    
    if (conn->next) 
        conn->next->prev = conn->prev;
    
    conn->prev = conn->next = NULL;
    conn->iface->vtable->remove_conn(conn);
}

MG_INTERNAL void mg_call(struct mg_connection *nc,
                         mg_event_handler_t ev_handler, 
                         void *user_data, 
                         int ev,
                         void *ev_data) 
{
    /** 协议handler存在调用协议handler,否则调用用户handler */
    if (ev_handler == NULL) {
        ev_handler = nc->proto_handler ? nc->proto_handler : nc->handler;
    }
  
    if (ev != MG_EV_POLL) {
        DBG(("%p %s ev=%d ev_data=%p flags=0x%lx rmbl=%d smbl=%d", nc,
         ev_handler == nc->handler ? "user" : "proto", ev, ev_data, nc->flags,
         (int) nc->recv_mbuf.len, (int) nc->send_mbuf.len));
    }

#if !defined(NO_LIBC) && MG_ENABLE_HEXDUMP
    if (nc->mgr->hexdump_file != NULL && ev != MG_EV_POLL && ev != MG_EV_RECV &&
              ev != MG_EV_SEND /* handled separately */) {
        mg_hexdump_connection(nc, nc->mgr->hexdump_file, NULL, 0, ev);
    }
#endif

    if (ev_handler != NULL) {
        unsigned long flags_before = nc->flags;
        ev_handler(nc, ev, ev_data MG_UD_ARG(user_data));
        
        /* Prevent user handler from fiddling with system flags. */
        if (ev_handler == nc->handler && nc->flags != flags_before) {
            nc->flags = (flags_before & ~_MG_CALLBACK_MODIFIABLE_FLAGS_MASK) |
                  (nc->flags & _MG_CALLBACK_MODIFIABLE_FLAGS_MASK);
        }
    }
  
    if (ev != MG_EV_POLL) 
        nc->mgr->num_calls++;
    
    if (ev != MG_EV_POLL) {
        DBG(("%p after %s flags=0x%lx rmbl=%d smbl=%d", nc,
         ev_handler == nc->handler ? "user" : "proto", nc->flags,
         (int) nc->recv_mbuf.len, (int) nc->send_mbuf.len));
    }

#if !MG_ENABLE_CALLBACK_USERDATA
    (void) user_data;
#endif
}


MG_INTERNAL void mg_timer(struct mg_connection *c, double now) 
{
    if (c->ev_timer_time > 0 && now >= c->ev_timer_time) {
        double old_value = c->ev_timer_time;
        c->ev_timer_time = 0;
        mg_call(c, NULL, c->user_data, MG_EV_TIMER, &old_value);
    }
}


MG_INTERNAL size_t recv_avail_size(struct mg_connection *conn, size_t max) 
{
    size_t avail;
    if (conn->recv_mbuf_limit < conn->recv_mbuf.len) return 0;
    avail = conn->recv_mbuf_limit - conn->recv_mbuf.len;
    return avail > max ? max : avail;
}

static int mg_do_recv(struct mg_connection *nc);



int mg_if_poll(struct mg_connection *nc, double now) 
{
    if ((nc->flags & MG_F_CLOSE_IMMEDIATELY) || 
        (nc->send_mbuf.len == 0 && (nc->flags & MG_F_SEND_AND_CLOSE))) {
        
        LOGDBG(TAG, "--> close connection[%p] now", nc);
        mg_close_conn(nc);
        return 0;
    }

#if MG_ENABLE_SSL
    if ((nc->flags & (MG_F_SSL | MG_F_LISTENING | MG_F_CONNECTING)) == MG_F_SSL) {
        /* SSL library may have data to be delivered to the app in its buffers,
         * drain them. */
        int recved = 0;
        do {
            if (nc->flags & (MG_F_WANT_READ | MG_F_WANT_WRITE)) break;
            if (recv_avail_size(nc, MG_TCP_IO_SIZE) <= 0) break;
            recved = mg_do_recv(nc);
        } while (recved > 0);
    }
#endif /* MG_ENABLE_SSL */

    mg_timer(nc, now);

    {
        time_t now_t = (time_t) now;
        mg_call(nc, NULL, nc->user_data, MG_EV_POLL, &now_t);
    }

    return 1;
}

void mg_destroy_conn(struct mg_connection *conn, int destroy_if) 
{
    if (conn->sock != INVALID_SOCKET) { /* Don't print timer-only conns */
        LOG(LL_DEBUG, ("%p 0x%lx %d", conn, conn->flags, destroy_if));
    }

    if (destroy_if) 
        conn->iface->vtable->destroy_conn(conn);

    if (conn->proto_data != NULL && conn->proto_data_destructor != NULL) {
        conn->proto_data_destructor(conn->proto_data);
    }

#if MG_ENABLE_SSL
    mg_ssl_if_conn_free(conn);
#endif

    mbuf_free(&conn->recv_mbuf);
    mbuf_free(&conn->send_mbuf);

    memset(conn, 0, sizeof(*conn));
    MG_FREE(conn);
}


void mg_close_conn(struct mg_connection *conn) 
{
#if MG_ENABLE_SSL
    if (conn->flags & MG_F_SSL_HANDSHAKE_DONE) {
        mg_ssl_if_conn_close_notify(conn);
    }
#endif

    /*
     * Clearly mark the connection as going away (if not already).
     * Some net_if impls (LwIP) need this for cleanly handling half-dead conns.
     */
    conn->flags |= MG_F_CLOSE_IMMEDIATELY;
    mg_remove_conn(conn);
    conn->iface->vtable->destroy_conn(conn);
    mg_call(conn, NULL, conn->user_data, MG_EV_CLOSE, NULL);
    mg_destroy_conn(conn, 0 /* destroy_if */);
}


void mg_mgr_init(struct mg_mgr *m, void *user_data) 
{
    struct mg_mgr_init_opts opts;
    memset(&opts, 0, sizeof(opts));
    mg_mgr_init_opt(m, user_data, opts);
}

void mg_mgr_init_opt(struct mg_mgr *m, void *user_data, struct mg_mgr_init_opts opts) 
{
    memset(m, 0, sizeof(*m));

#if MG_ENABLE_BROADCAST
    m->ctl[0] = m->ctl[1] = INVALID_SOCKET;
#endif
    m->user_data = user_data;

    /* 
     * 忽略SIGPIPE信号,防止客户端取消请求时,整个服务器进程被KILL
     */
    signal(SIGPIPE, SIG_IGN);

    {
        int i;
        if (opts.num_ifaces == 0) {
            opts.num_ifaces = mg_num_ifaces;
            opts.ifaces = mg_ifaces;
        }

        if (opts.main_iface != NULL) {
            opts.ifaces[MG_MAIN_IFACE] = opts.main_iface;
        }
        
        m->num_ifaces = opts.num_ifaces;
        m->ifaces = (struct mg_iface **) MG_MALLOC(sizeof(*m->ifaces) * opts.num_ifaces);
        for (i = 0; i < opts.num_ifaces; i++) {
            m->ifaces[i] = mg_if_create_iface(opts.ifaces[i], m);
            m->ifaces[i]->vtable->init(m->ifaces[i]);
        }
    }
    
    if (opts.nameserver != NULL) {
        m->nameserver = strdup(opts.nameserver);
    }
  
    DBG(("=================================="));
    DBG(("init mgr=%p", m));

#if MG_ENABLE_SSL
    {
        static int init_done;
        if (!init_done) {
            mg_ssl_if_init();
            init_done++;
        }
    }
#endif
}

void mg_mgr_free(struct mg_mgr *m) 
{
    struct mg_connection *conn, *tmp_conn;

    LOGDBG(TAG, "--> free mgr here...");

    if (m == NULL) return;
    
    /* Do one last poll, see https://github.com/cesanta/mongoose/issues/286 */
    mg_mgr_poll(m, 0);

#if MG_ENABLE_BROADCAST
    if (m->ctl[0] != INVALID_SOCKET) closesocket(m->ctl[0]);
    if (m->ctl[1] != INVALID_SOCKET) closesocket(m->ctl[1]);
    m->ctl[0] = m->ctl[1] = INVALID_SOCKET;
#endif

    for (conn = m->active_connections; conn != NULL; conn = tmp_conn) {
        tmp_conn = conn->next;
        mg_close_conn(conn);
    }

    {
        int i;
        for (i = 0; i < m->num_ifaces; i++) {
            m->ifaces[i]->vtable->free(m->ifaces[i]);
            MG_FREE(m->ifaces[i]);
        }
        MG_FREE(m->ifaces);
    }

    MG_FREE((char *) m->nameserver);
}

int mg_mgr_poll(struct mg_mgr *m, int timeout_ms) 
{
    int i, num_calls_before = m->num_calls;

    for (i = 0; i < m->num_ifaces; i++) {
        m->ifaces[i]->vtable->poll(m->ifaces[i], timeout_ms);
    }

    return (m->num_calls - num_calls_before);
}

int mg_vprintf(struct mg_connection *nc, const char *fmt, va_list ap) 
{
    char mem[MG_VPRINTF_BUFFER_SIZE], *buf = mem;
    int len;

    if ((len = mg_avprintf(&buf, sizeof(mem), fmt, ap)) > 0) {
        mg_send(nc, buf, len);
    }
    if (buf != mem && buf != NULL) {
        MG_FREE(buf); /* LCOV_EXCL_LINE */
    }               /* LCOV_EXCL_LINE */

    return len;
}

int mg_printf(struct mg_connection *conn, const char *fmt, ...) 
{
    int len;
    va_list ap;
    va_start(ap, fmt);
    len = mg_vprintf(conn, fmt, ap);
    va_end(ap);
    return len;
}



MG_INTERNAL struct mg_connection *mg_create_connection_base(struct mg_mgr *mgr, 
                                                            mg_event_handler_t callback,
                                                            struct mg_add_sock_opts opts) 
{
    struct mg_connection *conn;

    if ((conn = (struct mg_connection *) MG_CALLOC(1, sizeof(*conn))) != NULL) {
        conn->sock = INVALID_SOCKET;
        conn->handler = callback;
        conn->mgr = mgr;
        conn->last_io_time = (time_t) mg_time();
        conn->iface = (opts.iface != NULL ? opts.iface : mgr->ifaces[MG_MAIN_IFACE]);
        conn->flags = opts.flags & _MG_ALLOWED_CONNECT_FLAGS_MASK;
        conn->user_data = opts.user_data;
    
        /*
         * SIZE_MAX is defined as a long long constant in
         * system headers on some platforms and so it
         * doesn't compile with pedantic ansi flags.
         */
        conn->recv_mbuf_limit = ~0;
    } else {
        MG_SET_PTRPTR(opts.error_string, "failed to create connection");
    }

    return conn;
}

MG_INTERNAL struct mg_connection *mg_create_connection(struct mg_mgr *mgr, 
                                                        mg_event_handler_t callback,
                                                        struct mg_add_sock_opts opts) 
{
    struct mg_connection *conn = mg_create_connection_base(mgr, callback, opts);

    if (conn != NULL && !conn->iface->vtable->create_conn(conn)) {
        MG_FREE(conn);
        conn = NULL;
    }
    if (conn == NULL) {
        MG_SET_PTRPTR(opts.error_string, "failed to init connection");
    }

    return conn;
}


/*
 * Address format: [PROTO://][HOST]:PORT
 *
 * HOST could be IPv4/IPv6 address or a host name.
 * `host` is a destination buffer to hold parsed HOST part. Should be at least
 * MG_MAX_HOST_LEN bytes long.
 * `proto` is a returned socket type, either SOCK_STREAM or SOCK_DGRAM
 *
 * Return:
 *   -1   on parse error
 *    0   if HOST needs DNS lookup
 *   >0   length of the address string
 */
MG_INTERNAL int mg_parse_address(const char *str, union socket_address *sa,
                                 int *proto, char *host, size_t host_len) 
{
    unsigned int a, b, c, d, port = 0;
    int ch, len = 0;
#if MG_ENABLE_IPV6
    char buf[100];
#endif

    /*
    * MacOS needs that. If we do not zero it, subsequent bind() will fail.
    * Also, all-zeroes in the socket address means binding to all addresses
    * for both IPv4 and IPv6 (INADDR_ANY and IN6ADDR_ANY_INIT).
    */
    memset(sa, 0, sizeof(*sa));
    sa->sin.sin_family = AF_INET;

    *proto = SOCK_STREAM;

    if (strncmp(str, "udp://", 6) == 0) {
        str += 6;
        *proto = SOCK_DGRAM;
    } else if (strncmp(str, "tcp://", 6) == 0) {
        str += 6;
    }

    if (sscanf(str, "%u.%u.%u.%u:%u%n", &a, &b, &c, &d, &port, &len) == 5) {
        /* Bind to a specific IPv4 address, e.g. 192.168.1.5:8080 */
        sa->sin.sin_addr.s_addr =
            htonl(((uint32_t) a << 24) | ((uint32_t) b << 16) | c << 8 | d);
        sa->sin.sin_port = htons((uint16_t) port);
#if MG_ENABLE_IPV6
    } else if (sscanf(str, "[%99[^]]]:%u%n", buf, &port, &len) == 2 &&
             inet_pton(AF_INET6, buf, &sa->sin6.sin6_addr)) {
        /* IPv6 address, e.g. [3ffe:2a00:100:7031::1]:8080 */
        sa->sin6.sin6_family = AF_INET6;
        sa->sin.sin_port = htons((uint16_t) port);
#endif
#if MG_ENABLE_ASYNC_RESOLVER
    } else if (strlen(str) < host_len &&
             sscanf(str, "%[^ :]:%u%n", host, &port, &len) == 2) {
        sa->sin.sin_port = htons((uint16_t) port);
        if (mg_resolve_from_hosts_file(host, sa) != 0) {
            /*
            * if resolving from hosts file failed and the host
            * we are trying to resolve is `localhost` - we should
            * try to resolve it using `gethostbyname` and do not try
            * to resolve it via DNS server if gethostbyname has failed too
            */
            if (mg_ncasecmp(host, "localhost", 9) != 0) {
                return 0;
            }

#if MG_ENABLE_SYNC_RESOLVER
            if (!mg_resolve2(host, &sa->sin.sin_addr)) {
                return -1;
            }
#else   
            return -1;
#endif
        }
#endif
    } else if (sscanf(str, ":%u%n", &port, &len) == 1 ||
             sscanf(str, "%u%n", &port, &len) == 1) {
        /* If only port is specified, bind to IPv4, INADDR_ANY */
        sa->sin.sin_port = htons((uint16_t) port);
    } else {
        return -1;
    }

    /* Required for MG_ENABLE_ASYNC_RESOLVER=0 */
    (void) host;
    (void) host_len;

    ch = str[len]; /* Character that follows the address */
    return port < 0xffffUL && (ch == '\0' || ch == ',' || isspace(ch)) ? len : -1;
}

#if MG_ENABLE_SSL
MG_INTERNAL void mg_ssl_handshake(struct mg_connection *nc) 
{
    int err = 0;
    int server_side = (nc->listener != NULL);
    enum mg_ssl_if_result res;
    if (nc->flags & MG_F_SSL_HANDSHAKE_DONE) return;
    res = mg_ssl_if_handshake(nc);

    if (res == MG_SSL_OK) {
        nc->flags |= MG_F_SSL_HANDSHAKE_DONE;
        nc->flags &= ~(MG_F_WANT_READ | MG_F_WANT_WRITE);
        if (server_side) {
            mg_call(nc, NULL, nc->user_data, MG_EV_ACCEPT, &nc->sa);
        } else {
            mg_call(nc, NULL, nc->user_data, MG_EV_CONNECT, &err);
        }
    } else if (res == MG_SSL_WANT_READ) {
        nc->flags |= MG_F_WANT_READ;
    } else if (res == MG_SSL_WANT_WRITE) {
        nc->flags |= MG_F_WANT_WRITE;
    } else {
        if (!server_side) {
            err = res;
            mg_call(nc, NULL, nc->user_data, MG_EV_CONNECT, &err);
        }
        nc->flags |= MG_F_CLOSE_IMMEDIATELY;
    }
}
#endif /* MG_ENABLE_SSL */



struct mg_connection *mg_if_accept_new_conn(struct mg_connection *lc) 
{
    struct mg_add_sock_opts opts;
    struct mg_connection *nc;
    memset(&opts, 0, sizeof(opts));
    nc = mg_create_connection(lc->mgr, lc->handler, opts);
    
    if (nc == NULL) 
        return NULL;

    nc->listener = lc;
    nc->proto_handler = lc->proto_handler;
    nc->user_data = lc->user_data;
    nc->recv_mbuf_limit = lc->recv_mbuf_limit;
    nc->iface = lc->iface;
  
    if (lc->flags & MG_F_SSL) 
        nc->flags |= MG_F_SSL;

    mg_add_conn(nc->mgr, nc);
    LOG(LL_DEBUG, ("%p %p %d %d", lc, nc, nc->sock, (int) nc->flags));
    return nc;
}

void mg_if_accept_tcp_cb(struct mg_connection *nc, union socket_address *sa, size_t sa_len) 
{
    LOG(LL_DEBUG, ("%p %s://%s:%hu", nc, (nc->flags & MG_F_UDP ? "udp" : "tcp"),
                 inet_ntoa(sa->sin.sin_addr), ntohs(sa->sin.sin_port)));
    nc->sa = *sa;

#if MG_ENABLE_SSL
    if (nc->listener->flags & MG_F_SSL) {
        nc->flags |= MG_F_SSL;
        if (mg_ssl_if_conn_accept(nc, nc->listener) == MG_SSL_OK) {
            mg_ssl_handshake(nc);
        } else {
            mg_close_conn(nc);
        }
    } else
#endif
    {
        mg_call(nc, NULL, nc->user_data, MG_EV_ACCEPT, &nc->sa);
    }
    (void) sa_len;
}

void mg_send(struct mg_connection *nc, const void *buf, int len) 
{

#ifdef ENABLE_USE_BUFFER_LOCK
	std::unique_lock<std::mutex> _lock(nc->send_mbuf.bufLock);    
#endif

    nc->last_io_time = (time_t) 
    mg_time();
    mbuf_append(&nc->send_mbuf, buf, len);
}


static int mg_recv_tcp(struct mg_connection *nc, char *buf, size_t len);
static int mg_recv_udp(struct mg_connection *nc, char *buf, size_t len);

static int mg_do_recv(struct mg_connection *nc) 
{
    int res = 0;
    char *buf = NULL;
    size_t len = (nc->flags & MG_F_UDP ? MG_UDP_IO_SIZE : MG_TCP_IO_SIZE);
    if ((nc->flags & (MG_F_CLOSE_IMMEDIATELY | MG_F_CONNECTING)) ||
            ((nc->flags & MG_F_LISTENING) && !(nc->flags & MG_F_UDP))) {
        return -1;
    }
  
    len = recv_avail_size(nc, len);
    if (len == 0) return -2;
    if (nc->recv_mbuf.size < nc->recv_mbuf.len + len) {
        mbuf_resize(&nc->recv_mbuf, nc->recv_mbuf.len + len);
    }
  
    buf = nc->recv_mbuf.buf + nc->recv_mbuf.len;
    len = nc->recv_mbuf.size - nc->recv_mbuf.len;
    if (nc->flags & MG_F_UDP) {
        res = mg_recv_udp(nc, buf, len);
    } else {
        res = mg_recv_tcp(nc, buf, len);
    }
    return res;
}

/* @func - mg_if_can_recv_cb
 * @desc: 接收客户端发送的数据
 * @param
 *  nc - 连接对象指针
 * @return
 *  无
 */
void mg_if_can_recv_cb(struct mg_connection *nc) 
{
    mg_do_recv(nc);
}


static int mg_recv_tcp(struct mg_connection *nc, char *buf, size_t len) 
{
    int n = 0;

#if MG_ENABLE_SSL
    if (nc->flags & MG_F_SSL) {
        if (nc->flags & MG_F_SSL_HANDSHAKE_DONE) {
            n = mg_ssl_if_read(nc, buf, len);
            DBG(("%p <- %d bytes (SSL)", nc, n));
            if (n < 0) {
                if (n == MG_SSL_WANT_READ) {
                    nc->flags |= MG_F_WANT_READ;
                    n = 0;
                } else {
                    nc->flags |= MG_F_CLOSE_IMMEDIATELY;
                }
            } else if (n > 0) {
                nc->flags &= ~MG_F_WANT_READ;
            }
        } else {
            mg_ssl_handshake(nc);
        }
    } else
#endif

    {
        n = nc->iface->vtable->tcp_recv(nc, buf, len);
        DBG(("connection obj[%p] tcp recv data <- [%d] bytes", nc, n));
    }

    if (n > 0) {
        nc->recv_mbuf.len += n;
        nc->last_io_time = (time_t) mg_time();

        #if !defined(NO_LIBC) && MG_ENABLE_HEXDUMP
        if (nc->mgr && nc->mgr->hexdump_file != NULL) {
            mg_hexdump_connection(nc, nc->mgr->hexdump_file, buf, n, MG_EV_RECV);
        }
        #endif
        
        mbuf_trim(&nc->recv_mbuf);
        mg_call(nc, NULL, nc->user_data, MG_EV_RECV, &n);
    } else if (n < 0) {
        nc->flags |= MG_F_CLOSE_IMMEDIATELY;
    }

    mbuf_trim(&nc->recv_mbuf);
    return n;
}

static int mg_recv_udp(struct mg_connection *nc, char *buf, size_t len) 
{
    int n = 0;
    struct mg_connection *lc = nc;
    union socket_address sa;
    size_t sa_len = sizeof(sa);
    n = nc->iface->vtable->udp_recv(lc, buf, len, &sa, &sa_len);
    if (n < 0) {
        lc->flags |= MG_F_CLOSE_IMMEDIATELY;
        goto out;
    }
    
    if (nc->flags & MG_F_LISTENING) {
        /*
         * Do we have an existing connection for this source?
         * This is very inefficient for long connection lists.
         */
        lc = nc;
        for (nc = mg_next(lc->mgr, NULL); nc != NULL; nc = mg_next(lc->mgr, nc)) {
            if (memcmp(&nc->sa.sa, &sa.sa, sa_len) == 0 && nc->listener == lc) {
                break;
            }
        }
        
        if (nc == NULL) {
            struct mg_add_sock_opts opts;
            memset(&opts, 0, sizeof(opts));
            
            /* Create fake connection w/out sock initialization */
            nc = mg_create_connection_base(lc->mgr, lc->handler, opts);
            if (nc != NULL) {
                nc->sock = lc->sock;
                nc->listener = lc;
                nc->sa = sa;
                nc->proto_handler = lc->proto_handler;
                nc->user_data = lc->user_data;
                nc->recv_mbuf_limit = lc->recv_mbuf_limit;
                nc->flags = MG_F_UDP;
                
                /*
                * Long-lived UDP "connections" i.e. interactions that involve more
                * than one request and response are rare, most are transactional:
                * response is sent and the "connection" is closed. Or - should be.
                * But users (including ourselves) tend to forget about that part,
                * because UDP is connectionless and one does not think about
                * processing a UDP request as handling a connection that needs to be
                * closed. Thus, we begin with SEND_AND_CLOSE flag set, which should
                * be a reasonable default for most use cases, but it is possible to
                * turn it off the connection should be kept alive after processing.
                */
                nc->flags |= MG_F_SEND_AND_CLOSE;
                mg_add_conn(lc->mgr, nc);
                mg_call(nc, NULL, nc->user_data, MG_EV_ACCEPT, &nc->sa);
            }
        }
    }
    
    if (nc != NULL) {
        DBG(("%p <- %d bytes from %s:%d", nc, n, inet_ntoa(nc->sa.sin.sin_addr), ntohs(nc->sa.sin.sin_port)));
        if (nc == lc) {
            nc->recv_mbuf.len += n;
        } else {
            mbuf_append(&nc->recv_mbuf, buf, n);
        }
    
        mbuf_trim(&lc->recv_mbuf);
        lc->last_io_time = nc->last_io_time = (time_t) mg_time();

#if !defined(NO_LIBC) && MG_ENABLE_HEXDUMP
        if (nc->mgr && nc->mgr->hexdump_file != NULL) {
            mg_hexdump_connection(nc, nc->mgr->hexdump_file, buf, n, MG_EV_RECV);
        }
#endif
        mg_call(nc, NULL, nc->user_data, MG_EV_RECV, &n);
    }

out:
    mbuf_free(&lc->recv_mbuf);
    return n;
}


void mg_if_can_send_cb(struct mg_connection *nc) 
{    
    int n = 0;

#ifdef ENABLE_USE_BUFFER_LOCK
	std::unique_lock<std::mutex> _lock(nc->send_mbuf.bufLock);    
#endif 

    const char *buf = nc->send_mbuf.buf;
    size_t len = nc->send_mbuf.len;

    if (nc->flags & (MG_F_CLOSE_IMMEDIATELY | MG_F_CONNECTING)) {
        return;
    }

    if (!(nc->flags & MG_F_UDP)) {
        if (nc->flags & MG_F_LISTENING) return;
        if (len > MG_TCP_IO_SIZE) len = MG_TCP_IO_SIZE;
    }

#if MG_ENABLE_SSL
    if (nc->flags & MG_F_SSL) {
        if (nc->flags & MG_F_SSL_HANDSHAKE_DONE) {
            if (len > 0) {
                n = mg_ssl_if_write(nc, buf, len);
                DBG(("%p -> %d bytes (SSL)", nc, n));
            }
            if (n < 0) {
                if (n == MG_SSL_WANT_WRITE) {
                    nc->flags |= MG_F_WANT_WRITE;
                    n = 0;
                } else {
                    nc->flags |= MG_F_CLOSE_IMMEDIATELY;
                }
            } else {
                nc->flags &= ~MG_F_WANT_WRITE;
            }
        } else {
            mg_ssl_handshake(nc);
        }
    } else
#endif

    if (len > 0) {
        /* 发送数据是从mbuf的头部开始发送 */
        if (nc->flags & MG_F_UDP) {
            n = nc->iface->vtable->udp_send(nc, buf, len);
        } else {
            n = nc->iface->vtable->tcp_send(nc, buf, len);
        }
        DBG(("connection obj addr[%p] success send data -> [%d] bytes", nc, n));
    }

#if !defined(NO_LIBC) && MG_ENABLE_HEXDUMP
    if (n > 0 && nc->mgr && nc->mgr->hexdump_file != NULL) {
        mg_hexdump_connection(nc, nc->mgr->hexdump_file, buf, n, MG_EV_SEND);
    }
#endif

    if (n < 0) {            /* 成功发送的字节数小于0,将立即关闭该套接字 */
        nc->flags |= MG_F_CLOSE_IMMEDIATELY;
    } else if (n > 0) {     /* 发送成功后,调整发送缓冲区指针 */
        nc->last_io_time = (time_t) mg_time();
        
        /* 将缓冲区offset为n之后的数据前移动首部 */
        mbuf_remove(&nc->send_mbuf, n);
        
        /* 重新更新真个缓冲区的大小 */
        mbuf_trim(&nc->send_mbuf);
    }

    if (n != 0) 
        mg_call(nc, NULL, nc->user_data, MG_EV_SEND, &n);
}


/*
 * Schedules an async connect for a resolved address and proto.
 * Called from two places: `mg_connect_opt()` and from async resolver.
 * When called from the async resolver, it must trigger `MG_EV_CONNECT` event
 * with a failure flag to indicate connection failure.
 */
MG_INTERNAL struct mg_connection *mg_do_connect(struct mg_connection *nc,
                                                int proto,
                                                union socket_address *sa) 
{
    LOG(LL_DEBUG, ("%p %s://%s:%hu", nc, proto == SOCK_DGRAM ? "udp" : "tcp",
                 inet_ntoa(sa->sin.sin_addr), ntohs(sa->sin.sin_port)));

    nc->flags |= MG_F_CONNECTING;
    if (proto == SOCK_DGRAM) {
        nc->iface->vtable->connect_udp(nc);
    } else {
        nc->iface->vtable->connect_tcp(nc, sa);
    }
    mg_add_conn(nc->mgr, nc);
    return nc;
}

void mg_if_connect_cb(struct mg_connection *nc, int err) 
{
    LOG(LL_DEBUG, ("%p %s://%s:%hu -> %d", nc, (nc->flags & MG_F_UDP ? "udp" : "tcp"),
       inet_ntoa(nc->sa.sin.sin_addr), ntohs(nc->sa.sin.sin_port), err));
    
    nc->flags &= ~MG_F_CONNECTING;
    if (err != 0) {
        nc->flags |= MG_F_CLOSE_IMMEDIATELY;
    }

#if MG_ENABLE_SSL
    if (err == 0 && (nc->flags & MG_F_SSL)) {
        mg_ssl_handshake(nc);
    } else
#endif
    {
        mg_call(nc, NULL, nc->user_data, MG_EV_CONNECT, &err);
    }
}




struct mg_connection *mg_connect(struct mg_mgr *mgr, const char *address,
                                 MG_CB(mg_event_handler_t callback, void *user_data)) 
{
    struct mg_connect_opts opts;
    memset(&opts, 0, sizeof(opts));
    return mg_connect_opt(mgr, address, MG_CB(callback, user_data), opts);
}

struct mg_connection *mg_connect_opt(struct mg_mgr *mgr, const char *address,
                                     MG_CB(mg_event_handler_t callback, void *user_data),
                                     struct mg_connect_opts opts) 
{
    struct mg_connection *nc = NULL;
    int proto, rc;
    struct mg_add_sock_opts add_sock_opts;
    char host[MG_MAX_HOST_LEN];

    MG_COPY_COMMON_CONNECTION_OPTIONS(&add_sock_opts, &opts);

    if ((nc = mg_create_connection(mgr, callback, add_sock_opts)) == NULL) {
        return NULL;
    }

    if ((rc = mg_parse_address(address, &nc->sa, &proto, host, sizeof(host))) < 0) {
        /* Address is malformed */
        MG_SET_PTRPTR(opts.error_string, "cannot parse address");
        mg_destroy_conn(nc, 1 /* destroy_if */);
        return NULL;
    }

    nc->flags |= opts.flags & _MG_ALLOWED_CONNECT_FLAGS_MASK;
    nc->flags |= (proto == SOCK_DGRAM) ? MG_F_UDP : 0;

#if MG_ENABLE_CALLBACK_USERDATA
    nc->user_data = user_data;
#else
    nc->user_data = opts.user_data;
#endif

#if MG_ENABLE_SSL
    LOG(LL_DEBUG,
      ("%p %s %s,%s,%s", nc, address, (opts.ssl_cert ? opts.ssl_cert : "-"),
       (opts.ssl_key ? opts.ssl_key : "-"),
       (opts.ssl_ca_cert ? opts.ssl_ca_cert : "-")));

    if (opts.ssl_cert != NULL || opts.ssl_ca_cert != NULL || opts.ssl_psk_identity != NULL) {
        const char *err_msg = NULL;
        struct mg_ssl_if_conn_params params;
        if (nc->flags & MG_F_UDP) {
            MG_SET_PTRPTR(opts.error_string, "SSL for UDP is not supported");
            mg_destroy_conn(nc, 1 /* destroy_if */);
            return NULL;
        }
        
        memset(&params, 0, sizeof(params));
        params.cert = opts.ssl_cert;
        params.key = opts.ssl_key;
        params.ca_cert = opts.ssl_ca_cert;
        params.cipher_suites = opts.ssl_cipher_suites;
        params.psk_identity = opts.ssl_psk_identity;
        params.psk_key = opts.ssl_psk_key;
        if (opts.ssl_ca_cert != NULL) {
            if (opts.ssl_server_name != NULL) {
                if (strcmp(opts.ssl_server_name, "*") != 0) {
                    params.server_name = opts.ssl_server_name;
                }
            } else if (rc == 0) { /* If it's a DNS name, use host. */
                params.server_name = host;
            }
        }
      
        if (mg_ssl_if_conn_init(nc, &params, &err_msg) != MG_SSL_OK) {
            MG_SET_PTRPTR(opts.error_string, err_msg);
            mg_destroy_conn(nc, 1 /* destroy_if */);
            return NULL;
        }
        nc->flags |= MG_F_SSL;
    }
#endif /* MG_ENABLE_SSL */

    if (rc == 0) {
#if MG_ENABLE_ASYNC_RESOLVER
        /*
        * DNS resolution is required for host.
        * mg_parse_address() fills port in nc->sa, which we pass to resolve_cb()
        */
        struct mg_connection *dns_conn = NULL;
        struct mg_resolve_async_opts o;
        memset(&o, 0, sizeof(o));
        o.dns_conn = &dns_conn;
        o.nameserver = opts.nameserver;
        if (mg_resolve_async_opt(nc->mgr, host, MG_DNS_A_RECORD, resolve_cb, nc,
                                o) != 0) {
          MG_SET_PTRPTR(opts.error_string, "cannot schedule DNS lookup");
          mg_destroy_conn(nc, 1 /* destroy_if */);
          return NULL;
        }
        nc->priv_2 = dns_conn;
        nc->flags |= MG_F_RESOLVING;
        return nc;
#else
        MG_SET_PTRPTR(opts.error_string, "Resolver is disabled");
        mg_destroy_conn(nc, 1 /* destroy_if */);
        return NULL;
#endif
    } else {
        /* Address is parsed and resolved to IP. proceed with connect() */
        return mg_do_connect(nc, proto, &nc->sa);
    }
}

struct mg_connection *mg_bind(struct mg_mgr *srv, const char *address,
                              MG_CB(mg_event_handler_t event_handler, void *user_data)) 
{
    struct mg_bind_opts opts;
    memset(&opts, 0, sizeof(opts));
    return mg_bind_opt(srv, address, MG_CB(event_handler, user_data), opts);
}

struct mg_connection *mg_bind_opt(struct mg_mgr *mgr, const char *address,
                                  MG_CB(mg_event_handler_t callback, void *user_data),
                                  struct mg_bind_opts opts) 
{
    union socket_address sa;
    struct mg_connection *nc = NULL;
    int proto, rc;
    struct mg_add_sock_opts add_sock_opts;
    char host[MG_MAX_HOST_LEN];

#if MG_ENABLE_CALLBACK_USERDATA
    opts.user_data = user_data;
#endif

    if (callback == NULL) {
        MG_SET_PTRPTR(opts.error_string, "handler is required");
        return NULL;
    }

    MG_COPY_COMMON_CONNECTION_OPTIONS(&add_sock_opts, &opts);

    if (mg_parse_address(address, &sa, &proto, host, sizeof(host)) <= 0) {
        MG_SET_PTRPTR(opts.error_string, "cannot parse address");
        return NULL;
    }

    /*
     * 创建第一个连接对象,该连接用于监听客户端的连接
     */
    nc = mg_create_connection(mgr, callback, add_sock_opts);
    if (nc == NULL) {
        return NULL;
    }
    LOGDBG(TAG, "mg_bind_opt: create first connection[%p]", nc);


    nc->sa = sa;
    nc->flags |= MG_F_LISTENING;
    if (proto == SOCK_DGRAM) 
        nc->flags |= MG_F_UDP;

#if MG_ENABLE_SSL
    DBG(("%p %s %s,%s,%s", nc, address, (opts.ssl_cert ? opts.ssl_cert : "-"),
       (opts.ssl_key ? opts.ssl_key : "-"),
       (opts.ssl_ca_cert ? opts.ssl_ca_cert : "-")));

    if (opts.ssl_cert != NULL || opts.ssl_ca_cert != NULL) {
        const char *err_msg = NULL;
        struct mg_ssl_if_conn_params params;
        if (nc->flags & MG_F_UDP) {
            MG_SET_PTRPTR(opts.error_string, "SSL for UDP is not supported");
            mg_destroy_conn(nc, 1 /* destroy_if */);
            return NULL;
        }
        memset(&params, 0, sizeof(params));
        params.cert = opts.ssl_cert;
        params.key = opts.ssl_key;
        params.ca_cert = opts.ssl_ca_cert;
        params.cipher_suites = opts.ssl_cipher_suites;
        if (mg_ssl_if_conn_init(nc, &params, &err_msg) != MG_SSL_OK) {
            MG_SET_PTRPTR(opts.error_string, err_msg);
            mg_destroy_conn(nc, 1 /* destroy_if */);
            return NULL;
        }
        nc->flags |= MG_F_SSL;
    }
#endif /* MG_ENABLE_SSL */

    if (nc->flags & MG_F_UDP) {
        rc = nc->iface->vtable->listen_udp(nc, &nc->sa);
    } else {
        rc = nc->iface->vtable->listen_tcp(nc, &nc->sa);
    }
    
    if (rc != 0) {
        DBG(("Failed to open listener: %d", rc));
        MG_SET_PTRPTR(opts.error_string, "failed to open listener");
        mg_destroy_conn(nc, 1 /* destroy_if */);
        return NULL;
    }

    LOGDBG(TAG, "mg_bind_opt: add connection[%p] to mgr", nc);
    mg_add_conn(nc->mgr, nc);
    return nc;
}

struct mg_connection *mg_next(struct mg_mgr *s, struct mg_connection *conn) 
{
    return conn == NULL ? s->active_connections : conn->next;
}

#if MG_ENABLE_BROADCAST
void mg_broadcast(struct mg_mgr *mgr, mg_event_handler_t cb, void *data, size_t len) 
{
    struct ctl_msg ctl_msg;

    /*
    * Mongoose manager has a socketpair, `struct mg_mgr::ctl`,
    * where `mg_broadcast()` pushes the message.
    * `mg_mgr_poll()` wakes up, reads a message from the socket pair, and calls
    * specified callback for each connection. Thus the callback function executes
    * in event manager thread.
    */
    if (mgr->ctl[0] != INVALID_SOCKET && data != NULL && len < sizeof(ctl_msg.message)) {
        size_t dummy;

        ctl_msg.callback = cb;
        memcpy(ctl_msg.message, data, len);
        dummy = MG_SEND_FUNC(mgr->ctl[0], (char *) &ctl_msg, offsetof(struct ctl_msg, message) + len, 0);
        dummy = MG_RECV_FUNC(mgr->ctl[0], (char *) &len, 1, 0);
        (void) dummy; /* https://gcc.gnu.org/bugzilla/show_bug.cgi?id=25509 */
    }
}
#endif /* MG_ENABLE_BROADCAST */

static int isbyte(int n) 
{
    return n >= 0 && n <= 255;
}

static int parse_net(const char *spec, uint32_t *net, uint32_t *mask) 
{
    int n, a, b, c, d, slash = 32, len = 0;

    if ((sscanf(spec, "%d.%d.%d.%d/%d%n", &a, &b, &c, &d, &slash, &n) == 5 ||
       sscanf(spec, "%d.%d.%d.%d%n", &a, &b, &c, &d, &n) == 4) &&
      isbyte(a) && isbyte(b) && isbyte(c) && isbyte(d) && slash >= 0 &&
      slash < 33) {
        len = n;
        *net = ((uint32_t) a << 24) | ((uint32_t) b << 16) | ((uint32_t) c << 8) | d;
        *mask = slash ? 0xffffffffU << (32 - slash) : 0;
    }

    return len;
}

int mg_check_ip_acl(const char *acl, uint32_t remote_ip) 
{
    int allowed, flag;
    uint32_t net, mask;
    struct mg_str vec;

      /* If any ACL is set, deny by default */
    allowed = (acl == NULL || *acl == '\0') ? '+' : '-';

    while ((acl = mg_next_comma_list_entry(acl, &vec, NULL)) != NULL) {
        flag = vec.p[0];
        if ((flag != '+' && flag != '-') || parse_net(&vec.p[1], &net, &mask) == 0) {
            return -1;
        }

        if (net == (remote_ip & mask)) {
            allowed = flag;
        }
    }

    DBG(("%08x %c", (unsigned int) remote_ip, allowed));
    return allowed == '+';
}

/* Move data from one connection to another */
void mg_forward(struct mg_connection *from, struct mg_connection *to) 
{
    mg_send(to, from->recv_mbuf.buf, from->recv_mbuf.len);
    mbuf_remove(&from->recv_mbuf, from->recv_mbuf.len);
}

double mg_set_timer(struct mg_connection *c, double timestamp) 
{
    double result = c->ev_timer_time;
    c->ev_timer_time = timestamp;

    /*
    * If this connection is resolving, it's not in the list of active
    * connections, so not processed yet. It has a DNS resolver connection
    * linked to it. Set up a timer for the DNS connection.
    */
    DBG(("%p %p %d -> %lu", c, c->priv_2, (c->flags & MG_F_RESOLVING ? 1 : 0), (unsigned long) timestamp));
    if ((c->flags & MG_F_RESOLVING) && c->priv_2 != NULL) {
        mg_set_timer((struct mg_connection *) c->priv_2, timestamp);
    }
    return result;
}

void mg_sock_set(struct mg_connection *nc, sock_t sock) 
{
    if (sock != INVALID_SOCKET) {
        nc->iface->vtable->sock_set(nc, sock);
    }
}

void mg_if_get_conn_addr(struct mg_connection *nc, int remote, union socket_address *sa) 
{
    nc->iface->vtable->get_conn_addr(nc, remote, sa);
}

struct mg_connection *mg_add_sock_opt(struct mg_mgr *s, sock_t sock,
                                      MG_CB(mg_event_handler_t callback,
                                            void *user_data),
                                      struct mg_add_sock_opts opts) 
{

#if MG_ENABLE_CALLBACK_USERDATA
    opts.user_data = user_data;
#endif

    struct mg_connection *nc = mg_create_connection_base(s, callback, opts);
    if (nc != NULL) {
        mg_sock_set(nc, sock);
        mg_add_conn(nc->mgr, nc);
    }
    return nc;
}

struct mg_connection *mg_add_sock(struct mg_mgr *s, sock_t sock,
                                  MG_CB(mg_event_handler_t callback, void *user_data)) 
{
    struct mg_add_sock_opts opts;
    memset(&opts, 0, sizeof(opts));
    return mg_add_sock_opt(s, sock, MG_CB(callback, user_data), opts);
}

double mg_time(void) 
{
    return cs_time();
}


#ifndef CS_MONGOOSE_SRC_NET_IF_SOCKET_H_
#define CS_MONGOOSE_SRC_NET_IF_SOCKET_H_

#ifndef MG_ENABLE_NET_IF_SOCKET
#define MG_ENABLE_NET_IF_SOCKET MG_NET_IF == MG_NET_IF_SOCKET
#endif

extern const struct mg_iface_vtable mg_socket_iface_vtable;


#endif /* CS_MONGOOSE_SRC_NET_IF_SOCKET_H_ */

#ifndef CS_MONGOOSE_SRC_NET_IF_SOCKS_H_
#define CS_MONGOOSE_SRC_NET_IF_SOCKS_H_

#if MG_ENABLE_SOCKS
/* Amalgamated: #include "mg_net_if.h" */

extern const struct mg_iface_vtable mg_socks_iface_vtable;


#endif /* MG_ENABLE_SOCKS */

#endif /* CS_MONGOOSE_SRC_NET_IF_SOCKS_H_ */

extern const struct mg_iface_vtable mg_default_iface_vtable;

const struct mg_iface_vtable *mg_ifaces[] = {
    &mg_default_iface_vtable,
};

int mg_num_ifaces = (int) (sizeof(mg_ifaces) / sizeof(mg_ifaces[0]));

struct mg_iface *mg_if_create_iface(const struct mg_iface_vtable *vtable, struct mg_mgr *mgr) 
{
    struct mg_iface *iface = (struct mg_iface *) MG_CALLOC(1, sizeof(*iface));
    iface->mgr = mgr;
    iface->data = NULL;
    iface->vtable = vtable;
    return iface;
}

struct mg_iface *mg_find_iface(struct mg_mgr *mgr,
                               const struct mg_iface_vtable *vtable,
                               struct mg_iface *from) 
{
    int i = 0;
    if (from != NULL) {
        for (i = 0; i < mgr->num_ifaces; i++) {
            if (mgr->ifaces[i] == from) {
                i++;
                break;
            }
        }
    }

    for (; i < mgr->num_ifaces; i++) {
        if (mgr->ifaces[i]->vtable == vtable) {
            return mgr->ifaces[i];
        }
    }
    return NULL;
}

double mg_mgr_min_timer(const struct mg_mgr *mgr) 
{
    double min_timer = 0;
    struct mg_connection *nc;
    for (nc = mgr->active_connections; nc != NULL; nc = nc->next) {
        if (nc->ev_timer_time <= 0) continue;
        if (min_timer == 0 || nc->ev_timer_time < min_timer) {
            min_timer = nc->ev_timer_time;
        }
    }
    return min_timer;
}



#if MG_ENABLE_NET_IF_SOCKET


static sock_t mg_open_listening_socket(union socket_address *sa, int type, int proto);

void mg_set_non_blocking_mode(sock_t sock) 
{
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);
}

static int mg_is_error(void) 
{
    int err = mg_get_errno();
    return err != EINPROGRESS && err != EWOULDBLOCK
#ifndef WINCE
         && err != EAGAIN && err != EINTR
#endif
      ;
}

void mg_socket_if_connect_tcp(struct mg_connection *nc, const union socket_address *sa) 
{
    int rc, proto = 0;
    nc->sock = socket(AF_INET, SOCK_STREAM, proto);
    if (nc->sock == INVALID_SOCKET) {
        nc->err = mg_get_errno() ? mg_get_errno() : 1;
        return;
    }

#if !defined(MG_ESP8266)
    mg_set_non_blocking_mode(nc->sock);
#endif
  
    rc = connect(nc->sock, &sa->sa, sizeof(sa->sin));
    nc->err = rc < 0 && mg_is_error() ? mg_get_errno() : 0;
    DBG(("%p sock %d rc %d errno %d err %d", nc, nc->sock, rc, mg_get_errno(), nc->err));
}

void mg_socket_if_connect_udp(struct mg_connection *nc) 
{
    nc->sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (nc->sock == INVALID_SOCKET) {
        nc->err = mg_get_errno() ? mg_get_errno() : 1;
        return;
    }
    
    if (nc->flags & MG_F_ENABLE_BROADCAST) {
        int optval = 1;
        if (setsockopt(nc->sock, SOL_SOCKET, SO_BROADCAST, (const char *) &optval, sizeof(optval)) < 0) {
            nc->err = mg_get_errno() ? mg_get_errno() : 1;
            return;
        }
    }
    nc->err = 0;
}

int mg_socket_if_listen_tcp(struct mg_connection *nc, union socket_address *sa) 
{
    int proto = 0;
    sock_t sock = mg_open_listening_socket(sa, SOCK_STREAM, proto);
    if (sock == INVALID_SOCKET) {
        return (mg_get_errno() ? mg_get_errno() : 1);
    }
    mg_sock_set(nc, sock);
    return 0;
}

static int mg_socket_if_listen_udp(struct mg_connection *nc, union socket_address *sa) 
{
    sock_t sock = mg_open_listening_socket(sa, SOCK_DGRAM, 0);
    if (sock == INVALID_SOCKET) 
        return (mg_get_errno() ? mg_get_errno() : 1);
    mg_sock_set(nc, sock);
    return 0;
}


static int mg_socket_if_tcp_send(struct mg_connection *nc, const void *buf, size_t len) 
{
    int n = (int) MG_SEND_FUNC(nc->sock, buf, len, 0);
    if (n < 0 && !mg_is_error()) 
        n = 0;
    return n;
}

static int mg_socket_if_udp_send(struct mg_connection *nc, const void *buf, size_t len) 
{
    int n = sendto(nc->sock, buf, len, 0, &nc->sa.sa, sizeof(nc->sa.sin));
    if (n < 0 && !mg_is_error()) 
        n = 0;
    return n;
}

static int mg_socket_if_tcp_recv(struct mg_connection *nc, void *buf, size_t len) 
{
    int n = (int) MG_RECV_FUNC(nc->sock, buf, len, 0);
    if (n == 0) {
        /* Orderly shutdown of the socket, try flushing output. */
        nc->flags |= MG_F_SEND_AND_CLOSE;
    } else if (n < 0 && !mg_is_error()) {
        n = 0;
    }
    return n;
}

static int mg_socket_if_udp_recv(struct mg_connection *nc, 
                                  void *buf,
                                  size_t len, 
                                  union socket_address *sa,
                                  size_t *sa_len) 
{
    socklen_t sa_len_st = *sa_len;
    int n = recvfrom(nc->sock, buf, len, 0, &sa->sa, &sa_len_st);
    *sa_len = sa_len_st;
    if (n < 0 && !mg_is_error()) 
        n = 0;
    return n;
}

int mg_socket_if_create_conn(struct mg_connection *nc) 
{
    (void) nc;
    return 1;
}

void mg_socket_if_destroy_conn(struct mg_connection *nc) 
{
    if (nc->sock == INVALID_SOCKET) 
        return;
    
    if (!(nc->flags & MG_F_UDP)) {
        closesocket(nc->sock);
    } else {
        /* Only close outgoing UDP sockets or listeners. */
        if (nc->listener == NULL) 
            closesocket(nc->sock);
    }
    nc->sock = INVALID_SOCKET;
}

static int mg_accept_conn(struct mg_connection *lc) 
{
    struct mg_connection *nc;
    union socket_address sa;
    socklen_t sa_len = sizeof(sa);
  
    /* 接受该连接 */
    sock_t sock = accept(lc->sock, &sa.sa, &sa_len);
    if (sock == INVALID_SOCKET) {
        if (mg_is_error()) {
            LOGERR(TAG, "Accept failed, connection(0x%p), err:%d", lc, mg_get_errno());
        }
        return 0;
    }
  
    /* 创建新的连接对象 */
    nc = mg_if_accept_new_conn(lc);
    if (nc == NULL) {
        closesocket(sock);
        return 0;
    }

    LOGDBG(TAG, "mg_accept_conn: conn[%p] from remote ip:port[%s: %d]", nc, inet_ntoa(sa.sin.sin_addr), ntohs(sa.sin.sin_port));
    
    mg_sock_set(nc, sock);
    mg_if_accept_tcp_cb(nc, &sa, sa_len);
    return 1;
}


/* 'sa' must be an initialized address to bind to */
static sock_t mg_open_listening_socket(union socket_address *sa, int type, int proto) 
{
    socklen_t sa_len = (sa->sa.sa_family == AF_INET) ? sizeof(sa->sin) : sizeof(sa->sin6);
    sock_t sock = INVALID_SOCKET;

#if !MG_LWIP
  int on = 1;
#endif

    if ((sock = socket(sa->sa.sa_family, type, proto)) != INVALID_SOCKET &&
#if !MG_LWIP /* LWIP doesn't support either */
#if defined(_WIN32) && defined(SO_EXCLUSIVEADDRUSE) && !defined(WINCE)
      /* "Using SO_REUSEADDR and SO_EXCLUSIVEADDRUSE" http://goo.gl/RmrFTm */
      !setsockopt(sock, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, (void *) &on,
                  sizeof(on)) &&
#endif

#if !defined(_WIN32) || !defined(SO_EXCLUSIVEADDRUSE)
      /*
       * SO_RESUSEADDR is not enabled on Windows because the semantics of
       * SO_REUSEADDR on UNIX and Windows is different. On Windows,
       * SO_REUSEADDR allows to bind a socket to a port without error even if
       * the port is already open by another program. This is not the behavior
       * SO_REUSEADDR was designed for, and leads to hard-to-track failure
       * scenarios. Therefore, SO_REUSEADDR was disabled on Windows unless
       * SO_EXCLUSIVEADDRUSE is supported and set on a socket.
       */
      !setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (void *) &on, sizeof(on)) &&
#endif
#endif /* !MG_LWIP */

        !bind(sock, &sa->sa, sa_len) && (type == SOCK_DGRAM || listen(sock, SOMAXCONN) == 0)) {
#if !MG_LWIP
          mg_set_non_blocking_mode(sock);
          /* In case port was set to 0, get the real port number */
          (void) getsockname(sock, &sa->sa, &sa_len);
#endif
    } else if (sock != INVALID_SOCKET) {
        closesocket(sock);
        sock = INVALID_SOCKET;
    }

    return sock;
}

#define _MG_F_FD_CAN_READ 1
#define _MG_F_FD_CAN_WRITE 1 << 1
#define _MG_F_FD_ERROR 1 << 2


void mg_mgr_handle_conn(struct mg_connection *nc, int fd_flags, double now) 
{
    /* 标记是在何处设置(读/写) */
    int worth_logging = fd_flags != 0 || (nc->flags & (MG_F_WANT_READ | MG_F_WANT_WRITE));
    
    if (worth_logging) {
        DBG(("%p fd=%d fd_flags=%d nc_flags=0x%lx rmbl=%d smbl=%d", nc, nc->sock,
            fd_flags, nc->flags, (int) nc->recv_mbuf.len,
            (int) nc->send_mbuf.len));
    }

    if (!mg_if_poll(nc, now)) 
        return;

    if (nc->flags & MG_F_CONNECTING) {
        if (fd_flags != 0) {
            int err = 0;
#if !defined(MG_ESP8266)
            if (!(nc->flags & MG_F_UDP)) {
                socklen_t len = sizeof(err);
                int ret = getsockopt(nc->sock, SOL_SOCKET, SO_ERROR, (char *) &err, &len);
                if (ret != 0) {
                    err = 1;
                } else if (err == EAGAIN || err == EWOULDBLOCK) {
                    err = 0;
                }
            }
#else
           /*
            * On ESP8266 we use blocking connect.
            */
            err = nc->err;
#endif
            mg_if_connect_cb(nc, err);
        } else if (nc->err != 0) {
            mg_if_connect_cb(nc, nc->err);
        }
    }

    if (fd_flags & _MG_F_FD_CAN_READ) { /* 该连接有数据可读(接收到了数据) */
        if (nc->flags & MG_F_UDP) {     /* UDP */
            mg_if_can_recv_cb(nc);
        } else {                        /* TCP */
            if (nc->flags & MG_F_LISTENING) {   /* 连接处于监听状态(服务器的第一个连接一直处于监听状态) */
                mg_accept_conn(nc);     /* 与客户端建立新的连接 */
            } else {
                mg_if_can_recv_cb(nc);  /* 非监听状态,读取客户端发送的数据 */
            }
        }
    }

    /* 检查该连接是否可以发数据 */
    if (fd_flags & _MG_F_FD_CAN_WRITE) {
        // LOGDBG(TAG, "--> Send data here...");
        mg_if_can_send_cb(nc);    /* 往该连接发送数据 */
    }

    if (worth_logging) {
        DBG(("%p after fd=%d nc_flags=0x%lx rmbl=%d smbl=%d", nc, nc->sock,
         nc->flags, (int) nc->recv_mbuf.len, (int) nc->send_mbuf.len));
    }
}


#if MG_ENABLE_BROADCAST
static void mg_mgr_handle_ctl_sock(struct mg_mgr *mgr) 
{
    struct ctl_msg ctl_msg;
    int len = (int) MG_RECV_FUNC(mgr->ctl[1], (char *) &ctl_msg, sizeof(ctl_msg), 0);
    size_t dummy = MG_SEND_FUNC(mgr->ctl[1], ctl_msg.message, 1, 0);
    DBG(("read %d from ctl socket", len));
    
    (void) dummy; /* https://gcc.gnu.org/bugzilla/show_bug.cgi?id=25509 */
    if (len >= (int) sizeof(ctl_msg.callback) && ctl_msg.callback != NULL) {
        struct mg_connection *nc;
        for (nc = mg_next(mgr, NULL); nc != NULL; nc = mg_next(mgr, nc)) {
            ctl_msg.callback(nc, MG_EV_POLL,
                       ctl_msg.message MG_UD_ARG(nc->user_data));
        }
    }
}
#endif

/* Associate a socket to a connection. */
void mg_socket_if_sock_set(struct mg_connection *nc, sock_t sock) 
{
    mg_set_non_blocking_mode(sock);
    mg_set_close_on_exec(sock);
    nc->sock = sock;
    DBG(("%p %d", nc, sock));
}

void mg_socket_if_init(struct mg_iface *iface) 
{
    (void) iface;
    DBG(("%p using select()", iface->mgr));

#if MG_ENABLE_BROADCAST
    mg_socketpair(iface->mgr->ctl, SOCK_DGRAM);
#endif
}

void mg_socket_if_free(struct mg_iface *iface) 
{
    (void) iface;
}

void mg_socket_if_add_conn(struct mg_connection *nc) 
{
    (void) nc;
}

void mg_socket_if_remove_conn(struct mg_connection *nc) 
{
    (void) nc;
}

void mg_add_to_set(sock_t sock, fd_set *set, sock_t *max_fd) 
{
    if (sock != INVALID_SOCKET
#ifdef __unix__
      && sock < (sock_t) FD_SETSIZE
#endif
      ) {
        FD_SET(sock, set);
        if (*max_fd == INVALID_SOCKET || sock > *max_fd) {
            *max_fd = sock;
        }
    }
}


/*
 * skymixos - core poll
 */
time_t mg_socket_if_poll(struct mg_iface *iface, int timeout_ms) 
{
    struct mg_mgr *mgr = iface->mgr;
    double now = mg_time();
    double min_timer;
    struct mg_connection *nc, *tmp;
    struct timeval tv;
    fd_set read_set, write_set, err_set;
    sock_t max_fd = INVALID_SOCKET;
    int num_fds, num_ev, num_timers = 0;
    int iActiveConn = 0;

#ifdef __unix__
    int try_dup = 1;
#endif

    FD_ZERO(&read_set);
    FD_ZERO(&write_set);
    FD_ZERO(&err_set);

#if MG_ENABLE_BROADCAST
    mg_add_to_set(mgr->ctl[1], &read_set, &max_fd);
#endif

    /*
     * Note: it is ok to have connections with sock == INVALID_SOCKET in the list,
     * e.g. timer-only "connections".
     */
    min_timer = 0;

    for (nc = mgr->active_connections, num_fds = 0; nc != NULL; nc = tmp) {
        
        tmp = nc->next;
        if (nc->sock != INVALID_SOCKET) {
            num_fds++;
            iActiveConn++;
        #ifdef __unix__
            /* A hack to make sure all our file descriptos fit into FD_SETSIZE. */
            if (nc->sock >= (sock_t) FD_SETSIZE && try_dup) {
                int new_sock = dup(nc->sock);
                if (new_sock >= 0) {
                    if (new_sock < (sock_t) FD_SETSIZE) {
                        closesocket(nc->sock);
                        DBG(("new sock %d -> %d", nc->sock, new_sock));
                        nc->sock = new_sock;
                    } else {
                        closesocket(new_sock);
                        DBG(("new sock is still larger than FD_SETSIZE, disregard"));
                        try_dup = 0;
                    }
                } else {
                    try_dup = 0;
                }
            }
        #endif

            if (nc->recv_mbuf.len < nc->recv_mbuf_limit && (!(nc->flags & MG_F_UDP) || nc->listener == NULL)) {
                mg_add_to_set(nc->sock, &read_set, &max_fd);
            }

            if (((nc->flags & MG_F_CONNECTING) && !(nc->flags & MG_F_WANT_READ)) ||
                            (nc->send_mbuf.len > 0 && !(nc->flags & MG_F_CONNECTING))) {
                mg_add_to_set(nc->sock, &write_set, &max_fd);
                mg_add_to_set(nc->sock, &err_set, &max_fd);
            }
        }

        if (nc->ev_timer_time > 0) {
            if (num_timers == 0 || nc->ev_timer_time < min_timer) {
                min_timer = nc->ev_timer_time;
            }
            num_timers++;
        }
    }

    /*
     * If there is a timer to be fired earlier than the requested timeout,
     * adjust the timeout.
     */
    if (num_timers > 0) {
        double timer_timeout_ms = (min_timer - mg_time()) * 1000 + 1 /* rounding */;
        if (timer_timeout_ms < timeout_ms) {
            timeout_ms = (int) timer_timeout_ms;
        }
    }
    
    if (timeout_ms < 0) 
        timeout_ms = 0;

    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;

    // LOGDBG(TAG, "mg_socket_if_poll: select here, active num[%d]", iActiveConn);
    
    num_ev = select((int) max_fd + 1, &read_set, &write_set, &err_set, &tv);
    now = mg_time();

#if MG_ENABLE_BROADCAST
    if (num_ev > 0 && mgr->ctl[1] != INVALID_SOCKET && FD_ISSET(mgr->ctl[1], &read_set)) {
        mg_mgr_handle_ctl_sock(mgr);
    }
#endif

    for (nc = mgr->active_connections; nc != NULL; nc = tmp) {
        int fd_flags = 0;
        if (nc->sock != INVALID_SOCKET) {
            if (num_ev > 0) {
                fd_flags = (FD_ISSET(nc->sock, &read_set) && (!(nc->flags & MG_F_UDP) || nc->listener == NULL)
                        ? _MG_F_FD_CAN_READ
                        : 0) | (FD_ISSET(nc->sock, &write_set) ? _MG_F_FD_CAN_WRITE : 0) | (FD_ISSET(nc->sock, &err_set) ? _MG_F_FD_ERROR : 0);
            }
        }

        tmp = nc->next;
        mg_mgr_handle_conn(nc, fd_flags, now);
    }
    return (time_t) now;
}



#if MG_ENABLE_BROADCAST
MG_INTERNAL void mg_socketpair_close(sock_t *sock) 
{
    while (1) {
        if (closesocket(*sock) == -1 && errno == EINTR) 
            continue;
        break;
    }
    *sock = INVALID_SOCKET;
}


MG_INTERNAL sock_t
mg_socketpair_accept(sock_t sock, union socket_address *sa, socklen_t sa_len) 
{
    sock_t rc;
    while (1) {
        if ((rc = accept(sock, &sa->sa, &sa_len)) == INVALID_SOCKET && errno == EINTR)
            continue;
        break;
    }
    return rc;
}

int mg_socketpair(sock_t sp[2], int sock_type) 
{
    union socket_address sa, sa2;
    sock_t sock;
    socklen_t len = sizeof(sa.sin);
    int ret = 0;

    sock = sp[0] = sp[1] = INVALID_SOCKET;

    (void) memset(&sa, 0, sizeof(sa));
    sa.sin.sin_family = AF_INET;
    sa.sin.sin_addr.s_addr = htonl(0x7f000001); /* 127.0.0.1 */
    sa2 = sa;

    if ((sock = socket(AF_INET, sock_type, 0)) == INVALID_SOCKET) {
    } else if (bind(sock, &sa.sa, len) != 0) {
    } else if (sock_type == SOCK_STREAM && listen(sock, 1) != 0) {
    } else if (getsockname(sock, &sa.sa, &len) != 0) {
    } else if ((sp[0] = socket(AF_INET, sock_type, 0)) == INVALID_SOCKET) {
    } else if (sock_type == SOCK_STREAM && connect(sp[0], &sa.sa, len) != 0) {
    } else if (sock_type == SOCK_DGRAM &&
             (bind(sp[0], &sa2.sa, len) != 0 ||
              getsockname(sp[0], &sa2.sa, &len) != 0 ||
              connect(sp[0], &sa.sa, len) != 0 ||
              connect(sock, &sa2.sa, len) != 0)) {
    } else if ((sp[1] = (sock_type == SOCK_DGRAM ? sock : mg_socketpair_accept(
                                                            sock, &sa, len))) ==
             INVALID_SOCKET) {
    } else {
        mg_set_close_on_exec(sp[0]);
        mg_set_close_on_exec(sp[1]);
        if (sock_type == SOCK_STREAM) mg_socketpair_close(&sock);
        ret = 1;
    }

    if (!ret) {
        if (sp[0] != INVALID_SOCKET) mg_socketpair_close(&sp[0]);
        if (sp[1] != INVALID_SOCKET) mg_socketpair_close(&sp[1]);
        if (sock != INVALID_SOCKET) mg_socketpair_close(&sock);
    }

    return ret;
}
#endif /* MG_ENABLE_BROADCAST */

static void mg_sock_get_addr(sock_t sock, int remote, union socket_address *sa) 
{
    socklen_t slen = sizeof(*sa);
    memset(sa, 0, slen);
    if (remote) {
        getpeername(sock, &sa->sa, &slen);
    } else {
        getsockname(sock, &sa->sa, &slen);
    }
}

void mg_sock_to_str(sock_t sock, char *buf, size_t len, int flags) 
{
    union socket_address sa;
    mg_sock_get_addr(sock, flags & MG_SOCK_STRINGIFY_REMOTE, &sa);
    mg_sock_addr_to_str(&sa, buf, len, flags);
}

void mg_socket_if_get_conn_addr(struct mg_connection *nc, int remote, union socket_address *sa) 
{
    if ((nc->flags & MG_F_UDP) && remote) {
        memcpy(sa, &nc->sa, sizeof(*sa));
        return;
    }
    mg_sock_get_addr(nc->sock, remote, sa);
}

/* clang-format off */
#define MG_SOCKET_IFACE_VTABLE                                          \
  {                                                                     \
    mg_socket_if_init,                                                  \
    mg_socket_if_free,                                                  \
    mg_socket_if_add_conn,                                              \
    mg_socket_if_remove_conn,                                           \
    mg_socket_if_poll,                                                  \
    mg_socket_if_listen_tcp,                                            \
    mg_socket_if_listen_udp,                                            \
    mg_socket_if_connect_tcp,                                           \
    mg_socket_if_connect_udp,                                           \
    mg_socket_if_tcp_send,                                              \
    mg_socket_if_udp_send,                                              \
    mg_socket_if_tcp_recv,                                              \
    mg_socket_if_udp_recv,                                              \
    mg_socket_if_create_conn,                                           \
    mg_socket_if_destroy_conn,                                          \
    mg_socket_if_sock_set,                                              \
    mg_socket_if_get_conn_addr,                                         \
  }
/* clang-format on */

const struct mg_iface_vtable mg_socket_iface_vtable = MG_SOCKET_IFACE_VTABLE;
#if MG_NET_IF == MG_NET_IF_SOCKET
const struct mg_iface_vtable mg_default_iface_vtable = MG_SOCKET_IFACE_VTABLE;
#endif

#endif /* MG_ENABLE_NET_IF_SOCKET */



#if MG_ENABLE_SSL && MG_SSL_IF == MG_SSL_IF_OPENSSL

#ifdef __APPLE__
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

#include <openssl/ssl.h>
#ifndef KR_VERSION
#include <openssl/tls1.h>
#endif

struct mg_ssl_if_ctx {
    SSL *ssl;
    SSL_CTX *ssl_ctx;
    struct mbuf psk;
    size_t identity_len;
};

void mg_ssl_if_init() 
{
    SSL_library_init();
}

enum mg_ssl_if_result mg_ssl_if_conn_accept(struct mg_connection *nc,
                                            struct mg_connection *lc) 
{
    struct mg_ssl_if_ctx *ctx = (struct mg_ssl_if_ctx *) MG_CALLOC(1, sizeof(*ctx));
    struct mg_ssl_if_ctx *lc_ctx = (struct mg_ssl_if_ctx *) lc->ssl_if_data;
    nc->ssl_if_data = ctx;
    if (ctx == NULL || lc_ctx == NULL) return MG_SSL_ERROR;
    ctx->ssl_ctx = lc_ctx->ssl_ctx;
    if ((ctx->ssl = SSL_new(ctx->ssl_ctx)) == NULL) {
        return MG_SSL_ERROR;
    }
    return MG_SSL_OK;
}

static enum mg_ssl_if_result mg_use_cert(SSL_CTX *ctx, const char *cert,
                                         const char *key, const char **err_msg);
static enum mg_ssl_if_result mg_use_ca_cert(SSL_CTX *ctx, const char *cert);
static enum mg_ssl_if_result mg_set_cipher_list(SSL_CTX *ctx, const char *cl);
static enum mg_ssl_if_result mg_ssl_if_ossl_set_psk(struct mg_ssl_if_ctx *ctx,
                                                    const char *identity,
                                                    const char *key_str);

enum mg_ssl_if_result mg_ssl_if_conn_init(struct mg_connection *nc, 
                                            const struct mg_ssl_if_conn_params *params,
                                            const char **err_msg) 
{
    struct mg_ssl_if_ctx *ctx = (struct mg_ssl_if_ctx *) MG_CALLOC(1, sizeof(*ctx));
    DBG(("%p %s,%s,%s", nc, (params->cert ? params->cert : ""),
       (params->key ? params->key : ""),
       (params->ca_cert ? params->ca_cert : "")));
    
    if (ctx == NULL) {
        MG_SET_PTRPTR(err_msg, "Out of memory");
        return MG_SSL_ERROR;
    }
    
    nc->ssl_if_data = ctx;
    if (nc->flags & MG_F_LISTENING) {
        ctx->ssl_ctx = SSL_CTX_new(SSLv23_server_method());
    } else {
        ctx->ssl_ctx = SSL_CTX_new(SSLv23_client_method());
    }
  
    if (ctx->ssl_ctx == NULL) {
        MG_SET_PTRPTR(err_msg, "Failed to create SSL context");
        return MG_SSL_ERROR;
    }

#ifndef KR_VERSION
    /* Disable deprecated protocols. */
    SSL_CTX_set_options(ctx->ssl_ctx, SSL_OP_NO_SSLv2);
    SSL_CTX_set_options(ctx->ssl_ctx, SSL_OP_NO_SSLv3);
    SSL_CTX_set_options(ctx->ssl_ctx, SSL_OP_NO_TLSv1);
#ifdef MG_SSL_OPENSSL_NO_COMPRESSION
    SSL_CTX_set_options(ctx->ssl_ctx, SSL_OP_NO_COMPRESSION);
#endif
#ifdef MG_SSL_OPENSSL_CIPHER_SERVER_PREFERENCE
    SSL_CTX_set_options(ctx->ssl_ctx, SSL_OP_CIPHER_SERVER_PREFERENCE);
#endif
#else
/* Krypton only supports TLSv1.2 anyway. */
#endif

    if (params->cert != NULL &&
        mg_use_cert(ctx->ssl_ctx, params->cert, params->key, err_msg) != MG_SSL_OK) {
        return MG_SSL_ERROR;
    }

    if (params->ca_cert != NULL &&
            mg_use_ca_cert(ctx->ssl_ctx, params->ca_cert) != MG_SSL_OK) {
        MG_SET_PTRPTR(err_msg, "Invalid SSL CA cert");
        return MG_SSL_ERROR;
    }

    if (mg_set_cipher_list(ctx->ssl_ctx, params->cipher_suites) != MG_SSL_OK) {
        MG_SET_PTRPTR(err_msg, "Invalid cipher suite list");
        return MG_SSL_ERROR;
    }

    mbuf_init(&ctx->psk, 0);
    if (mg_ssl_if_ossl_set_psk(ctx, params->psk_identity, params->psk_key) !=
      MG_SSL_OK) {
        MG_SET_PTRPTR(err_msg, "Invalid PSK settings");
        return MG_SSL_ERROR;
    }

    if (!(nc->flags & MG_F_LISTENING) && (ctx->ssl = SSL_new(ctx->ssl_ctx)) == NULL) {
        MG_SET_PTRPTR(err_msg, "Failed to create SSL session");
        return MG_SSL_ERROR;
    }

    if (params->server_name != NULL) {
#ifdef KR_VERSION
        SSL_CTX_kr_set_verify_name(ctx->ssl_ctx, params->server_name);
#else
        SSL_set_tlsext_host_name(ctx->ssl, params->server_name);
#endif
    }

    nc->flags |= MG_F_SSL;
    return MG_SSL_OK;
}

static enum mg_ssl_if_result mg_ssl_if_ssl_err(struct mg_connection *nc, int res) 
{
    struct mg_ssl_if_ctx *ctx = (struct mg_ssl_if_ctx *) nc->ssl_if_data;
    int err = SSL_get_error(ctx->ssl, res);
    if (err == SSL_ERROR_WANT_READ) return MG_SSL_WANT_READ;
    if (err == SSL_ERROR_WANT_WRITE) return MG_SSL_WANT_WRITE;
    DBG(("%p %p SSL error: %d %d", nc, ctx->ssl_ctx, res, err));
    nc->err = err;
    return MG_SSL_ERROR;
}

enum mg_ssl_if_result mg_ssl_if_handshake(struct mg_connection *nc) 
{
    struct mg_ssl_if_ctx *ctx = (struct mg_ssl_if_ctx *) nc->ssl_if_data;
    int server_side = (nc->listener != NULL);
    int res;
    /* If descriptor is not yet set, do it now. */
    if (SSL_get_fd(ctx->ssl) < 0) {
        if (SSL_set_fd(ctx->ssl, nc->sock) != 1) return MG_SSL_ERROR;
    }
    res = server_side ? SSL_accept(ctx->ssl) : SSL_connect(ctx->ssl);
    if (res != 1) return mg_ssl_if_ssl_err(nc, res);
    return MG_SSL_OK;
}

int mg_ssl_if_read(struct mg_connection *nc, void *buf, size_t buf_size) 
{
    struct mg_ssl_if_ctx *ctx = (struct mg_ssl_if_ctx *) nc->ssl_if_data;
    int n = SSL_read(ctx->ssl, buf, buf_size);
    DBG(("%p %d -> %d", nc, (int) buf_size, n));
    if (n < 0) return mg_ssl_if_ssl_err(nc, n);
    if (n == 0) nc->flags |= MG_F_CLOSE_IMMEDIATELY;
    return n;
}

int mg_ssl_if_write(struct mg_connection *nc, const void *data, size_t len) 
{
    struct mg_ssl_if_ctx *ctx = (struct mg_ssl_if_ctx *) nc->ssl_if_data;
    int n = SSL_write(ctx->ssl, data, len);
    DBG(("%p %d -> %d", nc, (int) len, n));
    if (n <= 0) return mg_ssl_if_ssl_err(nc, n);
    return n;
}

void mg_ssl_if_conn_close_notify(struct mg_connection *nc) 
{
    struct mg_ssl_if_ctx *ctx = (struct mg_ssl_if_ctx *) nc->ssl_if_data;
    if (ctx == NULL) return;
    SSL_shutdown(ctx->ssl);
}

void mg_ssl_if_conn_free(struct mg_connection *nc) 
{
    struct mg_ssl_if_ctx *ctx = (struct mg_ssl_if_ctx *) nc->ssl_if_data;
    if (ctx == NULL) return;
    nc->ssl_if_data = NULL;
    if (ctx->ssl != NULL) SSL_free(ctx->ssl);
    if (ctx->ssl_ctx != NULL && nc->listener == NULL) SSL_CTX_free(ctx->ssl_ctx);
    mbuf_free(&ctx->psk);
    memset(ctx, 0, sizeof(*ctx));
    MG_FREE(ctx);
}

/*
 * Cipher suite options used for TLS negotiation.
 * https://wiki.mozilla.org/Security/Server_Side_TLS#Recommended_configurations
 */
static const char mg_s_cipher_list[] =
#if defined(MG_SSL_CRYPTO_MODERN)
    "ECDHE-ECDSA-AES256-GCM-SHA384:ECDHE-RSA-AES256-GCM-SHA384:"
    "ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES128-GCM-SHA256:"
    "DHE-RSA-AES128-GCM-SHA256:DHE-DSS-AES128-GCM-SHA256:kEDH+AESGCM:"
    "ECDHE-RSA-AES128-SHA256:ECDHE-ECDSA-AES128-SHA256:ECDHE-RSA-AES128-SHA:"
    "ECDHE-ECDSA-AES128-SHA:ECDHE-RSA-AES256-SHA384:ECDHE-ECDSA-AES256-SHA384:"
    "ECDHE-RSA-AES256-SHA:ECDHE-ECDSA-AES256-SHA:DHE-RSA-AES128-SHA256:"
    "DHE-RSA-AES128-SHA:DHE-DSS-AES128-SHA256:DHE-RSA-AES256-SHA256:"
    "DHE-DSS-AES256-SHA:DHE-RSA-AES256-SHA:"
    "!aNULL:!eNULL:!EXPORT:!DES:!RC4:!3DES:!MD5:!PSK"
#elif defined(MG_SSL_CRYPTO_OLD)
    "ECDHE-RSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES128-GCM-SHA256:"
    "ECDHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-AES256-GCM-SHA384:"
    "DHE-RSA-AES128-GCM-SHA256:DHE-DSS-AES128-GCM-SHA256:kEDH+AESGCM:"
    "ECDHE-RSA-AES128-SHA256:ECDHE-ECDSA-AES128-SHA256:ECDHE-RSA-AES128-SHA:"
    "ECDHE-ECDSA-AES128-SHA:ECDHE-RSA-AES256-SHA384:ECDHE-ECDSA-AES256-SHA384:"
    "ECDHE-RSA-AES256-SHA:ECDHE-ECDSA-AES256-SHA:DHE-RSA-AES128-SHA256:"
    "DHE-RSA-AES128-SHA:DHE-DSS-AES128-SHA256:DHE-RSA-AES256-SHA256:"
    "DHE-DSS-AES256-SHA:DHE-RSA-AES256-SHA:ECDHE-RSA-DES-CBC3-SHA:"
    "ECDHE-ECDSA-DES-CBC3-SHA:AES128-GCM-SHA256:AES256-GCM-SHA384:"
    "AES128-SHA256:AES256-SHA256:AES128-SHA:AES256-SHA:AES:DES-CBC3-SHA:"
    "HIGH:!aNULL:!eNULL:!EXPORT:!DES:!RC4:!MD5:!PSK:!aECDH:"
    "!EDH-DSS-DES-CBC3-SHA:!EDH-RSA-DES-CBC3-SHA:!KRB5-DES-CBC3-SHA"
#else /* Default - intermediate. */
    "ECDHE-RSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES128-GCM-SHA256:"
    "ECDHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-AES256-GCM-SHA384:"
    "DHE-RSA-AES128-GCM-SHA256:DHE-DSS-AES128-GCM-SHA256:kEDH+AESGCM:"
    "ECDHE-RSA-AES128-SHA256:ECDHE-ECDSA-AES128-SHA256:ECDHE-RSA-AES128-SHA:"
    "ECDHE-ECDSA-AES128-SHA:ECDHE-RSA-AES256-SHA384:ECDHE-ECDSA-AES256-SHA384:"
    "ECDHE-RSA-AES256-SHA:ECDHE-ECDSA-AES256-SHA:DHE-RSA-AES128-SHA256:"
    "DHE-RSA-AES128-SHA:DHE-DSS-AES128-SHA256:DHE-RSA-AES256-SHA256:"
    "DHE-DSS-AES256-SHA:DHE-RSA-AES256-SHA:AES128-GCM-SHA256:AES256-GCM-SHA384:"
    "AES128-SHA256:AES256-SHA256:AES128-SHA:AES256-SHA:AES:CAMELLIA:"
      "DES-CBC3-SHA:!aNULL:!eNULL:!EXPORT:!DES:!RC4:!MD5:!PSK:!aECDH:"
    "!EDH-DSS-DES-CBC3-SHA:!EDH-RSA-DES-CBC3-SHA:!KRB5-DES-CBC3-SHA"
#endif
    ;

/*
 * Default DH params for PFS cipher negotiation. This is a 2048-bit group.
 * Will be used if none are provided by the user in the certificate file.
 */
#if !MG_DISABLE_PFS && !defined(KR_VERSION)
static const char mg_s_default_dh_params[] =
    "\
-----BEGIN DH PARAMETERS-----\n\
MIIBCAKCAQEAlvbgD/qh9znWIlGFcV0zdltD7rq8FeShIqIhkQ0C7hYFThrBvF2E\n\
Z9bmgaP+sfQwGpVlv9mtaWjvERbu6mEG7JTkgmVUJrUt/wiRzwTaCXBqZkdUO8Tq\n\
+E6VOEQAilstG90ikN1Tfo+K6+X68XkRUIlgawBTKuvKVwBhuvlqTGerOtnXWnrt\n\
ym//hd3cd5PBYGBix0i7oR4xdghvfR2WLVu0LgdThTBb6XP7gLd19cQ1JuBtAajZ\n\
wMuPn7qlUkEFDIkAZy59/Hue/H2Q2vU/JsvVhHWCQBL4F1ofEAt50il6ZxR1QfFK\n\
9VGKDC4oOgm9DlxwwBoC2FjqmvQlqVV3kwIBAg==\n\
-----END DH PARAMETERS-----\n";
#endif

static enum mg_ssl_if_result mg_use_ca_cert(SSL_CTX *ctx, const char *cert) 
{
    if (cert == NULL || strcmp(cert, "*") == 0) {
        return MG_SSL_OK;
    }
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, 0);
    return SSL_CTX_load_verify_locations(ctx, cert, NULL) == 1 ? MG_SSL_OK
                                                             : MG_SSL_ERROR;
}

static enum mg_ssl_if_result mg_use_cert(SSL_CTX *ctx, const char *cert,
                                         const char *key,
                                         const char **err_msg) 
{
    if (key == NULL) key = cert;
    if (cert == NULL || cert[0] == '\0' || key == NULL || key[0] == '\0') {
        return MG_SSL_OK;
    } else if (SSL_CTX_use_certificate_file(ctx, cert, 1) == 0) {
        MG_SET_PTRPTR(err_msg, "Invalid SSL cert");
        return MG_SSL_ERROR;
    } else if (SSL_CTX_use_PrivateKey_file(ctx, key, 1) == 0) {
        MG_SET_PTRPTR(err_msg, "Invalid SSL key");
        return MG_SSL_ERROR;
    } else if (SSL_CTX_use_certificate_chain_file(ctx, cert) == 0) {
        MG_SET_PTRPTR(err_msg, "Invalid CA bundle");
        return MG_SSL_ERROR;
    } else {
        SSL_CTX_set_mode(ctx, SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);

#if !MG_DISABLE_PFS && !defined(KR_VERSION)
        BIO *bio = NULL;
        DH *dh = NULL;

        /* Try to read DH parameters from the cert/key file. */
        bio = BIO_new_file(cert, "r");
        if (bio != NULL) {
            dh = PEM_read_bio_DHparams(bio, NULL, NULL, NULL);
            BIO_free(bio);
        }
    
        /*
        * If there are no DH params in the file, fall back to hard-coded ones.
        * Not ideal, but better than nothing.
        */
        if (dh == NULL) {
            bio = BIO_new_mem_buf((void *) mg_s_default_dh_params, -1);
            dh = PEM_read_bio_DHparams(bio, NULL, NULL, NULL);
            BIO_free(bio);
        }
        
        if (dh != NULL) {
            SSL_CTX_set_tmp_dh(ctx, dh);
            SSL_CTX_set_options(ctx, SSL_OP_SINGLE_DH_USE);
            DH_free(dh);
        }
#if OPENSSL_VERSION_NUMBER > 0x10002000L
        SSL_CTX_set_ecdh_auto(ctx, 1);
#endif
#endif
    }
    return MG_SSL_OK;
}

static enum mg_ssl_if_result mg_set_cipher_list(SSL_CTX *ctx, const char *cl) 
{
    return (SSL_CTX_set_cipher_list(ctx, cl ? cl : mg_s_cipher_list) == 1
              ? MG_SSL_OK
              : MG_SSL_ERROR);
}

#ifndef KR_VERSION
static unsigned int mg_ssl_if_ossl_psk_cb(SSL *ssl, const char *hint,
                                          char *identity,
                                          unsigned int max_identity_len,
                                          unsigned char *psk,
                                          unsigned int max_psk_len) 
{
    struct mg_ssl_if_ctx *ctx =
        (struct mg_ssl_if_ctx *) SSL_CTX_get_app_data(SSL_get_SSL_CTX(ssl));
    
    size_t key_len = ctx->psk.len - ctx->identity_len - 1;
    DBG(("hint: '%s'", (hint ? hint : "")));
    if (ctx->identity_len + 1 > max_identity_len) {
        DBG(("identity too long"));
        return 0;
    }
  
    if (key_len > max_psk_len) {
        DBG(("key too long"));
        return 0;
    }
  
    memcpy(identity, ctx->psk.buf, ctx->identity_len + 1);
    memcpy(psk, ctx->psk.buf + ctx->identity_len + 1, key_len);
    (void) ssl;
    return key_len;
}

static enum mg_ssl_if_result mg_ssl_if_ossl_set_psk(struct mg_ssl_if_ctx *ctx,
                                                    const char *identity,
                                                    const char *key_str) 
{
    unsigned char key[32];
    size_t key_len;
    size_t i = 0;
    if (identity == NULL && key_str == NULL) return MG_SSL_OK;
    if (identity == NULL || key_str == NULL) return MG_SSL_ERROR;
    key_len = strlen(key_str);
    if (key_len != 32 && key_len != 64) return MG_SSL_ERROR;
    memset(key, 0, sizeof(key));
    key_len = 0;
  
    for (i = 0; key_str[i] != '\0'; i++) {
        unsigned char c;
        char hc = tolower((int) key_str[i]);
        if (hc >= '0' && hc <= '9') {
            c = hc - '0';
        } else if (hc >= 'a' && hc <= 'f') {
            c = hc - 'a' + 0xa;
        } else {
            return MG_SSL_ERROR;
        }
        key_len = i / 2;
        key[key_len] <<= 4;
        key[key_len] |= c;
    }
    key_len++;
    DBG(("identity = '%s', key = (%u)", identity, (unsigned int) key_len));
    ctx->identity_len = strlen(identity);
    mbuf_append(&ctx->psk, identity, ctx->identity_len + 1);
    mbuf_append(&ctx->psk, key, key_len);
    SSL_CTX_set_psk_client_callback(ctx->ssl_ctx, mg_ssl_if_ossl_psk_cb);
    SSL_CTX_set_app_data(ctx->ssl_ctx, ctx);
    return MG_SSL_OK;
}
#else
static enum mg_ssl_if_result mg_ssl_if_ossl_set_psk(struct mg_ssl_if_ctx *ctx,
                                                    const char *identity,
                                                    const char *key_str) 
{
    (void) ctx;
    (void) identity;
    (void) key_str;
    /* Krypton does not support PSK. */
    return MG_SSL_ERROR;
}
#endif /* defined(KR_VERSION) */

const char *mg_set_ssl(struct mg_connection *nc, const char *cert, const char *ca_cert) 
{
    const char *err_msg = NULL;
    struct mg_ssl_if_conn_params params;
    memset(&params, 0, sizeof(params));
    params.cert = cert;
    params.ca_cert = ca_cert;
    if (mg_ssl_if_conn_init(nc, &params, &err_msg) != MG_SSL_OK) {
        return err_msg;
    }
    return NULL;
}

#endif /* MG_ENABLE_SSL && MG_SSL_IF == MG_SSL_IF_OPENSSL */


/*
 * scan string until encountering one of `seps`, keeping track of component
 * boundaries in `res`.
 *
 * `p` will point to the char after the separator or it will be `end`.
 */
static void parse_uri_component(const char **p, const char *end,
                                const char *seps, struct mg_str *res) 
{
    const char *q;
    res->p = *p;
    for (; *p < end; (*p)++) {
        for (q = seps; *q != '\0'; q++) {
            if (**p == *q) break;
        }
        if (*q != '\0') break;
    }
    res->len = (*p) - res->p;
    if (*p < end) (*p)++;
}

int mg_parse_uri(const struct mg_str uri, struct mg_str *scheme,
                 struct mg_str *user_info, struct mg_str *host,
                 unsigned int *port, struct mg_str *path, struct mg_str *query,
                 struct mg_str *fragment) 
{
    struct mg_str rscheme = {0, 0}, ruser_info = {0, 0}, rhost = {0, 0},
                rpath = {0, 0}, rquery = {0, 0}, rfragment = {0, 0};
    unsigned int rport = 0;
    enum {
        P_START,
        P_SCHEME_OR_PORT,
        P_USER_INFO,
        P_HOST,
        P_PORT,
        P_REST
    } state = P_START;

    const char *p = uri.p, *end = p + uri.len;
    while (p < end) {
        switch (state) {
            case P_START:
                /*
                * expecting on of:
                * - `scheme://xxxx`
                * - `xxxx:port`
                * - `[a:b:c]:port`
                * - `xxxx/path`
                */
                if (*p == '[') {
                    state = P_HOST;
                    break;
                }
                for (; p < end; p++) {
                    if (*p == ':') {
                        state = P_SCHEME_OR_PORT;
                        break;
                    } else if (*p == '/') {
                        state = P_REST;
                        break;
                    }
                }
                if (state == P_START || state == P_REST) {
                    rhost.p = uri.p;
                    rhost.len = p - uri.p;
                }
                break;

            case P_SCHEME_OR_PORT:
                if (end - p >= 3 && strncmp(p, "://", 3) == 0) {
                    rscheme.p = uri.p;
                    rscheme.len = p - uri.p;
                    state = P_USER_INFO;
                    p += 3;
                } else {
                    rhost.p = uri.p;
                    rhost.len = p - uri.p;
                    state = P_PORT;
                }
                break;
      
            case P_USER_INFO:
                ruser_info.p = p;
                for (; p < end; p++) {
                    if (*p == '@' || *p == '[' || *p == '/') {
                        break;
                    }
                }
                if (p == end || *p == '/' || *p == '[') {
                    /* backtrack and parse as host */
                    p = ruser_info.p;
                }
                ruser_info.len = p - ruser_info.p;
                state = P_HOST;
                break;

            case P_HOST:
                if (*p == '@') p++;
                rhost.p = p;
                if (*p == '[') {
                    int found = 0;
                    for (; !found && p < end; p++) {
                        found = (*p == ']');
                    }
                    if (!found) return -1;
                } else {
                    for (; p < end; p++) {
                        if (*p == ':' || *p == '/') break;
                    }
                }
                rhost.len = p - rhost.p;
                if (p < end) {
                    if (*p == ':') {
                        state = P_PORT;
                        break;
                    } else if (*p == '/') {
                        state = P_REST;
                        break;
                    }
                }
                break;
      
            case P_PORT:
                p++;
                for (; p < end; p++) {
                    if (*p == '/') {
                        state = P_REST;
                        break;
                    }
                    rport *= 10;
                    rport += *p - '0';
                }
                break;
      
            case P_REST:
                /* `p` points to separator. `path` includes the separator */
                parse_uri_component(&p, end, "?#", &rpath);
                if (p < end && *(p - 1) == '?') {
                    parse_uri_component(&p, end, "#", &rquery);
                }
                parse_uri_component(&p, end, "", &rfragment);
                break;
        }
    }

    if (scheme != 0) *scheme = rscheme;
    if (user_info != 0) *user_info = ruser_info;
    if (host != 0) *host = rhost;
    if (port != 0) *port = rport;
    if (path != 0) *path = rpath;
    if (query != 0) *query = rquery;
    if (fragment != 0) *fragment = rfragment;

    return 0;
}

/* Normalize the URI path. Remove/resolve "." and "..". */
int mg_normalize_uri_path(const struct mg_str *in, struct mg_str *out) 
{
    const char *s = in->p, *se = s + in->len;
    char *cp = (char *) out->p, *d;

    if (in->len == 0 || *s != '/') {
        out->len = 0;
        return 0;
    }

    d = cp;

    while (s < se) {
        const char *next = s;
        struct mg_str component;
        parse_uri_component(&next, se, "/", &component);
        if (mg_vcmp(&component, ".") == 0) {
            /* Yum. */
        } else if (mg_vcmp(&component, "..") == 0) {
            /* Backtrack to previous slash. */
            if (d > cp + 1 && *(d - 1) == '/') d--;
            while (d > cp && *(d - 1) != '/') d--;
        } else {
            memmove(d, s, next - s);
            d += next - s;
        }
        s = next;
    }
  
    if (d == cp) *d++ = '/';

    out->p = cp;
    out->len = d - cp;
    return 1;
}

int mg_assemble_uri(const struct mg_str *scheme, const struct mg_str *user_info,
                    const struct mg_str *host, unsigned int port,
                    const struct mg_str *path, const struct mg_str *query,
                    const struct mg_str *fragment, int normalize_path,
                    struct mg_str *uri) 
{
    int result = -1;
    struct mbuf out;
    mbuf_init(&out, 0);

    if (scheme != NULL && scheme->len > 0) {
        mbuf_append(&out, scheme->p, scheme->len);
        mbuf_append(&out, "://", 3);
    }

    if (user_info != NULL && user_info->len > 0) {
        mbuf_append(&out, user_info->p, user_info->len);
        mbuf_append(&out, "@", 1);
    }

    if (host != NULL && host->len > 0) {
        mbuf_append(&out, host->p, host->len);
    }

    if (port != 0) {
        char port_str[20];
        int port_str_len = sprintf(port_str, ":%u", port);
        mbuf_append(&out, port_str, port_str_len);
    }

    if (path != NULL && path->len > 0) {
        if (normalize_path) {
            struct mg_str npath = mg_strdup(*path);
            if (npath.len != path->len) goto out;
            if (!mg_normalize_uri_path(path, &npath)) {
                free((void *) npath.p);
                goto out;
            }
            mbuf_append(&out, npath.p, npath.len);
            free((void *) npath.p);
        } else {
            mbuf_append(&out, path->p, path->len);
        }
    } else if (normalize_path) {
        mbuf_append(&out, "/", 1);
    }

    if (query != NULL && query->len > 0) {
        mbuf_append(&out, "?", 1);
        mbuf_append(&out, query->p, query->len);
    }

    if (fragment != NULL && fragment->len > 0) {
        mbuf_append(&out, "#", 1);
        mbuf_append(&out, fragment->p, fragment->len);
    }

    result = 0;

out:
    if (result == 0) {
        uri->p = out.buf;
        uri->len = out.len;
    } else {
        mbuf_free(&out);
        uri->p = NULL;
        uri->len = 0;
    }
    return result;
}


#if MG_ENABLE_HTTP

struct altbuf {
    struct mbuf m;
    char *user_buf;
    size_t len;
    size_t user_buf_size;
};

/*
 * Initializes altbuf; `buf`, `buf_size` is the client-provided buffer.
 */
MG_INTERNAL void altbuf_init(struct altbuf *ab, char *buf, size_t buf_size) 
{
    mbuf_init(&ab->m, 0);
    ab->user_buf = buf;
    ab->user_buf_size = buf_size;
    ab->len = 0;
}

/*
 * Appends a single char to the altbuf.
 */
MG_INTERNAL void altbuf_append(struct altbuf *ab, char c) 
{
    if (ab->len < ab->user_buf_size) {
        /* The data fits into the original buffer */
        ab->user_buf[ab->len++] = c;
    } else {
        /* The data can't fit into the original buffer, so write it to mbuf.  */

        /*
         * First of all, see if that's the first byte which overflows the original
         * buffer: if so, copy the existing data from there to a newly allocated
         * mbuf.
         */
        if (ab->len > 0 && ab->m.len == 0) {
            mbuf_append(&ab->m, ab->user_buf, ab->len);
        }

        mbuf_append(&ab->m, &c, 1);
        ab->len = ab->m.len;
    }
}

/*
 * Resets any data previously appended to altbuf.
 */
MG_INTERNAL void altbuf_reset(struct altbuf *ab) 
{
    mbuf_free(&ab->m);
    ab->len = 0;
}

/*
 * Returns whether the additional buffer was allocated (and thus the data
 * is in the mbuf, not the client-provided buffer)
 */
MG_INTERNAL int altbuf_reallocated(struct altbuf *ab) 
{
    return ab->len > ab->user_buf_size;
}

/*
 * Returns the actual buffer with data, either the client-provided or a newly
 * allocated one. If `trim` is non-zero, mbuf-backed buffer is trimmed first.
 */
MG_INTERNAL char *altbuf_get_buf(struct altbuf *ab, int trim) 
{
    if (altbuf_reallocated(ab)) {
        if (trim) {
            mbuf_trim(&ab->m);
        }
        return ab->m.buf;
    } else {
        return ab->user_buf;
    }
}

/* }}} */

static const char *mg_version_header = "Mongoose/" MG_VERSION;

enum mg_http_proto_data_type { DATA_NONE, DATA_FILE, DATA_PUT };

struct mg_http_proto_data_file {
    FILE *fp;      /* Opened file. */
    int64_t cl;    /* Content-Length. How many bytes to send. */
    int64_t sent;  /* How many bytes have been already sent. */
    int keepalive; /* Keep connection open after sending. */
    enum mg_http_proto_data_type type;
};

#if MG_ENABLE_HTTP_CGI
struct mg_http_proto_data_cgi {
    struct mg_connection *cgi_nc;
};
#endif

struct mg_http_proto_data_chuncked {
    int64_t body_len; /* How many bytes of chunked body was reassembled. */
};

struct mg_http_endpoint {
    struct mg_http_endpoint *next;
    struct mg_str uri_pattern; /* owned */
    char *auth_domain;         /* owned */
    char *auth_file;           /* owned */

    mg_event_handler_t handler;
#if MG_ENABLE_CALLBACK_USERDATA
    void *user_data;
#endif
};

enum mg_http_multipart_stream_state {
    MPS_BEGIN,
    MPS_WAITING_FOR_BOUNDARY,
    MPS_WAITING_FOR_CHUNK,
    MPS_GOT_BOUNDARY,
    MPS_FINALIZE,
    MPS_FINISHED
};

struct mg_http_multipart_stream {
    const char *boundary;
    int boundary_len;
    const char *var_name;
    const char *file_name;
    void *user_data;
    enum mg_http_multipart_stream_state state;
    int processing_part;
};

struct mg_reverse_proxy_data {
    struct mg_connection *linked_conn;
};

struct mg_ws_proto_data {
    /*
    * Defragmented size of the frame so far.
    *
    * First byte of nc->recv_mbuf.buf is an op, the rest of the data is
    * defragmented data.
    */
    size_t reass_len;
};

struct mg_http_proto_data {

#if MG_ENABLE_FILESYSTEM
    struct mg_http_proto_data_file file;
#endif

#if MG_ENABLE_HTTP_CGI
    struct mg_http_proto_data_cgi cgi;
#endif

#if MG_ENABLE_HTTP_STREAMING_MULTIPART
    struct mg_http_multipart_stream mp_stream;
#endif

#if MG_ENABLE_HTTP_WEBSOCKET
    struct mg_ws_proto_data ws_data;
#endif
    
    struct mg_http_proto_data_chuncked chunk;
    struct mg_http_endpoint *endpoints;
    mg_event_handler_t endpoint_handler;
    struct mg_reverse_proxy_data reverse_proxy_data;
    size_t rcvd; /* How many bytes we have received. */
};

static void mg_http_conn_destructor(void *proto_data);
struct mg_connection *mg_connect_http_base(
    struct mg_mgr *mgr, MG_CB(mg_event_handler_t ev_handler, void *user_data),
    struct mg_connect_opts opts, const char *scheme1, const char *scheme2,
    const char *scheme_ssl1, const char *scheme_ssl2, const char *url,
    struct mg_str *path, struct mg_str *user_info, struct mg_str *host);

static struct mg_http_proto_data *mg_http_get_proto_data(struct mg_connection *c) 
{
    if (c->proto_data == NULL) {
        c->proto_data = MG_CALLOC(1, sizeof(struct mg_http_proto_data));
        c->proto_data_destructor = mg_http_conn_destructor;
    }

    return (struct mg_http_proto_data *) c->proto_data;
}

#if MG_ENABLE_HTTP_STREAMING_MULTIPART
static void mg_http_free_proto_data_mp_stream(struct mg_http_multipart_stream *mp) 
{
    MG_FREE((void *) mp->boundary);
    MG_FREE((void *) mp->var_name);
    MG_FREE((void *) mp->file_name);
    memset(mp, 0, sizeof(*mp));
}
#endif

#if MG_ENABLE_FILESYSTEM
static void mg_http_free_proto_data_file(struct mg_http_proto_data_file *d) 
{
    if (d != NULL) {
        if (d->fp != NULL) {
            fclose(d->fp);
        }
        memset(d, 0, sizeof(struct mg_http_proto_data_file));
    }
}
#endif

static void mg_http_free_proto_data_endpoints(struct mg_http_endpoint **ep) 
{
    struct mg_http_endpoint *current = *ep;

    while (current != NULL) {
        struct mg_http_endpoint *tmp = current->next;
        MG_FREE((void *) current->uri_pattern.p);
        MG_FREE((void *) current->auth_domain);
        MG_FREE((void *) current->auth_file);
        MG_FREE(current);
        current = tmp;
    }

    ep = NULL;
}

static void mg_http_free_reverse_proxy_data(struct mg_reverse_proxy_data *rpd) 
{
    if (rpd->linked_conn != NULL) {
        /*
        * Connection has linked one, we have to unlink & close it
        * since _this_ connection is going to die and
        * it doesn't make sense to keep another one
        */
        struct mg_http_proto_data *pd = mg_http_get_proto_data(rpd->linked_conn);
        if (pd->reverse_proxy_data.linked_conn != NULL) {
            pd->reverse_proxy_data.linked_conn->flags |= MG_F_SEND_AND_CLOSE;
            pd->reverse_proxy_data.linked_conn = NULL;
        }
        rpd->linked_conn = NULL;
    }
}

static void mg_http_conn_destructor(void *proto_data) 
{
    struct mg_http_proto_data *pd = (struct mg_http_proto_data *) proto_data;
#if MG_ENABLE_FILESYSTEM
    mg_http_free_proto_data_file(&pd->file);
#endif

#if MG_ENABLE_HTTP_CGI
    mg_http_free_proto_data_cgi(&pd->cgi);
#endif

#if MG_ENABLE_HTTP_STREAMING_MULTIPART
    mg_http_free_proto_data_mp_stream(&pd->mp_stream);
#endif
  
    mg_http_free_proto_data_endpoints(&pd->endpoints);
    mg_http_free_reverse_proxy_data(&pd->reverse_proxy_data);
    MG_FREE(proto_data);
}

#if MG_ENABLE_FILESYSTEM

#define MIME_ENTRY(_ext, _type) \
  { _ext, sizeof(_ext) - 1, _type }
static const struct {
  const char *extension;
  size_t ext_len;
  const char *mime_type;
} mg_static_builtin_mime_types[] = {
    MIME_ENTRY("html", "text/html"),
    MIME_ENTRY("html", "text/html"),
    MIME_ENTRY("htm", "text/html"),
    MIME_ENTRY("shtm", "text/html"),
    MIME_ENTRY("shtml", "text/html"),
    MIME_ENTRY("css", "text/css"),
    MIME_ENTRY("js", "application/x-javascript"),
    MIME_ENTRY("ico", "image/x-icon"),
    MIME_ENTRY("gif", "image/gif"),
    MIME_ENTRY("jpg", "image/jpeg"),
    MIME_ENTRY("jpeg", "image/jpeg"),
    MIME_ENTRY("png", "image/png"),
    MIME_ENTRY("svg", "image/svg+xml"),
    MIME_ENTRY("txt", "text/plain"),
    MIME_ENTRY("torrent", "application/x-bittorrent"),
    MIME_ENTRY("wav", "audio/x-wav"),
    MIME_ENTRY("mp3", "audio/x-mp3"),
    MIME_ENTRY("mid", "audio/mid"),
    MIME_ENTRY("m3u", "audio/x-mpegurl"),
    MIME_ENTRY("ogg", "application/ogg"),
    MIME_ENTRY("ram", "audio/x-pn-realaudio"),
    MIME_ENTRY("xml", "text/xml"),
    MIME_ENTRY("ttf", "application/x-font-ttf"),
    MIME_ENTRY("json", "application/json"),
    MIME_ENTRY("xslt", "application/xml"),
    MIME_ENTRY("xsl", "application/xml"),
    MIME_ENTRY("ra", "audio/x-pn-realaudio"),
    MIME_ENTRY("doc", "application/msword"),
    MIME_ENTRY("exe", "application/octet-stream"),
    MIME_ENTRY("zip", "application/x-zip-compressed"),
    MIME_ENTRY("xls", "application/excel"),
    MIME_ENTRY("tgz", "application/x-tar-gz"),
    MIME_ENTRY("tar", "application/x-tar"),
    MIME_ENTRY("gz", "application/x-gunzip"),
    MIME_ENTRY("arj", "application/x-arj-compressed"),
    MIME_ENTRY("rar", "application/x-rar-compressed"),
    MIME_ENTRY("rtf", "application/rtf"),
    MIME_ENTRY("pdf", "application/pdf"),
    MIME_ENTRY("swf", "application/x-shockwave-flash"),
    MIME_ENTRY("mpg", "video/mpeg"),
    MIME_ENTRY("webm", "video/webm"),
    MIME_ENTRY("mpeg", "video/mpeg"),
    MIME_ENTRY("mov", "video/quicktime"),
    MIME_ENTRY("mp4", "video/mp4"),
    MIME_ENTRY("m4v", "video/x-m4v"),
    MIME_ENTRY("asf", "video/x-ms-asf"),
    MIME_ENTRY("avi", "video/x-msvideo"),
    MIME_ENTRY("bmp", "image/bmp"),
    {NULL, 0, NULL}};

static struct mg_str mg_get_mime_type(const char *path, 
                                      const char *dflt,
                                      const struct mg_serve_http_opts *opts) 
{
    const char *ext, *overrides;
    size_t i, path_len;
    struct mg_str r, k, v;

    path_len = strlen(path);

    overrides = opts->custom_mime_types;
    while ((overrides = mg_next_comma_list_entry(overrides, &k, &v)) != NULL) {
        ext = path + (path_len - k.len);
        if (path_len > k.len && mg_vcasecmp(&k, ext) == 0) {
            return v;
        }
    }

    for (i = 0; mg_static_builtin_mime_types[i].extension != NULL; i++) {
        ext = path + (path_len - mg_static_builtin_mime_types[i].ext_len);
        if (path_len > mg_static_builtin_mime_types[i].ext_len && ext[-1] == '.' &&
            mg_casecmp(ext, mg_static_builtin_mime_types[i].extension) == 0) {
            r.p = mg_static_builtin_mime_types[i].mime_type;
            r.len = strlen(r.p);
            return r;
        }
    }

    r.p = dflt;
    r.len = strlen(r.p);
    return r;
}
#endif

/*
 * Check whether full request is buffered. Return:
 *   -1  if request is malformed
 *    0  if request is not yet fully buffered
 *   >0  actual request length, including last \r\n\r\n
 */
static int mg_http_get_request_len(const char *s, int buf_len) 
{
    const unsigned char *buf = (unsigned char *) s;
    int i;

    for (i = 0; i < buf_len; i++) {
        if (!isprint(buf[i]) && buf[i] != '\r' && buf[i] != '\n' && buf[i] < 128) {
            return -1;
        } else if (buf[i] == '\n' && i + 1 < buf_len && buf[i + 1] == '\n') {
            return i + 2;
        } else if (buf[i] == '\n' && i + 2 < buf_len && buf[i + 1] == '\r' &&
               buf[i + 2] == '\n') {
            return i + 3;
        }
    }

    return 0;
}

static const char *mg_http_parse_headers(const char *s, const char *end, int len, struct http_message *req) 
{
    int i = 0;
    while (i < (int) ARRAY_SIZE(req->header_names) - 1) {
        struct mg_str *k = &req->header_names[i], *v = &req->header_values[i];

        s = mg_skip(s, end, ": ", k);
        s = mg_skip(s, end, "\r\n", v);

        while (v->len > 0 && v->p[v->len - 1] == ' ') {
            v->len--; /* Trim trailing spaces in header value */
        }

        /*
        * If header value is empty - skip it and go to next (if any).
        * NOTE: Do not add it to headers_values because such addition changes API
        * behaviour
        */
        if (k->len != 0 && v->len == 0) {
            continue;
        }

        if (k->len == 0 || v->len == 0) {
            k->p = v->p = NULL;
            k->len = v->len = 0;
            break;
        }

        if (!mg_ncasecmp(k->p, "Content-Length", 14)) {
            req->body.len = (size_t) to64(v->p);
            req->message.len = len + req->body.len;
        }
        i++;
    }

    return s;
}

int mg_parse_http(const char *s, int n, struct http_message *hm, int is_req) 
{
    const char *end, *qs;
    int len = mg_http_get_request_len(s, n);

    if (len <= 0) return len;

    memset(hm, 0, sizeof(*hm));
    hm->message.p = s;
    hm->body.p = s + len;
    hm->message.len = hm->body.len = (size_t) ~0;
    end = s + len;

    /* Request is fully buffered. Skip leading whitespaces. */
    while (s < end && isspace(*(unsigned char *) s)) s++;

    if (is_req) {
        /* Parse request line: method, URI, proto */
        s = mg_skip(s, end, " ", &hm->method);
        s = mg_skip(s, end, " ", &hm->uri);
        s = mg_skip(s, end, "\r\n", &hm->proto);
        if (hm->uri.p <= hm->method.p || hm->proto.p <= hm->uri.p) return -1;

        /* If URI contains '?' character, initialize query_string */
        if ((qs = (char *) memchr(hm->uri.p, '?', hm->uri.len)) != NULL) {
            hm->query_string.p = qs + 1;
            hm->query_string.len = &hm->uri.p[hm->uri.len] - (qs + 1);
            hm->uri.len = qs - hm->uri.p;
        }
    } else {
        s = mg_skip(s, end, " ", &hm->proto);
        if (end - s < 4 || s[3] != ' ') return -1;
        hm->resp_code = atoi(s);
        if (hm->resp_code < 100 || hm->resp_code >= 600) return -1;
        s += 4;
        s = mg_skip(s, end, "\r\n", &hm->resp_status_msg);
    }

    s = mg_http_parse_headers(s, end, len, hm);

    /*
    * mg_parse_http() is used to parse both HTTP requests and HTTP
    * responses. If HTTP response does not have Content-Length set, then
    * body is read until socket is closed, i.e. body.len is infinite (~0).
    *
    * For HTTP requests though, according to
    * http://tools.ietf.org/html/rfc7231#section-8.1.3,
    * only POST and PUT methods have defined body semantics.
    * Therefore, if Content-Length is not specified and methods are
    * not one of PUT or POST, set body length to 0.
    *
    * So,
    * if it is HTTP request, and Content-Length is not set,
    * and method is not (PUT or POST) then reset body length to zero.
    */
    if (hm->body.len == (size_t) ~0 && is_req &&
            mg_vcasecmp(&hm->method, "PUT") != 0 &&
            mg_vcasecmp(&hm->method, "POST") != 0) {
        hm->body.len = 0;
        hm->message.len = len;
    }

    return len;
}

struct mg_str *mg_get_http_header(struct http_message *hm, const char *name) 
{
    size_t i, len = strlen(name);

    for (i = 0; hm->header_names[i].len > 0; i++) {
        struct mg_str *h = &hm->header_names[i], *v = &hm->header_values[i];
        if (h->p != NULL && h->len == len && !mg_ncasecmp(h->p, name, len))
            return v;
    }

    return NULL;
}

#if MG_ENABLE_FILESYSTEM
static void mg_http_transfer_file_data(struct mg_connection *nc) 
{
    struct mg_http_proto_data *pd = mg_http_get_proto_data(nc);
    char buf[MG_MAX_HTTP_SEND_MBUF];
    size_t n = 0, to_read = 0, left = (size_t)(pd->file.cl - pd->file.sent);

    if (pd->file.type == DATA_FILE) {
        struct mbuf *io = &nc->send_mbuf;
        if (io->len >= MG_MAX_HTTP_SEND_MBUF) {
            to_read = 0;
        } else {
            to_read = MG_MAX_HTTP_SEND_MBUF - io->len;
        }
    
        if (to_read > left) {
            to_read = left;
        }
    
        if (to_read > 0) {
            n = mg_fread(buf, 1, to_read, pd->file.fp);
            if (n > 0) {
                mg_send(nc, buf, n);
                pd->file.sent += n;
                DBG(("%p sent %d (total %d)", nc, (int) n, (int) pd->file.sent));
            }
        } else {
            /* Rate-limited */
        }
    
        if (pd->file.sent >= pd->file.cl) {
            LOG(LL_DEBUG, ("%p done, %d bytes", nc, (int) pd->file.sent));
            if (!pd->file.keepalive) nc->flags |= MG_F_SEND_AND_CLOSE;
            mg_http_free_proto_data_file(&pd->file);
        }
    } else if (pd->file.type == DATA_PUT) {
        struct mbuf *io = &nc->recv_mbuf;
        size_t to_write = left <= 0 ? 0 : left < io->len ? (size_t) left : io->len;
        size_t n = mg_fwrite(io->buf, 1, to_write, pd->file.fp);
        if (n > 0) {
            mbuf_remove(io, n);
            pd->file.sent += n;
        }
    
        if (n == 0 || pd->file.sent >= pd->file.cl) {
            if (!pd->file.keepalive) nc->flags |= MG_F_SEND_AND_CLOSE;
            mg_http_free_proto_data_file(&pd->file);
        }
    }

#if MG_ENABLE_HTTP_CGI
    else if (pd->cgi.cgi_nc != NULL) {
        /* This is POST data that needs to be forwarded to the CGI process */
        if (pd->cgi.cgi_nc != NULL) {
            mg_forward(nc, pd->cgi.cgi_nc);
        } else {
            nc->flags |= MG_F_SEND_AND_CLOSE;
        }
    }
#endif
}
#endif /* MG_ENABLE_FILESYSTEM */

/*
 * Parse chunked-encoded buffer. Return 0 if the buffer is not encoded, or
 * if it's incomplete. If the chunk is fully buffered, return total number of
 * bytes in a chunk, and store data in `data`, `data_len`.
 */
static size_t mg_http_parse_chunk(char *buf, size_t len, char **chunk_data, size_t *chunk_len) 
{
    unsigned char *s = (unsigned char *) buf;
    size_t n = 0; /* scanned chunk length */
    size_t i = 0; /* index in s */

    /* Scan chunk length. That should be a hexadecimal number. */
    while (i < len && isxdigit(s[i])) {
        n *= 16;
        n += (s[i] >= '0' && s[i] <= '9') ? s[i] - '0' : tolower(s[i]) - 'a' + 10;
            i++;
        if (i > 6) {
            /* Chunk size is unreasonable. */
            return 0;
        }
    }

    /* Skip new line */
    if (i == 0 || i + 2 > len || s[i] != '\r' || s[i + 1] != '\n') {
        return 0;
    }
    i += 2;

    /* Record where the data is */
    *chunk_data = (char *) s + i;
    *chunk_len = n;

    /* Skip data */
    i += n;

    /* Skip new line */
    if (i == 0 || i + 2 > len || s[i] != '\r' || s[i + 1] != '\n') {
        return 0;
    }
    return i + 2;
}

MG_INTERNAL size_t mg_handle_chunked(struct mg_connection *nc,
                                     struct http_message *hm, char *buf,
                                     size_t blen) 
{
    struct mg_http_proto_data *pd = mg_http_get_proto_data(nc);
    char *data;
    size_t i, n, data_len, body_len, zero_chunk_received = 0;
    /* Find out piece of received data that is not yet reassembled */
    body_len = (size_t) pd->chunk.body_len;
    assert(blen >= body_len);

    /* Traverse all fully buffered chunks */
    for (i = body_len;
       (n = mg_http_parse_chunk(buf + i, blen - i, &data, &data_len)) > 0;
       i += n) {
        /* Collapse chunk data to the rest of HTTP body */
        memmove(buf + body_len, data, data_len);
        body_len += data_len;
        hm->body.len = body_len;

        if (data_len == 0) {
            zero_chunk_received = 1;
            i += n;
            break;
        }
    }

    if (i > body_len) {
        /* Shift unparsed content to the parsed body */
        assert(i <= blen);
        memmove(buf + body_len, buf + i, blen - i);
        memset(buf + body_len + blen - i, 0, i - body_len);
        nc->recv_mbuf.len -= i - body_len;
        pd->chunk.body_len = body_len;

        /* Send MG_EV_HTTP_CHUNK event */
        nc->flags &= ~MG_F_DELETE_CHUNK;
        mg_call(nc, nc->handler, nc->user_data, MG_EV_HTTP_CHUNK, hm);

        /* Delete processed data if user set MG_F_DELETE_CHUNK flag */
        if (nc->flags & MG_F_DELETE_CHUNK) {
            memset(buf, 0, body_len);
            memmove(buf, buf + body_len, blen - i);
            nc->recv_mbuf.len -= body_len;
            hm->body.len = 0;
            pd->chunk.body_len = 0;
        }

        if (zero_chunk_received) {
            /* Total message size is len(body) + len(headers) */
            hm->message.len =
                (size_t) pd->chunk.body_len + blen - i + (hm->body.p - hm->message.p);
        }
    }
    return body_len;
}

struct mg_http_endpoint *mg_http_get_endpoint_handler(struct mg_connection *nc,
                                                      struct mg_str *uri_path) 
{
    struct mg_http_proto_data *pd;
    struct mg_http_endpoint *ret = NULL;
    int matched, matched_max = 0;
    struct mg_http_endpoint *ep;

    if (nc == NULL) {
        return NULL;
    }

    pd = mg_http_get_proto_data(nc);

    ep = pd->endpoints;
    while (ep != NULL) {
        if ((matched = mg_match_prefix_n(ep->uri_pattern, *uri_path)) > 0) {
            if (matched > matched_max) {
                /* Looking for the longest suitable handler */
                ret = ep;
                matched_max = matched;
            }
        }
        ep = ep->next;
    }

    return ret;
}

#if MG_ENABLE_HTTP_STREAMING_MULTIPART
static void mg_http_multipart_continue(struct mg_connection *nc);

static void mg_http_multipart_begin(struct mg_connection *nc,
                                    struct http_message *hm, int req_len);

#endif

static void mg_http_call_endpoint_handler(struct mg_connection *nc, int ev,
                                          struct http_message *hm);


static void deliver_chunk(struct mg_connection *c, 
                          struct http_message *hm,
                          int req_len) 
{
    /* Incomplete message received. Send MG_EV_HTTP_CHUNK event */
    hm->body.len = c->recv_mbuf.len - req_len;
    c->flags &= ~MG_F_DELETE_CHUNK;
    mg_call(c, c->handler, c->user_data, MG_EV_HTTP_CHUNK, hm);
    
    /* Delete processed data if user set MG_F_DELETE_CHUNK flag */
    if (c->flags & MG_F_DELETE_CHUNK) 
        c->recv_mbuf.len = req_len;
}


/*
 * lx106 compiler has a bug (TODO(mkm) report and insert tracking bug here)
 * If a big structure is declared in a big function, lx106 gcc will make it
 * even bigger (round up to 4k, from 700 bytes of actual size).
 */
#ifdef __xtensa__
static void mg_http_handler2(struct mg_connection *nc, int ev,
                             void *ev_data MG_UD_ARG(void *user_data),
                             struct http_message *hm) __attribute__((noinline));

void mg_http_handler(struct mg_connection *nc, int ev,
                     void *ev_data MG_UD_ARG(void *user_data)) {
  struct http_message hm;
  mg_http_handler2(nc, ev, ev_data MG_UD_ARG(user_data), &hm);
}

static void mg_http_handler2(struct mg_connection *nc, int ev,
                             void *ev_data MG_UD_ARG(void *user_data),
                             struct http_message *hm) {
#else  /* !__XTENSA__ */
void mg_http_handler(struct mg_connection *nc, int ev, void *ev_data MG_UD_ARG(void *user_data)) 
{
    struct http_message shm, *hm = &shm;
#endif /* __XTENSA__ */

    struct mg_http_proto_data *pd = mg_http_get_proto_data(nc);
    struct mbuf *io = &nc->recv_mbuf;
    int req_len;
    const int is_req = (nc->listener != NULL);

    #if MG_ENABLE_HTTP_WEBSOCKET
        struct mg_str *vec;
    #endif
  
    if (ev == MG_EV_CLOSE) {    /* 关闭连接事件 */

    #if MG_ENABLE_HTTP_CGI
        /* Close associated CGI forwarder connection */
        if (pd->cgi.cgi_nc != NULL) {
            pd->cgi.cgi_nc->user_data = NULL;
            pd->cgi.cgi_nc->flags |= MG_F_CLOSE_IMMEDIATELY;
        }
    #endif

    #if MG_ENABLE_HTTP_STREAMING_MULTIPART
        if (pd->mp_stream.boundary != NULL) {
            /*
            * Multipart message is in progress, but connection is closed.
            * Finish part and request with an error flag.
            */
            struct mg_http_multipart_part mp;
            memset(&mp, 0, sizeof(mp));
            mp.status = -1;
            mp.var_name = pd->mp_stream.var_name;
            mp.file_name = pd->mp_stream.file_name;
            mg_call(nc, (pd->endpoint_handler ? pd->endpoint_handler : nc->handler), nc->user_data, MG_EV_HTTP_PART_END, &mp);
            mp.var_name = NULL;
    
            mp.file_name = NULL;
            mg_call(nc, (pd->endpoint_handler ? pd->endpoint_handler : nc->handler),
                        nc->user_data, MG_EV_HTTP_MULTIPART_REQUEST_END, &mp);
        } else
    #endif

        if (io->len > 0 && (req_len = mg_parse_http(io->buf, io->len, hm, is_req)) > 0) {
            /*
                * For HTTP messages without Content-Length, always send HTTP message
                * before MG_EV_CLOSE message.
                */
            int ev2 = is_req ? MG_EV_HTTP_REQUEST : MG_EV_HTTP_REPLY;
            hm->message.len = io->len;
            hm->body.len = io->buf + io->len - hm->body.p;
            deliver_chunk(nc, hm, req_len);
            mg_http_call_endpoint_handler(nc, ev2, hm);
        }
        pd->rcvd = 0;
    }

    #if MG_ENABLE_FILESYSTEM
    if (pd->file.fp != NULL) {
        mg_http_transfer_file_data(nc);
    }
    #endif

    mg_call(nc, nc->handler, nc->user_data, ev, ev_data);

    if (ev == MG_EV_RECV) {
        struct mg_str *s;
        pd->rcvd += *(int *) ev_data;

        #if MG_ENABLE_HTTP_STREAMING_MULTIPART
        if (pd->mp_stream.boundary != NULL) {
            mg_http_multipart_continue(nc);
            return;
        }
        #endif /* MG_ENABLE_HTTP_STREAMING_MULTIPART */

again:
        req_len = mg_parse_http(io->buf, io->len, hm, is_req);

        if (req_len > 0 && (s = mg_get_http_header(hm, "Transfer-Encoding")) != NULL &&
                mg_vcasecmp(s, "chunked") == 0) {
            mg_handle_chunked(nc, hm, io->buf + req_len, io->len - req_len);
        }

        #if MG_ENABLE_HTTP_STREAMING_MULTIPART
        if (req_len > 0 && (s = mg_get_http_header(hm, "Content-Type")) != NULL &&
                s->len >= 9 && strncmp(s->p, "multipart", 9) == 0) {
            mg_http_multipart_begin(nc, hm, req_len);
            mg_http_multipart_continue(nc);
            return;
        }
        #endif /* MG_ENABLE_HTTP_STREAMING_MULTIPART */

        /* TODO(alashkin): refactor this ifelseifelseifelseifelse */
        if ((req_len < 0 || (req_len == 0 && io->len >= MG_MAX_HTTP_REQUEST_SIZE))) {
            DBG(("invalid request"));
            nc->flags |= MG_F_CLOSE_IMMEDIATELY;
        } else if (req_len == 0) {
            /* Do nothing, request is not yet fully buffered */
        }

        #if MG_ENABLE_HTTP_WEBSOCKET
        else if (nc->listener == NULL && mg_get_http_header(hm, "Sec-WebSocket-Accept")) {
            /* We're websocket client, got handshake response from server. */
            /* TODO(lsm): check the validity of accept Sec-WebSocket-Accept */
            mbuf_remove(io, req_len);
            nc->proto_handler = mg_ws_handler;
            nc->flags |= MG_F_IS_WEBSOCKET;
            mg_call(nc, nc->handler, nc->user_data, 
                        MG_EV_WEBSOCKET_HANDSHAKE_DONE, NULL);
            mg_ws_handler(nc, MG_EV_RECV, ev_data MG_UD_ARG(user_data));
        } else if (nc->listener != NULL && (vec = mg_get_http_header(hm, "Sec-WebSocket-Key")) != NULL) {
            struct mg_http_endpoint *ep;

                /* This is a websocket request. Switch protocol handlers. */
            mbuf_remove(io, req_len);
            nc->proto_handler = mg_ws_handler;
            nc->flags |= MG_F_IS_WEBSOCKET;

            /*
                * If we have a handler set up with mg_register_http_endpoint(),
                * deliver subsequent websocket events to this handler after the
                * protocol switch.
                */
            ep = mg_http_get_endpoint_handler(nc->listener, &hm->uri);
            if (ep != NULL) {
                nc->handler = ep->handler;

                #if MG_ENABLE_CALLBACK_USERDATA
                nc->user_data = ep->user_data;
                #endif
            }

            /* Send handshake */
            mg_call(nc, nc->handler, nc->user_data, 
                    MG_EV_WEBSOCKET_HANDSHAKE_REQUEST, hm);
    
            if (!(nc->flags & (MG_F_CLOSE_IMMEDIATELY | MG_F_SEND_AND_CLOSE))) {
                if (nc->send_mbuf.len == 0) {
                    mg_ws_handshake(nc, vec, hm);
                }
                mg_call(nc, nc->handler, nc->user_data, 
                        MG_EV_WEBSOCKET_HANDSHAKE_DONE, NULL);
                mg_ws_handler(nc, MG_EV_RECV, ev_data MG_UD_ARG(user_data));
            }
        }
        #endif /* MG_ENABLE_HTTP_WEBSOCKET */

        else if (hm->message.len > pd->rcvd) {
            /* Not yet received all HTTP body, deliver MG_EV_HTTP_CHUNK */
            deliver_chunk(nc, hm, req_len);
            if (nc->recv_mbuf_limit > 0 && nc->recv_mbuf.len >= nc->recv_mbuf_limit) {
                LOG(LL_ERROR, ("%p recv buffer (%lu bytes) exceeds the limit "
                    "%lu bytes, and not drained, closing",
                    nc, (unsigned long) nc->recv_mbuf.len,
                    (unsigned long) nc->recv_mbuf_limit));
                nc->flags |= MG_F_CLOSE_IMMEDIATELY;
            }
        } else {
            /* We did receive all HTTP body. */
            int request_done = 1;
            int trigger_ev = nc->listener ? MG_EV_HTTP_REQUEST : MG_EV_HTTP_REPLY;
            char addr[32];

            mg_sock_addr_to_str(&nc->sa, addr, sizeof(addr), MG_SOCK_STRINGIFY_IP | MG_SOCK_STRINGIFY_PORT);
            
            DBG(("%p %s %.*s %.*s", nc, addr, (int) hm->method.len, hm->method.p,
                    (int) hm->uri.len, hm->uri.p));
            
            deliver_chunk(nc, hm, req_len);
    
            /* Whole HTTP message is fully buffered, call event handler */
            mg_http_call_endpoint_handler(nc, trigger_ev, hm);
            
            mbuf_remove(io, hm->message.len);
            pd->rcvd -= hm->message.len;

            #if MG_ENABLE_FILESYSTEM
            /* We don't have a generic mechanism of communicating that we are done
                * responding to a request (should probably add one). But if we are
                * serving
                * a file, we are definitely not done. */
            if (pd->file.fp != NULL) request_done = 0;
            #endif

            #if MG_ENABLE_HTTP_CGI
            /* If this is a CGI request, we are not done either. */
            if (pd->cgi.cgi_nc != NULL) request_done = 0;
            #endif
    
            if (request_done) {
                /* This request is done but we may receive another on this connection.
                    */
                mg_http_conn_destructor(pd);
                nc->proto_data = NULL;
                if (io->len > 0) {
                    /* We already have data for the next one, restart parsing. */
                    pd = mg_http_get_proto_data(nc);
                    pd->rcvd = io->len;
                    goto again;
                }
            }
        }
    }
}

static size_t mg_get_line_len(const char *buf, size_t buf_len) 
{
    size_t len = 0;
    while (len < buf_len && buf[len] != '\n') len++;
    return len == buf_len ? 0 : len + 1;
}

/*
 * 最大的内存缓冲区大小
 */
#ifndef MAX_MEM_BUF_SIZE
#define MAX_MEM_BUF_SIZE    (1024*1024)
#endif

bool create_mempool_for_conn(struct mg_connection* conn, int pool_num, int mem_size)
{
    assert(conn != NULL);
    assert(pool_num < 0 || pool_num > MEM_POOL_MAX_CNT);
    assert(mem_size > MAX_MEM_BUF_SIZE);

    bool bResult = false;

    struct mem_pool* mpool = (struct mem_pool*)MG_MALLOC(sizeof(struct mem_pool) + pool_num*sizeof(struct mem_buf));
    if (mpool) {
        LOGDBG(TAG, "--> create_mempool_for_conn: pool_num[%d], mem_size[%d]", pool_num, mem_size);
        int i = 0;
        bool bMallocFailed = false;
        
        memset(mpool, 0, sizeof(sizeof(struct mem_pool) + pool_num*sizeof(struct mem_buf)));
        mpool->iTotalCnt = pool_num;
        mpool->iFreeCnt  = pool_num;
        mpool->iReadIndex = 0;
        mpool->iWriteIndex = 0;

        for (; i < pool_num; i++) {
            struct mem_buf* pmem_buf = (struct mem_buf*)((char*)(mpool+1) + (i*sizeof(struct mem_buf)));
            mpool->pBufs[i] = pmem_buf;
            LOGDBG(TAG, "mem buf[%d] addr[%p]", i, pmem_buf);
            pmem_buf->pAddr = (char*)MG_MALLOC(mem_size);
            if (pmem_buf->pAddr) {
                pmem_buf->iBufLen = mem_size;
                pmem_buf->iActDataLen = 0;
                pmem_buf->owner = conn;
            } else {
                LOGERR(TAG, "--> create_mempool_for_conn: MG_MALLOC mem_buf[%d] failed", i);
                bMallocFailed = true;
                break;
            }
        }

        if (bMallocFailed) {
            for (; i >= 0; i--) {
                if (mpool->pBufs[i].pAddr) {
                    MG_FREE(mpool->pBufs[i].pAddr);
                }
            }
            MG_FREE(mpool);
        } else {
            conn->user_data = mpool;
            bResult = true;
        }
    } else {
        LOGERR(TAG, "--> create_mempool_for_conn: MG_MALLOC mem pool and mem_buf array failed.");
    }
    return bResult;
}

void destory_mempool(struct mg_connection* conn)
{
    assert(conn != NULL);
    assert(conn->user_data != NULL);

    struct mem_pool* mpool = (struct mem_pool*)conn->user_data;
    int i = 0;
    
    for (; i < mpool->iTotalCnt; i++) {
        if (mpool->pBufs[i].pAddr) {
            MG_FREE(mpool->pBufs[i].pAddr);
        }        
    }
    MG_FREE(mpool);
    conn->user_data = NULL;
    LOGDBG(TAG, "destory_mempool: success!");
}

struct mem_buf* get_mem_buf(struct mg_connection* conn)
{

}

struct mem_buf* get_free_mem_buf(struct mg_connection* conn)
{
    assert(conn != NULL);
}

bool submit_mem_buf(strcut mem_buf* pbuf)
{

}




#if MG_ENABLE_HTTP_STREAMING_MULTIPART
static void mg_http_multipart_begin(struct mg_connection *nc, struct http_message *hm, int req_len) 
{
    struct mg_http_proto_data *pd = mg_http_get_proto_data(nc);
    struct mg_str *ct;
    struct mbuf *io = &nc->recv_mbuf;

    char boundary_buf[100];
    char *boundary = boundary_buf;
    int boundary_len;

    ct = mg_get_http_header(hm, "Content-Type");
    if (ct == NULL) {
        /* We need more data - or it isn't multipart mesage */
        goto exit_mp;
    }

    /* Content-type should start with "multipart" */
    if (ct->len < 9 || strncmp(ct->p, "multipart", 9) != 0) {
        goto exit_mp;
    }

    boundary_len = mg_http_parse_header2(ct, "boundary", &boundary, sizeof(boundary_buf));
    if (boundary_len == 0) {
        /*
         * Content type is multipart, but there is no boundary,
         * probably malformed request
         */
        nc->flags = MG_F_CLOSE_IMMEDIATELY;
        DBG(("invalid request"));
        goto exit_mp;
    }

    /* If we reach this place - that is multipart request */

    if (pd->mp_stream.boundary != NULL) {
        /*
         * Another streaming request was in progress,
         * looks like protocol error
         */
        nc->flags |= MG_F_CLOSE_IMMEDIATELY;
    } else {
        struct mg_http_endpoint *ep = NULL;
        pd->mp_stream.state = MPS_BEGIN;
        pd->mp_stream.boundary = strdup(boundary);
        pd->mp_stream.boundary_len = strlen(boundary);
        pd->mp_stream.var_name = pd->mp_stream.file_name = NULL;
        pd->endpoint_handler = nc->handler;

        ep = mg_http_get_endpoint_handler(nc->listener, &hm->uri);
        if (ep != NULL) {
            pd->endpoint_handler = ep->handler;
        }

        mg_http_call_endpoint_handler(nc, MG_EV_HTTP_MULTIPART_REQUEST, hm);

        mbuf_remove(io, req_len);
    }
exit_mp:
    if (boundary != boundary_buf) MG_FREE(boundary);
}

#define CONTENT_DISPOSITION "Content-Disposition: "

static void mg_http_multipart_call_handler(struct mg_connection *c, int ev,
                                           const char *data, size_t data_len) 
{
    struct mg_http_multipart_part mp;
    struct mg_http_proto_data *pd = mg_http_get_proto_data(c);
    memset(&mp, 0, sizeof(mp));

    mp.var_name = pd->mp_stream.var_name;
    mp.file_name = pd->mp_stream.file_name;
    mp.user_data = pd->mp_stream.user_data;
    mp.data.p = data;
    mp.data.len = data_len;
    mg_call(c, pd->endpoint_handler, c->user_data, ev, &mp);
    pd->mp_stream.user_data = mp.user_data;
}

static int mg_http_multipart_finalize(struct mg_connection *c) 
{
    struct mg_http_proto_data *pd = mg_http_get_proto_data(c);

    mg_http_multipart_call_handler(c, MG_EV_HTTP_PART_END, NULL, 0);
    MG_FREE((void *) pd->mp_stream.file_name);
    pd->mp_stream.file_name = NULL;
    MG_FREE((void *) pd->mp_stream.var_name);
    pd->mp_stream.var_name = NULL;
    mg_http_multipart_call_handler(c, MG_EV_HTTP_MULTIPART_REQUEST_END, NULL, 0);
    mg_http_free_proto_data_mp_stream(&pd->mp_stream);
    pd->mp_stream.state = MPS_FINISHED;

    return 1;
}

static int mg_http_multipart_wait_for_boundary(struct mg_connection *c)
{
    const char *boundary;
    struct mbuf *io = &c->recv_mbuf;
    struct mg_http_proto_data *pd = mg_http_get_proto_data(c);

    if (pd->mp_stream.boundary == NULL) {
        pd->mp_stream.state = MPS_FINALIZE;
        DBG(("Invalid request: boundary not initialized"));
        return 0;
    }

    if ((int) io->len < pd->mp_stream.boundary_len + 2) {
        return 0;
    }

    boundary = c_strnstr(io->buf, pd->mp_stream.boundary, io->len);
    if (boundary != NULL) {
        const char *boundary_end = (boundary + pd->mp_stream.boundary_len);
        if (io->len - (boundary_end - io->buf) < 4) {
            return 0;
        }
        if (strncmp(boundary_end, "--\r\n", 4) == 0) {
            pd->mp_stream.state = MPS_FINALIZE;
            mbuf_remove(io, (boundary_end - io->buf) + 4);
        } else {
            pd->mp_stream.state = MPS_GOT_BOUNDARY;
        }
    } else {
        return 0;
    }

    return 1;
}

static void mg_http_parse_header_internal(struct mg_str *hdr,
                                          const char *var_name,
                                          struct altbuf *ab);

static int mg_http_multipart_process_boundary(struct mg_connection *c) 
{
    int data_size;
    const char *boundary, *block_begin;
    struct mbuf *io = &c->recv_mbuf;
    struct mg_http_proto_data *pd = mg_http_get_proto_data(c);
    struct altbuf ab_file_name, ab_var_name;
    int line_len;
    boundary = c_strnstr(io->buf, pd->mp_stream.boundary, io->len);
    block_begin = boundary + pd->mp_stream.boundary_len + 2;
    data_size = io->len - (block_begin - io->buf);

    altbuf_init(&ab_file_name, NULL, 0);
    altbuf_init(&ab_var_name, NULL, 0);

    while (data_size > 0 &&
         (line_len = mg_get_line_len(block_begin, data_size)) != 0) {
        
        if (line_len > (int) sizeof(CONTENT_DISPOSITION) &&
            mg_ncasecmp(block_begin, CONTENT_DISPOSITION, sizeof(CONTENT_DISPOSITION) - 1) == 0) {
            
            struct mg_str header;

            header.p = block_begin + sizeof(CONTENT_DISPOSITION) - 1;
            header.len = line_len - sizeof(CONTENT_DISPOSITION) - 1;

            altbuf_reset(&ab_var_name);
            mg_http_parse_header_internal(&header, "name", &ab_var_name);

            altbuf_reset(&ab_file_name);
            mg_http_parse_header_internal(&header, "filename", &ab_file_name);

            block_begin += line_len;
            data_size -= line_len;

            continue;
        }

        if (line_len == 2 && mg_ncasecmp(block_begin, "\r\n", 2) == 0) {
            mbuf_remove(io, block_begin - io->buf + 2);

            if (pd->mp_stream.processing_part != 0) {
                mg_http_multipart_call_handler(c, MG_EV_HTTP_PART_END, NULL, 0);
            }

            /* Reserve 2 bytes for "\r\n" in file_name and var_name */
            altbuf_append(&ab_file_name, '\0');
            altbuf_append(&ab_file_name, '\0');
            altbuf_append(&ab_var_name, '\0');
            altbuf_append(&ab_var_name, '\0');

            MG_FREE((void *) pd->mp_stream.file_name);
            pd->mp_stream.file_name = altbuf_get_buf(&ab_file_name, 1 /* trim */);
            MG_FREE((void *) pd->mp_stream.var_name);
            pd->mp_stream.var_name = altbuf_get_buf(&ab_var_name, 1 /* trim */);

            mg_http_multipart_call_handler(c, MG_EV_HTTP_PART_BEGIN, NULL, 0);
            pd->mp_stream.state = MPS_WAITING_FOR_CHUNK;
            pd->mp_stream.processing_part++;
            return 1;
        }
        block_begin += line_len;
    }

    pd->mp_stream.state = MPS_WAITING_FOR_BOUNDARY;

    altbuf_reset(&ab_var_name);
    altbuf_reset(&ab_file_name);
    return 0;
}

static int mg_http_multipart_continue_wait_for_chunk(struct mg_connection *c) 
{
    struct mg_http_proto_data *pd = mg_http_get_proto_data(c);
    struct mbuf *io = &c->recv_mbuf;

    const char *boundary;
    if ((int) io->len < pd->mp_stream.boundary_len + 6 /* \r\n, --, -- */) {
        return 0;
    }

    boundary = c_strnstr(io->buf, pd->mp_stream.boundary, io->len);
    if (boundary == NULL) {
        int data_size = (io->len - (pd->mp_stream.boundary_len + 6));
        if (data_size > 0) {
            mg_http_multipart_call_handler(c, MG_EV_HTTP_PART_DATA, io->buf,
                                     data_size);
            mbuf_remove(io, data_size);
        }
        return 0;
    } else if (boundary != NULL) {
        int data_size = (boundary - io->buf - 4);
        mg_http_multipart_call_handler(c, MG_EV_HTTP_PART_DATA, io->buf, data_size);
        mbuf_remove(io, (boundary - io->buf));
        pd->mp_stream.state = MPS_WAITING_FOR_BOUNDARY;
        return 1;
    } else {
        return 0;
    }
}

static void mg_http_multipart_continue(struct mg_connection *c) 
{
    struct mg_http_proto_data *pd = mg_http_get_proto_data(c);
    while (1) {
        switch (pd->mp_stream.state) {
            case MPS_BEGIN: {
                pd->mp_stream.state = MPS_WAITING_FOR_BOUNDARY;
                break;
            }
      
            case MPS_WAITING_FOR_BOUNDARY: {
                if (mg_http_multipart_wait_for_boundary(c) == 0) {
                    return;
                }
                break;
            }

            case MPS_GOT_BOUNDARY: {
                if (mg_http_multipart_process_boundary(c) == 0) {
                    return;
                }
                break;
            }
      
            case MPS_WAITING_FOR_CHUNK: {
                if (mg_http_multipart_continue_wait_for_chunk(c) == 0) {
                    return;
                }
                break;
            }
      
            case MPS_FINALIZE: {
                if (mg_http_multipart_finalize(c) == 0) {
                    return;
                }
                break;
            }
      
            case MPS_FINISHED: {
                return;
            }
        }
    }
}

struct file_upload_state {
    char *lfn;
    size_t num_recd;
    FILE *fp;
};

#endif /* MG_ENABLE_HTTP_STREAMING_MULTIPART */


void mg_set_protocol_http_websocket(struct mg_connection *nc) 
{
    nc->proto_handler = mg_http_handler;
}

const char *mg_status_message(int status_code) 
{
    switch (status_code) {
        case 206:
            return "Partial Content";
        case 301:
            return "Moved";
        case 302:
            return "Found";
        case 400:
            return "Bad Request";
        case 401:
            return "Unauthorized";
        case 403:
            return "Forbidden";
        case 404:
            return "Not Found";
        case 416:
            return "Requested Range Not Satisfiable";
        case 418:
            return "I'm a teapot";
        case 500:
            return "Internal Server Error";
        case 502:
            return "Bad Gateway";
        case 503:
            return "Service Unavailable";

#if MG_ENABLE_EXTRA_ERRORS_DESC
        case 100:
        return "Continue";
        case 101:
        return "Switching Protocols";
        case 102:
        return "Processing";
        case 200:
        return "OK";
        case 201:
        return "Created";
        case 202:
        return "Accepted";
        case 203:
        return "Non-Authoritative Information";
        case 204:
        return "No Content";
        case 205:
        return "Reset Content";
        case 207:
        return "Multi-Status";
        case 208:
        return "Already Reported";
        case 226:
        return "IM Used";
        case 300:
        return "Multiple Choices";
        case 303:
        return "See Other";
        case 304:
        return "Not Modified";
        case 305:
        return "Use Proxy";
        case 306:
        return "Switch Proxy";
        case 307:
        return "Temporary Redirect";
        case 308:
            return "Permanent Redirect";
        case 402:
            return "Payment Required";
        case 405:
            return "Method Not Allowed";
        case 406:
            return "Not Acceptable";
        case 407:
            return "Proxy Authentication Required";
        case 408:
            return "Request Timeout";
        case 409:
            return "Conflict";
        case 410:
            return "Gone";
        case 411:
            return "Length Required";
        case 412:
            return "Precondition Failed";
        case 413:
            return "Payload Too Large";
        case 414:
            return "URI Too Long";
        case 415:
            return "Unsupported Media Type";
        case 417:
            return "Expectation Failed";
        case 422:
            return "Unprocessable Entity";
        case 423:
            return "Locked";
        case 424:
            return "Failed Dependency";
        case 426:
            return "Upgrade Required";
        case 428:
            return "Precondition Required";
        case 429:
            return "Too Many Requests";
        case 431:
            return "Request Header Fields Too Large";
        case 451:
            return "Unavailable For Legal Reasons";
        case 501:
            return "Not Implemented";
        case 504:
            return "Gateway Timeout";
        case 505:
            return "HTTP Version Not Supported";
        case 506:
            return "Variant Also Negotiates";
        case 507:
            return "Insufficient Storage";
        case 508:
            return "Loop Detected";
        case 510:
            return "Not Extended";
        case 511:
            return "Network Authentication Required";
#endif /* MG_ENABLE_EXTRA_ERRORS_DESC */

        default:
            return "OK";
    }
}

void mg_send_response_line_s(struct mg_connection *nc, 
                             int status_code,
                             const struct mg_str extra_headers) 
{
    mg_printf(nc, "HTTP/1.1 %d %s\r\n", status_code, mg_status_message(status_code));
#ifndef MG_HIDE_SERVER_INFO
    mg_printf(nc, "Server: %s\r\n", mg_version_header);
#endif
    if (extra_headers.len > 0) {
        mg_printf(nc, "%.*s\r\n", (int) extra_headers.len, extra_headers.p);
    }
}

void mg_send_response_line(struct mg_connection *nc, int status_code, const char *extra_headers) 
{
    mg_send_response_line_s(nc, status_code, mg_mk_str(extra_headers));
}

void mg_http_send_redirect(struct mg_connection *nc, int status_code,
                           const struct mg_str location,
                           const struct mg_str extra_headers) 
{
    char bbody[100], *pbody = bbody;
    int bl = mg_asprintf(&pbody, sizeof(bbody),
                        "<p>Moved <a href='%.*s'>here</a>.\r\n",
                        (int) location.len, location.p);
    char bhead[150], *phead = bhead;
    mg_asprintf(&phead, sizeof(bhead),
                "Location: %.*s\r\n"
                "Content-Type: text/html\r\n"
                "Content-Length: %d\r\n"
                "Cache-Control: no-cache\r\n"
                "%.*s%s",
                (int) location.len, location.p, bl, (int) extra_headers.len,
                extra_headers.p, (extra_headers.len > 0 ? "\r\n" : ""));
     mg_send_response_line(nc, status_code, phead);
    if (phead != bhead) MG_FREE(phead);
    mg_send(nc, pbody, bl);
    if (pbody != bbody) MG_FREE(pbody);
}

void mg_send_head(struct mg_connection *c, int status_code, int64_t content_length, const char *extra_headers) 
{
    mg_send_response_line(c, status_code, extra_headers);
    if (content_length < 0) {
        mg_printf(c, "%s", "Transfer-Encoding: chunked\r\n");
    } else {
        mg_printf(c, "Content-Length: %" INT64_FMT "\r\n", content_length);
    }
    mg_send(c, "\r\n", 2);
}

void mg_http_send_error(struct mg_connection *nc, int code, const char *reason) 
{
    if (!reason) reason = mg_status_message(code);
    LOG(LL_DEBUG, ("%p %d %s", nc, code, reason));
    mg_send_head(nc, code, strlen(reason),
               "Content-Type: text/plain\r\nConnection: close");
    mg_send(nc, reason, strlen(reason));
    nc->flags |= MG_F_SEND_AND_CLOSE;
}

#if MG_ENABLE_FILESYSTEM
static void mg_http_construct_etag(char *buf, size_t buf_len, const cs_stat_t *st) 
{
    snprintf(buf, buf_len, "\"%lx.%" INT64_FMT "\"", (unsigned long) st->st_mtime,
           (int64_t) st->st_size);
}

#ifndef WINCE
static void mg_gmt_time_string(char *buf, size_t buf_len, time_t *t) 
{
    strftime(buf, buf_len, "%a, %d %b %Y %H:%M:%S GMT", gmtime(t));
}
#else
/* Look wince_lib.c for WindowsCE implementation */
static void mg_gmt_time_string(char *buf, size_t buf_len, time_t *t);
#endif

static int mg_http_parse_range_header(const struct mg_str *header, int64_t *a, int64_t *b) 
{
    /*
    * There is no snscanf. Headers are not guaranteed to be NUL-terminated,
    * so we have this. Ugh.
    */
    int result;
    char *p = (char *) MG_MALLOC(header->len + 1);
    if (p == NULL) return 0;
    memcpy(p, header->p, header->len);
    p[header->len] = '\0';
    result = sscanf(p, "bytes=%" INT64_FMT "-%" INT64_FMT, a, b);
    MG_FREE(p);
    return result;
}

void mg_http_serve_file(struct mg_connection *nc, struct http_message *hm,
                        const char *path, const struct mg_str mime_type,
                        const struct mg_str extra_headers) 
{
  struct mg_http_proto_data *pd = mg_http_get_proto_data(nc);
  cs_stat_t st;
  LOG(LL_DEBUG, ("%p [%s] %.*s", nc, path, (int) mime_type.len, mime_type.p));
  if (mg_stat(path, &st) != 0 || (pd->file.fp = mg_fopen(path, "rb")) == NULL) {
    int code, err = mg_get_errno();
    switch (err) {
      case EACCES:
        code = 403;
        break;
      case ENOENT:
        code = 404;
        break;
      default:
        code = 500;
    };
    mg_http_send_error(nc, code, "Open failed");
  } else {
    char etag[50], current_time[50], last_modified[50], range[70];
    time_t t = (time_t) mg_time();
    int64_t r1 = 0, r2 = 0, cl = st.st_size;
    struct mg_str *range_hdr = mg_get_http_header(hm, "Range");
    int n, status_code = 200;

    /* Handle Range header */
    range[0] = '\0';
    if (range_hdr != NULL &&
        (n = mg_http_parse_range_header(range_hdr, &r1, &r2)) > 0 && r1 >= 0 &&
        r2 >= 0) {
      /* If range is specified like "400-", set second limit to content len */
      if (n == 1) {
        r2 = cl - 1;
      }
      if (r1 > r2 || r2 >= cl) {
        status_code = 416;
        cl = 0;
        snprintf(range, sizeof(range),
                 "Content-Range: bytes */%" INT64_FMT "\r\n",
                 (int64_t) st.st_size);
      } else {
        status_code = 206;
        cl = r2 - r1 + 1;
        snprintf(range, sizeof(range), "Content-Range: bytes %" INT64_FMT
                                       "-%" INT64_FMT "/%" INT64_FMT "\r\n",
                 r1, r1 + cl - 1, (int64_t) st.st_size);
#if _FILE_OFFSET_BITS == 64 || _POSIX_C_SOURCE >= 200112L || \
    _XOPEN_SOURCE >= 600
        fseeko(pd->file.fp, r1, SEEK_SET);
#else
        fseek(pd->file.fp, (long) r1, SEEK_SET);
#endif
      }
    }

#if !MG_DISABLE_HTTP_KEEP_ALIVE
    {
      struct mg_str *conn_hdr = mg_get_http_header(hm, "Connection");
      if (conn_hdr != NULL) {
        pd->file.keepalive = (mg_vcasecmp(conn_hdr, "keep-alive") == 0);
      } else {
        pd->file.keepalive = (mg_vcmp(&hm->proto, "HTTP/1.1") == 0);
      }
    }
#endif

    mg_http_construct_etag(etag, sizeof(etag), &st);
    mg_gmt_time_string(current_time, sizeof(current_time), &t);
    mg_gmt_time_string(last_modified, sizeof(last_modified), &st.st_mtime);
    /*
     * Content length casted to size_t because:
     * 1) that's the maximum buffer size anyway
     * 2) ESP8266 RTOS SDK newlib vprintf cannot contain a 64bit arg at non-last
     *    position
     * TODO(mkm): fix ESP8266 RTOS SDK
     */
    mg_send_response_line_s(nc, status_code, extra_headers);
    mg_printf(nc,
              "Date: %s\r\n"
              "Last-Modified: %s\r\n"
              "Accept-Ranges: bytes\r\n"
              "Content-Type: %.*s\r\n"
              "Connection: %s\r\n"
              "Content-Length: %" SIZE_T_FMT
              "\r\n"
              "%sEtag: %s\r\n\r\n",
              current_time, last_modified, (int) mime_type.len, mime_type.p,
              (pd->file.keepalive ? "keep-alive" : "close"), (size_t) cl, range,
              etag);

    pd->file.cl = cl;
    pd->file.type = DATA_FILE;
    mg_http_transfer_file_data(nc);
  }
}

static void mg_http_serve_file2(struct mg_connection *nc, 
                                const char *path,
                                struct http_message *hm,
                                struct mg_serve_http_opts *opts) 
{
#if MG_ENABLE_HTTP_SSI
    if (mg_match_prefix(opts->ssi_pattern, strlen(opts->ssi_pattern), path) > 0) {
        mg_handle_ssi_request(nc, hm, path, opts);
        return;
    }
#endif
    mg_http_serve_file(nc, hm, path, mg_get_mime_type(path, "text/plain", opts),
                     mg_mk_str(opts->extra_headers));
}

#endif

int mg_url_decode(const char *src, int src_len, char *dst, int dst_len, int is_form_url_encoded) 
{
    int i, j, a, b;
#define HEXTOI(x) (isdigit(x) ? x - '0' : x - 'W')

    for (i = j = 0; i < src_len && j < dst_len - 1; i++, j++) {
        if (src[i] == '%') {
            if (i < src_len - 2 && isxdigit(*(const unsigned char *) (src + i + 1)) &&
                isxdigit(*(const unsigned char *) (src + i + 2))) {
                a = tolower(*(const unsigned char *) (src + i + 1));
                b = tolower(*(const unsigned char *) (src + i + 2));
                dst[j] = (char) ((HEXTOI(a) << 4) | HEXTOI(b));
                i += 2;
            } else {
                return -1;
            }
        } else if (is_form_url_encoded && src[i] == '+') {
            dst[j] = ' ';
        } else {
            dst[j] = src[i];
        }
    }

    dst[j] = '\0'; /* Null-terminate the destination */

    return i >= src_len ? j : -1;
}

int mg_get_http_var(const struct mg_str *buf, const char *name, char *dst, size_t dst_len) 
{
    const char *p, *e, *s;
    size_t name_len;
    int len;

    /*
    * According to the documentation function returns negative
    * value in case of error. For debug purposes it returns:
    * -1 - src is wrong (NUUL)
    * -2 - dst is wrong (NULL)
    * -3 - failed to decode url or dst is to small
    * -4 - name does not exist
    */
    if (dst == NULL || dst_len == 0) {
        len = -2;
    } else if (buf->p == NULL || name == NULL || buf->len == 0) {
        len = -1;
        dst[0] = '\0';
    } else {
        name_len = strlen(name);
        e = buf->p + buf->len;
        len = -4;
        dst[0] = '\0';

        for (p = buf->p; p + name_len < e; p++) {
            if ((p == buf->p || p[-1] == '&') && p[name_len] == '=' && !mg_ncasecmp(name, p, name_len)) {
                p += name_len + 1;
                s = (const char *) memchr(p, '&', (size_t)(e - p));
                if (s == NULL) {
                    s = e;
                }
                len = mg_url_decode(p, (size_t)(s - p), dst, dst_len, 1);
        
                /* -1 means: failed to decode or dst is too small */
                if (len == -1) {
                    len = -3;
                }
                break;
            }
        }
    }

    return len;
}

void mg_send_http_chunk(struct mg_connection *nc, const char *buf, size_t len) 
{
    char chunk_size[50];
    int n;

    n = snprintf(chunk_size, sizeof(chunk_size), "%lX\r\n", (unsigned long) len);
    mg_send(nc, chunk_size, n);
    mg_send(nc, buf, len);
    mg_send(nc, "\r\n", 2);
}

void mg_printf_http_chunk(struct mg_connection *nc, const char *fmt, ...) 
{
    char mem[MG_VPRINTF_BUFFER_SIZE], *buf = mem;
    int len;
    va_list ap;

    va_start(ap, fmt);
    len = mg_avprintf(&buf, sizeof(mem), fmt, ap);
    va_end(ap);

    if (len >= 0) {
        mg_send_http_chunk(nc, buf, len);
    }

    /* LCOV_EXCL_START */
    if (buf != mem && buf != NULL) {
        MG_FREE(buf);
    }
    /* LCOV_EXCL_STOP */
}

void mg_printf_html_escape(struct mg_connection *nc, const char *fmt, ...) 
{
    char mem[MG_VPRINTF_BUFFER_SIZE], *buf = mem;
    int i, j, len;
    va_list ap;

    va_start(ap, fmt);
    len = mg_avprintf(&buf, sizeof(mem), fmt, ap);
    va_end(ap);

    if (len >= 0) {
        for (i = j = 0; i < len; i++) {
            if (buf[i] == '<' || buf[i] == '>') {
                mg_send(nc, buf + j, i - j);
                mg_send(nc, buf[i] == '<' ? "&lt;" : "&gt;", 4);
                j = i + 1;
            }
        }
        mg_send(nc, buf + j, i - j);
    }

    /* LCOV_EXCL_START */
    if (buf != mem && buf != NULL) {
        MG_FREE(buf);
    }
    /* LCOV_EXCL_STOP */
}

static void mg_http_parse_header_internal(struct mg_str *hdr,
                                          const char *var_name,
                                          struct altbuf *ab) 
{
    int ch = ' ', ch1 = ',', ch2 = ';', n = strlen(var_name);
    const char *p, *end = hdr ? hdr->p + hdr->len : NULL, *s = NULL;

    /* Find where variable starts */
    for (s = hdr->p; s != NULL && s + n < end; s++) {
        if ((s == hdr->p || s[-1] == ch || s[-1] == ch1 || s[-1] == ';') &&
            s[n] == '=' && !strncmp(s, var_name, n))
            break;
    }

    if (s != NULL && &s[n + 1] < end) {
        s += n + 1;
        if (*s == '"' || *s == '\'') {
            ch = ch1 = ch2 = *s++;
        }
        p = s;
        while (p < end && p[0] != ch && p[0] != ch1 && p[0] != ch2) {
            if (ch != ' ' && p[0] == '\\' && p[1] == ch) p++;
                altbuf_append(ab, *p++);
        }

        if (ch != ' ' && *p != ch) {
            altbuf_reset(ab);
        }
    }

    /* If there is some data, append a NUL. */
    if (ab->len > 0) {
        altbuf_append(ab, '\0');
    }
}

int mg_http_parse_header2(struct mg_str *hdr, const char *var_name, char **buf,
                          size_t buf_size) 
{
    struct altbuf ab;
    altbuf_init(&ab, *buf, buf_size);
    if (hdr == NULL) return 0;
    if (*buf != NULL && buf_size > 0) *buf[0] = '\0';

    mg_http_parse_header_internal(hdr, var_name, &ab);

    /*
    * Get a (trimmed) buffer, and return a len without a NUL byte which might
    * have been added.
    */
    *buf = altbuf_get_buf(&ab, 1 /* trim */);
    return ab.len > 0 ? ab.len - 1 : 0;
}

int mg_http_parse_header(struct mg_str *hdr, const char *var_name, char *buf, size_t buf_size) 
{
    char *buf2 = buf;

    int len = mg_http_parse_header2(hdr, var_name, &buf2, buf_size);

    if (buf2 != buf) {
        /* Buffer was not enough and was reallocated: free it and just return 0 */
        MG_FREE(buf2);
        return 0;
    }

    return len;
}

int mg_get_http_basic_auth(struct http_message *hm, char *user, size_t user_len,
                           char *pass, size_t pass_len) 
{
    struct mg_str *hdr = mg_get_http_header(hm, "Authorization");
    if (hdr == NULL) return -1;
    return mg_parse_http_basic_auth(hdr, user, user_len, pass, pass_len);
}

int mg_parse_http_basic_auth(struct mg_str *hdr, char *user, size_t user_len,
                             char *pass, size_t pass_len) 
{
    char *buf = NULL;
    char fmt[64];
    int res = 0;

    if (mg_strncmp(*hdr, mg_mk_str("Basic "), 6) != 0) return -1;

    buf = (char *) MG_MALLOC(hdr->len);
    cs_base64_decode((unsigned char *) hdr->p + 6, hdr->len, buf, NULL);

    /* e.g. "%123[^:]:%321[^\n]" */
    snprintf(fmt, sizeof(fmt), "%%%" SIZE_T_FMT "[^:]:%%%" SIZE_T_FMT "[^\n]",
            user_len - 1, pass_len - 1);
    if (sscanf(buf, fmt, user, pass) == 0) {
        res = -1;
    }

    MG_FREE(buf);
    return res;
}

#if MG_ENABLE_FILESYSTEM
static int mg_is_file_hidden(const char *path,
                             const struct mg_serve_http_opts *opts,
                             int exclude_specials) 
{
    const char *p1 = opts->per_directory_auth_file;
    const char *p2 = opts->hidden_file_pattern;

    /* Strip directory path from the file name */
    const char *pdir = strrchr(path, DIRSEP);
    if (pdir != NULL) {
        path = pdir + 1;
    }

    return (exclude_specials && (!strcmp(path, ".") || !strcmp(path, ".."))) ||
            (p1 != NULL && mg_match_prefix(p1, strlen(p1), path) == strlen(p1)) ||
            (p2 != NULL && mg_match_prefix(p2, strlen(p2), path) > 0);
}

#if !MG_DISABLE_HTTP_DIGEST_AUTH

#ifndef MG_EXT_MD5
void mg_hash_md5_v(size_t num_msgs, const uint8_t *msgs[],
                   const size_t *msg_lens, uint8_t *digest) 
{
    size_t i;
    cs_md5_ctx md5_ctx;
    cs_md5_init(&md5_ctx);
    for (i = 0; i < num_msgs; i++) {
        cs_md5_update(&md5_ctx, msgs[i], msg_lens[i]);
    }
    cs_md5_final(digest, &md5_ctx);
}
#else
extern void mg_hash_md5_v(size_t num_msgs, const uint8_t *msgs[],
                          const size_t *msg_lens, uint8_t *digest);
#endif

void cs_md5(char buf[33], ...) {
  unsigned char hash[16];
  const uint8_t *msgs[20], *p;
  size_t msg_lens[20];
  size_t num_msgs = 0;
  va_list ap;

  va_start(ap, buf);
  while ((p = va_arg(ap, const unsigned char *) ) != NULL) {
    msgs[num_msgs] = p;
    msg_lens[num_msgs] = va_arg(ap, size_t);
    num_msgs++;
  }
  va_end(ap);

  mg_hash_md5_v(num_msgs, msgs, msg_lens, hash);
  cs_to_hex(buf, hash, sizeof(hash));
}

static void mg_mkmd5resp(const char *method, size_t method_len, const char *uri,
                         size_t uri_len, const char *ha1, size_t ha1_len,
                         const char *nonce, size_t nonce_len, const char *nc,
                         size_t nc_len, const char *cnonce, size_t cnonce_len,
                         const char *qop, size_t qop_len, char *resp) {
  static const char colon[] = ":";
  static const size_t one = 1;
  char ha2[33];
  cs_md5(ha2, method, method_len, colon, one, uri, uri_len, NULL);
  cs_md5(resp, ha1, ha1_len, colon, one, nonce, nonce_len, colon, one, nc,
         nc_len, colon, one, cnonce, cnonce_len, colon, one, qop, qop_len,
         colon, one, ha2, sizeof(ha2) - 1, NULL);
}

int mg_http_create_digest_auth_header(char *buf, size_t buf_len,
                                      const char *method, const char *uri,
                                      const char *auth_domain, const char *user,
                                      const char *passwd, const char *nonce) 
{
    static const char colon[] = ":", qop[] = "auth";
    static const size_t one = 1;
    char ha1[33], resp[33], cnonce[40];

    snprintf(cnonce, sizeof(cnonce), "%lx", (unsigned long) mg_time());
    cs_md5(ha1, user, (size_t) strlen(user), colon, one, auth_domain,
            (size_t) strlen(auth_domain), colon, one, passwd,
            (size_t) strlen(passwd), NULL);
    mg_mkmd5resp(method, strlen(method), uri, strlen(uri), ha1, sizeof(ha1) - 1,
                nonce, strlen(nonce), "1", one, cnonce, strlen(cnonce), qop,
                sizeof(qop) - 1, resp);
    return snprintf(buf, buf_len,
                    "Authorization: Digest username=\"%s\","
                    "realm=\"%s\",uri=\"%s\",qop=%s,nc=1,cnonce=%s,"
                    "nonce=%s,response=%s\r\n",
                    user, auth_domain, uri, qop, cnonce, nonce, resp);
}

/*
 * Check for authentication timeout.
 * Clients send time stamp encoded in nonce. Make sure it is not too old,
 * to prevent replay attacks.
 * Assumption: nonce is a hexadecimal number of seconds since 1970.
 */
static int mg_check_nonce(const char *nonce) 
{
    unsigned long now = (unsigned long) mg_time();
    unsigned long val = (unsigned long) strtoul(nonce, NULL, 16);
    return (now >= val) && (now - val < 60 * 60);
}

int mg_http_check_digest_auth(struct http_message *hm, const char *auth_domain, FILE *fp) 
{
    int ret = 0;
    struct mg_str *hdr;
    char username_buf[50], cnonce_buf[64], response_buf[40], uri_buf[200],
        qop_buf[20], nc_buf[20], nonce_buf[16];

    char *username = username_buf, *cnonce = cnonce_buf, *response = response_buf,
        *uri = uri_buf, *qop = qop_buf, *nc = nc_buf, *nonce = nonce_buf;

    /* Parse "Authorization:" header, fail fast on parse error */
    if (hm == NULL || fp == NULL ||
      (hdr = mg_get_http_header(hm, "Authorization")) == NULL ||
      mg_http_parse_header2(hdr, "username", &username, sizeof(username_buf)) ==
          0 ||
      mg_http_parse_header2(hdr, "cnonce", &cnonce, sizeof(cnonce_buf)) == 0 ||
      mg_http_parse_header2(hdr, "response", &response, sizeof(response_buf)) ==
          0 ||
      mg_http_parse_header2(hdr, "uri", &uri, sizeof(uri_buf)) == 0 ||
      mg_http_parse_header2(hdr, "qop", &qop, sizeof(qop_buf)) == 0 ||
      mg_http_parse_header2(hdr, "nc", &nc, sizeof(nc_buf)) == 0 ||
      mg_http_parse_header2(hdr, "nonce", &nonce, sizeof(nonce_buf)) == 0 ||
      mg_check_nonce(nonce) == 0) {
        ret = 0;
        goto clean;
    }

    /* NOTE(lsm): due to a bug in MSIE, we do not compare URIs */

    ret = mg_check_digest_auth(hm->method,
                    mg_mk_str_n(hm->uri.p,
                                hm->uri.len + (hm->query_string.len ? hm->query_string.len + 1 : 0)),
                    mg_mk_str(username), 
                    mg_mk_str(cnonce), 
                    mg_mk_str(response),
                    mg_mk_str(qop), 
                    mg_mk_str(nc), 
                    mg_mk_str(nonce), 
                    mg_mk_str(auth_domain),
                    fp);

clean:
    if (username != username_buf) MG_FREE(username);
    if (cnonce != cnonce_buf) MG_FREE(cnonce);
    if (response != response_buf) MG_FREE(response);
    if (uri != uri_buf) MG_FREE(uri);
    if (qop != qop_buf) MG_FREE(qop);
    if (nc != nc_buf) MG_FREE(nc);
    if (nonce != nonce_buf) MG_FREE(nonce);

    return ret;
}

int mg_check_digest_auth(struct mg_str method, struct mg_str uri,
                         struct mg_str username, struct mg_str cnonce,
                         struct mg_str response, struct mg_str qop,
                         struct mg_str nc, struct mg_str nonce,
                         struct mg_str auth_domain, FILE *fp) {
  char buf[128], f_user[sizeof(buf)], f_ha1[sizeof(buf)], f_domain[sizeof(buf)];
  char expected_response[33];

  /*
   * Read passwords file line by line. If should have htdigest format,
   * i.e. each line should be a colon-separated sequence:
   * USER_NAME:DOMAIN_NAME:HA1_HASH_OF_USER_DOMAIN_AND_PASSWORD
   */
  while (fgets(buf, sizeof(buf), fp) != NULL) {
    if (sscanf(buf, "%[^:]:%[^:]:%s", f_user, f_domain, f_ha1) == 3 &&
        mg_vcmp(&username, f_user) == 0 &&
        mg_vcmp(&auth_domain, f_domain) == 0) {
      /* Username and domain matched, check the password */
      mg_mkmd5resp(method.p, method.len, uri.p, uri.len, f_ha1, strlen(f_ha1),
                   nonce.p, nonce.len, nc.p, nc.len, cnonce.p, cnonce.len,
                   qop.p, qop.len, expected_response);
      LOG(LL_DEBUG,
          ("%.*s %s %.*s %s", (int) username.len, username.p, f_domain,
           (int) response.len, response.p, expected_response));
      return mg_ncasecmp(response.p, expected_response, response.len) == 0;
    }
  }

  /* None of the entries in the passwords file matched - return failure */
  return 0;
}

int mg_http_is_authorized(struct http_message *hm, 
                            struct mg_str path,
                            const char *domain, 
                            const char *passwords_file,
                            int flags) 
{
    char buf[MG_MAX_PATH];
    const char *p;
    FILE *fp;
    int authorized = 1;

    if (domain != NULL && passwords_file != NULL) {
        if (flags & MG_AUTH_FLAG_IS_GLOBAL_PASS_FILE) {
            fp = mg_fopen(passwords_file, "r");
        } else if (flags & MG_AUTH_FLAG_IS_DIRECTORY) {
            snprintf(buf, sizeof(buf), "%.*s%c%s", (int) path.len, path.p, DIRSEP,
               passwords_file);
            fp = mg_fopen(buf, "r");
        } else {
            p = strrchr(path.p, DIRSEP);
            if (p == NULL) p = path.p;
            snprintf(buf, sizeof(buf), "%.*s%c%s", (int) (p - path.p), path.p, DIRSEP,
               passwords_file);
            fp = mg_fopen(buf, "r");
        }

        if (fp != NULL) {
            authorized = mg_http_check_digest_auth(hm, domain, fp);
            fclose(fp);
        } else if (!(flags & MG_AUTH_FLAG_ALLOW_MISSING_FILE)) {
            authorized = 0;
        }
    }

    LOG(LL_DEBUG, ("%.*s %s %x %d", (int) path.len, path.p,
                 passwords_file ? passwords_file : "", flags, authorized));
    return authorized;
}
#else
int mg_http_is_authorized(struct http_message *hm, const struct mg_str path,
                          const char *domain, const char *passwords_file,
                          int flags) {
  (void) hm;
  (void) path;
  (void) domain;
  (void) passwords_file;
  (void) flags;
  return 1;
}
#endif

#if MG_ENABLE_DIRECTORY_LISTING
static void mg_escape(const char *src, char *dst, size_t dst_len) 
{
    size_t n = 0;
    while (*src != '\0' && n + 5 < dst_len) {
        unsigned char ch = *(unsigned char *) src++;
        if (ch == '<') {
            n += snprintf(dst + n, dst_len - n, "%s", "&lt;");
        } else {
            dst[n++] = ch;
        }
    }
    dst[n] = '\0';
}

static void mg_print_dir_entry(struct mg_connection *nc, const char *file_name, cs_stat_t *stp) 
{
    char size[64], mod[64], path[MG_MAX_PATH];
    int64_t fsize = stp->st_size;
    int is_dir = S_ISDIR(stp->st_mode);
    const char *slash = is_dir ? "/" : "";
    struct mg_str href;

    if (is_dir) {
        snprintf(size, sizeof(size), "%s", "[DIRECTORY]");
    } else {
        /*
        * We use (double) cast below because MSVC 6 compiler cannot
        * convert unsigned __int64 to double.
        */
        if (fsize < 1024) {
            snprintf(size, sizeof(size), "%d", (int) fsize);
        } else if (fsize < 0x100000) {
            snprintf(size, sizeof(size), "%.1fk", (double) fsize / 1024.0);
        } else if (fsize < 0x40000000) {
            snprintf(size, sizeof(size), "%.1fM", (double) fsize / 1048576);
        } else {
            snprintf(size, sizeof(size), "%.1fG", (double) fsize / 1073741824);
        }
    }
    strftime(mod, sizeof(mod), "%d-%b-%Y %H:%M", localtime(&stp->st_mtime));
    mg_escape(file_name, path, sizeof(path));
    href = mg_url_encode(mg_mk_str(file_name));
    mg_printf_http_chunk(nc,
                       "<tr><td><a href=\"%s%s\">%s%s</a></td>"
                       "<td>%s</td><td name=%" INT64_FMT ">%s</td></tr>\n",
                       href.p, slash, path, slash, mod, is_dir ? -1 : fsize,
                       size);
    free((void *) href.p);
}

static void mg_scan_directory(struct mg_connection *nc, 
                              const char *dir,
                              const struct mg_serve_http_opts *opts,
                              void (*func)(struct mg_connection *, const char *, cs_stat_t *)) 
{
    char path[MG_MAX_PATH + 1];
    cs_stat_t st;
    struct dirent *dp;
    DIR *dirp;

    LOG(LL_DEBUG, ("%p [%s]", nc, dir));
    if ((dirp = (opendir(dir))) != NULL) {
        while ((dp = readdir(dirp)) != NULL) {
            /* Do not show current dir and hidden files */
            if (mg_is_file_hidden((const char *) dp->d_name, opts, 1)) {
                continue;
            }
            snprintf(path, sizeof(path), "%s/%s", dir, dp->d_name);
            if (mg_stat(path, &st) == 0) {
                func(nc, (const char *) dp->d_name, &st);
            }
        }
        closedir(dirp);
    } else {
        LOG(LL_DEBUG, ("%p opendir(%s) -> %d", nc, dir, mg_get_errno()));
    }
}

static void mg_send_directory_listing(struct mg_connection *nc, 
                                      const char *dir,
                                      struct http_message *hm,
                                      struct mg_serve_http_opts *opts) 
{
    static const char *sort_js_code =
        "<script>function srt(tb, sc, so, d) {"
        "var tr = Array.prototype.slice.call(tb.rows, 0),"
        "tr = tr.sort(function (a, b) { var c1 = a.cells[sc], c2 = b.cells[sc],"
        "n1 = c1.getAttribute('name'), n2 = c2.getAttribute('name'), "
        "t1 = a.cells[2].getAttribute('name'), "
        "t2 = b.cells[2].getAttribute('name'); "
        "return so * (t1 < 0 && t2 >= 0 ? -1 : t2 < 0 && t1 >= 0 ? 1 : "
        "n1 ? parseInt(n2) - parseInt(n1) : "
        "c1.textContent.trim().localeCompare(c2.textContent.trim())); });";
    static const char *sort_js_code2 =
        "for (var i = 0; i < tr.length; i++) tb.appendChild(tr[i]); "
        "if (!d) window.location.hash = ('sc=' + sc + '&so=' + so); "
        "};"
        "window.onload = function() {"
        "var tb = document.getElementById('tb');"
        "var m = /sc=([012]).so=(1|-1)/.exec(window.location.hash) || [0, 2, 1];"
        "var sc = m[1], so = m[2]; document.onclick = function(ev) { "
        "var c = ev.target.rel; if (c) {if (c == sc) so *= -1; srt(tb, c, so); "
        "sc = c; ev.preventDefault();}};"
        "srt(tb, sc, so, true);"
        "}"
        "</script>";

    mg_send_response_line(nc, 200, opts->extra_headers);
    mg_printf(nc, "%s: %s\r\n%s: %s\r\n\r\n", "Transfer-Encoding", "chunked",
            "Content-Type", "text/html; charset=utf-8");

    mg_printf_http_chunk(
        nc,
        "<html><head><title>Index of %.*s</title>%s%s"
        "<style>th,td {text-align: left; padding-right: 1em; "
        "font-family: monospace; }</style></head>\n"
        "<body><h1>Index of %.*s</h1>\n<table cellpadding=0><thead>"
        "<tr><th><a href=# rel=0>Name</a></th><th>"
        "<a href=# rel=1>Modified</a</th>"
        "<th><a href=# rel=2>Size</a></th></tr>"
        "<tr><td colspan=3><hr></td></tr>\n"
        "</thead>\n"
        "<tbody id=tb>",
        (int) hm->uri.len, hm->uri.p, sort_js_code, sort_js_code2,
        (int) hm->uri.len, hm->uri.p);
    mg_scan_directory(nc, dir, opts, mg_print_dir_entry);
    mg_printf_http_chunk(nc,
                        "</tbody><tr><td colspan=3><hr></td></tr>\n"
                        "</table>\n"
                        "<address>%s</address>\n"
                        "</body></html>",
                        mg_version_header);
    mg_send_http_chunk(nc, "", 0);
    /* TODO(rojer): Remove when cesanta/dev/issues/197 is fixed. */
    nc->flags |= MG_F_SEND_AND_CLOSE;
}
#endif /* MG_ENABLE_DIRECTORY_LISTING */

/*
 * Given a directory path, find one of the files specified in the
 * comma-separated list of index files `list`.
 * First found index file wins. If an index file is found, then gets
 * appended to the `path`, stat-ed, and result of `stat()` passed to `stp`.
 * If index file is not found, then `path` and `stp` remain unchanged.
 */
MG_INTERNAL void mg_find_index_file(const char *path, const char *list,
                                    char **index_file, cs_stat_t *stp) {
  struct mg_str vec;
  size_t path_len = strlen(path);
  int found = 0;
  *index_file = NULL;

  /* Traverse index files list. For each entry, append it to the given */
  /* path and see if the file exists. If it exists, break the loop */
  while ((list = mg_next_comma_list_entry(list, &vec, NULL)) != NULL) {
    cs_stat_t st;
    size_t len = path_len + 1 + vec.len + 1;
    *index_file = (char *) MG_REALLOC(*index_file, len);
    if (*index_file == NULL) break;
    snprintf(*index_file, len, "%s%c%.*s", path, DIRSEP, (int) vec.len, vec.p);

    /* Does it exist? Is it a file? */
    if (mg_stat(*index_file, &st) == 0 && S_ISREG(st.st_mode)) {
      /* Yes it does, break the loop */
      *stp = st;
      found = 1;
      break;
    }
  }
  if (!found) {
    MG_FREE(*index_file);
    *index_file = NULL;
  }
  LOG(LL_DEBUG, ("[%s] [%s]", path, (*index_file ? *index_file : "")));
}

#if MG_ENABLE_HTTP_URL_REWRITES
static int mg_http_send_port_based_redirect(
    struct mg_connection *c, struct http_message *hm,
    const struct mg_serve_http_opts *opts) {
  const char *rewrites = opts->url_rewrites;
  struct mg_str a, b;
  char local_port[20] = {'%'};

  mg_conn_addr_to_str(c, local_port + 1, sizeof(local_port) - 1,
                      MG_SOCK_STRINGIFY_PORT);

  while ((rewrites = mg_next_comma_list_entry(rewrites, &a, &b)) != NULL) {
    if (mg_vcmp(&a, local_port) == 0) {
      mg_send_response_line(c, 301, NULL);
      mg_printf(c, "Content-Length: 0\r\nLocation: %.*s%.*s\r\n\r\n",
                (int) b.len, b.p, (int) (hm->proto.p - hm->uri.p - 1),
                hm->uri.p);
      return 1;
    }
  }

  return 0;
}

static void mg_reverse_proxy_handler(struct mg_connection *nc, int ev,
                                     void *ev_data MG_UD_ARG(void *user_data)) {
  struct http_message *hm = (struct http_message *) ev_data;
  struct mg_http_proto_data *pd = mg_http_get_proto_data(nc);

  if (pd == NULL || pd->reverse_proxy_data.linked_conn == NULL) {
    DBG(("%p: upstream closed", nc));
    return;
  }

  switch (ev) {
    case MG_EV_CONNECT:
      if (*(int *) ev_data != 0) {
        mg_http_send_error(pd->reverse_proxy_data.linked_conn, 502, NULL);
      }
      break;
    /* TODO(mkm): handle streaming */
    case MG_EV_HTTP_REPLY:
      mg_send(pd->reverse_proxy_data.linked_conn, hm->message.p,
              hm->message.len);
      pd->reverse_proxy_data.linked_conn->flags |= MG_F_SEND_AND_CLOSE;
      nc->flags |= MG_F_CLOSE_IMMEDIATELY;
      break;
    case MG_EV_CLOSE:
      pd->reverse_proxy_data.linked_conn->flags |= MG_F_SEND_AND_CLOSE;
      break;
  }

#if MG_ENABLE_CALLBACK_USERDATA
  (void) user_data;
#endif
}

void mg_http_reverse_proxy(struct mg_connection *nc,
                           const struct http_message *hm, struct mg_str mount,
                           struct mg_str upstream) {
  struct mg_connection *be;
  char burl[256], *purl = burl;
  int i;
  const char *error;
  struct mg_connect_opts opts;
  struct mg_str path = MG_NULL_STR, user_info = MG_NULL_STR, host = MG_NULL_STR;
  memset(&opts, 0, sizeof(opts));
  opts.error_string = &error;

  mg_asprintf(&purl, sizeof(burl), "%.*s%.*s", (int) upstream.len, upstream.p,
              (int) (hm->uri.len - mount.len), hm->uri.p + mount.len);

  be = mg_connect_http_base(nc->mgr, MG_CB(mg_reverse_proxy_handler, NULL),
                            opts, "http", NULL, "https", NULL, purl, &path,
                            &user_info, &host);
  LOG(LL_DEBUG, ("Proxying %.*s to %s (rule: %.*s)", (int) hm->uri.len,
                 hm->uri.p, purl, (int) mount.len, mount.p));

  if (be == NULL) {
    LOG(LL_ERROR, ("Error connecting to %s: %s", purl, error));
    mg_http_send_error(nc, 502, NULL);
    goto cleanup;
  }

  /* link connections to each other, they must live and die together */
  mg_http_get_proto_data(be)->reverse_proxy_data.linked_conn = nc;
  mg_http_get_proto_data(nc)->reverse_proxy_data.linked_conn = be;

  /* send request upstream */
  mg_printf(be, "%.*s %.*s HTTP/1.1\r\n", (int) hm->method.len, hm->method.p,
            (int) path.len, path.p);

  mg_printf(be, "Host: %.*s\r\n", (int) host.len, host.p);
  for (i = 0; i < MG_MAX_HTTP_HEADERS && hm->header_names[i].len > 0; i++) {
    struct mg_str hn = hm->header_names[i];
    struct mg_str hv = hm->header_values[i];

    /* we rewrite the host header */
    if (mg_vcasecmp(&hn, "Host") == 0) continue;
    /*
     * Don't pass chunked transfer encoding to the client because hm->body is
     * already dechunked when we arrive here.
     */
    if (mg_vcasecmp(&hn, "Transfer-encoding") == 0 &&
        mg_vcasecmp(&hv, "chunked") == 0) {
      mg_printf(be, "Content-Length: %" SIZE_T_FMT "\r\n", hm->body.len);
      continue;
    }
    /* We don't support proxying Expect: 100-continue. */
    if (mg_vcasecmp(&hn, "Expect") == 0 &&
        mg_vcasecmp(&hv, "100-continue") == 0) {
      continue;
    }

    mg_printf(be, "%.*s: %.*s\r\n", (int) hn.len, hn.p, (int) hv.len, hv.p);
  }

  mg_send(be, "\r\n", 2);
  mg_send(be, hm->body.p, hm->body.len);

cleanup:
  if (purl != burl) MG_FREE(purl);
}

static int mg_http_handle_forwarding(struct mg_connection *nc,
                                     struct http_message *hm,
                                     const struct mg_serve_http_opts *opts) 
{
    const char *rewrites = opts->url_rewrites;
    struct mg_str a, b;
    struct mg_str p1 = MG_MK_STR("http://"), p2 = MG_MK_STR("https://");

    while ((rewrites = mg_next_comma_list_entry(rewrites, &a, &b)) != NULL) {
        if (mg_strncmp(a, hm->uri, a.len) == 0) {
            if (mg_strncmp(b, p1, p1.len) == 0 || mg_strncmp(b, p2, p2.len) == 0) {
                mg_http_reverse_proxy(nc, hm, a, b);
                return 1;
            }
        }
    }

    return 0;
}
#endif /* MG_ENABLE_FILESYSTEM */

MG_INTERNAL int mg_uri_to_local_path(struct http_message *hm,
                                     const struct mg_serve_http_opts *opts,
                                     char **local_path,
                                     struct mg_str *remainder) 
{
    int ok = 1;
    const char *cp = hm->uri.p, *cp_end = hm->uri.p + hm->uri.len;
    struct mg_str root = {NULL, 0};
    const char *file_uri_start = cp;
    *local_path = NULL;
    remainder->p = NULL;
    remainder->len = 0;

    { /* 1. Determine which root to use. */

#if MG_ENABLE_HTTP_URL_REWRITES
        const char *rewrites = opts->url_rewrites;
#else
        const char *rewrites = "";
#endif
        struct mg_str *hh = mg_get_http_header(hm, "Host");
        struct mg_str a, b;
        /* Check rewrites first. */
        while ((rewrites = mg_next_comma_list_entry(rewrites, &a, &b)) != NULL) {
            if (a.len > 1 && a.p[0] == '@') {
                /* Host rewrite. */
                if (hh != NULL && hh->len == a.len - 1 && mg_ncasecmp(a.p + 1, hh->p, a.len - 1) == 0) {
                    root = b;
                    break;
                }
            } else {
                /* Regular rewrite, URI=directory */
                size_t match_len = mg_match_prefix_n(a, hm->uri);
                if (match_len > 0) {
                    file_uri_start = hm->uri.p + match_len;
                    if (*file_uri_start == '/' || file_uri_start == cp_end) {
                        /* Match ended at component boundary, ok. */
                    } else if (*(file_uri_start - 1) == '/') {
                        /* Pattern ends with '/', backtrack. */
                        file_uri_start--;
                    } else {
                        /* No match: must fall on the component boundary. */
                        continue;
                    }
                    root = b;
                    break;
                }
            }
        }
    
        /* If no rewrite rules matched, use DAV or regular document root. */
        if (root.p == NULL) {
#if MG_ENABLE_HTTP_WEBDAV
            if (opts->dav_document_root != NULL && mg_is_dav_request(&hm->method)) {
                root.p = opts->dav_document_root;
                root.len = strlen(opts->dav_document_root);
            } else
#endif
            {
                root.p = opts->document_root;
                root.len = strlen(opts->document_root);
            }
        }
        assert(root.p != NULL && root.len > 0);
    }

    { /* 2. Find where in the canonical URI path the local path ends. */
        const char *u = file_uri_start + 1;
        char *lp = (char *) MG_MALLOC(root.len + hm->uri.len + 1);
        char *lp_end = lp + root.len + hm->uri.len + 1;
        char *p = lp, *ps;
        int exists = 1;
        if (lp == NULL) {
            ok = 0;
            goto out;
        }
        memcpy(p, root.p, root.len);
        p += root.len;
        if (*(p - 1) == DIRSEP) p--;
        *p = '\0';
        ps = p;

    /* Chop off URI path components one by one and build local path. */
    while (u <= cp_end) {
      const char *next = u;
      struct mg_str component;
      if (exists) {
        cs_stat_t st;
        exists = (mg_stat(lp, &st) == 0);
        if (exists && S_ISREG(st.st_mode)) {
          /* We found the terminal, the rest of the URI (if any) is path_info.
           */
          if (*(u - 1) == '/') u--;
          break;
        }
      }
      if (u >= cp_end) break;
      parse_uri_component((const char **) &next, cp_end, "/", &component);
      if (component.len > 0) {
        int len;
        memmove(p + 1, component.p, component.len);
        len = mg_url_decode(p + 1, component.len, p + 1, lp_end - p - 1, 0);
        if (len <= 0) {
          ok = 0;
          break;
        }
        component.p = p + 1;
        component.len = len;
        if (mg_vcmp(&component, ".") == 0) {
          /* Yum. */
        } else if (mg_vcmp(&component, "..") == 0) {
          while (p > ps && *p != DIRSEP) p--;
          *p = '\0';
        } else {
          size_t i;
#ifdef _WIN32
          /* On Windows, make sure it's valid Unicode (no funny stuff). */
          wchar_t buf[MG_MAX_PATH * 2];
          if (to_wchar(component.p, buf, MG_MAX_PATH) == 0) {
            DBG(("[%.*s] smells funny", (int) component.len, component.p));
            ok = 0;
            break;
          }
#endif
          *p++ = DIRSEP;
          /* No NULs and DIRSEPs in the component (percent-encoded). */
          for (i = 0; i < component.len; i++, p++) {
            if (*p == '\0' || *p == DIRSEP
#ifdef _WIN32
                /* On Windows, "/" is also accepted, so check for that too. */
                ||
                *p == '/'
#endif
                ) {
              ok = 0;
              break;
            }
          }
        }
      }
      u = next;
    }
    if (ok) {
      *local_path = lp;
      if (u > cp_end) u = cp_end;
      remainder->p = u;
      remainder->len = cp_end - u;
    } else {
      MG_FREE(lp);
    }
  }

out:
  LOG(LL_DEBUG,
      ("'%.*s' -> '%s' + '%.*s'", (int) hm->uri.len, hm->uri.p,
       *local_path ? *local_path : "", (int) remainder->len, remainder->p));
  return ok;
}

static int mg_get_month_index(const char *s) 
{
    static const char *month_names[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                      "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    size_t i;

    for (i = 0; i < ARRAY_SIZE(month_names); i++)
        if (!strcmp(s, month_names[i])) return (int) i;

    return -1;
}

static int mg_num_leap_years(int year) 
{
    return year / 4 - year / 100 + year / 400;
}

/* Parse UTC date-time string, and return the corresponding time_t value. */
MG_INTERNAL time_t mg_parse_date_string(const char *datetime) 
{
    static const unsigned short days_before_month[] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
    char month_str[32];
    int second, minute, hour, day, month, year, leap_days, days;
    time_t result = (time_t) 0;

    if (((sscanf(datetime, "%d/%3s/%d %d:%d:%d", &day, month_str, &year, &hour,
               &minute, &second) == 6) ||
       (sscanf(datetime, "%d %3s %d %d:%d:%d", &day, month_str, &year, &hour,
               &minute, &second) == 6) ||
       (sscanf(datetime, "%*3s, %d %3s %d %d:%d:%d", &day, month_str, &year,
               &hour, &minute, &second) == 6) ||
       (sscanf(datetime, "%d-%3s-%d %d:%d:%d", &day, month_str, &year, &hour,
               &minute, &second) == 6)) &&
      year > 1970 && (month = mg_get_month_index(month_str)) != -1) {
        leap_days = mg_num_leap_years(year) - mg_num_leap_years(1970);
        year -= 1970;
        days = year * 365 + days_before_month[month] + (day - 1) + leap_days;
        result = days * 24 * 3600 + hour * 3600 + minute * 60 + second;
    }
    return result;
}

MG_INTERNAL int mg_is_not_modified(struct http_message *hm, cs_stat_t *st) 
{
    struct mg_str *hdr;
    if ((hdr = mg_get_http_header(hm, "If-None-Match")) != NULL) {
        char etag[64];
        mg_http_construct_etag(etag, sizeof(etag), st);
        return mg_vcasecmp(hdr, etag) == 0;
    } else if ((hdr = mg_get_http_header(hm, "If-Modified-Since")) != NULL) {
        return st->st_mtime <= mg_parse_date_string(hdr->p);
    } else {
        return 0;
    }
}

void mg_http_send_digest_auth_request(struct mg_connection *c, const char *domain) 
{
    mg_printf(c,
            "HTTP/1.1 401 Unauthorized\r\n"
            "WWW-Authenticate: Digest qop=\"auth\", "
            "realm=\"%s\", nonce=\"%lx\"\r\n"
            "Content-Length: 0\r\n\r\n",
            domain, (unsigned long) mg_time());
}

static void mg_http_send_options(struct mg_connection *nc) 
{
    mg_printf(nc, "%s",
            "HTTP/1.1 200 OK\r\nAllow: GET, POST, HEAD, CONNECT, OPTIONS"

#if MG_ENABLE_HTTP_WEBDAV
            ", MKCOL, PUT, DELETE, PROPFIND, MOVE\r\nDAV: 1,2"
#endif
            "\r\n\r\n");
    nc->flags |= MG_F_SEND_AND_CLOSE;
}

static int mg_is_creation_request(const struct http_message *hm) {
  return mg_vcmp(&hm->method, "MKCOL") == 0 || mg_vcmp(&hm->method, "PUT") == 0;
}

MG_INTERNAL void mg_send_http_file(struct mg_connection *nc, char *path,
                                   const struct mg_str *path_info,
                                   struct http_message *hm,
                                   struct mg_serve_http_opts *opts) {
  int exists, is_directory, is_cgi;
#if MG_ENABLE_HTTP_WEBDAV
  int is_dav = mg_is_dav_request(&hm->method);
#else
  int is_dav = 0;
#endif
  char *index_file = NULL;
  cs_stat_t st;

  exists = (mg_stat(path, &st) == 0);
  is_directory = exists && S_ISDIR(st.st_mode);

  if (is_directory)
    mg_find_index_file(path, opts->index_files, &index_file, &st);

  is_cgi =
      (mg_match_prefix(opts->cgi_file_pattern, strlen(opts->cgi_file_pattern),
                       index_file ? index_file : path) > 0);

  LOG(LL_DEBUG,
      ("%p %.*s [%s] exists=%d is_dir=%d is_dav=%d is_cgi=%d index=%s", nc,
       (int) hm->method.len, hm->method.p, path, exists, is_directory, is_dav,
       is_cgi, index_file ? index_file : ""));

  if (is_directory && hm->uri.p[hm->uri.len - 1] != '/' && !is_dav) {
    mg_printf(nc,
              "HTTP/1.1 301 Moved\r\nLocation: %.*s/\r\n"
              "Content-Length: 0\r\n\r\n",
              (int) hm->uri.len, hm->uri.p);
    MG_FREE(index_file);
    return;
  }

  /* If we have path_info, the only way to handle it is CGI. */
  if (path_info->len > 0 && !is_cgi) {
    mg_http_send_error(nc, 501, NULL);
    MG_FREE(index_file);
    return;
  }

  if (is_dav && opts->dav_document_root == NULL) {
    mg_http_send_error(nc, 501, NULL);
  } else if (!mg_http_is_authorized(
                 hm, mg_mk_str(path), opts->auth_domain, opts->global_auth_file,
                 ((is_directory ? MG_AUTH_FLAG_IS_DIRECTORY : 0) |
                  MG_AUTH_FLAG_IS_GLOBAL_PASS_FILE |
                  MG_AUTH_FLAG_ALLOW_MISSING_FILE)) ||
             !mg_http_is_authorized(
                 hm, mg_mk_str(path), opts->auth_domain,
                 opts->per_directory_auth_file,
                 ((is_directory ? MG_AUTH_FLAG_IS_DIRECTORY : 0) |
                  MG_AUTH_FLAG_ALLOW_MISSING_FILE))) {
    mg_http_send_digest_auth_request(nc, opts->auth_domain);
  } else if (is_cgi) {
#if MG_ENABLE_HTTP_CGI
    mg_handle_cgi(nc, index_file ? index_file : path, path_info, hm, opts);
#else
    mg_http_send_error(nc, 501, NULL);
#endif /* MG_ENABLE_HTTP_CGI */
  } else if ((!exists ||
              mg_is_file_hidden(path, opts, 0 /* specials are ok */)) &&
             !mg_is_creation_request(hm)) {
    mg_http_send_error(nc, 404, NULL);
#if MG_ENABLE_HTTP_WEBDAV
  } else if (!mg_vcmp(&hm->method, "PROPFIND")) {
    mg_handle_propfind(nc, path, &st, hm, opts);
#if !MG_DISABLE_DAV_AUTH
  } else if (is_dav &&
             (opts->dav_auth_file == NULL ||
              (strcmp(opts->dav_auth_file, "-") != 0 &&
               !mg_http_is_authorized(
                   hm, mg_mk_str(path), opts->auth_domain, opts->dav_auth_file,
                   ((is_directory ? MG_AUTH_FLAG_IS_DIRECTORY : 0) |
                    MG_AUTH_FLAG_IS_GLOBAL_PASS_FILE |
                    MG_AUTH_FLAG_ALLOW_MISSING_FILE))))) {
    mg_http_send_digest_auth_request(nc, opts->auth_domain);
#endif
  } else if (!mg_vcmp(&hm->method, "MKCOL")) {
    mg_handle_mkcol(nc, path, hm);
  } else if (!mg_vcmp(&hm->method, "DELETE")) {
    mg_handle_delete(nc, opts, path);
  } else if (!mg_vcmp(&hm->method, "PUT")) {
    mg_handle_put(nc, path, hm);
  } else if (!mg_vcmp(&hm->method, "MOVE")) {
    mg_handle_move(nc, opts, path, hm);
#if MG_ENABLE_FAKE_DAVLOCK
  } else if (!mg_vcmp(&hm->method, "LOCK")) {
    mg_handle_lock(nc, path);
#endif
#endif /* MG_ENABLE_HTTP_WEBDAV */
  } else if (!mg_vcmp(&hm->method, "OPTIONS")) {
    mg_http_send_options(nc);
  } else if (is_directory && index_file == NULL) {
#if MG_ENABLE_DIRECTORY_LISTING
    if (strcmp(opts->enable_directory_listing, "yes") == 0) {
      mg_send_directory_listing(nc, path, hm, opts);
    } else {
      mg_http_send_error(nc, 403, NULL);
    }
#else
    mg_http_send_error(nc, 501, NULL);
#endif
  } else if (mg_is_not_modified(hm, &st)) {
    mg_http_send_error(nc, 304, "Not Modified");
  } else {
    mg_http_serve_file2(nc, index_file ? index_file : path, hm, opts);
  }
  MG_FREE(index_file);
}

void mg_serve_http(struct mg_connection *nc, 
                    struct http_message *hm,
                    struct mg_serve_http_opts opts) 
{
    char *path = NULL;
    struct mg_str *hdr, path_info;
    uint32_t remote_ip = ntohl(*(uint32_t *) &nc->sa.sin.sin_addr);

    if (mg_check_ip_acl(opts.ip_acl, remote_ip) != 1) {
        /* Not allowed to connect */
        mg_http_send_error(nc, 403, NULL);
        nc->flags |= MG_F_SEND_AND_CLOSE;
        return;
    }

#if MG_ENABLE_HTTP_URL_REWRITES
    if (mg_http_handle_forwarding(nc, hm, &opts)) {
        return;
    }

    if (mg_http_send_port_based_redirect(nc, hm, &opts)) {
        return;
    }
#endif

    if (opts.document_root == NULL) {
        opts.document_root = ".";
    }
  
    if (opts.per_directory_auth_file == NULL) {
        opts.per_directory_auth_file = ".htpasswd";
    }
    
    if (opts.enable_directory_listing == NULL) {
        opts.enable_directory_listing = "yes";
    }
  
    if (opts.cgi_file_pattern == NULL) {
        opts.cgi_file_pattern = "**.cgi$|**.php$";
    }
  
    if (opts.ssi_pattern == NULL) {
        opts.ssi_pattern = "**.shtml$|**.shtm$";
    }
  
    if (opts.index_files == NULL) {
        opts.index_files = "index.html,index.htm,index.shtml,index.cgi,index.php";
    }
  
    /* Normalize path - resolve "." and ".." (in-place). */
    if (!mg_normalize_uri_path(&hm->uri, &hm->uri)) {
        mg_http_send_error(nc, 400, NULL);
        return;
    }
  
    if (mg_uri_to_local_path(hm, &opts, &path, &path_info) == 0) {
        mg_http_send_error(nc, 404, NULL);
        return;
    }
    mg_send_http_file(nc, path, &path_info, hm, &opts);

    MG_FREE(path);
    path = NULL;

    /* Close connection for non-keep-alive requests */
    if (mg_vcmp(&hm->proto, "HTTP/1.1") != 0 || ((hdr = mg_get_http_header(hm, "Connection")) != NULL && mg_vcmp(hdr, "keep-alive") != 0)) {
#if 0
        nc->flags |= MG_F_SEND_AND_CLOSE;
#endif
    }
}

#if MG_ENABLE_HTTP_STREAMING_MULTIPART
void mg_file_upload_handler(struct mg_connection *nc, int ev, void *ev_data,
                            mg_fu_fname_fn local_name_fn
                                MG_UD_ARG(void *user_data)) {
  switch (ev) {
    case MG_EV_HTTP_PART_BEGIN: {
      struct mg_http_multipart_part *mp =
          (struct mg_http_multipart_part *) ev_data;
      struct file_upload_state *fus;
      struct mg_str lfn = local_name_fn(nc, mg_mk_str(mp->file_name));
      mp->user_data = NULL;
      if (lfn.p == NULL || lfn.len == 0) {
        LOG(LL_ERROR, ("%p Not allowed to upload %s", nc, mp->file_name));
        mg_printf(nc,
                  "HTTP/1.1 403 Not Allowed\r\n"
                  "Content-Type: text/plain\r\n"
                  "Connection: close\r\n\r\n"
                  "Not allowed to upload %s\r\n",
                  mp->file_name);
        nc->flags |= MG_F_SEND_AND_CLOSE;
        return;
      }
      fus = (struct file_upload_state *) MG_CALLOC(1, sizeof(*fus));
      if (fus == NULL) {
        nc->flags |= MG_F_CLOSE_IMMEDIATELY;
        return;
      }
      fus->lfn = (char *) MG_MALLOC(lfn.len + 1);
      memcpy(fus->lfn, lfn.p, lfn.len);
      fus->lfn[lfn.len] = '\0';
      if (lfn.p != mp->file_name) MG_FREE((char *) lfn.p);
      LOG(LL_DEBUG,
          ("%p Receiving file %s -> %s", nc, mp->file_name, fus->lfn));
      fus->fp = mg_fopen(fus->lfn, "wb");
      if (fus->fp == NULL) {
        mg_printf(nc,
                  "HTTP/1.1 500 Internal Server Error\r\n"
                  "Content-Type: text/plain\r\n"
                  "Connection: close\r\n\r\n");
        LOG(LL_ERROR, ("Failed to open %s: %d\n", fus->lfn, mg_get_errno()));
        mg_printf(nc, "Failed to open %s: %d\n", fus->lfn, mg_get_errno());
        /* Do not close the connection just yet, discard remainder of the data.
         * This is because at the time of writing some browsers (Chrome) fail to
         * render response before all the data is sent. */
      }
      mp->user_data = (void *) fus;
      break;
    }
    case MG_EV_HTTP_PART_DATA: {
      struct mg_http_multipart_part *mp =
          (struct mg_http_multipart_part *) ev_data;
      struct file_upload_state *fus =
          (struct file_upload_state *) mp->user_data;
      if (fus == NULL || fus->fp == NULL) break;
      if (mg_fwrite(mp->data.p, 1, mp->data.len, fus->fp) != mp->data.len) {
        LOG(LL_ERROR, ("Failed to write to %s: %d, wrote %d", fus->lfn,
                       mg_get_errno(), (int) fus->num_recd));
        if (mg_get_errno() == ENOSPC
#ifdef SPIFFS_ERR_FULL
            || mg_get_errno() == SPIFFS_ERR_FULL
#endif
            ) {
          mg_printf(nc,
                    "HTTP/1.1 413 Payload Too Large\r\n"
                    "Content-Type: text/plain\r\n"
                    "Connection: close\r\n\r\n");
          mg_printf(nc, "Failed to write to %s: no space left; wrote %d\r\n",
                    fus->lfn, (int) fus->num_recd);
        } else {
          mg_printf(nc,
                    "HTTP/1.1 500 Internal Server Error\r\n"
                    "Content-Type: text/plain\r\n"
                    "Connection: close\r\n\r\n");
          mg_printf(nc, "Failed to write to %s: %d, wrote %d", mp->file_name,
                    mg_get_errno(), (int) fus->num_recd);
        }
        fclose(fus->fp);
        remove(fus->lfn);
        fus->fp = NULL;
        /* Do not close the connection just yet, discard remainder of the data.
         * This is because at the time of writing some browsers (Chrome) fail to
         * render response before all the data is sent. */
        return;
      }
      fus->num_recd += mp->data.len;
      LOG(LL_DEBUG, ("%p rec'd %d bytes, %d total", nc, (int) mp->data.len,
                     (int) fus->num_recd));
      break;
    }
    case MG_EV_HTTP_PART_END: {
      struct mg_http_multipart_part *mp =
          (struct mg_http_multipart_part *) ev_data;
      struct file_upload_state *fus =
          (struct file_upload_state *) mp->user_data;
      if (fus == NULL) break;
      if (mp->status >= 0 && fus->fp != NULL) {
        LOG(LL_DEBUG, ("%p Uploaded %s (%s), %d bytes", nc, mp->file_name,
                       fus->lfn, (int) fus->num_recd));
      } else {
        LOG(LL_ERROR, ("Failed to store %s (%s)", mp->file_name, fus->lfn));
        /*
         * mp->status < 0 means connection was terminated, so no reason to send
         * HTTP reply
         */
      }
      if (fus->fp != NULL) fclose(fus->fp);
      MG_FREE(fus->lfn);
      MG_FREE(fus);
      mp->user_data = NULL;
      /* Don't close the connection yet, there may be more files to come. */
      break;
    }
    case MG_EV_HTTP_MULTIPART_REQUEST_END: {
      mg_printf(nc,
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/plain\r\n"
                "Connection: close\r\n\r\n"
                "Ok.\r\n");
      nc->flags |= MG_F_SEND_AND_CLOSE;
      break;
    }
  }

#if MG_ENABLE_CALLBACK_USERDATA
    (void) user_data;
#endif
}

#endif /* MG_ENABLE_HTTP_STREAMING_MULTIPART */
#endif /* MG_ENABLE_FILESYSTEM */

struct mg_connection *mg_connect_http_base(
    struct mg_mgr *mgr, MG_CB(mg_event_handler_t ev_handler, void *user_data),
    struct mg_connect_opts opts, const char *scheme1, const char *scheme2,
    const char *scheme_ssl1, const char *scheme_ssl2, const char *url,
    struct mg_str *path, struct mg_str *user_info, struct mg_str *host) {
  struct mg_connection *nc = NULL;
  unsigned int port_i = 0;
  int use_ssl = 0;
  struct mg_str scheme, query, fragment;
  char conn_addr_buf[2];
  char *conn_addr = conn_addr_buf;

  if (mg_parse_uri(mg_mk_str(url), &scheme, user_info, host, &port_i, path,
                   &query, &fragment) != 0) {
    MG_SET_PTRPTR(opts.error_string, "cannot parse url");
    goto out;
  }

  /* If query is present, do not strip it. Pass to the caller. */
  if (query.len > 0) path->len += query.len + 1;

  if (scheme.len == 0 || mg_vcmp(&scheme, scheme1) == 0 ||
      (scheme2 != NULL && mg_vcmp(&scheme, scheme2) == 0)) {
    use_ssl = 0;
    if (port_i == 0) port_i = 80;
  } else if (mg_vcmp(&scheme, scheme_ssl1) == 0 ||
             (scheme2 != NULL && mg_vcmp(&scheme, scheme_ssl2) == 0)) {
    use_ssl = 1;
    if (port_i == 0) port_i = 443;
  } else {
    goto out;
  }

  mg_asprintf(&conn_addr, sizeof(conn_addr_buf), "tcp://%.*s:%u",
              (int) host->len, host->p, port_i);
  if (conn_addr == NULL) goto out;

  LOG(LL_DEBUG, ("%s use_ssl? %d %s", url, use_ssl, conn_addr));
  if (use_ssl) {
#if MG_ENABLE_SSL
    /*
     * Schema requires SSL, but no SSL parameters were provided in opts.
     * In order to maintain backward compatibility, use a faux-SSL with no
     * verification.
     */
    if (opts.ssl_ca_cert == NULL) {
      opts.ssl_ca_cert = "*";
    }
#else
    MG_SET_PTRPTR(opts.error_string, "ssl is disabled");
    goto out;
#endif
  }

  if ((nc = mg_connect_opt(mgr, conn_addr, MG_CB(ev_handler, user_data),
                           opts)) != NULL) {
    mg_set_protocol_http_websocket(nc);
  }

out:
  if (conn_addr != NULL && conn_addr != conn_addr_buf) MG_FREE(conn_addr);
  return nc;
}

struct mg_connection *mg_connect_http_opt(
    struct mg_mgr *mgr, MG_CB(mg_event_handler_t ev_handler, void *user_data),
    struct mg_connect_opts opts, const char *url, const char *extra_headers,
    const char *post_data) {
  struct mg_str user = MG_NULL_STR, null_str = MG_NULL_STR;
  struct mg_str host = MG_NULL_STR, path = MG_NULL_STR;
  struct mbuf auth;
  struct mg_connection *nc =
      mg_connect_http_base(mgr, MG_CB(ev_handler, user_data), opts, "http",
                           NULL, "https", NULL, url, &path, &user, &host);

  if (nc == NULL) {
    return NULL;
  }

  mbuf_init(&auth, 0);
  if (user.len > 0) {
    mg_basic_auth_header(user, null_str, &auth);
  }

  if (post_data == NULL) post_data = "";
  if (extra_headers == NULL) extra_headers = "";
  if (path.len == 0) path = mg_mk_str("/");
  if (host.len == 0) host = mg_mk_str("");

  mg_printf(nc, "%s %.*s HTTP/1.1\r\nHost: %.*s\r\nContent-Length: %" SIZE_T_FMT
                "\r\n%.*s%s\r\n%s",
            (post_data[0] == '\0' ? "GET" : "POST"), (int) path.len, path.p,
            (int) (path.p - host.p), host.p, strlen(post_data), (int) auth.len,
            (auth.buf == NULL ? "" : auth.buf), extra_headers, post_data);

  mbuf_free(&auth);
  return nc;
}

struct mg_connection *mg_connect_http(struct mg_mgr *mgr, 
                                      MG_CB(mg_event_handler_t ev_handler, void *user_data),
                                      const char *url, 
                                      const char *extra_headers, 
                                      const char *post_data) 
{
    struct mg_connect_opts opts;
    memset(&opts, 0, sizeof(opts));
    return mg_connect_http_opt(mgr, 
                                MG_CB(ev_handler, user_data), 
                                opts, 
                                url,
                                extra_headers, 
                                post_data);
}

size_t mg_parse_multipart(const char *buf, size_t buf_len, char *var_name,
                          size_t var_name_len, char *file_name,
                          size_t file_name_len, const char **data,
                          size_t *data_len) {
  static const char cd[] = "Content-Disposition: ";
  size_t hl, bl, n, ll, pos, cdl = sizeof(cd) - 1;
  int shl;

  if (buf == NULL || buf_len <= 0) return 0;
  if ((shl = mg_http_get_request_len(buf, buf_len)) <= 0) return 0;
  hl = shl;
  if (buf[0] != '-' || buf[1] != '-' || buf[2] == '\n') return 0;

  /* Get boundary length */
  bl = mg_get_line_len(buf, buf_len);

  /* Loop through headers, fetch variable name and file name */
  var_name[0] = file_name[0] = '\0';
  for (n = bl; (ll = mg_get_line_len(buf + n, hl - n)) > 0; n += ll) {
    if (mg_ncasecmp(cd, buf + n, cdl) == 0) {
      struct mg_str header;
      header.p = buf + n + cdl;
      header.len = ll - (cdl + 2);
      {
        char *var_name2 = var_name;
        mg_http_parse_header2(&header, "name", &var_name2, var_name_len);
        /* TODO: handle reallocated buffer correctly */
        if (var_name2 != var_name) {
          MG_FREE(var_name2);
          var_name[0] = '\0';
        }
      }
      {
        char *file_name2 = file_name;
        mg_http_parse_header2(&header, "filename", &file_name2, file_name_len);
        /* TODO: handle reallocated buffer correctly */
        if (file_name2 != file_name) {
          MG_FREE(file_name2);
          file_name[0] = '\0';
        }
      }
    }
  }

  /* Scan through the body, search for terminating boundary */
  for (pos = hl; pos + (bl - 2) < buf_len; pos++) {
    if (buf[pos] == '-' && !strncmp(buf, &buf[pos], bl - 2)) {
      if (data_len != NULL) *data_len = (pos - 2) - hl;
      if (data != NULL) *data = buf + hl;
      return pos;
    }
  }

  return 0;
}

void mg_register_http_endpoint_opt(struct mg_connection *nc,
                                   const char *uri_path,
                                   mg_event_handler_t handler,
                                   struct mg_http_endpoint_opts opts) 
{
    struct mg_http_proto_data *pd = NULL;
    struct mg_http_endpoint *new_ep = NULL;

    if (nc == NULL) return;
    new_ep = (struct mg_http_endpoint *) MG_CALLOC(1, sizeof(*new_ep));
    if (new_ep == NULL) return;

    pd = mg_http_get_proto_data(nc);
    new_ep->uri_pattern = mg_strdup(mg_mk_str(uri_path));
    if (opts.auth_domain != NULL && opts.auth_file != NULL) {
        new_ep->auth_domain = strdup(opts.auth_domain);
        new_ep->auth_file = strdup(opts.auth_file);
    }
    
    new_ep->handler = handler;

#if MG_ENABLE_CALLBACK_USERDATA
    new_ep->user_data = opts.user_data;
#endif
    new_ep->next = pd->endpoints;
    pd->endpoints = new_ep;
}

static void mg_http_call_endpoint_handler(struct mg_connection *nc, 
                                          int ev,
                                          struct http_message *hm) 
{
    struct mg_http_proto_data *pd = mg_http_get_proto_data(nc);
    void *user_data = nc->user_data;

    if (ev == MG_EV_HTTP_REQUEST
#if MG_ENABLE_HTTP_STREAMING_MULTIPART
      || ev == MG_EV_HTTP_MULTIPART_REQUEST
#endif
      ) {
        struct mg_http_endpoint *ep = mg_http_get_endpoint_handler(nc->listener, &hm->uri);
        if (ep != NULL) {
            #if MG_ENABLE_FILESYSTEM && !MG_DISABLE_HTTP_DIGEST_AUTH
            if (!mg_http_is_authorized(hm, hm->uri, ep->auth_domain, ep->auth_file,
                                 MG_AUTH_FLAG_IS_GLOBAL_PASS_FILE)) {
            mg_http_send_digest_auth_request(nc, ep->auth_domain);
            return;
        }
            #endif
            pd->endpoint_handler = ep->handler;
            
            #if MG_ENABLE_CALLBACK_USERDATA
            user_data = ep->user_data;
            #endif
        }
    }
    
    mg_call(nc, pd->endpoint_handler ? pd->endpoint_handler : nc->handler,
          user_data, ev, hm);
}

void mg_register_http_endpoint(struct mg_connection *nc, 
                                const char *uri_path,
                                MG_CB(mg_event_handler_t handler, 
                                void *user_data)) 
{
    struct mg_http_endpoint_opts opts;
    memset(&opts, 0, sizeof(opts));

#if MG_ENABLE_CALLBACK_USERDATA
    opts.user_data = user_data;
#endif
    mg_register_http_endpoint_opt(nc, uri_path, handler, opts);
}

#endif /* MG_ENABLE_HTTP */


#ifndef _WIN32
#include <signal.h>
#endif

#if MG_ENABLE_HTTP && MG_ENABLE_HTTP_CGI

#ifndef MG_MAX_CGI_ENVIR_VARS
#define MG_MAX_CGI_ENVIR_VARS 64
#endif

#ifndef MG_ENV_EXPORT_TO_CGI
#define MG_ENV_EXPORT_TO_CGI "MONGOOSE_CGI"
#endif

#define MG_F_HTTP_CGI_PARSE_HEADERS MG_F_USER_1

/*
 * This structure helps to create an environment for the spawned CGI program.
 * Environment is an array of "VARIABLE=VALUE\0" ASCIIZ strings,
 * last element must be NULL.
 * However, on Windows there is a requirement that all these VARIABLE=VALUE\0
 * strings must reside in a contiguous buffer. The end of the buffer is
 * marked by two '\0' characters.
 * We satisfy both worlds: we create an envp array (which is vars), all
 * entries are actually pointers inside buf.
 */
struct mg_cgi_env_block {
  struct mg_connection *nc;
  char buf[MG_CGI_ENVIRONMENT_SIZE];       /* Environment buffer */
  const char *vars[MG_MAX_CGI_ENVIR_VARS]; /* char *envp[] */
  int len;                                 /* Space taken */
  int nvars;                               /* Number of variables in envp[] */
};


static int mg_start_process(const char *interp, const char *cmd,
                            const char *env, const char *envp[],
                            const char *dir, sock_t sock) {
  char buf[500];
  pid_t pid = fork();
  (void) env;

  if (pid == 0) {
    /*
     * In Linux `chdir` declared with `warn_unused_result` attribute
     * To shutup compiler we have yo use result in some way
     */
    int tmp = chdir(dir);
    (void) tmp;
    (void) dup2(sock, 0);
    (void) dup2(sock, 1);
    closesocket(sock);

    /*
     * After exec, all signal handlers are restored to their default values,
     * with one exception of SIGCHLD. According to POSIX.1-2001 and Linux's
     * implementation, SIGCHLD's handler will leave unchanged after exec
     * if it was set to be ignored. Restore it to default action.
     */
    signal(SIGCHLD, SIG_DFL);

    if (interp == NULL) {
      execle(cmd, cmd, (char *) 0, envp); /* (char *) 0 to squash warning */
    } else {
      execle(interp, interp, cmd, (char *) 0, envp);
    }
    snprintf(buf, sizeof(buf),
             "Status: 500\r\n\r\n"
             "500 Server Error: %s%s%s: %s",
             interp == NULL ? "" : interp, interp == NULL ? "" : " ", cmd,
             strerror(errno));
    send(1, buf, strlen(buf), 0);
    _exit(EXIT_FAILURE); /* exec call failed */
  }

  return (pid != 0);
}


/*
 * Append VARIABLE=VALUE\0 string to the buffer, and add a respective
 * pointer into the vars array.
 */
static char *mg_addenv(struct mg_cgi_env_block *block, const char *fmt, ...) {
  int n, space;
  char *added = block->buf + block->len;
  va_list ap;

  /* Calculate how much space is left in the buffer */
  space = sizeof(block->buf) - (block->len + 2);
  if (space > 0) {
    /* Copy VARIABLE=VALUE\0 string into the free space */
    va_start(ap, fmt);
    n = vsnprintf(added, (size_t) space, fmt, ap);
    va_end(ap);

    /* Make sure we do not overflow buffer and the envp array */
    if (n > 0 && n + 1 < space &&
        block->nvars < (int) ARRAY_SIZE(block->vars) - 2) {
      /* Append a pointer to the added string into the envp array */
      block->vars[block->nvars++] = added;
      /* Bump up used length counter. Include \0 terminator */
      block->len += n + 1;
    }
  }

  return added;
}

static void mg_addenv2(struct mg_cgi_env_block *blk, const char *name) {
  const char *s;
  if ((s = getenv(name)) != NULL) mg_addenv(blk, "%s=%s", name, s);
}

static void mg_prepare_cgi_environment(struct mg_connection *nc,
                                       const char *prog,
                                       const struct mg_str *path_info,
                                       const struct http_message *hm,
                                       const struct mg_serve_http_opts *opts,
                                       struct mg_cgi_env_block *blk) {
  const char *s;
  struct mg_str *h;
  char *p;
  size_t i;
  char buf[100];
  size_t path_info_len = path_info != NULL ? path_info->len : 0;

  blk->len = blk->nvars = 0;
  blk->nc = nc;

  if ((s = getenv("SERVER_NAME")) != NULL) {
    mg_addenv(blk, "SERVER_NAME=%s", s);
  } else {
    mg_sock_to_str(nc->sock, buf, sizeof(buf), 3);
    mg_addenv(blk, "SERVER_NAME=%s", buf);
  }
  mg_addenv(blk, "SERVER_ROOT=%s", opts->document_root);
  mg_addenv(blk, "DOCUMENT_ROOT=%s", opts->document_root);
  mg_addenv(blk, "SERVER_SOFTWARE=%s/%s", "Mongoose", MG_VERSION);

  /* Prepare the environment block */
  mg_addenv(blk, "%s", "GATEWAY_INTERFACE=CGI/1.1");
  mg_addenv(blk, "%s", "SERVER_PROTOCOL=HTTP/1.1");
  mg_addenv(blk, "%s", "REDIRECT_STATUS=200"); /* For PHP */

  mg_addenv(blk, "REQUEST_METHOD=%.*s", (int) hm->method.len, hm->method.p);

  mg_addenv(blk, "REQUEST_URI=%.*s%s%.*s", (int) hm->uri.len, hm->uri.p,
            hm->query_string.len == 0 ? "" : "?", (int) hm->query_string.len,
            hm->query_string.p);

  mg_conn_addr_to_str(nc, buf, sizeof(buf),
                      MG_SOCK_STRINGIFY_REMOTE | MG_SOCK_STRINGIFY_IP);
  mg_addenv(blk, "REMOTE_ADDR=%s", buf);
  mg_conn_addr_to_str(nc, buf, sizeof(buf), MG_SOCK_STRINGIFY_PORT);
  mg_addenv(blk, "SERVER_PORT=%s", buf);

  s = hm->uri.p + hm->uri.len - path_info_len - 1;
  if (*s == '/') {
    const char *base_name = strrchr(prog, DIRSEP);
    mg_addenv(blk, "SCRIPT_NAME=%.*s/%s", (int) (s - hm->uri.p), hm->uri.p,
              (base_name != NULL ? base_name + 1 : prog));
  } else {
    mg_addenv(blk, "SCRIPT_NAME=%.*s", (int) (s - hm->uri.p + 1), hm->uri.p);
  }
  mg_addenv(blk, "SCRIPT_FILENAME=%s", prog);

  if (path_info != NULL && path_info->len > 0) {
    mg_addenv(blk, "PATH_INFO=%.*s", (int) path_info->len, path_info->p);
    /* Not really translated... */
    mg_addenv(blk, "PATH_TRANSLATED=%.*s", (int) path_info->len, path_info->p);
  }

#if MG_ENABLE_SSL
  mg_addenv(blk, "HTTPS=%s", (nc->flags & MG_F_SSL ? "on" : "off"));
#else
  mg_addenv(blk, "HTTPS=off");
#endif

  if ((h = mg_get_http_header((struct http_message *) hm, "Content-Type")) !=
      NULL) {
    mg_addenv(blk, "CONTENT_TYPE=%.*s", (int) h->len, h->p);
  }

  if (hm->query_string.len > 0) {
    mg_addenv(blk, "QUERY_STRING=%.*s", (int) hm->query_string.len,
              hm->query_string.p);
  }

  if ((h = mg_get_http_header((struct http_message *) hm, "Content-Length")) !=
      NULL) {
    mg_addenv(blk, "CONTENT_LENGTH=%.*s", (int) h->len, h->p);
  }

  mg_addenv2(blk, "PATH");
  mg_addenv2(blk, "TMP");
  mg_addenv2(blk, "TEMP");
  mg_addenv2(blk, "TMPDIR");
  mg_addenv2(blk, "PERLLIB");
  mg_addenv2(blk, MG_ENV_EXPORT_TO_CGI);

#ifdef _WIN32
  mg_addenv2(blk, "COMSPEC");
  mg_addenv2(blk, "SYSTEMROOT");
  mg_addenv2(blk, "SystemDrive");
  mg_addenv2(blk, "ProgramFiles");
  mg_addenv2(blk, "ProgramFiles(x86)");
  mg_addenv2(blk, "CommonProgramFiles(x86)");
#else
  mg_addenv2(blk, "LD_LIBRARY_PATH");
#endif /* _WIN32 */

  /* Add all headers as HTTP_* variables */
  for (i = 0; hm->header_names[i].len > 0; i++) {
    p = mg_addenv(blk, "HTTP_%.*s=%.*s", (int) hm->header_names[i].len,
                  hm->header_names[i].p, (int) hm->header_values[i].len,
                  hm->header_values[i].p);

    /* Convert variable name into uppercase, and change - to _ */
    for (; *p != '=' && *p != '\0'; p++) {
      if (*p == '-') *p = '_';
      *p = (char) toupper(*(unsigned char *) p);
    }
  }

  blk->vars[blk->nvars++] = NULL;
  blk->buf[blk->len++] = '\0';
}

static void mg_cgi_ev_handler(struct mg_connection *cgi_nc, int ev,
                              void *ev_data MG_UD_ARG(void *user_data)) {
#if !MG_ENABLE_CALLBACK_USERDATA
  void *user_data = cgi_nc->user_data;
#endif
  struct mg_connection *nc = (struct mg_connection *) user_data;
  (void) ev_data;

  if (nc == NULL) {
    /* The corresponding network connection was closed. */
    cgi_nc->flags |= MG_F_CLOSE_IMMEDIATELY;
    return;
  }

  switch (ev) {
    case MG_EV_RECV:
      /*
       * CGI script does not output reply line, like "HTTP/1.1 CODE XXXXX\n"
       * It outputs headers, then body. Headers might include "Status"
       * header, which changes CODE, and it might include "Location" header
       * which changes CODE to 302.
       *
       * Therefore we do not send the output from the CGI script to the user
       * until all CGI headers are received.
       *
       * Here we parse the output from the CGI script, and if all headers has
       * been received, send appropriate reply line, and forward all
       * received headers to the client.
       */
      if (nc->flags & MG_F_HTTP_CGI_PARSE_HEADERS) {
        struct mbuf *io = &cgi_nc->recv_mbuf;
        int len = mg_http_get_request_len(io->buf, io->len);

        if (len == 0) break;
        if (len < 0 || io->len > MG_MAX_HTTP_REQUEST_SIZE) {
          cgi_nc->flags |= MG_F_CLOSE_IMMEDIATELY;
          mg_http_send_error(nc, 500, "Bad headers");
        } else {
          struct http_message hm;
          struct mg_str *h;
          mg_http_parse_headers(io->buf, io->buf + io->len, io->len, &hm);
          if (mg_get_http_header(&hm, "Location") != NULL) {
            mg_printf(nc, "%s", "HTTP/1.1 302 Moved\r\n");
          } else if ((h = mg_get_http_header(&hm, "Status")) != NULL) {
            mg_printf(nc, "HTTP/1.1 %.*s\r\n", (int) h->len, h->p);
          } else {
            mg_printf(nc, "%s", "HTTP/1.1 200 OK\r\n");
          }
        }
        nc->flags &= ~MG_F_HTTP_CGI_PARSE_HEADERS;
      }
      if (!(nc->flags & MG_F_HTTP_CGI_PARSE_HEADERS)) {
        mg_forward(cgi_nc, nc);
      }
      break;
    case MG_EV_CLOSE:
      DBG(("%p CLOSE", cgi_nc));
      mg_http_free_proto_data_cgi(&mg_http_get_proto_data(nc)->cgi);
      nc->flags |= MG_F_SEND_AND_CLOSE;
      break;
  }
}

MG_INTERNAL void mg_handle_cgi(struct mg_connection *nc, const char *prog,
                               const struct mg_str *path_info,
                               const struct http_message *hm,
                               const struct mg_serve_http_opts *opts) {
  struct mg_cgi_env_block blk;
  char dir[MG_MAX_PATH];
  const char *p;
  sock_t fds[2];

  DBG(("%p [%s]", nc, prog));
  mg_prepare_cgi_environment(nc, prog, path_info, hm, opts, &blk);
  /*
   * CGI must be executed in its own directory. 'dir' must point to the
   * directory containing executable program, 'p' must point to the
   * executable program name relative to 'dir'.
   */
  if ((p = strrchr(prog, DIRSEP)) == NULL) {
    snprintf(dir, sizeof(dir), "%s", ".");
  } else {
    snprintf(dir, sizeof(dir), "%.*s", (int) (p - prog), prog);
    prog = p + 1;
  }

  if (!mg_socketpair(fds, SOCK_STREAM)) {
    nc->flags |= MG_F_CLOSE_IMMEDIATELY;
    return;
  }

#ifndef _WIN32
  struct sigaction sa;

  sigemptyset(&sa.sa_mask);
  sa.sa_handler = SIG_IGN;
  sa.sa_flags = 0;
  sigaction(SIGCHLD, &sa, NULL);
#endif

  if (mg_start_process(opts->cgi_interpreter, prog, blk.buf, blk.vars, dir,
                       fds[1]) != 0) {
    struct mg_connection *cgi_nc =
        mg_add_sock(nc->mgr, fds[0], mg_cgi_ev_handler MG_UD_ARG(nc));
    struct mg_http_proto_data *cgi_pd = mg_http_get_proto_data(nc);
    cgi_pd->cgi.cgi_nc = cgi_nc;
#if !MG_ENABLE_CALLBACK_USERDATA
    cgi_pd->cgi.cgi_nc->user_data = nc;
#endif
    nc->flags |= MG_F_HTTP_CGI_PARSE_HEADERS;
    /* Push POST data to the CGI */
    if (hm->body.len > 0) {
      mg_send(cgi_pd->cgi.cgi_nc, hm->body.p, hm->body.len);
    }
    mbuf_remove(&nc->recv_mbuf, nc->recv_mbuf.len);
  } else {
    closesocket(fds[0]);
    mg_http_send_error(nc, 500, "CGI failure");
  }

#ifndef _WIN32
  closesocket(fds[1]); /* On Windows, CGI stdio thread closes that socket */
#endif
}

MG_INTERNAL void mg_http_free_proto_data_cgi(struct mg_http_proto_data_cgi *d) {
  if (d == NULL) return;
  if (d->cgi_nc != NULL) {
    d->cgi_nc->flags |= MG_F_CLOSE_IMMEDIATELY;
    d->cgi_nc->user_data = NULL;
  }
  memset(d, 0, sizeof(*d));
}

#endif /* MG_ENABLE_HTTP && MG_ENABLE_HTTP_CGI */

/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#if MG_ENABLE_HTTP && MG_ENABLE_HTTP_SSI && MG_ENABLE_FILESYSTEM

static void mg_send_ssi_file(struct mg_connection *nc, struct http_message *hm,
                             const char *path, FILE *fp, int include_level,
                             const struct mg_serve_http_opts *opts);

static void mg_send_file_data(struct mg_connection *nc, FILE *fp) {
  char buf[BUFSIZ];
  size_t n;
  while ((n = mg_fread(buf, 1, sizeof(buf), fp)) > 0) {
    mg_send(nc, buf, n);
  }
}

static void mg_do_ssi_include(struct mg_connection *nc, struct http_message *hm,
                              const char *ssi, char *tag, int include_level,
                              const struct mg_serve_http_opts *opts) {
  char file_name[MG_MAX_PATH], path[MG_MAX_PATH], *p;
  FILE *fp;

  /*
   * sscanf() is safe here, since send_ssi_file() also uses buffer
   * of size MG_BUF_LEN to get the tag. So strlen(tag) is always < MG_BUF_LEN.
   */
  if (sscanf(tag, " virtual=\"%[^\"]\"", file_name) == 1) {
    /* File name is relative to the webserver root */
    snprintf(path, sizeof(path), "%s/%s", opts->document_root, file_name);
  } else if (sscanf(tag, " abspath=\"%[^\"]\"", file_name) == 1) {
    /*
     * File name is relative to the webserver working directory
     * or it is absolute system path
     */
    snprintf(path, sizeof(path), "%s", file_name);
  } else if (sscanf(tag, " file=\"%[^\"]\"", file_name) == 1 ||
             sscanf(tag, " \"%[^\"]\"", file_name) == 1) {
    /* File name is relative to the currect document */
    snprintf(path, sizeof(path), "%s", ssi);
    if ((p = strrchr(path, DIRSEP)) != NULL) {
      p[1] = '\0';
    }
    snprintf(path + strlen(path), sizeof(path) - strlen(path), "%s", file_name);
  } else {
    mg_printf(nc, "Bad SSI #include: [%s]", tag);
    return;
  }

  if ((fp = mg_fopen(path, "rb")) == NULL) {
    mg_printf(nc, "SSI include error: mg_fopen(%s): %s", path,
              strerror(mg_get_errno()));
  } else {
    mg_set_close_on_exec((sock_t) fileno(fp));
    if (mg_match_prefix(opts->ssi_pattern, strlen(opts->ssi_pattern), path) >
        0) {
      mg_send_ssi_file(nc, hm, path, fp, include_level + 1, opts);
    } else {
      mg_send_file_data(nc, fp);
    }
    fclose(fp);
  }
}

#if MG_ENABLE_HTTP_SSI_EXEC
static void do_ssi_exec(struct mg_connection *nc, char *tag) {
  char cmd[BUFSIZ];
  FILE *fp;

  if (sscanf(tag, " \"%[^\"]\"", cmd) != 1) {
    mg_printf(nc, "Bad SSI #exec: [%s]", tag);
  } else if ((fp = popen(cmd, "r")) == NULL) {
    mg_printf(nc, "Cannot SSI #exec: [%s]: %s", cmd, strerror(mg_get_errno()));
  } else {
    mg_send_file_data(nc, fp);
    pclose(fp);
  }
}
#endif /* MG_ENABLE_HTTP_SSI_EXEC */

/*
 * SSI directive has the following format:
 * <!--#directive parameter=value parameter=value -->
 */
static void mg_send_ssi_file(struct mg_connection *nc, struct http_message *hm,
                             const char *path, FILE *fp, int include_level,
                             const struct mg_serve_http_opts *opts) {
  static const struct mg_str btag = MG_MK_STR("<!--#");
  static const struct mg_str d_include = MG_MK_STR("include");
  static const struct mg_str d_call = MG_MK_STR("call");
#if MG_ENABLE_HTTP_SSI_EXEC
  static const struct mg_str d_exec = MG_MK_STR("exec");
#endif
  char buf[BUFSIZ], *p = buf + btag.len; /* p points to SSI directive */
  int ch, len, in_ssi_tag;

  if (include_level > 10) {
    mg_printf(nc, "SSI #include level is too deep (%s)", path);
    return;
  }

  in_ssi_tag = len = 0;
  while ((ch = fgetc(fp)) != EOF) {
    if (in_ssi_tag && ch == '>' && buf[len - 1] == '-' && buf[len - 2] == '-') {
      size_t i = len - 2;
      in_ssi_tag = 0;

      /* Trim closing --> */
      buf[i--] = '\0';
      while (i > 0 && buf[i] == ' ') {
        buf[i--] = '\0';
      }

      /* Handle known SSI directives */
      if (strncmp(p, d_include.p, d_include.len) == 0) {
        mg_do_ssi_include(nc, hm, path, p + d_include.len + 1, include_level,
                          opts);
      } else if (strncmp(p, d_call.p, d_call.len) == 0) {
        struct mg_ssi_call_ctx cctx;
        memset(&cctx, 0, sizeof(cctx));
        cctx.req = hm;
        cctx.file = mg_mk_str(path);
        cctx.arg = mg_mk_str(p + d_call.len + 1);
        mg_call(nc, NULL, nc->user_data, MG_EV_SSI_CALL,
                (void *) cctx.arg.p); /* NUL added above */
        mg_call(nc, NULL, nc->user_data, MG_EV_SSI_CALL_CTX, &cctx);
#if MG_ENABLE_HTTP_SSI_EXEC
      } else if (strncmp(p, d_exec.p, d_exec.len) == 0) {
        do_ssi_exec(nc, p + d_exec.len + 1);
#endif
      } else {
        /* Silently ignore unknown SSI directive. */
      }
      len = 0;
    } else if (ch == '<') {
      in_ssi_tag = 1;
      if (len > 0) {
        mg_send(nc, buf, (size_t) len);
      }
      len = 0;
      buf[len++] = ch & 0xff;
    } else if (in_ssi_tag) {
      if (len == (int) btag.len && strncmp(buf, btag.p, btag.len) != 0) {
        /* Not an SSI tag */
        in_ssi_tag = 0;
      } else if (len == (int) sizeof(buf) - 2) {
        mg_printf(nc, "%s: SSI tag is too large", path);
        len = 0;
      }
      buf[len++] = ch & 0xff;
    } else {
      buf[len++] = ch & 0xff;
      if (len == (int) sizeof(buf)) {
        mg_send(nc, buf, (size_t) len);
        len = 0;
      }
    }
  }

  /* Send the rest of buffered data */
  if (len > 0) {
    mg_send(nc, buf, (size_t) len);
  }
}

MG_INTERNAL void mg_handle_ssi_request(struct mg_connection *nc,
                                       struct http_message *hm,
                                       const char *path,
                                       const struct mg_serve_http_opts *opts) {
  FILE *fp;
  struct mg_str mime_type;
  DBG(("%p %s", nc, path));

  if ((fp = mg_fopen(path, "rb")) == NULL) {
    mg_http_send_error(nc, 404, NULL);
  } else {
    mg_set_close_on_exec((sock_t) fileno(fp));

    mime_type = mg_get_mime_type(path, "text/plain", opts);
    mg_send_response_line(nc, 200, opts->extra_headers);
    mg_printf(nc,
              "Content-Type: %.*s\r\n"
              "Connection: close\r\n\r\n",
              (int) mime_type.len, mime_type.p);
    mg_send_ssi_file(nc, hm, path, fp, 0, opts);
    fclose(fp);
    nc->flags |= MG_F_SEND_AND_CLOSE;
  }
}

#endif /* MG_ENABLE_HTTP_SSI && MG_ENABLE_HTTP && MG_ENABLE_FILESYSTEM */


#if MG_ENABLE_HTTP && MG_ENABLE_HTTP_WEBDAV

MG_INTERNAL int mg_is_dav_request(const struct mg_str *s) {
  static const char *methods[] = {
    "PUT",
    "DELETE",
    "MKCOL",
    "PROPFIND",
    "MOVE"
#if MG_ENABLE_FAKE_DAVLOCK
    ,
    "LOCK",
    "UNLOCK"
#endif
  };
  size_t i;

  for (i = 0; i < ARRAY_SIZE(methods); i++) {
    if (mg_vcmp(s, methods[i]) == 0) {
      return 1;
    }
  }

  return 0;
}

static int mg_mkdir(const char *path, uint32_t mode) {
#ifndef _WIN32
  return mkdir(path, mode);
#else
  (void) mode;
  return _mkdir(path);
#endif
}

static void mg_print_props(struct mg_connection *nc, const char *name,
                           cs_stat_t *stp) {
  char mtime[64];
  time_t t = stp->st_mtime; /* store in local variable for NDK compile */
  struct mg_str name_esc = mg_url_encode(mg_mk_str(name));
  mg_gmt_time_string(mtime, sizeof(mtime), &t);
  mg_printf(nc,
            "<d:response>"
            "<d:href>%s</d:href>"
            "<d:propstat>"
            "<d:prop>"
            "<d:resourcetype>%s</d:resourcetype>"
            "<d:getcontentlength>%" INT64_FMT
            "</d:getcontentlength>"
            "<d:getlastmodified>%s</d:getlastmodified>"
            "</d:prop>"
            "<d:status>HTTP/1.1 200 OK</d:status>"
            "</d:propstat>"
            "</d:response>\n",
            name_esc.p, S_ISDIR(stp->st_mode) ? "<d:collection/>" : "",
            (int64_t) stp->st_size, mtime);
  free((void *) name_esc.p);
}

MG_INTERNAL void mg_handle_propfind(struct mg_connection *nc, const char *path,
                                    cs_stat_t *stp, struct http_message *hm,
                                    struct mg_serve_http_opts *opts) {
  static const char header[] =
      "HTTP/1.1 207 Multi-Status\r\n"
      "Connection: close\r\n"
      "Content-Type: text/xml; charset=utf-8\r\n\r\n"
      "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
      "<d:multistatus xmlns:d='DAV:'>\n";
  static const char footer[] = "</d:multistatus>\n";
  const struct mg_str *depth = mg_get_http_header(hm, "Depth");

  /* Print properties for the requested resource itself */
  if (S_ISDIR(stp->st_mode) &&
      strcmp(opts->enable_directory_listing, "yes") != 0) {
    mg_printf(nc, "%s", "HTTP/1.1 403 Directory Listing Denied\r\n\r\n");
  } else {
    char uri[MG_MAX_PATH];
    mg_send(nc, header, sizeof(header) - 1);
    snprintf(uri, sizeof(uri), "%.*s", (int) hm->uri.len, hm->uri.p);
    mg_print_props(nc, uri, stp);
    if (S_ISDIR(stp->st_mode) && (depth == NULL || mg_vcmp(depth, "0") != 0)) {
      mg_scan_directory(nc, path, opts, mg_print_props);
    }
    mg_send(nc, footer, sizeof(footer) - 1);
    nc->flags |= MG_F_SEND_AND_CLOSE;
  }
}

#if MG_ENABLE_FAKE_DAVLOCK
/*
 * Windows explorer (probably there are another WebDav clients like it)
 * requires LOCK support in webdav. W/out this, it still works, but fails
 * to save file: shows error message and offers "Save As".
 * "Save as" works, but this message is very annoying.
 * This is fake lock, which doesn't lock something, just returns LOCK token,
 * UNLOCK always answers "OK".
 * With this fake LOCK Windows Explorer looks happy and saves file.
 * NOTE: that is not DAV LOCK imlementation, it is just a way to shut up
 * Windows native DAV client. This is why FAKE LOCK is not enabed by default
 */
MG_INTERNAL void mg_handle_lock(struct mg_connection *nc, const char *path) {
  static const char *reply =
      "HTTP/1.1 207 Multi-Status\r\n"
      "Connection: close\r\n"
      "Content-Type: text/xml; charset=utf-8\r\n\r\n"
      "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
      "<d:multistatus xmlns:d='DAV:'>\n"
      "<D:lockdiscovery>\n"
      "<D:activelock>\n"
      "<D:locktoken>\n"
      "<D:href>\n"
      "opaquelocktoken:%s%u"
      "</D:href>"
      "</D:locktoken>"
      "</D:activelock>\n"
      "</D:lockdiscovery>"
      "</d:multistatus>\n";
  mg_printf(nc, reply, path, (unsigned int) mg_time());
  nc->flags |= MG_F_SEND_AND_CLOSE;
}
#endif

MG_INTERNAL void mg_handle_mkcol(struct mg_connection *nc, const char *path,
                                 struct http_message *hm) {
  int status_code = 500;
  if (hm->body.len != (size_t) ~0 && hm->body.len > 0) {
    status_code = 415;
  } else if (!mg_mkdir(path, 0755)) {
    status_code = 201;
  } else if (errno == EEXIST) {
    status_code = 405;
  } else if (errno == EACCES) {
    status_code = 403;
  } else if (errno == ENOENT) {
    status_code = 409;
  } else {
    status_code = 500;
  }
  mg_http_send_error(nc, status_code, NULL);
}

static int mg_remove_directory(const struct mg_serve_http_opts *opts,
                               const char *dir) {
  char path[MG_MAX_PATH];
  struct dirent *dp;
  cs_stat_t st;
  DIR *dirp;

  if ((dirp = opendir(dir)) == NULL) return 0;

  while ((dp = readdir(dirp)) != NULL) {
    if (mg_is_file_hidden((const char *) dp->d_name, opts, 1)) {
      continue;
    }
    snprintf(path, sizeof(path), "%s%c%s", dir, '/', dp->d_name);
    mg_stat(path, &st);
    if (S_ISDIR(st.st_mode)) {
      mg_remove_directory(opts, path);
    } else {
      remove(path);
    }
  }
  closedir(dirp);
  rmdir(dir);

  return 1;
}

MG_INTERNAL void mg_handle_move(struct mg_connection *c,
                                const struct mg_serve_http_opts *opts,
                                const char *path, struct http_message *hm) {
  const struct mg_str *dest = mg_get_http_header(hm, "Destination");
  if (dest == NULL) {
    mg_http_send_error(c, 411, NULL);
  } else {
    const char *p = (char *) memchr(dest->p, '/', dest->len);
    if (p != NULL && p[1] == '/' &&
        (p = (char *) memchr(p + 2, '/', dest->p + dest->len - p)) != NULL) {
      char buf[MG_MAX_PATH];
      snprintf(buf, sizeof(buf), "%s%.*s", opts->dav_document_root,
               (int) (dest->p + dest->len - p), p);
      if (rename(path, buf) == 0) {
        mg_http_send_error(c, 200, NULL);
      } else {
        mg_http_send_error(c, 418, NULL);
      }
    } else {
      mg_http_send_error(c, 500, NULL);
    }
  }
}

MG_INTERNAL void mg_handle_delete(struct mg_connection *nc,
                                  const struct mg_serve_http_opts *opts,
                                  const char *path) {
  cs_stat_t st;
  if (mg_stat(path, &st) != 0) {
    mg_http_send_error(nc, 404, NULL);
  } else if (S_ISDIR(st.st_mode)) {
    mg_remove_directory(opts, path);
    mg_http_send_error(nc, 204, NULL);
  } else if (remove(path) == 0) {
    mg_http_send_error(nc, 204, NULL);
  } else {
    mg_http_send_error(nc, 423, NULL);
  }
}

/* Return -1 on error, 1 on success. */
static int mg_create_itermediate_directories(const char *path) {
  const char *s;

  /* Create intermediate directories if they do not exist */
  for (s = path + 1; *s != '\0'; s++) {
    if (*s == '/') {
      char buf[MG_MAX_PATH];
      cs_stat_t st;
      snprintf(buf, sizeof(buf), "%.*s", (int) (s - path), path);
      buf[sizeof(buf) - 1] = '\0';
      if (mg_stat(buf, &st) != 0 && mg_mkdir(buf, 0755) != 0) {
        return -1;
      }
    }
  }

  return 1;
}

MG_INTERNAL void mg_handle_put(struct mg_connection *nc, const char *path,
                               struct http_message *hm) {
  struct mg_http_proto_data *pd = mg_http_get_proto_data(nc);
  cs_stat_t st;
  const struct mg_str *cl_hdr = mg_get_http_header(hm, "Content-Length");
  int rc, status_code = mg_stat(path, &st) == 0 ? 200 : 201;

  mg_http_free_proto_data_file(&pd->file);
  if ((rc = mg_create_itermediate_directories(path)) == 0) {
    mg_printf(nc, "HTTP/1.1 %d OK\r\nContent-Length: 0\r\n\r\n", status_code);
  } else if (rc == -1) {
    mg_http_send_error(nc, 500, NULL);
  } else if (cl_hdr == NULL) {
    mg_http_send_error(nc, 411, NULL);
  } else if ((pd->file.fp = mg_fopen(path, "w+b")) == NULL) {
    mg_http_send_error(nc, 500, NULL);
  } else {
    const struct mg_str *range_hdr = mg_get_http_header(hm, "Content-Range");
    int64_t r1 = 0, r2 = 0;
    pd->file.type = DATA_PUT;
    mg_set_close_on_exec((sock_t) fileno(pd->file.fp));
    pd->file.cl = to64(cl_hdr->p);
    if (range_hdr != NULL &&
        mg_http_parse_range_header(range_hdr, &r1, &r2) > 0) {
      status_code = 206;
      fseeko(pd->file.fp, r1, SEEK_SET);
      pd->file.cl = r2 > r1 ? r2 - r1 + 1 : pd->file.cl - r1;
    }
    mg_printf(nc, "HTTP/1.1 %d OK\r\nContent-Length: 0\r\n\r\n", status_code);
    /* Remove HTTP request from the mbuf, leave only payload */
    mbuf_remove(&nc->recv_mbuf, hm->message.len - hm->body.len);
    mg_http_transfer_file_data(nc);
  }
}

#endif /* MG_ENABLE_HTTP && MG_ENABLE_HTTP_WEBDAV */
#ifdef MG_MODULE_LINES
#line 1 "mongoose/src/mg_http_websocket.c"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#if MG_ENABLE_HTTP && MG_ENABLE_HTTP_WEBSOCKET

/* Amalgamated: #include "common/cs_sha1.h" */

#ifndef MG_WEBSOCKET_PING_INTERVAL_SECONDS
#define MG_WEBSOCKET_PING_INTERVAL_SECONDS 5
#endif

#define FLAGS_MASK_FIN (1 << 7)
#define FLAGS_MASK_OP 0x0f

static int mg_is_ws_fragment(unsigned char flags) {
  return (flags & FLAGS_MASK_FIN) == 0 ||
         (flags & FLAGS_MASK_OP) == WEBSOCKET_OP_CONTINUE;
}

static int mg_is_ws_first_fragment(unsigned char flags) {
  return (flags & FLAGS_MASK_FIN) == 0 &&
         (flags & FLAGS_MASK_OP) != WEBSOCKET_OP_CONTINUE;
}

static int mg_is_ws_control_frame(unsigned char flags) {
  unsigned char op = (flags & FLAGS_MASK_OP);
  return op == WEBSOCKET_OP_CLOSE || op == WEBSOCKET_OP_PING ||
         op == WEBSOCKET_OP_PONG;
}

static void mg_handle_incoming_websocket_frame(struct mg_connection *nc,
                                               struct websocket_message *wsm) {
  if (wsm->flags & 0x8) {
    mg_call(nc, nc->handler, nc->user_data, MG_EV_WEBSOCKET_CONTROL_FRAME, wsm);
  } else {
    mg_call(nc, nc->handler, nc->user_data, MG_EV_WEBSOCKET_FRAME, wsm);
  }
}

static struct mg_ws_proto_data *mg_ws_get_proto_data(struct mg_connection *nc) {
  struct mg_http_proto_data *htd = mg_http_get_proto_data(nc);
  return (htd != NULL ? &htd->ws_data : NULL);
}

/*
 * Sends a Close websocket frame with the given data, and closes the underlying
 * connection. If `len` is ~0, strlen(data) is used.
 */
static void mg_ws_close(struct mg_connection *nc, const void *data,
                        size_t len) {
  if ((int) len == ~0) {
    len = strlen((const char *) data);
  }
  mg_send_websocket_frame(nc, WEBSOCKET_OP_CLOSE, data, len);
  nc->flags |= MG_F_SEND_AND_CLOSE;
}

static int mg_deliver_websocket_data(struct mg_connection *nc) {
  /* Using unsigned char *, cause of integer arithmetic below */
  uint64_t i, data_len = 0, frame_len = 0, new_data_len = nc->recv_mbuf.len,
              len, mask_len = 0, header_len = 0;
  struct mg_ws_proto_data *wsd = mg_ws_get_proto_data(nc);
  unsigned char *new_data = (unsigned char *) nc->recv_mbuf.buf,
                *e = (unsigned char *) nc->recv_mbuf.buf + nc->recv_mbuf.len;
  uint8_t flags;
  int ok, reass;

  if (wsd->reass_len > 0) {
    /*
     * We already have some previously received data which we need to
     * reassemble and deliver to the client code when we get the final
     * fragment.
     *
     * NOTE: it doesn't mean that the current message must be a continuation:
     * it might be a control frame (Close, Ping or Pong), which should be
     * handled without breaking the fragmented message.
     */

    size_t existing_len = wsd->reass_len;
    assert(new_data_len >= existing_len);

    new_data += existing_len;
    new_data_len -= existing_len;
  }

  flags = new_data[0];

  reass = new_data_len > 0 && mg_is_ws_fragment(flags) &&
          !(nc->flags & MG_F_WEBSOCKET_NO_DEFRAG);

  if (reass && mg_is_ws_control_frame(flags)) {
    /*
     * Control frames can't be fragmented, so if we encounter fragmented
     * control frame, close connection immediately.
     */
    mg_ws_close(nc, "fragmented control frames are illegal", ~0);
    return 0;
  } else if (new_data_len > 0 && !reass && !mg_is_ws_control_frame(flags) &&
             wsd->reass_len > 0) {
    /*
     * When in the middle of a fragmented message, only the continuations
     * and control frames are allowed.
     */
    mg_ws_close(nc, "non-continuation in the middle of a fragmented message",
                ~0);
    return 0;
  }

  if (new_data_len >= 2) {
    len = new_data[1] & 0x7f;
    mask_len = new_data[1] & FLAGS_MASK_FIN ? 4 : 0;
    if (len < 126 && new_data_len >= mask_len) {
      data_len = len;
      header_len = 2 + mask_len;
    } else if (len == 126 && new_data_len >= 4 + mask_len) {
      header_len = 4 + mask_len;
      data_len = ntohs(*(uint16_t *) &new_data[2]);
    } else if (new_data_len >= 10 + mask_len) {
      header_len = 10 + mask_len;
      data_len = (((uint64_t) ntohl(*(uint32_t *) &new_data[2])) << 32) +
                 ntohl(*(uint32_t *) &new_data[6]);
    }
  }

  frame_len = header_len + data_len;
  ok = (frame_len > 0 && frame_len <= new_data_len);

  /* Check for overflow */
  if (frame_len < header_len || frame_len < data_len) {
    ok = 0;
    mg_ws_close(nc, "overflowed message", ~0);
  }

  if (ok) {
    size_t cleanup_len = 0;
    struct websocket_message wsm;

    wsm.size = (size_t) data_len;
    wsm.data = new_data + header_len;
    wsm.flags = flags;

    /* Apply mask if necessary */
    if (mask_len > 0) {
      for (i = 0; i < data_len; i++) {
        new_data[i + header_len] ^= (new_data + header_len - mask_len)[i % 4];
      }
    }

    if (reass) {
      /* This is a message fragment */

      if (mg_is_ws_first_fragment(flags)) {
        /*
         * On the first fragmented frame, skip the first byte (op) and also
         * reset size to 1 (op), it'll be incremented with the data len below.
         */
        new_data += 1;
        wsd->reass_len = 1 /* op */;
      }

      /* Append this frame to the reassembled buffer */
      memmove(new_data, wsm.data, e - wsm.data);
      wsd->reass_len += wsm.size;
      nc->recv_mbuf.len -= wsm.data - new_data;

      if (flags & FLAGS_MASK_FIN) {
        /* On last fragmented frame - call user handler and remove data */
        wsm.flags = FLAGS_MASK_FIN | nc->recv_mbuf.buf[0];
        wsm.data = (unsigned char *) nc->recv_mbuf.buf + 1 /* op */;
        wsm.size = wsd->reass_len - 1 /* op */;
        cleanup_len = wsd->reass_len;
        wsd->reass_len = 0;

        /* Pass reassembled message to the client code. */
        mg_handle_incoming_websocket_frame(nc, &wsm);
        mbuf_remove(&nc->recv_mbuf, cleanup_len); /* Cleanup frame */
      }
    } else {
      /*
       * This is a complete message, not a fragment. It might happen in between
       * of a fragmented message (in this case, WebSocket protocol requires
       * current message to be a control frame).
       */
      cleanup_len = (size_t) frame_len;

      /* First of all, check if we need to react on a control frame. */
      switch (flags & FLAGS_MASK_OP) {
        case WEBSOCKET_OP_PING:
          mg_send_websocket_frame(nc, WEBSOCKET_OP_PONG, wsm.data, wsm.size);
          break;

        case WEBSOCKET_OP_CLOSE:
          mg_ws_close(nc, wsm.data, wsm.size);
          break;
      }

      /* Pass received message to the client code. */
      mg_handle_incoming_websocket_frame(nc, &wsm);

      /* Cleanup frame */
      memmove(nc->recv_mbuf.buf + wsd->reass_len,
              nc->recv_mbuf.buf + wsd->reass_len + cleanup_len,
              nc->recv_mbuf.len - wsd->reass_len - cleanup_len);
      nc->recv_mbuf.len -= cleanup_len;
    }
  }

  return ok;
}

struct ws_mask_ctx {
  size_t pos; /* zero means unmasked */
  uint32_t mask;
};

static uint32_t mg_ws_random_mask(void) {
  uint32_t mask;
/*
 * The spec requires WS client to generate hard to
 * guess mask keys. From RFC6455, Section 5.3:
 *
 * The unpredictability of the masking key is essential to prevent
 * authors of malicious applications from selecting the bytes that appear on
 * the wire.
 *
 * Hence this feature is essential when the actual end user of this API
 * is untrusted code that wouldn't have access to a lower level net API
 * anyway (e.g. web browsers). Hence this feature is low prio for most
 * mongoose use cases and thus can be disabled, e.g. when porting to a platform
 * that lacks rand().
 */
#if MG_DISABLE_WS_RANDOM_MASK
  mask = 0xefbeadde; /* generated with a random number generator, I swear */
#else
  if (sizeof(long) >= 4) {
    mask = (uint32_t) rand();
  } else if (sizeof(long) == 2) {
    mask = (uint32_t) rand() << 16 | (uint32_t) rand();
  }
#endif
  return mask;
}

static void mg_send_ws_header(struct mg_connection *nc, int op, size_t len,
                              struct ws_mask_ctx *ctx) {
  int header_len;
  unsigned char header[10];

  header[0] =
      (op & WEBSOCKET_DONT_FIN ? 0x0 : FLAGS_MASK_FIN) | (op & FLAGS_MASK_OP);
  if (len < 126) {
    header[1] = (unsigned char) len;
    header_len = 2;
  } else if (len < 65535) {
    uint16_t tmp = htons((uint16_t) len);
    header[1] = 126;
    memcpy(&header[2], &tmp, sizeof(tmp));
    header_len = 4;
  } else {
    uint32_t tmp;
    header[1] = 127;
    tmp = htonl((uint32_t)((uint64_t) len >> 32));
    memcpy(&header[2], &tmp, sizeof(tmp));
    tmp = htonl((uint32_t)(len & 0xffffffff));
    memcpy(&header[6], &tmp, sizeof(tmp));
    header_len = 10;
  }

  /* client connections enable masking */
  if (nc->listener == NULL) {
    header[1] |= 1 << 7; /* set masking flag */
    mg_send(nc, header, header_len);
    ctx->mask = mg_ws_random_mask();
    mg_send(nc, &ctx->mask, sizeof(ctx->mask));
    ctx->pos = nc->send_mbuf.len;
  } else {
    mg_send(nc, header, header_len);
    ctx->pos = 0;
  }
}

static void mg_ws_mask_frame(struct mbuf *mbuf, struct ws_mask_ctx *ctx) {
  size_t i;
  if (ctx->pos == 0) return;
  for (i = 0; i < (mbuf->len - ctx->pos); i++) {
    mbuf->buf[ctx->pos + i] ^= ((char *) &ctx->mask)[i % 4];
  }
}

void mg_send_websocket_frame(struct mg_connection *nc, int op, const void *data,
                             size_t len) {
  struct ws_mask_ctx ctx;
  DBG(("%p %d %d", nc, op, (int) len));
  mg_send_ws_header(nc, op, len, &ctx);
  mg_send(nc, data, len);

  mg_ws_mask_frame(&nc->send_mbuf, &ctx);

  if (op == WEBSOCKET_OP_CLOSE) {
    nc->flags |= MG_F_SEND_AND_CLOSE;
  }
}

void mg_send_websocket_framev(struct mg_connection *nc, int op,
                              const struct mg_str *strv, int strvcnt) {
  struct ws_mask_ctx ctx;
  int i;
  int len = 0;
  for (i = 0; i < strvcnt; i++) {
    len += strv[i].len;
  }

  mg_send_ws_header(nc, op, len, &ctx);

  for (i = 0; i < strvcnt; i++) {
    mg_send(nc, strv[i].p, strv[i].len);
  }

  mg_ws_mask_frame(&nc->send_mbuf, &ctx);

  if (op == WEBSOCKET_OP_CLOSE) {
    nc->flags |= MG_F_SEND_AND_CLOSE;
  }
}

void mg_printf_websocket_frame(struct mg_connection *nc, int op,
                               const char *fmt, ...) {
  char mem[MG_VPRINTF_BUFFER_SIZE], *buf = mem;
  va_list ap;
  int len;

  va_start(ap, fmt);
  if ((len = mg_avprintf(&buf, sizeof(mem), fmt, ap)) > 0) {
    mg_send_websocket_frame(nc, op, buf, len);
  }
  va_end(ap);

  if (buf != mem && buf != NULL) {
    MG_FREE(buf);
  }
}

MG_INTERNAL void mg_ws_handler(struct mg_connection *nc, int ev,
                               void *ev_data MG_UD_ARG(void *user_data)) {
  mg_call(nc, nc->handler, nc->user_data, ev, ev_data);

  switch (ev) {
    case MG_EV_RECV:
      do {
      } while (mg_deliver_websocket_data(nc));
      break;
    case MG_EV_POLL:
      /* Ping idle websocket connections */
      {
        time_t now = *(time_t *) ev_data;
        if (nc->flags & MG_F_IS_WEBSOCKET &&
            now > nc->last_io_time + MG_WEBSOCKET_PING_INTERVAL_SECONDS) {
          mg_send_websocket_frame(nc, WEBSOCKET_OP_PING, "", 0);
        }
      }
      break;
    default:
      break;
  }
#if MG_ENABLE_CALLBACK_USERDATA
  (void) user_data;
#endif
}

#ifndef MG_EXT_SHA1
void mg_hash_sha1_v(size_t num_msgs, const uint8_t *msgs[],
                    const size_t *msg_lens, uint8_t *digest) {
  size_t i;
  cs_sha1_ctx sha_ctx;
  cs_sha1_init(&sha_ctx);
  for (i = 0; i < num_msgs; i++) {
    cs_sha1_update(&sha_ctx, msgs[i], msg_lens[i]);
  }
  cs_sha1_final(digest, &sha_ctx);
}
#else
extern void mg_hash_sha1_v(size_t num_msgs, const uint8_t *msgs[],
                           const size_t *msg_lens, uint8_t *digest);
#endif

MG_INTERNAL void mg_ws_handshake(struct mg_connection *nc,
                                 const struct mg_str *key,
                                 struct http_message *hm) {
  static const char *magic = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
  const uint8_t *msgs[2] = {(const uint8_t *) key->p, (const uint8_t *) magic};
  const size_t msg_lens[2] = {key->len, 36};
  unsigned char sha[20];
  char b64_sha[30];
  struct mg_str *s;

  mg_hash_sha1_v(2, msgs, msg_lens, sha);
  mg_base64_encode(sha, sizeof(sha), b64_sha);
  mg_printf(nc, "%s",
            "HTTP/1.1 101 Switching Protocols\r\n"
            "Upgrade: websocket\r\n"
            "Connection: Upgrade\r\n");

  s = mg_get_http_header(hm, "Sec-WebSocket-Protocol");
  if (s != NULL) {
    mg_printf(nc, "Sec-WebSocket-Protocol: %.*s\r\n", (int) s->len, s->p);
  }
  mg_printf(nc, "Sec-WebSocket-Accept: %s%s", b64_sha, "\r\n\r\n");

  DBG(("%p %.*s %s", nc, (int) key->len, key->p, b64_sha));
}

void mg_send_websocket_handshake2(struct mg_connection *nc, const char *path,
                                  const char *host, const char *protocol,
                                  const char *extra_headers) {
  mg_send_websocket_handshake3(nc, path, host, protocol, extra_headers, NULL,
                               NULL);
}

void mg_send_websocket_handshake3(struct mg_connection *nc, const char *path,
                                  const char *host, const char *protocol,
                                  const char *extra_headers, const char *user,
                                  const char *pass) {
  mg_send_websocket_handshake3v(nc, mg_mk_str(path), mg_mk_str(host),
                                mg_mk_str(protocol), mg_mk_str(extra_headers),
                                mg_mk_str(user), mg_mk_str(pass));
}

void mg_send_websocket_handshake3v(struct mg_connection *nc,
                                   const struct mg_str path,
                                   const struct mg_str host,
                                   const struct mg_str protocol,
                                   const struct mg_str extra_headers,
                                   const struct mg_str user,
                                   const struct mg_str pass) {
  struct mbuf auth;
  char key[25];
  uint32_t nonce[4];
  nonce[0] = mg_ws_random_mask();
  nonce[1] = mg_ws_random_mask();
  nonce[2] = mg_ws_random_mask();
  nonce[3] = mg_ws_random_mask();
  mg_base64_encode((unsigned char *) &nonce, sizeof(nonce), key);

  mbuf_init(&auth, 0);
  if (user.len > 0) {
    mg_basic_auth_header(user, pass, &auth);
  }

  /*
   * NOTE: the  (auth.buf == NULL ? "" : auth.buf) is because cc3200 libc is
   * broken: it doesn't like zero length to be passed to %.*s
   * i.e. sprintf("f%.*so", (int)0, NULL), yields `f\0o`.
   * because it handles NULL specially (and incorrectly).
   */
  mg_printf(nc,
            "GET %.*s HTTP/1.1\r\n"
            "Upgrade: websocket\r\n"
            "Connection: Upgrade\r\n"
            "%.*s"
            "Sec-WebSocket-Version: 13\r\n"
            "Sec-WebSocket-Key: %s\r\n",
            (int) path.len, path.p, (int) auth.len,
            (auth.buf == NULL ? "" : auth.buf), key);

  /* TODO(mkm): take default hostname from http proto data if host == NULL */
  if (host.len > 0) {
    int host_len = (int) (path.p - host.p); /* Account for possible :PORT */
    mg_printf(nc, "Host: %.*s\r\n", host_len, host.p);
  }
  if (protocol.len > 0) {
    mg_printf(nc, "Sec-WebSocket-Protocol: %.*s\r\n", (int) protocol.len,
              protocol.p);
  }
  if (extra_headers.len > 0) {
    mg_printf(nc, "%.*s", (int) extra_headers.len, extra_headers.p);
  }
  mg_printf(nc, "\r\n");

  mbuf_free(&auth);
}

void mg_send_websocket_handshake(struct mg_connection *nc, const char *path,
                                 const char *extra_headers) {
  struct mg_str null_str = MG_NULL_STR;
  mg_send_websocket_handshake3v(
      nc, mg_mk_str(path), null_str /* host */, null_str /* protocol */,
      mg_mk_str(extra_headers), null_str /* user */, null_str /* pass */);
}

struct mg_connection *mg_connect_ws_opt(
    struct mg_mgr *mgr, MG_CB(mg_event_handler_t ev_handler, void *user_data),
    struct mg_connect_opts opts, const char *url, const char *protocol,
    const char *extra_headers) {
  struct mg_str null_str = MG_NULL_STR;
  struct mg_str host = MG_NULL_STR, path = MG_NULL_STR, user_info = MG_NULL_STR;
  struct mg_connection *nc =
      mg_connect_http_base(mgr, MG_CB(ev_handler, user_data), opts, "http",
                           "ws", "https", "wss", url, &path, &user_info, &host);
  if (nc != NULL) {
    mg_send_websocket_handshake3v(nc, path, host, mg_mk_str(protocol),
                                  mg_mk_str(extra_headers), user_info,
                                  null_str);
  }
  return nc;
}

struct mg_connection *mg_connect_ws(
    struct mg_mgr *mgr, MG_CB(mg_event_handler_t ev_handler, void *user_data),
    const char *url, const char *protocol, const char *extra_headers) {
  struct mg_connect_opts opts;
  memset(&opts, 0, sizeof(opts));
  return mg_connect_ws_opt(mgr, MG_CB(ev_handler, user_data), opts, url,
                           protocol, extra_headers);
}
#endif /* MG_ENABLE_HTTP && MG_ENABLE_HTTP_WEBSOCKET */
#ifdef MG_MODULE_LINES
#line 1 "mongoose/src/mg_util.c"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

/* Amalgamated: #include "common/cs_base64.h" */
/* Amalgamated: #include "mg_internal.h" */
/* Amalgamated: #include "mg_util.h" */

/* For platforms with limited libc */
#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

const char *mg_skip(const char *s, const char *end, const char *delims,
                    struct mg_str *v) {
  v->p = s;
  while (s < end && strchr(delims, *(unsigned char *) s) == NULL) s++;
  v->len = s - v->p;
  while (s < end && strchr(delims, *(unsigned char *) s) != NULL) s++;
  return s;
}

#if MG_ENABLE_FILESYSTEM && !defined(MG_USER_FILE_FUNCTIONS)
int mg_stat(const char *path, cs_stat_t *st) {
#ifdef _WIN32
  wchar_t wpath[MG_MAX_PATH];
  to_wchar(path, wpath, ARRAY_SIZE(wpath));
  DBG(("[%ls] -> %d", wpath, _wstati64(wpath, st)));
  return _wstati64(wpath, st);
#else
  return stat(path, st);
#endif
}

FILE *mg_fopen(const char *path, const char *mode) {
#ifdef _WIN32
  wchar_t wpath[MG_MAX_PATH], wmode[10];
  to_wchar(path, wpath, ARRAY_SIZE(wpath));
  to_wchar(mode, wmode, ARRAY_SIZE(wmode));
  return _wfopen(wpath, wmode);
#else
  return fopen(path, mode);
#endif
}

int mg_open(const char *path, int flag, int mode) { /* LCOV_EXCL_LINE */
#if defined(_WIN32) && !defined(WINCE)
  wchar_t wpath[MG_MAX_PATH];
  to_wchar(path, wpath, ARRAY_SIZE(wpath));
  return _wopen(wpath, flag, mode);
#else
  return open(path, flag, mode); /* LCOV_EXCL_LINE */
#endif
}

size_t mg_fread(void *ptr, size_t size, size_t count, FILE *f) {
  return fread(ptr, size, count, f);
}

size_t mg_fwrite(const void *ptr, size_t size, size_t count, FILE *f) {
  return fwrite(ptr, size, count, f);
}
#endif

void mg_base64_encode(const unsigned char *src, int src_len, char *dst) {
  cs_base64_encode(src, src_len, dst);
}

int mg_base64_decode(const unsigned char *s, int len, char *dst) {
  return cs_base64_decode(s, len, dst, NULL);
}

#if MG_ENABLE_THREADS
void *mg_start_thread(void *(*f)(void *), void *p) 
{
#ifdef WINCE
    return (void *) CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) f, p, 0, NULL);
#elif defined(_WIN32)
    return (void *) _beginthread((void(__cdecl *) (void *) ) f, 0, p);
#else
    pthread_t thread_id = (pthread_t) 0;
    pthread_attr_t attr;

    (void) pthread_attr_init(&attr);
    (void) pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

#if defined(MG_STACK_SIZE) && MG_STACK_SIZE > 1
    (void) pthread_attr_setstacksize(&attr, MG_STACK_SIZE);
#endif

    pthread_create(&thread_id, &attr, f, p);
    pthread_attr_destroy(&attr);

    return (void *) thread_id;
#endif
}
#endif /* MG_ENABLE_THREADS */


/* Set close-on-exec bit for a given socket. */
void mg_set_close_on_exec(sock_t sock) 
{
#if defined(_WIN32) && !defined(WINCE)
    (void) SetHandleInformation((HANDLE) sock, HANDLE_FLAG_INHERIT, 0);
#elif defined(__unix__)
    fcntl(sock, F_SETFD, FD_CLOEXEC);
#else
    (void) sock;
#endif
}

int mg_sock_addr_to_str(const union socket_address *sa, char *buf, size_t len,
                        int flags) 
{
    int is_v6;
    if (buf == NULL || len <= 0) return 0;
    memset(buf, 0, len);

#if MG_ENABLE_IPV6
    is_v6 = sa->sa.sa_family == AF_INET6;
#else
    is_v6 = 0;
#endif
  
    if (flags & MG_SOCK_STRINGIFY_IP) {
#if MG_ENABLE_IPV6
        const void *addr = NULL;
        char *start = buf;
        socklen_t capacity = len;
        if (!is_v6) {
            addr = &sa->sin.sin_addr;
        } else {
            addr = (void *) &sa->sin6.sin6_addr;
            if (flags & MG_SOCK_STRINGIFY_PORT) {
                *buf = '[';
                start++;
                capacity--;
            }
        }
        if (inet_ntop(sa->sa.sa_family, addr, start, capacity) == NULL) {
            goto cleanup;
        }

#elif defined(_WIN32) || MG_LWIP || (MG_NET_IF == MG_NET_IF_PIC32)
        /* Only Windoze Vista (and newer) have inet_ntop() */
        char *addr_str = inet_ntoa(sa->sin.sin_addr);
        if (addr_str != NULL) {
            strncpy(buf, inet_ntoa(sa->sin.sin_addr), len - 1);
        } else {
            goto cleanup;
        }
#else
        if (inet_ntop(AF_INET, (void *) &sa->sin.sin_addr, buf, len) == NULL) {
            goto cleanup;
        }
#endif
    }
    if (flags & MG_SOCK_STRINGIFY_PORT) {
        int port = ntohs(sa->sin.sin_port);
        if (flags & MG_SOCK_STRINGIFY_IP) {
            int buf_len = strlen(buf);
            snprintf(buf + buf_len, len - (buf_len + 1), "%s:%d", (is_v6 ? "]" : ""),
               port);
        } else {
            snprintf(buf, len, "%d", port);
        }
    }

    return strlen(buf);
cleanup:
    *buf = '\0';
    return 0;
}

int mg_conn_addr_to_str(struct mg_connection *nc, char *buf, size_t len, int flags) 
{
    union socket_address sa;
    me    mset(&sa, 0, sizeof(sa));
    mg_if_get_conn_addr(nc, flags & MG_SOCK_STRINGIFY_REMOTE, &sa);
    return mg_sock_addr_to_str(&sa, buf, len, flags);
}

#if MG_ENABLE_HEXDUMP
static int mg_hexdump_n(const void *buf, 
                        int len, 
                        char *dst, 
                        int dst_len,
                        int offset) 
{
    const unsigned char *p = (const unsigned char *) buf;
    char ascii[17] = "";
    int i, idx, n = 0;

    for (i = 0; i < len; i++) {
        idx = i % 16;
        if (idx == 0) {
            if (i > 0) 
                n += snprintf(dst + n, MAX(dst_len - n, 0), "  %s\n", ascii);
            n += snprintf(dst + n, MAX(dst_len - n, 0), "%04x ", i + offset);
        }
        if (dst_len - n < 0) {
            return n;
        }
        n += snprintf(dst + n, MAX(dst_len - n, 0), " %02x", p[i]);
        ascii[idx] = p[i] < 0x20 || p[i] > 0x7e ? '.' : p[i];
        ascii[idx + 1] = '\0';
    }

    while (i++ % 16) 
        n += snprintf(dst + n, MAX(dst_len - n, 0), "%s", "   ");
    n += snprintf(dst + n, MAX(dst_len - n, 0), "  %s\n", ascii);
    return n;
}

int mg_hexdump(const void *buf, int len, char *dst, int dst_len) 
{
    return mg_hexdump_n(buf, len, dst, dst_len, 0);
}

void mg_hexdumpf(FILE *fp, const void *buf, int len) 
{
    char tmp[80];
    int offset = 0, n;
    while (len > 0) {
        n = (len < 16 ? len : 16);
        mg_hexdump_n(((const char *) buf) + offset, n, tmp, sizeof(tmp), offset);
        fputs(tmp, fp);
        offset += n;
        len -= n;
    }
}

void mg_hexdump_connection(struct mg_connection *nc, 
                            const char *path,
                            const void *buf, 
                            int num_bytes, 
                            int ev) 
{
    FILE *fp = NULL;
    char src[60], dst[60];
    const char *tag = NULL;
        switch (ev) {
        case MG_EV_RECV:
            tag = "<-";
            break;
        
        case MG_EV_SEND:
            tag = "->";
            break;
    
        case MG_EV_ACCEPT:
            tag = "<A";
            break;
        
        case MG_EV_CONNECT:
            tag = "C>";
            break;
    
        case MG_EV_CLOSE:
            tag = "XX";
            break;
        }
    
    if (tag == NULL) return; /* Don't log MG_EV_TIMER, etc */

    if (strcmp(path, "-") == 0) {
        fp = stdout;
    } else if (strcmp(path, "--") == 0) {
        fp = stderr;
#if MG_ENABLE_FILESYSTEM
    } else {
        fp = mg_fopen(path, "a");
#endif
    }
    if (fp == NULL) return;

    mg_conn_addr_to_str(nc, src, sizeof(src),
                      MG_SOCK_STRINGIFY_IP | MG_SOCK_STRINGIFY_PORT);
    mg_conn_addr_to_str(nc, dst, sizeof(dst), MG_SOCK_STRINGIFY_IP |
                                                MG_SOCK_STRINGIFY_PORT |
                                                MG_SOCK_STRINGIFY_REMOTE);
    fprintf(fp, "%lu %p %s %s %s %d\n", (unsigned long) mg_time(), (void *) nc,
          src, tag, dst, (int) num_bytes);
    
    if (num_bytes > 0) {
        mg_hexdumpf(fp, buf, num_bytes);
    }
    if (fp != stdout && fp != stderr) fclose(fp);
}
#endif

int mg_is_big_endian(void) 
{
    static const int n = 1;
    /* TODO(mkm) use compiletime check with 4-byte char literal */
    return ((char *) &n)[0] == 0;
}

DO_NOT_WARN_UNUSED MG_INTERNAL int mg_get_errno(void) 
{
#ifndef WINCE
    return errno;
#else
    /* TODO(alashkin): translate error codes? */
    return GetLastError();
#endif
}

void mg_mbuf_append_base64_putc(char ch, void *user_data) 
{
    struct mbuf *mbuf = (struct mbuf *) user_data;
    mbuf_append(mbuf, &ch, sizeof(ch));
}

void mg_mbuf_append_base64(struct mbuf *mbuf, const void *data, size_t len) 
{
    struct cs_base64_ctx ctx;
    cs_base64_init(&ctx, mg_mbuf_append_base64_putc, mbuf);
    cs_base64_update(&ctx, (const char *) data, len);
    cs_base64_finish(&ctx);
}

void mg_basic_auth_header(const struct mg_str user, 
                            const struct mg_str pass,
                            struct mbuf *buf) 
{
    const char *header_prefix = "Authorization: Basic ";
    const char *header_suffix = "\r\n";

    struct cs_base64_ctx ctx;
    cs_base64_init(&ctx, mg_mbuf_append_base64_putc, buf);

    mbuf_append(buf, header_prefix, strlen(header_prefix));

    cs_base64_update(&ctx, user.p, user.len);
    if (pass.len > 0) {
        cs_base64_update(&ctx, ":", 1);
        cs_base64_update(&ctx, pass.p, pass.len);
    }
    cs_base64_finish(&ctx);
    mbuf_append(buf, header_suffix, strlen(header_suffix));
}

struct mg_str mg_url_encode_opt(const struct mg_str src,
                                const struct mg_str safe, 
                                unsigned int flags) 
{
    const char *hex =
        (flags & MG_URL_ENCODE_F_UPPERCASE_HEX ? "0123456789ABCDEF"
                                                : "0123456789abcdef");
    size_t i = 0;
    struct mbuf mb;
    mbuf_init(&mb, src.len);

    for (i = 0; i < src.len; i++) {
        const unsigned char c = *((const unsigned char *) src.p + i);
        if (isalnum(c) || mg_strchr(safe, c) != NULL) {
            mbuf_append(&mb, &c, 1);
        } else if (c == ' ' && (flags & MG_URL_ENCODE_F_SPACE_AS_PLUS)) {
            mbuf_append(&mb, "+", 1);
        } else {
            mbuf_append(&mb, "%", 1);
            mbuf_append(&mb, &hex[c >> 4], 1);
            mbuf_append(&mb, &hex[c & 15], 1);
        }
    }

    mbuf_append(&mb, "", 1);
    mbuf_trim(&mb);
    return mg_mk_str_n(mb.buf, mb.len - 1);
}

struct mg_str mg_url_encode(const struct mg_str src) 
{
    return mg_url_encode_opt(src, mg_mk_str("._-$,;~()/"), 0);
}


#if MG_ENABLE_SOCKS

/* Amalgamated: #include "mg_socks.h" */
/* Amalgamated: #include "mg_internal.h" */

/*
 *  https://www.ietf.org/rfc/rfc1928.txt paragraph 3, handle client handshake
 *
 *  +----+----------+----------+
 *  |VER | NMETHODS | METHODS  |
 *  +----+----------+----------+
 *  | 1  |    1     | 1 to 255 |
 *  +----+----------+----------+
 */
static void mg_socks5_handshake(struct mg_connection *c) 
{
    struct mbuf *r = &c->recv_mbuf;
    if (r->buf[0] != MG_SOCKS_VERSION) {
        c->flags |= MG_F_CLOSE_IMMEDIATELY;
    } else if (r->len > 2 && (size_t) r->buf[1] + 2 <= r->len) {
        /* https://www.ietf.org/rfc/rfc1928.txt paragraph 3 */
        unsigned char reply[2] = {MG_SOCKS_VERSION, MG_SOCKS_HANDSHAKE_FAILURE};
        int i;
        for (i = 2; i < r->buf[1] + 2; i++) {
            /* TODO(lsm): support other auth methods */
            if (r->buf[i] == MG_SOCKS_HANDSHAKE_NOAUTH) reply[1] = r->buf[i];
        }
        mbuf_remove(r, 2 + r->buf[1]);
        mg_send(c, reply, sizeof(reply));
        c->flags |= MG_SOCKS_HANDSHAKE_DONE; /* Mark handshake done */
    }
}

static void disband(struct mg_connection *c) 
{
    struct mg_connection *c2 = (struct mg_connection *) c->user_data;
    if (c2 != NULL) {
        c2->flags |= MG_F_SEND_AND_CLOSE;
        c2->user_data = NULL;
    }
    c->flags |= MG_F_SEND_AND_CLOSE;
    c->user_data = NULL;
}

static void relay_data(struct mg_connection *c) 
{
    struct mg_connection *c2 = (struct mg_connection *) c->user_data;
    if (c2 != NULL) {
        mg_send(c2, c->recv_mbuf.buf, c->recv_mbuf.len);
        mbuf_remove(&c->recv_mbuf, c->recv_mbuf.len);
    } else {
        c->flags |= MG_F_SEND_AND_CLOSE;
    }
}

static void serv_ev_handler(struct mg_connection *c, int ev, void *ev_data) 
{
    if (ev == MG_EV_CLOSE) {
        disband(c);
    } else if (ev == MG_EV_RECV) {
        relay_data(c);
    } else if (ev == MG_EV_CONNECT) {
        int res = *(int *) ev_data;
        if (res != 0) LOG(LL_ERROR, ("connect error: %d", res));
    }
}

static void mg_socks5_connect(struct mg_connection *c, const char *addr) 
{
    struct mg_connection *serv = mg_connect(c->mgr, addr, serv_ev_handler);
    serv->user_data = c;
    c->user_data = serv;
}

/*
 *  Request, https://www.ietf.org/rfc/rfc1928.txt paragraph 4
 *
 *  +----+-----+-------+------+----------+----------+
 *  |VER | CMD |  RSV  | ATYP | DST.ADDR | DST.PORT |
 *  +----+-----+-------+------+----------+----------+
 *  | 1  |  1  | X'00' |  1   | Variable |    2     |
 *  +----+-----+-------+------+----------+----------+
 */
static void mg_socks5_handle_request(struct mg_connection *c) 
{
    struct mbuf *r = &c->recv_mbuf;
    unsigned char *p = (unsigned char *) r->buf;
    unsigned char addr_len = 4, reply = MG_SOCKS_SUCCESS;
    int ver, cmd, atyp;
    char addr[300];

    if (r->len < 8) return; /* return if not fully buffered. min DST.ADDR is 2 */
    ver = p[0];
    cmd = p[1];
    atyp = p[3];

    /* TODO(lsm): support other commands */
    if (ver != MG_SOCKS_VERSION || cmd != MG_SOCKS_CMD_CONNECT) {
        reply = MG_SOCKS_CMD_NOT_SUPPORTED;
    } else if (atyp == MG_SOCKS_ADDR_IPV4) {
        addr_len = 4;
        if (r->len < (size_t) addr_len + 6) return; /* return if not buffered */
        snprintf(addr, sizeof(addr), "%d.%d.%d.%d:%d", p[4], p[5], p[6], p[7], p[8] << 8 | p[9]);
        mg_socks5_connect(c, addr);
    } else if (atyp == MG_SOCKS_ADDR_IPV6) {
        addr_len = 16;
        if (r->len < (size_t) addr_len + 6) return; /* return if not buffered */
        snprintf(addr, sizeof(addr), "[%x:%x:%x:%x:%x:%x:%x:%x]:%d",
             p[4] << 8 | p[5], p[6] << 8 | p[7], p[8] << 8 | p[9],
             p[10] << 8 | p[11], p[12] << 8 | p[13], p[14] << 8 | p[15],
             p[16] << 8 | p[17], p[18] << 8 | p[19], p[20] << 8 | p[21]);
        mg_socks5_connect(c, addr);
    } else if (atyp == MG_SOCKS_ADDR_DOMAIN) {
        addr_len = p[4] + 1;
        if (r->len < (size_t) addr_len + 6) return; /* return if not buffered */
        snprintf(addr, sizeof(addr), "%.*s:%d", p[4], p + 5,
             p[4 + addr_len] << 8 | p[4 + addr_len + 1]);
        mg_socks5_connect(c, addr);
    } else {
        reply = MG_SOCKS_ADDR_NOT_SUPPORTED;
    }

    /*
    *  Reply, https://www.ietf.org/rfc/rfc1928.txt paragraph 5
    *
    *  +----+-----+-------+------+----------+----------+
    *  |VER | REP |  RSV  | ATYP | BND.ADDR | BND.PORT |
    *  +----+-----+-------+------+----------+----------+
    *  | 1  |  1  | X'00' |  1   | Variable |    2     |
    *  +----+-----+-------+------+----------+----------+
    */
    {
        unsigned char buf[] = {MG_SOCKS_VERSION, reply, 0};
        mg_send(c, buf, sizeof(buf));
    }
    mg_send(c, r->buf + 3, addr_len + 1 + 2);

    mbuf_remove(r, 6 + addr_len);      /* Remove request from the input stream */
    c->flags |= MG_SOCKS_CONNECT_DONE; /* Mark ourselves as connected */
}

static void socks_handler(struct mg_connection *c, int ev, void *ev_data) 
{
    if (ev == MG_EV_RECV) {
        if (!(c->flags & MG_SOCKS_HANDSHAKE_DONE)) mg_socks5_handshake(c);
        if (c->flags & MG_SOCKS_HANDSHAKE_DONE && !(c->flags & MG_SOCKS_CONNECT_DONE)) {
            mg_socks5_handle_request(c);
        }
        if (c->flags & MG_SOCKS_CONNECT_DONE) relay_data(c);
    } else if (ev == MG_EV_CLOSE) {
        disband(c);
    }
    (void) ev_data;
}

void mg_set_protocol_socks(struct mg_connection *c) 
{
    c->proto_handler = socks_handler;
}
#endif



