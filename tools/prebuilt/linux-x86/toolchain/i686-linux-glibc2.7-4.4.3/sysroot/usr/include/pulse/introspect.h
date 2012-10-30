#ifndef foointrospecthfoo
#define foointrospecthfoo

/* $Id: introspect.h 1971 2007-10-28 19:13:50Z lennart $ */

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

#include <inttypes.h>

#include <pulse/operation.h>
#include <pulse/context.h>
#include <pulse/cdecl.h>
#include <pulse/channelmap.h>
#include <pulse/volume.h>

/** \page introspect Server Query and Control
 *
 * \section overv_sec Overview
 *
 * Sometimes it is necessary to query and modify global settings in the
 * server. For this, PulseAudio has the introspection API. It can list sinks,
 * sources, samples and other aspects of the server. It can also modify the
 * attributes of the server that will affect operations on a global level,
 * and not just the application's context.
 *
 * \section query_sec Querying
 *
 * All querying is done through callbacks. This design is necessary to
 * maintain an asynchronous design. The client will request the information
 * and some time later, the server will respond with the desired data.
 *
 * Some objects can have multiple entries at the server. When requesting all
 * of these at once, the callback will be called multiple times, once for
 * each object. When the list has been exhausted, the callback will be called
 * without an information structure and the eol parameter set to a non-zero
 * value.
 *
 * Note that even if a single object is requested, and not the entire list,
 * the terminating call will still be made.
 *
 * If an error occurs, the callback will be called without and information
 * structure and eol set to zero.
 *
 * Data members in the information structures are only valid during the
 * duration of the callback. If they are required after the callback is
 * finished, a deep copy must be performed.
 *
 * \subsection server_subsec Server Information
 *
 * The server can be queried about its name, the environment it's running on
 * and the currently active global defaults. Calling
 * pa_context_get_server_info() will get access to a pa_server_info structure
 * containing all of these.
 *
 * \subsection memstat_subsec Memory Usage
 *
 * Statistics about memory usage can be fetched using pa_context_stat(),
 * giving a pa_stat_info structure.
 *
 * \subsection sinksrc_subsec Sinks and Sources
 *
 * The server can have an arbitrary number of sinks and sources. Each sink
 * and source have both an index and a name associated with it. As such
 * there are three ways to get access to them:
 *
 * \li By index - pa_context_get_sink_info_by_index() /
 *                pa_context_get_source_info_by_index()
 * \li By name - pa_context_get_sink_info_by_name() /
 *               pa_context_get_source_info_by_name()
 * \li All - pa_context_get_sink_info_list() /
 *           pa_context_get_source_info_list()
 *
 * All three method use the same callback and will provide a pa_sink_info or
 * pa_source_info structure.
 *
 * \subsection siso_subsec Sink Inputs and Source Outputs
 *
 * Sink inputs and source outputs are the representations of the client ends
 * of streams inside the server. I.e. they connect a client stream to one of
 * the global sinks or sources.
 *
 * Sink inputs and source outputs only have an index to identify them. As
 * such, there are only two ways to get information about them:
 *
 * \li By index - pa_context_get_sink_input_info() /
 *                pa_context_get_source_output_info()
 * \li All - pa_context_get_sink_input_info_list() /
 *           pa_context_get_source_output_info_list()
 *
 * The structure returned is the pa_sink_input_info or pa_source_output_info
 * structure.
 *
 * \subsection samples_subsec Samples
 *
 * The list of cached samples can be retrieved from the server. Three methods
 * exist for querying the sample cache list:
 *
 * \li By index - pa_context_get_sample_info_by_index()
 * \li By name - pa_context_get_sample_info_by_name()
 * \li All - pa_context_get_sample_info_list()
 *
 * Note that this only retrieves information about the sample, not the sample
 * data itself.
 *
 * \subsection module_subsec Driver Modules
 *
 * PulseAudio driver modules are identified by index and are retrieved using either
 * pa_context_get_module_info() or pa_context_get_module_info_list(). The
 * information structure is called pa_module_info.
 *
 * \subsection autoload_subsec Autoload Entries
 *
 * Modules can be autoloaded as a result of a client requesting a certain
 * sink or source. This mapping between sink/source names and modules can be
 * queried from the server:
 *
 * \li By index - pa_context_get_autoload_info_by_index()
 * \li By sink/source name - pa_context_get_autoload_info_by_name()
 * \li All - pa_context_get_autoload_info_list()
 *
 * \subsection client_subsec Clients
 *
 * PulseAudio clients are also identified by index and are retrieved using
 * either pa_context_get_client_info() or pa_context_get_client_info_list().
 * The information structure is called pa_client_info.
 *
 * \section ctrl_sec Control
 *
 * Some parts of the server are only possible to read, but most can also be
 * modified in different ways. Note that these changes will affect all
 * connected clients and not just the one issuing the request.
 *
 * \subsection sinksrc_subsec Sinks and Sources
 *
 * The most common change one would want to do to sinks and sources is to
 * modify the volume of the audio. Identical to how sinks and sources can
 * be queried, there are two ways of identifying them:
 *
 * \li By index - pa_context_set_sink_volume_by_index() /
 *                pa_context_set_source_volume_by_index()
 * \li By name - pa_context_set_sink_volume_by_name() /
 *               pa_context_set_source_volume_by_name()
 *
 * It is also possible to mute a sink or source:
 *
 * \li By index - pa_context_set_sink_mute_by_index() /
 *                pa_context_set_source_mute_by_index()
 * \li By name - pa_context_set_sink_mute_by_name() /
 *               pa_context_set_source_mute_by_name()
 *
 * \subsection siso_subsec Sink Inputs and Source Outputs
 *
 * If an application desires to modify the volume of just a single stream
 * (commonly one of its own streams), this can be done by setting the volume
 * of its associated sink input, using pa_context_set_sink_input_volume().
 *
 * There is no support for modifying the volume of source outputs.
 *
 * It is also possible to remove sink inputs and source outputs, terminating
 * the streams associated with them:
 *
 * \li Sink input - pa_context_kill_sink_input()
 * \li Source output - pa_context_kill_source_output()
 *
 * \subsection module_subsec Modules
 *
 * Server modules can be remotely loaded and unloaded using
 * pa_context_load_module() and pa_context_unload_module().
 *
 * \subsection autoload_subsec Autoload Entries
 *
 * New module autoloading rules can be added, and existing can be removed
 * using pa_context_add_autoload() and pa_context_remove_autoload_by_index()
 * / pa_context_remove_autoload_by_name().
 *
 * \subsection client_subsec Clients
 *
 * The only operation supported on clients, is the possibility of kicking
 * them off the server using pa_context_kill_client().
 */

