#ifndef fooscachehfoo
#define fooscachehfoo

/* $Id: scache.h 1426 2007-02-13 15:35:19Z ossman $ */

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

#include <sys/types.h>

#include <pulse/context.h>
#include <pulse/stream.h>
#include <pulse/cdecl.h>

/** \page scache Sample Cache
 *
 * \section overv_sec Overview
 *
 * The sample cache provides a simple way of overcoming high network latencies
 * and reducing bandwidth. Instead of streaming a sound precisely when it
 * should be played, it is stored on the server and only the command to start
 * playing it needs to be sent.
 *
 * \section create_sec Creation
 *
 * To create a sample, the normal stream API is used (see \ref streams). The
 * function pa_stream_connect_upload() will make sure the stream is stored as
 * a sample on the server.
 *
 * To complete the upload, pa_stream_finish_upload() is called and the sample
 * will receive the same name as the stream. If the upload should be aborted,
 * simply call pa_stream_disconnect().
 *
 * \section play_sec Playing samples
 *
 * To play back a sample, simply call pa_context_play_sample():
 *
 * \code
 * pa_operation *o;
 *
 * o = pa_context_play_sample(my_context,
 *                            "sample2",       // Name of my sample
 *                            NULL,            // Use default sink
 *                            PA_VOLUME_NORM,  // Full volume
 *                            NULL,            // Don't need a callback
 *                            NULL
 *                            );
 * if (o)
 *     pa_operation_unref(o);
 * \endcode
 *
 * \section rem_sec Removing samples
 *
 * When a sample is no longer needed, it should be removed on the server to
 * save resources. The sample is deleted using pa_context_remove_sample().
 */

/** \file
 * All sample cache related routines */

PA_C_DECL_BEGIN

/** Make this stream a sample upload stream */
int pa_stream_connect_upload(pa_stream *s, size_t length);

/** Finish the sample upload, the stream name will become the sample name. You cancel a samp
 * le upload by issuing pa_stream_disconnect() */
int pa_stream_finish_upload(pa_stream *s);

/** Play a sample from the sample cache to the specified device. If the latter is NULL use the default sink. Returns an operation object */
pa_operation* pa_context_play_sample(
        pa_context *c               /**< Context */,
        const char *name            /**< Name of the sample to play */,
        const char *dev             /**< Sink to play this sample on */,
        pa_volume_t volume          /**< Volume to play this sample with */ ,
        pa_context_success_cb_t cb  /**< Call this function after successfully starting playback, or NULL */,
        void *userdata              /**< Userdata to pass to the callback */);

/** Remove a sample from the sample cache. Returns an operation object which may be used to cancel the operation while it is running */
pa_operation* pa_context_remove_sample(pa_context *c, const char *name, pa_context_success_cb_t, void *userdata);

PA_C_DECL_END

#endif
