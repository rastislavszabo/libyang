/**
 * @file printer/json.c
 * @author Radek Krejci <rkrejci@cesnet.cz>
 * @brief JSON printer for libyang data structure
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>

#include "common.h"
#include "printer.h"
#include "tree_data.h"
#include "resolve.h"
#include "tree_internal.h"

#define INDENT ""
#define LEVEL (level*2)

void json_print_nodes(struct lyout *out, int level, const struct lyd_node *root);

static void
json_print_attrs(struct lyout *out, int level, const struct lyd_node *node)
{
    struct lyd_attr *attr;

    for (attr = node->attr; attr; attr = attr->next) {
        if (attr->module != node->schema->module) {
            ly_print(out, "%*s\"%s:%s\":\"%s\"%s", LEVEL, INDENT, attr->module->name, attr->name, attr->value,
                     attr->next ? ",\n" : "\n");
        } else {
            ly_print(out, "%*s\"%s\":\"%s\"%s", LEVEL, INDENT, attr->name, attr->value, attr->next ? ",\n" : "\n");
        }
    }
}

static void
json_print_leaf(struct lyout *out, int level, const struct lyd_node *node, int onlyvalue)
{
    struct lyd_node_leaf_list *leaf = (struct lyd_node_leaf_list *)node;
    const char *schema = NULL;

    if (!onlyvalue) {
        if (!node->parent || nscmp(node, node->parent)) {
            /* print "namespace" */
            if (node->schema->module->type) {
                /* submodule, get module */
                schema = ((struct lys_submodule *)node->schema->module)->belongsto->name;
            } else {
                schema = node->schema->module->name;
            }
            ly_print(out, "%*s\"%s:%s\": ", LEVEL, INDENT, schema, node->schema->name);
        } else {
            ly_print(out, "%*s\"%s\": ", LEVEL, INDENT, node->schema->name);
        }
    }

    switch (leaf->value_type & LY_DATA_TYPE_MASK) {
    case LY_TYPE_BINARY:
    case LY_TYPE_STRING:
    case LY_TYPE_BITS:
    case LY_TYPE_ENUM:
    case LY_TYPE_IDENT:
    case LY_TYPE_INST:
        ly_print(out, "\"%s\"", leaf->value_str ? leaf->value_str : "");
        break;

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
        ly_print(out, "%s", leaf->value_str ? leaf->value_str : "null");
        break;

    case LY_TYPE_LEAFREF:
        json_print_leaf(out, level, leaf->value.leafref, 1);
        break;

    case LY_TYPE_EMPTY:
        ly_print(out, "[null]");
        break;

    default:
        /* error */
        ly_print(out, "\"(!error!)\"");
    }

    /* print attributes as sibling leaf */
    if (!onlyvalue && node->attr) {
        if (schema) {
            ly_print(out, ",\n%*s\"@%s:%s\": {\n", LEVEL, INDENT, schema, node->schema->name);
        } else {
            ly_print(out, ",\n%*s\"@%s\": {\n", LEVEL, INDENT, node->schema->name);
        }
        json_print_attrs(out, level + 1, node);
        ly_print(out, "%*s}", LEVEL, INDENT);
    }

    return;
}

static void
json_print_container(struct lyout *out, int level, const struct lyd_node *node)
{
    const char *schema;

    if (!node->parent || nscmp(node, node->parent)) {
        /* print "namespace" */
        if (node->schema->module->type) {
            /* submodule, get module */
            schema = ((struct lys_submodule *)node->schema->module)->belongsto->name;
        } else {
            schema = node->schema->module->name;
        }
        ly_print(out, "%*s\"%s:%s\": {\n", LEVEL, INDENT, schema, node->schema->name);
    } else {
        ly_print(out, "%*s\"%s\": {\n", LEVEL, INDENT, node->schema->name);
    }
    level++;
    if (node->attr) {
        ly_print(out, "%*s\"@\": {\n", LEVEL, INDENT);
        json_print_attrs(out, level + 1, node);
        ly_print(out, "%*s}%s", LEVEL, INDENT, node->child ? ",\n" : "");
    }
    json_print_nodes(out, level, node->child);
    level--;
    ly_print(out, "%*s}", LEVEL, INDENT);
}

