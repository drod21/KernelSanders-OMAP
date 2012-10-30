#ifndef fooversionhfoo /*-*-C-*-*/
#define fooversionhfoo

/* $Id: version.h.in 1426 2007-02-13 15:35:19Z ossman $ */

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

/* WARNING: Make sure to edit the real source file version.h.in! */

#include <pulse/cdecl.h>

/** \file
 * Define header version */

PA_C_DECL_BEGIN

/** Return the version of the header files. Keep in mind that this is
a macro and not a function, so it is impossible to get the pointer of
it. */
#define pa_get_headers_version() ("0.9.10")

/** Return the version of the library the current application is linked to. */
const char* pa_get_library_version(void);

/** The current API version. Version 6 relates to Polypaudio
 * 0.6. Prior versions (i.e. Polypaudio 0.5.1 and older) have
 * PA_API_VERSION undefined.  */
#define PA_API_VERSION 11

/** The current protocol version. Version 8 relates to Polypaudio 0.8/PulseAudio 0.9.
 * \since 0.8 */
#define PA_PROTOCOL_VERSION 12

PA_C_DECL_END

#endif
