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

#include <locale.h>
#include <libintl.h>
#include <syslog.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <glib-unix.h>
#include <gio/gio.h>

#include "daemon.h"

#define NAME_TO_CLAIM "org.isoftlinux.OSProber"

static GMainLoop *loop;
static gboolean debug = FALSE;

static void
on_bus_acquired(GDBusConnection  *connection,
                const gchar      *name,
                gpointer          user_data)
{
    Daemon *daemon;
    GError *local_error = NULL;
    GError **error = &local_error;

    daemon = daemon_new();
    if (daemon == NULL) {
        g_print("ERROR: failed to initialize daemon\n");
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED,
                    "Failed to initialize daemon");
        goto out;
    }

    syslog(LOG_INFO, "started daemon version %s", PROJECT_VERSION);
    closelog();

 out:
    if (local_error != NULL) {
        g_print("ERROR: %s\n", local_error->message);
        g_clear_error(&local_error);
        g_main_loop_quit(loop);
    }
}

static void
on_name_lost(GDBusConnection  *connection,
             const gchar      *name,
             gpointer          user_data)
{
    g_print("ERROR: got NameLost, exiting\n");
    g_main_loop_quit(loop);
}

static void
log_handler(const gchar   *domain,
            GLogLevelFlags level,
            const gchar   *message,
            gpointer       data)
{
    if ((level & G_LOG_LEVEL_MASK) == G_LOG_LEVEL_DEBUG && !debug)
        return;

    g_log_default_handler(domain, level, message, data);
}

static gboolean
on_signal_quit(gpointer data)
{
    g_main_loop_quit(data);
    return FALSE;
}

int
main(int argc, char *argv[])
{
    GError *error = NULL;
    gint ret = 1;
    GBusNameOwnerFlags flags;
    GOptionContext *context = NULL;
    static gboolean replace;
    static gboolean show_version;
    static GOptionEntry entries[] = {
        { "version", 0, 0, G_OPTION_ARG_NONE, &show_version, N_("Output version information and exit"), NULL },
        { "replace", 0, 0, G_OPTION_ARG_NONE, &replace, N_("Replace existing instance"), NULL },
        { "debug", 0, 0, G_OPTION_ARG_NONE, &debug, N_("Enable debugging code"), NULL },

        { NULL }
    };

    bindtextdomain(GETTEXT_PACKAGE, PROJECT_LOCALEDIR);
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    textdomain(GETTEXT_PACKAGE);

#if !GLIB_CHECK_VERSION(2, 35, 3)
    g_type_init();
#endif

    if (!g_setenv("GIO_USE_VFS", "local", TRUE)) {
#ifdef DEBUG
        g_print("DEBUG: failed to set GIO_USE_VFS enviorment!\n");
#endif
        goto out;
    }

    setlocale(LC_ALL, "");
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");

    context = g_option_context_new("");
    g_option_context_set_translation_domain(context, GETTEXT_PACKAGE);
    g_option_context_set_summary(context, _("Provides D-Bus interfaces for probering OS."));
    g_option_context_add_main_entries(context, entries, NULL);
    if (!g_option_context_parse(context, &argc, &argv, &error)) {
#ifdef DEBUG
        g_print("DEBUG: failed to parse option!\n");
#endif
        goto out;
    }
    g_option_context_free(context); context = NULL;

    if (show_version) {
        g_print("isoft-os-prober-daemon " PROJECT_VERSION "\n");
        ret = 0;
        goto out;
    }

    g_log_set_default_handler(log_handler, NULL);

    flags = G_BUS_NAME_OWNER_FLAGS_ALLOW_REPLACEMENT;
    if (replace)
        flags |= G_BUS_NAME_OWNER_FLAGS_REPLACE;
    g_bus_own_name(G_BUS_TYPE_SYSTEM,
                   NAME_TO_CLAIM,
                   flags,
                   on_bus_acquired,
                   NULL,
                   on_name_lost,
                   NULL,
                   NULL);

    loop = g_main_loop_new(NULL, FALSE);

    g_unix_signal_add(SIGINT, on_signal_quit, loop);
    g_unix_signal_add(SIGTERM, on_signal_quit, loop);
#ifdef DEBUG
    g_print("DEBUG: entering main loop\n");
#endif
    g_main_loop_run(loop);

#ifdef DEBUG
    g_print("DEBUG: exiting\n");
#endif
    g_main_loop_unref(loop);
    ret = 0;

out:
    if (error) g_error_free(error); error = NULL;
    return ret;
}
