/*
FxSound
Copyright (C) 2025  FxSound LLC

Contributors:
	www.theremino.com (2025)

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "codedefs.h"

#include <stdio.h>
#include "wtypes.h"

#include "slout.h"
#include "pt_defs.h"
#include "reg.h"
#include "pstr.h"
#include "operatingSystem.h"

#define DEF_GLOBAL
#include "u_reg.h"

/*
 * FUNCTION: regReadRegisteredOwner()
 * DESCRIPTION:
 *
 *  Ansi version of regReadRegisteredOwner_Wide()
 */
int PT_DECLSPEC regReadRegisteredOwner(char *cp_reg_owner, int i_length)
{return OKAY;}

/*
 * FUNCTION: regReadRegisteredOwner_Wide()
 * DESCRIPTION:
 *
 *  Read the "Registered Owner" for this machine.
 *  and pass it back.  Stuffs it in the passed buffer.
 */
int PT_DECLSPEC regReadRegisteredOwner_Wide(wchar_t *wcp_reg_owner, int i_length)
{return OKAY;}

/*
 * FUNCTION: regRemoveKey()
 * DESCRIPTION:
 *
 *  Ansi version of regRemoveKey_Wide() 
 */
int PT_DECLSPEC regRemoveKey(int i_key_class, char *cp_keyname)
{return OKAY;}

/*
 * FUNCTION: regRemoveKey_Wide()
 * DESCRIPTION:
 *
 *  Deletes the passed registry key.  
 */
int PT_DECLSPEC regRemoveKey_Wide(int i_key_class, wchar_t *wcp_keyname)
{return OKAY;} 

/*
 * FUNCTION: regCreateKey()
 * DESCRIPTION:
 *
 *  Ansi version of regCreateKey_Wide()
 */
int PT_DECLSPEC regCreateKey(int i_key_class, char *cp_keyname, char *cp_data)
{return OKAY;}

/*
 * FUNCTION: regCreateKey_Wide()
 * DESCRIPTION:
 *
 *  Creates a registry key at the specified location with
 *  the "default" value set to the passed in string value,
 */
int PT_DECLSPEC regCreateKey_Wide(int i_key_class, wchar_t *wcp_keyname, wchar_t *wcp_data)
{return OKAY;}

/*
 * FUNCTION: regReadKey_Wide()
 * DESCRIPTION:
 *
 *  Ansi version of regReadKey_Wide()
 */
int PT_DECLSPEC regReadKey(int i_key_class, char *cp_keyname, 
									int *ip_key_exists_flag, char *cp_data,
									unsigned long ul_buffer_size)
{*ip_key_exists_flag=0;return OKAY;}

/*
 * FUNCTION: regReadKey_Wide()
 * DESCRIPTION:
 *
 *  If the passed in key location exists, the function reads it and
 *  sets the exists flag to one. If the key doesn't exist or is not
 *  accessible, the exists flag is set to zero. 
 *
 *  The passed passed i_key_class can be REG_CLASSES_ROOT, REG_LOCAL_MACHINE, etc.
 *
 *  The passed l_buffer_size must be the size that the cp_data was allocated at.
 */
int PT_DECLSPEC regReadKey_Wide(int i_key_class, wchar_t *wcp_keyname, 
									int *ip_key_exists_flag, wchar_t *wcp_data,
									unsigned long ul_buffer_size)
{*ip_key_exists_flag=0;return OKAY;}

/*
* FUNCTION: regCreateKeyTest_Wide()
* DESCRIPTION:
*
*  Test if the machine can successfully Creates a registry key at the specified location with
*  the "default" value set to the passed in string value.
*
*  NOTE: If the write fails, it does not return NOT_OKAY.  Instead it passes back the success flag.
*/
int PT_DECLSPEC regCreateKeyTest_Wide(int i_key_class, wchar_t *wcp_keyname, wchar_t *wcp_data, int *ip_success_flag)
{*ip_success_flag=0;return OKAY;}