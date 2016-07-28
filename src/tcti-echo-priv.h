#ifndef TCTI_ECHO_PRIV_H
#define TCTI_ECHO_PRIV_H

#define TCTI_ECHO_MAGIC 0xd511f126d4656f6d

typedef enum {
    CAN_TRANSMIT,
    CAN_RECEIVE,
} EchoState;

typedef struct {
    TSS2_TCTI_CONTEXT_COMMON_V1  common;
    uint8_t                     *buf;
    size_t                       buf_size;
    size_t                       data_size;
    EchoState                    state;
} TCTI_ECHO_CONTEXT;

#endif /* TCTI_ECHO_PRIV_H */
