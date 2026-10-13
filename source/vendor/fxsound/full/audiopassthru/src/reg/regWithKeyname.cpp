/*
FxSound
Copyright (C) 2025  FxSound LLC

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
#include <stdio.h>
#include "wtypes.h"

#include "codedefs.h"
#include "slout.h"
#include "pt_defs.h"
#include "reg.h"
#include "pstr.h"

#include "u_reg.h"

/*
 * FUNCTION: regCreateKeyWithKeyname_Dword_Wide()
 * DESCRIPTION:
 *
 *  Creates a registry key at the specified location with
 *  the keyname value set to the passed in dword value,
 */
int PT_DECLSPEC regCreateKeyWithKeyname_Dword_Wide(int i_key_class, 
																	wchar_t *wcp_keypath, 
																	wchar_t *wcp_keyname, 
																	unsigned long ul_setting)
{return OKAY;}

/*
 * FUNCTION: regCreateKeyWithKeyname_String_Wide()
 * DESCRIPTION:
 *
 *  Creates a registry key at the specified location with
 *  the keyname value set to the passed in wide string value,
 */
int PT_DECLSPEC regCreateKeyWithKeyname_String_Wide(int i_key_class,
																	 wchar_t *wcp_keypath,
																	 wchar_t *wcp_keyname, 
													             wchar_t *wcp_data)
{return OKAY;}

/*
 * FUNCTION: regReadKeyWithKeyname_String_Wide()
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
int PT_DECLSPEC regReadKeyWithKeyname_String_Wide(int i_key_class, 
																  wchar_t *wcp_subkey, 
																  wchar_t *wcp_keyname, 
																  int *ip_key_exists_flag, 
																  wchar_t *wcp_data, 
																  unsigned long ul_buffer_size)
{*ip_key_exists_flag=0;return OKAY;}

/*
 * FUNCTION: regReadKeyWithKeyname_Dword_Wide()
 * DESCRIPTION:
 *
 *  If the passed in key location exists, the function reads it and
 *  sets the exists flag to one. If the key doesn't exist or is not
 *  accessible, the exists flag is set to zero. 
 *
 *  The passed passed i_key_class can be REG_CLASSES_ROOT, REG_LOCAL_MACHINE, etc.
 */
int PT_DECLSPEC regReadKeyWithKeyname_Dword_Wide(int i_key_class, 
																 wchar_t *wcp_subkey, 
																 wchar_t *wcp_keyname, 
																 int *ip_key_exists_flag, 
																 unsigned long *ulp_value)
{*ip_key_exists_flag=0;*ulp_value=0;return OKAY;}