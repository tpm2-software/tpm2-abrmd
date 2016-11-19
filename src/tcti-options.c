#include <stdio.h>

#include "tcti-options.h"
#include "tcti-echo.h"
#include "tss2-tcti-echo.h"
#ifdef HAVE_TCTI_DEVICE
#include "tcti-device.h"
#endif
#ifdef HAVE_TCTI_SOCKET
#include "tcti-socket.h"
#endif
#include "tcti-type-enum.h"

static gpointer tcti_options_parent_class = NULL;

enum
{
    PROP_0,
    PROP_TCTI_TYPE,
    PROP_ECHO_SIZE,
#ifdef HAVE_TCTI_DEVICE
    PROP_DEVICE_NAME,
#endif
#ifdef HAVE_TCTI_SOCKET
    PROP_SOCKET_ADDRESS,
    PROP_SOCKET_PORT,
#endif
    N_PROPERTIES
};

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

static void
tcti_options_set_property (GObject      *object,
                           guint         property_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
    TctiOptions *self = TCTI_OPTIONS (object);

    g_debug ("tcti_options_set_property");
    switch (property_id) {
    case PROP_TCTI_TYPE:
        g_debug ("  PROP_TCTI_TYPE");
        self->tcti_type = g_value_get_enum (value);
        g_debug ("  value: 0x%x", self->tcti_type);
        break;
    case PROP_ECHO_SIZE:
        g_debug ("  PROP_ECHO_SIZE");
        self->echo_size = g_value_get_uint (value);
        g_debug ("  value: 0x%x", self->echo_size);
        break;
#ifdef HAVE_TCTI_DEVICE
    case PROP_DEVICE_NAME:
        g_free (self->device_name);
        self->device_name = g_value_dup_string (value);
        g_debug ("TctiOptions set device_name: %s", self->device_name);
        break;
#endif
#ifdef HAVE_TCTI_SOCKET
    case PROP_SOCKET_ADDRESS:
        self->socket_address = g_value_dup_string (value);
        g_debug ("TctiOptions set socket_address: %s\n", self->socket_address);
        break;
    case PROP_SOCKET_PORT:
        self->socket_port = g_value_get_int (value);
        g_debug ("TctiOptions set socket_port: %d\n", self->socket_port);
        break;
#endif
    default:
        /* We don't have any other property... */
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}
static void
tcti_options_get_property (GObject    *object,
                           guint       property_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
    TctiOptions *self = TCTI_OPTIONS (object);

    g_debug("tcti_options_get_property");
    switch (property_id) {
    case PROP_TCTI_TYPE:
        g_debug ("  PROP_TCTI_TYPE: 0x%x", self->tcti_type);
        g_value_set_enum (value, self->tcti_type);
        break;
    case PROP_ECHO_SIZE:
        g_debug ("  PROP_ECHO_SIZE: 0x%x", self->echo_size);
        g_value_set_uint (value, self->echo_size);
        break;
#ifdef HAVE_TCTI_DEVICE
    case PROP_DEVICE_NAME:
        g_value_set_string (value, self->device_name);
        break;
#endif
#ifdef HAVE_TCTI_SOCKET
    case PROP_SOCKET_ADDRESS:
        g_value_set_string (value, self->socket_address);
        break;
    case PROP_SOCKET_PORT:
        g_value_set_int (value, self->socket_port);
        break;
#endif
    default:
        /* We don't have any other property... */
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}

/* override the parent finalize method so we can free the data associated with
 * the TctiOptions instance.
 */
static void
tcti_options_finalize (GObject *obj)
{
    TctiOptions *options = TCTI_OPTIONS (obj);

    g_free (options->device_name);
    g_free (options->socket_address);
    if (tcti_options_parent_class)
        G_OBJECT_CLASS (tcti_options_parent_class)->finalize (obj);
}
/* When the class is initialized we set the pointer to our finalize function.
 */
static void
tcti_options_class_init (gpointer klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    if (tcti_options_parent_class == NULL)
        tcti_options_parent_class = g_type_class_peek_parent (klass);

    object_class->finalize     = tcti_options_finalize;
    object_class->set_property = tcti_options_set_property;
    object_class->get_property = tcti_options_get_property;

    obj_properties[PROP_TCTI_TYPE] =
        g_param_spec_enum ("tcti",
                           "TCTI",
                           "TCTI used by tpm2-abrmd to communicate with the TPM",
                           TYPE_TCTI_TYPE_ENUM,
                           TCTI_TYPE_ECHO,
                           G_PARAM_CONSTRUCT | G_PARAM_READWRITE);
    obj_properties[PROP_ECHO_SIZE] =
        g_param_spec_uint ("tcti-echo-size",
                           "Echo buffer size",
                           "Size of buffer to allocate for echo TCTI",
                           TSS2_TCTI_ECHO_MIN_BUF,
                           TSS2_TCTI_ECHO_MAX_BUF,
                           TSS2_TCTI_ECHO_MAX_BUF,
                           G_PARAM_CONSTRUCT | G_PARAM_READWRITE);
#ifdef HAVE_TCTI_DEVICE
    obj_properties[PROP_DEVICE_NAME] =
        g_param_spec_string ("device-name",
                             "Device name",
                             "TPM2 device node",
                             "/dev/tpm0",
                             G_PARAM_CONSTRUCT | G_PARAM_READWRITE);
#endif
#ifdef HAVE_TCTI_SOCKET
    obj_properties[PROP_SOCKET_ADDRESS] =
        g_param_spec_string ("socket-address",
                             "Socket address",
                             "Address for socket TCTI to connect",
                             NULL  /* default value */,
                             G_PARAM_READWRITE);
    obj_properties[PROP_SOCKET_PORT] =
        g_param_spec_int ("socket-port",
                          "Port",
                          "Port for socket TCTI connection",
                          0,
                          65535,
                          2222,
                          G_PARAM_READWRITE);
#endif
    g_object_class_install_properties (object_class,
                                       N_PROPERTIES,
                                       obj_properties);
}
/* Upon first call to *_get_type we register the type with the GType system.
 * We keep a static GType around to speed up future calls.
 */
GType
tcti_options_get_type (void)
{
    static GType type = 0;
    if (type == 0) {
        type = g_type_register_static_simple (G_TYPE_OBJECT,
                                              "TctiOptions",
                                              sizeof (TctiOptionsClass),
                                              (GClassInitFunc) tcti_options_class_init,
                                              sizeof (TctiOptions),
                                              NULL,
                                              0);
    }
    return type;
}

/* A simple structure we use to map the string name for a TCTI to the
 * associated value from the TctiTypeEnum.
 */
typedef struct tcty_map_entry {
    char* name;
    TctiTypeEnum tcti_type;
} tcti_map_entry_t;

/* This array maps the string name for a TCTI to the enumeration value that
 * we use to identify it.
 */
tcti_map_entry_t tcti_map[] = {
    {
        .name      = "echo",
        .tcti_type = TCTI_TYPE_ECHO
    },
#ifdef HAVE_TCTI_DEVICE
    {
        .name      = "device",
        .tcti_type = TCTI_TYPE_DEVICE
    },
#endif
#ifdef HAVE_TCTI_SOCKET
    {
        .name      = "socket",
        .tcti_type = TCTI_TYPE_SOCKET
    },
#endif

};
#define ARRAY_LENGTH(array, type) (sizeof (array) / sizeof (type))

TctiOptions*
tcti_options_new (void)
{
    return TCTI_OPTIONS (g_object_new (TYPE_TCTI_OPTIONS, NULL));
}

/* callback used to handl the --tcti= option 
 * This is how we determin which TCTI the tabrmd will use to communicate with
 * the TPM.
 */
gboolean
tcti_parse_opt_callback (const gchar   *option_name,
                         const gchar   *value,
                         gpointer       data,
                         GError       **error)
{
    int i;
    gboolean ret = FALSE;
    GValue gvalue = { 0, };
    TctiOptions *options = TCTI_OPTIONS (data);
    tcti_map_entry_t *tcti_map_entry;

    g_debug ("tcti_parse_opt_callback");
    for (i = 0; i < ARRAY_LENGTH (tcti_map, tcti_map_entry_t); ++i) {
        tcti_map_entry = &tcti_map[i];
        if (!strcmp (tcti_map_entry->name, value)) {
            g_debug ("tcti name matched: %s", tcti_map_entry->name);
            g_value_init (&gvalue, tcti_type_enum_get_type ());
            g_value_set_enum (&gvalue, tcti_map_entry->tcti_type);
            g_object_set_property (G_OBJECT (options), "tcti", &gvalue);
            ret = TRUE;
        }
    }

    return ret;
}

GOptionGroup*
tcti_options_get_group (TctiOptions *options)
{
    GOptionGroup *group;
    GOptionArg   *opt_arg;

    GOptionEntry entries[] = {
        {
            .long_name       = "tcti",
            .short_name      = 't',
            .flags           = G_OPTION_FLAG_NONE,
            .arg             = G_OPTION_ARG_CALLBACK,
            .arg_data        = tcti_parse_opt_callback,
            .description     = "Downstream TCTI",
            .arg_description = "[echo"
#ifdef HAVE_TCTI_DEVICE
            "|device"
#endif
#ifdef HAVE_TCTI_SOCKET
            "|socket"
#endif
            "]"
        },
        {
            .long_name       = "tcti-echo-size",
            .short_name      = 'e',
            .flags           = G_OPTION_FLAG_NONE,
            .arg             = G_OPTION_ARG_INT,
            .arg_data        = &options->echo_size,
            .description     = "Size of buffer to allocate for echo TCTI",
            .arg_description = "[1024-8192]"
        },
#ifdef HAVE_TCTI_DEVICE
        {
            .long_name       = "tcti-device",
            .short_name      = 'd',
            .flags           = G_OPTION_FLAG_NONE,
            .arg             = G_OPTION_ARG_FILENAME,
            .arg_data        = &options->device_name,
            .description     = "TPM2 device nade, default is /dev/tpm0",
            .arg_description = "/dev/tpm0"
        },
#endif
#ifdef HAVE_TCTI_SOCKET
        {
            .long_name       = "tcti-socket-address",
            .short_name      = 'a',
            .flags           = G_OPTION_FLAG_NONE,
            .arg             = G_OPTION_ARG_STRING,
            .arg_data        = &options->socket_address,
            .description     = "address for socket TCTI",
            .arg_description = "127.0.0.1"
        },
        {
            .long_name       = "tcti-socket-port",
            .short_name      = 'p',
            .flags           = G_OPTION_FLAG_NONE,
            .arg             = G_OPTION_ARG_INT,
            .arg_data        = &options->socket_port,
            .description     = "port for socket TCTI",
            .arg_description = "0-65535",
        },
#endif
        { NULL }
    };

    group = g_option_group_new ("tcti",
                                "TCTI Options",
                                "Show downstream TCTI options",
                                options,
                                NULL);
    if (group == NULL)
        return NULL;
    g_option_group_add_entries (group, entries);

    return group;
}

Tcti*
tcti_options_get_tcti (TctiOptions *self)
{
    g_debug ("tcti_options_get_tcti");
    switch (self->tcti_type) {
    case TCTI_TYPE_ECHO:
        return TCTI (tcti_echo_new (self->echo_size));
#ifdef HAVE_TCTI_DEVICE
    case TCTI_TYPE_DEVICE:
        g_assert (self->device_name);
        return TCTI (tcti_device_new (self->device_name));
#endif
#ifdef HAVE_TCTI_SOCKET
    case TCTI_TYPE_SOCKET:
        g_assert (self->socket_address);
        return TCTI (tcti_socket_new (self->socket_address, self->socket_port));
#endif
    default:
        g_error ("unsupported TCTI type");
        break;
    }
}
