#include <windows.h>
#include "json.h"
#include <stdio.h>

static int g_passed = 0;
static int g_failed = 0;

#define TEST(name, cond) do { \
    if (cond) { g_passed++; printf("[PASS] %s\n", name); } \
    else { g_failed++; printf("[FAIL] %s\n", name); } \
} while(0)

#define TEST_STREQ(a, b) (strcmp((a), (b)) == 0)

void test_basic_types() {
    printf("\n=== Test: Basic Types ===\n");

    JsonNode* n = Json_Parse("null");
    TEST("Parse null", n && n->type == JSON_NULL);
    Json_Free(n);

    n = Json_Parse("true");
    TEST("Parse true", n && n->type == JSON_BOOL && n->value.boolValue == 1);
    Json_Free(n);

    n = Json_Parse("false");
    TEST("Parse false", n && n->type == JSON_BOOL && n->value.boolValue == 0);
    Json_Free(n);

    n = Json_Parse("123");
    TEST("Parse integer", n && n->type == JSON_NUMBER && n->value.numberValue == 123.0);
    Json_Free(n);

    n = Json_Parse("-456.789");
    TEST("Parse negative float", n && n->type == JSON_NUMBER && n->value.numberValue == -456.789);
    Json_Free(n);

    n = Json_Parse("1.5e10");
    TEST("Parse scientific notation", n && n->type == JSON_NUMBER);
    Json_Free(n);

    n = Json_Parse("\"hello world\"");
    TEST("Parse string", n && n->type == JSON_STRING && TEST_STREQ(n->value.stringValue.data, "hello world"));
    Json_Free(n);

    n = Json_Parse("\"hello\\nworld\"");
    TEST("Parse escaped newline", n && n->type == JSON_STRING && n->value.stringValue.length == 11);
    Json_Free(n);

    n = Json_Parse("\"escape: \\\\\" \\\\/ \\\\b \\\\f \\\\n \\\\r \\\\t\"");
    TEST("Parse all escapes", n && n->type == JSON_STRING);
    Json_Free(n);

    n = Json_Parse("\"\\u0041\\u0042\\u0043\"");
    TEST("Parse unicode escapes (ASCII)", n && n->type == JSON_STRING && TEST_STREQ(n->value.stringValue.data, "ABC"));
    Json_Free(n);

    n = Json_Parse("\"\\u00E9\"");
    TEST("Parse unicode escape (UTF-8)", n && n->type == JSON_STRING);
    Json_Free(n);

    TEST("Parse invalid: abc", Json_Parse("abc") == NULL);
    TEST("Parse invalid: \"unclosed", Json_Parse("\"unclosed") == NULL);
    TEST("Parse invalid: {", Json_Parse("{") == NULL);
    TEST("Parse invalid: [", Json_Parse("[") == NULL);
}

