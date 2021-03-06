
/**
 * @file printer_xml.c
 * @author Michal Vasko <mvasko@cesnet.cz>
 * @brief XML printer for libyang data structure
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
#define _XOPEN_SOURCE 700

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>

#include "common.h"
#include "printer.h"
#include "xml_internal.h"
#include "tree_data.h"
#include "tree_schema.h"
#include "resolve.h"
#include "tree_internal.h"

#define INDENT ""
#define LEVEL (level ? level*2-2 : 0)

void xml_print_node(struct lyout *out, int level, const struct lyd_node *node, int toplevel);

static void
xml_print_ns(struct lyout *out, const struct lyd_node *node)
{
    struct lyd_node *next, *cur;
    struct lyd_attr *attr;
    struct mlist {
        struct mlist *next;
        struct lys_module *module;
    } *mlist = NULL, *mlist_new;

    assert(out);
    assert(node);

    for (attr = node->attr; attr; attr = attr->next) {
        for (mlist_new = mlist; mlist_new; mlist_new = mlist_new->next) {
            if (attr->module == mlist_new->module) {
                break;
            }
        }

        if (!mlist_new) {
            mlist_new = malloc(sizeof *mlist_new);
            mlist_new->next = mlist;
            mlist_new->module = attr->module;
            mlist = mlist_new;
        }
    }

    if (!(node->schema->nodetype & (LYS_LEAF | LYS_LEAFLIST))) {
        LY_TREE_DFS_BEGIN(node->child, next, cur) {
            for (attr = cur->attr; attr; attr = attr->next) {
                for (mlist_new = mlist; mlist_new; mlist_new = mlist_new->next) {
                    if (attr->module == mlist_new->module) {
                        break;
                    }
                }

                if (!mlist_new) {
                    mlist_new = malloc(sizeof *mlist_new);
                    mlist_new->next = mlist;
                    mlist_new->module = attr->module;
                    mlist = mlist_new;
                }
            }
        LY_TREE_DFS_END(node->child, next, cur)}
    }

    /* print used namespaces */
    while (mlist) {
        mlist_new = mlist;
        mlist = mlist->next;

        ly_print(out, " xmlns:%s=\"%s\"", mlist_new->module->prefix, mlist_new->module->ns);
        free(mlist_new);
    }
}

static void
xml_print_attrs(struct lyout *out, const struct lyd_node *node)
{
    struct lyd_attr *attr;
    const char **prefs, **nss;
    const char *xml_expr;
    uint32_t ns_count, i;
    int rpc_filter = 0;

    /* technically, check for the extension get-filter-element-attributes from ietf-netconf */
    if (!strcmp(node->schema->name, "filter")
            && (!strcmp(node->schema->module->name, "ietf-netconf") || !strcmp(node->schema->module->name, "notifications"))) {
        rpc_filter = 1;
    }

    for (attr = node->attr; attr; attr = attr->next) {
        if (rpc_filter && !strcmp(attr->name, "type")) {
            ly_print(out, " %s=\"", attr->name);
        } else if (rpc_filter && !strcmp(attr->name, "select")) {
            xml_expr = transform_json2xml(node->schema->module, attr->value, &prefs, &nss, &ns_count);
            if (!xml_expr) {
                /* error */
                ly_print(out, "\"(!error!)\"");
                return;
            }

            for (i = 0; i < ns_count; ++i) {
                ly_print(out, " xmlns:%s=\"%s\"", prefs[i], nss[i]);
            }
            free(prefs);
            free(nss);

            ly_print(out, " %s=\"", attr->name);
            lyxml_dump_text(out, xml_expr);
            ly_print(out, "\"");

            lydict_remove(node->schema->module->ctx, xml_expr);
            continue;
        } else {
            ly_print(out, " %s:%s=\"", attr->module->prefix, attr->name);
        }
        lyxml_dump_text(out, attr->value);
        ly_print(out, "\"");
    }
}

