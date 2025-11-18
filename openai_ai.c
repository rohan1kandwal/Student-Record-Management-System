#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <curl/curl.h>
#include "openai_ai.h"
#include "student.h"
#include <cjson/cJSON.h>

#define OPENAI_URL "https://api.openai.com/v1/chat/completions"
#define MODEL_NAME "gpt-4o-mini"
#define BUF_SMALL 512
#define BUF_LARGE 4096

struct mem { char *ptr; size_t len; };
static size_t write_cb(void *data, size_t size, size_t nmemb, void *userp) {
    size_t real = size * nmemb;
    struct mem *m = (struct mem *)userp;
    char *tmp = realloc(m->ptr, m->len + real + 1);
    if (!tmp) return 0;
    m->ptr = tmp;
    memcpy(m->ptr + m->len, data, real);
    m->len += real;
    m->ptr[m->len] = '\0';
    return real;
}

static void student_to_json(const Student *s, char *out, int outlen) {
    if (!s) { out[0] = '\0'; return; }
    char marks[256] = {0};
    int p = snprintf(marks, sizeof(marks), "[");
    for (int i = 0; i < NUM_SUBJECTS && p < (int)sizeof(marks)-32; ++i)
        p += snprintf(marks + p, sizeof(marks) - p, "%g%s", s->marks[i], (i+1==NUM_SUBJECTS) ? "" : ",");
    strncat(marks, "]", sizeof(marks)-1);
    snprintf(out, outlen, "{\"roll\":%d,\"name\":\"%s\",\"marks\":%s,\"attendance\":%g}",
             s->roll, s->name, marks, s->attendance);
}

static char *call_openai_for_student(const char *student_json) {
    const char *key = getenv("OPENAI_API_KEY");
    if (!key) return NULL;

    CURL *curl = curl_easy_init();
    if (!curl) return NULL;

    struct mem resp = { .ptr = malloc(1), .len = 0 };
    curl_easy_setopt(curl, CURLOPT_URL, OPENAI_URL);

    struct curl_slist *hdrs = NULL;
    hdrs = curl_slist_append(hdrs, "Content-Type: application/json");
    char auth[512]; snprintf(auth, sizeof(auth), "Authorization: Bearer %s", key);
    hdrs = curl_slist_append(hdrs, auth);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hdrs);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resp);

    /* Build request with clearer system instructions and few-shot examples.
       We force JSON-only output, provide an allowed career list, and set temperature=0
       so results are deterministic and less likely to default to "engineering". */
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "model", MODEL_NAME);
    cJSON_AddNumberToObject(root, "temperature", 0.0);
    cJSON_AddNumberToObject(root, "max_tokens", 300);

    cJSON *messages = cJSON_CreateArray();

    /* System message: strict instructions and allowed career buckets */
    cJSON *sys = cJSON_CreateObject();
    cJSON_AddStringToObject(sys, "role", "system");
    cJSON_AddStringToObject(sys, "content",
        "You are an expert career counsellor. Output ONLY a single JSON object with keys: "
        "\"risk\" (one of HIGH, MEDIUM, LOW), "
        "\"career\" (one of: \"Computer Science\", \"Electronics / ECE\", \"Civil / Civil Eng\", "
        "\"Management\", \"Arts / Humanities\", \"Research / Academia\", \"Vocational / Trade\"), "
        "and \"explanation\" (brief). Do NOT output the token \"Unknown\" or empty career — always pick the best matching career from the allowed list. "
        "Do not output anything outside the single JSON object. Decide based on marks across subjects and attendance. Prefer non-engineering if languages/arts scores are clearly highest. Use clear, concise explanations referencing top subjects and attendance.");
    cJSON_AddItemToArray(messages, sys);

    /* Few-shot example 1: strong CS */
    cJSON *u1 = cJSON_CreateObject();
    cJSON_AddStringToObject(u1, "role", "user");
    cJSON_AddStringToObject(u1, "content",
        "Student: {\"roll\":101,\"name\":\"Alice\",\"marks\":[95,90,88,96,80],\"attendance\":92}");
    cJSON_AddItemToArray(messages, u1);
    cJSON *a1 = cJSON_CreateObject();
    cJSON_AddStringToObject(a1, "role", "assistant");
    cJSON_AddStringToObject(a1, "content",
        "{\"risk\":\"LOW\",\"career\":\"Computer Science\",\"explanation\":\"Very high CS and Math marks with high attendance; excellent fit for CS.\"}");
    cJSON_AddItemToArray(messages, a1);

    /* Few-shot example 2: strong English / Arts */
    cJSON *u2 = cJSON_CreateObject();
    cJSON_AddStringToObject(u2, "role", "user");
    cJSON_AddStringToObject(u2, "content",
        "Student: {\"roll\":102,\"name\":\"Bob\",\"marks\":[48,50,45,30,92],\"attendance\":88}");
    cJSON_AddItemToArray(messages, u2);
    cJSON *a2 = cJSON_CreateObject();
    cJSON_AddStringToObject(a2, "role", "assistant");
    cJSON_AddStringToObject(a2, "content",
        "{\"risk\":\"MEDIUM\",\"career\":\"Arts / Humanities\",\"explanation\":\"Very strong English with weaker STEM marks; recommend Arts/Humanities or language-related fields.\"}");
    cJSON_AddItemToArray(messages, a2);

    /* Few-shot example 3: management oriented */
    cJSON *u3 = cJSON_CreateObject();
    cJSON_AddStringToObject(u3, "role", "user");
    cJSON_AddStringToObject(u3, "content",
        "Student: {\"roll\":103,\"name\":\"Carol\",\"marks\":[72,68,65,70,85],\"attendance\":90}");
    cJSON_AddItemToArray(messages, u3);
    cJSON *a3 = cJSON_CreateObject();
    cJSON_AddStringToObject(a3, "role", "assistant");
    cJSON_AddStringToObject(a3, "content",
        "{\"risk\":\"LOW\",\"career\":\"Management\",\"explanation\":\"Balanced marks with strong English and good overall scores; suitable for Management/business studies.\"}");
    cJSON_AddItemToArray(messages, a3);

    /* Finally add the actual student to be analyzed */
    cJSON *user = cJSON_CreateObject();
    cJSON_AddStringToObject(user, "role", "user");
    char prompt[1024];
    snprintf(prompt, sizeof(prompt), "Student: %s\nReturn JSON as specified above.", student_json);
    cJSON_AddStringToObject(user, "content", prompt);
    cJSON_AddItemToArray(messages, user);

    cJSON_AddItemToObject(root, "messages", messages);
    char *body = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);
    CURLcode res = curl_easy_perform(curl);
    free(body);
    curl_slist_free_all(hdrs);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        free(resp.ptr);
        return NULL;
    }

    /* Parse assistant reply similar to previous logic */
    cJSON *j = cJSON_Parse(resp.ptr);
    char *assistant_text = NULL;
    if (j) {
        cJSON *choices = cJSON_GetObjectItem(j, "choices");
        if (cJSON_IsArray(choices) && cJSON_GetArraySize(choices) > 0) {
            cJSON *first = cJSON_GetArrayItem(choices, 0);
            cJSON *msg = cJSON_GetObjectItem(first, "message");
            if (msg) {
                cJSON *content = cJSON_GetObjectItem(msg, "content");
                if (cJSON_IsString(content)) assistant_text = strdup(content->valuestring);
            } else {
                cJSON *text = cJSON_GetObjectItem(first, "text");
                if (cJSON_IsString(text)) assistant_text = strdup(text->valuestring);
            }
        }
        cJSON_Delete(j);
    }
    free(resp.ptr);
    return assistant_text;
}

