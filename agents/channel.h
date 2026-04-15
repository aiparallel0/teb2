#ifndef CHANNEL_H
#define CHANNEL_H

#include "core/types.h"

/* Agent entry points — one per phase of the core loop */
AgentMsg coord_handle(AgentMsg msg);
AgentMsg clarify_handle(AgentMsg msg);
AgentMsg finance_handle(AgentMsg msg);
AgentMsg outreach_handle(AgentMsg msg);
AgentMsg research_handle(AgentMsg msg);
AgentMsg measure_handle(AgentMsg msg);
AgentMsg learn_handle(AgentMsg msg);

#endif /* CHANNEL_H */
