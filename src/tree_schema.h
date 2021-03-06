/**
 * @file tree_schema.h
 * @author Radek Krejci <rkrejci@cesnet.cz>
 * @brief libyang representation of data model trees.
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

#ifndef LY_TREE_SCHEMA_H_
#define LY_TREE_SCHEMA_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ingroup datatree
 * @brief Macro to iterate via all sibling elements without affecting the list itself
 *
 * Works for all types of nodes despite it is data or schema tree, but all the
 * parameters must be pointers to the same type.
 *
 * Use with opening curly bracket '{'. All parameters must be of the same type.
 *
 * @param START Pointer to the starting element.
 * @param ELEM Iterator.
 */
#define LY_TREE_FOR(START, ELEM) \
    for ((ELEM) = (START); \
         (ELEM); \
         (ELEM) = (ELEM)->next)

/**
 * @ingroup datatree
 * @brief Macro to iterate via all sibling elements allowing to modify the list itself (e.g. removing elements)
 *
 * Works for all types of nodes despite it is data or schema tree, but all the
 * parameters must be pointers to the same type.
 *
 * Use with opening curly bracket '{'. All parameters must be of the same type.
 *
 * @param START Pointer to the starting element.
 * @param NEXT Temporary storage to allow removing of the current iterator content.
 * @param ELEM Iterator.
 */
#define LY_TREE_FOR_SAFE(START, NEXT, ELEM) \
    for ((ELEM) = (START); \
         (ELEM) ? (NEXT = (ELEM)->next, 1) : 0; \
         (ELEM) = (NEXT))

/**
 * @ingroup datatree
 * @brief Macro to iterate via all elements in a tree. This is the opening part
 * to the #LY_TREE_DFS_END - they always have to be used together.
 *
 * The function follows deep-first search algorithm:
 * <pre>
 *     1
 *    / \
 *   2   4
 *  /   / \
 * 3   5   6
 * </pre>
 *
 * Works for all types of nodes despite it is data or schema tree, but all the
 * parameters must be pointers to the same type. Use the same parameters for
 * #LY_TREE_DFS_BEGIN and #LY_TREE_DFS_END.
 *
 * Since the next node is selected as part of #LY_TREE_DFS_END, do not use
 * continue statement between the #LY_TREE_DFS_BEGIN and #LY_TREE_DFS_BEGIN.
 *
 * Use with opening curly bracket '{' after the macro.
 *
 * @param START Pointer to the starting element processed first.
 * @param NEXT Temporary storage, do not use.
 * @param ELEM Iterator intended for use in the block.
 */
#define LY_TREE_DFS_BEGIN(START, NEXT, ELEM)                                  \
    for ((ELEM) = (NEXT) = (START);                                           \
         (ELEM);                                                              \
         (ELEM) = (NEXT))

/**
 * @ingroup datatree
 * @brief Macro to iterate via all elements in a tree. This is the closing part
 * to the #LY_TREE_DFS_BEGIN - they always have to be used together.
 *
 * Works for all types of nodes despite it is data or schema tree, but all the
 * parameters must be pointers to the same type. Use the same parameters for
 * #LY_TREE_DFS_BEGIN and #LY_TREE_DFS_END.
 *
 * Use with closing curly bracket '}' after the macro.
 *
 * @param START Pointer to the starting element processed first.
 * @param NEXT Temporary storage, do not use.
 * @param ELEM Iterator intended for use in the block.
 */
#define LY_TREE_DFS_END(START, NEXT, ELEM)                                    \
    /* select element for the next run - children first */                    \
    (NEXT) = (ELEM)->child;                                                   \
    if (sizeof(typeof(*(START))) == sizeof(struct lyd_node)) {                \
        /* child exception for lyd_node_leaf and lyd_node_leaflist */         \
        if (((struct lyd_node *)(ELEM))->schema->nodetype & (LYS_LEAF | LYS_LEAFLIST | LYS_ANYXML)) { \
            (NEXT) = NULL;                                                    \
        }                                                                     \
    }                                                                         \
    if (!(NEXT)) {                                                            \
        /* no children, so try siblings */                                    \
        (NEXT) = (ELEM)->next;                                                \
    }                                                                         \
    while (!(NEXT)) {                                                         \
        /* no siblings, go back through parents */                            \
        if (sizeof(typeof(*(START))) == sizeof(struct lys_node)) {            \
            /* lys_node_augment only */                                       \
            if ((ELEM)->parent && (((struct lys_node *)(ELEM)->parent)->nodetype == LYS_AUGMENT)) { \
                if ((START)->parent && (((struct lys_node *)(START)->parent)->nodetype == LYS_AUGMENT)) { \
                    /* lys_node prev is actually lys_node_augment target */   \
                    if ((ELEM)->parent->prev == (START)->parent->prev) {      \
                        break;                                                \
                    }                                                         \
                } else {                                                      \
                    if ((ELEM)->parent->prev == (START)->parent) {            \
                        break;                                                \
                    }                                                         \
                }                                                             \
            } else {                                                          \
                if ((START)->parent && (((struct lys_node *)(START)->parent)->nodetype == LYS_AUGMENT)) { \
                    if ((ELEM)->parent == (START)->parent->prev) {            \
                        break;                                                \
                    }                                                         \
                } /* else covered just below */                               \
            }                                                                 \
        }                                                                     \
        if ((ELEM)->parent == (START)->parent) {                              \
            /* we are done, no next element to process */                     \
            break;                                                            \
        }                                                                     \
        /* parent is already processed, go to its sibling */                  \
        if ((sizeof(typeof(*(START))) == sizeof(struct lys_node))             \
                && (((struct lys_node *)(ELEM)->parent)->nodetype == LYS_AUGMENT)) {  \
            (ELEM) = (ELEM)->parent->prev;                                    \
        } else {                                                              \
            (ELEM) = (ELEM)->parent;                                          \
        }                                                                     \
        (NEXT) = (ELEM)->next;                                                \
    }

/**
 * @addtogroup schematree
 * @{
 */

#define LY_REV_SIZE 11   /**< revision data string length (including terminating NULL byte) */

/**
 * @brief Schema input formats accepted by libyang [parser functions](@ref parsers).
 */
typedef enum {
    LYS_IN_UNKNOWN = 0,  /**< unknown format, used as return value in case of error */
    LYS_IN_YANG = 1,     /**< YANG schema input format, TODO not yet supported */
    LYS_IN_YIN = 2       /**< YIN schema input format */
} LYS_INFORMAT;

/**
 * @brief Schema output formats accepted by libyang [printer functions](@ref printers).
 */
typedef enum {
    LYS_OUT_UNKNOWN = 0, /**< unknown format, used as return value in case of error */
    LYS_OUT_YANG = 1,    /**< YANG schema output format */
    LYS_OUT_YIN = 2,     /**< YIN schema output format, TODO not yet supported */
    LYS_OUT_TREE,        /**< Tree schema output format, for more information see the [printers](@ref printers) page */
    LYS_OUT_INFO,        /**< Info schema output format, for more information see the [printers](@ref printers) page */
} LYS_OUTFORMAT;

/* shortcuts for common in and out formats */
#define LYS_YANG 1       /**< YANG schema format, used for #LYS_INFORMAT and #LYS_OUTFORMAT */
#define LY_YIN 2         /**< YIN schema format, used for #LYS_INFORMAT and #LYS_OUTFORMAT */

/**
 * @brief YANG schema node types
 *
 * Values are defined as separated bit values to allow checking using bitwise operations for multiple nodes.
 */
typedef enum lys_nodetype {
    LYS_UNKNOWN = 0x0000,        /**< uninitalized unknown statement node */
    LYS_AUGMENT = 0x0001,        /**< augment statement node */
    LYS_CONTAINER = 0x0002,      /**< container statement node */
    LYS_CHOICE = 0x0004,         /**< choice statement node */
    LYS_LEAF = 0x0008,           /**< leaf statement node */
    LYS_LEAFLIST = 0x0010,       /**< leaf-list statement node */
    LYS_LIST = 0x0020,           /**< list statement node */
    LYS_ANYXML = 0x0040,         /**< anyxml statement node */
    LYS_GROUPING = 0x0080,       /**< grouping statement node */
    LYS_CASE = 0x0100,           /**< case statement node */
    LYS_INPUT = 0x0200,          /**< input statement node */
    LYS_OUTPUT = 0x0400,         /**< output statement node */
    LYS_NOTIF = 0x0800,          /**< notification statement node */
    LYS_RPC = 0x1000,            /**< rpc statement node */
    LYS_USES = 0x2000            /**< uses statement node */
} LYS_NODE;

