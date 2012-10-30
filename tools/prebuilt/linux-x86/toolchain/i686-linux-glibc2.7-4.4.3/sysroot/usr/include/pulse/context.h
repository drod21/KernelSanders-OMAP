#ifndef foocontexthfoo
#define foocontexthfoo

/* $Id: context.h 1426 2007-02-13 15:35:19Z ossman $ */

/***
  This file is part of PulseAudio.

  Copyright 2004-2006 Lennart Poettering
  Copyright 2006 Pierre Ossman <ossman@cendio.se> for Cendio AB

  PulseAudio is free software; you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as published
  by the Free Software Foundation; either version 2 of the License,
  or (at your option) any later version.

  PulseAudio is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with PulseAudio; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
  USA.
***/

#include <pulse/sample.h>
#include <pulse/def.h>
#include <pulse/mainloop-api.h>
#include <pulse/cdecl.h>
#include <pulse/operation.h>

/** \page async Asynchronous API
 *
 * \section overv_sec Overview
 *
 * The asynchronous API is the native interface to the PulseAudio library.
 * It allows full access to all available functions. This also means that
 * it is rather complex and can take some time to fully master.
 *
 * \section mainloop_sec Main Loop Abstraction
 *
 * The API is based around an asynchronous event loop, or main loop,
 * abstraction. This abstraction contains three basic elements:
 *
 * \li Deferred events - Events that will trigger as soon as possible. Note
 *                       that some implementations may block all other events
 *                       when a deferred event is active.
 * \li I/O events - Events that trigger on file descriptor activities.
 * \li Times events - Events that trigger after a fixed ammount of time.
 *
 * The abstraction is represented as a number of function pointers in the
 * pa_mainloop_api structure.
 *
 * To actually be able to use these functions, an implementation needs to
 * be coupled to the abstraction. There are three of these shipped with
 * PulseAudio, but any other can be used with a minimal ammount of work,
 * provided it supports the three basic events listed above.
 *
 * The implementations shipped with PulseAudio are:
 *
 * \li \subpage mainloop - A minimal but fast implementation based on poll().
 * \li \subpage threaded_mainloop - A special version of the previous
 *                                  implementation where all of PulseAudio's
 *                                  internal handling runs in a separate
 *                                  thread.
 * \li \subpage glib-mainloop - A wrapper around GLIB's main loop. Available
 *                              for both GLIB 1.2 and GLIB 2.x.
 *
 * UNIX signals may be hooked to a main loop using the functions from
 * \ref mainloop-signal.h. These rely only on the main loop abstraction
 * and can therefore be used with any of the implementations.
 *
 * \section refcnt_sec Reference Counting
 *
 * Almost all objects in PulseAudio are reference counted. What that means
 * is that you rarely malloc() or free() any objects. Instead you increase
 * and decrease their reference counts. Whenever an object's reference
 * count reaches zero, that object gets destroy and any resources it uses
 * get freed.
 *
 * The benefit of this design is that an application need not worry about
 * whether or not it needs to keep an object around in case the library is
 * using it internally. If it is, then it has made sure it has its own
 * reference to it.
 *
 * Whenever the library creates an object, it will have an initial
 * reference count of one. Most of the time, this single reference will be
 * sufficient for the application, so all required reference count
 * interaction will be a single call to the objects unref function.
 *
 * \section context_sec Context
 *
 * A context is the basic object for a connection to a PulseAudio server.
 * It multiplexes commands, data streams and events through a single
 * channel.
 *
 * There is no need for more than one context per application, unless
 * connections to multiple servers are needed.
 *
 * \subsection ops_subsec Operations
 *
 * All operations on the context are performed asynchronously. I.e. the
 * client will not wait for the server to complete the request. To keep
 * track of all these in-flight operations, the application is given a
 * pa_operation object for each asynchronous operation.
 *
 * There are only two actions (besides reference counting) that can be
 * performed on a pa_operation: querying its state with
 * pa_operation_get_state() and aborting it with pa_operation_cancel().
 *
 * A pa_operation object is reference counted, so an application must
 * make sure to unreference it, even if it has no intention of using it.
 *
 * \subsection conn_subsec Connecting
 *
 * A context must be connected to a server before any operation can be
 * issued. Calling pa_context_connect() will initiate the connection
 * procedure. Unlike most asynchronous operations, connecting does not
 * result in a pa_operation object. Instead, the application should
 * register a callback using pa_context_set_state_callback().
 *
 * \subsection disc_subsec Disconnecting
 *
 * When the sound support is no longer needed, the connection needs to be
 * closed using pa_context_disconnect(). This is an immediate function that
 * works synchronously.
 *
 * Since the context object has references to other objects it must be
 * disconnected after use or there is a high risk of memory leaks. If the
 * connection has terminated by itself, then there is no need to explicitly
 * disconnect the context using pa_context_disconnect().
 *
 * \section Functions
 *
 * The sound server's functionality can be divided into a number of
 * subsections:
 *
 * \li \subpage streams
 * \li \subpage scache
 * \li \subpage introspect
 * \li \subpage subscribe
 */

