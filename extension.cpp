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

#include <icvar.h>

#define DEBUG 0

#include "extension.h"
#include "iplayerinfo.h"

#include "CDetour/detours.h"
#include "CDetour/detourhelpers.h"
#include "extensions/ISDKTools.h"

#include "codepatch/patchmanager.h"
#include "codepatch/autopatch.h"
#include "ladder_safe_drop_patch.h"

#define GAMEDATA_FILE "ladder_rambos"

CDetour *Detour_CTerrorGun_Deploy = NULL;
CDetour *Detour_CTerrorGun_Holster = NULL;
CDetour *Detour_CTerrorGun_Reload = NULL;
CDetour *Detour_CBaseShotgun_Reload = NULL;

CGlobalVars *gpGlobals = NULL;
ICvar *icvar = NULL;
IGameConfig *g_pGameConf = NULL;

LadderRambos g_LadderRambos;
SMEXT_LINK(&g_LadderRambos);

PatchManager s_PatchManager;

int g_iOffsetNextShoveTime;
int g_iOffsetPrimaryAmmoClip;
int g_iOffsetEntityOwner;
int g_iOffsetEntityMovetype;

const int g_iTeamIndexSurvivor = 2;
const int g_iMoveTypeLadder = 9;

float g_fLastBeenOnLadder[65];

ConVar g_CvarVersion("cssladders_version", SMEXT_CONF_VERSION, FCVAR_SPONLY|FCVAR_NOTIFY, "Ladder Rambos Extension Version");
ConVar g_CvarEnabled("cssladders_enabled", "0", FCVAR_CHEAT, "Enable the Survivors to shoot from ladders");
// ConVar g_CvarAllowMelee("cssladders_allow_melee", "1", FCVAR_CHEAT, "");
ConVar g_CvarAllowM2("cssladders_allow_m2", "0", FCVAR_CHEAT, "Allow shoving whilst on a ladder?");
ConVar g_CvarAllowReload("cssladders_allow_reload", "0", FCVAR_CHEAT, "Allow reloading whilst on a ladder?");

DETOUR_DECL_MEMBER0(CTerrorGun_Deploy, void)
{
	if (!g_CvarEnabled.GetBool())
	{
		return DETOUR_MEMBER_CALL(CTerrorGun_Deploy)();
	}

	int client = g_LadderRambos.GetOwnerIndex(this);
	IGamePlayer *player = g_LadderRambos.GetPlayer(client);
	
	DEBUG_LOG("CTerrorGun::Deploy() fired for client %i", client);

	if (g_LadderRambos.IsSurvivor(player) && g_LadderRambos.GetGameTime() - g_fLastBeenOnLadder[client] < 0.3)
	{
		return;
	}
	return DETOUR_MEMBER_CALL(CTerrorGun_Deploy)();
}

DETOUR_DECL_MEMBER1(CTerrorGun_Holster, void, void*, intendedWeapon)
{
	if (!g_CvarEnabled.GetBool())
	{
		return DETOUR_MEMBER_CALL(CTerrorGun_Holster)(intendedWeapon);
	}

	int client = g_LadderRambos.GetOwnerIndex(this);
	IGamePlayer *player = g_LadderRambos.GetPlayer(client);
	
	DEBUG_LOG("CTerrorGun::Holster() fired for client %i", client);
	
	g_fLastBeenOnLadder[client] = 0.0;
	if (g_LadderRambos.IsSurvivor(player) && g_LadderRambos.IsOnLadder(client) && g_LadderRambos.GetPrimaryAmmoClip(this) > 0 )
	{
		DEBUG_LOG("%s has %i ammo in clip", player->GetName(), g_LadderRambos.GetPrimaryAmmoClip(this));

		g_fLastBeenOnLadder[client] = g_LadderRambos.GetGameTime();
		if (!g_CvarAllowM2.GetBool())
		{
			g_LadderRambos.SetNextShoveTime(client, g_LadderRambos.GetGameTime() + 0.3);
		}
		return;
	}
	return DETOUR_MEMBER_CALL(CTerrorGun_Holster)(intendedWeapon);
}

DETOUR_DECL_MEMBER0(CTerrorGun_Reload, void)
{
	if (!g_CvarEnabled.GetBool() || g_CvarAllowReload.GetBool())
	{
		return DETOUR_MEMBER_CALL(CTerrorGun_Reload)();
	}
	
	int client = g_LadderRambos.GetOwnerIndex(this);
	IGamePlayer *player = g_LadderRambos.GetPlayer(client);
	
	DEBUG_LOG("CTerrorGun::Reload() fired for client %i", client);

	if (g_LadderRambos.IsSurvivor(player) && g_LadderRambos.IsOnLadder(client))
	{
		return;
	}
	return DETOUR_MEMBER_CALL(CTerrorGun_Reload)();
}

