#ifndef foochannelmaphfoo
#define foochannelmaphfoo

/* $Id: channelmap.h 1971 2007-10-28 19:13:50Z lennart $ */

/***
  This file is part of PulseAudio.

  Copyright 2005-2006 Lennart Poettering
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
#include <pulse/cdecl.h>

/** \page channelmap Channel Maps
 *
 * \section overv_sec Overview
 *
 * Channel maps provide a way to associate channels in a stream with a
 * specific speaker position. This relieves applications of having to
 * make sure their channel order is identical to the final output.
 *
 * \section init_sec Initialisation
 *
 * A channel map consists of an array of \ref pa_channel_position values,
 * one for each channel. This array is stored together with a channel count
 * in a pa_channel_map structure.
 *
 * Before filling the structure, the application must initialise it using
 * pa_channel_map_init(). There are also a number of convenience functions
 * for standard channel mappings:
 *
 * \li pa_channel_map_init_mono() - Create a channel map with only mono audio.
 * \li pa_channel_map_init_stereo() - Create a standard stereo mapping.
 * \li pa_channel_map_init_auto() - Create a standard channel map for up to
 *                                  six channels.
 *
 * \section conv_sec Convenience Functions
 *
 * The library contains a number of convenience functions for dealing with
 * channel maps:
 *
 * \li pa_channel_map_valid() - Tests if a channel map is valid.
 * \li pa_channel_map_equal() - Tests if two channel maps are identical.
 * \li pa_channel_map_snprint() - Creates a textual description of a channel
 *                                map.
 */

/** \file
 * Constants and routines for channel mapping handling */

PA_C_DECL_BEGIN

/** A list of channel labels */
typedef enum pa_channel_position {
    PA_CHANNEL_POSITION_INVALID = -1,
    PA_CHANNEL_POSITION_MONO = 0,

    PA_CHANNEL_POSITION_LEFT,
    PA_CHANNEL_POSITION_RIGHT,
    PA_CHANNEL_POSITION_CENTER,

    PA_CHANNEL_POSITION_FRONT_LEFT = PA_CHANNEL_POSITION_LEFT,
    PA_CHANNEL_POSITION_FRONT_RIGHT = PA_CHANNEL_POSITION_RIGHT,
    PA_CHANNEL_POSITION_FRONT_CENTER = PA_CHANNEL_POSITION_CENTER,

    PA_CHANNEL_POSITION_REAR_CENTER,
    PA_CHANNEL_POSITION_REAR_LEFT,
    PA_CHANNEL_POSITION_REAR_RIGHT,

    PA_CHANNEL_POSITION_LFE,
    PA_CHANNEL_POSITION_SUBWOOFER = PA_CHANNEL_POSITION_LFE,

    PA_CHANNEL_POSITION_FRONT_LEFT_OF_CENTER,
    PA_CHANNEL_POSITION_FRONT_RIGHT_OF_CENTER,

    PA_CHANNEL_POSITION_SIDE_LEFT,
    PA_CHANNEL_POSITION_SIDE_RIGHT,

    PA_CHANNEL_POSITION_AUX0,
    PA_CHANNEL_POSITION_AUX1,
    PA_CHANNEL_POSITION_AUX2,
    PA_CHANNEL_POSITION_AUX3,
    PA_CHANNEL_POSITION_AUX4,
    PA_CHANNEL_POSITION_AUX5,
    PA_CHANNEL_POSITION_AUX6,
    PA_CHANNEL_POSITION_AUX7,
    PA_CHANNEL_POSITION_AUX8,
    PA_CHANNEL_POSITION_AUX9,
    PA_CHANNEL_POSITION_AUX10,
    PA_CHANNEL_POSITION_AUX11,
    PA_CHANNEL_POSITION_AUX12,
    PA_CHANNEL_POSITION_AUX13,
    PA_CHANNEL_POSITION_AUX14,
    PA_CHANNEL_POSITION_AUX15,
    PA_CHANNEL_POSITION_AUX16,
    PA_CHANNEL_POSITION_AUX17,
    PA_CHANNEL_POSITION_AUX18,
    PA_CHANNEL_POSITION_AUX19,
    PA_CHANNEL_POSITION_AUX20,
    PA_CHANNEL_POSITION_AUX21,
    PA_CHANNEL_POSITION_AUX22,
    PA_CHANNEL_POSITION_AUX23,
    PA_CHANNEL_POSITION_AUX24,
    PA_CHANNEL_POSITION_AUX25,
    PA_CHANNEL_POSITION_AUX26,
    PA_CHANNEL_POSITION_AUX27,
    PA_CHANNEL_POSITION_AUX28,
    PA_CHANNEL_POSITION_AUX29,
    PA_CHANNEL_POSITION_AUX30,
    PA_CHANNEL_POSITION_AUX31,

    PA_CHANNEL_POSITION_TOP_CENTER,

    PA_CHANNEL_POSITION_TOP_FRONT_LEFT,
    PA_CHANNEL_POSITION_TOP_FRONT_RIGHT,
    PA_CHANNEL_POSITION_TOP_FRONT_CENTER,

    PA_CHANNEL_POSITION_TOP_REAR_LEFT,
    PA_CHANNEL_POSITION_TOP_REAR_RIGHT,
    PA_CHANNEL_POSITION_TOP_REAR_CENTER,

    PA_CHANNEL_POSITION_MAX
} pa_channel_position_t;

