/**
 * This file is part of libosso
 *
 * Copyright (C) 2005-2006 Nokia Corporation. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */

#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <libosso.h>

#include "../../src/osso-internal.h"

#define APP_NAME "test_osso_rpc"
#define APP_VER "0.0.1"
#define LOGFILE "/tmp/"APP_NAME".log"

#define TEST_SERVICE "com.nokia.rpc_test"
#define TEST_OBJECT  "/com/nokia/rpc_test"
#define TEST_IFACE   "com.nokia.rpc_test"

gint cb(const gchar *interface, const gchar *method,
	GArray *arguments, gpointer data, osso_rpc_t *retval);

int main(int nargs, char *argv[])
{
    FILE *f;
    GMainLoop *loop;
    osso_context_t *osso;

    f = fopen(LOGFILE, "a");

    fprintf(f, "launched\n");
    fclose(f);
    loop = g_main_loop_new(NULL, FALSE);

    osso = osso_initialize(APP_NAME, APP_VER, TRUE, NULL);
    if(osso == NULL)
	return 1;

    osso_rpc_set_default_cb_f(osso, (osso_rpc_cb_f*)cb, (gpointer)loop);
    dprint("cb = %p",cb);
    osso_rpc_set_cb_f(osso, TEST_SERVICE, TEST_OBJECT, TEST_IFACE,
		      (osso_rpc_cb_f*)cb,(gpointer)loop);
    g_main_loop_run(loop);
    osso_deinitialize(osso);

    return 0;
}

gint cb(const gchar *interface, const gchar *method,
	GArray *arguments, gpointer data, osso_rpc_t *retval)
{
    gboolean stop = FALSE;
    gint i;
    GMainLoop *loop;
    FILE *f;
    
    dprint("in cb");

    loop = (GMainLoop *)data;
    f = fopen(LOGFILE, "a");
    fprintf(f, "%s.%s(",interface,method);

    dprint("in cb");
    
    if(f == NULL) {
	retval->type = DBUS_TYPE_INT32;
	retval->value.i = OSSO_ERROR;
	return OSSO_OK;
    }
    if (arguments->len < 1) {
	dprint("No arguments! Should not happen.");
    }
    
    for(i=0; i < arguments->len; i++) {
	osso_rpc_t *arg;
	arg = &g_array_index(arguments, osso_rpc_t, i);
	switch(arg->type) {
	  case DBUS_TYPE_INVALID:
	    break;
	  case DBUS_TYPE_INT32:
	    fprintf(f,"%d",arg->value.i);
	    if(arg->value.i == 903) {
		sleep(5);
		stop = TRUE;
	    }
	    else if(arg->value.i == 666)
		stop = TRUE;
	    break;
	  case DBUS_TYPE_UINT32:
	    fprintf(f,"%d",arg->value.u);
	    break;
	  case DBUS_TYPE_BOOLEAN:
	    fprintf(f,"%s",arg->value.b?"TRUE":"FALSE");
	    break;
	  case DBUS_TYPE_DOUBLE:
	    fprintf(f,"%f",arg->value.d);
	    break;
	  case DBUS_TYPE_STRING:
	    fprintf(f,"'%s'",arg->value.s);
	    if(arg->value.s != NULL)
		if(strcmp(arg->value.s,"exit")==0)
		    stop = TRUE;
	    break;
	  default:
	    fprintf(f,"Unknown type: %d",arg->type);
	    break;	    
	}
	if(i < (arguments->len-1))
	    fprintf(f,",");
    }
    fprintf(f, ")\n");
    fprintf(f, "stop = %s\n",stop?"TRUE":"FALSE");
    fclose(f);
    if(strcmp(method,"echo")==0) {
	osso_rpc_t *arg;
	dprint("echo method");
	arg = &g_array_index(arguments, osso_rpc_t, 0);
        if (arg->type == DBUS_TYPE_STRING) {
	    retval->type = DBUS_TYPE_STRING;
	    retval->value.s = g_strdup(arg->value.s);
        } else {
            memcpy(retval, arg, sizeof(osso_rpc_t));
        }
    }
    else if(strcmp(method,"error")==0) {
	dprint("error method");
	retval->type = DBUS_TYPE_STRING;
	retval->value.s = "error";
	return OSSO_ERROR;
    }
    else {
	dprint("%s method",method);
	retval->type = DBUS_TYPE_INT32;
	retval->value.i = OSSO_OK;
    }
    if(stop)
	g_main_loop_quit(loop);

    return OSSO_OK;
}