DETOUR_DECL_MEMBER0(CBaseShotgun_Reload, void)
{
	if (!g_CvarEnabled.GetBool() || g_CvarAllowReload.GetBool())
	{
		return DETOUR_MEMBER_CALL(CBaseShotgun_Reload)();
	}
	
	int client = g_LadderRambos.GetOwnerIndex(this);
	IGamePlayer *player = g_LadderRambos.GetPlayer(client);
	
	DEBUG_LOG("CBaseShotgun::Reload() fired for client %i", client);

	if (g_LadderRambos.IsSurvivor(player) && g_LadderRambos.IsOnLadder(client))
	{
		return;
	}
	return DETOUR_MEMBER_CALL(CBaseShotgun_Reload)();
}

int LadderRambos::GetOwnerIndex(void *weapon)
{
	CBaseHandle &owner = *(CBaseHandle *)((unsigned char *)weapon + g_iOffsetEntityOwner);
	if (!owner.IsValid())
	{
		return -1;
	}
	
	int index = owner.GetEntryIndex();
	if (index > playerhelpers->GetMaxClients())
	{
		return -1;
	}
	return index;
}

IGamePlayer *LadderRambos::GetPlayer(int index)
{
	if (index < 1 || index > playerhelpers->GetMaxClients())
	{
		return NULL;
	}
	
	IGamePlayer *player;
	if ((player = playerhelpers->GetGamePlayer(index)) == NULL)
	{
		return NULL;
	}
	if (!player->IsInGame())
	{
		return NULL;
	}
	return player;
}

bool LadderRambos::IsSurvivor(IGamePlayer *player)
{
	if (player == NULL)
	{
		return false;
	}
	
	if (player->IsFakeClient())
	{
		return false;
	}
	
	IPlayerInfo *info;
	if ((info = player->GetPlayerInfo()) == NULL)
	{
		return false;
	}
	return (info->GetTeamIndex() == g_iTeamIndexSurvivor);
}

bool LadderRambos::IsOnLadder(int client)
{
	CBaseEntity *entity = g_LadderRambos.GetCBaseEntity(client);
	if (entity == NULL)
	{
		return false;
	}
	return (*(uint8_t *)((uint8_t *)entity + g_iOffsetEntityMovetype) == g_iMoveTypeLadder);
}

void LadderRambos::SetNextShoveTime(int client, float time)
{
	(*(float *)((uint8_t *)g_LadderRambos.GetCBaseEntity(client) + g_iOffsetNextShoveTime)) = time;
}

int LadderRambos::GetPrimaryAmmoClip(void *weapon)
{
	return (*(int *)((uint8_t *)weapon + g_iOffsetPrimaryAmmoClip));
}

float LadderRambos::GetGameTime()
{
	return gpGlobals->curtime;
}

CBaseEntity *LadderRambos::GetCBaseEntity(int num)
{
	edict_t *pEdict = PEntityOfEntIndex(num);
	if (!pEdict || pEdict->IsFree())
	{
		return NULL;
	}
	
	IServerUnknown *pUnk;
	if ((pUnk=pEdict->GetUnknown()) == NULL)
	{
		return NULL;
	}

	return pUnk->GetBaseEntity();
}

class BaseAccessor : public IConCommandBaseAccessor
{
public:
	bool RegisterConCommandBase(ConCommandBase *pCommandBase)
	{
		return META_REGCVAR(pCommandBase);
	}
} s_BaseAccessor;

bool LadderRambos::SDK_OnMetamodLoad( ISmmAPI *ismm, char *error, size_t maxlength, bool late )
{
	size_t maxlen=maxlength;
	GET_V_IFACE_CURRENT(GetEngineFactory, engine, IVEngineServer, INTERFACEVERSION_VENGINESERVER);
	GET_V_IFACE_CURRENT(GetEngineFactory, icvar, ICvar, CVAR_INTERFACE_VERSION);
	g_pCVar = icvar;
	gpGlobals = ismm->GetCGlobals();
	
	ConVar_Register(0, &s_BaseAccessor);
	return true;
}