void test_array() {
    printf("\n=== Test: Array ===\n");

    JsonNode* n = Json_Parse("[]");
    TEST("Parse empty array", n && n->type == JSON_ARRAY && Json_GetArraySize(n) == 0);
    Json_Free(n);

    n = Json_Parse("[1, 2, 3]");
    TEST("Parse array of numbers", n && n->type == JSON_ARRAY && Json_GetArraySize(n) == 3);
    if (n) {
        TEST("  item[0] == 1", Json_GetNumber(Json_GetArrayItem(n, 0), 0) == 1);
        TEST("  item[1] == 2", Json_GetNumber(Json_GetArrayItem(n, 1), 0) == 2);
        TEST("  item[2] == 3", Json_GetNumber(Json_GetArrayItem(n, 2), 0) == 3);
        Json_Free(n);
    }

    n = Json_Parse("[\"a\", \"b\", \"c\"]");
    TEST("Parse array of strings", n && n->type == JSON_ARRAY && Json_GetArraySize(n) == 3);
    if (n) {
        TEST("  item[0] == a", TEST_STREQ(Json_GetString(Json_GetArrayItem(n, 0), ""), "a"));
        TEST("  item[1] == b", TEST_STREQ(Json_GetString(Json_GetArrayItem(n, 1), ""), "b"));
        TEST("  item[2] == c", TEST_STREQ(Json_GetString(Json_GetArrayItem(n, 2), ""), "c"));
        Json_Free(n);
    }

    n = Json_Parse("[[1, 2], [3, 4]]");
    TEST("Parse nested array", n && n->type == JSON_ARRAY && Json_GetArraySize(n) == 2);
    if (n) {
        JsonNode* inner = Json_GetArrayItem(n, 0);
        TEST("  inner[0] size == 2", Json_GetArraySize(inner) == 2);
        TEST("  inner[0][0] == 1", Json_GetNumber(Json_GetArrayItem(inner, 0), 0) == 1);
        Json_Free(n);
    }

    n = Json_Parse("  [ 1 , 2 , 3 ]  ");
    TEST("Parse array with whitespace", n && n && Json_GetArraySize(n) == 3);
    Json_Free(n);

    TEST("Parse invalid: [1,]", Json_Parse("[1,]") == NULL);
    TEST("Parse invalid: [,1]", Json_Parse("[,1]") == NULL);
}

void test_object() {
    printf("\n=== Test: Object ===\n");

    JsonNode* n = Json_Parse("{}");
    TEST("Parse empty object", n && n->type == JSON_OBJECT && Json_GetObjectSize(n) == 0);
    Json_Free(n);

    n = Json_Parse("{\"name\": \"John\", \"age\": 30}");
    TEST("Parse simple object", n && n->type == JSON_OBJECT && Json_GetObjectSize(n) == 2);
    if (n) {
        TEST("  name == John", TEST_STREQ(Json_GetString(Json_GetObjectItem(n, "name"), ""), "John"));
        TEST("  age == 30", Json_GetNumber(Json_GetObjectItem(n, "age"), 0) == 30);
        Json_Free(n);
    }

    n = Json_Parse("{\"nested\": {\"key\": \"value\"}}");
    TEST("Parse nested object", n && n->type == JSON_OBJECT);
    if (n) {
        JsonNode* nested = Json_GetObjectItem(n, "nested");
        TEST("  nested.type == object", nested && nested->type == JSON_OBJECT);
        if (nested) {
            TEST("  nested.key == value", TEST_STREQ(Json_GetString(Json_GetObjectItem(nested, "key"), ""), "value"));
        }
        Json_Free(n);
    }

    n = Json_Parse("{\"active\": true, \"count\": 0}");
    TEST("Parse object with bool and zero", n && n->type == JSON_OBJECT);
    if (n) {
        TEST("  active == true", Json_GetBool(Json_GetObjectItem(n, "active"), 0) == 1);
        TEST("  count == 0", Json_GetNumber(Json_GetObjectItem(n, "count"), -1) == 0);
        Json_Free(n);
    }

    TEST("Parse invalid: {\"key\"}", Json_Parse("{\"key\"}") == NULL);
    TEST("Parse invalid: {\": val\"}", Json_Parse("{\": val\"}") == NULL);
    TEST("Parse invalid: {key: val}", Json_Parse("{key: val}") == NULL);
}