/** \file
 *
 * Routines for daemon introspection.
 */

PA_C_DECL_BEGIN

/** Stores information about sinks */
typedef struct pa_sink_info {
    const char *name;                  /**< Name of the sink */
    uint32_t index;                    /**< Index of the sink */
    const char *description;           /**< Description of this sink */
    pa_sample_spec sample_spec;        /**< Sample spec of this sink */
    pa_channel_map channel_map;        /**< Channel map \since 0.8 */
    uint32_t owner_module;             /**< Index of the owning module of this sink, or PA_INVALID_INDEX */
    pa_cvolume volume;                 /**< Volume of the sink */
    int mute;                          /**< Mute switch of the sink \since 0.8 */
    uint32_t monitor_source;           /**< Index of the monitor source connected to this sink */
    const char *monitor_source_name;   /**< The name of the monitor source */
    pa_usec_t latency;                 /**< Length of filled playback buffer of this sink */
    const char *driver;                /**< Driver name. \since 0.8 */
    pa_sink_flags_t flags;             /**< Flags \since 0.8 */
} pa_sink_info;

/** Callback prototype for pa_context_get_sink_info_by_name() and friends */
typedef void (*pa_sink_info_cb_t)(pa_context *c, const pa_sink_info *i, int eol, void *userdata);

/** Get information about a sink by its name */
pa_operation* pa_context_get_sink_info_by_name(pa_context *c, const char *name, pa_sink_info_cb_t cb, void *userdata);

/** Get information about a sink by its index */
pa_operation* pa_context_get_sink_info_by_index(pa_context *c, uint32_t id, pa_sink_info_cb_t cb, void *userdata);

/** Get the complete sink list */
pa_operation* pa_context_get_sink_info_list(pa_context *c, pa_sink_info_cb_t cb, void *userdata);