#define LYS_ANY 0x2FFF

/**
 * @brief Main schema node structure representing YANG module.
 *
 * Compatible with ::lys_submodule structure with exception of the last, #ns member, which is replaced by
 * ::lys_submodule#belongsto member. Sometimes, ::lys_submodule can be provided casted to ::lys_module. Such a thing
 * can be determined via the #type member value.
 *
 *
 */
struct lys_module {
    struct ly_ctx *ctx;              /**< libyang context of the module (mandatory) */
    const char *name;                /**< name of the module (mandatory) */
    const char *prefix;              /**< prefix of the module (mandatory) */
    const char *dsc;                 /**< description of the module */
    const char *ref;                 /**< cross-reference for the module */
    const char *org;                 /**< party/company responsible for the module */
    const char *contact;             /**< contact information for the module */
    uint8_t version:5;               /**< yang-version:
                                          - 0 = not specified, YANG 1.0 as default,
                                          - 1 = YANG 1.0,
                                          - 2 = YANG 1.1 not yet supported */
    uint8_t type:1;                  /**< 0 - structure type used to distinguish structure from ::lys_submodule */
    uint8_t deviated:1;              /**< deviated flag (true/false) if the module is deviated by some other module */
    uint8_t implemented:1;           /**< flag if the module is implemented, not just imported */
    const char *uri;                 /**< source of this module in URI format (can be NULL) */

    /* array sizes */
    uint8_t rev_size;                /**< number of elements in #rev array */
    uint8_t imp_size;                /**< number of elements in #imp array */
    uint8_t inc_size;                /**< number of elements in #inc array */
    uint8_t tpdf_size;               /**< number of elements in #tpdf array */
    uint32_t ident_size;             /**< number of elements in #ident array */
    uint8_t features_size;           /**< number of elements in #features array */
    uint8_t augment_size;            /**< number of elements in #augment array */
    uint8_t deviation_size;          /**< number of elements in #deviation array */

    struct lys_revision *rev;        /**< array of the module revisions, revisions[0] is always the last (newest)
                                          revision of the module */
    struct lys_import *imp;          /**< array of imported modules */
    struct lys_include *inc;         /**< array of included submodules */
    struct lys_tpdf *tpdf;           /**< array of typedefs */
    struct lys_ident *ident;         /**< array of identities */
    struct lys_feature *features;    /**< array of feature definitions */
    struct lys_node_augment *augment;/**< array of augments */
    struct lys_deviation *deviation; /**< array of specified deviations */

    struct lys_node *data;           /**< first data statement, includes also RPCs and Notifications */

    /* specific module's items in comparison to submodules */
    const char *ns;                  /**< namespace of the module (mandatory) */
};

/**
 * @brief Submodule schema node structure that can be included into a YANG module.
 *
 * Compatible with ::lys_module structure with exception of the last, #belongsto member, which is replaced by
 * ::lys_module#ns member. Sometimes, ::lys_submodule can be provided casted to ::lys_module. Such a thing can
 * be determined via the #type member value.
 *
 *
 */
struct lys_submodule {
    struct ly_ctx *ctx;              /**< libyang context of the submodule (mandatory) */
    const char *name;                /**< name of the submodule (mandatory) */
    const char *prefix;              /**< prefix of the belongs-to module */
    const char *dsc;                 /**< description of the submodule */
    const char *ref;                 /**< cross-reference for the submodule */
    const char *org;                 /**< party responsible for the submodule */
    const char *contact;             /**< contact information for the submodule */
    uint8_t version:5;               /**< yang-version:
                                          - 0 = not specified, YANG 1.0 as default,
                                          - 1 = YANG 1.0,
                                          - 2 = YANG 1.1 not yet supported */
    uint8_t type:1;                  /**< 1 - structure type used to distinguish structure from ::lys_module */
    uint8_t deviated:1;              /**< deviated flag (true/false) if the module is deviated by some other module */
    uint8_t implemented:1;           /**< flag if the module is implemented, not just imported */
    const char *uri;                 /**< origin URI of the submodule */

    /* array sizes */
    uint8_t rev_size;                /**< number of elements in #rev array */
    uint8_t imp_size;                /**< number of elements in #imp array */
    uint8_t inc_size;                /**< number of elements in #inc array */
    uint8_t tpdf_size;               /**< number of elements in #tpdf array */
    uint32_t ident_size;             /**< number of elements in #ident array */
    uint8_t features_size;           /**< number of elements in #features array */
    uint8_t augment_size;            /**< number of elements in #augment array */
    uint8_t deviation_size;          /**< number of elements in #deviation array */

    struct lys_revision *rev;        /**< array of the module revisions, revisions[0] is always the last (newest)
                                          revision of the submodule */
    struct lys_import *imp;          /**< array of imported modules */
    struct lys_include *inc;         /**< array of included submodules */
    struct lys_tpdf *tpdf;           /**< array of typedefs */
    struct lys_ident *ident;         /**< array if identities */
    struct lys_feature *features;    /**< array of feature definitions */
    struct lys_node_augment *augment;/**< array of augments */
    struct lys_deviation *deviation; /**< array of specified deviations */

    struct lys_node *data;           /**< first data statement, includes also RPCs and Notifications */

    /* specific submodule's items in comparison to modules */
    struct lys_module *belongsto;    /**< belongs-to (parent module) */
};

/**
 * @brief YANG built-in types
 */
typedef enum {
    LY_TYPE_DER = 0,     /**< Derived type */
    LY_TYPE_BINARY,      /**< Any binary data ([RFC 6020 sec 9.8](http://tools.ietf.org/html/rfc6020#section-9.8)) */
    LY_TYPE_BITS,        /**< A set of bits or flags ([RFC 6020 sec 9.7](http://tools.ietf.org/html/rfc6020#section-9.7)) */
    LY_TYPE_BOOL,        /**< "true" or "false" ([RFC 6020 sec 9.5](http://tools.ietf.org/html/rfc6020#section-9.5)) */
    LY_TYPE_DEC64,       /**< 64-bit signed decimal number ([RFC 6020 sec 9.3](http://tools.ietf.org/html/rfc6020#section-9.3))*/
    LY_TYPE_EMPTY,       /**< A leaf that does not have any value ([RFC 6020 sec 9.11](http://tools.ietf.org/html/rfc6020#section-9.11)) */
    LY_TYPE_ENUM,        /**< Enumerated strings ([RFC 6020 sec 9.6](http://tools.ietf.org/html/rfc6020#section-9.6)) */
    LY_TYPE_IDENT,       /**< A reference to an abstract identity ([RFC 6020 sec 9.10](http://tools.ietf.org/html/rfc6020#section-9.10)) */
    LY_TYPE_INST,        /**< References a data tree node ([RFC 6020 sec 9.13](http://tools.ietf.org/html/rfc6020#section-9.13)) */
    LY_TYPE_LEAFREF,     /**< A reference to a leaf instance ([RFC 6020 sec 9.9](http://tools.ietf.org/html/rfc6020#section-9.9))*/
    LY_TYPE_STRING,      /**< Human-readable string ([RFC 6020 sec 9.4](http://tools.ietf.org/html/rfc6020#section-9.4)) */
    LY_TYPE_UNION,       /**< Choice of member types ([RFC 6020 sec 9.12](http://tools.ietf.org/html/rfc6020#section-9.12)) */
    LY_TYPE_INT8,        /**< 8-bit signed integer ([RFC 6020 sec 9.2](http://tools.ietf.org/html/rfc6020#section-9.2)) */
    LY_TYPE_UINT8,       /**< 8-bit unsigned integer ([RFC 6020 sec 9.2](http://tools.ietf.org/html/rfc6020#section-9.2)) */
    LY_TYPE_INT16,       /**< 16-bit signed integer ([RFC 6020 sec 9.2](http://tools.ietf.org/html/rfc6020#section-9.2)) */
    LY_TYPE_UINT16,      /**< 16-bit unsigned integer ([RFC 6020 sec 9.2](http://tools.ietf.org/html/rfc6020#section-9.2)) */
    LY_TYPE_INT32,       /**< 32-bit signed integer ([RFC 6020 sec 9.2](http://tools.ietf.org/html/rfc6020#section-9.2)) */
    LY_TYPE_UINT32,      /**< 32-bit unsigned integer ([RFC 6020 sec 9.2](http://tools.ietf.org/html/rfc6020#section-9.2)) */
    LY_TYPE_INT64,       /**< 64-bit signed integer ([RFC 6020 sec 9.2](http://tools.ietf.org/html/rfc6020#section-9.2)) */
    LY_TYPE_UINT64,      /**< 64-bit unsigned integer ([RFC 6020 sec 9.2](http://tools.ietf.org/html/rfc6020#section-9.2)) */
} LY_DATA_TYPE;
#define LY_DATA_TYPE_COUNT 20        /**< number of #LY_DATA_TYPE built-in types */
#define LY_DATA_TYPE_MASK 0x3f       /**< mask for valid type values, 2 bits are reserver for #LY_TYPE_LEAFREF_UNRES and
                                          #LY_TYPE_INST_UNRES in case of parsing with #LYD_OPT_FILTER or #LYD_OPT_EDIT
                                          options. */