void test_complex() {
    printf("\n=== Test: Complex Nested ===\n");

    const char* json =
        "{"
        "\"status\": 200,"
        "\"message\": \"success\","
        "\"data\": {"
            "\"id\": 42,"
            "\"name\": \"test\","
            "\"active\": true,"
            "\"tags\": [\"api\", \"v1\", \"rest\"],"
            "\"meta\": {"
                "\"version\": \"1.0\","
                "\"count\": 3,"
                "\"levels\": [1, 2, 3, 4, 5]"
            "},"
            "\"mixed\": [1, \"two\", true, null, {\"x\": 9}]"
        "},"
        "\"list\": ["
            "{\"id\": 1},"
            "{\"id\": 2},"
            "{\"id\": 3}"
        "],"
        "\"empty_arr\": [],"
        "\"empty_obj\": {}"
        "}";

    JsonNode* root = Json_Parse(json);
    TEST("Parse complex JSON", root && root->type == JSON_OBJECT);
    if (!root) return;

    TEST("  status == 200", Json_GetNumber(Json_GetObjectItem(root, "status"), 0) == 200);
    TEST("  message == success", TEST_STREQ(Json_GetString(Json_GetObjectItem(root, "message"), ""), "success"));

    JsonNode* data = Json_GetObjectItem(root, "data");
    TEST("  data exists", data && data->type == JSON_OBJECT);
    if (data) {
        TEST("  data.id == 42", Json_GetNumber(Json_GetObjectItem(data, "id"), 0) == 42);
        TEST("  data.name == test", TEST_STREQ(Json_GetString(Json_GetObjectItem(data, "name"), ""), "test"));
        TEST("  data.active == true", Json_GetBool(Json_GetObjectItem(data, "active"), 0) == 1);

        JsonNode* tags = Json_GetObjectItem(data, "tags");
        TEST("  data.tags is array", tags && tags->type == JSON_ARRAY);
        if (tags) {
            TEST("  data.tags size == 3", Json_GetArraySize(tags) == 3);
            TEST("  data.tags[0] == api", TEST_STREQ(Json_GetString(Json_GetArrayItem(tags, 0), ""), "api"));
            TEST("  data.tags[2] == rest", TEST_STREQ(Json_GetString(Json_GetArrayItem(tags, 2), ""), "rest"));
        }

        JsonNode* meta = Json_GetObjectItem(data, "meta");
        TEST("  data.meta exists", meta && meta->type == JSON_OBJECT);
        if (meta) {
            TEST("  data.meta.version == 1.0", TEST_STREQ(Json_GetString(Json_GetObjectItem(meta, "version"), ""), "1.0"));
            TEST("  data.meta.count == 3", Json_GetNumber(Json_GetObjectItem(meta, "count"), 0) == 3);

            JsonNode* levels = Json_GetObjectItem(meta, "levels");
            TEST("  data.meta.levels is array", levels && levels->type == JSON_ARRAY);
            if (levels) {
                TEST("  data.meta.levels size == 5", Json_GetArraySize(levels) == 5);
                TEST("  data.meta.levels[4] == 5", Json_GetNumber(Json_GetArrayItem(levels, 4), 0) == 5);
            }
        }

        JsonNode* mixed = Json_GetObjectItem(data, "mixed");
        TEST("  data.mixed size == 5", mixed && Json_GetArraySize(mixed) == 5);
        if (mixed) {
            TEST("  data.mixed[0] == 1", Json_GetNumber(Json_GetArrayItem(mixed, 0), 0) == 1);
            TEST("  data.mixed[1] == two", TEST_STREQ(Json_GetString(Json_GetArrayItem(mixed, 1), ""), "two"));
            TEST("  data.mixed[2] == true", Json_GetBool(Json_GetArrayItem(mixed, 2), 0) == 1);
            TEST("  data.mixed[3] == null", Json_GetArrayItem(mixed, 3) && Json_GetArrayItem(mixed, 3)->type == JSON_NULL);
            JsonNode* mObj = Json_GetArrayItem(mixed, 4);
            TEST("  data.mixed[4].x == 9", mObj && Json_GetNumber(Json_GetObjectItem(mObj, "x"), 0) == 9);
        }
    }

    JsonNode* list = Json_GetObjectItem(root, "list");
    TEST("  list size == 3", list && Json_GetArraySize(list) == 3);
    if (list) {
        for (int i = 0; i < 3; i++) {
            char buf[32];
            snprintf(buf, sizeof(buf), "  list[%d].id == %d", i, i + 1);
            JsonNode* item = Json_GetArrayItem(list, i);
            char exp[32];
            snprintf(exp, sizeof(exp), "%d", i + 1);
            TEST(buf, item && Json_GetNumber(Json_GetObjectItem(item, "id"), 0) == i + 1);
        }
    }

    JsonNode* empty_arr = Json_GetObjectItem(root, "empty_arr");
    TEST("  empty_arr size == 0", empty_arr && Json_GetArraySize(empty_arr) == 0);

    JsonNode* empty_obj = Json_GetObjectItem(root, "empty_obj");
    TEST("  empty_obj size == 0", empty_obj && Json_GetObjectSize(empty_obj) == 0);

    Json_Free(root);
}