/** A list of channel mapping definitions for pa_channel_map_init_auto() */
typedef enum pa_channel_map_def {
    PA_CHANNEL_MAP_AIFF,   /**< The mapping from RFC3551, which is based on AIFF-C */
    PA_CHANNEL_MAP_ALSA,   /**< The default mapping used by ALSA */
    PA_CHANNEL_MAP_AUX,    /**< Only aux channels */
    PA_CHANNEL_MAP_WAVEEX, /**< Microsoft's WAVEFORMATEXTENSIBLE mapping */
    PA_CHANNEL_MAP_OSS,    /**< The default channel mapping used by OSS as defined in the OSS 4.0 API specs */

    PA_CHANNEL_MAP_DEFAULT = PA_CHANNEL_MAP_AIFF /**< The default channel map */
} pa_channel_map_def_t;

/** A channel map which can be used to attach labels to specific
 * channels of a stream. These values are relevant for conversion and
 * mixing of streams */
typedef struct pa_channel_map {
    uint8_t channels; /**< Number of channels */
    pa_channel_position_t map[PA_CHANNELS_MAX]; /**< Channel labels */
} pa_channel_map;

/** Initialize the specified channel map and return a pointer to it */
pa_channel_map* pa_channel_map_init(pa_channel_map *m);

/** Initialize the specified channel map for monoaural audio and return a pointer to it */
pa_channel_map* pa_channel_map_init_mono(pa_channel_map *m);

/** Initialize the specified channel map for stereophonic audio and return a pointer to it */
pa_channel_map* pa_channel_map_init_stereo(pa_channel_map *m);

/** Initialize the specified channel map for the specified number
 * of channels using default labels and return a pointer to it. */
pa_channel_map* pa_channel_map_init_auto(pa_channel_map *m, unsigned channels, pa_channel_map_def_t def);

/** Return a text label for the specified channel position */
const char* pa_channel_position_to_string(pa_channel_position_t pos) PA_GCC_PURE;

/** Return a human readable text label for the specified channel position. \since 0.9.7 */
const char* pa_channel_position_to_pretty_string(pa_channel_position_t pos);

/** The maximum length of strings returned by pa_channel_map_snprint() */
#define PA_CHANNEL_MAP_SNPRINT_MAX 336

/** Make a humand readable string from the specified channel map */
char* pa_channel_map_snprint(char *s, size_t l, const pa_channel_map *map);

/** Parse a channel position list into a channel map structure. \since 0.8.1 */
pa_channel_map *pa_channel_map_parse(pa_channel_map *map, const char *s);

/** Compare two channel maps. Return 1 if both match. */
int pa_channel_map_equal(const pa_channel_map *a, const pa_channel_map *b) PA_GCC_PURE;

/** Return non-zero of the specified channel map is considered valid */
int pa_channel_map_valid(const pa_channel_map *map) PA_GCC_PURE;

PA_C_DECL_END

#endif