#define LY_TYPE_LEAFREF_UNRES 0x40   /**< flag for unresolved leafref, the rest of bits store the target node's type */
#define LY_TYPE_INST_UNRES 0x80      /**< flag for unresolved instance-identifier, always use in conjunction with LY_TYPE_INST */

/**
 * @brief YANG type structure providing information from the schema
 */
struct lys_type {
    const char *module_name;         /**< module name of the type referenced in der pointer*/
    LY_DATA_TYPE base;               /**< base type */
    struct lys_tpdf *der;            /**< pointer to the superior typedef. If NULL,
                                          structure provides information about one of the built-in types */

    union {
        /* LY_TYPE_BINARY */
        struct {
            struct lys_restr *length;/**< length restriction (optional), see
                                          [RFC 6020 sec. 9.4.4](http://tools.ietf.org/html/rfc6020#section-9.4.4) */
        } binary;                    /**< part for #LY_TYPE_BINARY */

        /* LY_TYPE_BITS */
        struct {
            struct lys_type_bit {
                const char *name;    /**< bit's name (mandatory) */
                const char *dsc;     /**< bit's description (optional) */
                const char *ref;     /**< bit's reference (optional) */
                uint8_t status;      /**< bit's status, one of LYS_NODE_STATUS_* values (or 0 for default) */
                uint32_t pos;        /**< bit's position (mandatory) */
            } *bit;                  /**< array of bit definitions */
            int count;               /**< number of bit definitions in the bit array */
        } bits;                      /**< part for #LY_TYPE_BITS */

        /* LY_TYPE_DEC64 */
        struct {
            struct lys_restr *range; /**< range restriction (optional), see
                                          [RFC 6020 sec. 9.2.4](http://tools.ietf.org/html/rfc6020#section-9.2.4) */
            uint8_t dig;             /**< fraction-digits restriction (mandatory) */
        } dec64;                     /**< part for #LY_TYPE_DEC64 */

        /* LY_TYPE_ENUM */
        struct {
            struct lys_type_enum {
                const char *name;    /**< enum's name (mandatory) */
                const char *dsc;     /**< enum's description (optional) */
                const char *ref;     /**< enum's reference (optional) */
                uint8_t status;      /**< enum's status, one of LYS_NODE_STATUS_* values (or 0 for default) */
                int32_t value;       /**< enum's value (mandatory) */
            } *enm;                  /**< array of enum definitions */
            int count;               /**< number of enum definitions in the enm array */
        } enums;                     /**< part for #LY_TYPE_ENUM */

        /* LY_TYPE_IDENT */
        struct {
            struct lys_ident *ref;   /**< pointer (reference) to the identity definition (mandatory) */
        } ident;                     /**< part for #LY_TYPE_IDENT */

        /* LY_TYPE_INST */
        struct {
            int8_t req;              /**< require-identifier restriction, see
                                          [RFC 6020 sec. 9.13.2](http://tools.ietf.org/html/rfc6020#section-9.13.2):
                                          - -1 = false,
                                          - 0 not defined,
                                          - 1 = true */
        } inst;                      /**< part for #LY_TYPE_INST */

        /* LY_TYPE_*INT* */
        struct {
            struct lys_restr *range; /**< range restriction (optional), see
                                          [RFC 6020 sec. 9.2.4](http://tools.ietf.org/html/rfc6020#section-9.2.4) */
        } num;                       /**< part for integer types */

        /* LY_TYPE_LEAFREF */
        struct {
            const char *path;        /**< path to the referred leaf or leaf-list node (mandatory), see
                                          [RFC 6020 sec. 9.9.2](http://tools.ietf.org/html/rfc6020#section-9.9.2) */
            struct lys_node_leaf* target; /**< target schema node according to path */
        } lref;                      /**< part for #LY_TYPE_LEAFREF */

        /* LY_TYPE_STRING */
        struct {
            struct lys_restr *length;/**< length restriction (optional), see
                                          [RFC 6020 sec. 9.4.4](http://tools.ietf.org/html/rfc6020#section-9.4.4) */
            struct lys_restr *patterns;   /**< array of pattern restrictions (optional), see
                                          [RFC 6020 sec. 9.4.6](http://tools.ietf.org/html/rfc6020#section-9.4.6) */
            int pat_count;                /**< number of pattern definitions in the patterns array */
        } str;                       /**< part for #LY_TYPE_STRING */

        /* LY_TYPE_UNION */
        struct {
            struct lys_type *types;  /**< array of union's subtypes */
            int count;               /**< number of subtype definitions in types array */
        } uni;                       /**< part for #LY_TYPE_UNION */
    } info;                          /**< detailed type-specific information */
};

/**
 * @defgroup nacmflags NACM flags
 * @ingroup schematree
 *
 * Flags to support NACM YANG extensions following the [RFC 6536](https://tools.ietf.org/html/rfc6536)
 *
 * @{
 */
#define LYS_NACM_DENYW   0x01        /**< default-deny-write extension used */
#define LYS_NACM_DENYA   0x02        /**< default-deny-all extension used */
/**
 * @}
 */

/**
 * @defgroup snodeflags Schema nodes flags
 * @ingroup schematree
 *
 * Various flags for schema nodes.
 *
 * @{
 */
#define LYS_CONFIG_W     0x01        /**< config true; */
#define LYS_CONFIG_R     0x02        /**< config false; */
#define LYS_CONFIG_MASK  0x03        /**< mask for config value */
#define LYS_STATUS_CURR  0x04        /**< status current; */
#define LYS_STATUS_DEPRC 0x08        /**< status deprecated; */
#define LYS_STATUS_OBSLT 0x10        /**< status obsolete; */
#define LYS_STATUS_MASK  0x1c        /**< mask for status value */
#define LYS_MAND_TRUE    0x20        /**< mandatory true; applicable only to
                                          ::lys_node_choice, ::lys_node_leaf and ::lys_node_anyxml */
#define LYS_MAND_FALSE   0x40        /**< mandatory false; applicable only to
                                          ::lys_node_choice, ::lys_node_leaf and ::lys_node_anyxml */
#define LYS_MAND_MASK    0x60        /**< mask for mandatory values */
#define LYS_USERORDERED  0x80        /**< ordered-by user lists, applicable only to
                                          ::lys_node_list and ::lys_node_leaflist */
#define LYS_FENABLED     0x80        /**< feature enabled flag, applicable only to ::lys_feature */
/**
 * @}
 */

