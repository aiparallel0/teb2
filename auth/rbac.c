#include "core/types.h"
#include "auth/auth.h"

/*
 * Static permission table.
 * Rows: UserRole.  Columns: Permission.
 * Value 1 = allowed, 0 = denied.
 */
static const int perm_table[2][9] = {
    /* ROLE_USER  */ { 1, 1, 1, 1, 1, 1, 1, 1, 0 },
    /* ROLE_ADMIN */ { 1, 1, 1, 1, 1, 1, 1, 1, 1 }
};

int rbac_allow(UserRole role, Permission required)
{
    int ridx = (int)role;
    int pidx = (int)required;

    if (ridx < 0 || ridx >= 2)  return 0;
    if (pidx < 0 || pidx >= 9)  return 0;
    return perm_table[ridx][pidx];
}