static void
xml_print_leaf(struct lyout *out, int level, const struct lyd_node *node, int toplevel)
{
    const struct lyd_node_leaf_list *leaf = (struct lyd_node_leaf_list *)node;
    const char *ns;
    const char **prefs, **nss;
    const char *xml_expr;
    uint32_t ns_count, i;

    if (!node->parent || nscmp(node, node->parent)) {
        /* print "namespace" */
        if (node->schema->module->type) {
            /* submodule, get module */
            ns = ((struct lys_submodule *)node->schema->module)->belongsto->ns;
        } else {
            ns = node->schema->module->ns;
        }
        ly_print(out, "%*s<%s xmlns=\"%s\"", LEVEL, INDENT, node->schema->name, ns);
    } else {
        ly_print(out, "%*s<%s", LEVEL, INDENT, node->schema->name);
    }

    if (toplevel) {
        xml_print_ns(out, node);
    }

    xml_print_attrs(out, node);

    switch (leaf->value_type & LY_DATA_TYPE_MASK) {
    case LY_TYPE_BINARY:
    case LY_TYPE_STRING:
    case LY_TYPE_BITS:
    case LY_TYPE_ENUM:
    case LY_TYPE_BOOL:
    case LY_TYPE_DEC64:
    case LY_TYPE_INT8:
    case LY_TYPE_INT16:
    case LY_TYPE_INT32:
    case LY_TYPE_INT64:
    case LY_TYPE_UINT8:
    case LY_TYPE_UINT16:
    case LY_TYPE_UINT32:
    case LY_TYPE_UINT64:
        if (!leaf->value_str) {
            ly_print(out, "/>");
        } else {
            ly_print(out, ">");
            lyxml_dump_text(out, leaf->value_str);
            ly_print(out, "</%s>", node->schema->name);
        }
        break;

    case LY_TYPE_IDENT:
    case LY_TYPE_INST:
        xml_expr = transform_json2xml(node->schema->module, ((struct lyd_node_leaf_list *)node)->value_str,
                                      &prefs, &nss, &ns_count);
        if (!xml_expr) {
            /* error */
            ly_print(out, "\"(!error!)\"");
            return;
        }

        for (i = 0; i < ns_count; ++i) {
            ly_print(out, " xmlns:%s=\"%s\"", prefs[i], nss[i]);
        }
        free(prefs);
        free(nss);

        if (xml_expr[0]) {
            ly_print(out, ">");
            lyxml_dump_text(out, xml_expr);
            ly_print(out, "</%s>", node->schema->name);
        } else {
            ly_print(out, "/>");
        }
        lydict_remove(node->schema->module->ctx, xml_expr);
        break;

    case LY_TYPE_LEAFREF:
        ly_print(out, ">");
        lyxml_dump_text(out, ((struct lyd_node_leaf_list *)(leaf->value.leafref))->value_str);
        ly_print(out, "</%s>", node->schema->name);
        break;

    case LY_TYPE_EMPTY:
        ly_print(out, "/>");
        break;

    default:
        /* error */
        ly_print(out, "\"(!error!)\"");
    }

    if (level) {
        ly_print(out, "\n");
    }
}

static void
xml_print_container(struct lyout *out, int level, const struct lyd_node *node, int toplevel)
{
    struct lyd_node *child;
    const char *ns;

    if (!node->parent || nscmp(node, node->parent)) {
        /* print "namespace" */
        if (node->schema->module->type) {
            /* submodule, get module */
            ns = ((struct lys_submodule *)node->schema->module)->belongsto->ns;
        } else {
            ns = node->schema->module->ns;
        }
        ly_print(out, "%*s<%s xmlns=\"%s\"", LEVEL, INDENT, node->schema->name, ns);
    } else {
        ly_print(out, "%*s<%s", LEVEL, INDENT, node->schema->name);
    }

    if (toplevel) {
        xml_print_ns(out, node);
    }

    xml_print_attrs(out, node);

    if (!node->child) {
        ly_print(out, "/>%s", level ? "\n" : "");
        return;
    }
    ly_print(out, ">%s", level ? "\n" : "");

    LY_TREE_FOR(node->child, child) {
        xml_print_node(out, level ? level + 1 : 0, child, 0);
    }

    ly_print(out, "%*s</%s>%s", LEVEL, INDENT, node->schema->name, level ? "\n" : "");
}