/**
 * @brief Common structure representing single YANG data statement describing.
 *
 * This is a common structure to allow having a homogeneous tree of nodes despite the nodes are actually
 * heterogeneous. It allow one to go through the tree in a simple way. However, if you want to work with
 * the node in some way or get more appropriate information, you are supposed to cast it to the appropriate
 * lys_node_* structure according to the #nodetype value.
 *
 * To traverse through all the child elements, use #LY_TREE_FOR or #LY_TREE_FOR_SAFE macro.
 *
 * To cover all possible schema nodes, the ::lys_node type is used in ::lyd_node#schema for referencing schema
 * definition for a specific data node instance.
 *
 * The #private member is completely out of libyang control. It is just a pointer to allow libyang
 * caller to store some proprietary data (e.g. callbacks) connected with the specific schema tree node.
 */
struct lys_node {
    const char *name;                /**< node name (mandatory) */
    const char *dsc;                 /**< description statement (optional) */
    const char *ref;                 /**< reference statement (optional) */
    uint8_t flags;                   /**< [schema node flags](@ref snodeflags) */
    uint8_t nacm;                    /**< [NACM extension flags](@ref nacmflags) */
    struct lys_module *module;       /**< pointer to the node's module (mandatory) */

    LYS_NODE nodetype;               /**< type of the node (mandatory) */
    struct lys_node *parent;         /**< pointer to the parent node, NULL in case of a top level node */
    struct lys_node *child;          /**< pointer to the first child node */
    struct lys_node *next;           /**< pointer to the next sibling node (NULL if there is no one) */
    struct lys_node *prev;           /**< pointer to the previous sibling node \note Note that this pointer is
                                          never NULL. If there is no sibling node, pointer points to the node
                                          itself. In case of the first node, this pointer points to the last
                                          node in the list. */

    uint8_t features_size;           /**< number of elements in the #features array */
    struct lys_feature **features;   /**< array of pointers to feature definitions, this is not the array of feature
                                          definitions themselves, but the array of if-feature references */
    void *private;                   /**< private caller's data, not used by libyang */
};

/**
 * @brief Schema container node structure.
 *
 * Beginning of the structure is completely compatible with ::lys_node structure extending it by the #when,
 * #presence, #must_size, #tpdf_size, #must and #tpdf members.
 *
 * The container schema node can be instantiated in the data tree, so the ::lys_node_container can be directly
 * referenced from ::lyd_node#schema.
 */
struct lys_node_container {
    const char *name;                /**< node name (mandatory) */
    const char *dsc;                 /**< description statement (optional) */
    const char *ref;                 /**< reference statement (optional) */
    uint8_t flags;                   /**< [schema node flags](@ref snodeflags) */
    uint8_t nacm;                    /**< [NACM extension flags](@ref nacmflags) */
    struct lys_module *module;       /**< pointer to the node's module (mandatory) */

    LYS_NODE nodetype;               /**< type of the node (mandatory) - #LYS_CONTAINER */
    struct lys_node *parent;         /**< pointer to the parent node, NULL in case of a top level node */
    struct lys_node *child;          /**< pointer to the first child node */
    struct lys_node *next;           /**< pointer to the next sibling node (NULL if there is no one) */
    struct lys_node *prev;           /**< pointer to the previous sibling node \note Note that this pointer is
                                          never NULL. If there is no sibling node, pointer points to the node
                                          itself. In case of the first node, this pointer points to the last
                                          node in the list. */

    uint8_t features_size;           /**< number of elements in the #features array */
    struct lys_feature **features;   /**< array of pointers to feature definitions, this is not the array of feature
                                          definitions themselves, but the array of if-feature references */
    void *private;                   /**< private caller's data, not used by libyang */

    /* specific container's data */
    struct lys_when *when;           /**< when statement (optional) */
    const char *presence;            /**< presence description, used also as a presence flag (optional) */

    uint8_t must_size;               /**< number of elements in the #must array */
    uint8_t tpdf_size;               /**< number of elements in the #tpdf array */

    struct lys_restr *must;          /**< array of must constraints */
    struct lys_tpdf *tpdf;           /**< array of typedefs */
};

/**
 * @brief Schema choice node structure.
 *
 * Beginning of the structure is completely compatible with ::lys_node structure extending it by the #when and
 * #dflt members.
 *
 * The choice schema node has no instance in the data tree, so the ::lys_node_choice cannot be directly referenced from
 * ::lyd_node#schema.
 */
struct lys_node_choice {
    const char *name;                /**< node name (mandatory) */
    const char *dsc;                 /**< description statement (optional) */
    const char *ref;                 /**< reference statement (optional) */
    uint8_t flags;                   /**< [schema node flags](@ref snodeflags) */
    uint8_t nacm;                    /**< [NACM extension flags](@ref nacmflags) */
    struct lys_module *module;       /**< pointer to the node's module (mandatory) */

    LYS_NODE nodetype;               /**< type of the node (mandatory) - #LYS_CHOICE */
    struct lys_node *parent;         /**< pointer to the parent node, NULL in case of a top level node */
    struct lys_node *child;          /**< pointer to the first child node */
    struct lys_node *next;           /**< pointer to the next sibling node (NULL if there is no one) */
    struct lys_node *prev;           /**< pointer to the previous sibling node \note Note that this pointer is
                                          never NULL. If there is no sibling node, pointer points to the node
                                          itself. In case of the first node, this pointer points to the last
                                          node in the list. */

    uint8_t features_size;           /**< number of elements in the #features array */
    struct lys_feature **features;   /**< array of pointers to feature definitions, this is not the array of feature
                                          definitions themselves, but the array of if-feature references */
    void *private;                   /**< private caller's data, not used by libyang */

    /* specific choice's data */
    struct lys_when *when;           /**< when statement (optional) */
    struct lys_node *dflt;           /**< default case of the choice (optional) */
};

/**
 * @brief Schema leaf node structure.
 *
 * Beginning of the structure is completely compatible with ::lys_node structure extending it by the #when, #type,
 * #units, #must_size, #must and #dflt members. In addition, the structure is compatible with the ::lys_node_leaflist
 * structure except the last #dflt member, which is replaced by ::lys_node_leaflist#min and ::lys_node_leaflist#max
 * members.
 *
 * ::lys_node_leaf is terminating node in the schema tree, so the #child member value is always NULL.
 *
 * The leaf schema node can be instantiated in the data tree, so the ::lys_node_leaf can be directly referenced from
 * ::lyd_node#schema.
 */
struct lys_node_leaf {
    const char *name;                /**< node name (mandatory) */
    const char *dsc;                 /**< description statement (optional) */
    const char *ref;                 /**< reference statement (optional) */
    uint8_t flags;                   /**< [schema node flags](@ref snodeflags) */
    uint8_t nacm;                    /**< [NACM extension flags](@ref nacmflags) */
    struct lys_module *module;       /**< pointer to the node's module (mandatory) */

    LYS_NODE nodetype;               /**< type of the node (mandatory) - #LYS_LEAF */
    struct lys_node *parent;         /**< pointer to the parent node, NULL in case of a top level node */
    struct lys_node *child;          /**< always NULL */
    struct lys_node *next;           /**< pointer to the next sibling node (NULL if there is no one) */
    struct lys_node *prev;           /**< pointer to the previous sibling node \note Note that this pointer is
                                          never NULL. If there is no sibling node, pointer points to the node
                                          itself. In case of the first node, this pointer points to the last
                                          node in the list. */

    uint8_t features_size;           /**< number of elements in the #features array */
    struct lys_feature **features;   /**< array of pointers to feature definitions, this is not the array of feature
                                          definitions themselves, but the array of if-feature references */
    void *private;                   /**< private caller's data, not used by libyang */

    /* specific leaf's data */
    struct lys_when *when;           /**< when statement (optional) */
    struct lys_type type;            /**< YANG data type definition of the leaf (mandatory) */
    const char *units;               /**< units of the data type (optional) */

    uint8_t must_size;               /**< number of elements in the #must array */
    struct lys_restr *must;          /**< array of must constraints */

    /* to this point, struct lys_node_leaf is compatible with struct lys_node_leaflist */
    const char *dflt;                /**< default value of the type */
};

