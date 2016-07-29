#include "tcti-type-enum.h"

GType
tcti_type_enum_get_type (void)
{
    static GType tcti_type_enum_type = 0;

    if (!tcti_type_enum_type) {
        static const GEnumValue my_enum_values[] = {
            { TCTI_TYPE_ECHO, "TCTI for a dumb echo service", "echo" },
            { 0, NULL, NULL }
        };

        tcti_type_enum_type =
            g_enum_register_static ("TctiTypeEnum", my_enum_values);
    }

    return tcti_type_enum_type;
}
