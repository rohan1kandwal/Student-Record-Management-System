#ifndef OPENAI_AI_H
#define OPENAI_AI_H

#include "student.h"

typedef enum { RISK_LOW = 0, RISK_MEDIUM = 1, RISK_HIGH = 2 } RiskLevel;

/* Same API as old ai.h so rest of project stays unchanged after include replacement */
RiskLevel ai_predict_risk(const Student *s);
const char *ai_suggest_career(const Student *s); /* pointer to internal static buffer */
void ai_explain(const Student *s, char *outbuf, int bufsize);

#endif /* OPENAI_AI_H */