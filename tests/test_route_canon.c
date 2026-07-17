/*
 * test_route_canon.c — Unit tests for cbm_route_canon_path().
 *
 * Verifies that framework-specific route-parameter placeholder syntaxes
 * (":id", "{id}", "<id>", "${id}") collapse to a single "{}" token so that a
 * client call site and a server handler rendezvous on the same Route QN
 * regardless of the language/framework that produced each side.
 * See src/pipeline/pass_route_nodes.c.
 */
#include "test_framework.h"
#include "pipeline/pipeline_internal.h"
#include "pipeline/route_node_classifiers.h"

#include <string.h>

TEST(route_canon_static_unchanged) {
    char b[128];
    ASSERT_STR_EQ(cbm_route_canon_path("/products/categories", b, sizeof(b)),
                  "/products/categories");
    PASS();
}

TEST(route_canon_colon_param) {
    char b[128];
    ASSERT_STR_EQ(cbm_route_canon_path("/players/:id", b, sizeof(b)), "/players/{}");
    PASS();
}

TEST(route_canon_brace_param) {
    char b[128];
    ASSERT_STR_EQ(cbm_route_canon_path("/players/{id}", b, sizeof(b)), "/players/{}");
    PASS();
}

/* The core invariant: Axum "{id}" and a JS client ":id" must converge. */
TEST(route_canon_colon_and_brace_converge) {
    char a[128];
    char c[128];
    cbm_route_canon_path("/clients/{id}/authorized-users", a, sizeof(a));
    cbm_route_canon_path("/clients/:clientId/authorized-users", c, sizeof(c));
    ASSERT_STR_EQ(a, c);
    ASSERT_STR_EQ(a, "/clients/{}/authorized-users");
    PASS();
}

/* Parameter names are intentionally discarded ("{id}" == ":requestId"). */
TEST(route_canon_param_name_agnostic) {
    char a[128];
    char c[128];
    cbm_route_canon_path("/link-requests/{id}/status", a, sizeof(a));
    cbm_route_canon_path("/link-requests/:requestId/status", c, sizeof(c));
    ASSERT_STR_EQ(a, c);
    PASS();
}

TEST(route_canon_angle_param) {
    char b[128];
    ASSERT_STR_EQ(cbm_route_canon_path("/users/<int:id>", b, sizeof(b)), "/users/{}");
    PASS();
}

TEST(route_canon_template_interpolation) {
    char b[128];
    ASSERT_STR_EQ(cbm_route_canon_path("/players/${playerId}", b, sizeof(b)), "/players/{}");
    PASS();
}

TEST(route_canon_multiple_params) {
    char b[128];
    ASSERT_STR_EQ(cbm_route_canon_path("/orders/{id}/items/{itemIndex}", b, sizeof(b)),
                  "/orders/{}/items/{}");
    PASS();
}

/* A ':' that is not at a segment start is literal, not a route parameter. */
TEST(route_canon_colon_mid_segment_is_literal) {
    char b[128];
    ASSERT_STR_EQ(cbm_route_canon_path("/a/b:c", b, sizeof(b)), "/a/b:c");
    PASS();
}

TEST(route_canon_null_and_empty) {
    char b[8];
    ASSERT_STR_EQ(cbm_route_canon_path("", b, sizeof(b)), "");
    ASSERT_STR_EQ(cbm_route_canon_path(NULL, b, sizeof(b)), "");
    PASS();
}

/* A tight output buffer must still yield a bounded, NUL-terminated string. */
TEST(route_canon_truncation_safe) {
    char b[6];
    const char *r = cbm_route_canon_path("/players/:id", b, sizeof(b));
    ASSERT(strlen(r) < sizeof(b));
    PASS();
}

TEST(route_node_hash_segment_classifier_contract) {
    static const unsigned char max_length[] = "123456789012";
    static const unsigned char over_max_length[] = "1234567890123";
    static const unsigned char short_letter_number[] = "a1";
    static const unsigned char meaningful_word[] = "api";
    static const unsigned char uppercase[] = "ABC";
    static const unsigned char punctuation[] = "a-1";
    static const unsigned char embedded_nul[] = {'1', '2', '\0', '3'};

    ASSERT(cbm_pipeline_is_hash_segment(NULL, 1) == 0);
    ASSERT(cbm_pipeline_is_hash_segment((const unsigned char *)"", 0) == 0);
    ASSERT(cbm_pipeline_is_hash_segment(max_length, sizeof(max_length) - 1) != 0);
    ASSERT(cbm_pipeline_is_hash_segment(over_max_length, sizeof(over_max_length) - 1) == 0);
    ASSERT(cbm_pipeline_is_hash_segment(short_letter_number, sizeof(short_letter_number) - 1) != 0);
    ASSERT(cbm_pipeline_is_hash_segment(meaningful_word, sizeof(meaningful_word) - 1) == 0);
    ASSERT(cbm_pipeline_is_hash_segment(uppercase, sizeof(uppercase) - 1) == 0);
    ASSERT(cbm_pipeline_is_hash_segment(punctuation, sizeof(punctuation) - 1) == 0);
    ASSERT(cbm_pipeline_is_hash_segment(embedded_nul, sizeof(embedded_nul)) == 0);
    PASS();
}

TEST(route_node_broker_route_classifier_contract) {
    static const char *const broker_prefixes[] = {
        "__route__infra__topic", "__route__pubsub__topic", "__route__cloud_tasks__topic",
        "__route__async__topic", "__route__cloud_scheduler__topic", "__route__kafka__topic",
        "__route__sqs__topic"};

    for (size_t i = 0; i < sizeof(broker_prefixes) / sizeof(broker_prefixes[0]); i++) {
        ASSERT(cbm_pipeline_is_broker_route(broker_prefixes[i]) != 0);
    }
    ASSERT(cbm_pipeline_is_broker_route("__route__GET__/users") == 0);
    ASSERT(cbm_pipeline_is_broker_route("__route__INFRA__topic") == 0);
    ASSERT(cbm_pipeline_is_broker_route("prefix__route__infra__topic") == 0);
    ASSERT(cbm_pipeline_is_broker_route(NULL) == 0);
    PASS();
}

SUITE(route_canon) {
    RUN_TEST(route_canon_static_unchanged);
    RUN_TEST(route_canon_colon_param);
    RUN_TEST(route_canon_brace_param);
    RUN_TEST(route_canon_colon_and_brace_converge);
    RUN_TEST(route_canon_param_name_agnostic);
    RUN_TEST(route_canon_angle_param);
    RUN_TEST(route_canon_template_interpolation);
    RUN_TEST(route_canon_multiple_params);
    RUN_TEST(route_canon_colon_mid_segment_is_literal);
    RUN_TEST(route_canon_null_and_empty);
    RUN_TEST(route_canon_truncation_safe);
    RUN_TEST(route_node_hash_segment_classifier_contract);
    RUN_TEST(route_node_broker_route_classifier_contract);
}
