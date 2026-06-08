#pragma once

//#include <windows.h>

typedef enum JsonType {
    JSON_NULL,
    JSON_BOOL,
    JSON_NUMBER,
    JSON_STRING,
    JSON_ARRAY,
    JSON_OBJECT
} JsonType;

typedef struct JsonNode JsonNode;

struct JsonNode {
    JsonType type;
    union {
        int    boolValue;
        double numberValue;
        struct {
            char* data;
            int   length;
        } stringValue;
        struct {
            JsonNode** items;
            int        count;
        } arrayValue;
        struct {
            char** keys;
            JsonNode** values;
            int    count;
        } objectValue;
    } value;
    JsonNode* parent;
};

JsonNode* Json_Parse(const char* jsonStr);
void      Json_Free(JsonNode* node);

const char* Json_TypeName(JsonType type);
const char* Json_ToString(JsonNode* node, int* outLength);

JsonNode* Json_GetArrayItem(JsonNode* array, int index);
JsonNode* Json_GetObjectItem(JsonNode* object, const char* key);

const char* Json_GetString(JsonNode* node, const char* defaultValue);
double      Json_GetNumber(JsonNode* node, double defaultValue);
int         Json_GetBool(JsonNode* node, int defaultValue);
int         Json_GetArraySize(JsonNode* array);
int         Json_GetObjectSize(JsonNode* object);