/**
 * @brief Schema leaf-list node structure.
 *
 * Beginning of the structure is completely compatible with ::lys_node structure extending it by the #when, #type,
 * #units, #must_size, #must, #min and #max members. In addition, the structure is compatible with the ::lys_node_leaf
 * structure except the last #min and #max members, which are replaced by ::lys_node_leaf#dflt member.
 *
 * ::lys_node_leaflist is terminating node in the schema tree, so the #child member value is always NULL.
 *
 * The leaf-list schema node can be instantiated in the data tree, so the ::lys_node_leaflist can be directly
 * referenced from ::lyd_node#schema.
 */
struct lys_node_leaflist {
    const char *name;                /**< node name (mandatory) */
    const char *dsc;                 /**< description statement (optional) */
    const char *ref;                 /**< reference statement (optional) */
    uint8_t flags;                   /**< [schema node flags](@ref snodeflags) */
    uint8_t nacm;                    /**< [NACM extension flags](@ref nacmflags) */
    struct lys_module *module;       /**< pointer to the node's module (mandatory) */

    LYS_NODE nodetype;               /**< type of the node (mandatory) - #LYS_LEAFLIST */
    struct lys_node *parent;         /**< pointer to the parent node, NULL in case of a top level node */
    struct lys_node *child;          /**< always NULL */
    struct lys_node *next;           /**< pointer to the next sibling node (NULL if there is no one) */
    struct lys_node *prev;           /**< pointer to the previous sibling node \note Note that this pointer is
                                          never NULL. If there is no sibling node, pointer points to the node
                                          itself. In case of the first node, this pointer points to the last
                                          node in the list. */

    uint8_t features_size;           /**< number of elements in the #features array */
    struct lys_feature **features;   /**< array of pointers to feature definitions, this is not the array of feature
                                          definitions themselves, but the array of if-feature references */
    void *private;                   /**< private caller's data, not used by libyang */

    /* specific leaf-list's data */
    struct lys_when *when;           /**< when statement (optional) */
    struct lys_type type;            /**< YANG data type definition of the leaf (mandatory) */
    const char *units;               /**< units of the data type (optional) */

    uint8_t must_size;               /**< number of elements in the #must array */
    struct lys_restr *must;          /**< array of must constraints */

    /* to this point, struct lys_node_leaflist is compatible with struct lys_node_leaf */
    uint32_t min;                    /**< min-elements constraint (optional) */
    uint32_t max;                    /**< max-elements constraint, 0 means unbounded (optional) */
};

/**
 * @brief Schema list node structure.
 *
 * Beginning of the structure is completely compatible with ::lys_node structure extending it by the #when, #min,
 * #max, #must_size, #tpdf_size, #keys_size, #unique_size, #must, #tpdf, #keys and #unique members.
 *
 * The list schema node can be instantiated in the data tree, so the ::lys_node_list can be directly referenced from
 * ::lyd_node#schema.
 */
struct lys_node_list {
    const char *name;                /**< node name (mandatory) */
    const char *dsc;                 /**< description statement (optional) */
    const char *ref;                 /**< reference statement (optional) */
    uint8_t flags;                   /**< [schema node flags](@ref snodeflags) */
    uint8_t nacm;                    /**< [NACM extension flags](@ref nacmflags) */
    struct lys_module *module;       /**< pointer to the node's module (mandatory) */

    LYS_NODE nodetype;               /**< type of the node (mandatory) - #LYS_LIST */
    struct lys_node *parent;         /**< pointer to the parent node, NULL in case of a top level node */
    struct lys_node *child;          /**< pointer to the first child node */
    struct lys_node *next;           /**< pointer to the next sibling node (NULL if there is no one) */
    struct lys_node *prev;           /**< pointer to the previous sibling node \note Note that this pointer is
                                          never NULL. If there is no sibling node, pointer points to the node
                                          itself. In case of the first node, this pointer points to the last
                                          node in the list. */

    uint8_t features_size;           /**< number of elements in the #features array */
    struct lys_feature **features;   /**< array of pointers to feature definitions, this is not the array of feature
                                          definitions themselves, but the array of if-feature references */
    void *private;                   /**< private caller's data, not used by libyang */

    /* specific list's data */
    struct lys_when *when;           /**< when statement (optional) */

    uint32_t min;                    /**< min-elements constraint */
    uint32_t max;                    /**< max-elements constraint, 0 means unbounded */

    uint8_t must_size;               /**< number of elements in the #must array */
    uint8_t tpdf_size;               /**< number of elements in the #tpdf array */
    uint8_t keys_size;               /**< number of elements in the #keys array */
    uint8_t unique_size;             /**< number of elements in the #unique array (number of unique statements) */

    struct lys_restr *must;          /**< array of must constraints */
    struct lys_tpdf *tpdf;           /**< array of typedefs */
    struct lys_node_leaf **keys;     /**< array of pointers to the key nodes */
    struct lys_unique *unique;       /**< array of unique statement structures */
};

/**
 * @brief Schema anyxml node structure.
 *
 * Beginning of the structure is completely compatible with ::lys_node structure extending it by the #when, #must_size
 * and #must members.
 *
 * ::lys_node_anyxml is terminating node in the schema tree, so the #child member value is always NULL.
 *
 * The anyxml schema node can be instantiated in the data tree, so the ::lys_node_anyxml can be directly referenced from
 * ::lyd_node#schema.
 */
struct lys_node_anyxml {
    const char *name;                /**< node name (mandatory) */
    const char *dsc;                 /**< description statement (optional) */
    const char *ref;                 /**< reference statement (optional) */
    uint8_t flags;                   /**< [schema node flags](@ref snodeflags) */
    uint8_t nacm;                    /**< [NACM extension flags](@ref nacmflags) */
    struct lys_module *module;       /**< pointer to the node's module (mandatory) */

    LYS_NODE nodetype;               /**< type of the node (mandatory) - #LYS_ANYXML */
    struct lys_node *parent;         /**< pointer to the parent node, NULL in case of a top level node */
    struct lys_node *child;          /**< always NULL */
    struct lys_node *next;           /**< pointer to the next sibling node (NULL if there is no one) */
    struct lys_node *prev;           /**< pointer to the previous sibling node \note Note that this pointer is
                                          never NULL. If there is no sibling node, pointer points to the node
                                          itself. In case of the first node, this pointer points to the last
                                          node in the list. */

    uint8_t features_size;           /**< number of elements in the #features array */
    struct lys_feature **features;   /**< array of pointers to feature definitions, this is not the array of feature
                                          definitions themselves, but the array of if-feature references */
    void *private;                   /**< private caller's data, not used by libyang */

    /* specific anyxml's data */
    struct lys_when *when;           /**< when statement (optional) */
    uint8_t must_size;               /**< number of elements in the #must array */
    struct lys_restr *must;          /**< array of must constraints */
};

/**
 * @brief Schema uses node structure.
 *
 * Beginning of the structure is completely compatible with ::lys_node structure extending it by the #when, #grp,
 * #refine_size, #augment_size, #refine and #augment members.
 *
 * ::lys_node_uses is terminating node in the schema tree. However, it references data from a specific grouping so the
 * #child pointer points to the copy of grouping data applying specified refine and augment statements.
 *
 * The uses schema node has no instance in the data tree, so the ::lys_node_uses cannot be directly referenced from
 * ::lyd_node#schema.
 */
struct lys_node_uses {
    const char *name;                /**< node name (mandatory) */
    const char *dsc;                 /**< description statement (optional) */
    const char *ref;                 /**< reference statement (optional) */
    uint8_t flags;                   /**< [schema node flags](@ref snodeflags) - only LYS_STATUS_* values are allowed */
    uint8_t nacm;                    /**< [NACM extension flags](@ref nacmflags) */
    struct lys_module *module;       /**< pointer to the node's module (mandatory) */

    LYS_NODE nodetype;               /**< type of the node (mandatory) - #LYS_USES */
    struct lys_node *parent;         /**< pointer to the parent node, NULL in case of a top level node */
    struct lys_node *child;          /**< pointer to the first child node imported from the referenced grouping */
    struct lys_node *next;           /**< pointer to the next sibling node (NULL if there is no one) */
    struct lys_node *prev;           /**< pointer to the previous sibling node \note Note that this pointer is
                                          never NULL. If there is no sibling node, pointer points to the node
                                          itself. In case of the first node, this pointer points to the last
                                          node in the list. */

