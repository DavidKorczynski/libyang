/**
 * @file int8.c
 * @author Radek Iša <isa@cesnet.cz>
 * @brief test for int8 values
 *
 * Copyright (c) 2020 CESNET, z.s.p.o.
 *
 * This source code is licensed under BSD 3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://opensource.org/licenses/BSD-3-Clause
 */

#define  _UTEST_MAIN_
#include "../utests.h"

/* GLOBAL INCLUDE HEADERS */
#include <ctype.h>

/* LOCAL INCLUDE HEADERS */
#include "libyang.h"
#include "path.h"

#define MODULE_CREATE_YIN(MOD_NAME, NODES) \
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" \
    "<module name=\"" MOD_NAME "\"\n" \
    "        xmlns=\"urn:ietf:params:xml:ns:yang:yin:1\"\n" \
    "        xmlns:pref=\"urn:tests:" MOD_NAME "\">\n" \
    "  <yang-version value=\"1.1\"/>\n" \
    "  <namespace uri=\"urn:tests:" MOD_NAME "\"/>\n" \
    "  <prefix value=\"pref\"/>\n" \
    NODES \
    "</module>\n"

#define MODULE_CREATE_YANG(MOD_NAME, NODES) \
    "module " MOD_NAME " {\n" \
    "  yang-version 1.1;\n" \
    "  namespace \"urn:tests:" MOD_NAME "\";\n" \
    "  prefix pref;\n" \
    NODES \
    "}\n"

