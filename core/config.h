#ifndef CONFIG_H
#define CONFIG_H

#include "core/types.h"

Config load_config(const char *env_path) __attribute__((warn_unused_result));
Err    validate_config(const Config *cfg) __attribute__((warn_unused_result));

#endif /* CONFIG_H */