/** Stores information about sources */
typedef struct pa_source_info {
    const char *name ;                  /**< Name of the source */
    uint32_t index;                     /**< Index of the source */
    const char *description;            /**< Description of this source */
    pa_sample_spec sample_spec;         /**< Sample spec of this source */
    pa_channel_map channel_map;         /**< Channel map \since 0.8 */
    uint32_t owner_module;              /**< Owning module index, or PA_INVALID_INDEX */
    pa_cvolume volume;                  /**< Volume of the source \since 0.8 */
    int mute;                           /**< Mute switch of the sink \since 0.8 */
    uint32_t monitor_of_sink;           /**< If this is a monitor source the index of the owning sink, otherwise PA_INVALID_INDEX */
    const char *monitor_of_sink_name;   /**< Name of the owning sink, or PA_INVALID_INDEX */
    pa_usec_t latency;                  /**< Length of filled record buffer of this source. \since 0.5 */
    const char *driver;                 /**< Driver name \since 0.8 */
    pa_source_flags_t flags;            /**< Flags \since 0.8 */
} pa_source_info;

/** Callback prototype for pa_context_get_source_info_by_name() and friends */
typedef void (*pa_source_info_cb_t)(pa_context *c, const pa_source_info *i, int eol, void *userdata);

/** Get information about a source by its name */
pa_operation* pa_context_get_source_info_by_name(pa_context *c, const char *name, pa_source_info_cb_t cb, void *userdata);

/** Get information about a source by its index */
pa_operation* pa_context_get_source_info_by_index(pa_context *c, uint32_t id, pa_source_info_cb_t cb, void *userdata);

/** Get the complete source list */
pa_operation* pa_context_get_source_info_list(pa_context *c, pa_source_info_cb_t cb, void *userdata);

/** Server information */
typedef struct pa_server_info {
    const char *user_name;              /**< User name of the daemon process */
    const char *host_name;              /**< Host name the daemon is running on */
    const char *server_version;         /**< Version string of the daemon */
    const char *server_name;            /**< Server package name (usually "pulseaudio") */
    pa_sample_spec sample_spec;         /**< Default sample specification */
    const char *default_sink_name;      /**< Name of default sink. \since 0.4 */
    const char *default_source_name;    /**< Name of default sink. \since 0.4*/
    uint32_t cookie;                    /**< A random cookie for identifying this instance of PulseAudio. \since 0.8 */
} pa_server_info;

/** Callback prototype for pa_context_get_server_info() */
typedef void (*pa_server_info_cb_t) (pa_context *c, const pa_server_info*i, void *userdata);

/** Get some information about the server */
pa_operation* pa_context_get_server_info(pa_context *c, pa_server_info_cb_t cb, void *userdata);

/** Stores information about modules */
typedef struct pa_module_info {
    uint32_t index;                     /**< Index of the module */
    const char*name,                    /**< Name of the module */
        *argument;                      /**< Argument string of the module */
    uint32_t n_used;                    /**< Usage counter or PA_INVALID_INDEX */
    int auto_unload;                    /**< Non-zero if this is an autoloaded module */
} pa_module_info;

/** Callback prototype for pa_context_get_module_info() and firends*/
typedef void (*pa_module_info_cb_t) (pa_context *c, const pa_module_info*i, int eol, void *userdata);

/** Get some information about a module by its index */
pa_operation* pa_context_get_module_info(pa_context *c, uint32_t idx, pa_module_info_cb_t cb, void *userdata);

/** Get the complete list of currently loaded modules */
pa_operation* pa_context_get_module_info_list(pa_context *c, pa_module_info_cb_t cb, void *userdata);

/** Stores information about clients */
typedef struct pa_client_info {
    uint32_t index;                      /**< Index of this client */
    const char *name;                    /**< Name of this client */
    uint32_t owner_module;               /**< Index of the owning module, or PA_INVALID_INDEX */
    const char *driver;                  /**< Driver name \since 0.8 */
} pa_client_info;

/** Callback prototype for pa_context_get_client_info() and firends*/
typedef void (*pa_client_info_cb_t) (pa_context *c, const pa_client_info*i, int eol, void *userdata);

/** Get information about a client by its index */
pa_operation* pa_context_get_client_info(pa_context *c, uint32_t idx, pa_client_info_cb_t cb, void *userdata);

/** Get the complete client list */
pa_operation* pa_context_get_client_info_list(pa_context *c, pa_client_info_cb_t cb, void *userdata);