    uint8_t features_size;           /**< number of elements in the #features array */
    struct lys_feature **features;   /**< array of pointers to feature definitions, this is not the array of feature
                                          definitions themselves, but the array of if-feature references */
    void *private;                   /**< private caller's data, not used by libyang */

    /* specific uses's data */
    struct lys_when *when;           /**< when statement (optional) */
    struct lys_node_grp *grp;        /**< referred grouping definition (mandatory) */

    uint16_t refine_size;            /**< number of elements in the #refine array */
    uint16_t augment_size;           /**< number of elements in the #augment array */

    struct lys_refine *refine;       /**< array of refine changes to the referred grouping */
    struct lys_node_augment *augment;/**< array of local augments to the referred grouping */
};

/**
 * @brief Schema grouping node structure.
 *
 * Beginning of the structure is completely compatible with ::lys_node structure extending it by the #tpdf_size and
 * #tpdf members.
 *
 * ::lys_node_grp contains data specifications in the schema tree. However, the data does not directly form the schema
 * data tree. Instead, they are referenced via uses (::lys_node_uses) statement and copies of the grouping data are
 * actually placed into the uses nodes. Therefore, the nodes you can find under the ::lys_node_grp are not referenced
 * from ::lyd_node#schema.
 */
struct lys_node_grp {
    const char *name;                /**< node name (mandatory) */
    const char *dsc;                 /**< description statement (optional) */
    const char *ref;                 /**< reference statement (optional) */
    uint8_t flags;                   /**< [schema node flags](@ref snodeflags) - only LYS_STATUS_* values are allowed */
    uint8_t nacm;                    /**< [NACM extension flags](@ref nacmflags) - always 0 in ::lys_node_grp */
    struct lys_module *module;       /**< pointer to the node's module (mandatory) */

    LYS_NODE nodetype;               /**< type of the node (mandatory) - #LYS_GROUPING */
    struct lys_node *parent;         /**< pointer to the parent node, NULL in case of a top level node */
    struct lys_node *child;          /**< pointer to the first child node */
    struct lys_node *next;           /**< pointer to the next sibling node (NULL if there is no one) */
    struct lys_node *prev;           /**< pointer to the previous sibling node \note Note that this pointer is
                                          never NULL. If there is no sibling node, pointer points to the node
                                          itself. In case of the first node, this pointer points to the last
                                          node in the list. */

    uint8_t features_size;           /**< number of elements in the #features array */
    struct lys_feature **features;   /**< array of pointers to feature definitions, this is not the array of feature
                                          definitions themselves, but the array of if-feature references */
    void *private;                   /**< private caller's data, not used by libyang */

    /* specific grouping's data */
    uint8_t tpdf_size;               /**< number of elements in #tpdf array */
    struct lys_tpdf *tpdf;           /**< array of typedefs */
};

/**
 * @brief Schema case node structure.
 *
 * Beginning of the structure is completely compatible with ::lys_node structure extending it by the #when member.
 *
 * The case schema node has no instance in the data tree, so the ::lys_node_case cannot be directly referenced from
 * ::lyd_node#schema.
 */
struct lys_node_case {
    const char *name;                /**< node name (mandatory) */
    const char *dsc;                 /**< description statement (optional) */
    const char *ref;                 /**< reference statement (optional) */
    uint8_t flags;                   /**< [schema node flags](@ref snodeflags) */
    uint8_t nacm;                    /**< [NACM extension flags](@ref nacmflags) */
    struct lys_module *module;       /**< pointer to the node's module (mandatory) */

    LYS_NODE nodetype;               /**< type of the node (mandatory) - #LYS_CASE */
    struct lys_node *parent;         /**< pointer to the parent node, NULL in case of a top level node */
    struct lys_node *child;          /**< pointer to the first child node */
    struct lys_node *next;           /**< pointer to the next sibling node (NULL if there is no one) */
    struct lys_node *prev;           /**< pointer to the previous sibling node \note Note that this pointer is
                                          never NULL. If there is no sibling node, pointer points to the node
                                          itself. In case of the first node, this pointer points to the last
                                          node in the list. */

    uint8_t features_size;           /**< number of elements in the #features array */
    struct lys_feature **features;   /**< array of pointers to feature definitions, this is not the array of feature
                                          definitions themselves, but the array of if-feature references */
    void *private;                   /**< private caller's data, not used by libyang */

    /* specific case's data */
    struct lys_when *when;           /**< when statement (optional) */
};

/**
 * @brief RPC input and output node structure.
 *
 * The structure is compatible with ::lys_node, but the most parts are not usable. Therefore the ::lys_node#name,
 * ::lys_node#dsc, ::lys_node#ref, ::lys_node#flags and ::lys_node#nacm were replaced by empty bytes in fill arrays.
 * The reason to keep these useless bytes in the structure is to keep the #nodetype, #parent, #child, #next and #prev
 * members accessible when functions are using the object via a generic ::lyd_node structure. But note that the
 * ::lys_node#features_size is replaced by the #tpdf_size member and ::lys_node#features is replaced by the #tpdf
 * member.
 *
 */
struct lys_node_rpc_inout {
    void *fill1[3];                  /**< padding for compatibility with ::lys_node - name, dsc and ref */
    uint8_t fill2[2];                /**< padding for compatibility with ::lys_node - flags and nacm */
    struct lys_module *module;       /**< link to the node's data model */

    LYS_NODE nodetype;               /**< type of the node (mandatory) - #LYS_INPUT or #LYS_OUTPUT */
    struct lys_node *parent;         /**< pointer to the parent rpc node  */
    struct lys_node *child;          /**< pointer to the first child node */
    struct lys_node *next;           /**< pointer to the next sibling node (NULL if there is no one) */
    struct lys_node *prev;           /**< pointer to the previous sibling node \note Note that this pointer is
                                          never NULL. If there is no sibling node, pointer points to the node
                                          itself. In case of the first node, this pointer points to the last
                                          node in the list. */

    /* specific list's data */
    uint8_t tpdf_size;                /**< number of elements in the #tpdf array */
    struct lys_tpdf *tpdf;            /**< array of typedefs */

    /* again ::lys_node compatible data */
    void *private;                   /**< private caller's data, not used by libyang */
};

/**
 * @brief Schema notification node structure.
 *
 * Beginning of the structure is completely compatible with ::lys_node structure extending it by the #tpdf_size and
 * #tpdf members.
 */
struct lys_node_notif {
    const char *name;                /**< node name (mandatory) */
    const char *dsc;                 /**< description statement (optional) */
    const char *ref;                 /**< reference statement (optional) */
    uint8_t flags;                   /**< [schema node flags](@ref snodeflags) */
    uint8_t nacm;                    /**< [NACM extension flags](@ref nacmflags) */
    struct lys_module *module;       /**< pointer to the node's module (mandatory) */

    LYS_NODE nodetype;               /**< type of the node (mandatory) - #LYS_NOTIF */
    struct lys_node *parent;         /**< pointer to the parent node, NULL in case of a top level node */
    struct lys_node *child;          /**< pointer to the first child node */
    struct lys_node *next;           /**< pointer to the next sibling node (NULL if there is no one) */
    struct lys_node *prev;           /**< pointer to the previous sibling node \note Note that this pointer is
                                          never NULL. If there is no sibling node, pointer points to the node
                                          itself. In case of the first node, this pointer points to the last
                                          node in the list. */

    uint8_t features_size;           /**< number of elements in the #features array */
    struct lys_feature **features;   /**< array of pointers to feature definitions, this is not the array of feature
                                          definitions themselves, but the array of if-feature references */
    void *private;                   /**< private caller's data, not used by libyang */

    /* specific rpc's data */
    uint8_t tpdf_size;               /**< number of elements in the #tpdf array */
    struct lys_tpdf *tpdf;           /**< array of typedefs */
};

/**
 * @brief Schema rpc node structure.
 *
 * Beginning of the structure is completely compatible with ::lys_node structure extending it by the #tpdf_size and
 * #tpdf members.
 */