void test_tostring() {
    printf("\n=== Test: ToString ===\n");

    JsonNode* n;

    n = Json_Parse("null");
    const char* s = Json_ToString(n, NULL);
    TEST("ToString null", s && TEST_STREQ(s, "null"));
    Json_Free(n);

    n = Json_Parse("true");
    s = Json_ToString(n, NULL);
    TEST("ToString true", s && TEST_STREQ(s, "true"));
    Json_Free(n);

    n = Json_Parse("false");
    s = Json_ToString(n, NULL);
    TEST("ToString false", s && TEST_STREQ(s, "false"));
    Json_Free(n);

    n = Json_Parse("123");
    s = Json_ToString(n, NULL);
    TEST("ToString number", s && TEST_STREQ(s, "123"));
    Json_Free(n);

    n = Json_Parse("\"hello\"");
    s = Json_ToString(n, NULL);
    TEST("ToString string", s && TEST_STREQ(s, "\"hello\""));
    Json_Free(n);

    n = Json_Parse("\"hello\\nworld\"");
    s = Json_ToString(n, NULL);
    TEST("ToString escaped string", s);
    Json_Free(n);

    n = Json_Parse("[1, 2, 3]");
    s = Json_ToString(n, NULL);
    TEST("ToString array", s && strstr(s, "[1,2,3]") != NULL);
    Json_Free(n);

    n = Json_Parse("{\"a\": 1, \"b\": 2}");
    s = Json_ToString(n, NULL);
    TEST("ToString object", s && strstr(s, "{\"a\":1,\"b\":2}") != NULL);
    Json_Free(n);

    n = Json_Parse("[]");
    s = Json_ToString(n, NULL);
    TEST("ToString empty array", s && TEST_STREQ(s, "[]"));
    Json_Free(n);

    n = Json_Parse("{}");
    s = Json_ToString(n, NULL);
    TEST("ToString empty object", s && TEST_STREQ(s, "{}"));
    Json_Free(n);

    n = Json_Parse("\"quote\\\"here\"");
    s = Json_ToString(n, NULL);
    TEST("ToString quote in string", s && strstr(s, "\\\"") != NULL);
    Json_Free(n);

    int len = 0;
    n = Json_Parse("{\"x\":1}");
    s = Json_ToString(n, &len);
    TEST("ToString with length", s && len > 0 && len == (int)strlen(s));
    Json_Free(n);
}

