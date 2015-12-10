/**
 * vim: set ts=4 :
 * =============================================================================
 * L4D2 Ladder Rambos SourceMod Extension
 * Copyright (C) 2015 Ilya "Visor" Komarov
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, AlliedModders LLC gives you permission to link the
 * code of this program (as well as its derivative works) to "Half-Life 2," the
 * "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
 * by the Valve Corporation.  You must obey the GNU General Public License in
 * all respects for all other code used.  Additionally, AlliedModders LLC grants
 * this exception to all derivative works.  AlliedModders LLC defines further
 * exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
 * or <http://www.sourcemod.net/license.php>.
 *
 * Version: $Id$
 */

#ifndef __LADDER_SAFE_DROP_PATCH_H__
#define __LADDER_SAFE_DROP_PATCH_H__

#include "codepatch/icodepatch.h"

class LadderSafeDrop
{
public:
    /* 
        patch the engine to skip setting addons mode for clients
        so we can do it manually
    */
    static void Patch();

    /*
        remove the previous patch and restore the binary back to normal 
    */
    static void Unpatch();
	
	static void OnExtensionStateChanged( IConVar *var, const char *pOldValue, float flOldValue );
};

#endif // __LADDER_SAFE_DROP_PATCH_H__