struct lys_node_rpc {
    const char *name;                /**< node name (mandatory) */
    const char *dsc;                 /**< description statement (optional) */
    const char *ref;                 /**< reference statement (optional) */
    uint8_t flags;                   /**< [schema node flags](@ref snodeflags) */
    uint8_t nacm;                    /**< [NACM extension flags](@ref nacmflags) */
    struct lys_module *module;       /**< pointer to the node's module (mandatory) */

    LYS_NODE nodetype;               /**< type of the node (mandatory) - #LYS_RPC */
    struct lys_node *parent;         /**< pointer to the parent node, NULL in case of a top level node */
    struct lys_node *child;          /**< pointer to the first child node */
    struct lys_node *next;           /**< pointer to the next sibling node (NULL if there is no one) */
    struct lys_node *prev;           /**< pointer to the previous sibling node \note Note that this pointer is
                                          never NULL. If there is no sibling node, pointer points to the node
                                          itself. In case of the first node, this pointer points to the last
                                          node in the list. */

    uint8_t features_size;           /**< number of elements in the #features array */
    struct lys_feature **features;   /**< array of pointers to feature definitions, this is not the array of feature
                                          definitions themselves, but the array of if-feature references */
    void *private;                   /**< private caller's data, not used by libyang */

    /* specific rpc's data */
    uint8_t tpdf_size;               /**< number of elements in the #tpdf array */
    struct lys_tpdf *tpdf;           /**< array of typedefs */
};

/**
 * @brief YANG augment structure (covering both possibilities - uses's substatement as well as (sub)module's substatement).
 *
 * This structure is partially interchangeable with ::lys_node structure with the following exceptions:
 * - ::lys_node#name member is replaced by ::lys_node_augment#target_name member
 * - ::lys_node_augment structure is extended by the #when and #target member
 *
 * ::lys_node_augment is not placed between all other nodes defining data node. However, it must be compatible with
 * ::lys_node structure since its children actually keeps the parent pointer to point to the original augment node
 * instead of the target node they augments (the target node is accessible via the ::lys_node_augment#target pointer).
 * The fact that a schema node comes from augment can be get via testing the #nodetype of its parent - the value in
 * ::lys_node_augment is #LYS_AUGMENT.
 */
struct lys_node_augment {
    const char *target_name;         /**< schema node identifier of the node where the augment content is supposed to be
                                          placed (mandatory). */
    const char *dsc;                 /**< description statement (optional) */
    const char *ref;                 /**< reference statement (optional) */
    uint8_t flags;                   /**< [schema node flags](@ref snodeflags) */
    uint8_t nacm;                    /**< [NACM extension flags](@ref nacmflags) */
    struct lys_module *module;       /**< pointer to the node's module (mandatory) */

    LYS_NODE nodetype;               /**< #LYS_AUGMENT */
    struct lys_node *parent;         /**< uses node or NULL in case of module's top level augment */
    struct lys_node *child;          /**< augmenting data \note The child here points to the data which are also
                                          placed as children in the target node. Children are connected within the
                                          child list of the target, but their parent member still points to the augment
                                          node (this way they can be distinguished from the original target's children).
                                          It is necessary to check this carefully. */

    /* replaces #next and #prev members of ::lys_node */
    struct lys_when *when;           /**< when statement (optional) */
    struct lys_node *target;         /**< pointer to the target node TODO refer to augmentation description */

    /* again compatible members with ::lys_node */
    uint8_t features_size;           /**< number of elements in the #features array */
    struct lys_feature **features;   /**< array of pointers to feature definitions, this is not the array of feature
                                          definitions themselves, but the array of if-feature references */
    void *private;                   /**< private caller's data, not used by libyang */

};

/**
 * @brief YANG uses's refine substatement structure, see [RFC 6020 sec. 7.12.2](http://tools.ietf.org/html/rfc6020#section-7.12.2)
 */
struct lys_refine {
    const char *target_name;         /**< descendant schema node identifier of the target node to be refined (mandatory) */
    const char *dsc;                 /**< description statement (optional) */
    const char *ref;                 /**< reference statement (optional) */
    uint8_t flags;                   /**< [schema node flags](@ref snodeflags) */

    uint16_t target_type;            /**< limitations (get from specified refinements) for target node type:
                                          - 0 = no limitations,
                                          - ORed #LYS_NODE values if there are some limitations */

    uint8_t must_size;               /**< number of elements in the #must array */
    struct lys_restr *must;          /**< array of additional must restrictions to be added to the target */

    union {
        const char *dflt;            /**< new default value. Applicable to #LYS_LEAF and #LYS_CHOICE target nodes. In case of
                                          #LYS_CHOICE, it must be possible to resolve the value to the default branch node */
        const char *presence;        /**< presence description. Applicable to #LYS_CONTAINER target node */
        struct {
            uint32_t min;            /**< new min-elements value. Applicable to #LYS_LIST and #LYS_LEAFLIST target nodes */
            uint32_t max;            /**< new max-elements value. Applicable to #LYS_LIST and #LYS_LEAFLIST target nodes */
        } list;                      /**< container for list's attributes - applicable to #LYS_LIST and #LYS_LEAFLIST target nodes */
    } mod;                           /**< mutually exclusive target modifications according to the possible target_type */
};


/**
 * @brief Possible deviation modifications, see [RFC 6020 sec. 7.18.3.2](http://tools.ietf.org/html/rfc6020#section-7.18.3.2)
 */
typedef enum lys_deviate_type {
    LY_DEVIATE_NO,                   /**< not-supported */
    LY_DEVIATE_ADD,                  /**< add */
    LY_DEVIATE_RPL,                  /**< replace */
    LY_DEVIATE_DEL                   /**< delete */
} LYS_DEVIATE_TYPE;

/**
 * @brief YANG deviate statement structure, see [RFC 6020 sec. 7.18.3.2](http://tools.ietf.org/html/rfc6020#section-7.18.3.2)
 */
struct lys_deviate {
    LYS_DEVIATE_TYPE mod;            /**< type of deviation modification */

    uint8_t flags;                   /**< Properties: config, mandatory */
    const char *dflt;                /**< Properties: default (both type and choice represented as string value */
    uint32_t min;                    /**< Properties: min-elements */
    uint32_t max;                    /**< Properties: max-elements */
    uint8_t must_size;               /**< Properties: must - number of elements in the #must array */
    uint8_t unique_size;             /**< Properties: unique - number of elements in the #unique array */
    struct lys_restr *must;          /**< Properties: must - array of must constraints */
    struct lys_unique *unique;       /**< Properties: unique - array of unique statement structures */
    struct lys_type *type;           /**< Properties: type - pointer to type in target, type cannot be deleted or added */
    const char *units;               /**< Properties: units */
};

/**
 * @brief YANG deviation statement structure, see [RFC 6020 sec. 7.18.3](http://tools.ietf.org/html/rfc6020#section-7.18.3)
 */
struct lys_deviation {
    const char *target_name;         /**< schema node identifier of the node where the deviation is supposed to be
                                          applied (mandatory). */
    const char *dsc;                 /**< description (optional) */
    const char *ref;                 /**< reference (optional) */
    struct lys_node *target;         /**< pointer to the target node TODO refer to deviation description */

    uint8_t deviate_size;            /**< number of elements in the #deviate array */
    struct lys_deviate *deviate;     /**< deviate information */
};

/**
 * @brief YANG import structure used to reference other schemas (modules).
 */
struct lys_import {
    struct lys_module *module;       /**< link to the imported module (mandatory) */
    const char *prefix;              /**< prefix for the data from the imported schema (mandatory) */
    char rev[LY_REV_SIZE];           /**< revision-date of the imported module (optional) */
};

/**
 * @brief YANG include structure used to reference submodules.
 */
struct lys_include {
    struct lys_submodule *submodule; /**< link to the included submodule (mandatory) */
    char rev[LY_REV_SIZE];           /**< revision-date of the included submodule (optional) */
};

/**
 * @brief YANG revision statement for (sub)modules
 */
struct lys_revision {
    char date[LY_REV_SIZE];          /**< revision-date (mandatory) */
    const char *dsc;                 /**< revision's dsc (optional) */
    const char *ref;                 /**< revision's reference (optional) */
};

