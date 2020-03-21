/*
===========================================================================
Copyright (C) 2005 - 2015, ioquake3 contributors
Copyright (C) 2013 - 2015, OpenJK contributors

This file is part of the OpenJK source code.

OpenJK is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <http://www.gnu.org/licenses/>.
===========================================================================
*/

#pragma once

#include "qcommon/q_shared.hpp"

#define MAXPRINTMSG 4096

typedef enum
{
	NA_BOT,
	NA_BAD,					// an address lookup failed
	NA_LOOPBACK,
	NA_BROADCAST,
	NA_IP,
	NA_IPX,
	NA_BROADCAST_IPX
} netadrtype_t;

/*
typedef struct netadr_s
{
	netadrtype_t	type;
	union {
		byte		ip[4];
		int32_t		ipi;
	};
	uint16_t	port;
} netadr_t;
*/

typedef struct
{
	netadrtype_t	type;

	byte	ip[4];
	byte	ipx[10];

	unsigned short	port;
} netadr_t;

/*
==============================================================

NON-PORTABLE SYSTEM SERVICES

==============================================================
*/

typedef enum {
	AXIS_SIDE,
	AXIS_FORWARD,
	AXIS_UP,
	AXIS_ROLL,
	AXIS_YAW,
	AXIS_PITCH,
	MAX_JOYSTICK_AXIS
} joystickAxis_t;

typedef enum {
  // bk001129 - make sure SE_NONE is zero
	SE_NONE = 0,	// evTime is still valid
	SE_KEY,		// evValue is a key code, evValue2 is the down flag
	SE_CHAR,	// evValue is an ascii char
	SE_MOUSE,	// evValue and evValue2 are reletive signed x / y moves
	SE_JOYSTICK_AXIS,	// evValue is an axis number and evValue2 is the current state (-127 to 127)
	SE_CONSOLE,	// evPtr is a char*
	SE_AIO_FCLOSE,  // evPtr is a pointer to fsh[h]
	SE_MAX
} sysEventType_t;

typedef struct sysEvent_s {
	int				evTime;
	sysEventType_t	evType;
	int				evValue, evValue2;
	int				evPtrLength;	// bytes of data pointed to by evPtr, for journaling
	void			*evPtr;			// this must be manually freed if not NULL
} sysEvent_t;

typedef enum graphicsApi_e
{
	GRAPHICS_API_GENERIC,

	// Only OpenGL needs special treatment..
	GRAPHICS_API_OPENGL,
} graphicsApi_t;

// Graphics API
typedef struct window_s
{
	void *handle; // OS-dependent window handle
	graphicsApi_t api;
} window_t;

typedef enum glProfile_e
{
	GLPROFILE_COMPATIBILITY,
	GLPROFILE_CORE,
	GLPROFILE_ES,
} glProfile_t;

typedef enum glContextFlag_e
{
	GLCONTEXT_DEBUG = (1 << 1),
} glContextFlag_t;

typedef struct windowDesc_s
{
	graphicsApi_t api;

	// Only used if api == GRAPHICS_API_OPENGL
	struct gl_
	{
		int majorVersion;
		int minorVersion;
		glProfile_t profile;
		uint32_t contextFlags;
	} gl;
} windowDesc_t;