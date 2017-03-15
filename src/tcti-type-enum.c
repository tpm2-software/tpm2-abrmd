#include "tcti-type-enum.h"

GType
tcti_type_enum_get_type (void)
{
    static GType tcti_type_enum_type = 0;

    if (!tcti_type_enum_type) {
        static const GEnumValue my_enum_values[] = {
            { TCTI_TYPE_NONE, "No TCTI configured.", "none" },
#ifdef HAVE_TCTI_DEVICE
            { TCTI_TYPE_DEVICE, "TCTI for tpm device node", "device" },
#endif
#ifdef HAVE_TCTI_SOCKET
            { TCTI_TYPE_SOCKET, "TCTI for tcp socket", "socket" },
#endif
            { 0, NULL, NULL }
        };

        tcti_type_enum_type =
            g_enum_register_static ("TctiTypeEnum", my_enum_values);
    }

    return tcti_type_enum_type;
}
