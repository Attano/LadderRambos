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

#include "extension.h"
#include "ladder_safe_drop_patch.h"
#include "CDetour/detourhelpers.h"

static void *pCTerrorPlayer_PreThink = NULL;
static int offset;
static patch_t pCTerrorPlayer_PreThinkRestore;

const int g_iVerificationByte = 0x09;

static bool isPatched = false;

void LadderSafeDrop::Patch()
{
    DEBUG_LOG("LadderSafeDrop - Patching ...");
    
    bool firstTime = (pCTerrorPlayer_PreThink == NULL);
    if (firstTime)
    {
        if (!g_pGameConf->GetMemSig("CTerrorPlayer::PreThink", &pCTerrorPlayer_PreThink) || !pCTerrorPlayer_PreThink) 
        { 
            ERROR_LOG("Ladder Rambos -- Could not obtain signature for CTerrorPlayer::PreThink()");
            return;
        }

		if (!g_pGameConf->GetOffset("CTerrorPlayer::PreThink__SafeDropLogic", &offset) || !offset)
		{
			ERROR_LOG("Ladder Rambos -- Could not obtain offset for CTerrorPlayer::PreThink__SafeDropLogic");
			return;
		}
		
		// DEBUG_LOG("offset value: %i", *(pCTerrorPlayer_PreThink + offset));
		/*
		// check offset
		if (*(pCTerrorPlayer_PreThink + offset) != g_iVerificationByte)
		{
			ERROR_LOG("Ladder Rambos -- Safe Ladder Drop offset seems to be incorrect");
			return;
		}
		*/
    }
	
    patch_t safeDropPatch;
    safeDropPatch.bytes = 1;

    // MOVETYPE_LADDER (9) -> garbage value
    safeDropPatch.patch[0] = 0x14;

    ApplyPatch(pCTerrorPlayer_PreThink, /*offset*/offset, &safeDropPatch, /*restore*/firstTime ? &pCTerrorPlayer_PreThinkRestore : NULL);
	
    DEBUG_LOG("LadderSafeDrop -- 'SafeDropLogic' patched to garbage value");
}

void LadderSafeDrop::Unpatch()
{
    DEBUG_LOG("LadderSafeDrop - Unpatching ...");

    if (pCTerrorPlayer_PreThink)
    {
        ApplyPatch(pCTerrorPlayer_PreThink, /*offset*/offset, &pCTerrorPlayer_PreThinkRestore, /*restore*/NULL);
        DEBUG_LOG("LadderSafeDrop -- 'SafeDropLogic' restored");
    }
}

void LadderSafeDrop::OnExtensionStateChanged( IConVar *var, const char *pOldValue, float flOldValue )
{
    if (isPatched == g_CvarEnabled.GetBool())
        return;

    DEBUG_LOG("CVAR cssladders_enabled changed to %i...", g_CvarEnabled.GetBool());
    
    if (!isPatched)
    {
        DEBUG_LOG("Enabling LadderSafeDrop patch...");
        LadderSafeDrop::Patch();
    }
    else
    {
        DEBUG_LOG("Disabling LadderSafeDrop patch...");
        LadderSafeDrop::Unpatch();
    }
	
	isPatched = g_CvarEnabled.GetBool();
}