/** Stores information about sink inputs */
typedef struct pa_sink_input_info {
    uint32_t index;                      /**< Index of the sink input */
    const char *name;                    /**< Name of the sink input */
    uint32_t owner_module;               /**< Index of the module this sink input belongs to, or PA_INVALID_INDEX when it does not belong to any module */
    uint32_t client;                     /**< Index of the client this sink input belongs to, or PA_INVALID_INDEX when it does not belong to any client */
    uint32_t sink;                       /**< Index of the connected sink */
    pa_sample_spec sample_spec;          /**< The sample specification of the sink input */
    pa_channel_map channel_map;          /**< Channel map */
    pa_cvolume volume;                   /**< The volume of this sink input */
    pa_usec_t buffer_usec;               /**< Latency due to buffering in sink input, see pa_latency_info for details */
    pa_usec_t sink_usec;                 /**< Latency of the sink device, see pa_latency_info for details */
    const char *resample_method;         /**< Thre resampling method used by this sink input. \since 0.7 */
    const char *driver;                  /**< Driver name \since 0.8 */
    int mute;                            /**< Stream muted \since 0.9.7 */
} pa_sink_input_info;

/** Callback prototype for pa_context_get_sink_input_info() and firends*/
typedef void (*pa_sink_input_info_cb_t) (pa_context *c, const pa_sink_input_info *i, int eol, void *userdata);

/** Get some information about a sink input by its index */
pa_operation* pa_context_get_sink_input_info(pa_context *c, uint32_t idx, pa_sink_input_info_cb_t cb, void *userdata);

/** Get the complete sink input list */
pa_operation* pa_context_get_sink_input_info_list(pa_context *c, pa_sink_input_info_cb_t cb, void *userdata);

/** Stores information about source outputs */
typedef struct pa_source_output_info {
    uint32_t index;                      /**< Index of the sink input */
    const char *name;                    /**< Name of the sink input */
    uint32_t owner_module;               /**< Index of the module this sink input belongs to, or PA_INVALID_INDEX when it does not belong to any module */
    uint32_t client;                     /**< Index of the client this sink input belongs to, or PA_INVALID_INDEX when it does not belong to any client */
    uint32_t source;                     /**< Index of the connected source */
    pa_sample_spec sample_spec;          /**< The sample specification of the source output */
    pa_channel_map channel_map;          /**< Channel map */
    pa_usec_t buffer_usec;               /**< Latency due to buffering in the source output, see pa_latency_info for details. \since 0.5 */
    pa_usec_t source_usec;               /**< Latency of the source device, see pa_latency_info for details. \since 0.5 */
    const char *resample_method;         /**< Thre resampling method used by this source output. \since 0.7 */
    const char *driver;                  /**< Driver name \since 0.8 */
} pa_source_output_info;

/** Callback prototype for pa_context_get_source_output_info() and firends*/
typedef void (*pa_source_output_info_cb_t) (pa_context *c, const pa_source_output_info *i, int eol, void *userdata);

/** Get information about a source output by its index */
pa_operation* pa_context_get_source_output_info(pa_context *c, uint32_t idx, pa_source_output_info_cb_t cb, void *userdata);

/** Get the complete list of source outputs */
pa_operation* pa_context_get_source_output_info_list(pa_context *c, pa_source_output_info_cb_t cb, void *userdata);

/** Set the volume of a sink device specified by its index */
pa_operation* pa_context_set_sink_volume_by_index(pa_context *c, uint32_t idx, const pa_cvolume *volume, pa_context_success_cb_t cb, void *userdata);

/** Set the volume of a sink device specified by its name */
pa_operation* pa_context_set_sink_volume_by_name(pa_context *c, const char *name, const pa_cvolume *volume, pa_context_success_cb_t cb, void *userdata);

/** Set the mute switch of a sink device specified by its index \since 0.8 */
pa_operation* pa_context_set_sink_mute_by_index(pa_context *c, uint32_t idx, int mute, pa_context_success_cb_t cb, void *userdata);

/** Set the mute switch of a sink device specified by its name \since 0.8 */
pa_operation* pa_context_set_sink_mute_by_name(pa_context *c, const char *name, int mute, pa_context_success_cb_t cb, void *userdata);

/** Set the volume of a sink input stream */
pa_operation* pa_context_set_sink_input_volume(pa_context *c, uint32_t idx, const pa_cvolume *volume, pa_context_success_cb_t cb, void *userdata);

