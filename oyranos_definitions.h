/*
 * Oyranos is an open source Colour Management System 
 * 
 * Copyright (C) 2004  Kai-Uwe Behrmann 
 *
 * Autor: Kai-Uwe Behrmann <ku.b@gmx.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 * 
 * -----------------------------------------------------------------------------
 *
 * macros and definitions
 * 
 */

/* Date:      25. 11. 2004 */


#ifndef OYRANOS_DEFINITIONS_H
#define OYRANOS_DEFINITIONS_H


#define OY_DEFAULT_USER_PROFILE_PATH    "~/.color/icc"
#define OY_DEFAULT_SYSTEM_PROFILE__PATH "/usr/share/color/icc"


/*
 * users will usually configure their own settings, while administrators can
 * configure system wide settings
 */

/* --- definitions --- */
#define OY_SLASH                 "/"
#define OY_SLASH_C               '/'
#define OY_KEY                   "sw/oyranos"
#define OY_REGISTRED_PROFILES    OY_KEY "/device_profiles"
#define OY_SYS                   "system/"
#define OY_USER                  "user/"

#ifndef MAX_PATH
#define MAX_PATH 1024
#endif

#define OY_SLEEP "SLEEP"

/* path names */

#define OY_USER_PATHS                OY_USER OY_KEY OY_SLASH "paths"
#define OY_USER_PATH                 "path"


/* --- default profiles --- */

#define OY_DEFAULT_IMAGE_PROFILE     OY_USER OY_KEY OY_SLASH "default" OY_SLASH "profile_image"
#define OY_DEFAULT_WORKSPACE_PROFILE OY_USER OY_KEY OY_SLASH "default" OY_SLASH "profile_workspace"
#define OY_DEFAULT_CMYK_PROFILE      OY_USER OY_KEY OY_SLASH "default" OY_SLASH "profile_cmyk"


/* --- profile checking --- */


/* --- profile access through oyranos --- */


/* device profiles */


// debugging variable - set 0 to off (default), set 1 to switch debugging on
extern int oy_debug;

#endif //OYRANOS_DEFINITIONS_H
