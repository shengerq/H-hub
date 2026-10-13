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

/* dfxpSession.cpp */

#include "codedefs.h"

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#include "u_dfxp.h" 

#include "dfxp.h"
#include "reg.h"
#include "DfxSdk.h"
#include "mth.h"

/*
 * FUNCTION: dfxp_SessionWriteIntegerValue() 
 * DESCRIPTION:
 *   Saves the passed integer session value in the registry.
 */
int dfxp_SessionWriteIntegerValue(PT_HANDLE *hp_dfxp, wchar_t *wcp_key_name, int i_value)
{
auto h=(dfxpHdlType*)hp_dfxp;if(!h||!wcp_key_name)return NOT_OKAY;int i=0;for(;i<h->hhub_session_count;i++)if(wcscmp(h->hhub_session_names[i],wcp_key_name)==0)break;if(i==h->hhub_session_count){if(i>=64||wcslen(wcp_key_name)>=128)return NOT_OKAY;wcscpy_s(h->hhub_session_names[i],128,wcp_key_name);++h->hhub_session_count;}h->hhub_session_values[i]=i_value;return OKAY;
}

/*
 * FUNCTION: dfxp_SessionReadIntegerValue() 
 * DESCRIPTION:
 *   Reads the session integer value from the registry.
 *
 *   If the last session info does not exist or if this is the first time
 *   run since installation, set to the default values.
 */
int dfxp_SessionReadIntegerValue(PT_HANDLE *hp_dfxp, wchar_t *wcp_key_name, int i_default_value, int *ip_value)
{
auto h=(dfxpHdlType*)hp_dfxp;if(!h||!wcp_key_name||!ip_value)return NOT_OKAY;*ip_value=i_default_value;for(int i=0;i<h->hhub_session_count;i++)if(wcscmp(h->hhub_session_names[i],wcp_key_name)==0){*ip_value=(int)h->hhub_session_values[i];break;}return OKAY;
}

/*
 * FUNCTION: dfxp_SessionWriteRealValue() 
 * DESCRIPTION:
 *   Saves the passed real session value in the registry.
 */
int dfxp_SessionWriteRealValue(PT_HANDLE *hp_dfxp, wchar_t *wcp_key_name, realtype r_value)
{
auto h=(dfxpHdlType*)hp_dfxp;if(!h||!wcp_key_name)return NOT_OKAY;int i=0;for(;i<h->hhub_session_count;i++)if(wcscmp(h->hhub_session_names[i],wcp_key_name)==0)break;if(i==h->hhub_session_count){if(i>=64||wcslen(wcp_key_name)>=128)return NOT_OKAY;wcscpy_s(h->hhub_session_names[i],128,wcp_key_name);++h->hhub_session_count;}h->hhub_session_values[i]=r_value;return OKAY;
}

/*
 * FUNCTION: dfxp_SessionReadRealValue() 
 * DESCRIPTION:
 *   Reads the session real value from the registry.
 *
 *   If the last session info does not exist or if this is the first time
 *   run since installation, set to the default values.
 */
int dfxp_SessionReadRealValue(PT_HANDLE *hp_dfxp, wchar_t *wcp_key_name, realtype r_default_value, realtype *rp_value)
{
auto h=(dfxpHdlType*)hp_dfxp;if(!h||!wcp_key_name||!rp_value)return NOT_OKAY;*rp_value=r_default_value;for(int i=0;i<h->hhub_session_count;i++)if(wcscmp(h->hhub_session_names[i],wcp_key_name)==0){*rp_value=(realtype)h->hhub_session_values[i];break;}return OKAY;
}


