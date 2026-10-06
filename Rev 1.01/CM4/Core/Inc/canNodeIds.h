#ifndef CAN_NODE_IDS_H
#define CAN_NODE_IDS_H

#define CANID_NONE            0x00    /* 0 is not a valid CANopen node ID */
#define CANID_INSPECTION      0x68
#define CANID_FBE_COATER      0x6E
#define CANID_LIQUID_COATER   0x6E
#define CANID_CLEANER         0x48

typedef enum {
    CAN_UNIT_NONE = 0,
    CAN_UNIT_INSPECTION,
    CAN_UNIT_FBE_COATER,
    CAN_UNIT_LIQUID_COATER,
    CAN_UNIT_CLEANER,
    CAN_UNIT_COUNT
} can_unit_t;

unsigned char can_node_id_for_unit(can_unit_t unit);

#endif