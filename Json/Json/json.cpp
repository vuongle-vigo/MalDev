#include "json.h"
#include "CRT.h"
//#include <stdlib.h>
//#include <string.h>
//#include <ctype.h>
//#include <stdio.h>

static const char* g_parsePos = NULL;
static const char* g_parseEnd = NULL;
static char g_errorMsg[64] = { 0 };

static const char* skipWhitespace(const char* p) {
    while (p < g_parseEnd && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')) p++;
    return p;
}

static void setError(const char* msg) {
    size_t len = crt_strlen(msg);
    if (len >= sizeof(g_errorMsg)) len = sizeof(g_errorMsg) - 1;
    crt_memcpy(g_errorMsg, msg, len);
    g_errorMsg[len] = '\0';
}

static JsonNode* allocNode(JsonType type) {
    JsonNode* node = (JsonNode*)crt_calloc(1, sizeof(JsonNode));
    if (node) node->type = type;
    return node;
}

static int hexValue(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static int parseString(const char** p, char** outStr, int* outLen) {
    const char* start = *p;
    if (*start != '"') return 0;

    const char* ptr = start + 1;
    int capacity = 64;
    int length = 0;
    char* buffer = (char*)crt_malloc(capacity);
    if (!buffer) return 0;

    while (ptr < g_parseEnd && *ptr != '"') {
        if (*ptr == '\\' && ptr + 1 < g_parseEnd) {
            ptr++;
            switch (*ptr) {
                case '"':  buffer[length++] = '"';  break;
                case '\\': buffer[length++] = '\\'; break;
                case '/':  buffer[length++] = '/';  break;
                case 'b':  buffer[length++] = '\b'; break;
                case 'f':  buffer[length++] = '\f'; break;
                case 'n':  buffer[length++] = '\n'; break;
                case 'r':  buffer[length++] = '\r'; break;
                case 't':  buffer[length++] = '\t'; break;
                case 'u':
                    if (ptr + 4 < g_parseEnd) {
                        int h = hexValue(ptr[1]);
                        int l = hexValue(ptr[2]);
                        int h2 = hexValue(ptr[3]);
                        int l2 = hexValue(ptr[4]);
                        if (h >= 0 && l >= 0 && h2 >= 0 && l2 >= 0) {
                            unsigned int cp = (h << 12) | (l << 8) | (h2 << 4) | l2;
                            if (cp >= 0xD800 && cp <= 0xDBFF) {
                                if (ptr[5] == '\\' && ptr[6] == 'u') {
                                    int h3 = hexValue(ptr[7]);
                                    int l3 = hexValue(ptr[8]);
                                    int h4 = hexValue(ptr[9]);
                                    int l4 = hexValue(ptr[10]);
                                    if (h3 >= 0 && l3 >= 0 && h4 >= 0 && l4 >= 0) {
                                        unsigned int cp2 = (h3 << 12) | (l3 << 8) | (h4 << 4) | l4;
                                        unsigned int codepoint = 0x10000 + ((cp - 0xD800) << 10) + (cp2 - 0xDC00);
                                        if (codepoint < 0x80) {
                                            buffer[length++] = (char)codepoint;
                                        } else if (codepoint < 0x800) {
                                            buffer[length++] = (char)(0xC0 | (codepoint >> 6));
                                            buffer[length++] = (char)(0x80 | (codepoint & 0x3F));
                                        } else {
                                            buffer[length++] = (char)(0xE0 | (codepoint >> 12));
                                            buffer[length++] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
                                            buffer[length++] = (char)(0x80 | (codepoint & 0x3F));
                                        }
                                        ptr += 10;
                                    } else { buffer[length++] = (char)cp; ptr += 4; }
                                } else { buffer[length++] = (char)cp; ptr += 4; }
                            } else if (cp < 0x80) {
                                buffer[length++] = (char)cp;
                                ptr += 4;
                            } else if (cp < 0x800) {
                                buffer[length++] = (char)(0xC0 | (cp >> 6));
                                buffer[length++] = (char)(0x80 | (cp & 0x3F));
                                ptr += 4;
                            } else {
                                buffer[length++] = (char)(0xE0 | (cp >> 12));
                                buffer[length++] = (char)(0x80 | ((cp >> 6) & 0x3F));
                                buffer[length++] = (char)(0x80 | (cp & 0x3F));
                                ptr += 4;
                            }
                            continue;
                        }
                    }
                    crt_free(buffer);
                    return 0;
                default:
                    crt_free(buffer);
                    return 0;
            }
        } else if ((unsigned char)*ptr < 0x20) {
            crt_free(buffer);
            return 0;
        } else {
            buffer[length++] = *ptr;
        }

        if (length >= capacity - 4) {
            capacity *= 2;
            char* tmp = (char*)crt_realloc(buffer, capacity);
            if (!tmp) { crt_free(buffer); return 0; }
            buffer = tmp;
        }
        ptr++;
    }

    if (ptr >= g_parseEnd || *ptr != '"') {
        crt_free(buffer);
        return 0;
    }

    buffer[length] = '\0';
    *outStr = buffer;
    *outLen = length;
    *p = ptr + 1;
    return 1;
}

static int parseNumber(const char** p, double* outVal) {
    const char* start = *p;
    const char* ptr = start;

    if (*ptr == '-') ptr++;
    if (ptr >= g_parseEnd) return 0;

    int hasDigits = 0;
    while (ptr < g_parseEnd && crt_isdigit((unsigned char)*ptr)) {
        hasDigits = 1;
        ptr++;
    }

    if (!hasDigits) { *p = start; return 0; }

    if (ptr < g_parseEnd && *ptr == '.') {
        ptr++;
        hasDigits = 0;
        while (ptr < g_parseEnd && crt_isdigit((unsigned char)*ptr)) {
            hasDigits = 1;
            ptr++;
        }
        if (!hasDigits) { *p = start; return 0; }
    }

    if (ptr < g_parseEnd && (*ptr == 'e' || *ptr == 'E')) {
        ptr++;
        if (ptr < g_parseEnd && (*ptr == '+' || *ptr == '-')) ptr++;
        int expDigits = 0;
        while (ptr < g_parseEnd && crt_isdigit((unsigned char)*ptr)) {
            expDigits = 1;
            ptr++;
        }
        if (!expDigits) { *p = start; return 0; }
    }

    char buf[64];
    int len = (int)(ptr - start);
    if (len >= (int)sizeof(buf)) len = sizeof(buf) - 1;
    crt_memcpy(buf, start, len);
    buf[len] = '\0';

    char* endptr;
    *outVal = crt_strtod(buf, &endptr);
    *p = ptr;
    return endptr != buf;
}

static int isWhitespace(char c);

static int parseValue(const char** p, JsonNode** outNode);
static int parseArray(const char** p, JsonNode** outNode);
static int parseObject(const char** p, JsonNode** outNode);

static int parseArray(const char** p, JsonNode** outNode) {
    if (**p != '[') return 0;
    (*p)++;

    JsonNode* node = allocNode(JSON_ARRAY);
    if (!node) return 0;

    int capacity = 4;
    int count = 0;
    node->value.arrayValue.items = (JsonNode**)crt_calloc(capacity, sizeof(JsonNode*));

    *p = skipWhitespace(*p);
    if (**p != ']') {
        while (1) {
            JsonNode* item = NULL;
            if (!parseValue(p, &item)) {
                Json_Free(node);
                return 0;
            }
            if (count >= capacity) {
                capacity *= 2;
                JsonNode** tmp = (JsonNode**)crt_realloc(node->value.arrayValue.items, capacity * sizeof(JsonNode*));
                if (!tmp) { Json_Free(node); return 0; }
                node->value.arrayValue.items = tmp;
            }
            node->value.arrayValue.items[count++] = item;
            *p = skipWhitespace(*p);
            if (**p == ']') break;
            if (**p != ',') { Json_Free(node); return 0; }
            (*p)++;
            *p = skipWhitespace(*p);
        }
    }

    *p = skipWhitespace(*p);
    if (**p != ']') { Json_Free(node); return 0; }
    (*p)++;

    node->value.arrayValue.count = count;
    *outNode = node;
    return 1;
}

static int parseObject(const char** p, JsonNode** outNode) {
    if (**p != '{') return 0;
    (*p)++;

    JsonNode* node = allocNode(JSON_OBJECT);
    if (!node) return 0;

    int capacity = 4;
    int count = 0;
    node->value.objectValue.keys = (char**)crt_calloc(capacity, sizeof(char*));
    node->value.objectValue.values = (JsonNode**)crt_calloc(capacity, sizeof(JsonNode*));

    *p = skipWhitespace(*p);
    if (**p != '}') {
        while (1) {
            char* key = NULL;
            int keyLen = 0;
            if (!parseString(p, &key, &keyLen)) { Json_Free(node); return 0; }
            *p = skipWhitespace(*p);
            if (**p != ':') { crt_free(key); Json_Free(node); return 0; }
            (*p)++;
            *p = skipWhitespace(*p);

            JsonNode* value = NULL;
            if (!parseValue(p, &value)) { crt_free(key); Json_Free(node); return 0; }

            if (count >= capacity) {
                capacity *= 2;
                char** tmpKeys = (char**)crt_realloc(node->value.objectValue.keys, capacity * sizeof(char*));
                JsonNode** tmpVals = (JsonNode**)crt_realloc(node->value.objectValue.values, capacity * sizeof(JsonNode*));
                if (!tmpKeys || !tmpVals) {
                    crt_free(key); Json_Free(node);
                    if (tmpKeys) crt_free(tmpKeys);
                    if (tmpVals) crt_free(tmpVals);
                    return 0;
                }
                node->value.objectValue.keys = tmpKeys;
                node->value.objectValue.values = tmpVals;
            }
            node->value.objectValue.keys[count] = key;
            node->value.objectValue.values[count] = value;
            count++;

            *p = skipWhitespace(*p);
            if (**p == '}') break;
            if (**p != ',') { Json_Free(node); return 0; }
            (*p)++;
            *p = skipWhitespace(*p);
        }
    }

    *p = skipWhitespace(*p);
    if (**p != '}') { Json_Free(node); return 0; }
    (*p)++;

    node->value.objectValue.count = count;
    *outNode = node;
    return 1;
}

static int isWhitespace(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

static int parseValue(const char** p, JsonNode** outNode) {
    *p = skipWhitespace(*p);
    if (*p >= g_parseEnd) return 0;

    const char ch = **p;

    if (ch == 'n' && crt_strncmp(*p, "null", 4) == 0 && ((*p)[4] == ',' || (*p)[4] == ']' || (*p)[4] == '}' || isWhitespace((*p)[4]) || (*p)[4] == '\0' || (*p)[4] == ':')) {
        JsonNode* node = allocNode(JSON_NULL);
        *p += 4;
        *outNode = node;
        return 1;
    }
    if (ch == 't' && crt_strncmp(*p, "true", 4) == 0 && ((*p)[4] == ',' || (*p)[4] == ']' || (*p)[4] == '}' || isWhitespace((*p)[4]) || (*p)[4] == '\0' || (*p)[4] == ':')) {
        JsonNode* node = allocNode(JSON_BOOL);
        node->value.boolValue = 1;
        *p += 4;
        *outNode = node;
        return 1;
    }
    if (ch == 'f' && crt_strncmp(*p, "false", 5) == 0 && ((*p)[5] == ',' || (*p)[5] == ']' || (*p)[5] == '}' || isWhitespace((*p)[5]) || (*p)[5] == '\0' || (*p)[5] == ':')) {
        JsonNode* node = allocNode(JSON_BOOL);
        node->value.boolValue = 0;
        *p += 5;
        *outNode = node;
        return 1;
    }

    if (ch == '"') {
        char* str = NULL;
        int len = 0;
        if (!parseString(p, &str, &len)) return 0;
        JsonNode* node = allocNode(JSON_STRING);
        if (!node) { crt_free(str); return 0; }
        node->value.stringValue.data = str;
        node->value.stringValue.length = len;
        *outNode = node;
        return 1;
    }

    if (ch == '[') return parseArray(p, outNode);
    if (ch == '{') return parseObject(p, outNode);

    if (ch == '-' || crt_isdigit((unsigned char)ch)) {
        double num = 0;
        if (!parseNumber(p, &num)) return 0;
        JsonNode* node = allocNode(JSON_NUMBER);
        if (!node) return 0;
        node->value.numberValue = num;
        *outNode = node;
        return 1;
    }

    return 0;
}

JsonNode* Json_Parse(const char* jsonStr) {
    if (!jsonStr) return NULL;
    g_parsePos = jsonStr;
    g_parseEnd = jsonStr + crt_strlen(jsonStr);
    memset(g_errorMsg, 0, sizeof(g_errorMsg));

    g_parsePos = skipWhitespace(g_parsePos);
    JsonNode* root = NULL;
    if (!parseValue(&g_parsePos, &root)) {
        if (root) Json_Free(root);
        return NULL;
    }
    return root;
}

void Json_Free(JsonNode* node) {
    if (!node) return;
    switch (node->type) {
        case JSON_STRING:
        crt_free(node->value.stringValue.data);
        break;
        case JSON_ARRAY:
        for (int i = 0; i < node->value.arrayValue.count; i++) {
            Json_Free(node->value.arrayValue.items[i]);
        }
        crt_free(node->value.arrayValue.items);
        break;
        case JSON_OBJECT:
        for (int i = 0; i < node->value.objectValue.count; i++) {
            crt_free(node->value.objectValue.keys[i]);
            Json_Free(node->value.objectValue.values[i]);
        }
        crt_free(node->value.objectValue.keys);
        crt_free(node->value.objectValue.values);
        break;
        default:
        break;
    }
    crt_free(node);
}

const char* Json_TypeName(JsonType type) {
    switch (type) {
        case JSON_NULL:    return "null";
        case JSON_BOOL:    return "bool";
        case JSON_NUMBER:  return "number";
        case JSON_STRING:  return "string";
        case JSON_ARRAY:   return "array";
        case JSON_OBJECT:  return "object";
        default:           return "unknown";
    }
}

static void appendChar(char** buf, int* pos, int* cap, char c) {
    if (*pos >= *cap - 1) {
        *cap *= 2;
        char* tmp = (char*)crt_realloc(*buf, *cap);
        if (!tmp) return;
        *buf = tmp;
    }
    (*buf)[(*pos)++] = c;
}

static void appendStr(char** buf, int* pos, int* cap, const char* s, int len) {
    for (int i = 0; i < len; i++) {
        if (*pos >= *cap - 2) {
            *cap *= 2;
            char* tmp = (char*)crt_realloc(*buf, *cap);
            if (!tmp) return;
            *buf = tmp;
        }
        (*buf)[(*pos)++] = s[i];
    }
}

static int toStringRecursive(JsonNode* node, char** buf, int* pos, int* cap) {
    if (!node) {
        appendStr(buf, pos, cap, "null", 4);
        return 1;
    }

    switch (node->type) {
        case JSON_NULL:
        appendStr(buf, pos, cap, "null", 4);
        break;

        case JSON_BOOL:
        if (node->value.boolValue) appendStr(buf, pos, cap, "true", 4);
        else appendStr(buf, pos, cap, "false", 5);
        break;

        case JSON_NUMBER: {
            char tmp[64];
            int len = crt_snprintf(tmp, sizeof(tmp), "%.15g", node->value.numberValue);
            appendStr(buf, pos, cap, tmp, len);
            break;
        }

        case JSON_STRING: {
            appendChar(buf, pos, cap, '"');
            const char* s = node->value.stringValue.data;
            int len = node->value.stringValue.length;
            for (int i = 0; i < len; i++) {
                unsigned char c = (unsigned char)s[i];
                if (c == '"')       { appendStr(buf, pos, cap, "\\\"", 2); }
                else if (c == '\\') { appendStr(buf, pos, cap, "\\\\", 2); }
                else if (c == '/')  { appendStr(buf, pos, cap, "\\/", 2); }
                else if (c == '\b') { appendStr(buf, pos, cap, "\\b", 2); }
                else if (c == '\f') { appendStr(buf, pos, cap, "\\f", 2); }
                else if (c == '\n') { appendStr(buf, pos, cap, "\\n", 2); }
                else if (c == '\r') { appendStr(buf, pos, cap, "\\r", 2); }
                else if (c == '\t') { appendStr(buf, pos, cap, "\\t", 2); }
                else if (c < 0x20) {
                    char esc[7];
                    crt_snprintf(esc, sizeof(esc), "\\u%04X", c);
                    appendStr(buf, pos, cap, esc, 6);
                } else {
                    appendChar(buf, pos, cap, (char)c);
                }
            }
            appendChar(buf, pos, cap, '"');
            break;
        }

        case JSON_ARRAY: {
            appendChar(buf, pos, cap, '[');
            for (int i = 0; i < node->value.arrayValue.count; i++) {
                if (i > 0) appendChar(buf, pos, cap, ',');
                toStringRecursive(node->value.arrayValue.items[i], buf, pos, cap);
            }
            appendChar(buf, pos, cap, ']');
            break;
        }

        case JSON_OBJECT: {
            appendChar(buf, pos, cap, '{');
            for (int i = 0; i < node->value.objectValue.count; i++) {
                if (i > 0) appendChar(buf, pos, cap, ',');
                appendChar(buf, pos, cap, '"');
                appendStr(buf, pos, cap, node->value.objectValue.keys[i], crt_strlen(node->value.objectValue.keys[i]));
                appendChar(buf, pos, cap, '"');
                appendChar(buf, pos, cap, ':');
                toStringRecursive(node->value.objectValue.values[i], buf, pos, cap);
            }
            appendChar(buf, pos, cap, '}');
            break;
        }
    }
    return 1;
}

const char* Json_ToString(JsonNode* node, int* outLength) {
    static char* g_staticBuf = NULL;
    static int   g_staticCap = 0;

    if (g_staticBuf) { crt_free(g_staticBuf); g_staticBuf = NULL; g_staticCap = 0; }

    int cap = 256;
    g_staticBuf = (char*)crt_malloc(cap);
    if (!g_staticBuf) return NULL;

    int pos = 0;
    toStringRecursive(node, &g_staticBuf, &pos, &cap);
    appendChar(&g_staticBuf, &pos, &cap, '\0');
    g_staticCap = cap;

    if (outLength) *outLength = pos;
    return g_staticBuf;
}

JsonNode* Json_GetArrayItem(JsonNode* array, int index) {
    if (!array || array->type != JSON_ARRAY) return NULL;
    if (index < 0 || index >= array->value.arrayValue.count) return NULL;
    return array->value.arrayValue.items[index];
}

JsonNode* Json_GetObjectItem(JsonNode* object, const char* key) {
    if (!object || object->type != JSON_OBJECT || !key) return NULL;
    for (int i = 0; i < object->value.objectValue.count; i++) {
        if (crt_strcmp(object->value.objectValue.keys[i], key) == 0) {
            return object->value.objectValue.values[i];
        }
    }
    return NULL;
}

const char* Json_GetString(JsonNode* node, const char* defaultValue) {
    if (!node || node->type != JSON_STRING) return defaultValue;
    return node->value.stringValue.data;
}

double Json_GetNumber(JsonNode* node, double defaultValue) {
    if (!node || node->type != JSON_NUMBER) return defaultValue;
    return node->value.numberValue;
}

int Json_GetBool(JsonNode* node, int defaultValue) {
    if (!node || node->type != JSON_BOOL) return defaultValue;
    return node->value.boolValue;
}

int Json_GetArraySize(JsonNode* array) {
    if (!array || array->type != JSON_ARRAY) return 0;
    return array->value.arrayValue.count;
}

int Json_GetObjectSize(JsonNode* object) {
    if (!object || object->type != JSON_OBJECT) return 0;
    return object->value.objectValue.count;
}