#define TEST_SUCCESS_XML(MOD_NAME, DATA, TYPE, ...)\
    {\
        struct lyd_node *tree;\
        const char *data = "<port xmlns=\"urn:tests:" MOD_NAME "\">" DATA "</port>";\
        CHECK_PARSE_LYD_PARAM(data, LYD_XML, 0, LYD_VALIDATE_PRESENT, LY_SUCCESS, tree);\
        CHECK_LYSC_NODE(tree->schema, NULL, 0, 0x5, 1, "port", 0, LYS_LEAF, 0, 0, 0, 0);\
        CHECK_LYD_NODE_TERM((struct lyd_node_term *) tree, 0, 0, 0, 0, 1, TYPE, ## __VA_ARGS__);\
        lyd_free_all(tree);\
    }

#define TEST_SUCCESS_JSON(MOD_NAME, DATA, TYPE, ...)\
    {\
        struct lyd_node *tree;\
        const char *data = "{\"" MOD_NAME ":port\":" DATA "}";\
        CHECK_PARSE_LYD_PARAM(data, LYD_JSON, 0, LYD_VALIDATE_PRESENT, LY_SUCCESS, tree);\
        CHECK_LYSC_NODE(tree->schema, NULL, 0, 0x5, 1, "port", 0, LYS_LEAF, 0, 0, 0, 0);\
        CHECK_LYD_NODE_TERM((struct lyd_node_term *) tree, 0, 0, 0, 0, 1, TYPE, ## __VA_ARGS__);\
        lyd_free_all(tree);\
    }

#define TEST_ERROR_XML(MOD_NAME, DATA)\
    {\
        struct lyd_node *tree;\
        const char *data = "<port xmlns=\"urn:tests:" MOD_NAME "\">" DATA "</port>";\
        CHECK_PARSE_LYD_PARAM(data, LYD_XML, 0, LYD_VALIDATE_PRESENT, LY_EVALID, tree);\
        assert_null(tree);\
    }

#define TEST_ERROR_JSON(MOD_NAME, DATA)\
    {\
        struct lyd_node *tree;\
        const char *data = "{\"" MOD_NAME ":port\":" DATA "}";\
        CHECK_PARSE_LYD_PARAM(data, LYD_JSON, 0, LYD_VALIDATE_PRESENT, LY_EVALID, tree);\
        assert_null(tree);\
    }

static void
test_schema_yang(void **state)
{
    const char *schema;
    const struct lys_module *mod;
    struct lysc_node_leaf *lysc_leaf;
    struct lysp_node_leaf *lysp_leaf;
    struct lysc_pattern *pattern;
    struct lysc_range *range;

    /* TEST BASE STRING */
    schema = MODULE_CREATE_YANG("base", "leaf port {type string;}");
    UTEST_ADD_MODULE(schema, LYS_IN_YANG, NULL, &mod);
    assert_non_null(mod);
    lysc_leaf = (void *) mod->compiled->data;
    CHECK_LYSC_NODE_LEAF(lysc_leaf, NULL, 0, 0x5, 1, "port", 0, 0, 0, NULL, 0, 0, NULL, NULL);
    CHECK_LYSC_TYPE_NUM((struct lysc_type_num *)lysc_leaf->type, LY_TYPE_STRING, 0, 0);
    lysp_leaf = (void *) mod->parsed->data;
    CHECK_LYSP_NODE_LEAF(lysp_leaf, NULL, 0, 0x0, 0, "port", 0, 0, NULL, 0, 0, NULL, NULL);
    CHECK_LYSP_TYPE(&(lysp_leaf->type), 0, 0, 0, 0, 0, 0x0, 0, 0, "string", 0, 0, 1, 0, 0, 0);

    /* TEST MODULE T0 */
    schema = MODULE_CREATE_YANG("T0", "leaf port {type string"
            "{length \"10 .. max\";}"
            "}");
    UTEST_ADD_MODULE(schema, LYS_IN_YANG, NULL, &mod);
    assert_non_null(mod);
    lysc_leaf = (void *) mod->compiled->data;
    CHECK_LYSC_NODE_LEAF(lysc_leaf, NULL, 0, 0x5, 1, "port", 0, 0, 0, NULL, 0, 0, NULL, NULL);
    CHECK_LYSC_TYPE_STR((struct lysc_type_str *)lysc_leaf->type, 0, 1, 0);
    range = ((struct lysc_type_str *)lysc_leaf->type)->length;
    CHECK_LYSC_RANGE(range, NULL, NULL, NULL, 0, 1, NULL);
    assert_int_equal(range->parts[0].min_u64, 10);
    assert_true(range->parts[0].max_u64 == 18446744073709551615);
    lysp_leaf = (void *) mod->parsed->data;
    CHECK_LYSP_NODE_LEAF(lysp_leaf, NULL, 0, 0x0, 0, "port", 0, 0, NULL, 0, 0, NULL, NULL);
    CHECK_LYSP_TYPE(&(lysp_leaf->type), 0, 0, 0, 0, 0, 0x10, 0, 1, "string", 0, 0, 1, 0, 0, 0);
    CHECK_LYSP_RESTR(lysp_leaf->type.length, "10 .. max", NULL, NULL, NULL, 0, NULL);

    /* TEST MODULE T1 */
    schema = MODULE_CREATE_YANG("T1", "leaf port {type string"
            "{length \"min .. 20 | 50\";}"
            "}");
    UTEST_ADD_MODULE(schema, LYS_IN_YANG, NULL, &mod);
    assert_non_null(mod);
    lysc_leaf = (void *) mod->compiled->data;
    CHECK_LYSC_NODE_LEAF(lysc_leaf, NULL, 0, 0x5, 1, "port", 0, 0, 0, NULL, 0, 0, NULL, NULL);
    CHECK_LYSC_TYPE_STR((struct lysc_type_str *)lysc_leaf->type, 0, 1, 0);
    range = ((struct lysc_type_str *)lysc_leaf->type)->length;
    CHECK_LYSC_RANGE(range, NULL, NULL, NULL, 0, 2, NULL);
    assert_int_equal(range->parts[0].min_u64, 0);
    assert_int_equal(range->parts[0].max_u64, 20);
    assert_int_equal(range->parts[1].min_u64, 50);
    assert_int_equal(range->parts[1].max_u64, 50);
    lysp_leaf = (void *) mod->parsed->data;
    CHECK_LYSP_NODE_LEAF(lysp_leaf, NULL, 0, 0x0, 0, "port", 0, 0, NULL, 0, 0, NULL, NULL);
    CHECK_LYSP_TYPE(&(lysp_leaf->type), 0, 0, 0, 0, 0, 0x10, 0, 1, "string", 0, 0, 1, 0, 0, 0);
    CHECK_LYSP_RESTR(lysp_leaf->type.length, "min .. 20 | 50", NULL, NULL, NULL, 0, NULL);

    // TEST MODULE T2
    schema = MODULE_CREATE_YANG("T2", "leaf port {type string"
            "{length \"10 .. 20 | 50 .. 100 | 255\";}"
            "}");
    UTEST_ADD_MODULE(schema, LYS_IN_YANG, NULL, &mod);
    assert_non_null(mod);
    lysc_leaf = (void *) mod->compiled->data;
    CHECK_LYSC_NODE_LEAF(lysc_leaf, NULL, 0, 0x5, 1, "port", 0, 0, 0, NULL, 0, 0, NULL, NULL);
    CHECK_LYSC_TYPE_STR((struct lysc_type_str *)lysc_leaf->type, 0, 1, 0);
    range = ((struct lysc_type_str *)lysc_leaf->type)->length;
    CHECK_LYSC_RANGE(range, NULL, NULL, NULL, 0, 3, NULL);
    assert_int_equal(range->parts[0].min_u64, 10);
    assert_int_equal(range->parts[0].max_u64, 20);
    assert_int_equal(range->parts[1].min_u64, 50);
    assert_int_equal(range->parts[1].max_u64, 100);
    assert_int_equal(range->parts[2].min_u64, 255);
    assert_int_equal(range->parts[2].max_u64, 255);
    lysp_leaf = (void *) mod->parsed->data;
    CHECK_LYSP_NODE_LEAF(lysp_leaf, NULL, 0, 0x0, 0, "port", 0, 0, NULL, 0, 0, NULL, NULL);
    CHECK_LYSP_TYPE(&(lysp_leaf->type), 0, 0, 0, 0, 0, 0x10, 0, 1, "string", 0, 0, 1, 0, 0, 0);
    CHECK_LYSP_RESTR(lysp_leaf->type.length, "10 .. 20 | 50 .. 100 | 255", NULL, NULL, NULL, 0, NULL);

    // SUBTYPE MODULE T2
    schema = MODULE_CREATE_YANG("TS0",
            "typedef my_type {"
            "    type string {length \"10 .. 20 | 50 .. 100 | 255\";}"
            "}"
            "leaf port {type my_type {length \"min .. 15 | max\";}}");
    UTEST_ADD_MODULE(schema, LYS_IN_YANG, NULL, &mod);
    assert_non_null(mod);
    lysc_leaf = (void *) mod->compiled->data;
    CHECK_LYSC_NODE_LEAF(lysc_leaf, NULL, 0, 0x5, 1, "port", 0, 0, 0, NULL, 0, 0, NULL, NULL);
    CHECK_LYSC_TYPE_STR((struct lysc_type_str *)lysc_leaf->type, 0, 1, 0);
    range = ((struct lysc_type_str *)lysc_leaf->type)->length;
    CHECK_LYSC_RANGE(range, NULL, NULL, NULL, 0, 2, NULL);
    assert_int_equal(range->parts[0].min_u64, 10);
    assert_int_equal(range->parts[0].max_u64, 15);
    assert_int_equal(range->parts[1].min_u64, 255);
    assert_int_equal(range->parts[1].max_u64, 255);
    lysp_leaf = (void *) mod->parsed->data;
    CHECK_LYSP_NODE_LEAF(lysp_leaf, NULL, 0, 0x0, 0, "port", 0, 0, NULL, 0, 0, NULL, NULL);
    CHECK_LYSP_TYPE(&(lysp_leaf->type), 0, 0, 0, 0, 0, 0x10, 0, 1, "my_type", 0, 0, 1, 0, 0, 0);
    CHECK_LYSP_RESTR(lysp_leaf->type.length, "min .. 15 | max", NULL, NULL, NULL, 0, NULL);

    // ERROR TESTS NEGATIVE VALUE
    schema = MODULE_CREATE_YANG("ERR0", "leaf port {type string {length \"-1 .. 20\";}}");
    UTEST_INVALID_MODULE(schema, LYS_IN_YANG, NULL, LY_EVALID);
    CHECK_LOG_CTX("Invalid length restriction - value \"-1\" does not fit the type limitations.",
            "/ERR0:port");

    schema = MODULE_CREATE_YANG("ERR1", "leaf port {type string {length \"100 .. 18446744073709551616\";}}");
    UTEST_INVALID_MODULE(schema, LYS_IN_YANG, NULL, LY_EVALID);
    CHECK_LOG_CTX("Invalid length restriction - invalid value \"18446744073709551616\".",
            "/ERR1:port");

    schema = MODULE_CREATE_YANG("ERR2", "leaf port {type string {length \"10 .. 20 | 20 .. 30\";}}");
    UTEST_INVALID_MODULE(schema, LYS_IN_YANG, NULL, LY_EVALID);
    CHECK_LOG_CTX("Invalid length restriction - values are not in ascending order (20).",
            "/ERR2:port");

    schema = MODULE_CREATE_YANG("ERR3",
            "typedef my_type {"
            "    type string;"
            "}"
            "leaf port {type my_type {length \"-1 .. 15\";}}");
    UTEST_INVALID_MODULE(schema, LYS_IN_YANG, NULL, LY_EVALID);
    CHECK_LOG_CTX("Invalid length restriction - value \"-1\" does not fit the type limitations.",
            "/ERR3:port");

    /////////////////////////////
    // PATTERN
    schema = MODULE_CREATE_YANG("TPATTERN_0", "leaf port {type string"
            "{pattern '[a-zA-Z_][a-zA-Z0-9\\-_.]*';}"
            "}");
    UTEST_ADD_MODULE(schema, LYS_IN_YANG, NULL, &mod);
    assert_non_null(mod);
    lysc_leaf = (void *) mod->compiled->data;
    CHECK_LYSC_NODE_LEAF(lysc_leaf, NULL, 0, 0x5, 1, "port", 0, 0, 0, NULL, 0, 0, NULL, NULL);
    CHECK_LYSC_TYPE_STR((struct lysc_type_str *)lysc_leaf->type, 0, 0, 1);
    pattern = ((struct lysc_type_str *)lysc_leaf->type)->patterns[0];
    CHECK_LYSC_PATTERN(pattern, NULL, NULL, NULL, "[a-zA-Z_][a-zA-Z0-9\\-_.]*", 0, 0, NULL);
    lysp_leaf = (void *) mod->parsed->data;
    CHECK_LYSP_NODE_LEAF(lysp_leaf, NULL, 0, 0x0, 0, "port", 0, 0, NULL, 0, 0, NULL, NULL);
    CHECK_LYSP_TYPE(&(lysp_leaf->type), 0, 0, 0, 0, 0, 0x40, 0, 0, "string", 0, 1, 1, 0, 0, 0);
    // CHECK_LYSP_RESTR(&(lysp_leaf->type.patterns[0]), "[a-zA-Z_][a-zA-Z0-9\\-_.]*", NULL, NULL, NULL, 0, NULL);

    schema = MODULE_CREATE_YANG("TPATTERN_1", "leaf port {type string{"
            "   pattern '[a-zA-Z_][a-zA-Z0-9\\-_.]*' ;"
            "   pattern 'abc.*' {modifier invert-match;}"
            "}}");
    UTEST_ADD_MODULE(schema, LYS_IN_YANG, NULL, &mod);
    assert_non_null(mod);
    lysc_leaf = (void *) mod->compiled->data;
    CHECK_LYSC_NODE_LEAF(lysc_leaf, NULL, 0, 0x5, 1, "port", 0, 0, 0, NULL, 0, 0, NULL, NULL);
    CHECK_LYSC_TYPE_STR((struct lysc_type_str *)lysc_leaf->type, 0, 0, 2);
    pattern = ((struct lysc_type_str *)lysc_leaf->type)->patterns[0];
    CHECK_LYSC_PATTERN(pattern, NULL, NULL, NULL, "[a-zA-Z_][a-zA-Z0-9\\-_.]*", 0, 0, NULL);
    pattern = ((struct lysc_type_str *)lysc_leaf->type)->patterns[1];
    CHECK_LYSC_PATTERN(pattern, NULL, NULL, NULL, "abc.*", 0, 0x1, NULL);
    lysp_leaf = (void *) mod->parsed->data;
    CHECK_LYSP_NODE_LEAF(lysp_leaf, NULL, 0, 0x0, 0, "port", 0, 0, NULL, 0, 0, NULL, NULL);
    CHECK_LYSP_TYPE(&(lysp_leaf->type), 0, 0, 0, 0, 0, 0x40, 0, 0, "string", 0, 2, 1, 0, 0, 0);
    // CHECK_LYSP_RESTR(&(lysp_leaf->type.patterns[0]), "[a-zA-Z_][a-zA-Z0-9\\-_.]*", NULL, NULL, NULL, 0, NULL);
    // CHECK_LYSP_RESTR(&(lysp_leaf->type.patterns[1]), "abc.*", NULL, NULL, NULL, 0, NULL);

    schema = MODULE_CREATE_YANG("TPATTERN_2",
            "typedef my_type {"
            "   type string{"
            "       pattern '[a-zA-Z_][a-zA-Z0-9\\-_.]*' ;"
            "       pattern 'abc.*' {modifier invert-match;}"
            "}}"
            "leaf port {type my_type {pattern 'bcd.*';}}");
    UTEST_ADD_MODULE(schema, LYS_IN_YANG, NULL, &mod);
    assert_non_null(mod);
    lysc_leaf = (void *) mod->compiled->data;
    CHECK_LYSC_NODE_LEAF(lysc_leaf, NULL, 0, 0x5, 1, "port", 0, 0, 0, NULL, 0, 0, NULL, NULL);
    CHECK_LYSC_TYPE_STR((struct lysc_type_str *)lysc_leaf->type, 0, 0, 3);
    pattern = ((struct lysc_type_str *)lysc_leaf->type)->patterns[0];
    CHECK_LYSC_PATTERN(pattern, NULL, NULL, NULL, "[a-zA-Z_][a-zA-Z0-9\\-_.]*", 0, 0, NULL);
    pattern = ((struct lysc_type_str *)lysc_leaf->type)->patterns[1];
    CHECK_LYSC_PATTERN(pattern, NULL, NULL, NULL, "abc.*", 0, 0x1, NULL);
    pattern = ((struct lysc_type_str *)lysc_leaf->type)->patterns[2];
    CHECK_LYSC_PATTERN(pattern, NULL, NULL, NULL, "bcd.*", 0, 0x0, NULL);
    lysp_leaf = (void *) mod->parsed->data;
    CHECK_LYSP_NODE_LEAF(lysp_leaf, NULL, 0, 0x0, 0, "port", 0, 0, NULL, 0, 0, NULL, NULL);
    CHECK_LYSP_TYPE(&(lysp_leaf->type), 0, 0, 0, 0, 0, 0x40, 0, 0, "my_type", 0, 1, 1, 0, 0, 0);
    // CHECK_LYSP_RESTR(&(lysp_leaf->type.patterns[0]), "bcd.*", NULL, NULL, NULL, 0, NULL);

    ////////////////////////////
    // TEST pattern error
    schema = MODULE_CREATE_YANG("TPATTERN_ERR_0", "leaf port {type string {"
            "pattern '[a-zA-Z_[a-zA-Z0-9\\-_.*';"
            "}}");
    UTEST_INVALID_MODULE(schema, LYS_IN_YANG, NULL, LY_EVALID);
    CHECK_LOG_CTX("Regular expression \"[a-zA-Z_[a-zA-Z0-9\\-_.*\" is not valid (\"\": missing terminating ] for character class).",
            "/TPATTERN_ERR_0:port");

    ////////////////////////////
    // DEFAUT VALUE
    // "       default \"a10i-j\";"
    schema = MODULE_CREATE_YANG("TDEFAULT_0",
            "typedef my_type {"
            "   type string{"
            "       pattern \"[a-zA-Z_][a-zA-Z0-9\\\\-_.]*\";"
            "       length  \"2 .. 5 | 10\";"
            "   }"
            "   default \"a1i-j\";"
            "}"
            "leaf port {type my_type;}");
    UTEST_ADD_MODULE(schema, LYS_IN_YANG, NULL, &mod);
    assert_non_null(mod);
    lysc_leaf = (void *) mod->compiled->data;
    CHECK_LYSC_NODE_LEAF(lysc_leaf, NULL, 0, 0x5, 1, "port", 0, 0, 0, NULL, 0, 0, NULL, "a1i-j");
    CHECK_LYSC_TYPE_STR((struct lysc_type_str *)lysc_leaf->type, 0, 1, 1);
    pattern = ((struct lysc_type_str *)lysc_leaf->type)->patterns[0];
    CHECK_LYSC_PATTERN(pattern, NULL, NULL, NULL, "[a-zA-Z_][a-zA-Z0-9\\-_.]*", 0, 0, NULL);
    range = ((struct lysc_type_str *)lysc_leaf->type)->length;
    CHECK_LYSC_RANGE(range, NULL, NULL, NULL, 0, 2, NULL);
    assert_int_equal(range->parts[0].min_u64, 2);
    assert_int_equal(range->parts[0].max_u64, 5);
    assert_int_equal(range->parts[1].min_u64, 10);
    assert_int_equal(range->parts[1].max_u64, 10);
    lysp_leaf = (void *) mod->parsed->data;
    CHECK_LYSP_NODE_LEAF(lysp_leaf, NULL, 0, 0x0, 0, "port", 0, 0, NULL, 0, 0, NULL, NULL);
    CHECK_LYSP_TYPE(&(lysp_leaf->type), 0, 0, 0, 0, 0, 0x0, 0, 0, "my_type", 0, 0, 1, 0, 0, 0);
}

static void
test_schema_yin(void **state)
{
    const char *schema;
    const struct lys_module *mod;
    struct lysc_node_leaf *lysc_leaf;
    struct lysp_node_leaf *lysp_leaf;
    struct lysc_pattern *pattern;
    struct lysc_range *range;

    /* TEST BASE STRING */
    schema = MODULE_CREATE_YIN("base", "<leaf name=\"port\"> <type name=\"string\"/> </leaf>");
    UTEST_ADD_MODULE(schema, LYS_IN_YIN, NULL, &mod);
    assert_non_null(mod);
    lysc_leaf = (void *) mod->compiled->data;
    CHECK_LYSC_NODE_LEAF(lysc_leaf, NULL, 0, 0x5, 1, "port", 0, 0, 0, NULL, 0, 0, NULL, NULL);
    CHECK_LYSC_TYPE_NUM((struct lysc_type_num *)lysc_leaf->type, LY_TYPE_STRING, 0, 0);
    lysp_leaf = (void *) mod->parsed->data;
    CHECK_LYSP_NODE_LEAF(lysp_leaf, NULL, 0, 0x0, 0, "port", 0, 0, NULL, 0, 0, NULL, NULL);
    CHECK_LYSP_TYPE(&(lysp_leaf->type), 0, 0, 0, 0, 0, 0x0, 0, 0, "string", 0, 0, 1, 0, 0, 0);

    /* TEST MODULE T0 */
    schema = MODULE_CREATE_YIN("T0", "<leaf name=\"port\"> <type name=\"string\">"
            "<length value=\"10 .. max\"/>"
            "</type> </leaf>");
    UTEST_ADD_MODULE(schema, LYS_IN_YIN, NULL, &mod);
    assert_non_null(mod);
    lysc_leaf = (void *) mod->compiled->data;
    CHECK_LYSC_NODE_LEAF(lysc_leaf, NULL, 0, 0x5, 1, "port", 0, 0, 0, NULL, 0, 0, NULL, NULL);
    CHECK_LYSC_TYPE_STR((struct lysc_type_str *)lysc_leaf->type, 0, 1, 0);
    range = ((struct lysc_type_str *)lysc_leaf->type)->length;
    CHECK_LYSC_RANGE(range, NULL, NULL, NULL, 0, 1, NULL);
    assert_int_equal(range->parts[0].min_u64, 10);
    assert_true(range->parts[0].max_u64 == 18446744073709551615);
    lysp_leaf = (void *) mod->parsed->data;
    CHECK_LYSP_NODE_LEAF(lysp_leaf, NULL, 0, 0x0, 0, "port", 0, 0, NULL, 0, 0, NULL, NULL);
    CHECK_LYSP_TYPE(&(lysp_leaf->type), 0, 0, 0, 0, 0, 0x10, 0, 1, "string", 0, 0, 1, 0, 0, 0);
    CHECK_LYSP_RESTR(lysp_leaf->type.length, "10 .. max", NULL, NULL, NULL, 0, NULL);

    /* TEST MODULE T1 */
    schema = MODULE_CREATE_YIN("T1", "<leaf name=\"port\"> <type name=\"string\">"
            "  <length value=\"min .. 20 | 50\"/>"
            "</type></leaf>");
    UTEST_ADD_MODULE(schema, LYS_IN_YIN, NULL, &mod);
    assert_non_null(mod);
    lysc_leaf = (void *) mod->compiled->data;
    CHECK_LYSC_NODE_LEAF(lysc_leaf, NULL, 0, 0x5, 1, "port", 0, 0, 0, NULL, 0, 0, NULL, NULL);
    CHECK_LYSC_TYPE_STR((struct lysc_type_str *)lysc_leaf->type, 0, 1, 0);
    range = ((struct lysc_type_str *)lysc_leaf->type)->length;
    CHECK_LYSC_RANGE(range, NULL, NULL, NULL, 0, 2, NULL);
    assert_int_equal(range->parts[0].min_u64, 0);
    assert_int_equal(range->parts[0].max_u64, 20);
    assert_int_equal(range->parts[1].min_u64, 50);
    assert_int_equal(range->parts[1].max_u64, 50);
    lysp_leaf = (void *) mod->parsed->data;
    CHECK_LYSP_NODE_LEAF(lysp_leaf, NULL, 0, 0x0, 0, "port", 0, 0, NULL, 0, 0, NULL, NULL);
    CHECK_LYSP_TYPE(&(lysp_leaf->type), 0, 0, 0, 0, 0, 0x10, 0, 1, "string", 0, 0, 1, 0, 0, 0);
    CHECK_LYSP_RESTR(lysp_leaf->type.length, "min .. 20 | 50", NULL, NULL, NULL, 0, NULL);

    /* TEST MODULE T2 */
    schema = MODULE_CREATE_YIN("T2", "<leaf name=\"port\"> <type name=\"string\">"
            "<length value=\"10 .. 20 | 50 .. 100 | 255\"/>"
            "</type></leaf>");
    UTEST_ADD_MODULE(schema, LYS_IN_YIN, NULL, &mod);
    assert_non_null(mod);
    lysc_leaf = (void *) mod->compiled->data;
    CHECK_LYSC_NODE_LEAF(lysc_leaf, NULL, 0, 0x5, 1, "port", 0, 0, 0, NULL, 0, 0, NULL, NULL);
    CHECK_LYSC_TYPE_STR((struct lysc_type_str *)lysc_leaf->type, 0, 1, 0);
    range = ((struct lysc_type_str *)lysc_leaf->type)->length;
    CHECK_LYSC_RANGE(range, NULL, NULL, NULL, 0, 3, NULL);
    assert_int_equal(range->parts[0].min_u64, 10);
    assert_int_equal(range->parts[0].max_u64, 20);
    assert_int_equal(range->parts[1].min_u64, 50);
    assert_int_equal(range->parts[1].max_u64, 100);
    assert_int_equal(range->parts[2].min_u64, 255);
    assert_int_equal(range->parts[2].max_u64, 255);
    lysp_leaf = (void *) mod->parsed->data;
    CHECK_LYSP_NODE_LEAF(lysp_leaf, NULL, 0, 0x0, 0, "port", 0, 0, NULL, 0, 0, NULL, NULL);
    CHECK_LYSP_TYPE(&(lysp_leaf->type), 0, 0, 0, 0, 0, 0x10, 0, 1, "string", 0, 0, 1, 0, 0, 0);
    CHECK_LYSP_RESTR(lysp_leaf->type.length, "10 .. 20 | 50 .. 100 | 255", NULL, NULL, NULL, 0, NULL);

    /* SUBTYPE MODULE T2 */
    schema = MODULE_CREATE_YIN("TS0",
            "<typedef name=\"my_type\">"
            "    <type name=\"string\"> <length value=\"10 .. 20 | 50 .. 100 | 255\"/> </type>"
            "</typedef>"
            "<leaf name=\"port\"> <type name=\"my_type\">"
            "    <length value=\"min .. 15 | max\"/>"
            "</type> </leaf>");
    UTEST_ADD_MODULE(schema, LYS_IN_YIN, NULL, &mod);
    assert_non_null(mod);
    lysc_leaf = (void *) mod->compiled->data;
    CHECK_LYSC_NODE_LEAF(lysc_leaf, NULL, 0, 0x5, 1, "port", 0, 0, 0, NULL, 0, 0, NULL, NULL);
    CHECK_LYSC_TYPE_STR((struct lysc_type_str *)lysc_leaf->type, 0, 1, 0);
    range = ((struct lysc_type_str *)lysc_leaf->type)->length;
    CHECK_LYSC_RANGE(range, NULL, NULL, NULL, 0, 2, NULL);
    assert_int_equal(range->parts[0].min_u64, 10);
    assert_int_equal(range->parts[0].max_u64, 15);
    assert_int_equal(range->parts[1].min_u64, 255);
    assert_int_equal(range->parts[1].max_u64, 255);
    lysp_leaf = (void *) mod->parsed->data;
    CHECK_LYSP_NODE_LEAF(lysp_leaf, NULL, 0, 0x0, 0, "port", 0, 0, NULL, 0, 0, NULL, NULL);
    CHECK_LYSP_TYPE(&(lysp_leaf->type), 0, 0, 0, 0, 0, 0x10, 0, 1, "my_type", 0, 0, 1, 0, 0, 0);
    CHECK_LYSP_RESTR(lysp_leaf->type.length, "min .. 15 | max", NULL, NULL, NULL, 0, NULL);

    /* ERROR TESTS NEGATIVE VALUE */
    schema = MODULE_CREATE_YIN("ERR0", "<leaf name=\"port\"> <type name=\"string\">"
            "<length value =\"-1 .. 20\"/> </type></leaf>");
    UTEST_INVALID_MODULE(schema, LYS_IN_YIN, NULL, LY_EVALID);
    CHECK_LOG_CTX("Invalid length restriction - value \"-1\" does not fit the type limitations.",
            "/ERR0:port");

    schema = MODULE_CREATE_YIN("ERR1", "<leaf name=\"port\"> <type name=\"string\">"
            "<length value=\"100 .. 18446744073709551616\"/>"
            "</type> </leaf>");
    UTEST_INVALID_MODULE(schema, LYS_IN_YIN, NULL, LY_EVALID);
    CHECK_LOG_CTX("Invalid length restriction - invalid value \"18446744073709551616\".",
            "/ERR1:port");

    schema = MODULE_CREATE_YIN("ERR2", "<leaf name=\"port\">"
            "<type name=\"string\"> <length value=\"10 .. 20 | 20 .. 30\"/>"
            "</type> </leaf>");
    UTEST_INVALID_MODULE(schema, LYS_IN_YIN, NULL, LY_EVALID);
    CHECK_LOG_CTX("Invalid length restriction - values are not in ascending order (20).",
            "/ERR2:port");

    schema = MODULE_CREATE_YIN("ERR3",
            "<typedef name=\"my_type\"> <type name=\"string\"/> </typedef>"
            "<leaf name=\"port\"> <type name=\"my_type\"> <length value=\"-1 .. 15\"/>"
            "</type> </leaf>");
    UTEST_INVALID_MODULE(schema, LYS_IN_YIN, NULL, LY_EVALID);
    CHECK_LOG_CTX("Invalid length restriction - value \"-1\" does not fit the type limitations.",
            "/ERR3:port");

    /*
     * PATTERN
     */
    schema = MODULE_CREATE_YIN("TPATTERN_0", "<leaf name=\"port\"> <type name=\"string\">"
            "<pattern value=\"[a-zA-Z_][a-zA-Z0-9\\-_.]*\"/>"
            "</type> </leaf>");
    UTEST_ADD_MODULE(schema, LYS_IN_YIN, NULL, &mod);
    assert_non_null(mod);
    lysc_leaf = (void *) mod->compiled->data;
    CHECK_LYSC_NODE_LEAF(lysc_leaf, NULL, 0, 0x5, 1, "port", 0, 0, 0, NULL, 0, 0, NULL, NULL);
    CHECK_LYSC_TYPE_STR((struct lysc_type_str *)lysc_leaf->type, 0, 0, 1);
    pattern = ((struct lysc_type_str *)lysc_leaf->type)->patterns[0];
    CHECK_LYSC_PATTERN(pattern, NULL, NULL, NULL, "[a-zA-Z_][a-zA-Z0-9\\-_.]*", 0, 0, NULL);
    lysp_leaf = (void *) mod->parsed->data;
    CHECK_LYSP_NODE_LEAF(lysp_leaf, NULL, 0, 0x0, 0, "port", 0, 0, NULL, 0, 0, NULL, NULL);
    CHECK_LYSP_TYPE(&(lysp_leaf->type), 0, 0, 0, 0, 0, 0x40, 0, 0, "string", 0, 1, 1, 0, 0, 0);
    // CHECK_LYSP_RESTR(&(lysp_leaf->type.patterns[0]), "[a-zA-Z_][a-zA-Z0-9\\-_.]*", NULL, NULL, NULL, 0, NULL);

    schema = MODULE_CREATE_YIN("TPATTERN_1", "<leaf name=\"port\"> <type name=\"string\">"
            "   <pattern value=\"[a-zA-Z_][a-zA-Z0-9\\-_.]*\"/>"
            "   <pattern value=\"abc.*\"> <modifier value=\"invert-match\"/> </pattern>"
            "</type> </leaf>");
    UTEST_ADD_MODULE(schema, LYS_IN_YIN, NULL, &mod);
    assert_non_null(mod);
    lysc_leaf = (void *) mod->compiled->data;
    CHECK_LYSC_NODE_LEAF(lysc_leaf, NULL, 0, 0x5, 1, "port", 0, 0, 0, NULL, 0, 0, NULL, NULL);
    CHECK_LYSC_TYPE_STR((struct lysc_type_str *)lysc_leaf->type, 0, 0, 2);
    pattern = ((struct lysc_type_str *)lysc_leaf->type)->patterns[0];
    CHECK_LYSC_PATTERN(pattern, NULL, NULL, NULL, "[a-zA-Z_][a-zA-Z0-9\\-_.]*", 0, 0, NULL);
    pattern = ((struct lysc_type_str *)lysc_leaf->type)->patterns[1];
    CHECK_LYSC_PATTERN(pattern, NULL, NULL, NULL, "abc.*", 0, 0x1, NULL);
    lysp_leaf = (void *) mod->parsed->data;
    CHECK_LYSP_NODE_LEAF(lysp_leaf, NULL, 0, 0x0, 0, "port", 0, 0, NULL, 0, 0, NULL, NULL);
    CHECK_LYSP_TYPE(&(lysp_leaf->type), 0, 0, 0, 0, 0, 0x40, 0, 0, "string", 0, 2, 1, 0, 0, 0);
    // CHECK_LYSP_RESTR(&(lysp_leaf->type.patterns[0]), "[a-zA-Z_][a-zA-Z0-9\\-_.]*", NULL, NULL, NULL, 0, NULL);
    // CHECK_LYSP_RESTR(&(lysp_leaf->type.patterns[1]), "abc.*", NULL, NULL, NULL, 0, NULL);

    schema = MODULE_CREATE_YIN("TPATTERN_2",
            "<typedef name=\"my_type\">"
            "   <type name=\"string\">"
            "       <pattern value=\"[a-zA-Z_][a-zA-Z0-9\\-_.]*\"/>"
            "       <pattern value=\"abc.*\"> <modifier value=\"invert-match\"/> </pattern>"
            "</type> </typedef>"
            "<leaf name=\"port\"><type name=\"my_type\"> <pattern value=\"bcd.*\"/> </type></leaf>");
    UTEST_ADD_MODULE(schema, LYS_IN_YIN, NULL, &mod);
    assert_non_null(mod);
    lysc_leaf = (void *) mod->compiled->data;
    CHECK_LYSC_NODE_LEAF(lysc_leaf, NULL, 0, 0x5, 1, "port", 0, 0, 0, NULL, 0, 0, NULL, NULL);
    CHECK_LYSC_TYPE_STR((struct lysc_type_str *)lysc_leaf->type, 0, 0, 3);
    pattern = ((struct lysc_type_str *)lysc_leaf->type)->patterns[0];
    CHECK_LYSC_PATTERN(pattern, NULL, NULL, NULL, "[a-zA-Z_][a-zA-Z0-9\\-_.]*", 0, 0, NULL);
    pattern = ((struct lysc_type_str *)lysc_leaf->type)->patterns[1];
    CHECK_LYSC_PATTERN(pattern, NULL, NULL, NULL, "abc.*", 0, 0x1, NULL);
    pattern = ((struct lysc_type_str *)lysc_leaf->type)->patterns[2];
    CHECK_LYSC_PATTERN(pattern, NULL, NULL, NULL, "bcd.*", 0, 0x0, NULL);
    lysp_leaf = (void *) mod->parsed->data;
    CHECK_LYSP_NODE_LEAF(lysp_leaf, NULL, 0, 0x0, 0, "port", 0, 0, NULL, 0, 0, NULL, NULL);
    CHECK_LYSP_TYPE(&(lysp_leaf->type), 0, 0, 0, 0, 0, 0x40, 0, 0, "my_type", 0, 1, 1, 0, 0, 0);
    // CHECK_LYSP_RESTR(&(lysp_leaf->type.patterns[0]), "bcd.*", NULL, NULL, NULL, 0, NULL);

    /*
     * TEST pattern error
     * */
    schema = MODULE_CREATE_YIN("TPATTERN_ERR_0",
            "<leaf name=\"port\"> <type name=\"string\">"
            "   <pattern value=\"[a-zA-Z_][a-zA-Z0-9\\-_.*\"/>"
            "</type> </leaf>");
    UTEST_INVALID_MODULE(schema, LYS_IN_YIN, NULL, LY_EVALID);
    CHECK_LOG_CTX("Regular expression \"[a-zA-Z_][a-zA-Z0-9\\-_.*\" is not valid (\"\": missing terminating ] for character class).",
            "/TPATTERN_ERR_0:port");

    /*
     * DEFAUT VALUE
     */
    schema = MODULE_CREATE_YIN("TDEFAULT_0",
            "<typedef name=\"my_type\">"
            "   <type name=\"string\">"
            "       <pattern value=\"[a-zA-Z_][a-zA-Z0-9\\-_.]*\"/>"
            "       <length  value=\"2 .. 5 | 10\"/>"
            "   </type>"
            "   <default value=\"a1i-j\"/>"
            "</typedef>"
            "<leaf name=\"port\"> <type name=\"my_type\"/> </leaf>");
    UTEST_ADD_MODULE(schema, LYS_IN_YIN, NULL, &mod);
    assert_non_null(mod);
    lysc_leaf = (void *) mod->compiled->data;
    CHECK_LYSC_NODE_LEAF(lysc_leaf, NULL, 0, 0x5, 1, "port", 0, 0, 0, NULL, 0, 0, NULL, "a1i-j");
    CHECK_LYSC_TYPE_STR((struct lysc_type_str *)lysc_leaf->type, 0, 1, 1);
    pattern = ((struct lysc_type_str *)lysc_leaf->type)->patterns[0];
    CHECK_LYSC_PATTERN(pattern, NULL, NULL, NULL, "[a-zA-Z_][a-zA-Z0-9\\-_.]*", 0, 0, NULL);
    range = ((struct lysc_type_str *)lysc_leaf->type)->length;
    CHECK_LYSC_RANGE(range, NULL, NULL, NULL, 0, 2, NULL);
    assert_int_equal(range->parts[0].min_u64, 2);
    assert_int_equal(range->parts[0].max_u64, 5);
    assert_int_equal(range->parts[1].min_u64, 10);
    assert_int_equal(range->parts[1].max_u64, 10);
    lysp_leaf = (void *) mod->parsed->data;
    CHECK_LYSP_NODE_LEAF(lysp_leaf, NULL, 0, 0x0, 0, "port", 0, 0, NULL, 0, 0, NULL, NULL);
    CHECK_LYSP_TYPE(&(lysp_leaf->type), 0, 0, 0, 0, 0, 0x0, 0, 0, "my_type", 0, 0, 1, 0, 0, 0);
}

static void
test_schema_print(void **state)
{
    const char *schema_yang, *schema_yin;
    char *printed;
    const struct lys_module *mod;

    // test print yang to yin
    schema_yang = MODULE_CREATE_YANG("PRINT0",
            "leaf port {type string {"
            "length \"min .. 20 | 50\";"
            "pattern '[a-zA-Z_[a-zA-Z0-9\\-_.]*';"
            "}}");
    schema_yin = MODULE_CREATE_YIN("PRINT0",
            "  <leaf name=\"port\">\n"
            "    <type name=\"string\">\n"
            "      <length value=\"min .. 20 | 50\"/>\n"
            "      <pattern value=\"[a-zA-Z_[a-zA-Z0-9\\-_.]*\"/>\n"
            "    </type>\n"
            "  </leaf>\n");

    UTEST_ADD_MODULE(schema_yang, LYS_IN_YANG, NULL, &mod);
    assert_non_null(mod);
    assert_int_equal(LY_SUCCESS, lys_print_mem(&printed, mod, LYS_OUT_YIN, 0));
    assert_string_equal(printed, schema_yin);
    free(printed);

    // test print yin to yang
    schema_yang = MODULE_CREATE_YANG("PRINT1",
            "  leaf port {\n"
            "    type string {\n"
            "      length \"min .. 20 | 50\";\n"
            "      pattern \"[a-zA-Z_[a-zA-Z0-9\\\\-_.]*\";\n"
            "    }\n"
            "  }\n");
    schema_yin = MODULE_CREATE_YIN("PRINT1",
            "<leaf name=\"port\">"
            "    <type name=\"string\">"
            "        <length value=\"min .. 20 | 50\"/>"
            "        <pattern value=\"[a-zA-Z_[a-zA-Z0-9\\-_.]*\"/>"
            "    </type>"
            "</leaf>");

    UTEST_ADD_MODULE(schema_yin, LYS_IN_YIN, NULL, &mod);
    assert_non_null(mod);
    assert_int_equal(LY_SUCCESS, lys_print_mem(&printed, mod, LYS_OUT_YANG, 0));
    assert_string_equal(printed, schema_yang);
    free(printed);
}

static void
test_data_xml(void **state)
{
    const char *schema;
    struct lyd_node *tree;
    const char *data;
    struct lysc_node_container *lysc_root;
    struct lyd_node_inner *lyd_root;

    /* TEST INVERTED PATTERN ADN LENGTH */
    schema = MODULE_CREATE_YANG("defs", "leaf port {type string {"
            "       length  \"5 .. 10 | 20\";"
            "       pattern '[a-zA-Z_][a-zA-Z0-9\\-_.]*' ;"
            "       pattern 'p4.*' {modifier invert-match;}"
            "}}");
    UTEST_ADD_MODULE(schema, LYS_IN_YANG, NULL, NULL);
    /* inverted value */
    TEST_SUCCESS_XML("defs", "abcde", STRING, "abcde");
    TEST_ERROR_XML("defs", "p4abc");
    CHECK_LOG_CTX("String \"p4abc\" does not conform to the inverted pattern \"p4.*\".",
            "Schema location /defs:port, line number 1.");
    /* size 20 */
    TEST_SUCCESS_XML("defs", "ahojahojahojahojahoj", STRING, "ahojahojahojahojahoj");
    TEST_SUCCESS_XML("defs", "abc-de", STRING, "abc-de");

    /* TEST DEFAULT VALUE */
    schema = MODULE_CREATE_YANG("T1",
            "container cont {\n"
            "    leaf port {type string {length \"0 .. 50 | 105\";} default \"test\";}"
            "}");
    /* using default value */
    UTEST_ADD_MODULE(schema, LYS_IN_YANG, NULL, NULL);
    data = "<cont xmlns=\"urn:tests:" "T1" "\">"  "</cont>";
    CHECK_PARSE_LYD_PARAM(data, LYD_XML, 0, LYD_VALIDATE_PRESENT, LY_SUCCESS, tree);
    lysc_root = (void *)tree->schema;
    CHECK_LYSC_NODE(lysc_root->child, NULL, 0, 0x205, 1, "port", 0, LYS_LEAF, 1, 0, 0, 0);
    lyd_root = ((struct lyd_node_inner *) tree);
    CHECK_LYD_NODE_TERM((struct lyd_node_term *) lyd_root->child, 1, 0, 0, 1, 1,
            STRING, "test");\
    lyd_free_all(tree);

    /* rewriting default value */
    UTEST_ADD_MODULE(schema, LYS_IN_YANG, NULL, NULL);
    data = "<cont xmlns=\"urn:tests:T1\">" "<port> 52 </port>" "</cont>";
    CHECK_PARSE_LYD_PARAM(data, LYD_XML, 0, LYD_VALIDATE_PRESENT, LY_SUCCESS, tree);
    lysc_root = (void *)tree->schema;
    CHECK_LYSC_NODE(lysc_root->child, NULL, 0, 0x205, 1, "port", 0, LYS_LEAF, 1, 0, 0, 0);
    lyd_root = ((struct lyd_node_inner *) tree);
    CHECK_LYD_NODE_TERM((struct lyd_node_term *) lyd_root->child, 0, 0, 0, 1, 1,
            STRING, " 52 ");
    lyd_free_all(tree);

    /* NO RESTRICTION TESTS */
    schema = MODULE_CREATE_YANG("T2", "leaf port {type string;}");
    UTEST_ADD_MODULE(schema, LYS_IN_YANG, NULL, NULL);
    /* space on start and new line */
    TEST_SUCCESS_XML("T2", " 50  \n\t 64", STRING, " 50  \n\t 64");
    /* nuber as string */
    TEST_SUCCESS_XML("T2", "50", STRING, "50");
    TEST_SUCCESS_XML("T2", "+250", STRING, "+250");
    /* references */
    TEST_SUCCESS_XML("T2", "&quot;", STRING, "\"");
    TEST_SUCCESS_XML("T2", "|&amp;|", STRING, "|&|");
    TEST_SUCCESS_XML("T2", "&apos;", STRING, "'");
    TEST_SUCCESS_XML("T2", "&lt;", STRING, "<");
    TEST_SUCCESS_XML("T2", "&gt;", STRING, ">");
    /* special characters */
    TEST_SUCCESS_XML("T2", "\"", STRING, "\"");
    TEST_SUCCESS_XML("T2", "'", STRING, "'");
    TEST_SUCCESS_XML("T2", ">", STRING, ">");

    TEST_SUCCESS_XML("T2", "<![CDATA[<greeting>Hello, world! & Wecome</greeting>]]>", STRING,
            "<greeting>Hello, world! & Wecome</greeting>");
    TEST_SUCCESS_XML("T2", "this isn't <!--' this is comment '--> comment",
            STRING, "this isn't comment");
    /* ERROR end of CDATA withnout begining */
    // TODO: TEST NA TISk => PORUŠUJE NORMU!!!
    TEST_ERROR_XML("T2", "]]>");
    CHECK_LOG_CTX("", "");
    TEST_ERROR_XML("T2", "< df");
    CHECK_LOG_CTX("Child element \"df\" inside a terminal node \"port\" found.",
            "Schema location /T2:port, data location /T2:port, line number 1.");
    TEST_ERROR_XML("T2", "&text;");
    CHECK_LOG_CTX("Entity reference \"&text;</po\" not supported, only predefined references allowed.", "Line number 1.");


    // The string data type can contain characters, line feeds, carriage returns, and tab characters
    /* UTF-8 */

    /* ERROR LENGTH  */

    /* ALIGMENT TEST */

    /* DEFAULT VALUE */

}

static void
test_data_json(void **state)
{
    (void) state;
    const char *schema;

// struct lyd_node *tree;
// const char *data;

    schema = MODULE_CREATE_YANG("defs", "leaf port {type int8 {range \"0 .. 50 | 105\";}}");
    UTEST_ADD_MODULE(schema, LYS_IN_YANG, NULL, NULL);

    TEST_SUCCESS_JSON("defs", "50", INT8, "50", 50);
    TEST_SUCCESS_JSON("defs", "50", INT8, "50", 50);
    TEST_SUCCESS_JSON("defs", "105", INT8, "105", 105);
    TEST_SUCCESS_JSON("defs", "0", INT8, "0", 0);
    TEST_SUCCESS_JSON("defs", "-0", INT8, "0", 0);
    TEST_ERROR_JSON("defs", "-1");
    CHECK_LOG_CTX("Value \"-1\" does not satisfy the range constraint.",
            "Schema location /defs:port, line number 1.");
    TEST_ERROR_JSON("defs", "51");
    CHECK_LOG_CTX("Value \"51\" does not satisfy the range constraint.",
            "Schema location /defs:port, line number 1.");
    TEST_ERROR_JSON("defs", "106");
    CHECK_LOG_CTX("Value \"106\" does not satisfy the range constraint.",
            "Schema location /defs:port, line number 1.");
    TEST_ERROR_JSON("defs", "104");
    CHECK_LOG_CTX("Value \"104\" does not satisfy the range constraint.",
            "Schema location /defs:port, line number 1.");
    TEST_ERROR_JSON("defs", "60");
    CHECK_LOG_CTX("Value \"60\" does not satisfy the range constraint.",
            "Schema location /defs:port, line number 1.");

    schema = MODULE_CREATE_YANG("T0", "leaf port {type int8; }");
    UTEST_ADD_MODULE(schema, LYS_IN_YANG, NULL, NULL);
    TEST_SUCCESS_JSON("T0", "-128", INT8, "-128", -128);
    TEST_SUCCESS_JSON("T0", "-100", INT8, "-100", -100);
    TEST_SUCCESS_JSON("T0", "0", INT8, "0", 0);
    TEST_SUCCESS_JSON("T0", "10", INT8, "10", 10);
    TEST_SUCCESS_JSON("T0", "50", INT8, "50", 50);
    TEST_SUCCESS_JSON("T0", "127", INT8, "127", 127);
    // leading zeros
    TEST_ERROR_JSON("T0", "015");
    TEST_ERROR_JSON("T0", "-015");
    TEST_ERROR_JSON("defs", "+50");
    TEST_ERROR_JSON("T0", "-129");
    CHECK_LOG_CTX("Value is out of int8's min/max bounds.",
            "Schema location /T0:port, line number 1.");
    TEST_ERROR_JSON("T0", "128");
    CHECK_LOG_CTX("Value is out of int8's min/max bounds.",
            "Schema location /T0:port, line number 1.");
    TEST_ERROR_JSON("T0", "256");
    CHECK_LOG_CTX("Value is out of int8's min/max bounds.",
            "Schema location /T0:port, line number 1.");
    TEST_ERROR_JSON("T0", "1024");
    CHECK_LOG_CTX("Value is out of int8's min/max bounds.",
            "Schema location /T0:port, line number 1.");

    // check if there isnt default value then crash
    // default value
    struct lysc_node_container *lysc_root;
    struct lyd_node_inner *lyd_root;
    const char *data;
    struct lyd_node *tree;

    schema = MODULE_CREATE_YANG("T1",
            "container cont {\n"
            "    leaf port {type int8 {range \"0 .. 50 | 105\";} default \"20\";}"
            "}");
    // check using default value
    UTEST_ADD_MODULE(schema, LYS_IN_YANG, NULL, NULL);
    data = "{\"T1:cont\":{}}";
    CHECK_PARSE_LYD_PARAM(data, LYD_JSON, 0, LYD_VALIDATE_PRESENT, LY_SUCCESS, tree);
    lysc_root = (void *)tree->schema;
    CHECK_LYSC_NODE(lysc_root->child, NULL, 0, 0x205, 1, "port", 0, LYS_LEAF, 1, 0, 0, 0);
    lyd_root = ((struct lyd_node_inner *) tree);
    CHECK_LYD_NODE_TERM((struct lyd_node_term *) lyd_root->child, 1, 0, 0, 1, 1,
            INT8, "20", 20);\
    lyd_free_all(tree);

    // check rewriting default value
    UTEST_ADD_MODULE(schema, LYS_IN_YANG, NULL, NULL);
    data = "{\"T1:cont\":{\":port\":30}}";
    CHECK_PARSE_LYD_PARAM(data, LYD_JSON, 0, LYD_VALIDATE_PRESENT, LY_SUCCESS, tree);
    lysc_root = (void *)tree->schema;
    CHECK_LYSC_NODE(lysc_root->child, NULL, 0, 0x205, 1, "port", 0, LYS_LEAF, 1, 0, 0, 0);
    lyd_root = ((struct lyd_node_inner *) tree);
    CHECK_LYD_NODE_TERM((struct lyd_node_term *) lyd_root->child, 0, 0, 0, 1, 1,
            INT8, "30", 30);
    lyd_free_all(tree);

}

static void
test_diff(void **state)
{
    (void) state;
    const char *schema;

    schema = MODULE_CREATE_YANG("defs", "leaf port {type int8 {range \"0 .. 50 | 120\";}}");
    UTEST_ADD_MODULE(schema, LYS_IN_YANG, NULL, NULL);

    struct lyd_node *model_1, *model_2;
    struct lyd_node *diff;
    const char *data_1 = "<port xmlns=\"urn:tests:defs\"> 5 </port>";
    const char *data_2 = "<port xmlns=\"urn:tests:defs\"> 6 </port>";
    const char *diff_expected = "<port xmlns=\"urn:tests:defs\" "
            "xmlns:yang=\"urn:ietf:params:xml:ns:yang:1\" "
            "yang:operation=\"replace\" yang:orig-default=\"false\" yang:orig-value=\"5\">"
            "6</port>";

    CHECK_PARSE_LYD_PARAM(data_1, LYD_XML, 0, LYD_VALIDATE_PRESENT, LY_SUCCESS, model_1);
    CHECK_PARSE_LYD_PARAM(data_2, LYD_XML, 0, LYD_VALIDATE_PRESENT, LY_SUCCESS, model_2);
    assert_int_equal(LY_SUCCESS, lyd_diff_siblings(model_1, model_2, 0, &diff));
    assert_non_null(diff);
    CHECK_LYD_STRING_PARAM(diff, diff_expected, LYD_XML, LYD_PRINT_WITHSIBLINGS | LYD_PRINT_SHRINK);
    assert_int_equal(LY_SUCCESS, lyd_diff_apply_all(&model_1, diff));
    CHECK_LYD(model_1, model_2);
    lyd_free_all(model_1);
    lyd_free_all(model_2);
    lyd_free_all(diff);

    // create data from diff
    diff_expected = "<port xmlns=\"urn:tests:defs\" "
            "xmlns:yang=\"urn:ietf:params:xml:ns:yang:1\" "
            "yang:operation=\"replace\" yang:orig-default=\"false\" yang:orig-value=\"5\">"
            "120</port>";
    CHECK_PARSE_LYD_PARAM(diff_expected, LYD_XML, 0, LYD_VALIDATE_PRESENT, LY_SUCCESS, diff);
    data_1 = "<port xmlns=\"urn:tests:defs\"> 5 </port>";
    CHECK_PARSE_LYD_PARAM(data_1, LYD_XML, 0, LYD_VALIDATE_PRESENT, LY_SUCCESS, model_1);
    assert_int_equal(LY_SUCCESS, lyd_diff_apply_all(&model_1, diff));
    const char *expected = "<port xmlns=\"urn:tests:defs\">120</port>";

    CHECK_LYD_STRING_PARAM(model_1, expected, LYD_XML, LYD_PRINT_WITHSIBLINGS | LYD_PRINT_SHRINK);
    lyd_free_all(model_1);
    lyd_free_all(diff);

    //////////////////////////////////
    // check creating data out of range
    diff_expected = "<port xmlns=\"urn:tests:defs\" "
            "xmlns:yang=\"urn:ietf:params:xml:ns:yang:1\" "
            "yang:operation=\"replace\" yang:orig-default=\"false\" yang:orig-value=\"5\">"
            "121</port>";
    CHECK_PARSE_LYD_PARAM(diff_expected, LYD_XML, 0, LYD_VALIDATE_PRESENT, LY_EVALID, model_1);
    CHECK_LOG_CTX("Value \"121\" does not satisfy the range constraint.",
            "Schema location /defs:port, line number 1.");

    //////////////////////////////
    // diff from default value
    data_1 = "<cont xmlns=\"urn:tests:T0\"></cont>";
    data_2 = "<cont xmlns=\"urn:tests:T0\"> <port> 6 </port> </cont>";
    diff_expected = "<cont xmlns=\"urn:tests:T0\""
            " xmlns:yang=\"urn:ietf:params:xml:ns:yang:1\""
            " yang:operation=\"create\"><port>6</port></cont>";

    schema = MODULE_CREATE_YANG("T0",
            "container cont {\n"
            "    leaf port {type int8; default \"20\";}"
            "}");
    UTEST_ADD_MODULE(schema, LYS_IN_YANG, NULL, NULL);
    CHECK_PARSE_LYD_PARAM(data_1, LYD_XML, 0, LYD_VALIDATE_PRESENT, LY_SUCCESS, model_1);
    CHECK_PARSE_LYD_PARAM(data_2, LYD_XML, 0, LYD_VALIDATE_PRESENT, LY_SUCCESS, model_2);
    assert_int_equal(LY_SUCCESS, lyd_diff_siblings(model_1, model_2, 0, &diff));
    assert_non_null(diff);
    CHECK_LYD_STRING_PARAM(diff, diff_expected, LYD_XML, LYD_PRINT_WITHSIBLINGS | LYD_PRINT_SHRINK);
    assert_int_equal(LY_SUCCESS, lyd_diff_apply_all(&model_1, diff));
    CHECK_LYD(model_1, model_2);
    lyd_free_all(diff);

    lyd_free_all(model_1);
    lyd_free_all(model_2);
}

static void
test_print(void **state)
{
    (void) state;

    const char *schema = MODULE_CREATE_YANG("defs", "leaf port {type int8 {range \"0 .. 50\";}}");

    UTEST_ADD_MODULE(schema, LYS_IN_YANG, NULL, NULL);

    struct lyd_node *model_1;
    const char *data_1 = "<port xmlns=\"urn:tests:defs\"> 50 </port>";

    CHECK_PARSE_LYD_PARAM(data_1, LYD_XML, 0, LYD_VALIDATE_PRESENT, LY_SUCCESS, model_1);

    // XML
    const char *expected_xml = "<port xmlns=\"urn:tests:defs\">50</port>";

    CHECK_LYD_STRING_PARAM(model_1, expected_xml, LYD_XML, LYD_PRINT_SHRINK | LYD_PRINT_WITHSIBLINGS | LYD_PRINT_SHRINK);

    // JSON
    const char *expected_json = "{\"defs:port\":50}";

    CHECK_LYD_STRING_PARAM(model_1, expected_json, LYD_JSON, LYD_PRINT_SHRINK | LYD_PRINT_WITHSIBLINGS | LYD_PRINT_SHRINK);

    lyd_free_all(model_1);
}

static void
test_plugin_store(void **state)
{
    (void) state;

    const char *val_text = NULL;
    struct ly_err_item *err = NULL;
    const struct lys_module *mod;
    struct lyd_value value = {0};
    struct lysc_type_plugin *type = &(ly_builtin_type_plugins[LY_TYPE_INT8]);
    struct lysc_type *lysc_type;
    LY_ERR ly_ret;

    // create schema. Prepare common used variables
    const char *schema = MODULE_CREATE_YANG("defs", "leaf port {type int8 {range \"-50 .. 50\";}}");

    UTEST_ADD_MODULE(schema, LYS_IN_YANG, NULL, &mod);
    lysc_type = ((struct lysc_node_leaf *) mod->compiled->data)->type;

    // check proper type
    assert_int_equal(LY_TYPE_INT8, type->type);
    assert_string_equal("libyang 2 - integer, version 1", type->id);

    // check store
    // options = LY_TYPE_STORE_IMPLEMENT | LY_TYPE_STORE_DYNAMIC
    // hint    = LYD_VALHINT_DECNUM, LYD_VALHINT_HEXNUM, LYD_VALHINT_OCTNUM
    val_text = "20";
    assert_int_equal(LY_SUCCESS, type->store(UTEST_LYCTX, lysc_type, val_text, strlen(val_text),
            0, LY_PREF_XML, NULL, LYD_VALHINT_DECNUM, NULL, &value, NULL, &err));
    CHECK_LYD_VALUE(value, INT8, "20", 20);
    assert_ptr_equal(value.realtype, lysc_type);
    type->free(UTEST_LYCTX, &value);

    val_text = "-20";
    assert_int_equal(LY_SUCCESS, type->store(UTEST_LYCTX, lysc_type, val_text, strlen(val_text),
            0, LY_PREF_XML, NULL, LYD_VALHINT_DECNUM, NULL, &value, NULL, &err));
    CHECK_LYD_VALUE(value, INT8, "-20", -20);
    assert_ptr_equal(value.realtype, lysc_type);
    type->free(UTEST_LYCTX, &value);

    val_text = "0xf";
    ly_ret = type->store(UTEST_LYCTX, lysc_type, val_text, strlen(val_text),
            0, LY_PREF_XML, NULL, LYD_VALHINT_HEXNUM, NULL, &value, NULL, &err);
    assert_int_equal(LY_SUCCESS, ly_ret);
    CHECK_LYD_VALUE(value, INT8, "15", 15);
    assert_ptr_equal(value.realtype, lysc_type);
    type->free(UTEST_LYCTX, &value);

    val_text = "1B";
    ly_ret = type->store(UTEST_LYCTX, lysc_type, val_text, strlen(val_text),
            0, LY_PREF_XML, NULL, LYD_VALHINT_HEXNUM, NULL, &value, NULL, &err);
    assert_int_equal(LY_SUCCESS, ly_ret);
    CHECK_LYD_VALUE(value, INT8, "27", 27);
    assert_ptr_equal(value.realtype, lysc_type);
    type->free(UTEST_LYCTX, &value);

    val_text = "-0xf";
    ly_ret = type->store(UTEST_LYCTX, lysc_type, val_text, strlen(val_text),
            0, LY_PREF_XML, NULL, LYD_VALHINT_HEXNUM, NULL, &value, NULL, &err);
    assert_int_equal(LY_SUCCESS, ly_ret);
    CHECK_LYD_VALUE(value, INT8, "-15", -15);
    assert_ptr_equal(value.realtype, lysc_type);
    type->free(UTEST_LYCTX, &value);

    val_text = "027";
    ly_ret = type->store(UTEST_LYCTX, lysc_type, val_text, strlen(val_text),
            0, LY_PREF_XML, NULL, LYD_VALHINT_OCTNUM, NULL, &value, NULL, &err);
    assert_int_equal(LY_SUCCESS, ly_ret);
    CHECK_LYD_VALUE(value, INT8, "23", 23);
    assert_ptr_equal(value.realtype, lysc_type);
    type->free(UTEST_LYCTX, &value);

    val_text = "-027";
    ly_ret = type->store(UTEST_LYCTX, lysc_type, val_text, strlen(val_text),
            0, LY_PREF_XML, NULL, LYD_VALHINT_OCTNUM, NULL, &value, NULL, &err);
    assert_int_equal(LY_SUCCESS, ly_ret);
    CHECK_LYD_VALUE(value, INT8, "-23", -23);
    assert_ptr_equal(value.realtype, lysc_type);
    type->free(UTEST_LYCTX, &value);

    ///////////////////////////
    // minor tests
    // dynamic alocated input text
    val_text = "0xa";
    char *alloc = (char *) malloc(strlen(val_text) + 1);

    memcpy(alloc, val_text, strlen(val_text) + 1);
    ly_ret = type->store(UTEST_LYCTX, lysc_type, alloc, strlen(val_text),
            LY_TYPE_STORE_DYNAMIC, LY_PREF_XML, NULL, LYD_VALHINT_HEXNUM, NULL, &value, NULL, &err);
    alloc = NULL;
    assert_int_equal(LY_SUCCESS, ly_ret);
    CHECK_LYD_VALUE(value, INT8, "10", 10);
    type->free(UTEST_LYCTX, &value);

    // wrong lysc_type of value
    struct lysc_type lysc_type_test = *lysc_type;

    lysc_type_test.basetype = LY_TYPE_UINT8;
    val_text = "20";
    ly_ret = type->store(UTEST_LYCTX, &lysc_type_test, val_text, strlen(val_text),
            0, LY_PREF_XML, NULL, LYD_VALHINT_HEXNUM, NULL, &value, NULL, &err);
    assert_int_equal(LY_EINT, ly_ret);

    ///////////////////////////
    // ERROR TESTS
    val_text = "";
    err = NULL;
    ly_ret = type->store(UTEST_LYCTX, lysc_type, val_text, strlen(val_text),
            0, LY_PREF_XML, NULL, LYD_VALHINT_HEXNUM, NULL, &value, NULL, &err);
    assert_int_equal(LY_EVALID, ly_ret);
    ly_err_free(err);

    val_text = "";
    err = NULL;
    ly_ret = type->store(UTEST_LYCTX, lysc_type, val_text, 1,
            0, LY_PREF_XML, NULL, LYD_VALHINT_HEXNUM, NULL, &value, NULL, &err);
    assert_int_equal(LY_EVALID, ly_ret);
    ly_err_free(err);

    val_text = "10 b";
    err = NULL;
    ly_ret = type->store(UTEST_LYCTX, lysc_type, val_text, strlen(val_text),
            0, LY_PREF_XML, NULL, LYD_VALHINT_HEXNUM, NULL, &value, NULL, &err);
    assert_int_equal(LY_EVALID, ly_ret);
    ly_err_free(err);

    val_text = "a";
    err = NULL;
    ly_ret = type->store(UTEST_LYCTX, lysc_type, val_text, strlen(val_text),
            0, LY_PREF_XML, NULL, LYD_VALHINT_DECNUM, NULL, &value, NULL, &err);
    assert_int_equal(LY_EVALID, ly_ret);
    ly_err_free(err);
}

static void
test_plugin_compare(void **state)
{
    struct ly_err_item *err = NULL;
    const struct lys_module *mod;
    struct lyd_value values[10];
    struct lysc_type_plugin *type = &(ly_builtin_type_plugins[LY_TYPE_INT8]);
    struct lysc_type *lysc_type;
    LY_ERR ly_ret;
    const char *schema;

    // different type
    const char *diff_type_text = "20";
    struct lyd_value  diff_type_val;
    struct lysc_type *diff_type;

    // compare different type. same type but derivated..

    // create schema. Prepare common used variables
    schema = MODULE_CREATE_YANG("T0", "typedef my_int_type {type int8; }"
            "leaf p1 {type my_int_type;}"
            "leaf p2 {type my_int_type;}"
            "leaf p3 {type my_int_type{range \"0 .. 50\";}}"
            "leaf p4 {type uint8;}");
    UTEST_ADD_MODULE(schema, LYS_IN_YANG, NULL, &mod);
    lysc_type = ((struct lysc_node_leaf *) mod->compiled->data)->type;

    // CREATE VALUES
    const char *val_init[] = {"20", "30", "-30", "0", "-0", "20"};

    for (int unsigned it = 0; it < sizeof(val_init) / sizeof(val_init[0]); it++) {
        ly_ret = type->store(UTEST_LYCTX, lysc_type, val_init[it], strlen(val_init[it]),
                0, LY_PREF_XML, NULL, LYD_VALHINT_DECNUM, NULL, &(values[it]), NULL, &err);
        assert_int_equal(LY_SUCCESS, ly_ret);
    }

    ///////////////////////////////
    // BASIC TEST;
    assert_int_equal(LY_SUCCESS, type->compare(&(values[0]), &(values[0])));
    assert_int_equal(LY_SUCCESS, type->compare(&(values[0]), &(values[5])));
    assert_int_equal(LY_ENOT, type->compare(&(values[0]), &(values[1])));
    assert_int_equal(LY_ENOT, type->compare(&(values[1]), &(values[0])));
    assert_int_equal(LY_ENOT, type->compare(&(values[1]), &(values[2])));
    assert_int_equal(LY_SUCCESS, type->compare(&(values[3]), &(values[4])));

    ///////////////////////////////
    // SAME TYPE but different node
    diff_type_text = "20";
    diff_type = ((struct lysc_node_leaf *) mod->compiled->data->next)->type;
    ly_ret = diff_type->plugin->store(UTEST_LYCTX, diff_type, diff_type_text, strlen(diff_type_text),
            0, LY_PREF_XML, NULL, LYD_VALHINT_DECNUM, NULL, &diff_type_val, NULL, &err);
    assert_int_equal(LY_SUCCESS, ly_ret);
    assert_int_equal(diff_type->plugin->type, LY_TYPE_INT8);
    assert_int_equal(LY_SUCCESS, type->compare(&diff_type_val, &(values[0])));
    assert_int_equal(LY_ENOT, type->compare(&diff_type_val, &(values[1])));
    type->free(UTEST_LYCTX, &(diff_type_val));

    ////////////////////////////////
    // derivated type add some limitations
    diff_type_text = "20";
    diff_type = ((struct lysc_node_leaf *) mod->compiled->data->next->next)->type;
    ly_ret = diff_type->plugin->store(UTEST_LYCTX, diff_type, diff_type_text, strlen(diff_type_text),
            0, LY_PREF_XML, NULL, LYD_VALHINT_DECNUM, NULL, &diff_type_val, NULL, &err);
    assert_int_equal(LY_SUCCESS, ly_ret);
    assert_int_equal(diff_type->plugin->type, LY_TYPE_INT8);
    assert_int_equal(LY_ENOT, type->compare(&diff_type_val, &(values[0])));
    assert_int_equal(LY_ENOT, type->compare(&diff_type_val, &(values[1])));
    type->free(UTEST_LYCTX, &(diff_type_val));

    /////////////////////////////////
    // different type (UINT8)
    diff_type_text = "20";
    diff_type = ((struct lysc_node_leaf *) mod->compiled->data->next->next->next)->type;
    ly_ret = diff_type->plugin->store(UTEST_LYCTX, diff_type, diff_type_text, strlen(diff_type_text),
            0, LY_PREF_XML, NULL, LYD_VALHINT_DECNUM, NULL, &diff_type_val, NULL, &err);
    assert_int_equal(LY_SUCCESS, ly_ret);
    assert_int_equal(diff_type->plugin->type, LY_TYPE_UINT8);
    assert_int_equal(LY_ENOT, type->compare(&diff_type_val, &(values[0])));
    assert_int_equal(LY_ENOT, type->compare(&diff_type_val, &(values[1])));
    type->free(UTEST_LYCTX, &(diff_type_val));

    // delete values
    for (int unsigned it = 0; it < sizeof(val_init) / sizeof(val_init[0]); it++) {
        type->free(UTEST_LYCTX, &(values[it]));
    }
}

static void
test_plugin_print(void **state)
{
    struct ly_err_item *err = NULL;
    const struct lys_module *mod;
    struct lyd_value values[10];
    struct lysc_type_plugin *type = &(ly_builtin_type_plugins[LY_TYPE_INT8]);
    struct lysc_type *lysc_type;
    LY_ERR ly_ret;

    // create schema. Prepare common used variables
    const char *schema = MODULE_CREATE_YANG("defs", "leaf port {type int8;}");

    UTEST_ADD_MODULE(schema, LYS_IN_YANG, NULL, &mod);
    lysc_type = ((struct lysc_node_leaf *) mod->compiled->data)->type;

    // CREATE VALUES
    const char *val_init[] = {"20", "0x4A", "-f", "0", "-0", "-20"};

    for (int unsigned it = 0; it < sizeof(val_init) / sizeof(val_init[0]); it++) {
        ly_ret = type->store(UTEST_LYCTX, lysc_type, val_init[it], strlen(val_init[it]),
                0, LY_PREF_XML, NULL, LYD_VALHINT_HEXNUM, NULL, &(values[it]), NULL, &err);
        assert_int_equal(LY_SUCCESS, ly_ret);
    }

    // print value
    ly_bool dynamic = 0;

    assert_string_equal("32", type->print(&(values[0]), LY_PREF_XML, NULL, &dynamic));
    assert_string_equal("74", type->print(&(values[1]), LY_PREF_XML, NULL, &dynamic));
    assert_string_equal("-15", type->print(&(values[2]), LY_PREF_XML, NULL, &dynamic));
    assert_string_equal("0", type->print(&(values[3]), LY_PREF_XML, NULL, &dynamic));
    assert_string_equal("0", type->print(&(values[4]), LY_PREF_XML, NULL, &dynamic));
    assert_string_equal("-32", type->print(&(values[5]), LY_PREF_XML, NULL, &dynamic));

    for (int unsigned it = 0; it < sizeof(val_init) / sizeof(val_init[0]); it++) {
        type->free(UTEST_LYCTX, &(values[it]));
    }
}

static void
test_plugin_dup(void **state)
{

    struct ly_err_item *err = NULL;
    const struct lys_module *mod;
    struct lyd_value values[10];
    struct lysc_type_plugin *type = &(ly_builtin_type_plugins[LY_TYPE_INT8]);
    struct lysc_type *lysc_type[2];
    const char *schema;
    LY_ERR ly_ret;

    // create schema. Prepare common used variables
    schema = MODULE_CREATE_YANG("T0", "leaf port {type int8;}");
    UTEST_ADD_MODULE(schema, LYS_IN_YANG, NULL, &mod);
    lysc_type[0] = ((struct lysc_node_leaf *) mod->compiled->data)->type;

    schema = MODULE_CREATE_YANG("T1",
            "typedef my_int_type {"
            "    type int8 {range \"-100 .. 100\";} default 20;"
            "}"
            "leaf port {type my_int_type; }");
    UTEST_ADD_MODULE(schema, LYS_IN_YANG, NULL, &mod);
    lysc_type[1] = ((struct lysc_node_leaf *) mod->compiled->data)->type;

    // CREATE VALUES
    const char *val_init[] = {"20", "0x4A", "-f", "0", "-0x80", "-20"};

    for (int unsigned it = 0; it < sizeof(val_init) / sizeof(val_init[0]); it++) {
        ly_ret = type->store(UTEST_LYCTX, lysc_type[it % 2], val_init[it], strlen(val_init[it]),
                0, LY_PREF_XML, NULL, LYD_VALHINT_HEXNUM, NULL, &(values[it]), NULL, &err);
        assert_int_equal(LY_SUCCESS, ly_ret);
    }

    // print duplicate value
    struct lyd_value dup_value;

    assert_int_equal(LY_SUCCESS, type->duplicate(UTEST_LYCTX, &(values[0]), &dup_value));
    CHECK_LYD_VALUE(dup_value, INT8, "32", 0x20);
    assert_ptr_equal(dup_value.realtype, values[0].realtype);
    type->free(UTEST_LYCTX, &dup_value);

    assert_int_equal(LY_SUCCESS, type->duplicate(UTEST_LYCTX, &(values[1]), &dup_value));
    CHECK_LYD_VALUE(dup_value, INT8, "74", 0x4a);
    assert_ptr_equal(dup_value.realtype, values[1].realtype);
    type->free(UTEST_LYCTX, &dup_value);

    assert_int_equal(LY_SUCCESS, type->duplicate(UTEST_LYCTX, &(values[2]), &dup_value));
    CHECK_LYD_VALUE(dup_value, INT8, "-15", -0xf);
    assert_ptr_equal(dup_value.realtype, values[2].realtype);
    type->free(UTEST_LYCTX, &dup_value);

    assert_int_equal(LY_SUCCESS, type->duplicate(UTEST_LYCTX, &(values[3]), &dup_value));
    CHECK_LYD_VALUE(dup_value, INT8, "0", 0x0);
    assert_ptr_equal(dup_value.realtype, values[3].realtype);
    type->free(UTEST_LYCTX, &dup_value);

    assert_int_equal(LY_SUCCESS, type->duplicate(UTEST_LYCTX, &(values[4]), &dup_value));
    CHECK_LYD_VALUE(dup_value, INT8, "-128", -0x80);
    assert_ptr_equal(dup_value.realtype, values[4].realtype);
    type->free(UTEST_LYCTX, &dup_value);

    assert_int_equal(LY_SUCCESS, type->duplicate(UTEST_LYCTX, &(values[5]), &dup_value));
    CHECK_LYD_VALUE(dup_value, INT8, "-32", -0x20);
    assert_ptr_equal(dup_value.realtype, values[5].realtype);
    type->free(UTEST_LYCTX, &dup_value);

    // error tests
    assert_int_equal(LY_EINVAL, type->duplicate(NULL, &(values[0]), &dup_value));

    for (int unsigned it = 0; it < sizeof(val_init) / sizeof(val_init[0]); it++) {
        type->free(UTEST_LYCTX, &(values[it]));
    }
}

int
main(void)
{
    const struct CMUnitTest tests[] = {
        UTEST(test_schema_yang),
        UTEST(test_schema_yin),
        UTEST(test_schema_print),
        UTEST(test_data_xml),
// UTEST(test_data_json),
// UTEST(test_diff),
// UTEST(test_print),
//
// UTEST(test_plugin_store),
// UTEST(test_plugin_compare),
// UTEST(test_plugin_print),
// UTEST(test_plugin_dup),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
