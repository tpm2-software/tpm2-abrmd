#include "tcti-type-enum.h"

GType
tcti_type_enum_get_type (void)
{
    static volatile gsize g_define_type_id__volatile = 0;

    if (g_once_init_enter (&g_define_type_id__volatile)) {
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

        GType tcti_type_enum_type =
            g_enum_register_static ("TctiTypeEnum", my_enum_values);
        g_once_init_leave (&g_define_type_id__volatile, tcti_type_enum_type);
    }

    return g_define_type_id__volatile;
}