static int parse_assistant_json(const char *txt, char *risk, int rlen, char *career, int clen, char *explain, int elen) {
    if (!txt) return 0;
    cJSON *root = cJSON_Parse(txt);
    if (!root) return 0;
    cJSON *jr = cJSON_GetObjectItemCaseSensitive(root, "risk");
    cJSON *jc = cJSON_GetObjectItemCaseSensitive(root, "career");
    cJSON *je = cJSON_GetObjectItemCaseSensitive(root, "explanation");
    if (cJSON_IsString(jr) && jr->valuestring) { strncpy(risk, jr->valuestring, rlen-1); risk[rlen-1] = '\0'; }
    if (cJSON_IsString(jc) && jc->valuestring) { strncpy(career, jc->valuestring, clen-1); career[clen-1] = '\0'; }
    if (cJSON_IsString(je) && je->valuestring) { strncpy(explain, je->valuestring, elen-1); explain[elen-1] = '\0'; }
    cJSON_Delete(root);
    return 1;
}

RiskLevel ai_predict_risk(const Student *s) {
    char sj[512]; student_to_json(s, sj, sizeof(sj));
    char *assistant = call_openai_for_student(sj);
    if (!assistant) {
        double avg = student_average(s);
        if (avg < 45.0 || s->attendance < 50.0) return RISK_HIGH;
        if (avg < 60.0 || s->attendance < 65.0) return RISK_MEDIUM;
        return RISK_LOW;
    }
    char risk[BUF_SMALL]={0}, career[BUF_SMALL]={0}, explain[BUF_LARGE]={0};
    int ok = parse_assistant_json(assistant, risk, sizeof(risk), career, sizeof(career), explain, sizeof(explain));
    free(assistant);
    if (!ok) return RISK_LOW;
    for (char *p = risk; *p; ++p) *p = (char)toupper((unsigned char)*p);
    if (strcmp(risk, "HIGH") == 0) return RISK_HIGH;
    if (strcmp(risk, "MEDIUM") == 0) return RISK_MEDIUM;
    return RISK_LOW;
}

const char *ai_suggest_career(const Student *s) {
    static char career_buf[BUF_SMALL];
    career_buf[0] = '\0';
    char sj[512]; student_to_json(s, sj, sizeof(sj));
    char *assistant = call_openai_for_student(sj);
    if (!assistant) { strncpy(career_buf, "Unknown", sizeof(career_buf)-1); return career_buf; }
    char risk[BUF_SMALL]={0}, explain[BUF_LARGE]={0};
    if (!parse_assistant_json(assistant, risk, sizeof(risk), career_buf, sizeof(career_buf), explain, sizeof(explain))) {
        strncpy(career_buf, "Unknown", sizeof(career_buf)-1);
    }
    free(assistant);
    return career_buf;
}

void ai_explain(const Student *s, char *outbuf, int bufsize) {
    if (!outbuf || bufsize <= 0) return;
    char sj[512]; student_to_json(s, sj, sizeof(sj));
    char *assistant = call_openai_for_student(sj);
    if (!assistant) {
        snprintf(outbuf, bufsize, "No AI available; local fallback used.");
        return;
    }
    char risk[BUF_SMALL]={0}, career[BUF_SMALL]={0}, explain[BUF_LARGE]={0};
    parse_assistant_json(assistant, risk, sizeof(risk), career, sizeof(career), explain, sizeof(explain));
    free(assistant);
    snprintf(outbuf, bufsize, "Risk: %s. Career: %s. Explanation: %s",
             risk[0] ? risk : "Unknown", career[0] ? career : "Unknown", explain[0] ? explain : "None");
}
