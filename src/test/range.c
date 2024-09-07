#include "range.h"
#include "test_util.h"

int rg_safe(struct range *rpt, int begin, int end)
{
    for (int i = begin; i < end; i++) {
        if (!rpt[i].valid) {
            // not valid
            continue;
        }

        for (int k = i + 1; k < end; k++) {
            if (rpt[k].valid && rg_intersect(&rpt[i], &rpt[k])) {
                // not safe!
                return 0;
            }
        }
    }

    // safe.
    return 1;
}

int rg_find(struct range *rarr, struct range *ro, int size)
{
    for (int i = 0; i < size; i++) {
        if (rarr[i].valid && rarr + i != ro && rg_intersect(ro, &rarr[i])) {
            // intersection detected! probably a bug!
            return i;
        }
    }

    // not found, safe.
    return RG_ERR;
}