/** Set the mute switch of a sink input stream \since 0.9.7 */
pa_operation* pa_context_set_sink_input_mute(pa_context *c, uint32_t idx, int mute, pa_context_success_cb_t cb, void *userdata);

/** Set the volume of a source device specified by its index \since 0.8 */
pa_operation* pa_context_set_source_volume_by_index(pa_context *c, uint32_t idx, const pa_cvolume *volume, pa_context_success_cb_t cb, void *userdata);

/** Set the volume of a source device specified by its name \since 0.8 */
pa_operation* pa_context_set_source_volume_by_name(pa_context *c, const char *name, const pa_cvolume *volume, pa_context_success_cb_t cb, void *userdata);

/** Set the mute switch of a source device specified by its index \since 0.8 */
pa_operation* pa_context_set_source_mute_by_index(pa_context *c, uint32_t idx, int mute, pa_context_success_cb_t cb, void *userdata);

/** Set the mute switch of a source device specified by its name \since 0.8 */
pa_operation* pa_context_set_source_mute_by_name(pa_context *c, const char *name, int mute, pa_context_success_cb_t cb, void *userdata);

/** Memory block statistics */
typedef struct pa_stat_info {
    uint32_t memblock_total;           /**< Currently allocated memory blocks */
    uint32_t memblock_total_size;      /**< Currentl total size of allocated memory blocks */
    uint32_t memblock_allocated;       /**< Allocated memory blocks during the whole lifetime of the daemon */
    uint32_t memblock_allocated_size;  /**< Total size of all memory blocks allocated during the whole lifetime of the daemon */
    uint32_t scache_size;              /**< Total size of all sample cache entries. \since 0.4 */
} pa_stat_info;

/** Callback prototype for pa_context_stat() */
typedef void (*pa_stat_info_cb_t) (pa_context *c, const pa_stat_info *i, void *userdata);

/** Get daemon memory block statistics */
pa_operation* pa_context_stat(pa_context *c, pa_stat_info_cb_t cb, void *userdata);

/** Stores information about sample cache entries */
typedef struct pa_sample_info {
    uint32_t index;                       /**< Index of this entry */
    const char *name;                     /**< Name of this entry */
    pa_cvolume volume;                    /**< Default volume of this entry */
    pa_sample_spec sample_spec;           /**< Sample specification of the sample */
    pa_channel_map channel_map;           /**< The channel map */
    pa_usec_t duration;                   /**< Duration of this entry */
    uint32_t bytes;                       /**< Length of this sample in bytes. \since 0.4 */
    int lazy;                             /**< Non-zero when this is a lazy cache entry. \since 0.5 */
    const char *filename;                 /**< In case this is a lazy cache entry, the filename for the sound file to be loaded on demand. \since 0.5 */
} pa_sample_info;

/** Callback prototype for pa_context_get_sample_info_by_name() and firends */
typedef void (*pa_sample_info_cb_t)(pa_context *c, const pa_sample_info *i, int eol, void *userdata);

/** Get information about a sample by its name */
pa_operation* pa_context_get_sample_info_by_name(pa_context *c, const char *name, pa_sample_info_cb_t cb, void *userdata);

/** Get information about a sample by its index */
pa_operation* pa_context_get_sample_info_by_index(pa_context *c, uint32_t idx, pa_sample_info_cb_t cb, void *userdata);

/** Get the complete list of samples stored in the daemon. */
pa_operation* pa_context_get_sample_info_list(pa_context *c, pa_sample_info_cb_t cb, void *userdata);

/** Kill a client. \since 0.5 */
pa_operation* pa_context_kill_client(pa_context *c, uint32_t idx, pa_context_success_cb_t cb, void *userdata);

/** Kill a sink input. \since 0.5 */
pa_operation* pa_context_kill_sink_input(pa_context *c, uint32_t idx, pa_context_success_cb_t cb, void *userdata);

/** Kill a source output. \since 0.5 */
pa_operation* pa_context_kill_source_output(pa_context *c, uint32_t idx, pa_context_success_cb_t cb, void *userdata);

/** Callback prototype for pa_context_load_module() and pa_context_add_autoload() */
typedef void (*pa_context_index_cb_t)(pa_context *c, uint32_t idx, void *userdata);

