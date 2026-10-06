#include "canNodeIds.h"

unsigned char can_node_id_for_unit(can_unit_t unit)
{
    switch (unit) {
    case CAN_UNIT_INSPECTION:     return CANID_INSPECTION;
    case CAN_UNIT_FBE_COATER:     return CANID_FBE_COATER;
    case CAN_UNIT_LIQUID_COATER:  return CANID_LIQUID_COATER;
    case CAN_UNIT_CLEANER:        return CANID_CLEANER;
    default:                      return CANID_NONE;
    }
}