void test_http_response() {
    printf("\n=== Test: HTTP Response Simulation ===\n");

    const char* httpResponse =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: 186\r\n"
        "\r\n"
        "{"
        "\"status\":200,"
        "\"message\":\"success\","
        "\"data\":{"
            "\"token\":\"abc123xyz\","
            "\"expires_in\":3600,"
            "\"user\":{"
                "\"id\":1,"
                "\"username\":\"admin\","
                "\"roles\":[\"admin\",\"user\"],"
                "\"profile\":{"
                    "\"email\":\"admin@test.com\","
                    "\"verified\":true"
                "}"
            "},"
            "\"settings\":{\"theme\":\"dark\",\"notifications\":false}"
        "}"
        "}";

    const char* jsonStart = strstr(httpResponse, "{");
    JsonNode* root = Json_Parse(jsonStart);
    TEST("Parse HTTP response body", root && root->type == JSON_OBJECT);
    if (!root) return;

    TEST("  status == 200", Json_GetNumber(Json_GetObjectItem(root, "status"), 0) == 200);
    TEST("  message == success", TEST_STREQ(Json_GetString(Json_GetObjectItem(root, "message"), ""), "success"));

    JsonNode* data = Json_GetObjectItem(root, "data");
    TEST("  data exists", data != NULL);
    if (data) {
        TEST("  data.token", TEST_STREQ(Json_GetString(Json_GetObjectItem(data, "token"), ""), "abc123xyz"));
        TEST("  data.expires_in == 3600", Json_GetNumber(Json_GetObjectItem(data, "expires_in"), 0) == 3600);

        JsonNode* user = Json_GetObjectItem(data, "user");
        TEST("  data.user exists", user != NULL);
        if (user) {
            TEST("  data.user.id == 1", Json_GetNumber(Json_GetObjectItem(user, "id"), 0) == 1);
            TEST("  data.user.username == admin", TEST_STREQ(Json_GetString(Json_GetObjectItem(user, "username"), ""), "admin"));

            JsonNode* roles = Json_GetObjectItem(user, "roles");
            TEST("  data.user.roles size == 2", roles && Json_GetArraySize(roles) == 2);

            JsonNode* profile = Json_GetObjectItem(user, "profile");
            TEST("  data.user.profile exists", profile != NULL);
            if (profile) {
                TEST("  data.user.profile.email", TEST_STREQ(Json_GetString(Json_GetObjectItem(profile, "email"), ""), "admin@test.com"));
                TEST("  data.user.profile.verified == true", Json_GetBool(Json_GetObjectItem(profile, "verified"), 0) == 1);
            }
        }

        JsonNode* settings = Json_GetObjectItem(root, "settings");
        TEST("  settings.theme == dark", settings && TEST_STREQ(Json_GetString(Json_GetObjectItem(settings, "theme"), ""), "dark"));
        TEST("  settings.notifications == false", settings && Json_GetBool(Json_GetObjectItem(settings, "notifications"), 1) == 0);
    }

    Json_Free(root);
}

void test_getter_defaults() {
    printf("\n=== Test: Getter Defaults ===\n");

    JsonNode* n = Json_Parse("\"hello\"");
    TEST("GetString default on string", TEST_STREQ(Json_GetString(n, "default"), "hello"));
    Json_Free(n);

    n = Json_Parse("123");
    TEST("GetNumber default on number", Json_GetNumber(n, 999.0) == 123.0);
    Json_Free(n);

    n = Json_Parse("true");
    TEST("GetBool default on bool", Json_GetBool(n, 0) == 1);
    TEST("GetBool default on null node", Json_GetBool(NULL, 42) == 42);
    TEST("GetNumber default on null node", Json_GetNumber(NULL, 99.0) == 99.0);
    TEST("GetString default on null node", TEST_STREQ(Json_GetString(NULL, "nope"), "nope"));
    Json_Free(n);

    n = Json_Parse("{\"x\":1}");
    TEST("GetObjectItem missing key returns null", Json_GetObjectItem(n, "nonexistent") == NULL);
    Json_Free(n);

    n = Json_Parse("[1,2,3]");
    TEST("GetArrayItem out of bounds returns null", Json_GetArrayItem(n, -1) == NULL);
    TEST("GetArrayItem out of bounds returns null (end)", Json_GetArrayItem(n, 100) == NULL);
    Json_Free(n);
}

int main() {
    printf("========================================\n");
    printf("   JSON Parser Test Suite\n");
    printf("========================================\n");

    test_basic_types();
    test_array();
    test_object();
    test_complex();
    test_tostring();
    test_http_response();
    test_getter_defaults();

    printf("\n========================================\n");
    printf("   Results: %d passed, %d failed\n", g_passed, g_failed);
    printf("========================================\n");

    return g_failed > 0 ? 1 : 0;
}