static void
xml_print_list(struct lyout *out, int level, const struct lyd_node *node, int is_list, int toplevel)
{
    struct lyd_node *child;
    const char *ns;

    if (is_list) {
        /* list print */
        if (!node->parent || nscmp(node, node->parent)) {
            /* print "namespace" */
            if (node->schema->module->type) {
                /* submodule, get module */
                ns = ((struct lys_submodule *)node->schema->module)->belongsto->ns;
            } else {
                ns = node->schema->module->ns;
            }
            ly_print(out, "%*s<%s xmlns=\"%s\"", LEVEL, INDENT, node->schema->name, ns);
        } else {
            ly_print(out, "%*s<%s", LEVEL, INDENT, node->schema->name);
        }

        if (toplevel) {
            xml_print_ns(out, node);
        }
        xml_print_attrs(out, node);

        if (!node->child) {
            ly_print(out, "/>%s", level ? "\n" : "");
            return;
        }
        ly_print(out, ">%s", level ? "\n" : "");

        LY_TREE_FOR(node->child, child) {
            xml_print_node(out, level ? level + 1 : 0, child, 0);
        }

        ly_print(out, "%*s</%s>%s", LEVEL, INDENT, node->schema->name, level ? "\n" : "");
    } else {
        /* leaf-list print */
        xml_print_leaf(out, level, node, toplevel);
    }
}

static void
xml_print_anyxml(struct lyout *out, int level, const struct lyd_node *node, int toplevel)
{
    FILE *stream;
    char *buf;
    size_t buf_size;
    struct lyd_node_anyxml *axml = (struct lyd_node_anyxml *)node;
    const char *ns;

    if (!node->parent || nscmp(node, node->parent)) {
        /* print "namespace" */
        if (node->schema->module->type) {
            /* submodule, get module */
            ns = ((struct lys_submodule *)node->schema->module)->belongsto->ns;
        } else {
            ns = node->schema->module->ns;
        }
        ly_print(out, "%*s<%s xmlns=\"%s\"", LEVEL, INDENT, node->schema->name, ns);
    } else {
        ly_print(out, "%*s<%s", LEVEL, INDENT, node->schema->name);
    }

    if (toplevel) {
        xml_print_ns(out, node);
    }
    xml_print_attrs(out, node);

    if (axml->value) {
        ly_print(out, ">%s", level ? "\n" : "");

        /* dump the anyxml into a buffer */
        stream = open_memstream(&buf, &buf_size);
        lyxml_dump(stream, axml->value, 0);
        fclose(stream);

        ly_print(out, "%s</%s>%s", buf, node->schema->name, level ? "\n" : "");
        free(buf);
    } else {
        ly_print(out, "/>%s", level ? "\n" : "");
    }
}

void
xml_print_node(struct lyout *out, int level, const struct lyd_node *node, int toplevel)
{
    switch (node->schema->nodetype) {
    case LYS_NOTIF:
    case LYS_RPC:
    case LYS_CONTAINER:
        xml_print_container(out, level, node, toplevel);
        break;
    case LYS_LEAF:
        xml_print_leaf(out, level, node, toplevel);
        break;
    case LYS_LEAFLIST:
        xml_print_list(out, level, node, 0, toplevel);
        break;
    case LYS_LIST:
        xml_print_list(out, level, node, 1, toplevel);
        break;
    case LYS_ANYXML:
        xml_print_anyxml(out, level, node, toplevel);
        break;
    default:
        LOGINT;
        break;
    }
}

int
xml_print_data(struct lyout *out, const struct lyd_node *root, int format)
{
    const struct lyd_node *node;

    /* content */
    LY_TREE_FOR(root, node) {
        xml_print_node(out, format ? 1 : 0, node, 1);
    }

    return EXIT_SUCCESS;
}

