#include <string.h>
#include "core/prompts.h"
#include "core/pd.h"

/*
 * Prompt registry. The embedded bodies live in core/pd and are
 * declared by the auto-generated core/pd.h. Keeping the mapping table
 * hand-maintained (rather than auto-generated) lets us rename a .md
 * file without regenerating a header, and gives one obvious place to
 * see every live prompt name.
 */
struct entry { const char *name; const char *body; };

static const struct entry TABLE[] = {
    { "system/persona",    P_SYSTEM_PERSONA    },
    { "system/guardrails", P_SYSTEM_GUARDRAILS },
    { "clarify",           P_CLARIFY           },
    { "decompose",         P_DECOMPOSE         },
    { "router",            P_ROUTER            },
    { "research",          P_RESEARCH          },
    { "browse",            P_BROWSE            },
    { "measure",           P_MEASURE           },
    { "learn",             P_LEARN             },
    { "outreach.nudge",    P_OUTREACH_NUDGE    },
    { "outreach.notify",   P_OUTREACH_NOTIFY   },
    { "outreach.reply",    P_OUTREACH_REPLY    },
    { "outreach.cold",     P_OUTREACH_COLD     },
    { "outreach.followup", P_OUTREACH_FOLLOWUP },
    { "outreach.apology",  P_OUTREACH_APOLOGY  },
    { "finance.risk",      P_FINANCE_RISK      },
    { "finance.forecast",  P_FINANCE_FORECAST  },
    { "finance.categorize",P_FINANCE_CATEGORIZE},
    { "finance.receipt",   P_FINANCE_RECEIPT   },
    { "plugin.webhook",    P_PLUGIN_WEBHOOK    },
    { "plugin.oauth_choose", P_PLUGIN_OAUTH_CHOOSE },
    { "plugin.error_repair", P_PLUGIN_ERROR_REPAIR },
    { "exec.code",         P_EXEC_CODE         },
    { "exec.code_review",  P_EXEC_CODE_REVIEW  },
    { "exec.refactor",     P_EXEC_REFACTOR     },
    { "exec.write",        P_EXEC_WRITE        },
    { "exec.summarize",    P_EXEC_SUMMARIZE    },
    { "exec.extract",      P_EXEC_EXTRACT      },
    { "exec.classify",     P_EXEC_CLASSIFY     },
    { "exec.sql",          P_EXEC_SQL          },
    { "exec.translate",    P_EXEC_TRANSLATE    },
    { "exec.rewrite",      P_EXEC_REWRITE      },
    { "exec.sentiment",    P_EXEC_SENTIMENT    },
    { "exec.plan",         P_EXEC_PLAN         },
    { "data.redact",       P_DATA_REDACT       },
    { "data.moderate",     P_DATA_MODERATE     },
    { "data.json_repair",  P_DATA_JSON_REPAIR  },
    { "meeting.agenda",    P_MEETING_AGENDA    },
    { "meeting.notes",     P_MEETING_NOTES     },
    { "meeting.retro",     P_MEETING_RETRO     },
    { "doc.qa",            P_DOC_QA            },
    { "doc.outline",       P_DOC_OUTLINE       },
    { "triage.ticket",     P_TRIAGE_TICKET     },
    { "persona.onboarding",  P_PERSONA_ONBOARDING  },
    { "memory.compact",      P_MEMORY_COMPACT      },
    { "learn.dedup",         P_LEARN_DEDUP         },
    { "decompose.critic",    P_DECOMPOSE_CRITIC    },
    { "measure.critic",      P_MEASURE_CRITIC      },
    { "kb.qa",               P_KB_QA               },
    { "planner.weekly",      P_PLANNER_WEEKLY      },
    { "report.weekly",       P_REPORT_WEEKLY       },
    { "outreach.escalation", P_OUTREACH_ESCALATION },
    { "code.debug",          P_CODE_DEBUG          },
};
#define N_PROMPTS (sizeof(TABLE) / sizeof(TABLE[0]))

static const char *NAMES[N_PROMPTS + 1];
static int names_built = 0;

const char *prompt_get(const char *name)
{
    size_t i;
    if (!name) return 0;
    for (i = 0; i < N_PROMPTS; i++)
        if (strcmp(TABLE[i].name, name) == 0) return TABLE[i].body;
    return 0;
}

const char *const *prompt_list(void)
{
    size_t i;
    if (!names_built) {
        for (i = 0; i < N_PROMPTS; i++) NAMES[i] = TABLE[i].name;
        NAMES[N_PROMPTS] = 0;
        names_built = 1;
    }
    return NAMES;
}

size_t prompt_count(void) { return N_PROMPTS; }
