#ifndef TYPES_EXT_H
#define TYPES_EXT_H

/* Umbrella header: re-exports the six per-domain type headers so that
 * existing call-sites keep compiling while new code can pull only the
 * specific domain it needs. New modules should prefer including the
 * individual domain header directly. */

#include "core/types_workflow.h"
#include "core/types_finance.h"
#include "core/types_enterprise.h"
#include "core/types_analytics.h"
#include "core/types_content.h"
#include "core/types_memory.h"

#endif /* TYPES_EXT_H */