/** \file
 * Connection contexts for asynchrononous communication with a
 * server. A pa_context object wraps a connection to a PulseAudio
 * server using its native protocol. */

/** \example pacat.c
 * A playback and recording tool using the asynchronous API */

/** \example paplay.c
 * A sound file playback tool using the asynchronous API, based on libsndfile */

PA_C_DECL_BEGIN

/** An opaque connection context to a daemon */
typedef struct pa_context pa_context;

/** Generic notification callback prototype */
typedef void (*pa_context_notify_cb_t)(pa_context *c, void *userdata);

/** A generic callback for operation completion */
typedef void (*pa_context_success_cb_t) (pa_context *c, int success, void *userdata);

/** Instantiate a new connection context with an abstract mainloop API
 * and an application name */
pa_context *pa_context_new(pa_mainloop_api *mainloop, const char *name);

/** Decrease the reference counter of the context by one */
void pa_context_unref(pa_context *c);

/** Increase the reference counter of the context by one */
pa_context* pa_context_ref(pa_context *c);

/** Set a callback function that is called whenever the context status changes */
void pa_context_set_state_callback(pa_context *c, pa_context_notify_cb_t cb, void *userdata);

/** Return the error number of the last failed operation */
int pa_context_errno(pa_context *c);

/** Return non-zero if some data is pending to be written to the connection */
int pa_context_is_pending(pa_context *c);

/** Return the current context status */
pa_context_state_t pa_context_get_state(pa_context *c);

/** Connect the context to the specified server. If server is NULL,
connect to the default server. This routine may but will not always
return synchronously on error. Use pa_context_set_state_callback() to
be notified when the connection is established. If flags doesn't have
PA_NOAUTOSPAWN set and no specific server is specified or accessible a
new daemon is spawned. If api is non-NULL, the functions specified in
the structure are used when forking a new child process. */
int pa_context_connect(pa_context *c, const char *server, pa_context_flags_t flags, const pa_spawn_api *api);

/** Terminate the context connection immediately */
void pa_context_disconnect(pa_context *c);

/** Drain the context. If there is nothing to drain, the function returns NULL */
pa_operation* pa_context_drain(pa_context *c, pa_context_notify_cb_t cb, void *userdata);

/** Tell the daemon to exit. The returned operation is unlikely to
 * complete succesfully, since the daemon probably died before
 * returning a success notification */
pa_operation* pa_context_exit_daemon(pa_context *c, pa_context_success_cb_t cb, void *userdata);

/** Set the name of the default sink. \since 0.4 */
pa_operation* pa_context_set_default_sink(pa_context *c, const char *name, pa_context_success_cb_t cb, void *userdata);

/** Set the name of the default source. \since 0.4 */
pa_operation* pa_context_set_default_source(pa_context *c, const char *name, pa_context_success_cb_t cb, void *userdata);

/** Returns 1 when the connection is to a local daemon. Returns negative when no connection has been made yet. \since 0.5 */
int pa_context_is_local(pa_context *c);

/** Set a different application name for context on the server. \since 0.5 */
pa_operation* pa_context_set_name(pa_context *c, const char *name, pa_context_success_cb_t cb, void *userdata);

/** Return the server name this context is connected to. \since 0.7 */
const char* pa_context_get_server(pa_context *c);

/** Return the protocol version of the library. \since 0.8 */
uint32_t pa_context_get_protocol_version(pa_context *c);

/** Return the protocol version of the connected server. \since 0.8 */
uint32_t pa_context_get_server_protocol_version(pa_context *c);

PA_C_DECL_END

#endif
