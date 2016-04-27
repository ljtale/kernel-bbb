/*
 * Potentially helpful debug functions @ljtale
 */

#include <linux/universal-drv.h>

char *universal_req_type_str(enum universal_req_type type) {
    switch (type) {
        case REGMAP_INIT:
            return "regmap init";
        case DEVM_ALLOCATE:
            return "devm allocation";
        case OF_NODE_MATCH:
            return "of node match";
        default:
            return "invalid type";
    }
}
EXPORT_SYMBOL_GPL(universal_req_type_str);
