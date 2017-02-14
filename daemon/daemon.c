/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * Copyright (C) 2017 Leslie Zhai <xiang.zhai@i-soft.com.cn>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib/gi18n.h>

#include "daemon.h"

enum {
    PROP_0,
    PROP_DAEMON_VERSION,
};

struct DaemonPrivate {
    GDBusConnection *bus_connection;
    GHashTable *extension_ifaces;
    GDBusMethodInvocation *context;
};

static void daemon_osprober_iface_init(OSProberOSProberIface *iface);

G_DEFINE_TYPE_WITH_CODE(Daemon, daemon, OSPROBER_TYPE_OSPROBER_SKELETON, G_IMPLEMENT_INTERFACE(OSPROBER_TYPE_OSPROBER, daemon_osprober_iface_init));

#define DAEMON_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE((o), TYPE_DAEMON, DaemonPrivate))

static const GDBusErrorEntry osprober_error_entries[] =
{
    { ERROR_FAILED, "org.isoftlinux.OSProber.Error.Failed" },
    { ERROR_PERMISSION_DENIED, "org.isoftlinux.OSProber.Error.PermissionDenied" },
    { ERROR_NOT_SUPPORTED, "org.isoftlinux.OSProber.Error.NotSupported" }
};

GQuark
error_quark()
{
    static volatile gsize quark_volatile = 0;

    g_dbus_error_register_error_domain("osprober_error",
                                       &quark_volatile,
                                       osprober_error_entries,
                                       G_N_ELEMENTS(osprober_error_entries));

    return (GQuark)quark_volatile;
}
#define ENUM_ENTRY(NAME, DESC) { NAME, "" #NAME "", DESC }

GType
error_get_type()
{
    static GType etype = 0;

    if (etype == 0) {
        static const GEnumValue values[] =
        {
            ENUM_ENTRY(ERROR_FAILED, "Failed"),
            ENUM_ENTRY(ERROR_PERMISSION_DENIED, "PermissionDenied"),
            ENUM_ENTRY(ERROR_NOT_SUPPORTED, "NotSupported"),
            { 0, 0, 0 }
        };
        g_assert(NUM_ERRORS == G_N_ELEMENTS(values) - 1);
        etype = g_enum_register_static("Error", values);
    }
    return etype;
}

static void
daemon_init(Daemon *daemon)
{
    GError *error = NULL;

    daemon->priv = DAEMON_GET_PRIVATE(daemon);
    daemon->priv->extension_ifaces = daemon_read_extension_ifaces();
    daemon->priv->context = NULL;
}

static void
daemon_finalize(GObject *object)
{
    Daemon *daemon = NULL;

    g_return_if_fail(IS_DAEMON(object));
    daemon = DAEMON(object);
    if (daemon->priv->bus_connection) {
        g_object_unref(daemon->priv->bus_connection);
        daemon->priv->bus_connection = NULL;
    }

    G_OBJECT_CLASS(daemon_parent_class)->finalize(object);
}

static gboolean
register_osprober_daemon(Daemon *daemon)
{
    GError *error = NULL;

    daemon->priv->bus_connection = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
    if (daemon->priv->bus_connection == NULL) {
        if (error != NULL) {
            g_error_free(error);
            error = NULL;
        }
        return FALSE;
    }

    if (!g_dbus_interface_skeleton_export(G_DBUS_INTERFACE_SKELETON(daemon),
                                          daemon->priv->bus_connection,
                                          "/org/isoftlinux/OSProber",
                                          &error)) {
        if (error != NULL) {
            g_error_free(error);
            error = NULL;
        }
        return FALSE;
    }

    return TRUE;
}

Daemon *
daemon_new()
{
    Daemon *daemon = DAEMON(g_object_new(TYPE_DAEMON, NULL));
    if (!register_osprober_daemon(DAEMON(daemon))) {
        g_object_unref(daemon);
        daemon = NULL;
        return NULL;
    }

    return daemon;
}

static void
throw_error(GDBusMethodInvocation *context,
            gint                   error_code,
            const gchar           *format,
            ...)
{
    va_list args;
    gchar *message = NULL;

    va_start(args, format);
    message = g_strdup_vprintf(format, args);
    va_end(args);

    g_dbus_method_invocation_return_error(context, ERROR, error_code, "%s", message);

    if (message) g_free(message); message = NULL;
}

static const gchar *
daemon_get_daemon_version(OSProberOSProber *object)
{
    return PROJECT_VERSION;
}

static gboolean 
daemon_probe(OSProberOSProber *object, 
             GDBusMethodInvocation *invocation) 
{
    Daemon *daemon = (Daemon *)object;
    FILE *fptr = NULL;
    char buf[1024];
    char *token = NULL;
    /* os-prober's output alike:
       /dev/sda1:Windows NT/2000/XP:WinNT:chain
       ^-------^ ^----------------^ ^---^ ^---^
         part.    OS name for boot  short May change: type of boot loader
            loader's pretty   name  required. Usually there is only
            output                  a 'linux' style bootloader or
                                    a chain one for other partitions
                                    with their own boot sectors.
     */
    char toks[4][128];
    int status = -1;
    int i;

    system("/usr/bin/os-prober > /tmp/os-prober.txt");
    fptr = fopen("/tmp/os-prober.txt", "r");
    if (fptr) {
        status = 0;
        while (fgets(buf, sizeof(buf), fptr)) {
            token = strtok(buf, ":");
            i = 0;
            while (token) {
                memset(toks[i], 0, sizeof(toks[i]));
                strncpy(toks[i], token, sizeof(toks[i]) - 1);
                token = strtok(NULL, ":");
                i++;
            }
            osprober_osprober_emit_found(g_object_ref(OSPROBER_OSPROBER(daemon)), 
                                         toks[0], 
                                         toks[1], 
                                         toks[2]); /* so we do NOT use the 4th 
                                                      value might changed */
        }
        fclose(fptr);
        fptr = NULL;
    }

    osprober_osprober_emit_finished(g_object_ref(OSPROBER_OSPROBER(daemon)), 
                                    status);

    return TRUE;
}

GHashTable *
daemon_get_extension_ifaces(Daemon *daemon)
{
    return daemon->priv->extension_ifaces;
}

static void
daemon_get_property(GObject    *object,
                    guint       prop_id,
                    GValue     *value,
                    GParamSpec *pspec)
{
    Daemon *daemon = DAEMON(object);
    switch (prop_id) {
    case PROP_DAEMON_VERSION:
        g_value_set_string(value, PROJECT_VERSION);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void
daemon_set_property(GObject      *object,
                    guint         prop_id,
                    const GValue *value,
                    GParamSpec   *pspec)
{
    switch (prop_id) {
    case PROP_DAEMON_VERSION:
        g_assert_not_reached();
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void
daemon_class_init(DaemonClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->finalize = daemon_finalize;
    object_class->get_property = daemon_get_property;
    object_class->set_property = daemon_set_property;

    g_type_class_add_private(klass, sizeof(DaemonPrivate));

    g_object_class_override_property(object_class,
                                     PROP_DAEMON_VERSION,
                                     "daemon-version");
}

static void
daemon_osprober_iface_init(OSProberOSProberIface *iface)
{
    iface->get_daemon_version = daemon_get_daemon_version;
    iface->handle_probe = daemon_probe;
}
