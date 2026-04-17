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
    { "sales.qualify",              P_SALES_QUALIFY              },
    { "sales.account_research",     P_SALES_ACCOUNT_RESEARCH     },
    { "marketing.seo_brief",        P_MARKETING_SEO_BRIEF        },
    { "marketing.ad_variants",      P_MARKETING_AD_VARIANTS      },
    { "marketing.social_thread",    P_MARKETING_SOCIAL_THREAD    },
    { "product.user_story",         P_PRODUCT_USER_STORY         },
    { "product.prioritize_rice",    P_PRODUCT_PRIORITIZE_RICE    },
    { "product.release_notes",      P_PRODUCT_RELEASE_NOTES      },
    { "hr.resume_screen",           P_HR_RESUME_SCREEN           },
    { "hr.interview_questions",     P_HR_INTERVIEW_QUESTIONS     },
    { "legal.contract_review",      P_LEGAL_CONTRACT_REVIEW      },
    { "ops.incident_postmortem",    P_OPS_INCIDENT_POSTMORTEM    },
    { "ops.runbook_draft",          P_OPS_RUNBOOK_DRAFT          },
    { "ops.log_triage",             P_OPS_LOG_TRIAGE             },
    { "code.test_gen",              P_CODE_TEST_GEN              },
    { "code.pr_description",        P_CODE_PR_DESCRIPTION        },
    { "code.commit_msg",            P_CODE_COMMIT_MSG            },
    { "safety.injection_detect",    P_SAFETY_INJECTION_DETECT    },
    { "data.entity_extract",        P_DATA_ENTITY_EXTRACT        },
    { "data.anomaly_detect",        P_DATA_ANOMALY_DETECT        },
    { "integration.field_map",      P_INTEGRATION_FIELD_MAP      },
    { "integration.webhook_transform", P_INTEGRATION_WEBHOOK_TRANSFORM },
    { "email.thread_summary",       P_EMAIL_THREAD_SUMMARY       },
    { "task.prioritize_eisenhower", P_TASK_PRIORITIZE_EISENHOWER },
    { "support.reply",              P_SUPPORT_REPLY              },
    { "support.macro_suggest",      P_SUPPORT_MACRO_SUGGEST      },
    { "support.faq_generate",       P_SUPPORT_FAQ_GENERATE       },
    { "sales.discovery_notes",      P_SALES_DISCOVERY_NOTES      },
    { "sales.proposal_draft",       P_SALES_PROPOSAL_DRAFT       },
    { "marketing.landing_copy",     P_MARKETING_LANDING_COPY     },
    { "marketing.newsletter",       P_MARKETING_NEWSLETTER       },
    { "product.okr_draft",          P_PRODUCT_OKR_DRAFT          },
    { "product.project_brief",      P_PRODUCT_PROJECT_BRIEF      },
    { "hr.performance_review",      P_HR_PERFORMANCE_REVIEW      },
    { "hr.one_on_one",              P_HR_ONE_ON_ONE              },
    { "hr.offboarding_checklist",   P_HR_OFFBOARDING_CHECKLIST   },
    { "ops.deploy_plan",            P_OPS_DEPLOY_PLAN            },
    { "ops.rollback_plan",          P_OPS_ROLLBACK_PLAN          },
    { "ops.dependency_advisory",    P_OPS_DEPENDENCY_ADVISORY    },
    { "data.metric_definition",     P_DATA_METRIC_DEFINITION     },
    { "data.sql_explain",           P_DATA_SQL_EXPLAIN           },
    { "code.migration_plan",        P_CODE_MIGRATION_PLAN        },
    { "finance.invoice_review",     P_FINANCE_INVOICE_REVIEW     },
    { "finance.budget_variance",    P_FINANCE_BUDGET_VARIANCE    },
    { "task.daily_standup",         P_TASK_DAILY_STANDUP         },
    { "task.inbox_triage",          P_TASK_INBOX_TRIAGE          },
    { "sales.win_loss",             P_SALES_WIN_LOSS             },
    { "sales.renewal_risk",         P_SALES_RENEWAL_RISK         },
    { "sales.forecast_rollup",      P_SALES_FORECAST_ROLLUP      },
    { "marketing.case_study",       P_MARKETING_CASE_STUDY       },
    { "marketing.press_release",    P_MARKETING_PRESS_RELEASE    },
    { "support.churn_risk",         P_SUPPORT_CHURN_RISK         },
    { "support.csat_followup",      P_SUPPORT_CSAT_FOLLOWUP      },
    { "product.feature_spec",       P_PRODUCT_FEATURE_SPEC       },
    { "product.experiment_design",  P_PRODUCT_EXPERIMENT_DESIGN  },
    { "ops.oncall_handoff",         P_OPS_ONCALL_HANDOFF         },
    { "ops.slo_review",             P_OPS_SLO_REVIEW             },
    { "code.api_design",            P_CODE_API_DESIGN            },
    { "code.adr",                   P_CODE_ADR                   },
    { "data.dashboard_spec",        P_DATA_DASHBOARD_SPEC        },
    { "data.data_quality",          P_DATA_DATA_QUALITY          },
    { "legal.privacy_notice",       P_LEGAL_PRIVACY_NOTICE       },
    { "legal.nda_check",            P_LEGAL_NDA_CHECK            },
    { "hr.job_description",         P_HR_JOB_DESCRIPTION         },
    { "hr.pip_plan",                P_HR_PIP_PLAN                },
    { "integration.retry_policy",   P_INTEGRATION_RETRY_POLICY   },
    { "integration.rate_limit_plan",P_INTEGRATION_RATE_LIMIT_PLAN},
    { "research.competitive_analysis", P_RESEARCH_COMPETITIVE_ANALYSIS },
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