/**
 * @brief YANG typedef structure providing information from the schema
 */
struct lys_tpdf {
    const char *name;                /**< name of the newly defined type (mandatory) */
    const char *dsc;                 /**< description statement (optional) */
    const char *ref;                 /**< reference statement (optional) */
    uint8_t flags;                   /**< [schema node flags](@ref snodeflags) - only LYS_STATUS_ values (or 0) are allowed */
    struct lys_module *module;       /**< pointer to the module where the data type is defined (mandatory),
                                          NULL in case of built-in typedefs */

    struct lys_type type;            /**< base type from which the typedef is derived (mandatory). In case of a special
                                          built-in typedef (from yang_types.c), only the base member is filled */
    const char *units;               /**< units of the newly defined type (optional) */
    const char *dflt;                /**< default value of the newly defined type (optional) */
};

/**
 * @brief YANG list's unique statement structure, see [RFC 6020 sec. 7.8.3](http://tools.ietf.org/html/rfc6020#section-7.8.3)
 */
struct lys_unique {
    const char **expr;               /**< array of unique expressions specifying target leafs to be unique */
    uint8_t expr_size;               /**< size of the #axpr array */
};

/**
 * @brief YANG feature definition structure
 */
struct lys_feature {
    const char *name;                /**< feature name (mandatory) */
    const char *dsc;                 /**< description statement (optional) */
    const char *ref;                 /**< reference statement (optional) */
    uint8_t flags;                   /**< [schema node flags](@ref snodeflags) - only LYS_STATUS_* values and
                                          #LYS_FENABLED value are allowed */
    struct lys_module *module;       /**< link to the features's data model (mandatory) */

    uint8_t features_size;           /**< number of elements in the #features array */
    struct lys_feature **features;   /**< array of pointers to feature definitions, this is not the array of feature
                                          definitions themselves, but the array of if-feature references */
};

/**
 * @brief YANG validity restriction (must, length, etc.) structure providing information from the schema
 */
struct lys_restr {
    const char *expr;                /**< The restriction expression/value (mandatory) */
    const char *dsc;                 /**< description (optional) */
    const char *ref;                 /**< reference (optional) */
    const char *eapptag;             /**< error-app-tag value (optional) */
    const char *emsg;                /**< error-message (optional) */
};

/**
 * @brief YANG when restriction, see [RFC 6020 sec. 7.19.5](http://tools.ietf.org/html/rfc6020#section-7.19.5)
 */
struct lys_when {
    const char *cond;                /**< specified condition (mandatory) */
    const char *dsc;                 /**< description (optional) */
    const char *ref;                 /**< reference (optional) */
};

/**
 * @brief Structure to hold information about identity, see  [RFC 6020 sec. 7.16](http://tools.ietf.org/html/rfc6020#section-7.16)
 *
 * First 5 members maps to ::lys_node.
 */
struct lys_ident {
    const char *name;                /**< identity name (mandatory) */
    const char *dsc;                 /**< description statement (optional) */
    const char *ref;                 /**< reference statement (optional) */
    uint8_t flags;                   /**< [schema node flags](@ref snodeflags) - only LYS_STATUS_ values are allowed */
    struct lys_module *module;       /**< pointer to the module where the identity is defined */

    struct lys_ident *base;          /**< pointer to the base identity */
    struct lys_ident_der *der;       /**< list of pointers to the derived identities */
};
/**
 * @brief Structure to serialize pointers to the identities.
 *
 * The list of derived identities cannot be static since any new schema can
 * extend the current set of derived identities.
 *
 * TODO: the list actually could be just a static array and we could reallocate it whenever it
 * is needed. Since we are not going to allow removing a particular schema from
 * the context, we don't need to remove a subset of pointers to derived
 * identities.
 */
struct lys_ident_der {
    struct lys_ident *ident;         /**< pointer to the identity */
    struct lys_ident_der *next;      /**< next record, NULL in case of the last record in the list */
};

/**
 * @brief Get list of all the defined features in the module and its submodules.
 *
 * @param[in] module Module to explore.
 * @param[out] states Optional output parameter providing states of all features
 * returned by function in the resulting array. Indexes in both arrays corresponds
 * each other. Similarly to lys_feature_state(), possible values in the state array
 * are 1 (enabled) and 0 (disabled). Caller is supposed to free the array when it
 * is no more needed.
 * @return NULL-terminated array of all the defined features. The returned array
 * must be freed by the caller, do not free names in the array. Also remember
 * that the names will be freed with freeing the context of the module.
 */
const char **lys_features_list(const struct lys_module *module, uint8_t **states);

/**
 * @brief Enable specified feature in the module
 *
 * By default, when the module is loaded by libyang parser, all features are disabled.
 *
 * @param[in] module Module where the feature will be enabled.
 * @param[in] feature Name of the feature to enable. To enable all features at once, use asterisk character.
 * @return 0 on success, 1 when the feature is not defined in the specified module
 */
int lys_features_enable(const struct lys_module *module, const char *feature);

/**
 * @brief Disable specified feature in the module
 *
 * By default, when the module is loaded by libyang parser, all features are disabled.
 *
 * @param[in] module Module where the feature will be disabled.
 * @param[in] feature Name of the feature to disable. To disable all features at once, use asterisk character.
 * @return 0 on success, 1 when the feature is not defined in the specified module
 */
int lys_features_disable(const struct lys_module *module, const char *feature);

/**
 * @brief Get the current status of the specified feature in the module.
 *
 * @param[in] module Module where the feature is defined.
 * @param[in] feature Name of the feature to inspect.
 * @return
 * - 1 if feature is enabled,
 * - 0 if feature is disabled,
 * - -1 in case of error (e.g. feature is not defined)
 */
int lys_features_state(const struct lys_module *module, const char *feature);

/**
 * @brief Check if the schema node is enabled in the schema tree, i.e. there is no disabled if-feature statement
 * affecting the node.
 *
 * @param[in] node Schema node to check.
 * @param[in] recursive - 0 to check if-feature only in the \p node schema node,
 * - 1 to check if-feature in all ascendant schema nodes
 * - 2 to check if-feature in all ascendant schema nodes that cannot have an instance in a data tree
 * @return - NULL if enabled,
 * - pointer to the disabling feature if disabled.
 */
const struct lys_feature *lys_is_disabled(const struct lys_node *node, int recursive);

/**
 * @brief Get next schema tree (sibling) node element that can be instanciated in a data tree. Returned node can
 * be from an augment.
 *
 * lys_getnext() is supposed to be called sequentially. In the first call, the \p last parameter is usually NULL
 * and function starts returning i) the first \p parent child or ii) the first top level element of the \p module.
 * Consequent calls suppose to provide the previously returned node as the \p last parameter and still the same
 * \p parent and \p module parameters.
 *
 * @param[in] last Previously returned schema tree node, or NULL in case of the first call.
 * @param[in] parent Parent of the subtree where the function starts processing
 * @param[in] module In case of iterating on top level elements, the \p parent is NULL and module must be specified.
 * @param[in] options ORed options LYS_GETNEXT_*.
 * @return Next schema tree node that can be instanciated in a data tree, NULL in case there is no such element
 */
const struct lys_node *lys_getnext(const struct lys_node *last, const struct lys_node *parent,
                                   const struct lys_module *module, int options);

/**
 * @brief options for lys_getnext() to allow returning choice, case, grouping, input and output nodes.
 */
#define LYS_GETNEXT_WITHCHOICE   0x01
#define LYS_GETNEXT_WITHCASE     0x02
#define LYS_GETNEXT_WITHGROUPING 0x04
#define LYS_GETNEXT_WITHINOUT    0x08

/**
 * @brief Return parent node in the schema tree.
 *
 * In case of augmenting node, it returns the target tree node where the augmenting
 * node was placed, not the augment definition node. Function just wraps usage of the
 * ::lys_node#parent pointer in this special case.
 *
 * @param[in] node Child node to the returned parent node.
 * @return The parent node from the schema tree, NULL in case of top level nodes.
 */
struct lys_node *lys_parent(const struct lys_node *node);

/**@} */

#ifdef __cplusplus
}
#endif

#endif /* LY_TREE_SCHEMA_H_ */