static void
json_print_leaf_list(struct lyout *out, int level, const struct lyd_node *node, int is_list)
{
    const char *schema = NULL;
    const struct lyd_node *list = node;
    int flag_empty = 0, flag_attrs = 0;

    if (!list->child) {
        /* empty, e.g. in case of filter */
        flag_empty = 1;
    }

    if (!node->parent || nscmp(node, node->parent)) {
        /* print "namespace" */
        if (node->schema->module->type) {
            /* submodule, get module */
            schema = ((struct lys_submodule *)node->schema->module)->belongsto->name;
        } else {
            schema = node->schema->module->name;
        }
        ly_print(out, "%*s\"%s:%s\":", LEVEL, INDENT, schema, node->schema->name);
    } else {
        ly_print(out, "%*s\"%s\":", LEVEL, INDENT, node->schema->name);
    }

    if (flag_empty) {
        ly_print(out, " null");
        return;
    }
    ly_print(out, " [\n");

    if (!is_list) {
        ++level;
    }

    while (list) {
        if (is_list) {
            /* list print */
            ++level;
            ly_print(out, "%*s{\n", LEVEL, INDENT);
            ++level;
            if (list->attr) {
                ly_print(out, "%*s\"@\": {\n", LEVEL, INDENT);
                json_print_attrs(out, level + 1, node);
                ly_print(out, "%*s}%s", LEVEL, INDENT, list->child ? ",\n" : "");
            }
            json_print_nodes(out, level, list->child);
            --level;
            ly_print(out, "%*s}", LEVEL, INDENT);
            --level;
        } else {
            /* leaf-list print */
            ly_print(out, "%*s", LEVEL, INDENT);
            json_print_leaf(out, level, list, 1);
            if (list->attr) {
                flag_attrs = 1;
            }
        }
        for (list = list->next; list && list->schema != node->schema; list = list->next);
        if (list) {
            ly_print(out, ",\n");
        }
    }

    if (!is_list) {
        --level;
    }

    ly_print(out, "\n%*s]", LEVEL, INDENT);

    /* attributes */
    if (!is_list && flag_attrs) {
        if (schema) {
            ly_print(out, ",\n%*s\"@%s:%s\": [\n", LEVEL, INDENT, schema, node->schema->name);
        } else {
            ly_print(out, ",\n%*s\"@%s\": [\n", LEVEL, INDENT, node->schema->name);
        }
        level++;
        for (list = node; list; ) {
            if (list->attr) {
                ly_print(out, "%*s{ ", LEVEL, INDENT);
                json_print_attrs(out, 0, list);
                ly_print(out, "%*s}", LEVEL, INDENT);
            } else {
                ly_print(out, "%*snull", LEVEL, INDENT);
            }


            for (list = list->next; list && list->schema != node->schema; list = list->next);
            if (list) {
                ly_print(out, ",\n");
            }
        }
        level--;
        ly_print(out, "\n%*s]", LEVEL, INDENT);
    }
}

static void
json_print_anyxml(struct lyout *out, int level, const struct lyd_node *node)
{
    const char *schema = NULL;

    if (!node->parent || nscmp(node, node->parent)) {
        /* print "namespace" */
        if (node->schema->module->type) {
            /* submodule, get module */
            schema = ((struct lys_submodule *)node->schema->module)->belongsto->name;
        } else {
            schema = node->schema->module->name;
        }
        ly_print(out, "%*s\"%s:%s\": [null]", LEVEL, INDENT, schema, node->schema->name);
    } else {
        ly_print(out, "%*s\"%s\": [null]", LEVEL, INDENT, node->schema->name);
    }

    /* print attributes as sibling leaf */
    if (node->attr) {
        if (schema) {
            ly_print(out, ",\n%*s\"@%s:%s\": {\n", LEVEL, INDENT, schema, node->schema->name);
        } else {
            ly_print(out, ",\n%*s\"@%s\": {\n", LEVEL, INDENT, node->schema->name);
        }
        json_print_attrs(out, level + 1, node);
        ly_print(out, "%*s}", LEVEL, INDENT);
    }
}

void
json_print_nodes(struct lyout *out, int level, const struct lyd_node *root)
{
    const struct lyd_node *node, *iter;

    LY_TREE_FOR(root, node) {
        switch (node->schema->nodetype) {
        case LYS_RPC:
        case LYS_NOTIF:
        case LYS_CONTAINER:
            if (node->prev->next) {
                /* print the previous comma */
                ly_print(out, ",\n");
            }
            json_print_container(out, level, node);
            break;
        case LYS_LEAF:
            if (node->prev->next) {
                /* print the previous comma */
                ly_print(out, ",\n");
            }
            json_print_leaf(out, level, node, 0);
            break;
        case LYS_LEAFLIST:
        case LYS_LIST:
            /* is it already printed? */
            for (iter = node->prev; iter->next; iter = iter->prev) {
                if (iter == node) {
                    continue;
                }
                if (iter->schema == node->schema) {
                    /* the list has alread some previous instance and therefore it is already printed */
                    break;
                }
            }
            if (!iter->next) {
                if (node->prev->next) {
                    /* print the previous comma */
                    ly_print(out, ",\n");
                }

                /* print the list/leaflist */
                json_print_leaf_list(out, level, node, node->schema->nodetype == LYS_LIST ? 1 : 0);
            }
            break;
        case LYS_ANYXML:
            if (node->prev->next) {
                /* print the previous comma */
                ly_print(out, ",\n");
            }
            json_print_anyxml(out, level, node);
            break;
        default:
            LOGINT;
            break;
        }
    }
    ly_print(out, "\n");
}

int
json_print_data(struct lyout *out, const struct lyd_node *root)
{
    int level = 0;

    /* start */
    ly_print(out, "{\n");

    /* content */
    json_print_nodes(out, level + 1, root);

    /* end */
    ly_print(out, "}\n");

    return EXIT_SUCCESS;
}
