#ifndef TSS2_TCTI_ECHO_PRIV_H
#define TSS2_TCTI_ECHO_PRIV_H

typedef enum {
    CAN_TRANSMIT,
    CAN_RECEIVE,
} EchoState;

typedef struct {
    TSS2_TCTI_CONTEXT_COMMON_V1  common;
    size_t                       buf_size;
    size_t                       data_size;
    EchoState                    state;
    uint8_t                      buf[];
} TCTI_ECHO_CONTEXT;

#endif /* TCTI_ECHO_PRIV_H */
