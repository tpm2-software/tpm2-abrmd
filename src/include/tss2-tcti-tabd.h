#ifndef TSS2_TCTI_TABD_H
#define TSS2_TCTI_TABD_H

#include <tss2/tpm20.h>
#include <tss2/tss2_tcti.h>

/* Macros to access data from the TSS2_TCTI_CONTEXT_COMMON_V1 structure */
#define tss2_tcti_context_magic(context) \
    ((TSS2_TCTI_CONTEXT_VERSION*)context)->magic
#define tss2_tcti_context_version(context) \
    ((TSS2_TCTI_CONTEXT_VERSION*)context)->version
#define tss2_tcti_context_transmit(context) \
    ((TSS2_TCTI_CONTEXT_COMMON_V1*)context)->transmit
#define tss2_tcti_context_receive(context) \
    ((TSS2_TCTI_CONTEXT_COMMON_V1*)context)->receive
#define tss2_tcti_context_finalize(context) \
    ((TSS2_TCTI_CONTEXT_COMMON_V1*)context)->finalize
#define tss2_tcti_context_cancel(context) \
    ((TSS2_TCTI_CONTEXT_COMMON_V1*)context)->cancel
#define tss2_tcti_context_get_poll_handles(context) \
    ((TSS2_TCTI_CONTEXT_COMMON_V1*)context)->getPollHandles
#define tss2_tcti_context_set_locality(context) \
    ((TSS2_TCTI_CONTEXT_COMMON_V1*)context)->setLocality

/* Wrapper to sanity test each call through the function pointers in
 * TSS2_TCTI_CONTEXT_COMMON_V1 structure to functions that take only a
 * single parameter. We must do this due to limitations in the c
 * preprocessor that requires at least one parameter for variadic macros.
 */
#define tss2_tcti_command_one(name, context) \
    ((context == NULL) ? \
        TSS2_TCTI_RC_BAD_CONTEXT : \
        (tss2_tcti_context_version (context) < 1) ? \
            TSS2_TCTI_RC_ABI_MISMATCH : \
            (tss2_tcti_context_##name (context) == NULL) ? \
                TSS2_TCTI_RC_NOT_IMPLEMENTED : \
                tss2_tcti_context_##name (context) (context))

/* Wrapper to sanity test parameters for calls through the function
 * pointers in TSS2_TCTI_CONTEXT_COMMON_V1 structure for functions that
 * take more than one parameter.
 */
#define tss2_tcti_command(name, context, ...) \
    ((context == NULL) ? \
        TSS2_TCTI_RC_BAD_CONTEXT : \
        (tss2_tcti_context_version (context) < 1) ? \
            TSS2_TCTI_RC_ABI_MISMATCH : \
            (tss2_tcti_context_##name (context) == NULL) ? \
                TSS2_TCTI_RC_NOT_IMPLEMENTED : \
                tss2_tcti_context_##name (context) (context, __VA_ARGS__))

/* Top level macros to invoke the functions from the common TCTI structure.
 * Use these in your code.
 */
#define tss2_tcti_transmit(context, size, command) \
    tss2_tcti_command (transmit, context, size, command)
#define tss2_tcti_receive(context, size, response, timeout) \
    tss2_tcti_command(receive, context, size, response, timeout)
#define tss2_tcti_finalize(context) \
    tss2_tcti_command_one (finalize, context)
#define tss2_tcti_cancel(context) \
    tss2_tcti_command_one (cancel, context)
#define tss2_tcti_get_poll_handles(context, handles, num_handles) \
    tss2_tcti_command (get_poll_handles, context, handles, num_handles)
#define tss2_tcti_set_locality(context, locality) \
    tss2_tcti_command (set_locality, context, locality)

TSS2_RC tss2_tcti_tabd_init (TSS2_TCTI_CONTEXT *context, size_t *size);

#endif /* TSS2_TCTI_TABD_H */