bool LadderRambos::SDK_OnLoad( char *error, size_t maxlength, bool late )
{
	char conf_error[255];
	if (!gameconfs->LoadGameConfigFile(GAMEDATA_FILE, &g_pGameConf, conf_error, sizeof(conf_error)))
	{
		if (!strlen(conf_error))
		{
			snprintf(error, maxlength, "Ladder Rambos -- Could not read %s.txt: %s", GAMEDATA_FILE, conf_error);
		}  
		else 
		{
			snprintf(error, maxlength, "Ladder Rambos -- Could not read %s.txt.", GAMEDATA_FILE);
		}
		return false;
	}

	// Obtain necessary offsets
	if (!g_pGameConf->GetOffset("CTerrorPlayer::m_fNextShoveTime", &g_iOffsetNextShoveTime) || !g_iOffsetNextShoveTime)
	{
		snprintf(error, maxlength, "Ladder Rambos -- Could not obtain offset for CTerrorPlayer::m_fNextShoveTime");
		return false;
	}
	
	if (!g_pGameConf->GetOffset("CBaseCombatWeapon::m_iClip1", &g_iOffsetPrimaryAmmoClip) || !g_iOffsetPrimaryAmmoClip)
	{
		snprintf(error, maxlength, "Ladder Rambos -- Could not obtain offset for CBaseCombatWeapon::m_iClip1");
		return false;
	}
	
	if (!g_pGameConf->GetOffset("CBaseEntity::m_owner", &g_iOffsetEntityOwner) || !g_iOffsetEntityOwner)
	{
		snprintf(error, maxlength, "Ladder Rambos -- Could not obtain offset for CBaseEntity::m_owner");
		return false;
	}

	if (!g_pGameConf->GetOffset("CBaseEntity::movetype", &g_iOffsetEntityMovetype) || !g_iOffsetEntityMovetype)
	{
		snprintf(error, maxlength, "Ladder Rambos -- Could not obtain offset for CBaseEntity::movetype");
		return false;
	}
	
	CDetourManager::Init(g_pSM->GetScriptingEngine(), g_pGameConf);	

	return SetupHooks(error, maxlength);
}

void LadderRambos::SDK_OnAllLoaded()
{
	g_CvarEnabled.InstallChangeCallback(LadderSafeDrop::OnExtensionStateChanged);
	//LadderSafeDrop::Patch();
}

void LadderRambos::SDK_OnUnload()
{
	LadderSafeDrop::Unpatch();
	ConVar_Unregister();
	RemoveHooks();
	gameconfs->CloseGameConfigFile(g_pGameConf);
}

bool LadderRambos::SetupHooks(char *error, size_t maxlength)
{
	Detour_CTerrorGun_Deploy = DETOUR_CREATE_MEMBER(CTerrorGun_Deploy, "CTerrorGun::Deploy");
	if (Detour_CTerrorGun_Deploy) 
	{
		Detour_CTerrorGun_Deploy->EnableDetour();
	}
	else
	{
		snprintf(error, maxlength, "Ladder Rambos -- Could not obtain signature for CTerrorGun::Deploy()");
		ERROR_LOG(error);
		RemoveHooks();
		return false;
	}
	
	Detour_CTerrorGun_Holster = DETOUR_CREATE_MEMBER(CTerrorGun_Holster, "CTerrorGun::Holster");
	if (Detour_CTerrorGun_Holster) 
	{
		Detour_CTerrorGun_Holster->EnableDetour();
	}
	else
	{
		snprintf(error, maxlength, "Ladder Rambos -- Could not obtain signature for CTerrorGun::Holster()");
		ERROR_LOG(error);
		RemoveHooks();
		return false;
	}
	
	Detour_CTerrorGun_Reload = DETOUR_CREATE_MEMBER(CTerrorGun_Reload, "CTerrorGun::Reload");
	if (Detour_CTerrorGun_Reload) 
	{
		Detour_CTerrorGun_Reload->EnableDetour();
	}
	else
	{
		snprintf(error, maxlength, "Ladder Rambos -- Could not obtain signature for CTerrorGun::Reload()");
		ERROR_LOG(error);
		RemoveHooks();
		return false;
	}
	
	Detour_CBaseShotgun_Reload = DETOUR_CREATE_MEMBER(CBaseShotgun_Reload, "CBaseShotgun::Reload");
	if (Detour_CBaseShotgun_Reload) 
	{
		Detour_CBaseShotgun_Reload->EnableDetour();
	}
	else
	{
		snprintf(error, maxlength, "Ladder Rambos -- Could not obtain signature for CBaseShotgun::Reload()");
		ERROR_LOG(error);
		RemoveHooks();
		return false;
	}
	
	return true;
}

void LadderRambos::RemoveHooks()
{
	if (Detour_CTerrorGun_Deploy) 
	{
		Detour_CTerrorGun_Deploy->DisableDetour();
	}
	if (Detour_CTerrorGun_Holster) 
	{
		Detour_CTerrorGun_Holster->DisableDetour();
	}
	if (Detour_CTerrorGun_Reload) 
	{
		Detour_CTerrorGun_Reload->DisableDetour();
	}
	if (Detour_CBaseShotgun_Reload) 
	{
		Detour_CBaseShotgun_Reload->DisableDetour();
	}
}