/** Load a module. \since 0.5 */
pa_operation* pa_context_load_module(pa_context *c, const char*name, const char *argument, pa_context_index_cb_t cb, void *userdata);

/** Unload a module. \since 0.5 */
pa_operation* pa_context_unload_module(pa_context *c, uint32_t idx, pa_context_success_cb_t cb, void *userdata);

/** Type of an autoload entry. \since 0.5 */
typedef enum pa_autoload_type {
    PA_AUTOLOAD_SINK = 0,
    PA_AUTOLOAD_SOURCE = 1
} pa_autoload_type_t;

/** Stores information about autoload entries. \since 0.5 */
typedef struct pa_autoload_info {
    uint32_t index;               /**< Index of this autoload entry */
    const char *name;             /**< Name of the sink or source */
    pa_autoload_type_t type;   /**< Type of the autoload entry */
    const char *module;           /**< Module name to load */
    const char *argument;         /**< Argument string for module */
} pa_autoload_info;

/** Callback prototype for pa_context_get_autoload_info_by_name() and firends */
typedef void (*pa_autoload_info_cb_t)(pa_context *c, const pa_autoload_info *i, int eol, void *userdata);

/** Get info about a specific autoload entry. \since 0.6 */
pa_operation* pa_context_get_autoload_info_by_name(pa_context *c, const char *name, pa_autoload_type_t type, pa_autoload_info_cb_t cb, void *userdata);

/** Get info about a specific autoload entry. \since 0.6 */
pa_operation* pa_context_get_autoload_info_by_index(pa_context *c, uint32_t idx, pa_autoload_info_cb_t cb, void *userdata);

/** Get the complete list of autoload entries. \since 0.5 */
pa_operation* pa_context_get_autoload_info_list(pa_context *c, pa_autoload_info_cb_t cb, void *userdata);

/** Add a new autoload entry. \since 0.5 */
pa_operation* pa_context_add_autoload(pa_context *c, const char *name, pa_autoload_type_t type, const char *module, const char*argument, pa_context_index_cb_t, void* userdata);

/** Remove an autoload entry. \since 0.6 */
pa_operation* pa_context_remove_autoload_by_name(pa_context *c, const char *name, pa_autoload_type_t type, pa_context_success_cb_t cb, void* userdata);

/** Remove an autoload entry. \since 0.6 */
pa_operation* pa_context_remove_autoload_by_index(pa_context *c, uint32_t idx, pa_context_success_cb_t cb, void* userdata);

/** Move the specified sink input to a different sink. \since 0.9.5 */
pa_operation* pa_context_move_sink_input_by_name(pa_context *c, uint32_t idx, char *sink_name, pa_context_success_cb_t cb, void* userdata);

/** Move the specified sink input to a different sink. \since 0.9.5 */
pa_operation* pa_context_move_sink_input_by_index(pa_context *c, uint32_t idx, uint32_t sink_idx, pa_context_success_cb_t cb, void* userdata);

/** Move the specified source output to a different source. \since 0.9.5 */
pa_operation* pa_context_move_source_output_by_name(pa_context *c, uint32_t idx, char *source_name, pa_context_success_cb_t cb, void* userdata);

/** Move the specified source output to a different source. \since 0.9.5 */
pa_operation* pa_context_move_source_output_by_index(pa_context *c, uint32_t idx, uint32_t source_idx, pa_context_success_cb_t cb, void* userdata);

/** Suspend/Resume a sink. \since 0.9.7 */
pa_operation* pa_context_suspend_sink_by_name(pa_context *c, char *sink_name, int suspend, pa_context_success_cb_t cb, void* userdata);

/** Suspend/Resume a sink. If idx is PA_INVALID_INDEX all sinks will be suspended. \since 0.9.7 */
pa_operation* pa_context_suspend_sink_by_index(pa_context *c, uint32_t idx, int suspend,  pa_context_success_cb_t cb, void* userdata);

/** Suspend/Resume a source. \since 0.9.7 */
pa_operation* pa_context_suspend_source_by_name(pa_context *c, char *source_name, int suspend, pa_context_success_cb_t cb, void* userdata);

/** Suspend/Resume a source. If idx is PA_INVALID_INDEX all sources will be suspended. \since 0.9.7 */
pa_operation* pa_context_suspend_source_by_index(pa_context *c, uint32_t idx, int suspend, pa_context_success_cb_t cb, void* userdata);

PA_C_DECL_END

#endif
