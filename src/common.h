/**
 * @file common.h
 * @author Radek Krejci <rkrejci@cesnet.cz>
 * @brief common internal definitions for libyang
 *
 * Copyright (c) 2015 CESNET, z.s.p.o.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of the Company nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 */

#ifndef LY_COMMON_H_
#define LY_COMMON_H_

#include <stdint.h>

#include "dict_private.h"
#include "libyang.h"
#include "resolve.h"

#ifdef __GNUC__
#  define UNUSED(x) UNUSED_ ## x __attribute__((__unused__))
#else
#  define UNUSED(x) UNUSED_ ## x
#endif

#ifndef __WORDSIZE
#  if defined __x86_64__ && !defined __ILP32__
#   define __WORDSIZE 64
#  else
#   define __WORDSIZE 32
#  endif
#endif

#ifndef __INT64_C
#  if __WORDSIZE == 64
#    define __INT64_C(c)  c ## L
#    define __UINT64_C(c) c ## UL
#  else
#    define __INT64_C(c)  c ## LL
#    define __UINT64_C(c) c ## ULL
#  endif
#endif

/*
 * If the compiler supports attribute to mark objects as hidden, mark all
 * objects as hidden and export only objects explicitly marked to be part of
 * the public API.
 */
#define API __attribute__((visibility("default")))

/*
 * logger
 */
extern volatile uint8_t ly_log_level;
void ly_log(LY_LOG_LEVEL level, const char *format, ...);

#define LOGERR(errno, str, args...)                                 \
	ly_errno = errno;                                               \
	ly_log(LY_LLERR, str, ##args)

#define LOGWRN(str, args...)                                        \
	if (ly_log_level >= LY_LLWRN) {                                 \
		ly_log(LY_LLWRN, str, ##args);                              \
	}

#define LOGVRB(str, args...)                                        \
	if (ly_log_level >= LY_LLVRB) {                                 \
		ly_log(LY_LLVRB, str, ##args);                              \
	}

#ifdef NDEBUG
#define LOGDBG(str, args...)
#else
#define LOGDBG(str, args...)                                        \
	if (ly_log_level >= LY_LLDBG) {                                 \
		ly_log(LY_LLDBG, str, ##args);                              \
	}
#endif

#define LOGMEM LOGERR(LY_EMEM, "Memory allocation failed (%s()).", __func__)

#define LOGINT LOGERR(LY_EINT, "Internal error (%s:%d).", __FILE__, __LINE__)

enum LY_ERR {
    LYE_SPEC = -2,
    LYE_LINE = -1,

    LYE_XML_MISS,
    LYE_XML_INVAL,
    LYE_XML_INCHAR,

    LYE_EOF,
    LYE_INSTMT,
    LYE_INID,
    LYE_INDATE,
    LYE_INARG,
    LYE_MISSSTMT1,
    LYE_MISSSTMT2,
    LYE_MISSARG,
    LYE_TOOMANY,
    LYE_DUPID,
    LYE_DUPLEAFLIST,
    LYE_DUPLIST,
    LYE_ENUM_DUPVAL,
    LYE_ENUM_DUPNAME,
    LYE_ENUM_WS,
    LYE_BITS_DUPVAL,
    LYE_BITS_DUPNAME,
    LYE_INMOD,
    LYE_INMOD_LEN,
    LYE_KEY_NLEAF,
    LYE_KEY_TYPE,
    LYE_KEY_CONFIG,
    LYE_KEY_MISS,
    LYE_KEY_DUP,
    LYE_INREGEX,
    LYE_INRESOLV,

    LYE_NORESOLV,
    LYE_INELEM,
    LYE_INELEM_LEN,
    LYE_MISSELEM,
    LYE_INVAL,
    LYE_INATTR,
    LYE_MISSATTR,
    LYE_OORVAL,
    LYE_INCHAR,
    LYE_INPRED,
    LYE_MCASEDATA,
    LYE_NOCOND,

    LYE_XPATH_INTOK,
    LYE_XPATH_EOF,
    LYE_XPATH_INOP_1,
    LYE_XPATH_INOP_2,
    LYE_XPATH_INCTX,
    LYE_XPATH_INARGCOUNT,
    LYE_XPATH_INARGTYPE
};
void ly_vlog(enum LY_ERR code, unsigned int line, ...);

#define LOGVAL(code, line, args...) ly_vlog(code, line, ##args)

#ifdef NDEBUG
#    define LOGLINE(node) 0
#    define LOGLINE_IDX(node, idx) 0
#else
#    define LOGLINE(node) (node)->line
#    define LOGLINE_IDX(node, idx) (node)->line[idx]
#endif

/**
 * @brief Basic functionality like strpbrk(3). However, it searches string \p s
 *        backwards up to most \p s_len characters.
 *
 * @param[in] s String to search backwards.
 * @param[in] accept String of characters that are searched for.
 * @param[in] s_len Backward length of \p s.
 *
 * @return Pointer to the first backward occurence of a character from
 *         \p accept or \p s - \p s_len if not found.
 */
const char *strpbrk_backwards(const char *s, const char *accept, unsigned int s_len);

char *strnchr(const char *s, int c, unsigned int len);

const char *strnodetype(LYS_NODE type);

/**
 * @brief Transform expression from JSON format to XML format.
 * Output arrays point to strings in the dictionary, but without
 * correcting their ref_count -> do not touch them. Prefixes of
 * the namespaces are prefixes specified by the module itself. Output
 * parameters are optional, but either all 3 are set or none
 * of them are. Logs directly.
 *
 * @param[in] module Module with imports to use.
 * @param[in] expr JSON expression.
 * @param[out] prefixes Array of pointers to prefixes. After use free them with free(*prefixes).
 * Can be NULL.
 * @param[out] namespaces Array of pointers to full namespaces. After use free them with
 * free(*namespaces). Can be NULL.
 * @param[out] ns_count Number of elements in both \p prefixes and \p namespaces arrays.
 * Can be NULL.
 *
 * @return Transformed XML expression in the dictionary, NULL on error.
 */
const char *transform_json2xml(const struct lys_module *module, const char *expr, const char ***prefixes, const char ***namespaces,
                               uint32_t *ns_count);

/**
 * @brief Transform expression from XML data format (prefixes and separate NS definitions) to
 *        JSON format (prefixes are module names instead). Logs directly.
 *
 * @param[in] ctx libyang context to use.
 * @param[in] expr XML expression.
 * @param[in] xml XML element with the expression.
 * @param[in] log Whether to log errors or not.
 *
 * @return Transformed JSON expression in the dictionary, NULL on error.
 */
const char *transform_xml2json(struct ly_ctx *ctx, const char *expr, struct lyxml_elem *xml, int log);

/**
 * @brief Transform expression from the schema format (prefixes of imports) to
 *        JSON format (prefixes are module names directly). Logs directly.
 *
 * @param[in] module Module (schema) with imports to search.
 * @param[in] expr Expression from \p module.
 * @param[in] line Line in the input file.
 *
 * @return Transformed JSON expression in the dictionary, NULL on error.
 */
const char *transform_schema2json(const struct lys_module *module, const char *expr, uint32_t line);

#endif /* LY_COMMON_H_ */
