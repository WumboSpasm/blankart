// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  d_netcmd.c
/// \brief host/client network commands
///        commands are executed through the command buffer
///	       like console commands, other miscellaneous commands (at the end)

#include "d_player.h"
#include "doomdef.h"

#include "console.h"
#include "command.h"
#include "i_time.h"
#include "i_system.h"
#include "g_game.h"
#include "hu_stuff.h"
#include "g_input.h"
#include "m_menu.h"
#include "p_mobj.h"
#include "r_local.h"
#include "r_skins.h"
#include "p_local.h"
#include "p_setup.h"
#include "s_sound.h"
#include "i_sound.h"
#include "m_misc.h"
#include "am_map.h"
#include "byteptr.h"
#include "d_netfil.h"
#include "p_spec.h"
#include "m_cheat.h"
#include "d_clisrv.h"
#include "d_net.h"
#include "v_video.h"
#include "d_main.h"
#include "m_random.h"
#include "f_finale.h"
#include "filesrch.h"
#include "mserv.h"
#include "z_zone.h"
#include "lua_script.h"
#include "lua_hook.h"
#include "m_cond.h"
#include "m_anigif.h"
#include "md5.h"

// SRB2kart
#include "k_kart.h"
#include "k_battle.h"
#include "k_pwrlv.h"
#include "y_inter.h"
#include "k_color.h"
#include "k_grandprix.h"
#include "k_boss.h"
#include "k_follower.h"
#include "doomstat.h"
#include "deh_tables.h"
#include "m_perfstats.h"

#ifdef NETGAME_DEVMODE
#define CV_RESTRICT CV_NETVAR
#else
#define CV_RESTRICT 0
#endif

#ifdef HAVE_DISCORDRPC
#include "discord.h"
#endif

// ------
// protos
// ------

static void Got_NameAndColor(UINT8 **cp, INT32 playernum);
static void Got_WeaponPref(UINT8 **cp, INT32 playernum);
static void Got_PowerLevel(UINT8 **cp, INT32 playernum);
static void Got_PartyInvite(UINT8 **cp, INT32 playernum);
static void Got_AcceptPartyInvite(UINT8 **cp, INT32 playernum);
static void Got_CancelPartyInvite(UINT8 **cp, INT32 playernum);
static void Got_LeaveParty(UINT8 **cp, INT32 playernum);
static void Got_Mapcmd(UINT8 **cp, INT32 playernum);
static void Got_ExitLevelcmd(UINT8 **cp, INT32 playernum);
static void Got_SetupVotecmd(UINT8 **cp, INT32 playernum);
static void Got_ModifyVotecmd(UINT8 **cp, INT32 playernum);
static void Got_PickVotecmd(UINT8 **cp, INT32 playernum);
static void Got_GiveItemcmd(UINT8 **cp, INT32 playernum);
static void Got_RequestAddfilecmd(UINT8 **cp, INT32 playernum);
static void Got_Addfilecmd(UINT8 **cp, INT32 playernum);
static void Got_Pause(UINT8 **cp, INT32 playernum);
static void Got_Respawn(UINT8 **cp, INT32 playernum);
static void Got_RandomSeed(UINT8 **cp, INT32 playernum);
static void Got_RunSOCcmd(UINT8 **cp, INT32 playernum);
static void Got_Teamchange(UINT8 **cp, INT32 playernum);
static void Got_Clearscores(UINT8 **cp, INT32 playernum);
static void Got_DiscordInfo(UINT8 **cp, INT32 playernum);
static void Got_ScheduleTaskcmd(UINT8 **cp, INT32 playernum);
static void Got_ScheduleClearcmd(UINT8 **cp, INT32 playernum);
static void Got_Automatecmd(UINT8 **cp, INT32 playernum);

static void PointLimit_OnChange(void);
static void TimeLimit_OnChange(void);
static void NumLaps_OnChange(void);
static void Mute_OnChange(void);

static void AutoBalance_OnChange(void);
static void TeamScramble_OnChange(void);

static void NetTimeout_OnChange(void);
static void JoinTimeout_OnChange(void);

static void Lagless_OnChange (void);

static void Gravity_OnChange(void);
static void ForceSkin_OnChange(void);

static void Name_OnChange(void);
static void Name2_OnChange(void);
static void Name3_OnChange(void);
static void Name4_OnChange(void);
static void Skin_OnChange(void);
static void Skin2_OnChange(void);
static void Skin3_OnChange(void);
static void Skin4_OnChange(void);

static void Follower_OnChange(void);
static void Follower2_OnChange(void);
static void Follower3_OnChange(void);
static void Follower4_OnChange(void);
static void Followercolor_OnChange(void);
static void Followercolor2_OnChange(void);
static void Followercolor3_OnChange(void);
static void Followercolor4_OnChange(void);

static void Color_OnChange(void);
static void Color2_OnChange(void);
static void Color3_OnChange(void);
static void Color4_OnChange(void);
static void DummyConsvar_OnChange(void);
static void SoundTest_OnChange(void);

static void KartFrantic_OnChange(void);
static void KartSpeed_OnChange(void);
static void KartEncore_OnChange(void);
static void KartComeback_OnChange(void);
static void KartEliminateLast_OnChange(void);
static void KartRings_OnChange(void);

static void Schedule_OnChange(void);

#ifdef NETGAME_DEVMODE
static void Fishcake_OnChange(void);
#endif

static void Command_Playdemo_f(void);
static void Command_Timedemo_f(void);
static void Command_Stopdemo_f(void);
static void Command_StartMovie_f(void);
static void Command_StopMovie_f(void);
static void Command_Map_f(void);
static void Command_RandomMap(void);
static void Command_RestartLevel(void);
static void Command_ResetCamera_f(void);

static void Command_View_f (void);
static void Command_SetViews_f(void);

static void Command_Invite_f(void);
static void Command_CancelInvite_f(void);
static void Command_AcceptInvite_f(void);
static void Command_RejectInvite_f(void);
static void Command_LeaveParty_f(void);

static void Command_Addfile(void);
static void Command_ListWADS_f(void);
static void Command_ListDoomednums_f(void);
static void Command_RunSOC(void);
static void Command_Pause(void);
static void Command_Respawn(void);

static void Command_Version_f(void);
#ifdef UPDATE_ALERT
static void Command_ModDetails_f(void);
#endif
static void Command_ShowGametype_f(void);
FUNCNORETURN static ATTRNORETURN void Command_Quit_f(void);
static void Command_Playintro_f(void);

static void Command_Displayplayer_f(void);

static void Command_ExitLevel_f(void);
static void Command_Showmap_f(void);
static void Command_Mapmd5_f(void);

static void Command_Teamchange_f(void);
static void Command_Teamchange2_f(void);
static void Command_Teamchange3_f(void);
static void Command_Teamchange4_f(void);

static void Command_ServerTeamChange_f(void);

static void Command_Clearscores_f(void);

// Remote Administration
static void Command_Changepassword_f(void);
static void Command_Login_f(void);
static void Got_Verification(UINT8 **cp, INT32 playernum);
static void Got_Removal(UINT8 **cp, INT32 playernum);
static void Command_Verify_f(void);
static void Command_RemoveAdmin_f(void);
static void Command_MotD_f(void);
static void Got_MotD_f(UINT8 **cp, INT32 playernum);

static void Command_ShowScores_f(void);
static void Command_ShowTime_f(void);

static void Command_Isgamemodified_f(void);
static void Command_Cheats_f(void);
#ifdef _DEBUG
static void Command_Togglemodified_f(void);
static void Command_Archivetest_f(void);
#endif

static void Command_KartGiveItem_f(void);

static void Command_Schedule_Add(void);
static void Command_Schedule_Clear(void);
static void Command_Schedule_List(void);

static void Command_Automate_Set(void);

// =========================================================================
//                           CLIENT VARIABLES
// =========================================================================

static CV_PossibleValue_t usemouse_cons_t[] = {{0, "Off"}, {1, "On"}, {2, "Force"}, {0, NULL}};

#ifdef LJOYSTICK
static CV_PossibleValue_t joyport_cons_t[] = {{1, "/dev/js0"}, {2, "/dev/js1"}, {3, "/dev/js2"},
	{4, "/dev/js3"}, {0, NULL}};
#else
// accept whatever value - it is in fact the joystick device number
#define usejoystick_cons_t NULL
#endif

static CV_PossibleValue_t teamscramble_cons_t[] = {{0, "Off"}, {1, "Random"}, {2, "Points"}, {0, NULL}};

static CV_PossibleValue_t startingliveslimit_cons_t[] = {{1, "MIN"}, {99, "MAX"}, {0, NULL}};

static CV_PossibleValue_t sleeping_cons_t[] = {{0, "MIN"}, {1000/TICRATE, "MAX"}, {0, NULL}};

static CV_PossibleValue_t pause_cons_t[] = {{0, "Server"}, {1, "All"}, {0, NULL}};

consvar_t cv_showinputjoy = CVAR_INIT ("showinputjoy", "Off", 0, CV_OnOff, NULL);

#ifdef NETGAME_DEVMODE
static consvar_t cv_fishcake = CVAR_INIT ("fishcake", "Off", CV_CALL|CV_NOSHOWHELP|CV_RESTRICT, CV_OnOff, Fishcake_OnChange);
#endif
static consvar_t cv_dummyconsvar = CVAR_INIT ("dummyconsvar", "Off", CV_CALL|CV_NOSHOWHELP, CV_OnOff, DummyConsvar_OnChange);

consvar_t cv_restrictskinchange = CVAR_INIT ("restrictskinchange", "No", CV_NETVAR|CV_CHEAT, CV_YesNo, NULL);
consvar_t cv_allowteamchange = CVAR_INIT ("allowteamchange", "Yes", CV_NETVAR, CV_YesNo, NULL);

static CV_PossibleValue_t ingamecap_cons_t[] = {{0, "MIN"}, {MAXPLAYERS-1, "MAX"}, {0, NULL}};
consvar_t cv_ingamecap = CVAR_INIT ("ingamecap", "0", CV_NETVAR, ingamecap_cons_t, NULL);

static CV_PossibleValue_t spectatorreentry_cons_t[] = {{0, "MIN"}, {10*60, "MAX"}, {0, NULL}};
consvar_t cv_spectatorreentry = CVAR_INIT ("spectatorreentry", "30", CV_NETVAR, spectatorreentry_cons_t, NULL);

static CV_PossibleValue_t antigrief_cons_t[] = {{20, "MIN"}, {60, "MAX"}, {0, "Off"}, {0, NULL}};
consvar_t cv_antigrief = CVAR_INIT ("antigrief", "30", CV_NETVAR, antigrief_cons_t, NULL);

consvar_t cv_startinglives = CVAR_INIT ("startinglives", "3", CV_NETVAR|CV_CHEAT|CV_NOSHOWHELP, startingliveslimit_cons_t, NULL);

static CV_PossibleValue_t respawntime_cons_t[] = {{1, "MIN"}, {30, "MAX"}, {0, "Off"}, {0, NULL}};
consvar_t cv_respawntime = CVAR_INIT ("respawndelay", "1", CV_NETVAR|CV_CHEAT, respawntime_cons_t, NULL);

consvar_t cv_seenames = CVAR_INIT ("seenames", "On", CV_SAVE, CV_OnOff, NULL);

// names
consvar_t cv_playername[MAXSPLITSCREENPLAYERS] = {
	CVAR_INIT ("name", "Sonic", CV_SAVE|CV_CALL|CV_NOINIT, NULL, Name_OnChange),
	CVAR_INIT ("name2", "Tails", CV_SAVE|CV_CALL|CV_NOINIT, NULL, Name2_OnChange),
	CVAR_INIT ("name3", "Knuckles", CV_SAVE|CV_CALL|CV_NOINIT, NULL, Name3_OnChange),
	CVAR_INIT ("name4", "Eggman", CV_SAVE|CV_CALL|CV_NOINIT, NULL, Name4_OnChange)
};
// player colors
UINT16 lastgoodcolor[MAXSPLITSCREENPLAYERS] = {SKINCOLOR_BLUE, SKINCOLOR_BLUE, SKINCOLOR_BLUE, SKINCOLOR_BLUE};
consvar_t cv_playercolor[MAXSPLITSCREENPLAYERS] = {
	CVAR_INIT ("color", "Red", CV_SAVE|CV_CALL|CV_NOINIT, Color_cons_t, Color_OnChange),
	CVAR_INIT ("color2", "Orange", CV_SAVE|CV_CALL|CV_NOINIT, Color_cons_t, Color2_OnChange),
	CVAR_INIT ("color3", "Blue", CV_SAVE|CV_CALL|CV_NOINIT, Color_cons_t, Color3_OnChange),
	CVAR_INIT ("color4", "Red", CV_SAVE|CV_CALL|CV_NOINIT, Color_cons_t, Color4_OnChange)
};
// player's skin, saved for commodity, when using a favorite skins wad..
consvar_t cv_skin[MAXSPLITSCREENPLAYERS] = {
	CVAR_INIT ("skin", DEFAULTSKIN, CV_SAVE|CV_CALL|CV_NOINIT, NULL, Skin_OnChange),
	CVAR_INIT ("skin2", DEFAULTSKIN2, CV_SAVE|CV_CALL|CV_NOINIT, NULL, Skin2_OnChange),
	CVAR_INIT ("skin3", DEFAULTSKIN3, CV_SAVE|CV_CALL|CV_NOINIT, NULL, Skin3_OnChange),
	CVAR_INIT ("skin4", DEFAULTSKIN4, CV_SAVE|CV_CALL|CV_NOINIT, NULL, Skin4_OnChange)
};

// player's followers. Also saved.
consvar_t cv_follower[MAXSPLITSCREENPLAYERS] = {
	CVAR_INIT ("follower", "None", CV_SAVE|CV_CALL|CV_NOINIT, NULL, Follower_OnChange),
	CVAR_INIT ("follower2", "None", CV_SAVE|CV_CALL|CV_NOINIT, NULL, Follower2_OnChange),
	CVAR_INIT ("follower3", "None", CV_SAVE|CV_CALL|CV_NOINIT, NULL, Follower3_OnChange),
	CVAR_INIT ("follower4", "None", CV_SAVE|CV_CALL|CV_NOINIT, NULL, Follower4_OnChange)
};

// player's follower colors... Also saved...
consvar_t cv_followercolor[MAXSPLITSCREENPLAYERS] = {
	CVAR_INIT ("followercolor", "1", CV_SAVE|CV_CALL|CV_NOINIT, Followercolor_cons_t, Followercolor_OnChange),
	CVAR_INIT ("followercolor2", "1", CV_SAVE|CV_CALL|CV_NOINIT, Followercolor_cons_t, Followercolor2_OnChange),
	CVAR_INIT ("followercolor3", "1", CV_SAVE|CV_CALL|CV_NOINIT, Followercolor_cons_t, Followercolor3_OnChange),
	CVAR_INIT ("followercolor4", "1", CV_SAVE|CV_CALL|CV_NOINIT, Followercolor_cons_t, Followercolor4_OnChange)
};

consvar_t cv_skipmapcheck = CVAR_INIT ("skipmapcheck", "Off", CV_SAVE, CV_OnOff, NULL);

INT32 cv_debug;

consvar_t cv_usemouse = CVAR_INIT ("use_mouse", "Off", CV_SAVE|CV_CALL,usemouse_cons_t, I_StartupMouse);

consvar_t cv_usejoystick[MAXSPLITSCREENPLAYERS] = {
	CVAR_INIT ("use_gamepad", "1", CV_SAVE|CV_CALL, usejoystick_cons_t, I_InitJoystick1),
	CVAR_INIT ("use_gamepad2", "2", CV_SAVE|CV_CALL, usejoystick_cons_t, I_InitJoystick2),
	CVAR_INIT ("use_joystick3", "3", CV_SAVE|CV_CALL, usejoystick_cons_t, I_InitJoystick3),
	CVAR_INIT ("use_joystick4", "4", CV_SAVE|CV_CALL, usejoystick_cons_t, I_InitJoystick4)
};

#if (defined (LJOYSTICK) || defined (HAVE_SDL))
consvar_t cv_joyscale[MAXSPLITSCREENPLAYERS] = {
	CVAR_INIT ("padscale", "1", CV_SAVE|CV_CALL, NULL, I_JoyScale),
	CVAR_INIT ("padscale2", "1", CV_SAVE|CV_CALL, NULL, I_JoyScale2),
	CVAR_INIT ("padscale3", "1", CV_SAVE|CV_CALL, NULL, I_JoyScale3),
	CVAR_INIT ("padscale4", "1", CV_SAVE|CV_CALL, NULL, I_JoyScale4)
};

#ifdef LJOYSTICK
consvar_t cv_joyport[MAXSPLITSCREENPLAYERS] = { //Alam: for later
	CVAR_INIT ("padport", "/dev/js0", CV_SAVE, joyport_cons_t, NULL),
	CVAR_INIT ("padport2", "/dev/js0", CV_SAVE, joyport_cons_t, NULL),
	CVAR_INIT ("padport3", "/dev/js0", CV_SAVE, joyport_cons_t, NULL),
	CVAR_INIT ("padport4", "/dev/js0", CV_SAVE, joyport_cons_t, NULL)
};
#endif
#else
consvar_t cv_joyscale[MAXSPLITSCREENPLAYERS] = { //Alam: Dummy for save
	CVAR_INIT ("padscale", "1", CV_SAVE|CV_HIDEN, NULL, NULL),
	CVAR_INIT ("padscale2", "1", CV_SAVE|CV_HIDEN, NULL, NULL),
	CVAR_INIT ("padscale3", "1", CV_SAVE|CV_HIDEN, NULL, NULL),
	CVAR_INIT ("padscale4", "1", CV_SAVE|CV_HIDEN, NULL, NULL)
};
#endif

// SRB2kart
consvar_t cv_superring = 			CVAR_INIT ("superring", 		"On", CV_NETVAR|CV_CHEAT, CV_OnOff, NULL);
consvar_t cv_sneaker = 				CVAR_INIT ("sneaker", 			"On", CV_NETVAR|CV_CHEAT, CV_OnOff, NULL);
consvar_t cv_rocketsneaker = 		CVAR_INIT ("rocketsneaker", 	"On", CV_NETVAR|CV_CHEAT, CV_OnOff, NULL);
consvar_t cv_invincibility = 		CVAR_INIT ("invincibility", 	"On", CV_NETVAR|CV_CHEAT, CV_OnOff, NULL);
consvar_t cv_banana = 				CVAR_INIT ("banana", 			"On", CV_NETVAR|CV_CHEAT, CV_OnOff, NULL);
consvar_t cv_eggmanmonitor = 		CVAR_INIT ("eggmanmonitor", 	"On", CV_NETVAR|CV_CHEAT, CV_OnOff, NULL);
consvar_t cv_orbinaut = 			CVAR_INIT ("orbinaut", 			"On", CV_NETVAR|CV_CHEAT, CV_OnOff, NULL);
consvar_t cv_jawz = 				CVAR_INIT ("jawz", 				"On", CV_NETVAR|CV_CHEAT, CV_OnOff, NULL);
consvar_t cv_mine = 				CVAR_INIT ("mine", 				"On", CV_NETVAR|CV_CHEAT, CV_OnOff, NULL);
consvar_t cv_landmine = 			CVAR_INIT ("landmine", 			"On", CV_NETVAR|CV_CHEAT, CV_OnOff, NULL);
consvar_t cv_droptarget = 			CVAR_INIT ("droptarget", 		"On", CV_NETVAR|CV_CHEAT, CV_OnOff, NULL);
consvar_t cv_ballhog = 				CVAR_INIT ("ballhog", 			"On", CV_NETVAR|CV_CHEAT, CV_OnOff, NULL);
consvar_t cv_selfpropelledbomb =	CVAR_INIT ("selfpropelledbomb", "On", CV_NETVAR|CV_CHEAT, CV_OnOff, NULL);
consvar_t cv_grow = 				CVAR_INIT ("grow", 				"On", CV_NETVAR|CV_CHEAT, CV_OnOff, NULL);
consvar_t cv_shrink = 				CVAR_INIT ("shrink", 			"On", CV_NETVAR|CV_CHEAT, CV_OnOff, NULL);
consvar_t cv_lightningshield = 		CVAR_INIT ("lightningshield", 	"On", CV_NETVAR|CV_CHEAT, CV_OnOff, NULL);
consvar_t cv_bubbleshield = 		CVAR_INIT ("bubbleshield", 		"On", CV_NETVAR|CV_CHEAT, CV_OnOff, NULL);
consvar_t cv_flameshield = 			CVAR_INIT ("flameshield", 		"On", CV_NETVAR|CV_CHEAT, CV_OnOff, NULL);
consvar_t cv_hyudoro = 				CVAR_INIT ("hyudoro", 			"On", CV_NETVAR|CV_CHEAT, CV_OnOff, NULL);
consvar_t cv_pogospring = 			CVAR_INIT ("pogospring", 		"On", CV_NETVAR|CV_CHEAT, CV_OnOff, NULL);
consvar_t cv_kitchensink = 			CVAR_INIT ("kitchensink", 		"On", CV_NETVAR|CV_CHEAT, CV_OnOff, NULL);

consvar_t cv_dualsneaker = 			CVAR_INIT ("dualsneaker", 		"On", CV_NETVAR|CV_CHEAT, CV_OnOff, NULL);
consvar_t cv_triplesneaker = 		CVAR_INIT ("triplesneaker", 	"On", CV_NETVAR|CV_CHEAT, CV_OnOff, NULL);
consvar_t cv_triplebanana = 		CVAR_INIT ("triplebanana", 		"On", CV_NETVAR|CV_CHEAT, CV_OnOff, NULL);
consvar_t cv_decabanana = 			CVAR_INIT ("decabanana", 		"On", CV_NETVAR|CV_CHEAT, CV_OnOff, NULL);
consvar_t cv_tripleorbinaut = 		CVAR_INIT ("tripleorbinaut", 	"On", CV_NETVAR|CV_CHEAT, CV_OnOff, NULL);
consvar_t cv_quadorbinaut = 		CVAR_INIT ("quadorbinaut", 		"On", CV_NETVAR|CV_CHEAT, CV_OnOff, NULL);
consvar_t cv_dualjawz = 			CVAR_INIT ("dualjawz", 			"On", CV_NETVAR|CV_CHEAT, CV_OnOff, NULL);

static CV_PossibleValue_t kartminimap_cons_t[] = {{0, "MIN"}, {10, "MAX"}, {0, NULL}};
consvar_t cv_kartminimap = CVAR_INIT ("kartminimap", "4", CV_SAVE, kartminimap_cons_t, NULL);
consvar_t cv_kartcheck = CVAR_INIT ("kartcheck", "Yes", CV_SAVE, CV_YesNo, NULL);
static CV_PossibleValue_t kartinvinsfx_cons_t[] = {{0, "Music"}, {1, "SFX"}, {0, NULL}};
consvar_t cv_kartinvinsfx = CVAR_INIT ("kartinvinsfx", "SFX", CV_SAVE, kartinvinsfx_cons_t, NULL);
consvar_t cv_kartspeed = CVAR_INIT ("kartspeed", "Auto", CV_NETVAR|CV_CALL|CV_NOINIT, kartspeed_cons_t, KartSpeed_OnChange);
static CV_PossibleValue_t kartbumpers_cons_t[] = {{1, "MIN"}, {12, "MAX"}, {0, NULL}};
consvar_t cv_kartbumpers = CVAR_INIT ("kartbumpers", "3", CV_NETVAR|CV_CHEAT, kartbumpers_cons_t, NULL);
consvar_t cv_kartfrantic = CVAR_INIT ("kartfrantic", "Off", CV_NETVAR|CV_CHEAT|CV_CALL|CV_NOINIT, CV_OnOff, KartFrantic_OnChange);
consvar_t cv_kartcomeback = CVAR_INIT ("kartcomeback", "On", CV_NETVAR|CV_CHEAT|CV_CALL|CV_NOINIT, CV_OnOff, KartComeback_OnChange);
static CV_PossibleValue_t kartencore_cons_t[] = {{-1, "Auto"}, {0, "Off"}, {1, "On"}, {0, NULL}};
consvar_t cv_kartencore = CVAR_INIT ("kartencore", "Auto", CV_NETVAR|CV_CALL|CV_NOINIT, kartencore_cons_t, KartEncore_OnChange);
static CV_PossibleValue_t kartvoterulechanges_cons_t[] = {{0, "Never"}, {1, "Sometimes"}, {2, "Frequent"}, {3, "Always"}, {0, NULL}};
consvar_t cv_kartvoterulechanges = CVAR_INIT ("kartvoterulechanges", "Frequent", CV_NETVAR, kartvoterulechanges_cons_t, NULL);
static CV_PossibleValue_t kartspeedometer_cons_t[] = {{0, "Off"}, {1, "Percentage"}, {2, "Kilometers"}, {3, "Miles"}, {4, "Fracunits"}, {0, NULL}};
consvar_t cv_kartspeedometer = CVAR_INIT ("kartdisplayspeed", "Percentage", CV_SAVE, kartspeedometer_cons_t, NULL); // use tics in display
static CV_PossibleValue_t kartvoices_cons_t[] = {{0, "Never"}, {1, "Tasteful"}, {2, "Meme"}, {0, NULL}};
consvar_t cv_kartvoices = CVAR_INIT ("kartvoices", "Tasteful", CV_SAVE, kartvoices_cons_t, NULL);

static CV_PossibleValue_t kartbot_cons_t[] = {
	{0, "Off"},
	{1, "Lv.1"},
	{2, "Lv.2"},
	{3, "Lv.3"},
	{4, "Lv.4"},
	{5, "Lv.5"},
	{6, "Lv.6"},
	{7, "Lv.7"},
	{8, "Lv.8"},
	{9, "Lv.9"},
	{10,"Lv.10"},
	{11,"Lv.11"},
	{12,"Lv.12"},
	{13,"Lv.MAX"},
	{0, NULL}
};
consvar_t cv_kartbot = CVAR_INIT ("kartbot", "0", CV_NETVAR, kartbot_cons_t, NULL);

consvar_t cv_karteliminatelast = CVAR_INIT ("karteliminatelast", "Yes", CV_NETVAR|CV_CHEAT|CV_CALL, CV_YesNo, KartEliminateLast_OnChange);

// Toggles for new features
consvar_t cv_kartrings = CVAR_INIT ("kartrings", "Off", CV_NETVAR|CV_CHEAT|CV_CALL|CV_NOINIT, CV_OnOff, KartRings_OnChange);

consvar_t cv_kartusepwrlv = CVAR_INIT ("kartusepwrlv", "Yes", CV_NETVAR|CV_CHEAT, CV_YesNo, NULL);

static CV_PossibleValue_t kartdebugitem_cons_t[] =
{
#define FOREACH( name, n ) { n, #name }
	KART_ITEM_ITERATOR,
#undef  FOREACH
	{0}
};
consvar_t cv_kartdebugitem = CVAR_INIT ("kartdebugitem", "0", CV_NETVAR|CV_CHEAT|CV_NOSHOWHELP, kartdebugitem_cons_t, NULL);
static CV_PossibleValue_t kartdebugamount_cons_t[] = {{1, "MIN"}, {255, "MAX"}, {0, NULL}};
consvar_t cv_kartdebugamount = CVAR_INIT ("kartdebugamount", "1", CV_NETVAR|CV_CHEAT|CV_NOSHOWHELP, kartdebugamount_cons_t, NULL);
#ifdef DEVELOP
#define VALUE "Yes"
#else
#define VALUE "No"
#endif
consvar_t cv_kartallowgiveitem = CVAR_INIT ("kartallowgiveitem", VALUE, CV_NETVAR|CV_CHEAT|CV_NOSHOWHELP, CV_YesNo, NULL);
#undef VALUE

consvar_t cv_kartdebugdistribution = CVAR_INIT ("kartdebugdistribution", "Off", CV_NETVAR|CV_CHEAT|CV_NOSHOWHELP, CV_OnOff, NULL);
consvar_t cv_kartdebughuddrop = CVAR_INIT ("kartdebughuddrop", "Off", CV_NETVAR|CV_CHEAT|CV_NOSHOWHELP, CV_OnOff, NULL);
static CV_PossibleValue_t kartdebugwaypoint_cons_t[] = {{0, "Off"}, {1, "Forwards"}, {2, "Backwards"}, {0, NULL}};
consvar_t cv_kartdebugwaypoints = CVAR_INIT ("kartdebugwaypoints", "Off", CV_NETVAR|CV_CHEAT|CV_NOSHOWHELP, kartdebugwaypoint_cons_t, NULL);
consvar_t cv_kartdebugbotpredict = CVAR_INIT ("kartdebugbotpredict", "Off", CV_NETVAR|CV_CHEAT|CV_NOSHOWHELP, CV_OnOff, NULL);

consvar_t cv_kartdebugcheckpoint = CVAR_INIT ("kartdebugcheckpoint", "Off", CV_NOSHOWHELP, CV_OnOff, NULL);
consvar_t cv_kartdebugnodes = CVAR_INIT ("kartdebugnodes", "Off", CV_NOSHOWHELP, CV_OnOff, NULL);
consvar_t cv_kartdebugcolorize = CVAR_INIT ("kartdebugcolorize", "Off", CV_NOSHOWHELP, CV_OnOff, NULL);
consvar_t cv_kartdebugdirector = CVAR_INIT ("kartdebugdirector", "Off", CV_NOSHOWHELP, CV_OnOff, NULL);

static CV_PossibleValue_t votetime_cons_t[] = {{10, "MIN"}, {3600, "MAX"}, {0, NULL}};
consvar_t cv_votetime = CVAR_INIT ("votetime", "20", CV_NETVAR, votetime_cons_t, NULL);

consvar_t cv_gravity = CVAR_INIT ("gravity", "0.8", CV_RESTRICT|CV_FLOAT|CV_CALL, NULL, Gravity_OnChange); // change DEFAULT_GRAVITY if you change this

consvar_t cv_soundtest = CVAR_INIT ("soundtest", "0", CV_CALL, NULL, SoundTest_OnChange);

static CV_PossibleValue_t minitimelimit_cons_t[] = {{15, "MIN"}, {9999, "MAX"}, {0, NULL}};
consvar_t cv_countdowntime = CVAR_INIT ("countdowntime", "30", CV_NETVAR|CV_CHEAT, minitimelimit_cons_t, NULL);

consvar_t cv_autobalance = CVAR_INIT ("autobalance", "Off", CV_SAVE|CV_NETVAR|CV_CALL, CV_OnOff, AutoBalance_OnChange);
consvar_t cv_teamscramble = CVAR_INIT ("teamscramble", "Off", CV_SAVE|CV_NETVAR|CV_CALL|CV_NOINIT, teamscramble_cons_t, TeamScramble_OnChange);
consvar_t cv_scrambleonchange = CVAR_INIT ("scrambleonchange", "Off", CV_SAVE|CV_NETVAR, teamscramble_cons_t, NULL);

consvar_t cv_itemfinder = CVAR_INIT ("itemfinder", "Off", CV_CALL|CV_NOSHOWHELP, CV_OnOff, ItemFinder_OnChange);

// Scoring type options
consvar_t cv_overtime = CVAR_INIT ("overtime", "Yes", CV_NETVAR|CV_CHEAT, CV_YesNo, NULL);

consvar_t cv_rollingdemos = CVAR_INIT ("rollingdemos", "On", CV_SAVE, CV_OnOff, NULL);

static CV_PossibleValue_t pointlimit_cons_t[] = {{1, "MIN"}, {MAXSCORE, "MAX"}, {0, "None"}, {0, NULL}};
consvar_t cv_pointlimit = CVAR_INIT ("pointlimit", "None", CV_SAVE|CV_NETVAR|CV_CALL|CV_NOINIT, pointlimit_cons_t, PointLimit_OnChange);
static CV_PossibleValue_t timelimit_cons_t[] = {{1, "MIN"}, {30, "MAX"}, {0, "None"}, {0, NULL}};
consvar_t cv_timelimit = CVAR_INIT ("timelimit", "None", CV_SAVE|CV_NETVAR|CV_CALL|CV_NOINIT, timelimit_cons_t, TimeLimit_OnChange);

static CV_PossibleValue_t numlaps_cons_t[] = {{1, "MIN"}, {MAX_LAPS, "MAX"}, {0, "Map default"}, {0, NULL}};
consvar_t cv_numlaps = CVAR_INIT ("numlaps", "Map default", CV_SAVE|CV_NETVAR|CV_CALL|CV_CHEAT, numlaps_cons_t, NumLaps_OnChange);

// Point and time limits for every gametype
INT32 pointlimits[NUMGAMETYPES];
INT32 timelimits[NUMGAMETYPES];

consvar_t cv_forceskin = CVAR_INIT ("forceskin", "None", CV_NETVAR|CV_CALL|CV_CHEAT, NULL, ForceSkin_OnChange);

consvar_t cv_downloading = CVAR_INIT ("downloading", "On", 0, CV_OnOff, NULL);
consvar_t cv_allowexitlevel = CVAR_INIT ("allowexitlevel", "No", CV_NETVAR, CV_YesNo, NULL);

consvar_t cv_netstat = CVAR_INIT ("netstat", "Off", 0, CV_OnOff, NULL); // show bandwidth statistics
static CV_PossibleValue_t nettimeout_cons_t[] = {{TICRATE/7, "MIN"}, {60*TICRATE, "MAX"}, {0, NULL}};
consvar_t cv_nettimeout = CVAR_INIT ("nettimeout", "210", CV_CALL|CV_SAVE, nettimeout_cons_t, NetTimeout_OnChange);
//static CV_PossibleValue_t jointimeout_cons_t[] = {{5*TICRATE, "MIN"}, {60*TICRATE, "MAX"}, {0, NULL}};
consvar_t cv_jointimeout = CVAR_INIT ("jointimeout", "210", CV_CALL|CV_SAVE, nettimeout_cons_t, JoinTimeout_OnChange);
consvar_t cv_maxping = CVAR_INIT ("maxdelay", "20", CV_SAVE, CV_Unsigned, NULL);

consvar_t cv_lagless = CVAR_INIT ("lagless", "On", CV_SAVE|CV_NETVAR|CV_CALL, CV_OnOff, Lagless_OnChange);

static CV_PossibleValue_t pingtimeout_cons_t[] = {{8, "MIN"}, {120, "MAX"}, {0, NULL}};
consvar_t cv_pingtimeout = CVAR_INIT ("maxdelaytimeout", "10", CV_SAVE|CV_NETVAR, pingtimeout_cons_t, NULL);

// show your ping on the HUD next to framerate. Defaults to warning only (shows up if your ping is > maxping)
static CV_PossibleValue_t showping_cons_t[] = {{0, "Off"}, {1, "Always"}, {2, "Warning"}, {0, NULL}};
consvar_t cv_showping = CVAR_INIT ("showping", "Always", CV_SAVE, showping_cons_t, NULL);

static CV_PossibleValue_t pingmeasurement_cons_t[] = {{0, "Frames"}, {1, "Milliseconds"}, {0, NULL}};
consvar_t cv_pingmeasurement = CVAR_INIT ("pingmeasurement", "Frames", CV_SAVE, pingmeasurement_cons_t, NULL);

consvar_t cv_showviewpointtext = CVAR_INIT ("showviewpointtext", "On", CV_SAVE, CV_OnOff, NULL);

// Intermission time Tails 04-19-2002
static CV_PossibleValue_t inttime_cons_t[] = {{0, "MIN"}, {3600, "MAX"}, {0, NULL}};
consvar_t cv_inttime = CVAR_INIT ("inttime", "10", CV_SAVE|CV_NETVAR, inttime_cons_t, NULL);

static CV_PossibleValue_t advancemap_cons_t[] = {{0, "Same"}, {1, "Next"}, {2, "Random"}, {3, "Vote"}, {0, NULL}};
consvar_t cv_advancemap = CVAR_INIT ("advancemap", "Vote", CV_NETVAR, advancemap_cons_t, NULL);

consvar_t cv_runscripts = CVAR_INIT ("runscripts", "Yes", 0, CV_YesNo, NULL);

consvar_t cv_pause = CVAR_INIT ("pausepermission", "Server", CV_SAVE|CV_NETVAR, pause_cons_t, NULL);
consvar_t cv_mute = CVAR_INIT ("mute", "Off", CV_NETVAR|CV_CALL, CV_OnOff, Mute_OnChange);

consvar_t cv_sleep = CVAR_INIT ("cpusleep", "1", CV_SAVE, sleeping_cons_t, NULL);

static CV_PossibleValue_t perfstats_cons_t[] = {
	{PS_OFF, "Off"},
	{PS_RENDER, "Rendering"},
	{PS_LOGIC, "Logic"},
	{PS_BOT, "Bots"},
	{PS_THINKFRAME, "ThinkFrame"},
	{0, NULL}
};
consvar_t cv_perfstats = CVAR_INIT ("perfstats", "Off", 0, perfstats_cons_t, NULL);

consvar_t cv_director = CVAR_INIT ("director", "Off", 0, CV_OnOff, NULL);

consvar_t cv_schedule = CVAR_INIT ("schedule", "On", CV_NETVAR|CV_CALL, CV_OnOff, Schedule_OnChange);

consvar_t cv_automate = CVAR_INIT ("automate", "On", CV_NETVAR, CV_OnOff, NULL);

char timedemo_name[256];
boolean timedemo_csv;
char timedemo_csv_id[256];
boolean timedemo_quit;

INT16 gametype = GT_RACE;
UINT32 gametyperules = 0;
INT16 gametypecount = GT_FIRSTFREESLOT;

boolean forceresetplayers = false;
boolean deferencoremode = false;
UINT8 splitscreen = 0;
boolean circuitmap = false;
INT32 adminplayers[MAXPLAYERS];

// Scheduled commands.
scheduleTask_t **schedule = NULL;
size_t schedule_size = 0;
size_t schedule_len = 0;

// Automation commands
char *automate_commands[AEV__MAX];

const char *automate_names[AEV__MAX] =
{
	"RoundStart", // AEV_ROUNDSTART
	"IntermissionStart", // AEV_INTERMISSIONSTART
	"VoteStart" // AEV_VOTESTART
};

/// \warning Keep this up-to-date if you add/remove/rename net text commands
const char *netxcmdnames[MAXNETXCMD - 1] =
{
	"NAMEANDCOLOR", // XD_NAMEANDCOLOR
	"WEAPONPREF", // XD_WEAPONPREF
	"KICK", // XD_KICK
	"NETVAR", // XD_NETVAR
	"SAY", // XD_SAY
	"MAP", // XD_MAP
	"EXITLEVEL", // XD_EXITLEVEL
	"ADDFILE", // XD_ADDFILE
	"PAUSE", // XD_PAUSE
	"ADDPLAYER", // XD_ADDPLAYER
	"TEAMCHANGE", // XD_TEAMCHANGE
	"CLEARSCORES", // XD_CLEARSCORES
	"VERIFIED", // XD_VERIFIED
	"RANDOMSEED", // XD_RANDOMSEED
	"RUNSOC", // XD_RUNSOC
	"REQADDFILE", // XD_REQADDFILE
	"SETMOTD", // XD_SETMOTD
	"RESPAWN", // XD_RESPAWN
	"DEMOTED", // XD_DEMOTED
	"LUACMD", // XD_LUACMD
	"LUAVAR", // XD_LUAVAR
	"LUAFILE", // XD_LUAFILE

	// SRB2Kart
	"SETUPVOTE", // XD_SETUPVOTE
	"MODIFYVOTE", // XD_MODIFYVOTE
	"PICKVOTE", // XD_PICKVOTE
	"REMOVEPLAYER", // XD_REMOVEPLAYER
	"POWERLEVEL", // XD_POWERLEVEL
	"PARTYINVITE", // XD_PARTYINVITE
	"ACCEPTPARTYINVITE", // XD_ACCEPTPARTYINVITE
	"LEAVEPARTY", // XD_LEAVEPARTY
	"CANCELPARTYINVITE", // XD_CANCELPARTYINVITE
	"GIVEITEM", // XD_GIVEITEM
	"ADDBOT", // XD_ADDBOT
	"DISCORD", // XD_DISCORD
	"PLAYSOUND", // XD_PLAYSOUND
	"SCHEDULETASK", // XD_SCHEDULETASK
	"SCHEDULECLEAR", // XD_SCHEDULECLEAR
	"AUTOMATE" // XD_AUTOMATE
};

// =========================================================================
//                           SERVER STARTUP
// =========================================================================

/** Registers server commands and variables.
  * Anything required by a dedicated server should probably go here.
  *
  * \sa D_RegisterClientCommands
  */
void D_RegisterServerCommands(void)
{
	INT32 i;
	Forceskin_cons_t[0].value = -1;
	Forceskin_cons_t[0].strvalue = "Off";

	for (i = 0; i < NUMGAMETYPES; i++)
	{
		gametype_cons_t[i].value = i;
		gametype_cons_t[i].strvalue = Gametype_Names[i];
	}
	gametype_cons_t[NUMGAMETYPES].value = 0;
	gametype_cons_t[NUMGAMETYPES].strvalue = NULL;

	// Set the values to 0/NULL, it will be overwritten later when a skin is assigned to the slot.
	for (i = 1; i < MAXSKINS; i++)
	{
		Forceskin_cons_t[i].value = 0;
		Forceskin_cons_t[i].strvalue = NULL;
	}
	RegisterNetXCmd(XD_NAMEANDCOLOR, Got_NameAndColor);
	RegisterNetXCmd(XD_WEAPONPREF, Got_WeaponPref);
	RegisterNetXCmd(XD_POWERLEVEL, Got_PowerLevel);
	RegisterNetXCmd(XD_PARTYINVITE, Got_PartyInvite);
	RegisterNetXCmd(XD_ACCEPTPARTYINVITE, Got_AcceptPartyInvite);
	RegisterNetXCmd(XD_CANCELPARTYINVITE, Got_CancelPartyInvite);
	RegisterNetXCmd(XD_LEAVEPARTY, Got_LeaveParty);
	RegisterNetXCmd(XD_MAP, Got_Mapcmd);
	RegisterNetXCmd(XD_EXITLEVEL, Got_ExitLevelcmd);
	RegisterNetXCmd(XD_ADDFILE, Got_Addfilecmd);
	RegisterNetXCmd(XD_REQADDFILE, Got_RequestAddfilecmd);
	RegisterNetXCmd(XD_PAUSE, Got_Pause);
	RegisterNetXCmd(XD_RESPAWN, Got_Respawn);
	RegisterNetXCmd(XD_RUNSOC, Got_RunSOCcmd);
	RegisterNetXCmd(XD_LUACMD, Got_Luacmd);
	RegisterNetXCmd(XD_LUAFILE, Got_LuaFile);

	RegisterNetXCmd(XD_SETUPVOTE, Got_SetupVotecmd);
	RegisterNetXCmd(XD_MODIFYVOTE, Got_ModifyVotecmd);
	RegisterNetXCmd(XD_PICKVOTE, Got_PickVotecmd);

	RegisterNetXCmd(XD_GIVEITEM, Got_GiveItemcmd);

	RegisterNetXCmd(XD_SCHEDULETASK, Got_ScheduleTaskcmd);
	RegisterNetXCmd(XD_SCHEDULECLEAR, Got_ScheduleClearcmd);
	RegisterNetXCmd(XD_AUTOMATE, Got_Automatecmd);

	// Remote Administration
	COM_AddCommand("password", Command_Changepassword_f);
	COM_AddCommand("login", Command_Login_f); // useful in dedicated to kick off remote admin
	COM_AddCommand("promote", Command_Verify_f);
	RegisterNetXCmd(XD_VERIFIED, Got_Verification);
	COM_AddCommand("demote", Command_RemoveAdmin_f);
	RegisterNetXCmd(XD_DEMOTED, Got_Removal);

	COM_AddCommand("motd", Command_MotD_f);
	RegisterNetXCmd(XD_SETMOTD, Got_MotD_f); // For remote admin

	RegisterNetXCmd(XD_TEAMCHANGE, Got_Teamchange);
	COM_AddCommand("serverchangeteam", Command_ServerTeamChange_f);

	RegisterNetXCmd(XD_CLEARSCORES, Got_Clearscores);
	COM_AddCommand("clearscores", Command_Clearscores_f);
	COM_AddCommand("map", Command_Map_f);
	COM_AddCommand("randommap", Command_RandomMap);
	COM_AddCommand("restartlevel", Command_RestartLevel);

	COM_AddCommand("exitgame", Command_ExitGame_f);
	COM_AddCommand("retry", Command_Retry_f);
	COM_AddCommand("exitlevel", Command_ExitLevel_f);
	COM_AddCommand("showmap", Command_Showmap_f);
	COM_AddCommand("mapmd5", Command_Mapmd5_f);

	COM_AddCommand("addfile", Command_Addfile);
	COM_AddCommand("listwad", Command_ListWADS_f);
	COM_AddCommand("listmapthings", Command_ListDoomednums_f);

	COM_AddCommand("runsoc", Command_RunSOC);
	COM_AddCommand("pause", Command_Pause);
	COM_AddCommand("respawn", Command_Respawn);

	COM_AddCommand("gametype", Command_ShowGametype_f);
	COM_AddCommand("version", Command_Version_f);
#ifdef UPDATE_ALERT
	COM_AddCommand("mod_details", Command_ModDetails_f);
#endif
	COM_AddCommand("quit", Command_Quit_f);

	COM_AddCommand("saveconfig", Command_SaveConfig_f);
	COM_AddCommand("loadconfig", Command_LoadConfig_f);
	COM_AddCommand("changeconfig", Command_ChangeConfig_f);
	COM_AddCommand("isgamemodified", Command_Isgamemodified_f); // test
	COM_AddCommand("showscores", Command_ShowScores_f);
	COM_AddCommand("showtime", Command_ShowTime_f);
	COM_AddCommand("cheats", Command_Cheats_f); // test
#ifdef _DEBUG
	COM_AddCommand("togglemodified", Command_Togglemodified_f);
	COM_AddCommand("archivetest", Command_Archivetest_f);
#endif

	COM_AddCommand("downloads", Command_Downloads_f);

	COM_AddCommand("kartgiveitem", Command_KartGiveItem_f);

	COM_AddCommand("schedule_add", Command_Schedule_Add);
	COM_AddCommand("schedule_clear", Command_Schedule_Clear);
	COM_AddCommand("schedule_list", Command_Schedule_List);

	COM_AddCommand("automate_set", Command_Automate_Set);

	// for master server connection
	AddMServCommands();

	// p_mobj.c
	CV_RegisterVar(&cv_itemrespawntime);
	CV_RegisterVar(&cv_itemrespawn);

	// misc
	CV_RegisterVar(&cv_pointlimit);
	CV_RegisterVar(&cv_numlaps);

	CV_RegisterVar(&cv_autobalance);
	CV_RegisterVar(&cv_teamscramble);
	CV_RegisterVar(&cv_scrambleonchange);

	CV_RegisterVar(&cv_inttime);
	CV_RegisterVar(&cv_advancemap);
	CV_RegisterVar(&cv_timelimit);
	CV_RegisterVar(&cv_playbackspeed);
	CV_RegisterVar(&cv_forceskin);
	CV_RegisterVar(&cv_downloading);

	K_RegisterKartStuff(); // SRB2kart

	CV_RegisterVar(&cv_startinglives);
	CV_RegisterVar(&cv_countdowntime);
	CV_RegisterVar(&cv_runscripts);
	CV_RegisterVar(&cv_overtime);
	CV_RegisterVar(&cv_pause);
	CV_RegisterVar(&cv_mute);

	RegisterNetXCmd(XD_RANDOMSEED, Got_RandomSeed);

	CV_RegisterVar(&cv_allowexitlevel);
	CV_RegisterVar(&cv_restrictskinchange);
	CV_RegisterVar(&cv_allowteamchange);
	CV_RegisterVar(&cv_ingamecap);
	CV_RegisterVar(&cv_spectatorreentry);
	CV_RegisterVar(&cv_antigrief);
	CV_RegisterVar(&cv_respawntime);

	// d_clisrv
	CV_RegisterVar(&cv_maxplayers);
	CV_RegisterVar(&cv_joindelay);
	CV_RegisterVar(&cv_resynchattempts);
	CV_RegisterVar(&cv_maxsend);
	CV_RegisterVar(&cv_noticedownload);
	CV_RegisterVar(&cv_downloadspeed);
	CV_RegisterVar(&cv_httpsource);
#ifndef NONET
	CV_RegisterVar(&cv_allownewplayer);
#ifdef VANILLAJOINNEXTROUND
	CV_RegisterVar(&cv_joinnextround);
#endif
	CV_RegisterVar(&cv_showjoinaddress);
	CV_RegisterVar(&cv_blamecfail);
#endif

	COM_AddCommand("ping", Command_Ping_f);
	CV_RegisterVar(&cv_nettimeout);
	CV_RegisterVar(&cv_jointimeout);
	CV_RegisterVar(&cv_kicktime);
	CV_RegisterVar(&cv_skipmapcheck);
	CV_RegisterVar(&cv_sleep);
	CV_RegisterVar(&cv_maxping);
	CV_RegisterVar(&cv_lagless);
	CV_RegisterVar(&cv_pingtimeout);
	CV_RegisterVar(&cv_showping);
	CV_RegisterVar(&cv_pingmeasurement);
	CV_RegisterVar(&cv_showviewpointtext);

	CV_RegisterVar(&cv_director);

	CV_RegisterVar(&cv_schedule);
	CV_RegisterVar(&cv_automate);

	CV_RegisterVar(&cv_dummyconsvar);

#ifdef USE_STUN
	CV_RegisterVar(&cv_stunserver);
#endif

	CV_RegisterVar(&cv_discordinvites);
	RegisterNetXCmd(XD_DISCORD, Got_DiscordInfo);
}

// =========================================================================
//                           CLIENT STARTUP
// =========================================================================

/** Registers client commands and variables.
  * Nothing needed for a dedicated server should be registered here.
  *
  * \sa D_RegisterServerCommands
  */
void D_RegisterClientCommands(void)
{
	INT32 i;

	for (i = 0; i < MAXSKINCOLORS; i++)
	{
		Color_cons_t[i].value = i;
		Color_cons_t[i].strvalue = skincolors[i].name;
	}

	for (i = 2; i < MAXSKINCOLORS; i++)
	{
		Followercolor_cons_t[i].value = i-2;
		Followercolor_cons_t[i].strvalue = skincolors[i-2].name;
	}

	Followercolor_cons_t[1].value = FOLLOWERCOLOR_MATCH;
	Followercolor_cons_t[1].strvalue = "Match"; // Add "Match" option, which will make the follower color match the player's

	Followercolor_cons_t[0].value = FOLLOWERCOLOR_OPPOSITE;
	Followercolor_cons_t[0].strvalue = "Opposite"; // Add "Opposite" option, ...which is like "Match", but for coloropposite.

	Color_cons_t[MAXSKINCOLORS].value = Followercolor_cons_t[MAXSKINCOLORS+2].value = 0;
	Color_cons_t[MAXSKINCOLORS].strvalue = Followercolor_cons_t[MAXSKINCOLORS+2].strvalue = NULL;

	// Set default player names
	// Monster Iestyn (12/08/19): not sure where else I could have actually put this, but oh well
	for (i = 0; i < MAXPLAYERS; i++)
		sprintf(player_names[i], "Player %c", 'A' + i); // SRB2Kart: Letters like Sonic 3!

	if (dedicated)
		return;

	COM_AddCommand("numthinkers", Command_Numthinkers_f);
	COM_AddCommand("countmobjs", Command_CountMobjs_f);

	COM_AddCommand("changeteam", Command_Teamchange_f);
	COM_AddCommand("changeteam2", Command_Teamchange2_f);
	COM_AddCommand("changeteam3", Command_Teamchange3_f);
	COM_AddCommand("changeteam4", Command_Teamchange4_f);

	COM_AddCommand("invite", Command_Invite_f);
	COM_AddCommand("cancelinvite", Command_CancelInvite_f);
	COM_AddCommand("acceptinvite", Command_AcceptInvite_f);
	COM_AddCommand("rejectinvite", Command_RejectInvite_f);
	COM_AddCommand("leaveparty", Command_LeaveParty_f);

	COM_AddCommand("playdemo", Command_Playdemo_f);
	COM_AddCommand("timedemo", Command_Timedemo_f);
	COM_AddCommand("stopdemo", Command_Stopdemo_f);
	COM_AddCommand("playintro", Command_Playintro_f);

	COM_AddCommand("resetcamera", Command_ResetCamera_f);

	COM_AddCommand("view", Command_View_f);
	COM_AddCommand("view2", Command_View_f);
	COM_AddCommand("view3", Command_View_f);
	COM_AddCommand("view4", Command_View_f);

	COM_AddCommand("setviews", Command_SetViews_f);

	COM_AddCommand("setcontrol", Command_Setcontrol_f);
	COM_AddCommand("setcontrol2", Command_Setcontrol2_f);
	COM_AddCommand("setcontrol3", Command_Setcontrol3_f);
	COM_AddCommand("setcontrol4", Command_Setcontrol4_f);

	COM_AddCommand("screenshot", M_ScreenShot);
	COM_AddCommand("startmovie", Command_StartMovie_f);
	COM_AddCommand("stopmovie", Command_StopMovie_f);

	CV_RegisterVar(&cv_screenshot_option);
	CV_RegisterVar(&cv_screenshot_folder);
	CV_RegisterVar(&cv_screenshot_colorprofile);
	CV_RegisterVar(&cv_moviemode);
	CV_RegisterVar(&cv_movie_option);
	CV_RegisterVar(&cv_movie_folder);
	// PNG variables
	CV_RegisterVar(&cv_zlib_level);
	CV_RegisterVar(&cv_zlib_memory);
	CV_RegisterVar(&cv_zlib_strategy);
	CV_RegisterVar(&cv_zlib_window_bits);
	// APNG variables
	CV_RegisterVar(&cv_zlib_levela);
	CV_RegisterVar(&cv_zlib_memorya);
	CV_RegisterVar(&cv_zlib_strategya);
	CV_RegisterVar(&cv_zlib_window_bitsa);
	CV_RegisterVar(&cv_apng_delay);
	CV_RegisterVar(&cv_apng_downscale);
	// GIF variables
	CV_RegisterVar(&cv_gif_optimize);
	CV_RegisterVar(&cv_gif_downscale);
	CV_RegisterVar(&cv_gif_dynamicdelay);
	CV_RegisterVar(&cv_gif_localcolortable);

	// register these so it is saved to config
	for (i = 0; i < MAXSPLITSCREENPLAYERS; i++)
	{
		CV_RegisterVar(&cv_playername[i]);
		CV_RegisterVar(&cv_playercolor[i]);
		CV_RegisterVar(&cv_skin[i]);
		CV_RegisterVar(&cv_follower[i]);
		CV_RegisterVar(&cv_followercolor[i]);
	}

	// preferred number of players
	CV_RegisterVar(&cv_splitplayers);

	CV_RegisterVar(&cv_seenames);
	CV_RegisterVar(&cv_rollingdemos);
	CV_RegisterVar(&cv_netstat);
	CV_RegisterVar(&cv_netticbuffer);
	CV_RegisterVar(&cv_mindelay);

#ifdef NETGAME_DEVMODE
	CV_RegisterVar(&cv_fishcake);
#endif

	// HUD
	CV_RegisterVar(&cv_itemfinder);
	CV_RegisterVar(&cv_showinputjoy);

	// time attack ghost options are also saved to config
	CV_RegisterVar(&cv_ghost_besttime);
	CV_RegisterVar(&cv_ghost_bestlap);
	CV_RegisterVar(&cv_ghost_last);
	CV_RegisterVar(&cv_ghost_guest);
	CV_RegisterVar(&cv_ghost_staff);

	COM_AddCommand("displayplayer", Command_Displayplayer_f);

	CV_RegisterVar(&cv_recordmultiplayerdemos);
	CV_RegisterVar(&cv_netdemosyncquality);

	// FIXME: not to be here.. but needs be done for config loading
	CV_RegisterVar(&cv_globalgamma);
	CV_RegisterVar(&cv_globalsaturation);

	CV_RegisterVar(&cv_rhue);
	CV_RegisterVar(&cv_yhue);
	CV_RegisterVar(&cv_ghue);
	CV_RegisterVar(&cv_chue);
	CV_RegisterVar(&cv_bhue);
	CV_RegisterVar(&cv_mhue);

	CV_RegisterVar(&cv_rgamma);
	CV_RegisterVar(&cv_ygamma);
	CV_RegisterVar(&cv_ggamma);
	CV_RegisterVar(&cv_cgamma);
	CV_RegisterVar(&cv_bgamma);
	CV_RegisterVar(&cv_mgamma);

	CV_RegisterVar(&cv_rsaturation);
	CV_RegisterVar(&cv_ysaturation);
	CV_RegisterVar(&cv_gsaturation);
	CV_RegisterVar(&cv_csaturation);
	CV_RegisterVar(&cv_bsaturation);
	CV_RegisterVar(&cv_msaturation);

	// m_menu.c
	//CV_RegisterVar(&cv_compactscoreboard);
	CV_RegisterVar(&cv_chatheight);
	CV_RegisterVar(&cv_chatwidth);
	CV_RegisterVar(&cv_chattime);
	CV_RegisterVar(&cv_chatspamprotection);
	CV_RegisterVar(&cv_consolechat);
	CV_RegisterVar(&cv_chatnotifications);
	CV_RegisterVar(&cv_chatbacktint);

	CV_RegisterVar(&cv_shoutname);
	CV_RegisterVar(&cv_shoutcolor);
	CV_RegisterVar(&cv_autoshout);

	CV_RegisterVar(&cv_songcredits);
	CV_RegisterVar(&cv_tutorialprompt);
	CV_RegisterVar(&cv_showfocuslost);
	CV_RegisterVar(&cv_pauseifunfocused);

	// g_input.c
	for (i = 0; i < MAXSPLITSCREENPLAYERS; i++)
	{
		CV_RegisterVar(&cv_kickstartaccel[i]);
		CV_RegisterVar(&cv_shrinkme[i]);
		CV_RegisterVar(&cv_turnaxis[i]);
		CV_RegisterVar(&cv_moveaxis[i]);
		CV_RegisterVar(&cv_brakeaxis[i]);
		CV_RegisterVar(&cv_aimaxis[i]);
		CV_RegisterVar(&cv_lookaxis[i]);
		CV_RegisterVar(&cv_fireaxis[i]);
		CV_RegisterVar(&cv_driftaxis[i]);
		CV_RegisterVar(&cv_lookbackaxis[i]);
		CV_RegisterVar(&cv_deadzone[i]);
		CV_RegisterVar(&cv_digitaldeadzone[i]);
	}

	// filesrch.c
	CV_RegisterVar(&cv_addons_option);
	CV_RegisterVar(&cv_addons_folder);
	CV_RegisterVar(&cv_addons_md5);
	CV_RegisterVar(&cv_addons_showall);
	CV_RegisterVar(&cv_addons_search_type);
	CV_RegisterVar(&cv_addons_search_case);

	CV_RegisterVar(&cv_controlperkey);

	CV_RegisterVar(&cv_usemouse);
	CV_RegisterVar(&cv_invertmouse);
	CV_RegisterVar(&cv_mousesens);
	CV_RegisterVar(&cv_mouseysens);
	//CV_RegisterVar(&cv_mousemove);

	for (i = 0; i < MAXSPLITSCREENPLAYERS; i++)
	{
		CV_RegisterVar(&cv_usejoystick[i]);
		CV_RegisterVar(&cv_joyscale[i]);
#ifdef LJOYSTICK
		CV_RegisterVar(&cv_joyport[i]);
#endif
	}

	// s_sound.c
	CV_RegisterVar(&cv_soundvolume);
	CV_RegisterVar(&cv_closedcaptioning);
	CV_RegisterVar(&cv_digmusicvolume);
	CV_RegisterVar(&cv_numChannels);

	// screen.c
	CV_RegisterVar(&cv_fullscreen);
	CV_RegisterVar(&cv_renderview);
	CV_RegisterVar(&cv_renderhitbox);
	CV_RegisterVar(&cv_vhseffect);
	CV_RegisterVar(&cv_shittyscreen);
	CV_RegisterVar(&cv_renderer);
	CV_RegisterVar(&cv_scr_depth);
	CV_RegisterVar(&cv_scr_width);
	CV_RegisterVar(&cv_scr_height);

	CV_RegisterVar(&cv_soundtest);

	CV_RegisterVar(&cv_invincmusicfade);
	CV_RegisterVar(&cv_growmusicfade);

	CV_RegisterVar(&cv_resetspecialmusic);

	CV_RegisterVar(&cv_resume);
	CV_RegisterVar(&cv_perfstats);

	// ingame object placing
	COM_AddCommand("objectplace", Command_ObjectPlace_f);
	//COM_AddCommand("writethings", Command_Writethings_f);
	CV_RegisterVar(&cv_speed);
	CV_RegisterVar(&cv_opflags);
	CV_RegisterVar(&cv_ophoopflags);
	CV_RegisterVar(&cv_mapthingnum);
//	CV_RegisterVar(&cv_grid);
//	CV_RegisterVar(&cv_snapto);

	// add cheat commands
	COM_AddCommand("noclip", Command_CheatNoClip_f);
	COM_AddCommand("god", Command_CheatGod_f);
	COM_AddCommand("setrings", Command_Setrings_f);
	COM_AddCommand("setlives", Command_Setlives_f);
	COM_AddCommand("devmode", Command_Devmode_f);
	COM_AddCommand("savecheckpoint", Command_Savecheckpoint_f);
	COM_AddCommand("scale", Command_Scale_f);
	COM_AddCommand("gravflip", Command_Gravflip_f);
	COM_AddCommand("hurtme", Command_Hurtme_f);
	COM_AddCommand("teleport", Command_Teleport_f);
	COM_AddCommand("rteleport", Command_RTeleport_f);
	COM_AddCommand("skynum", Command_Skynum_f);
	COM_AddCommand("weather", Command_Weather_f);
#ifdef _DEBUG
	COM_AddCommand("causecfail", Command_CauseCfail_f);
#endif
#ifdef LUA_ALLOW_BYTECODE
	COM_AddCommand("dumplua", Command_Dumplua_f);
#endif

#ifdef HAVE_DISCORDRPC
	CV_RegisterVar(&cv_discordrp);
	CV_RegisterVar(&cv_discordstreamer);
	CV_RegisterVar(&cv_discordasks);
#endif
}

/** Checks if a name (as received from another player) is okay.
  * A name is okay if it is no fewer than 1 and no more than ::MAXPLAYERNAME
  * chars long (not including NUL), it does not begin or end with a space,
  * it does not contain non-printing characters (according to isprint(), which
  * allows space), it does not start with a digit, and no other player is
  * currently using it.
  * \param name      Name to check.
  * \param playernum Player who wants the name, so we can check if they already
  *                  have it, and let them keep it if so.
  * \sa CleanupPlayerName, SetPlayerName, Got_NameAndColor
  * \author Graue <graue@oceanbase.org>
  */

static boolean AllowedPlayerNameChar(char ch)
{
	if (!isprint(ch) || ch == ';' || ch == '"' || (UINT8)(ch) >= 0x80)
		return false;

	return true;
}

boolean EnsurePlayerNameIsGood(char *name, INT32 playernum)
{
	INT32 ix;

	if (strlen(name) == 0 || strlen(name) > MAXPLAYERNAME)
		return false; // Empty or too long.
	if (name[0] == ' ' || name[strlen(name)-1] == ' ')
		return false; // Starts or ends with a space.
	if (isdigit(name[0]))
		return false; // Starts with a digit.
	if (name[0] == '@' || name[0] == '~')
		return false; // Starts with an admin symbol.

	// Check if it contains a non-printing character.
	// Note: ANSI C isprint() considers space a printing character.
	// Also don't allow semicolons, since they are used as
	// console command separators.

	// Also, anything over 0x80 is disallowed too, since compilers love to
	// differ on whether they're printable characters or not.
	for (ix = 0; name[ix] != '\0'; ix++)
		if (!AllowedPlayerNameChar(name[ix]))
			return false;

	// Check if a player is currently using the name, case-insensitively.
	for (ix = 0; ix < MAXPLAYERS; ix++)
	{
		if (ix != playernum && playeringame[ix]
			&& strcasecmp(name, player_names[ix]) == 0)
		{
			// We shouldn't kick people out just because
			// they joined the game with the same name
			// as someone else -- modify the name instead.
			size_t len = strlen(name);

			// Recursion!
			// Slowly strip characters off the end of the
			// name until we no longer have a duplicate.
			if (len > 1)
			{
				name[len-1] = '\0';
				if (!EnsurePlayerNameIsGood (name, playernum))
					return false;
			}
			else if (len == 1) // Agh!
			{
				// Last ditch effort...
				sprintf(name, "%d", M_RandomKey(10));
				if (!EnsurePlayerNameIsGood (name, playernum))
					return false;
			}
			else
				return false;
		}
	}

	return true;
}

/** Cleans up a local player's name before sending a name change.
  * Spaces at the beginning or end of the name are removed. Then if the new
  * name is identical to another player's name, ignoring case, the name change
  * is canceled, and the name in cv_playername[n].value
  * is restored to what it was before.
  *
  * We assume that if playernum is in ::g_localplayers
  * (unless clientjoin is true, a necessary evil)
  * the console variable ::cv_playername[n] is
  * already set to newname. However, the player name table is assumed to
  * contain the old name.
  *
  * \param playernum Player number who has requested a name change.
  *                  Should be in ::g_localplayers.
  * \param newname   New name for that player; should already be in
  *                  ::cv_playername if player is a local player.
  * \sa cv_playername, SendNameAndColor, SetPlayerName
  * \author Graue <graue@oceanbase.org>
  */
void CleanupPlayerName(INT32 playernum, const char *newname)
{
	char *buf;
	char *p;
	char *tmpname = NULL;
	INT32 i;
	boolean namefailed = true;
	boolean clientjoin = !!(playernum >= MAXPLAYERS);

	if (clientjoin)
		playernum -= MAXPLAYERS;

	buf = Z_StrDup(newname);

	do
	{
		p = buf;

		while (*p == ' ')
			p++; // remove leading spaces

		if (strlen(p) == 0)
			break; // empty names not allowed

		if (isdigit(*p))
			break; // names starting with digits not allowed

		if (*p == '@' || *p == '~')
			break; // names that start with @ or ~ (admin symbols) not allowed

		tmpname = p;

		do
		{
			if (!AllowedPlayerNameChar(*p))
				break;
		}
		while (*++p) ;

		if (*p)/* bad char found */
			break;

		// Remove trailing spaces.
		p = &tmpname[strlen(tmpname)-1]; // last character
		while (*p == ' ' && p >= tmpname)
		{
			*p = '\0';
			p--;
		}

		if (strlen(tmpname) == 0)
			break; // another case of an empty name

		// Truncate name if it's too long (max MAXPLAYERNAME chars
		// excluding NUL).
		if (strlen(tmpname) > MAXPLAYERNAME)
			tmpname[MAXPLAYERNAME] = '\0';

		// Remove trailing spaces again.
		p = &tmpname[strlen(tmpname)-1]; // last character
		while (*p == ' ' && p >= tmpname)
		{
			*p = '\0';
			p--;
		}

		// no stealing another player's name
		if (!clientjoin)
		{
			for (i = 0; i < MAXPLAYERS; i++)
			{
				if (i != playernum && playeringame[i]
					&& strcasecmp(tmpname, player_names[i]) == 0)
				{
					break;
				}
			}

			if (i < MAXPLAYERS)
				break;
		}

		// name is okay then
		namefailed = false;
	} while (0);

	if (namefailed)
		tmpname = player_names[playernum];

	// set consvars whether namefailed or not, because even if it succeeded,
	// spaces may have been removed
	if (clientjoin)
		CV_StealthSet(&cv_playername[playernum], tmpname);
	else
	{
		for (i = 0; i <= splitscreen; i++)
		{
			if (playernum == g_localplayers[i])
			{
				CV_StealthSet(&cv_playername[i], tmpname);
				break;
			}
		}

		if (i > splitscreen)
		{
			I_Assert(((void)"CleanupPlayerName used on non-local player", 0));
		}
	}

	Z_Free(buf);
}

/** Sets a player's name, if it is good.
  * If the name is not good (indicating a modified or buggy client), it is not
  * set, and if we are the server in a netgame, the player responsible is
  * kicked with a consistency failure.
  *
  * This function prints a message indicating the name change, unless the game
  * is currently showing the intro, e.g. when processing autoexec.cfg.
  *
  * \param playernum Player number who has requested a name change.
  * \param newname   New name for that player. Should be good, but won't
  *                  necessarily be if the client is maliciously modified or
  *                  buggy.
  * \sa CleanupPlayerName, EnsurePlayerNameIsGood
  * \author Graue <graue@oceanbase.org>
  */
static void SetPlayerName(INT32 playernum, char *newname)
{
	if (EnsurePlayerNameIsGood(newname, playernum))
	{
		if (strcasecmp(newname, player_names[playernum]) != 0)
		{
			if (!LUA_HookNameChange(&players[playernum], newname))
			{
				// Name change rejected by Lua
				if (playernum == consoleplayer)
					CV_StealthSet(cv_playername, player_names[consoleplayer]);
				return;
			}

			if (netgame)
				HU_AddChatText(va("\x82*%s renamed to %s", player_names[playernum], newname), false);

			player_name_changes[playernum]++;

			strcpy(player_names[playernum], newname);
			demo_extradata[playernum] |= DXD_NAME;
		}
	}
	else
	{
		CONS_Printf(M_GetText("Player %d sent a bad name change\n"), playernum+1);
		if (server && netgame)
			SendKick(playernum, KICK_MSG_CON_FAIL);
	}
}

UINT8 CanChangeSkin(INT32 playernum)
{
	// Of course we can change if we're not playing
	if (!Playing() || !addedtogame)
		return true;

	// Force skin in effect.
	if ((cv_forceskin.value != -1) || (mapheaderinfo[gamemap-1] && mapheaderinfo[gamemap-1]->forcecharacter[0] != '\0'))
		return false;

	// Can change skin in intermission and whatnot.
	if (gamestate != GS_LEVEL)
		return true;

	// Server has skin change restrictions.
	if (cv_restrictskinchange.value)
	{
		// Can change skin during initial countdown.
		if (leveltime < starttime)
			return true;

		// Not in game, so you can change
		if (players[playernum].spectator || players[playernum].playerstate == PST_DEAD || players[playernum].playerstate == PST_REBORN)
			return true;

		return false;
	}

	return true;
}

static void ForceAllSkins(INT32 forcedskin)
{
	INT32 i, j;
	for (i = 0; i < MAXPLAYERS; ++i)
	{
		if (!playeringame[i])
			continue;

		SetPlayerSkinByNum(i, forcedskin);

		// If it's me (or my brother (or my sister (or my trusty pet dog))), set appropriate skin value in cv_skin
		if (dedicated) // But don't do this for dedicated servers, of course.
			continue;

		for (j = 0; j <= splitscreen; j++)
		{
			if (i == g_localplayers[j])
			{
				CV_StealthSet(&cv_skin[j], skins[forcedskin].name);
				break;
			}
		}
	}
}

static const char *
VaguePartyDescription (int playernum, int *party_sizes, int default_color)
{
	static char party_description
		[1 + MAXPLAYERNAME + 1 + sizeof " and x others"];
	const char *name;
	int size;
	name = player_names[playernum];
	size = party_sizes[playernum];
	/*
	less than check for the dumb compiler because I KNOW it'll
	complain about "writing x bytes into an area of y bytes"!!!
	*/
	if (size > 1 && size <= MAXSPLITSCREENPLAYERS)
	{
		sprintf(party_description,
				"\x83%s%c and %d other%s",
				name,
				default_color,
				( size - 1 ),
				( (size > 2) ? "s" : "" )
		);
	}
	else
	{
		sprintf(party_description,
				"\x83%s%c",
				name,
				default_color
		);
	}
	return party_description;
}

static INT32 snacpending[MAXSPLITSCREENPLAYERS] = {0,0,0,0};
static INT32 chmappending = 0;

// name, color, or skin has changed
//
static void SendNameAndColor(UINT8 n)
{
	const INT32 playernum = g_localplayers[n];
	player_t *player = &players[playernum];

	char buf[MAXPLAYERNAME+9];
	char *p;

	if (splitscreen < n)
		return; // can happen if skin4/color4/name4 changed

	if (playernum == -1)
		return;

	p = buf;

	// don't allow inaccessible colors
	if (!skincolors[cv_playercolor[n].value].accessible)
	{
		if (player->skincolor && skincolors[player->skincolor].accessible)
			CV_StealthSetValue(&cv_playercolor[n], player->skincolor);
		else if (skins[player->skin].prefcolor && skincolors[skins[player->skin].prefcolor].accessible)
			CV_StealthSetValue(&cv_playercolor[n], skins[player->skin].prefcolor);
		else if (skincolors[atoi(cv_playercolor[n].defaultvalue)].accessible)
			CV_StealthSet(&cv_playercolor[n], cv_playercolor[n].defaultvalue);
		else {
			UINT16 i = 0;
			while (i<numskincolors && !skincolors[i].accessible) i++;
			CV_StealthSetValue(&cv_playercolor[n], (i != numskincolors) ? i : SKINCOLOR_BLUE);
		}
	}

	// ditto for follower colour:
	if (!cv_followercolor[n].value)
		CV_StealthSet(&cv_followercolor[n], "Match"); // set it to "Match". I don't care about your stupidity!

	// so like, this is sent before we even use anything like cvars or w/e so it's possible that follower is set to a pretty yikes value, so let's fix that before we send garbage that could crash the game:
	if (cv_follower[n].value >= numfollowers || cv_follower[n].value < -1)
		CV_StealthSet(&cv_follower[n], "-1");

	if (!strcmp(cv_playername[n].string, player_names[playernum])
		&& cv_playercolor[n].value == player->skincolor
		&& !strcmp(cv_skin[n].string, skins[player->skin].name)
		&& cv_follower[n].value == player->followerskin
		&& cv_followercolor[n].value == player->followercolor)
		return;

	player->availabilities = R_GetSkinAvailabilities();

	// We'll handle it later if we're not playing.
	if (!Playing())
		return;

	// If you're not in a netgame, merely update the skin, color, and name.
	if (!netgame)
	{
		INT32 foundskin;

		CleanupPlayerName(playernum, cv_playername[n].zstring);
		strcpy(player_names[playernum], cv_playername[n].zstring);

		player->skincolor = cv_playercolor[n].value;

		K_KartResetPlayerColor(player);

		// Update follower for local games:
		if (cv_follower[n].value >= -1 && cv_follower[n].value != player->followerskin)
			K_SetFollowerByNum(playernum, cv_follower[n].value);

		player->followercolor = cv_followercolor[n].value;

		if (metalrecording && n == 0)
		{ // Starring Metal Sonic as themselves, obviously.
			SetPlayerSkinByNum(playernum, 5);
			CV_StealthSet(&cv_skin[n], skins[5].name);
		}
		else if ((foundskin = R_SkinAvailable(cv_skin[n].string)) != -1 && R_SkinUsable(playernum, foundskin))
		{
			cv_skin[n].value = foundskin;
			SetPlayerSkin(playernum, cv_skin[n].string);
			CV_StealthSet(&cv_skin[n], skins[cv_skin[n].value].name);
		}
		else
		{
			cv_skin[n].value = players[playernum].skin;
			CV_StealthSet(&cv_skin[n], skins[player->skin].name);
			// will always be same as current
			SetPlayerSkin(playernum, cv_skin[n].string);
		}

		return;
	}

	snacpending[n]++;

	// Don't change name if muted
	if (player_name_changes[playernum] >= MAXNAMECHANGES)
	{
		CV_StealthSet(&cv_playername[n], player_names[playernum]);
		HU_AddChatText("\x85*You must wait to change your name again", false);
	}
	else if (cv_mute.value && !(server || IsPlayerAdmin(playernum)))
		CV_StealthSet(&cv_playername[n], player_names[playernum]);
	else // Cleanup name if changing it
		CleanupPlayerName(playernum, cv_playername[n].zstring);

	// Don't change skin if the server doesn't want you to.
	if (!CanChangeSkin(playernum))
		CV_StealthSet(&cv_skin[n], skins[player->skin].name);

	// check if player has the skin loaded (cv_skin may have
	// the name of a skin that was available in the previous game)
	cv_skin[n].value = R_SkinAvailable(cv_skin[n].string);
	if ((cv_skin[n].value < 0) || !R_SkinUsable(playernum, cv_skin[n].value))
	{
		CV_StealthSet(&cv_skin[n], DEFAULTSKIN);
		cv_skin[n].value = 0;
	}

	// Finally write out the complete packet and send it off.
	WRITESTRINGN(p, cv_playername[n].zstring, MAXPLAYERNAME);
	WRITEUINT32(p, (UINT32)player->availabilities);
	WRITEUINT16(p, (UINT16)cv_playercolor[n].value);
	WRITEUINT8(p, (UINT8)cv_skin[n].value);
	WRITESINT8(p, (SINT8)cv_follower[n].value);
	WRITEUINT16(p, (UINT16)cv_followercolor[n].value);

	SendNetXCmdForPlayer(n, XD_NAMEANDCOLOR, buf, p - buf);
}

static void Got_NameAndColor(UINT8 **cp, INT32 playernum)
{
	player_t *p = &players[playernum];
	char name[MAXPLAYERNAME+1];
	UINT16 color, followercolor;
	UINT8 skin;
	SINT8 follower;
	SINT8 localplayer = -1;
	UINT8 i;

#ifdef PARANOIA
	if (playernum < 0 || playernum > MAXPLAYERS)
		I_Error("There is no player %d!", playernum);
#endif

	for (i = 0; i <= splitscreen; i++)
	{
		if (playernum == g_localplayers[i])
		{
			snacpending[i]--;

#ifdef PARANOIA
			if (snacpending[i] < 0)
				I_Error("snacpending[%d] negative!", i);
#endif

			localplayer = i;
			break;
		}
	}

	READSTRINGN(*cp, name, MAXPLAYERNAME);
	p->availabilities = READUINT32(*cp);
	color = READUINT16(*cp);
	skin = READUINT8(*cp);
	follower = READSINT8(*cp);
	followercolor = READUINT16(*cp);

	// set name
	if (player_name_changes[playernum] < MAXNAMECHANGES)
	{
		if (strcasecmp(player_names[playernum], name) != 0)
			SetPlayerName(playernum, name);
	}

	// set color
	p->skincolor = color % numskincolors;
	if (p->mo)
		p->mo->color = (UINT16)p->skincolor;
	demo_extradata[playernum] |= DXD_COLOR;

	// normal player colors
	if (server && !P_IsMachineLocalPlayer(p))
	{
		boolean kick = false;

		// team colors
		if (G_GametypeHasTeams())
		{
			if (p->ctfteam == 1 && p->skincolor != skincolor_redteam)
				kick = true;
			else if (p->ctfteam == 2 && p->skincolor != skincolor_blueteam)
				kick = true;
		}

		// don't allow inaccessible colors
		if (skincolors[p->skincolor].accessible == false)
			kick = true;

		// availabilities
		for (i = 0; i < MAXSKINS; i++)
		{
			UINT32 playerhasunlocked = (p->availabilities & (1 << i));
			boolean islocked = false;
			UINT8 j;

			for (j = 0; j < MAXUNLOCKABLES; j++)
			{
				if (unlockables[j].type != SECRET_SKIN)
					continue;

				if (unlockables[j].variable == i)
				{
					islocked = true;
					break;
				}
			}

			if (islocked == false && playerhasunlocked == true)
			{
				// hacked client that enabled every bit
				kick = true;
				break;
			}
		}

		if (kick)
		{
			CONS_Alert(CONS_WARNING, M_GetText("Illegal color change received from %s (team: %d), color: %d)\n"), player_names[playernum], p->ctfteam, p->skincolor);
			SendKick(playernum, KICK_MSG_CON_FAIL);
			return;
		}
	}

	// set skin
	if (cv_forceskin.value >= 0 && (netgame || multiplayer)) // Server wants everyone to use the same player
	{
		const INT32 forcedskin = cv_forceskin.value;
		SetPlayerSkinByNum(playernum, forcedskin);

		if (localplayer != -1)
			CV_StealthSet(&cv_skin[localplayer], skins[forcedskin].name);
	}
	else
		SetPlayerSkinByNum(playernum, skin);

	// set follower colour:
	// Don't bother doing garbage and kicking if we receive None,
	// this is both silly and a waste of time,
	// this will be handled properly in K_HandleFollower.
	p->followercolor = followercolor;

	// set follower
	K_SetFollowerByNum(playernum, follower);

#ifdef HAVE_DISCORDRPC
	if (playernum == consoleplayer)
		DRPC_UpdatePresence();
#endif
}

enum {
	WP_KICKSTARTACCEL = 1<<0,
	WP_SHRINKME = 1<<1,
	WP_FLIPCAM = 1<<2,
};

void WeaponPref_Send(UINT8 ssplayer)
{
	UINT8 prefs = 0;

	if (cv_kickstartaccel[ssplayer].value)
		prefs |= WP_KICKSTARTACCEL;

	if (cv_shrinkme[ssplayer].value)
		prefs |= WP_SHRINKME;
	
	if (cv_flipcam[ssplayer].value)
		prefs |= WP_FLIPCAM;

	SendNetXCmdForPlayer(ssplayer, XD_WEAPONPREF, &prefs, 1);
}

void WeaponPref_Save(UINT8 **cp, INT32 playernum)
{
	player_t *player = &players[playernum];

	UINT8 prefs = 0;

	if (player->pflags & PF_KICKSTARTACCEL)
		prefs |= WP_KICKSTARTACCEL;

	if (player->pflags & PF_SHRINKME)
		prefs |= WP_SHRINKME;
	
	if (player->pflags & PF_FLIPCAM)
		prefs |= WP_FLIPCAM;

	WRITEUINT8(*cp, prefs);
}

void WeaponPref_Parse(UINT8 **cp, INT32 playernum)
{
	player_t *player = &players[playernum];

	UINT8 prefs = READUINT8(*cp);

	player->pflags &= ~(PF_KICKSTARTACCEL|PF_SHRINKME);

	if (prefs & WP_KICKSTARTACCEL)
		player->pflags |= PF_KICKSTARTACCEL;

	if (prefs & WP_SHRINKME)
		player->pflags |= PF_SHRINKME;
	
	if (prefs & WP_FLIPCAM)
		player->pflags |= PF_FLIPCAM;

	if (leveltime < 2)
	{
		// BAD HACK: No other place I tried to slot this in
		// made it work for the host when they initally host,
		// so this will have to do.
		K_UpdateShrinkCheat(player);
	}
}

static void Got_WeaponPref(UINT8 **cp,INT32 playernum)
{
	WeaponPref_Parse(cp, playernum);

	// SEE ALSO g_demo.c
	demo_extradata[playernum] |= DXD_WEAPONPREF;
}

static void Got_PowerLevel(UINT8 **cp,INT32 playernum)
{
	UINT16 race = (UINT16)READUINT16(*cp);
	UINT16 battle = (UINT16)READUINT16(*cp);

	clientpowerlevels[playernum][PWRLV_RACE] = min(PWRLVRECORD_MAX, race);
	clientpowerlevels[playernum][PWRLV_BATTLE] = min(PWRLVRECORD_MAX, battle);

	CONS_Debug(DBG_GAMELOGIC, "set player %d to power %d\n", playernum, race);
}

static void Got_PartyInvite(UINT8 **cp,INT32 playernum)
{
	UINT8 invitee;

	boolean kick = false;

	invitee = READUINT8 (*cp);

	if (
			invitee < MAXPLAYERS &&
			playeringame[invitee] &&
			playerconsole[playernum] == playernum/* only consoleplayer may! */
	){
		invitee = playerconsole[invitee];
		/* you cannot invite yourself or your computer */
		if (invitee == playernum)
			kick = true;
	}
	else
		kick = true;

	if (kick)
	{
		CONS_Alert(CONS_WARNING, M_GetText("Illegal splitscreen invitation received from %s\n"), player_names[playernum]);
		if (server)
			SendKick(playernum, KICK_MSG_CON_FAIL);
		return;
	}

	if (splitscreen_invitations[invitee] < 0)
	{
		splitscreen_invitations[invitee] = playernum;

		if (invitee == consoleplayer)/* hey that's me! */
		{
			HU_AddChatText(va(
						"\x82*You have been invited to join %s.",
						VaguePartyDescription(
							playernum, splitscreen_party_size, '\x82')
			), true);
		}
	}
}

static void Got_AcceptPartyInvite(UINT8 **cp,INT32 playernum)
{
	int invitation;
	int old_party_size;
	int views;

	(void)cp;

	if (playerconsole[playernum] != playernum)
	{
		CONS_Alert(CONS_WARNING, M_GetText("Illegal accept splitscreen invite received from %s\n"), player_names[playernum]);
		if (server)
			SendKick(playernum, KICK_MSG_CON_FAIL);
		return;
	}

	invitation = splitscreen_invitations[playernum];

	if (invitation >= 0)
	{
		if (splitscreen_partied[invitation])
		{
			HU_AddChatText(va(
						"\x82*%s joined your party!",
						VaguePartyDescription(
							playernum, splitscreen_original_party_size, '\x82')
			), true);
		}
		else if (playernum == consoleplayer)
		{
			HU_AddChatText(va(
						"\x82*You joined %s's party!",
						VaguePartyDescription(
							invitation, splitscreen_party_size, '\x82')
			), true);
		}

		old_party_size = splitscreen_party_size[invitation];
		views = splitscreen_original_party_size[playernum];

		if (( old_party_size + views ) <= MAXSPLITSCREENPLAYERS)
		{
			G_RemovePartyMember(playernum);
			G_AddPartyMember(invitation, playernum);
		}

		splitscreen_invitations[playernum] = -1;
	}
}

static void Got_CancelPartyInvite(UINT8 **cp,INT32 playernum)
{
	UINT8 invitee;

	invitee = READUINT8 (*cp);

	if (
			invitee < MAXPLAYERS &&
			playeringame[invitee]
	){
		invitee = playerconsole[invitee];

		if (splitscreen_invitations[invitee] == playerconsole[playernum])
		{
			splitscreen_invitations[invitee] = -1;

			if (consoleplayer == invitee)
			{
				HU_AddChatText("\x85*Your invitation has been rescinded.", true);
			}
		}
	}
	else
	{
		CONS_Alert(CONS_WARNING, M_GetText("Illegal cancel splitscreen invite received from %s\n"), player_names[playernum]);
		if (server)
			SendKick(playernum, KICK_MSG_CON_FAIL);
	}
}

static void Got_LeaveParty(UINT8 **cp,INT32 playernum)
{
	(void)cp;

	if (playerconsole[playernum] != playernum)
	{
		CONS_Alert(CONS_WARNING, M_GetText("Illegal accept splitscreen invite received from %s\n"), player_names[playernum]);
		if (server)
			SendKick(playernum, KICK_MSG_CON_FAIL);
		return;
	}

	if (consoleplayer == splitscreen_invitations[playernum])
	{
		HU_AddChatText(va(
					"\x85*\x83%s\x85 rejected your invitation.",
					player_names[playernum]
		), true);
	}

	splitscreen_invitations[playernum] = -1;

	if (splitscreen_party_size[playernum] >
			splitscreen_original_party_size[playernum])
	{
		if (splitscreen_partied[playernum] && playernum != consoleplayer)
		{
			HU_AddChatText(va(
						"\x85*%s left your party.",
						VaguePartyDescription(
							playernum, splitscreen_original_party_size, '\x85')
			), true);
		}

		G_RemovePartyMember(playernum);
		G_ResetSplitscreen(playernum);
	}
}

void D_SendPlayerConfig(UINT8 n)
{
	UINT8 buf[4];
	UINT8 *p = buf;

	SendNameAndColor(n);
	WeaponPref_Send(n);

	if (n == 0)
	{
		// Send it over
		WRITEUINT16(p, vspowerlevel[PWRLV_RACE]);
		WRITEUINT16(p, vspowerlevel[PWRLV_BATTLE]);
	}
	else
	{
		// Splitscreen players have invalid powerlevel
		WRITEUINT16(p, 0);
		WRITEUINT16(p, 0);
	}

	SendNetXCmdForPlayer(n, XD_POWERLEVEL, buf, p-buf);
}

// Only works for displayplayer, sorry!
static void Command_ResetCamera_f(void)
{
	P_ResetCamera(&players[g_localplayers[0]], &camera[0]);
}

/* Consider replacing nametonum with this */
static INT32 LookupPlayer(const char *s)
{
	INT32 playernum;

	if (*s == '0')/* clever way to bypass atoi */
		return 0;

	if (( playernum = atoi(s) ))
	{
		playernum = max(min(playernum, MAXPLAYERS-1), 0);/* not out of range */
		return playernum;
	}

	for (playernum = 0; playernum < MAXPLAYERS; ++playernum)
	{
		/* Match name case-insensitively: fully, or partially the start. */
		if (playeringame[playernum])
			if (strnicmp(player_names[playernum], s, strlen(s)) == 0)
		{
			return playernum;
		}
	}
	return -1;
}

static INT32 FindPlayerByPlace(INT32 place)
{
	INT32 playernum;
	for (playernum = 0; playernum < MAXPLAYERS; ++playernum)
		if (playeringame[playernum])
	{
		if (players[playernum].position == place)
		{
			return playernum;
		}
	}
	return -1;
}

//
// GetViewablePlayerPlaceRange
// Return in first and last, that player available to view, sorted by placement
// in the race.
//
static void GetViewablePlayerPlaceRange(INT32 *first, INT32 *last)
{
	INT32 i;
	INT32 place;

	(*first) = MAXPLAYERS;
	(*last) = 0;

	for (i = 0; i < MAXPLAYERS; ++i)
		if (G_CouldView(i))
	{
		place = players[i].position;
		if (place < (*first))
			(*first) = place;
		if (place > (*last))
			(*last) = place;
	}
}

#define PRINTVIEWPOINT( pre,suf ) \
	CONS_Printf(pre"viewing \x84(%d) \x83%s\x80"suf".\n",\
			(*displayplayerp), player_names[(*displayplayerp)]);
static void Command_View_f(void)
{
	INT32 *displayplayerp;
	INT32 olddisplayplayer;
	int viewnum;
	const char *playerparam;
	INT32 placenum;
	INT32 playernum;
	INT32 firstplace, lastplace;
	char c;
	/* easy peasy */
	c = COM_Argv(0)[strlen(COM_Argv(0))-1];/* may be digit */
	switch (c)
	{
		case '2': viewnum = 2; break;
		case '3': viewnum = 3; break;
		case '4': viewnum = 4; break;
		default:  viewnum = 1;
	}

	if (viewnum > 1 && !( multiplayer && demo.playback ))
	{
		CONS_Alert(CONS_NOTICE,
				"You must be viewing a multiplayer replay to use this.\n");
		return;
	}

	if (demo.freecam)
		return;

	displayplayerp = &displayplayers[viewnum-1];

	if (COM_Argc() > 1)/* switch to player */
	{
		playerparam = COM_Argv(1);
		if (playerparam[0] == '#')/* search by placement */
		{
			placenum = atoi(&playerparam[1]);
			playernum = FindPlayerByPlace(placenum);
			if (playernum == -1 || !G_CouldView(playernum))
			{
				GetViewablePlayerPlaceRange(&firstplace, &lastplace);
				if (playernum == -1)
				{
					CONS_Alert(CONS_WARNING, "There is no player in that place! ");
				}
				else
				{
					CONS_Alert(CONS_WARNING,
							"That player cannot be viewed currently! "
							"The first player that you can view is \x82#%d\x80; ",
							firstplace);
				}
				CONS_Printf("Last place is \x82#%d\x80.\n", lastplace);
				return;
			}
		}
		else
		{
			if (( playernum = LookupPlayer(COM_Argv(1)) ) == -1)
			{
				CONS_Alert(CONS_WARNING, "There is no player by that name!\n");
				return;
			}
			if (!playeringame[playernum])
			{
				CONS_Alert(CONS_WARNING, "There is no player using that slot!\n");
				return;
			}
		}

		olddisplayplayer = (*displayplayerp);
		G_ResetView(viewnum, playernum, false);

		/* The player we wanted was corrected to who it already was. */
		if ((*displayplayerp) == olddisplayplayer)
			return;

		if ((*displayplayerp) != playernum)/* differ parameter */
		{
			/* skipped some */
			CONS_Alert(CONS_NOTICE, "That player cannot be viewed currently.\n");
			PRINTVIEWPOINT ("Now "," instead")
		}
		else
			PRINTVIEWPOINT ("Now ",)
	}
	else/* print current view */
	{
		if (r_splitscreen < viewnum-1)/* We can't see those guys! */
			return;
		PRINTVIEWPOINT ("Currently ",)
	}
}
#undef PRINTVIEWPOINT

static void Command_SetViews_f(void)
{
	UINT8 splits;
	UINT8 newsplits;

	if (!( demo.playback && multiplayer ))
	{
		CONS_Alert(CONS_NOTICE,
				"You must be viewing a multiplayer replay to use this.\n");
		return;
	}

	if (COM_Argc() != 2)
	{
		CONS_Printf("setviews <views>: set the number of split screens\n");
		return;
	}

	splits = r_splitscreen+1;

	newsplits = atoi(COM_Argv(1));
	newsplits = min(max(newsplits, 1), 4);
	if (newsplits > splits)
		G_AdjustView(newsplits, 0, true);
	else
	{
		r_splitscreen = newsplits-1;
		R_ExecuteSetViewSize();
	}
}

static void
Command_Invite_f (void)
{
	UINT8 buffer[1];

	int invitee;

	if (COM_Argc() != 2)
	{
		CONS_Printf("invite <player>: Invite a player to your party.\n");
		return;
	}

	if (r_splitscreen >= MAXSPLITSCREENPLAYERS)
	{
		CONS_Alert(CONS_WARNING, "Your party is full!\n");
		return;
	}

	invitee = LookupPlayer(COM_Argv(1));

	if (invitee == -1)
	{
		CONS_Alert(CONS_WARNING, "There is no player by that name!\n");
		return;
	}
	if (!playeringame[invitee])
	{
		CONS_Alert(CONS_WARNING, "There is no player using that slot!\n");
		return;
	}

	if (invitee == consoleplayer)
	{
		CONS_Alert(CONS_WARNING, "You cannot invite yourself! Bruh!\n");
		return;
	}

	if (splitscreen_invitations[invitee] >= 0)
	{
		CONS_Alert(CONS_WARNING,
				"That player has already been invited to join another party.\n");
	}

	if (( splitscreen_party_size[consoleplayer] +
				splitscreen_original_party_size[invitee] ) > MAXSPLITSCREENPLAYERS)
	{
		CONS_Alert(CONS_WARNING,
				"That player joined with too many "
				"splitscreen players for your party.\n");
	}

	CONS_Printf(
			"Inviting %s...\n",
			VaguePartyDescription(
				invitee, splitscreen_original_party_size, '\x80')
	);

	buffer[0] = invitee;

	SendNetXCmd(XD_PARTYINVITE, buffer, sizeof buffer);
}

static void
Command_CancelInvite_f (void)
{
	UINT8 buffer[1];

	int invitee;

	if (COM_Argc() != 2)
	{
		CONS_Printf("cancelinvite <player>: Rescind a party invitation.\n");
		return;
	}

	invitee = LookupPlayer(COM_Argv(1));

	if (invitee == -1)
	{
		CONS_Alert(CONS_WARNING, "There is no player by that name!\n");
		return;
	}
	if (!playeringame[invitee])
	{
		CONS_Alert(CONS_WARNING, "There is no player using that slot!\n");
		return;
	}

	if (splitscreen_invitations[invitee] != consoleplayer)
	{
		CONS_Alert(CONS_WARNING,
				"You have not invited this player!\n");
	}

	CONS_Printf(
			"Rescinding invite to %s...\n",
			VaguePartyDescription(
				invitee, splitscreen_original_party_size, '\x80')
	);

	buffer[0] = invitee;

	SendNetXCmd(XD_CANCELPARTYINVITE, buffer, sizeof buffer);
}

static boolean
CheckPartyInvite (void)
{
	if (splitscreen_invitations[consoleplayer] < 0)
	{
		CONS_Alert(CONS_WARNING, "There is no open party invitation.\n");
		return false;
	}
	return true;
}

static void
Command_AcceptInvite_f (void)
{
	if (CheckPartyInvite())
		SendNetXCmd(XD_ACCEPTPARTYINVITE, NULL, 0);
}

static void
Command_RejectInvite_f (void)
{
	if (CheckPartyInvite())
	{
		CONS_Printf("\x85Rejecting invite...\n");

		SendNetXCmd(XD_LEAVEPARTY, NULL, 0);
	}
}

static void
Command_LeaveParty_f (void)
{
	if (r_splitscreen > splitscreen)
	{
		CONS_Printf("\x85Leaving party...\n");

		SendNetXCmd(XD_LEAVEPARTY, NULL, 0);
	}
}

// ========================================================================

// play a demo, add .lmp for external demos
// eg: playdemo demo1 plays the internal game demo
//
// UINT8 *demofile; // demo file buffer
static void Command_Playdemo_f(void)
{
	char name[256];

	if (COM_Argc() < 2)
	{
		CONS_Printf("playdemo <demoname> [-addfiles / -force]:\n");
		CONS_Printf(M_GetText(
					"Play back a demo file. The full path from your Kart directory must be given.\n\n"

					"* With \"-addfiles\", any required files are added from a list contained within the demo file.\n"
					"* With \"-force\", the demo is played even if the necessary files have not been added.\n"));
		return;
	}

	if (netgame)
	{
		CONS_Printf(M_GetText("You can't play a demo while in a netgame.\n"));
		return;
	}

	// disconnect from server here?
	if (demo.playback)
		G_StopDemo();
	if (metalplayback)
		G_StopMetalDemo();

	// open the demo file
	strcpy(name, COM_Argv(1));
	// dont add .lmp so internal game demos can be played

	CONS_Printf(M_GetText("Playing back demo '%s'.\n"), name);

	demo.loadfiles = strcmp(COM_Argv(2), "-addfiles") == 0;
	demo.ignorefiles = strcmp(COM_Argv(2), "-force") == 0;

	// Internal if no extension, external if one exists
	// If external, convert the file name to a path in SRB2's home directory
	if (FIL_CheckExtension(name))
		G_DoPlayDemo(va("%s"PATHSEP"%s", srb2home, name));
	else
		G_DoPlayDemo(name);
}

static void Command_Timedemo_f(void)
{
	size_t i = 0;

	if (COM_Argc() < 2)
	{
		CONS_Printf(M_GetText("timedemo <demoname> [-csv [<trialid>]] [-quit]: time a demo\n"));
		return;
	}

	if (netgame)
	{
		CONS_Printf(M_GetText("You can't play a demo while in a netgame.\n"));
		return;
	}

	// disconnect from server here?
	if (demo.playback)
		G_StopDemo();
	if (metalplayback)
		G_StopMetalDemo();

	// open the demo file
	strcpy (timedemo_name, COM_Argv(1));
	// dont add .lmp so internal game demos can be played

	// print timedemo results as CSV?
	i = COM_CheckParm("-csv");
	timedemo_csv = (i > 0);
	if (COM_CheckParm("-quit") != i + 1)
		strcpy(timedemo_csv_id, COM_Argv(i + 1)); // user-defined string to identify row
	else
		timedemo_csv_id[0] = 0;

	// exit after the timedemo?
	timedemo_quit = (COM_CheckParm("-quit") > 0);

	CONS_Printf(M_GetText("Timing demo '%s'.\n"), timedemo_name);

	G_TimeDemo(timedemo_name);
}

// stop current demo
static void Command_Stopdemo_f(void)
{
	G_CheckDemoStatus();
	CONS_Printf(M_GetText("Stopped demo.\n"));
}

static void Command_StartMovie_f(void)
{
	M_StartMovie();
}

static void Command_StopMovie_f(void)
{
	M_StopMovie();
}

INT32 mapchangepending = 0;

/** Runs a map change.
  * The supplied data are assumed to be good. If provided by a user, they will
  * have already been checked in Command_Map_f().
  *
  * Do \b NOT call this function directly from a menu! M_Responder() is called
  * from within the event processing loop, and this function calls
  * SV_SpawnServer(), which calls CL_ConnectToServer(), which gives you "Press
  * ESC to abort", which calls I_GetKey(), which adds an event. In other words,
  * 63 old events will get reexecuted, with ridiculous results. Just don't do
  * it (without setting delay to 1, which is the current solution).
  *
  * \param mapnum          Map number to change to.
  * \param gametype        Gametype to switch to.
  * \param pencoremode     Is this 'Encore Mode'?
  * \param resetplayers    1 to reset player scores and lives and such, 0 not to.
  * \param delay           Determines how the function will be executed: 0 to do
  *                        it all right now (must not be done from a menu), 1 to
  *                        do step one and prepare step two, 2 to do step two.
  * \param skipprecutscene To skip the precutscence or not?
  * \sa D_GameTypeChanged, Command_Map_f
  * \author Graue <graue@oceanbase.org>
  */
void D_MapChange(INT32 mapnum, INT32 newgametype, boolean pencoremode, boolean resetplayers, INT32 delay, boolean skipprecutscene, boolean FLS)
{
	static char buf[2+MAX_WADPATH+1+4];
	static char *buf_p = buf;
	// The supplied data are assumed to be good.
	I_Assert(delay >= 0 && delay <= 2);

	if (mapnum != -1)
	{
		CV_SetValue(&cv_nextmap, mapnum);
	}

	CONS_Debug(DBG_GAMELOGIC, "Map change: mapnum=%d gametype=%d pencoremode=%d resetplayers=%d delay=%d skipprecutscene=%d\n",
	           mapnum, newgametype, pencoremode, resetplayers, delay, skipprecutscene);

	if ((netgame || multiplayer) && !((gametype == newgametype) && (gametypedefaultrules[newgametype] & GTR_CAMPAIGN)))
		FLS = false;

	// Too lazy to change the input value for every instance of this function.......
	if (bossinfo.boss == true)
	{
		pencoremode = bossinfo.encore;
	}
	else if (grandprixinfo.gp == true)
	{
		pencoremode = grandprixinfo.encore;
	}

	if (delay != 2)
	{
		UINT8 flags = 0;
		const char *mapname = G_BuildMapName(mapnum);
		I_Assert(W_CheckNumForName(mapname) != LUMPERROR);
		buf_p = buf;
		if (pencoremode)
			flags |= 1;
		if (!resetplayers)
			flags |= 1<<1;
		if (skipprecutscene)
			flags |= 1<<2;
		if (FLS)
			flags |= 1<<3;
		WRITEUINT8(buf_p, flags);

		// new gametype value
		WRITEUINT8(buf_p, newgametype);

		WRITESTRINGN(buf_p, mapname, MAX_WADPATH);
	}

	if (delay == 1)
		mapchangepending = 1;
	else
	{
		mapchangepending = 0;
		// spawn the server if needed
		// reset players if there is a new one
		if (!IsPlayerAdmin(consoleplayer))
		{
			if (SV_SpawnServer())
				buf[0] &= ~(1<<1);
			if (!Playing()) // you failed to start a server somehow, so cancel the map change
				return;
		}

		chmappending++;

		if (netgame)
			WRITEUINT32(buf_p, M_RandomizedSeed()); // random seed
		SendNetXCmd(XD_MAP, buf, buf_p - buf);
	}
}

void D_SetupVote(void)
{
	UINT8 buf[5*2]; // four UINT16 maps (at twice the width of a UINT8), and two gametypes
	UINT8 *p = buf;
	INT32 i;
	UINT8 secondgt = G_SometimesGetDifferentGametype();
	INT16 votebuffer[4] = {-1,-1,-1,0};

	if ((cv_kartencore.value == 1) && (gametyperules & GTR_CIRCUIT))
		WRITEUINT8(p, (gametype|0x80));
	else
		WRITEUINT8(p, gametype);
	WRITEUINT8(p, secondgt);
	secondgt &= ~0x80;

	for (i = 0; i < 4; i++)
	{
		UINT16 m;
		if (i == 2) // sometimes a different gametype
			m = G_RandMap(G_TOLFlag(secondgt), prevmap, ((secondgt != gametype) ? 2 : 0), 0, true, votebuffer);
		else if (i >= 3) // unknown-random and force-unknown MAP HELL
			m = G_RandMap(G_TOLFlag(gametype), prevmap, 0, (i-2), (i < 4), votebuffer);
		else
			m = G_RandMap(G_TOLFlag(gametype), prevmap, 0, 0, true, votebuffer);
		if (i < 3)
			votebuffer[i] = m; // min() is a dumb workaround for gcc 4.4 array-bounds error
		WRITEUINT16(p, m);
	}

	SendNetXCmd(XD_SETUPVOTE, buf, p - buf);
}

void D_ModifyClientVote(UINT8 player, SINT8 voted, UINT8 splitplayer)
{
	char buf[2];
	char *p = buf;

	if (splitplayer > 0)
		player = g_localplayers[splitplayer];

	WRITESINT8(p, voted);
	WRITEUINT8(p, player);
	SendNetXCmd(XD_MODIFYVOTE, &buf, 2);
}

void D_PickVote(void)
{
	char buf[2];
	char* p = buf;
	SINT8 temppicks[MAXPLAYERS];
	SINT8 templevels[MAXPLAYERS];
	SINT8 votecompare = -1;
	UINT8 numvotes = 0, key = 0;
	INT32 i;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i] || players[i].spectator)
			continue;
		if (votes[i] != -1)
		{
			temppicks[numvotes] = i;
			templevels[numvotes] = votes[i];
			numvotes++;
			if (votecompare == -1)
				votecompare = votes[i];
		}
	}

	key = M_RandomKey(numvotes);

	if (numvotes > 0)
	{
		WRITESINT8(p, temppicks[key]);
		WRITESINT8(p, templevels[key]);
	}
	else
	{
		WRITESINT8(p, -1);
		WRITESINT8(p, 0);
	}

	SendNetXCmd(XD_PICKVOTE, &buf, 2);
}

static char *
ConcatCommandArgv (int start, int end)
{
	char *final;

	size_t size;

	int i;
	char *p;

	size = 0;

	for (i = start; i < end; ++i)
	{
		/*
		one space after each argument, but terminating
		character on final argument
		*/
		size += strlen(COM_Argv(i)) + 1;
	}

	final = ZZ_Alloc(size);
	p = final;

	--end;/* handle the final argument separately */
	for (i = start; i < end; ++i)
	{
		p += sprintf(p, "%s ", COM_Argv(i));
	}
	/* at this point "end" is actually the last argument's position */
	strcpy(p, COM_Argv(end));

	return final;
}

//
// Warp to map code.
// Called either from map <mapname> console command, or idclev cheat.
//
// Largely rewritten by James.
//
static void Command_Map_f(void)
{
	size_t first_option;
	size_t option_force;
	size_t option_gametype;
	size_t option_encore;
	size_t option_skill;
	const char *gametypename;
	boolean newresetplayers;

	boolean mustmodifygame;

	INT32 newmapnum;

	char   *    mapname;
	char   *realmapname = NULL;

	INT32 newgametype = gametype;
	boolean newencoremode = (cv_kartencore.value == 1);

	INT32 d;

	if (client && !IsPlayerAdmin(consoleplayer))
	{
		CONS_Printf(M_GetText("Only the server or a remote admin can use this.\n"));
		return;
	}

	option_force    =   COM_CheckPartialParm("-f");
	option_gametype =   COM_CheckPartialParm("-g");
	option_encore   =   COM_CheckPartialParm("-e");
	option_skill    =   COM_CheckPartialParm("-s");
	newresetplayers = ! COM_CheckParm("-noresetplayers");

	mustmodifygame = !(netgame || multiplayer) && !majormods;

	if (mustmodifygame && !option_force)
	{
		/* May want to be more descriptive? */
		CONS_Printf(M_GetText("Sorry, level change disabled in single player.\n"));
		return;
	}

	if (!newresetplayers && !cv_debug)
	{
		CONS_Printf(M_GetText("DEVMODE must be enabled.\n"));
		return;
	}

	if (option_gametype)
	{
		if (!multiplayer)
		{
			CONS_Printf(M_GetText(
						"You can't switch gametypes in single player!\n"));
			return;
		}
		else if (COM_Argc() < option_gametype + 2)/* no argument after? */
		{
			CONS_Alert(CONS_ERROR,
					"No gametype name follows parameter '%s'.\n",
					COM_Argv(option_gametype));
			return;
		}
	}

	if (!( first_option = COM_FirstOption() ))
		first_option = COM_Argc();

	if (first_option < 2)
	{
		/* I'm going over the fucking lines and I DON'T CAREEEEE */
		CONS_Printf("map <name / [MAP]code / number> [-gametype <type>] [-force]:\n");
		CONS_Printf(M_GetText(
					"Warp to a map, by its name, two character code, with optional \"MAP\" prefix, or by its number (though why would you).\n"
					"All parameters are case-insensitive and may be abbreviated.\n"));
		return;
	}

	mapname = ConcatCommandArgv(1, first_option);

	newmapnum = G_FindMapByNameOrCode(mapname, &realmapname);

	if (newmapnum == 0)
	{
		CONS_Alert(CONS_ERROR, M_GetText("Could not find any map described as '%s'.\n"), mapname);
		Z_Free(mapname);
		return;
	}

	if (mustmodifygame && option_force)
	{
		G_SetGameModified(multiplayer, true);
	}

	// new gametype value
	// use current one by default
	if (option_gametype)
	{
		gametypename = COM_Argv(option_gametype + 1);

		newgametype = G_GetGametypeByName(gametypename);

		if (newgametype == -1) // reached end of the list with no match
		{
			/* Did they give us a gametype number? That's okay too! */
			if (isdigit(gametypename[0]))
			{
				d = atoi(gametypename);
				if (d >= 0 && d < gametypecount)
					newgametype = d;
				else
				{
					CONS_Alert(CONS_ERROR,
							"Gametype number %d is out of range. Use a number between"
							" 0 and %d inclusive. ...Or just use the name. :v\n",
							d,
							gametypecount-1);
					Z_Free(realmapname);
					Z_Free(mapname);
					return;
				}
			}
			else
			{
				CONS_Alert(CONS_ERROR,
						"'%s' is not a gametype.\n",
						gametypename);
				Z_Free(realmapname);
				Z_Free(mapname);
				return;
			}
		}
	}
	else if (!Playing())
	{
		newresetplayers = true;
		if (mapheaderinfo[newmapnum-1])
		{
			// Let's just guess so we don't have to specify the gametype EVERY time...
			newgametype = (mapheaderinfo[newmapnum-1]->typeoflevel & TOL_RACE) ? GT_RACE : GT_BATTLE;
		}
	}

	// new encoremode value
	if (option_encore)
	{
		newencoremode = !newencoremode;

		if (!M_SecretUnlocked(SECRET_ENCORE) && newencoremode == true && !option_force)
		{
			CONS_Alert(CONS_NOTICE, M_GetText("You haven't unlocked Encore Mode yet!\n"));
			return;
		}
	}

	if (!option_force && newgametype == gametype && Playing()) // SRB2Kart
		newresetplayers = false; // if not forcing and gametypes is the same

	// don't use a gametype the map doesn't support
	if (cv_debug || option_force || cv_skipmapcheck.value)
		fromlevelselect = false; // The player wants us to trek on anyway.  Do so.
	// G_TOLFlag handles both multiplayer gametype and ignores it for !multiplayer
	else
	{
		if (!(
					mapheaderinfo[newmapnum-1] &&
					mapheaderinfo[newmapnum-1]->typeoflevel & G_TOLFlag(newgametype)
		))
		{
			CONS_Alert(CONS_WARNING, M_GetText("%s (%s) doesn't support %s mode!\n(Use -force to override)\n"), realmapname, G_BuildMapName(newmapnum),
				(multiplayer ? gametype_cons_t[newgametype].strvalue : "Single Player"));
			Z_Free(realmapname);
			Z_Free(mapname);
			return;
		}
		else
		{
			fromlevelselect =
				( netgame || multiplayer ) &&
				newgametype == gametype    &&
				gametypedefaultrules[newgametype] & GTR_CAMPAIGN;
		}
	}

	if (!(netgame || multiplayer))
	{
		if (newgametype == GT_BATTLE)
		{
			grandprixinfo.gp = false;
			K_ResetBossInfo();

			if (mapheaderinfo[newmapnum-1] &&
				mapheaderinfo[newmapnum-1]->typeoflevel & TOL_BOSS)
			{
				bossinfo.boss = true;
				bossinfo.encore = newencoremode;
			}
		}
		else // default GP
		{
			grandprixinfo.gamespeed = (cv_kartspeed.value == KARTSPEED_AUTO ? KARTSPEED_NORMAL : cv_kartspeed.value);
			grandprixinfo.masterbots = false;

			if (option_skill)
			{
				const char *masterstr = "Master";
				const char *skillname = COM_Argv(option_skill + 1);
				INT32 newskill = -1;
				INT32 j;

				if (!strcasecmp(masterstr, skillname))
				{
					newskill = KARTGP_MASTER;
				}
				else
				{
					for (j = 0; kartspeed_cons_t[j].strvalue; j++)
					{
						if (!strcasecmp(kartspeed_cons_t[j].strvalue, skillname))
						{
							newskill = (INT16)kartspeed_cons_t[j].value;
							break;
						}
					}

					if (!kartspeed_cons_t[j].strvalue) // reached end of the list with no match
					{
						INT32 num = atoi(COM_Argv(option_skill + 1)); // assume they gave us a skill number, which is okay too
						if (num >= KARTSPEED_EASY && num <= KARTGP_MASTER)
							newskill = (INT16)num;
					}
				}

				if (newskill != -1)
				{
					if (newskill == KARTGP_MASTER)
					{
						grandprixinfo.gamespeed = KARTSPEED_HARD;
						grandprixinfo.masterbots = true;
					}
					else
					{
						grandprixinfo.gamespeed = newskill;
						grandprixinfo.masterbots = false;
					}
				}
			}

			grandprixinfo.encore = newencoremode;

			grandprixinfo.gp = true;
			grandprixinfo.roundnum = 0;
			grandprixinfo.cup = NULL;
			grandprixinfo.wonround = false;

			bossinfo.boss = false;

			grandprixinfo.initalize = true;
		}
	}

	// Prevent warping to locked levels
	// ... unless you're in a dedicated server.  Yes, technically this means you can view any level by
	// running a dedicated server and joining it yourself, but that's better than making dedicated server's
	// lives hell.
	if (!dedicated && M_MapLocked(newmapnum))
	{
		CONS_Alert(CONS_NOTICE, M_GetText("You need to unlock this level before you can warp to it!\n"));
		Z_Free(realmapname);
		Z_Free(mapname);
		return;
	}

	if (tutorialmode && tutorialgcs)
	{
		G_CopyControls(gamecontrol[0], gamecontroldefault[0][gcs_custom], gcl_full, num_gcl_full); // using gcs_custom as temp storage
	}
	tutorialmode = false; // warping takes us out of tutorial mode

	D_MapChange(newmapnum, newgametype, newencoremode, newresetplayers, 0, false, fromlevelselect);

	Z_Free(realmapname);
}

/** Receives a map command and changes the map.
  *
  * \param cp        Data buffer.
  * \param playernum Player number responsible for the message. Should be
  *                  ::serverplayer or ::adminplayer.
  * \sa D_MapChange
  */
static void Got_Mapcmd(UINT8 **cp, INT32 playernum)
{
	char mapname[MAX_WADPATH+1];
	UINT8 flags;
	INT32 resetplayer = 1, lastgametype;
	UINT8 skipprecutscene, FLS;
	boolean pencoremode;

	forceresetplayers = deferencoremode = false;

	if (playernum != serverplayer && !IsPlayerAdmin(playernum))
	{
		CONS_Alert(CONS_WARNING, M_GetText("Illegal map change received from %s\n"), player_names[playernum]);
		if (server)
			SendKick(playernum, KICK_MSG_CON_FAIL);
		return;
	}

	if (chmappending)
		chmappending--;

	flags = READUINT8(*cp);

	pencoremode = ((flags & 1) != 0);

	resetplayer = ((flags & (1<<1)) == 0);

	lastgametype = gametype;
	gametype = READUINT8(*cp);
	G_SetGametype(gametype); // I fear putting that macro as an argument

	if (gametype < 0 || gametype >= gametypecount)
		gametype = lastgametype;
	else if (gametype != lastgametype)
		D_GameTypeChanged(lastgametype); // emulate consvar_t behavior for gametype

	if (!(gametyperules & GTR_CIRCUIT) && !bossinfo.boss)
		pencoremode = false;

	skipprecutscene = ((flags & (1<<2)) != 0);

	FLS = ((flags & (1<<3)) != 0);

	READSTRINGN(*cp, mapname, MAX_WADPATH);

	if (netgame)
		P_SetRandSeed(READUINT32(*cp));

	if (!skipprecutscene)
	{
		DEBFILE(va("Warping to %s [resetplayer=%d lastgametype=%d gametype=%d cpnd=%d]\n",
			mapname, resetplayer, lastgametype, gametype, chmappending));
		CON_LogMessage(M_GetText("Speeding off to level...\n"));
	}

	if (demo.playback && !demo.timing)
		precache = false;

	if (resetplayer && !FLS)
	{
		emeralds = 0;
		memset(&luabanks, 0, sizeof(luabanks));
	}

	demo.savemode = (cv_recordmultiplayerdemos.value == 2) ? DSM_WILLAUTOSAVE : DSM_NOTSAVING;
	demo.savebutton = 0;

	G_InitNew(pencoremode, mapname, resetplayer, skipprecutscene, FLS);
	if (demo.playback && !demo.timing)
		precache = true;
	if (demo.timing)
		G_DoneLevelLoad();

	if (metalrecording)
		G_BeginMetal();
	if (demo.recording) // Okay, level loaded, character spawned and skinned,
		G_BeginRecording(); // I AM NOW READY TO RECORD.
	demo.deferstart = true;

#ifdef HAVE_DISCORDRPC
	DRPC_UpdatePresence();
#endif
}

static void Command_RandomMap(void)
{
	INT32 oldmapnum;
	INT32 newmapnum;
	INT32 newgametype;
	boolean newencoremode;
	boolean newresetplayers;

	if (client && !IsPlayerAdmin(consoleplayer))
	{
		CONS_Printf(M_GetText("Only the server or a remote admin can use this.\n"));
		return;
	}

	// TODO: Handle singleplayer conditions.
	// The existing ones are way too annoyingly complicated and "anti-cheat" for my tastes.

	if (Playing())
	{
		newgametype = gametype;
		newencoremode = encoremode;
		newresetplayers = false;

		if (gamestate == GS_LEVEL)
		{
			oldmapnum = gamemap-1;
		}
		else
		{
			oldmapnum = prevmap;
		}
	}
	else
	{
		newgametype = cv_newgametype.value;
		newencoremode = false;
		newresetplayers = true;
		oldmapnum = -1;
	}

	newmapnum = G_RandMap(G_TOLFlag(newgametype), oldmapnum, 0, 0, false, NULL) + 1;
	D_MapChange(newmapnum, newgametype, newencoremode, newresetplayers, 0, false, false);
}

static void Command_RestartLevel(void)
{
	if (client && !IsPlayerAdmin(consoleplayer))
	{
		CONS_Printf(M_GetText("Only the server or a remote admin can use this.\n"));
		return;
	}

	if (!Playing())
	{
		CONS_Printf(M_GetText("You must be in a game to use this.\n"));
		return;
	}

	D_MapChange(gamemap, gametype, encoremode, false, 0, false, false);
}

static void Command_Pause(void)
{
	UINT8 buf[2];
	UINT8 *cp = buf;

	if (COM_Argc() > 1)
		WRITEUINT8(cp, (char)(atoi(COM_Argv(1)) != 0));
	else
		WRITEUINT8(cp, (char)(!paused));

	if (dedicated)
		WRITEUINT8(cp, 1);
	else
		WRITEUINT8(cp, 0);

	if (cv_pause.value || server || (IsPlayerAdmin(consoleplayer)))
	{
		if (!paused && (!(gamestate == GS_LEVEL || gamestate == GS_INTERMISSION || gamestate == GS_VOTING || gamestate == GS_WAITINGPLAYERS)))
		{
			CONS_Printf(M_GetText("You can't pause here.\n"));
			return;
		}
		else if (modeattacking)	// in time attack, pausing restarts the map
		{
			M_ModeAttackRetry(0);	// directly call from m_menu;
			return;
		}

		SendNetXCmd(XD_PAUSE, &buf, 2);
	}
	else
		CONS_Printf(M_GetText("Only the server or a remote admin can use this.\n"));
}

static void Got_Pause(UINT8 **cp, INT32 playernum)
{
	UINT8 dedicatedpause = false;
	const char *playername;

	if (netgame && !cv_pause.value && playernum != serverplayer && !IsPlayerAdmin(playernum))
	{
		CONS_Alert(CONS_WARNING, M_GetText("Illegal pause command received from %s\n"), player_names[playernum]);
		if (server)
			SendKick(playernum, KICK_MSG_CON_FAIL);
		return;
	}

	if (modeattacking && !demo.playback)
		return;

	paused = READUINT8(*cp);
	dedicatedpause = READUINT8(*cp);

	if (!demo.playback)
	{
		if (netgame)
		{
			if (dedicatedpause)
				playername = "SERVER";
			else
				playername = player_names[playernum];

			if (paused)
				CONS_Printf(M_GetText("Game paused by %s\n"), playername);
			else
				CONS_Printf(M_GetText("Game unpaused by %s\n"), playername);
		}

		if (paused)
		{
			if (!menuactive || netgame)
				S_PauseAudio();
		}
		else
			S_ResumeAudio();
	}

	I_UpdateMouseGrab();
}

// Command for stuck characters in netgames, griefing, etc.
static void Command_Respawn(void)
{
	UINT8 buf[4];
	UINT8 *cp = buf;



	if (!(gamestate == GS_LEVEL || gamestate == GS_INTERMISSION || gamestate == GS_VOTING))
	{
		CONS_Printf(M_GetText("You must be in a level to use this.\n"));
		return;
	}

	if (players[consoleplayer].mo && !P_IsObjectOnGround(players[consoleplayer].mo)) // KART: Nice try, but no, you won't be cheesing spb anymore.
	{
		CONS_Printf(M_GetText("You must be on the floor to use this.\n"));
		return;
	}

	// todo: this probably isnt necessary anymore with v2
	if (players[consoleplayer].mo && (P_PlayerInPain(&players[consoleplayer]) || spbplace == players[consoleplayer].position)) // KART: Nice try, but no, you won't be cheesing spb anymore (x2)
	{
		CONS_Printf(M_GetText("Nice try.\n"));
		return;
	}

	WRITEINT32(cp, consoleplayer);
	SendNetXCmd(XD_RESPAWN, &buf, 4);
}

static void Got_Respawn(UINT8 **cp, INT32 playernum)
{
	INT32 respawnplayer = READINT32(*cp);

	// You can't respawn someone else. Nice try, there.
	if (respawnplayer != playernum || P_PlayerInPain(&players[respawnplayer]) || spbplace == players[respawnplayer].position) // srb2kart: "|| (!(gametyperules & GTR_CIRCUIT))"
	{
		CONS_Alert(CONS_WARNING, M_GetText("Illegal respawn command received from %s\n"), player_names[playernum]);
		if (server)
			SendKick(playernum, KICK_MSG_CON_FAIL);
		return;
	}

	if (players[respawnplayer].mo)
	{
		// incase the above checks were modified to allow sending a respawn on these occasions:
		if (!P_IsObjectOnGround(players[respawnplayer].mo))
			return;

		P_DamageMobj(players[respawnplayer].mo, NULL, NULL, 1,DMG_INSTAKILL);
		demo_extradata[playernum] |= DXD_RESPAWN;
	}
}

/** Deals with an ::XD_RANDOMSEED message in a netgame.
  * These messages set the position of the random number LUT and are crucial to
  * correct synchronization.
  *
  * Such a message should only ever come from the ::serverplayer. If it comes
  * from any other player, it is ignored.
  *
  * \param cp        Data buffer.
  * \param playernum Player responsible for the message. Must be ::serverplayer.
  * \author Graue <graue@oceanbase.org>
  */
static void Got_RandomSeed(UINT8 **cp, INT32 playernum)
{
	UINT32 seed;

	seed = READUINT32(*cp);

	if (playernum != serverplayer) // it's not from the server, wtf?
		return;

	P_SetRandSeed(seed);
}

/** Clears all players' scores in a netgame.
  * Only the server or a remote admin can use this command, for obvious reasons.
  *
  * \sa XD_CLEARSCORES, Got_Clearscores
  * \author SSNTails <http://www.ssntails.org>
  */
static void Command_Clearscores_f(void)
{
	if (!(server || (IsPlayerAdmin(consoleplayer))))
		return;

	SendNetXCmd(XD_CLEARSCORES, NULL, 1);
}

/** Handles an ::XD_CLEARSCORES message, which resets all players' scores in a
  * netgame to zero.
  *
  * \param cp        Data buffer.
  * \param playernum Player responsible for the message. Must be ::serverplayer
  *                  or ::adminplayer.
  * \sa XD_CLEARSCORES, Command_Clearscores_f
  * \author SSNTails <http://www.ssntails.org>
  */
static void Got_Clearscores(UINT8 **cp, INT32 playernum)
{
	INT32 i;

	(void)cp;
	if (playernum != serverplayer && !IsPlayerAdmin(playernum))
	{
		CONS_Alert(CONS_WARNING, M_GetText("Illegal clear scores command received from %s\n"), player_names[playernum]);
		if (server)
			SendKick(playernum, KICK_MSG_CON_FAIL);
		return;
	}

	for (i = 0; i < MAXPLAYERS; i++)
		players[i].score = 0;

	CONS_Printf(M_GetText("Scores have been reset by the server.\n"));
}

// Team changing functions
static void HandleTeamChangeCommand(UINT8 localplayer)
{
	const char *commandname = NULL;
	changeteam_union NetPacket;
	boolean error = false;
	UINT16 usvalue;
	NetPacket.value.l = NetPacket.value.b = 0;

	switch (localplayer)
	{
		case 0:
			commandname = "changeteam";
			break;
		default:
			commandname = va("changeteam%d", localplayer+1);
			break;
	}

	//      0         1
	// changeteam  <color>

	if (COM_Argc() <= 1)
	{
		if (G_GametypeHasTeams())
			CONS_Printf(M_GetText("%s <team>: switch to a new team (%s)\n"), commandname, "red, blue or spectator");
		else if (G_GametypeHasSpectators())
			CONS_Printf(M_GetText("%s <team>: switch to a new team (%s)\n"), commandname, "spectator or playing");
		else
			CONS_Alert(CONS_NOTICE, M_GetText("This command cannot be used in this gametype.\n"));
		return;
	}

	if (G_GametypeHasTeams())
	{
		if (!strcasecmp(COM_Argv(1), "red") || !strcasecmp(COM_Argv(1), "1"))
			NetPacket.packet.newteam = 1;
		else if (!strcasecmp(COM_Argv(1), "blue") || !strcasecmp(COM_Argv(1), "2"))
			NetPacket.packet.newteam = 2;
		else if (!strcasecmp(COM_Argv(1), "spectator") || !strcasecmp(COM_Argv(1), "0"))
			NetPacket.packet.newteam = 0;
		else
			error = true;
	}
	else if (G_GametypeHasSpectators())
	{
		if (!strcasecmp(COM_Argv(1), "spectator") || !strcasecmp(COM_Argv(1), "0"))
			NetPacket.packet.newteam = 0;
		else if (!strcasecmp(COM_Argv(1), "playing") || !strcasecmp(COM_Argv(1), "1"))
			NetPacket.packet.newteam = 3;
		else
			error = true;
	}
	else
	{
		CONS_Alert(CONS_NOTICE, M_GetText("This command cannot be used in this gametype.\n"));
		return;
	}

	if (error)
	{
		if (G_GametypeHasTeams())
			CONS_Printf(M_GetText("%s <team>: switch to a new team (%s)\n"), commandname, "red, blue or spectator");
		else if (G_GametypeHasSpectators())
			CONS_Printf(M_GetText("%s <team>: switch to a new team (%s)\n"), commandname, "spectator or playing");
		return;
	}

	if (players[g_localplayers[localplayer]].spectator)
		error = !(NetPacket.packet.newteam || (players[g_localplayers[localplayer]].pflags & PF_WANTSTOJOIN)); // :lancer:
	else if (G_GametypeHasTeams())
		error = (NetPacket.packet.newteam == players[g_localplayers[localplayer]].ctfteam);
	else if (G_GametypeHasSpectators() && !players[g_localplayers[localplayer]].spectator)
		error = (NetPacket.packet.newteam == 3);
#ifdef PARANOIA
	else
		I_Error("Invalid gametype after initial checks!");
#endif

	if (error)
	{
		CONS_Alert(CONS_NOTICE, M_GetText("You're already on that team!\n"));
		return;
	}

	if (!cv_allowteamchange.value && NetPacket.packet.newteam) // allow swapping to spectator even in locked teams.
	{
		CONS_Alert(CONS_NOTICE, M_GetText("The server is not allowing team changes at the moment.\n"));
		return;
	}

	usvalue = SHORT(NetPacket.value.l|NetPacket.value.b);
	SendNetXCmdForPlayer(localplayer, XD_TEAMCHANGE, &usvalue, sizeof(usvalue));
}

static void Command_Teamchange_f(void)
{
	HandleTeamChangeCommand(0);
}

static void Command_Teamchange2_f(void)
{
	HandleTeamChangeCommand(1);
}

static void Command_Teamchange3_f(void)
{
	HandleTeamChangeCommand(2);
}

static void Command_Teamchange4_f(void)
{
	HandleTeamChangeCommand(3);
}

static void Command_ServerTeamChange_f(void)
{
	changeteam_union NetPacket;
	boolean error = false;
	UINT16 usvalue;
	NetPacket.value.l = NetPacket.value.b = 0;

	if (!(server || (IsPlayerAdmin(consoleplayer))))
	{
		CONS_Printf(M_GetText("Only the server or a remote admin can use this.\n"));
		return;
	}

	//        0              1         2
	// serverchangeteam <playernum>  <team>

	if (COM_Argc() < 3)
	{
		if (G_GametypeHasTeams())
			CONS_Printf(M_GetText("serverchangeteam <playernum> <team>: switch player to a new team (%s)\n"), "red, blue or spectator");
		else if (G_GametypeHasSpectators())
			CONS_Printf(M_GetText("serverchangeteam <playernum> <team>: switch player to a new team (%s)\n"), "spectator or playing");
		else
			CONS_Alert(CONS_NOTICE, M_GetText("This command cannot be used in this gametype.\n"));
		return;
	}

	if (G_GametypeHasTeams())
	{
		if (!strcasecmp(COM_Argv(2), "red") || !strcasecmp(COM_Argv(2), "1"))
			NetPacket.packet.newteam = 1;
		else if (!strcasecmp(COM_Argv(2), "blue") || !strcasecmp(COM_Argv(2), "2"))
			NetPacket.packet.newteam = 2;
		else if (!strcasecmp(COM_Argv(2), "spectator") || !strcasecmp(COM_Argv(2), "0"))
			NetPacket.packet.newteam = 0;
		else
			error = true;
	}
	else if (G_GametypeHasSpectators())
	{
		if (!strcasecmp(COM_Argv(2), "spectator") || !strcasecmp(COM_Argv(2), "0"))
			NetPacket.packet.newteam = 0;
		else if (!strcasecmp(COM_Argv(2), "playing") || !strcasecmp(COM_Argv(2), "1"))
			NetPacket.packet.newteam = 3;
		else
			error = true;
	}
	else
	{
		CONS_Alert(CONS_NOTICE, M_GetText("This command cannot be used in this gametype.\n"));
		return;
	}

	if (error)
	{
		if (G_GametypeHasTeams())
			CONS_Printf(M_GetText("serverchangeteam <playernum> <team>: switch player to a new team (%s)\n"), "red, blue or spectator");
		else if (G_GametypeHasSpectators())
			CONS_Printf(M_GetText("serverchangeteam <playernum> <team>: switch player to a new team (%s)\n"), "spectator or playing");
		return;
	}

	NetPacket.packet.playernum = atoi(COM_Argv(1));

	if (!playeringame[NetPacket.packet.playernum])
	{
		CONS_Alert(CONS_NOTICE, M_GetText("There is no player %d!\n"), NetPacket.packet.playernum);
		return;
	}

	if (G_GametypeHasTeams())
	{
		if (NetPacket.packet.newteam == players[NetPacket.packet.playernum].ctfteam ||
			(players[NetPacket.packet.playernum].spectator && !NetPacket.packet.newteam))
			error = true;
	}
	else if (G_GametypeHasSpectators())
	{
		if ((players[NetPacket.packet.playernum].spectator && !NetPacket.packet.newteam) ||
			(!players[NetPacket.packet.playernum].spectator && NetPacket.packet.newteam == 3))
			error = true;
	}
#ifdef PARANOIA
	else
		I_Error("Invalid gametype after initial checks!");
#endif

	if (error)
	{
		CONS_Alert(CONS_NOTICE, M_GetText("That player is already on that team!\n"));
		return;
	}

	NetPacket.packet.verification = true; // This signals that it's a server change

	usvalue = SHORT(NetPacket.value.l|NetPacket.value.b);
	SendNetXCmd(XD_TEAMCHANGE, &usvalue, sizeof(usvalue));
}

//todo: This and the other teamchange functions are getting too long and messy. Needs cleaning.
static void Got_Teamchange(UINT8 **cp, INT32 playernum)
{
	changeteam_union NetPacket;
	boolean error = false, wasspectator = false;
	NetPacket.value.l = NetPacket.value.b = READINT16(*cp);

	if (!G_GametypeHasTeams() && !G_GametypeHasSpectators()) //Make sure you're in the right gametype.
	{
		// this should never happen unless the client is hacked/buggy
		CONS_Alert(CONS_WARNING, M_GetText("Illegal team change received from player %s\n"), player_names[playernum]);
		if (server)
			SendKick(playernum, KICK_MSG_CON_FAIL);
	}

	if (NetPacket.packet.verification) // Special marker that the server sent the request
	{
		if (playernum != serverplayer && (!IsPlayerAdmin(playernum)))
		{
			CONS_Alert(CONS_WARNING, M_GetText("Illegal team change received from player %s\n"), player_names[playernum]);
			if (server)
				SendKick(playernum, KICK_MSG_CON_FAIL);
			return;
		}
		playernum = NetPacket.packet.playernum;
	}

	// Prevent multiple changes in one go.
	if (players[playernum].spectator && !(players[playernum].pflags & PF_WANTSTOJOIN) && !NetPacket.packet.newteam)
		return;
	else if (G_GametypeHasTeams())
	{
		if (NetPacket.packet.newteam && (NetPacket.packet.newteam == (unsigned)players[playernum].ctfteam))
			return;
	}
	else if (G_GametypeHasSpectators())
	{
		if (!players[playernum].spectator && NetPacket.packet.newteam == 3)
			return;
	}
	else
	{
		if (playernum != serverplayer && (!IsPlayerAdmin(playernum)))
		{
			CONS_Alert(CONS_WARNING, M_GetText("Illegal team change received from player %s\n"), player_names[playernum]);
			if (server)
				SendKick(playernum, KICK_MSG_CON_FAIL);
		}
		return;
	}

	// Don't switch team, just go away, please, go awaayyyy, aaauuauugghhhghgh
	if (!LUA_HookTeamSwitch(&players[playernum], NetPacket.packet.newteam, players[playernum].spectator, NetPacket.packet.autobalance, NetPacket.packet.scrambled))
		return;

	//Make sure that the right team number is sent. Keep in mind that normal clients cannot change to certain teams in certain gametypes.
#ifdef PARANOIA
	if (!G_GametypeHasTeams() && !G_GametypeHasSpectators())
		I_Error("Invalid gametype after initial checks!");
#endif

	if (!cv_allowteamchange.value)
	{
		if (!NetPacket.packet.verification && NetPacket.packet.newteam)
			error = true; //Only admin can change status, unless changing to spectator.
	}

	if (server && ((NetPacket.packet.newteam < 0 || NetPacket.packet.newteam > 3) || error))
	{
		CONS_Alert(CONS_WARNING, M_GetText("Illegal team change received from player %s\n"), player_names[playernum]);
		SendKick(playernum, KICK_MSG_CON_FAIL);
	}

	//Safety first!
	// (not respawning spectators here...)
	if (!players[playernum].spectator)
	{
		if (players[playernum].mo)
		{
			P_DamageMobj(players[playernum].mo, NULL, NULL, 1,
				(NetPacket.packet.newteam ? DMG_INSTAKILL : DMG_SPECTATOR));
		}
		//else
		if (!NetPacket.packet.newteam)
		{
			players[playernum].playerstate = PST_REBORN;
		}
		
	}
	else
		wasspectator = true;

	players[playernum].pflags &= ~PF_WANTSTOJOIN;

	//Now that we've done our error checking and killed the player
	//if necessary, put the player on the correct team/status.
	if (G_GametypeHasTeams())
	{
		if (!NetPacket.packet.newteam)
		{
			players[playernum].ctfteam = 0;
			players[playernum].spectator = true;
		}
		else
		{
			players[playernum].ctfteam = NetPacket.packet.newteam;
			players[playernum].pflags |= PF_WANTSTOJOIN; //players[playernum].spectator = false;
		}
	}
	else if (G_GametypeHasSpectators())
	{
		if (!NetPacket.packet.newteam)
			players[playernum].spectator = true;
		else
			players[playernum].pflags |= PF_WANTSTOJOIN; //players[playernum].spectator = false;
	}

	if (NetPacket.packet.autobalance)
	{
		if (NetPacket.packet.newteam == 1)
			CONS_Printf(M_GetText("%s was autobalanced to the %c%s%c.\n"), player_names[playernum], '\x85', M_GetText("Red Team"), '\x80');
		else if (NetPacket.packet.newteam == 2)
			CONS_Printf(M_GetText("%s was autobalanced to the %c%s%c.\n"), player_names[playernum], '\x84', M_GetText("Blue Team"), '\x80');
	}
	else if (NetPacket.packet.scrambled)
	{
		if (NetPacket.packet.newteam == 1)
			CONS_Printf(M_GetText("%s was scrambled to the %c%s%c.\n"), player_names[playernum], '\x85', M_GetText("Red Team"), '\x80');
		else if (NetPacket.packet.newteam == 2)
			CONS_Printf(M_GetText("%s was scrambled to the %c%s%c.\n"), player_names[playernum], '\x84', M_GetText("Blue Team"), '\x80');
	}
	else if (NetPacket.packet.newteam == 1)
	{
		CONS_Printf(M_GetText("%s switched to the %c%s%c.\n"), player_names[playernum], '\x85', M_GetText("Red Team"), '\x80');
	}
	else if (NetPacket.packet.newteam == 2)
	{
		CONS_Printf(M_GetText("%s switched to the %c%s%c.\n"), player_names[playernum], '\x84', M_GetText("Blue Team"), '\x80');
	}
	else if (NetPacket.packet.newteam == 0 && !wasspectator)
		HU_AddChatText(va("\x82*%s became a spectator.", player_names[playernum]), false); // "entered the game" text was moved to P_SpectatorJoinGame

	// Reset away view (some code referenced from P_SpectatorJoinGame)
	{
		UINT8 i = 0;
		INT32 *localplayertable = (splitscreen_partied[consoleplayer] ? splitscreen_party[consoleplayer] : g_localplayers);

		for (i = 0; i < r_splitscreen; i++)
		{
			if (localplayertable[i] == playernum)
			{
				LUA_HookViewpointSwitch(players+playernum, players+playernum, true);
				displayplayers[i] = playernum;
				break;
			}
		}
	}

	/*if (G_GametypeHasTeams())
	{
		if (NetPacket.packet.newteam)
		{
			UINT8 i;
			for (i = 0; i <= splitscreen; i++)
			{
				if (playernum == g_localplayers[i]) //CTF and Team Match colors.
					CV_SetValue(&cv_playercolor[i], NetPacket.packet.newteam + 5); - -this calculation is totally wrong
			}
		}
	}*/

	if (gamestate != GS_LEVEL)
		return;

	demo_extradata[playernum] |= DXD_PLAYSTATE;

	// Clear player score and rings if a spectator.
	if (players[playernum].spectator)
	{
		players[playernum].spectatorreentry = (cv_spectatorreentry.value * TICRATE);
		
		if (gametyperules & GTR_BUMPERS) // SRB2kart
		{
			players[playernum].roundscore = 0;
			K_CalculateBattleWanted();
		}

		K_PlayerForfeit(playernum, true);

		if (players[playernum].mo)
			players[playernum].mo->health = 1;

		K_StripItems(&players[playernum]);
	}

	K_CheckBumpers(); // SRB2Kart
	P_CheckRacers(); // also SRB2Kart
}

//
// Attempts to make password system a little sane without
// rewriting the entire goddamn XD_file system
//
#define BASESALT "basepasswordstorage"

void D_SetPassword(const char *pw)
{
	D_MD5PasswordPass((const UINT8 *)pw, strlen(pw), BASESALT, &adminpassmd5);
	adminpasswordset = true;
}

// Remote Administration
static void Command_Changepassword_f(void)
{
#ifdef NOMD5
	// If we have no MD5 support then completely disable XD_LOGIN responses for security.
	CONS_Alert(CONS_NOTICE, "Remote administration commands are not supported in this build.\n");
#else
	if (client) // cannot change remotely
	{
		CONS_Printf(M_GetText("Only the server can use this.\n"));
		return;
	}

	if (COM_Argc() != 2)
	{
		CONS_Printf(M_GetText("password <password>: change remote admin password\n"));
		return;
	}

	D_SetPassword(COM_Argv(1));
	CONS_Printf(M_GetText("Password set.\n"));
#endif
}

static void Command_Login_f(void)
{
#ifdef NOMD5
	// If we have no MD5 support then completely disable XD_LOGIN responses for security.
	CONS_Alert(CONS_NOTICE, "Remote administration commands are not supported in this build.\n");
#else
	const char *pw;

	if (!netgame)
	{
		CONS_Printf(M_GetText("This only works in a netgame.\n"));
		return;
	}

	// If the server uses login, it will effectively just remove admin privileges
	// from whoever has them. This is good.
	if (COM_Argc() != 2)
	{
		CONS_Printf(M_GetText("login <password>: Administrator login\n"));
		return;
	}

	pw = COM_Argv(1);

	// Do the base pass to get what the server has (or should?)
	D_MD5PasswordPass((const UINT8 *)pw, strlen(pw), BASESALT, &netbuffer->u.md5sum);

	// Do the final pass to get the comparison the server will come up with
	D_MD5PasswordPass(netbuffer->u.md5sum, 16, va("PNUM%02d", consoleplayer), &netbuffer->u.md5sum);

	CONS_Printf(M_GetText("Sending login... (Notice only given if password is correct.)\n"));

	netbuffer->packettype = PT_LOGIN;
	HSendPacket(servernode, true, 0, 16);
#endif
}

boolean IsPlayerAdmin(INT32 playernum)
{
#if defined (TESTERS) || defined (HOSTTESTERS)
	(void)playernum;
	return false;
#elif defined (DEVELOP)
	return playernum != serverplayer;
#else
	INT32 i;
	for (i = 0; i < MAXPLAYERS; i++)
		if (playernum == adminplayers[i])
			return true;

	return false;
#endif
}

void SetAdminPlayer(INT32 playernum)
{
	INT32 i;
	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (playernum == adminplayers[i])
			return; // Player is already admin

		if (adminplayers[i] == -1)
		{
			adminplayers[i] = playernum; // Set the player to a free spot
			break; // End the loop now. If it keeps going, the same player might get assigned to two slots.
		}
	}
}

void ClearAdminPlayers(void)
{
	memset(adminplayers, -1, sizeof(adminplayers));
}

void RemoveAdminPlayer(INT32 playernum)
{
	INT32 i;
	for (i = 0; i < MAXPLAYERS; i++)
		if (playernum == adminplayers[i])
			adminplayers[i] = -1;
}

static void Command_Verify_f(void)
{
	char buf[8]; // Should be plenty
	char *temp;
	INT32 playernum;

	if (client)
	{
		CONS_Printf(M_GetText("Only the server can use this.\n"));
		return;
	}

	if (!netgame)
	{
		CONS_Printf(M_GetText("This only works in a netgame.\n"));
		return;
	}

	if (COM_Argc() != 2)
	{
		CONS_Printf(M_GetText("promote <playernum>: give admin privileges to a player\n"));
		return;
	}

	strlcpy(buf, COM_Argv(1), sizeof (buf));

	playernum = atoi(buf);

	temp = buf;

	WRITEUINT8(temp, playernum);

	if (playeringame[playernum])
		SendNetXCmd(XD_VERIFIED, buf, 1);
}

static void Got_Verification(UINT8 **cp, INT32 playernum)
{
	INT16 num = READUINT8(*cp);

	if (playernum != serverplayer) // it's not from the server (hacker or bug)
	{
		CONS_Alert(CONS_WARNING, M_GetText("Illegal verification received from %s (serverplayer is %s)\n"), player_names[playernum], player_names[serverplayer]);
		if (server)
			SendKick(playernum, KICK_MSG_CON_FAIL);
		return;
	}

	SetAdminPlayer(num);

	if (num != consoleplayer)
		return;

	CONS_Printf(M_GetText("You are now a server administrator.\n"));
}

static void Command_RemoveAdmin_f(void)
{
	char buf[8]; // Should be plenty
	char *temp;
	INT32 playernum;

	if (client)
	{
		CONS_Printf(M_GetText("Only the server can use this.\n"));
		return;
	}

	if (COM_Argc() != 2)
	{
		CONS_Printf(M_GetText("demote <playernum>: remove admin privileges from a player\n"));
		return;
	}

	strlcpy(buf, COM_Argv(1), sizeof(buf));

	playernum = atoi(buf);

	temp = buf;

	WRITEUINT8(temp, playernum);

	if (playeringame[playernum])
		SendNetXCmd(XD_DEMOTED, buf, 1);
}

static void Got_Removal(UINT8 **cp, INT32 playernum)
{
	UINT8 num = READUINT8(*cp);

	if (playernum != serverplayer) // it's not from the server (hacker or bug)
	{
		CONS_Alert(CONS_WARNING, M_GetText("Illegal demotion received from %s (serverplayer is %s)\n"), player_names[playernum], player_names[serverplayer]);
		if (server)
			SendKick(playernum, KICK_MSG_CON_FAIL);
		return;
	}

	RemoveAdminPlayer(num);

	if (num != consoleplayer)
		return;

	CONS_Printf(M_GetText("You are no longer a server administrator.\n"));
}

void Schedule_Run(void)
{
	size_t i;

	if (schedule_len == 0)
	{
		// No scheduled tasks to run.
		return;
	}

	if (!cv_schedule.value)
	{
		// We don't WANT to run tasks.
		return;
	}

	if (K_CanChangeRules() == false)
	{
		// Don't engage in automation while in a restricted context.
		return;
	}

	for (i = 0; i < schedule_len; i++)
	{
		scheduleTask_t *task = schedule[i];

		if (task == NULL)
		{
			// Shouldn't happen.
			break;
		}

		if (task->timer > 0)
		{
			task->timer--;
		}

		if (task->timer == 0)
		{
			// Reset timer
			task->timer = task->basetime;

			// Run command for server
			if (server)
			{
				CONS_Printf(
					"%d seconds elapsed, running \"" "\x82" "%s" "\x80" "\".\n",
					task->basetime,
					task->command
				);

				COM_BufAddText(task->command);
				COM_BufAddText("\n");
			}
		}
	}
}

void Schedule_Insert(scheduleTask_t *addTask)
{
	if (schedule_len >= schedule_size)
	{
		if (schedule_size == 0)
		{
			schedule_size = 8;
		}
		else
		{
			schedule_size *= 2;
		}
		
		schedule = Z_ReallocAlign(
			(void*) schedule,
			sizeof(scheduleTask_t*) * schedule_size,
			PU_STATIC,
			NULL,
			sizeof(scheduleTask_t*) * 8
		);
	}

	schedule[schedule_len] = addTask;
	schedule_len++;
}

void Schedule_Add(INT16 basetime, INT16 timeleft, const char *command)
{
	scheduleTask_t *task = (scheduleTask_t*) Z_CallocAlign(
		sizeof(scheduleTask_t),
		PU_STATIC,
		NULL,
		sizeof(scheduleTask_t) * 8
	);

	task->basetime = basetime;
	task->timer = timeleft;
	task->command = Z_StrDup(command);

	Schedule_Insert(task);
}

void Schedule_Clear(void)
{
	size_t i;

	for (i = 0; i < schedule_len; i++)
	{
		scheduleTask_t *task = schedule[i];

		if (task->command)
			Z_Free(task->command);
	}

	schedule_len = 0;
	schedule_size = 0;
	schedule = NULL;
}

void Automate_Set(automateEvents_t type, const char *command)
{
	if (!server)
	{
		// Since there's no list command or anything for this,
		// we don't need this code to run for anyone but the server.
		return;
	}

	if (automate_commands[type] != NULL)
	{
		// Free the old command.
		Z_Free(automate_commands[type]);
	}

	if (command == NULL || strlen(command) == 0)
	{
		// Remove the command.
		automate_commands[type] = NULL;
	}
	else
	{
		// New command.
		automate_commands[type] = Z_StrDup(command);
	}
}

void Automate_Run(automateEvents_t type)
{
	if (!server)
	{
		// Only the server should be doing this.
		return;
	}

	if (K_CanChangeRules() == false)
	{
		// Don't engage in automation while in a restricted context.
		return;
	}

#ifdef PARANOIA
	if (type >= AEV__MAX)
	{
		// Shouldn't happen.
		I_Error("Attempted to run invalid automation type.");
		return;
	}
#endif

	if (!cv_automate.value)
	{
		// We don't want to run automation.
		return;
	}

	if (automate_commands[type] == NULL)
	{
		// No command to run.
		return;
	}

	CONS_Printf(
		"Running %s automate command \"" "\x82" "%s" "\x80" "\"...\n",
		automate_names[type],
		automate_commands[type]
	);

	COM_BufAddText(automate_commands[type]);
	COM_BufAddText("\n");
}

void Automate_Clear(void)
{
	size_t i;

	for (i = 0; i < AEV__MAX; i++)
	{
		Automate_Set(i, NULL);
	}
}

static void Command_MotD_f(void)
{
	size_t i, j;
	char *mymotd;

	if ((j = COM_Argc()) < 2)
	{
		CONS_Printf(M_GetText("motd <message>: Set a message that clients see upon join.\n"));
		return;
	}

	if (!(server || (IsPlayerAdmin(consoleplayer))))
	{
		CONS_Printf(M_GetText("Only the server or a remote admin can use this.\n"));
		return;
	}

	mymotd = Z_Malloc(sizeof(motd), PU_STATIC, NULL);

	strlcpy(mymotd, COM_Argv(1), sizeof motd);
	for (i = 2; i < j; i++)
	{
		strlcat(mymotd, " ", sizeof motd);
		strlcat(mymotd, COM_Argv(i), sizeof motd);
	}

	// Disallow non-printing characters and semicolons.
	for (i = 0; mymotd[i] != '\0'; i++)
		if (!isprint(mymotd[i]) || mymotd[i] == ';')
		{
			Z_Free(mymotd);
			return;
		}

	if ((netgame || multiplayer) && client)
		SendNetXCmd(XD_SETMOTD, mymotd, i); // send the actual size of the motd string, not the full buffer's size
	else
	{
		strcpy(motd, mymotd);
		CONS_Printf(M_GetText("Message of the day set.\n"));
	}

	Z_Free(mymotd);
}

static void Got_MotD_f(UINT8 **cp, INT32 playernum)
{
	char *mymotd = Z_Malloc(sizeof(motd), PU_STATIC, NULL);
	INT32 i;
	boolean kick = false;

	READSTRINGN(*cp, mymotd, sizeof(motd));

	// Disallow non-printing characters and semicolons.
	for (i = 0; mymotd[i] != '\0'; i++)
		if (!isprint(mymotd[i]) || mymotd[i] == ';')
			kick = true;

	if ((playernum != serverplayer && !IsPlayerAdmin(playernum)) || kick)
	{
		CONS_Alert(CONS_WARNING, M_GetText("Illegal motd change received from %s\n"), player_names[playernum]);
		if (server)
			SendKick(playernum, KICK_MSG_CON_FAIL);
		Z_Free(mymotd);
		return;
	}

	strcpy(motd, mymotd);

	CONS_Printf(M_GetText("Message of the day set.\n"));

	Z_Free(mymotd);
}

static void Command_RunSOC(void)
{
	const char *fn;
	char buf[255];
	size_t length = 0;

	if (COM_Argc() != 2)
	{
		CONS_Printf(M_GetText("runsoc <socfile.soc> or <lumpname>: run a soc\n"));
		return;
	}
	else
		fn = COM_Argv(1);

	if (netgame && !(server || IsPlayerAdmin(consoleplayer)))
	{
		CONS_Printf(M_GetText("Only the server or a remote admin can use this.\n"));
		return;
	}

	if (!(netgame || multiplayer))
	{
		if (!P_RunSOC(fn))
			CONS_Printf(M_GetText("Could not find SOC.\n"));
		else
			G_SetGameModified(multiplayer, false);
		return;
	}

	nameonly(strcpy(buf, fn));
	length = strlen(buf)+1;

	SendNetXCmd(XD_RUNSOC, buf, length);
}

static void Got_RunSOCcmd(UINT8 **cp, INT32 playernum)
{
	char filename[256];
	filestatus_t ncs = FS_NOTCHECKED;

	if (playernum != serverplayer && !IsPlayerAdmin(playernum))
	{
		CONS_Alert(CONS_WARNING, M_GetText("Illegal runsoc command received from %s\n"), player_names[playernum]);
		if (server)
			SendKick(playernum, KICK_MSG_CON_FAIL);
		return;
	}

	READSTRINGN(*cp, filename, 255);

	// Maybe add md5 support?
	if (strstr(filename, ".soc") != NULL)
	{
		ncs = findfile(filename,NULL,true);

		if (ncs != FS_FOUND)
		{
			Command_ExitGame_f();
			if (ncs == FS_NOTFOUND)
			{
				CONS_Printf(M_GetText("The server tried to add %s,\nbut you don't have this file.\nYou need to find it in order\nto play on this server.\n"), filename);
				M_StartMessage(va("The server added a file\n(%s)\nthat you do not have.\n\nPress ESC\n",filename), NULL, MM_NOTHING);
			}
			else
			{
				CONS_Printf(M_GetText("Unknown error finding soc file (%s) the server added.\n"), filename);
				M_StartMessage(va("Unknown error trying to load a file\nthat the server added\n(%s).\n\nPress ESC\n",filename), NULL, MM_NOTHING);
			}
			return;
		}
	}

	P_RunSOC(filename);
	G_SetGameModified(true, false);
}

/** Adds a pwad at runtime.
  * Searches for sounds, maps, music, new images.
  */
static void Command_Addfile(void)
{
#ifndef TESTERS
	size_t argc = COM_Argc(); // amount of arguments total
	size_t curarg; // current argument index

	const char *addedfiles[argc]; // list of filenames already processed
	size_t numfilesadded = 0; // the amount of filenames processed

	if (argc < 2)
	{
		CONS_Printf(M_GetText("addfile <filename.pk3/wad/lua/soc> [filename2...] [...]: Load add-ons\n"));
		return;
	}

	// start at one to skip command name
	for (curarg = 1; curarg < argc; curarg++)
	{
		const char *fn, *p;
		char buf[256];
		char *buf_p = buf;
		INT32 i;
		size_t ii;
		int musiconly; // W_VerifyNMUSlumps isn't boolean
		boolean fileadded = false;

		fn = COM_Argv(curarg);

		// For the amount of filenames previously processed...
		for (ii = 0; ii < numfilesadded; ii++)
		{
			// If this is one of them, don't try to add it.
			if (!strcmp(fn, addedfiles[ii]))
			{
				fileadded = true;
				break;
			}
		}

		// If we've added this one, skip to the next one.
		if (fileadded)
		{
			CONS_Alert(CONS_WARNING, M_GetText("Already processed %s, skipping\n"), fn);
			continue;
		}

		// Disallow non-printing characters and semicolons.
		for (i = 0; fn[i] != '\0'; i++)
			if (!isprint(fn[i]) || fn[i] == ';')
				return;

		musiconly = W_VerifyNMUSlumps(fn, false);

		if (musiconly == -1)
		{
			addedfiles[numfilesadded++] = fn;
			continue;
		}

		if (!musiconly)
		{
			// ... But only so long as they contain nothing more then music and sprites.
			if (netgame && !(server || IsPlayerAdmin(consoleplayer)))
			{
				CONS_Printf(M_GetText("Only the server or a remote admin can use this.\n"));
				continue;
			}
			G_SetGameModified(multiplayer, false);
		}

		// Add file on your client directly if it is trivial, or you aren't in a netgame.
		if (!(netgame || multiplayer) || musiconly)
		{
			P_AddWadFile(fn);
			addedfiles[numfilesadded++] = fn;
			continue;
		}

		p = fn+strlen(fn);
		while(--p >= fn)
			if (*p == '\\' || *p == '/' || *p == ':')
				break;
		++p;

		// check total packet size and no of files currently loaded
		// See W_LoadWadFile in w_wad.c
		if (numwadfiles >= MAX_WADFILES)
		{
			CONS_Alert(CONS_ERROR, M_GetText("Too many files loaded to add %s\n"), fn);
			return;
		}

		WRITESTRINGN(buf_p,p,240);

		// calculate and check md5
		{
			UINT8 md5sum[16];
#ifdef NOMD5
			memset(md5sum,0,16);
#else
			FILE *fhandle;
			boolean valid = true;

			if ((fhandle = W_OpenWadFile(&fn, true)) != NULL)
			{
				tic_t t = I_GetTime();
				CONS_Debug(DBG_SETUP, "Making MD5 for %s\n",fn);
				md5_stream(fhandle, md5sum);
				CONS_Debug(DBG_SETUP, "MD5 calc for %s took %f second\n", fn, (float)(I_GetTime() - t)/TICRATE);
				fclose(fhandle);
			}
			else // file not found
				continue;

			for (i = 0; i < numwadfiles; i++)
			{
				if (!memcmp(wadfiles[i]->md5sum, md5sum, 16))
				{
					CONS_Alert(CONS_ERROR, M_GetText("%s is already loaded\n"), fn);
					valid = false;
					break;
				}
			}

			if (valid == false)
			{
				continue;
			}
#endif
			WRITEMEM(buf_p, md5sum, 16);
		}

		addedfiles[numfilesadded++] = fn;

		if (IsPlayerAdmin(consoleplayer) && (!server)) // Request to add file
			SendNetXCmd(XD_REQADDFILE, buf, buf_p - buf);
		else
			SendNetXCmd(XD_ADDFILE, buf, buf_p - buf);
	}
#endif/*TESTERS*/
}

static void Got_RequestAddfilecmd(UINT8 **cp, INT32 playernum)
{
	char filename[241];
	filestatus_t ncs = FS_NOTCHECKED;
	UINT8 md5sum[16];
	boolean kick = false;
	boolean toomany = false;
	INT32 i,j;

	READSTRINGN(*cp, filename, 240);
	READMEM(*cp, md5sum, 16);

	// Only the server processes this message.
	if (client)
		return;

	// Disallow non-printing characters and semicolons.
	for (i = 0; filename[i] != '\0'; i++)
		if (!isprint(filename[i]) || filename[i] == ';')
			kick = true;

	if ((playernum != serverplayer && !IsPlayerAdmin(playernum)) || kick)
	{
		CONS_Alert(CONS_WARNING, M_GetText("Illegal addfile command received from %s\n"), player_names[playernum]);
		SendKick(playernum, KICK_MSG_CON_FAIL);
		return;
	}

	// See W_LoadWadFile in w_wad.c
	if (numwadfiles >= MAX_WADFILES)
		toomany = true;
	else
		ncs = findfile(filename,md5sum,true);

	if (ncs != FS_FOUND || toomany)
	{
		char message[275];

		if (toomany)
			sprintf(message, M_GetText("Too many files loaded to add %s\n"), filename);
		else if (ncs == FS_NOTFOUND)
			sprintf(message, M_GetText("The server doesn't have %s\n"), filename);
		else if (ncs == FS_MD5SUMBAD)
			sprintf(message, M_GetText("Checksum mismatch on %s\n"), filename);
		else
			sprintf(message, M_GetText("Unknown error finding wad file (%s)\n"), filename);

		CONS_Printf("%s",message);

		for (j = 0; j < MAXPLAYERS; j++)
			if (adminplayers[j])
				COM_BufAddText(va("sayto %d %s", adminplayers[j], message));

		return;
	}

	COM_BufAddText(va("addfile %s\n", filename));
}

static void Got_Addfilecmd(UINT8 **cp, INT32 playernum)
{
	char filename[241];
	filestatus_t ncs = FS_NOTCHECKED;
	UINT8 md5sum[16];

	READSTRINGN(*cp, filename, 240);
	READMEM(*cp, md5sum, 16);

	if (playernum != serverplayer)
	{
		CONS_Alert(CONS_WARNING, M_GetText("Illegal addfile command received from %s\n"), player_names[playernum]);
		if (server)
			SendKick(playernum, KICK_MSG_CON_FAIL);
		return;
	}

	ncs = findfile(filename,md5sum,true);

	if (ncs != FS_FOUND || !P_AddWadFile(filename))
	{
		Command_ExitGame_f();
		if (ncs == FS_FOUND)
		{
			CONS_Printf(M_GetText("The server tried to add %s,\nbut you have too many files added.\nRestart the game to clear loaded files\nand play on this server."), filename);
			M_StartMessage(va("The server added a file \n(%s)\nbut you have too many files added.\nRestart the game to clear loaded files.\n\nPress ESC\n",filename), NULL, MM_NOTHING);
		}
		else if (ncs == FS_NOTFOUND)
		{
			CONS_Printf(M_GetText("The server tried to add %s,\nbut you don't have this file.\nYou need to find it in order\nto play on this server."), filename);
			M_StartMessage(va("The server added a file \n(%s)\nthat you do not have.\n\nPress ESC\n",filename), NULL, MM_NOTHING);
		}
		else if (ncs == FS_MD5SUMBAD)
		{
			CONS_Printf(M_GetText("Checksum mismatch while loading %s.\nMake sure you have the copy of\nthis file that the server has.\n"), filename);
			M_StartMessage(va("Checksum mismatch while loading \n%s.\nThe server seems to have a\ndifferent version of this file.\n\nPress ESC\n",filename), NULL, MM_NOTHING);
		}
		else
		{
			CONS_Printf(M_GetText("Unknown error finding wad file (%s) the server added.\n"), filename);
			M_StartMessage(va("Unknown error trying to load a file\nthat the server added \n(%s).\n\nPress ESC\n",filename), NULL, MM_NOTHING);
		}
		return;
	}

	G_SetGameModified(true, false);
}

static void Command_ListWADS_f(void)
{
	INT32 i = numwadfiles;
	char *tempname;
	CONS_Printf(M_GetText("There are %d wads loaded:\n"),numwadfiles);
	for (i--; i >= 0; i--)
	{
		nameonly(tempname = va("%s", wadfiles[i]->filename));
		if (!i)
			CONS_Printf("\x82 IWAD\x80: %s\n", tempname);
		else if (i <= mainwads)
			CONS_Printf("\x82 * %.2d\x80: %s\n", i, tempname);
		else if (!wadfiles[i]->important)
			CONS_Printf("\x86   %.2d: %s\n", i, tempname);
		else
			CONS_Printf("   %.2d: %s\n", i, tempname);
	}
}

#define MAXDOOMEDNUM 4095

static void Command_ListDoomednums_f(void)
{
	INT16 i, j, focusstart = 0, focusend = 0;
	INT32 argc = COM_Argc(), argstart = 0;
	INT16 table[MAXDOOMEDNUM];
	boolean nodoubles = false;
	UINT8 doubles[(MAXDOOMEDNUM+8/8)];

	if (argc > 1)
	{
		nodoubles = (strcmp(COM_Argv(1), "-nodoubles") == 0);
		if (nodoubles)
		{
			argc--;
			argstart++;
		}
	}

	switch (argc)
	{
		case 1:
			focusend = MAXDOOMEDNUM;
			break;
		case 3:
			focusend = atoi(COM_Argv(argstart+2));
			if (focusend < 1 || focusend > MAXDOOMEDNUM)
			{
				CONS_Printf("arg 2: doomednum \x82""%d \x85out of range (1-4095)\n", focusend);
				return;
			}
			//FALLTHRU
		case 2:
			focusstart = atoi(COM_Argv(argstart+1));
			if (focusstart < 1 || focusstart > MAXDOOMEDNUM)
			{
				CONS_Printf("arg 1: doomednum \x82""%d \x85out of range (1-4095)\n", focusstart);
				return;
			}
			if (!focusend)
				focusend = focusstart;
			else if (focusend < focusstart) // silently and helpfully swap.
			{
				j = focusstart;
				focusstart = focusend;
				focusend = j;
			}
			break;
		default:
			CONS_Printf("listmapthings: \x86too many arguments!\n");
			return;
	}

	// see P_SpawnNonMobjMapThing
	memset(table, 0, sizeof(table));
	memset(doubles, 0, sizeof(doubles));
	for (i = 1; i <= MAXPLAYERS; i++)
		table[i-1] = MT_PLAYER; // playerstarts
	table[33-1] = table[34-1] = table[35-1] = MT_PLAYER; // battle/team starts
	table[750-1] = table[777-1] = table[778-1] = MT_UNKNOWN; // slopes
	for (i = 600; i <= 609; i++)
		table[i-1] = MT_RING; // placement patterns
	table[1705-1] = table[1713-1] = MT_HOOP; // types of hoop

	CONS_Printf("\x82""Checking for double defines...\n");
	for (i = 1; i < MT_FIRSTFREESLOT+NUMMOBJFREESLOTS; i++)
	{
		j = mobjinfo[i].doomednum;
		if (j < (focusstart ? focusstart : 1) || j > focusend)
			continue;
		if (table[--j])
		{
			doubles[j/8] |= 1<<(j&7);
			CONS_Printf("	doomednum \x82""%d""\x80 is \x85""double-defined\x80 by ", j+1);
			if (i < MT_FIRSTFREESLOT)
			{
				CONS_Printf("\x87""hardcode %s <-- MAJOR ERROR\n", MOBJTYPE_LIST[i]);
				continue;
			}
			CONS_Printf("\x81""freeslot MT_""%s\n", FREE_MOBJS[i-MT_FIRSTFREESLOT]);
			continue;
		}
		table[j] = i;
	}
	CONS_Printf("\x82Printing doomednum usage...\n");
	if (!focusstart)
	{
		i = 35; // skip MT_PLAYER spam
		if (!nodoubles)
			CONS_Printf("	doomednums \x82""1-35""\x80 are used by ""\x87""hardcode MT_PLAYER\n");
	}
	else
		i = focusstart-1;

	for (; i < focusend; i++)
	{
		if (nodoubles && !(doubles[i/8] & 1<<(i&7)))
			continue;
		if (!table[i])
		{
			if (focusstart)
			{
				CONS_Printf("	doomednum \x82""%d""\x80 is \x83""free!", i+1);
				if (i < 99) // above the humble crawla? how dare you
					CONS_Printf(" (Don't freeslot this low...)");
				CONS_Printf("\n");
			}
			continue;
		}
		CONS_Printf("	doomednum \x82""%d""\x80 is used by ", i+1);
		if (table[i] < MT_FIRSTFREESLOT)
		{
			CONS_Printf("\x87""hardcode %s\n", MOBJTYPE_LIST[table[i]]);
			continue;
		}
		CONS_Printf("\x81""freeslot MT_""%s\n", FREE_MOBJS[table[i]-MT_FIRSTFREESLOT]);
	}
}

#undef MAXDOOMEDNUM

// =========================================================================
//                            MISC. COMMANDS
// =========================================================================

/** Prints program version.
  */
static void Command_Version_f(void)
{
#ifdef DEVELOP
	CONS_Printf("SRB2Kart %s-%s (%s %s)\n", compbranch, comprevision, compdate, comptime);
#else
	CONS_Printf("SRB2Kart %s (%s %s %s %s) ", VERSIONSTRING, compdate, comptime, comprevision, compbranch);
#endif

	// Base library
#if defined( HAVE_SDL)
	CONS_Printf("SDL ");
#elif defined(_WINDOWS)
	CONS_Printf("DD ");
#endif

	// OS
	// Would be nice to use SDL_GetPlatform for this
#if defined (_WIN32) || defined (_WIN64)
	CONS_Printf("Windows ");
#elif defined(__linux__)
	CONS_Printf("Linux ");
#elif defined(MACOSX)
	CONS_Printf("macOS ");
#elif defined(UNIXCOMMON)
	CONS_Printf("Unix (Common) ");
#else
	CONS_Printf("Other OS ");
#endif

	// Bitness
	if (sizeof(void*) == 4)
		CONS_Printf("32-bit ");
	else if (sizeof(void*) == 8)
		CONS_Printf("64-bit ");
	else // 16-bit? 128-bit?
		CONS_Printf("Bits Unknown ");

	// No ASM?
#ifdef NOASM
	CONS_Printf("\x85" "NOASM " "\x80");
#endif

	// Debug build
#ifdef _DEBUG
	CONS_Printf("\x85" "DEBUG " "\x80");
#endif

	// DEVELOP build
#if defined(TESTERS)
	CONS_Printf("\x88" "TESTERS " "\x80");
#elif defined(HOSTTESTERS)
	CONS_Printf("\x82" "HOSTTESTERS " "\x80");
#elif defined(DEVELOP)
	CONS_Printf("\x87" "DEVELOP " "\x80");
#endif

	if (compuncommitted)
		CONS_Printf("\x85" "! UNCOMMITTED CHANGES ! " "\x80");

	CONS_Printf("\n");
}

#ifdef UPDATE_ALERT
static void Command_ModDetails_f(void)
{
	CONS_Printf(M_GetText("Mod ID: %d\nMod Version: %d\nCode Base:%d\n"), MODID, MODVERSION, CODEBASE);
}
#endif

// Returns current gametype being used.
//
static void Command_ShowGametype_f(void)
{
	const char *gametypestr = NULL;

	if (!(netgame || multiplayer)) // print "Single player" instead of "Race"
	{
		CONS_Printf(M_GetText("Current gametype is %s\n"), "Single Player");
		return;
	}

	// get name string for current gametype
	if (gametype >= 0 && gametype < gametypecount)
		gametypestr = Gametype_Names[gametype];

	if (gametypestr)
		CONS_Printf(M_GetText("Current gametype is %s\n"), gametypestr);
	else // string for current gametype was not found above (should never happen)
		CONS_Printf(M_GetText("Unknown gametype set (%d)\n"), gametype);
}

/** Plays the intro.
  */
static void Command_Playintro_f(void)
{
	if (netgame)
		return;

	if (dirmenu)
		closefilemenu(true);

	F_StartIntro();
}

/** Quits the game immediately.
  */
FUNCNORETURN static ATTRNORETURN void Command_Quit_f(void)
{
	LUA_HookBool(true, HOOK(GameQuit));
	I_Quit();
}

void ItemFinder_OnChange(void)
{
	if (!cv_itemfinder.value)
		return; // it's fine.

	if (!M_SecretUnlocked(SECRET_ITEMFINDER))
	{
		CONS_Printf(M_GetText("You haven't earned this yet.\n"));
		CV_StealthSetValue(&cv_itemfinder, 0);
		return;
	}
	else if (netgame || multiplayer)
	{
		CONS_Printf(M_GetText("This only works in single player.\n"));
		CV_StealthSetValue(&cv_itemfinder, 0);
		return;
	}
}

/** Deals with a pointlimit change by printing the change to the console.
  * If the gametype is single player, cooperative, or race, the pointlimit is
  * silently disabled again.
  *
  * Timelimit and pointlimit can be used at the same time.
  *
  * We don't check immediately for the pointlimit having been reached,
  * because you would get "caught" when turning it up in the menu.
  * \sa cv_pointlimit, TimeLimit_OnChange
  * \author Graue <graue@oceanbase.org>
  */
static void PointLimit_OnChange(void)
{
	// Don't allow pointlimit in Single Player/Co-Op/Race!
	if (server && Playing() && !(gametyperules & GTR_POINTLIMIT))
	{
		if (cv_pointlimit.value)
			CV_StealthSetValue(&cv_pointlimit, 0);
		return;
	}

	if (cv_pointlimit.value)
	{
		CONS_Printf(M_GetText("Levels will end after %s scores %d point%s.\n"),
			G_GametypeHasTeams() ? M_GetText("a team") : M_GetText("someone"),
			cv_pointlimit.value,
			cv_pointlimit.value > 1 ? "s" : "");
	}
	else if (netgame || multiplayer)
		CONS_Printf(M_GetText("Point limit disabled\n"));
}

static void NetTimeout_OnChange(void)
{
	connectiontimeout = (tic_t)cv_nettimeout.value;
}

static void JoinTimeout_OnChange(void)
{
	jointimeout = (tic_t)cv_jointimeout.value;
}

static void
Lagless_OnChange (void)
{
	/* don't back out of dishonesty, or go lagless after playing honestly */
	if (cv_lagless.value && gamestate == GS_LEVEL)
		server_lagless = true;
}

UINT32 timelimitintics = 0;
UINT32 extratimeintics = 0;
UINT32 secretextratime = 0;

/** Deals with a timelimit change by printing the change to the console.
  * If the gametype is single player, cooperative, or race, the timelimit is
  * silently disabled again.
  *
  * Timelimit and pointlimit can be used at the same time.
  *
  * \sa cv_timelimit, PointLimit_OnChange
  */
static void TimeLimit_OnChange(void)
{
	// Don't allow timelimit in Single Player/Co-Op/Race!
	if (server && Playing() && cv_timelimit.value != 0 && (bossinfo.boss || !(gametyperules & GTR_TIMELIMIT)))
	{
		CV_SetValue(&cv_timelimit, 0);
		return;
	}

	if (cv_timelimit.value != 0)
	{
		CONS_Printf(M_GetText("Rounds will end after %d minute%s.\n"),cv_timelimit.value,cv_timelimit.value == 1 ? "" : "s"); // Graue 11-17-2003
		timelimitintics = cv_timelimit.value * (60*TICRATE);

		// Note the deliberate absence of any code preventing
		//   pointlimit and timelimit from being set simultaneously.
		// Some people might like to use them together. It works.
	}

#ifdef HAVE_DISCORDRPC
	DRPC_UpdatePresence();
#endif
}

/** Adjusts certain settings to match a changed gametype.
  *
  * \param lastgametype The gametype we were playing before now.
  * \sa D_MapChange
  * \author Graue <graue@oceanbase.org>
  * \todo Get rid of the hardcoded stuff, ugly stuff, etc.
  */
void D_GameTypeChanged(INT32 lastgametype)
{
	if (netgame)
	{
		const char *oldgt = NULL, *newgt = NULL;

		if (lastgametype >= 0 && lastgametype < gametypecount)
			oldgt = Gametype_Names[lastgametype];
		if (gametype >= 0 && lastgametype < gametypecount)
			newgt = Gametype_Names[gametype];

		if (oldgt && newgt)
			CONS_Printf(M_GetText("Gametype was changed from %s to %s\n"), oldgt, newgt);
	}
	// Only do the following as the server, not as remote admin.
	// There will always be a server, and this only needs to be done once.
	if (server && (multiplayer || netgame))
	{
		if (!cv_timelimit.changed) // user hasn't changed limits
		{
			CV_SetValue(&cv_timelimit, timelimits[gametype]);
		}
		if (!cv_pointlimit.changed)
		{
			CV_SetValue(&cv_pointlimit, pointlimits[gametype]);
		}
	}
	/* -- no longer useful
	else if (!multiplayer && !netgame)
	{
		G_SetGametype(GT_RACE);
	}
	*/

	// reset timelimit and pointlimit in race/coop, prevent stupid cheats
	if (server)
	{
		if (!(gametyperules & GTR_TIMELIMIT))
		{
			if (cv_timelimit.value)
				CV_SetValue(&cv_timelimit, 0);
		}
		if (!(gametyperules & GTR_POINTLIMIT))
		{
			if (cv_pointlimit.value)
				CV_SetValue(&cv_pointlimit, 0);
		}
	}

	// don't retain teams in other modes or between changes from ctf to team match.
	// also, stop any and all forms of team scrambling that might otherwise take place.
	if (G_GametypeHasTeams())
	{
		INT32 i;
		for (i = 0; i < MAXPLAYERS; i++)
			if (playeringame[i])
				players[i].ctfteam = 0;

		if (server || (IsPlayerAdmin(consoleplayer)))
		{
			CV_StealthSetValue(&cv_teamscramble, 0);
			teamscramble = 0;
		}
	}
}

static void Gravity_OnChange(void)
{
	if (!M_SecretUnlocked(SECRET_PANDORA) && !netgame && !cv_debug
		&& strcmp(cv_gravity.string, cv_gravity.defaultvalue))
	{
		CONS_Printf(M_GetText("You haven't earned this yet.\n"));
		CV_StealthSet(&cv_gravity, cv_gravity.defaultvalue);
		return;
	}
#ifndef NETGAME_GRAVITY
	if(netgame)
	{
		CV_StealthSet(&cv_gravity, cv_gravity.defaultvalue);
		return;
	}
#endif

	if (!CV_IsSetToDefault(&cv_gravity))
		G_SetGameModified(multiplayer, true);
	gravity = cv_gravity.value;
}

static void SoundTest_OnChange(void)
{
	INT32 sfxfreeint = (INT32)sfxfree;
	if (cv_soundtest.value < 0)
	{
		CV_SetValue(&cv_soundtest, sfxfreeint-1);
		return;
	}

	if (cv_soundtest.value >= sfxfreeint)
	{
		CV_SetValue(&cv_soundtest, 0);
		return;
	}

	S_StopSounds();
	S_StartSound(NULL, cv_soundtest.value);
}

static void AutoBalance_OnChange(void)
{
	autobalance = (INT16)cv_autobalance.value;
}

static void TeamScramble_OnChange(void)
{
	INT16 i = 0, j = 0, playercount = 0;
	boolean repick = true;
	INT32 blue = 0, red = 0;
	INT32 maxcomposition = 0;
	INT16 newteam = 0;
	INT32 retries = 0;
	boolean success = false;

	// Don't trigger outside level or intermission!
	if (!(gamestate == GS_LEVEL || gamestate == GS_INTERMISSION || gamestate == GS_VOTING))
		return;

	if (!cv_teamscramble.value)
		teamscramble = 0;

	if (!G_GametypeHasTeams() && (server || IsPlayerAdmin(consoleplayer)))
	{
		CONS_Alert(CONS_NOTICE, M_GetText("This command cannot be used in this gametype.\n"));
		CV_StealthSetValue(&cv_teamscramble, 0);
		return;
	}

	// If a team scramble is already in progress, do not allow another one to be started!
	if (teamscramble)
		return;

retryscramble:

	// Clear related global variables. These will get used again in p_tick.c/y_inter.c as the teams are scrambled.
	memset(&scrambleplayers, 0, sizeof(scrambleplayers));
	memset(&scrambleteams, 0, sizeof(scrambleplayers));
	scrambletotal = scramblecount = 0;
	blue = red = maxcomposition = newteam = playercount = 0;
	repick = true;

	// Put each player's node in the array.
	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (playeringame[i] && !players[i].spectator)
		{
			scrambleplayers[playercount] = i;
			playercount++;
		}
	}

	if (playercount < 2)
	{
		CV_StealthSetValue(&cv_teamscramble, 0);
		return; // Don't scramble one or zero players.
	}

	// Randomly place players on teams.
	if (cv_teamscramble.value == 1)
	{
		maxcomposition = playercount / 2;

		// Now randomly assign players to teams.
		// If the teams get out of hand, assign the rest to the other team.
		for (i = 0; i < playercount; i++)
		{
			if (repick)
				newteam = (INT16)((M_RandomByte() % 2) + 1);

			// One team has the most players they can get, assign the rest to the other team.
			if (red == maxcomposition || blue == maxcomposition)
			{
				if (red == maxcomposition)
					newteam = 2;
				else //if (blue == maxcomposition)
					newteam = 1;

				repick = false;
			}

			scrambleteams[i] = newteam;

			if (newteam == 1)
				red++;
			else
				blue++;
		}
	}
	else if (cv_teamscramble.value == 2) // Same as before, except split teams based on current score.
	{
		// Now, sort the array based on points scored.
		for (i = 1; i < playercount; i++)
		{
			for (j = i; j < playercount; j++)
			{
				INT16 tempplayer = 0;

				if ((players[scrambleplayers[i-1]].score > players[scrambleplayers[j]].score))
				{
					tempplayer = scrambleplayers[i-1];
					scrambleplayers[i-1] = scrambleplayers[j];
					scrambleplayers[j] = tempplayer;
				}
			}
		}

		// Now assign players to teams based on score. Scramble in pairs.
		// If there is an odd number, one team will end up with the unlucky slob who has no points. =(
		for (i = 0; i < playercount; i++)
		{
			if (repick)
			{
				newteam = (INT16)((M_RandomByte() % 2) + 1);
				repick = false;
			}
			// (i != 2) means it does ABBABABA, instead of ABABABAB.
			// Team A gets 1st, 4th, 6th, 8th.
			// Team B gets 2nd, 3rd, 5th, 7th.
			// So 1st on one team, 2nd/3rd on the other, then alternates afterwards.
			// Sounds strange on paper, but works really well in practice!
			else if (i != 2)
			{
				// We will only randomly pick the team for the first guy.
				// Otherwise, just alternate back and forth, distributing players.
				newteam = 3 - newteam;
			}

			scrambleteams[i] = newteam;
		}
	}

	// Check to see if our random selection actually
	// changed anybody. If not, we run through and try again.
	for (i = 0; i < playercount; i++)
	{
		if (players[scrambleplayers[i]].ctfteam != scrambleteams[i])
			success = true;
	}

	if (!success && retries < 5)
	{
		retries++;
		goto retryscramble; //try again
	}

	// Display a witty message, but only during scrambles specifically triggered by an admin.
	if (cv_teamscramble.value)
	{
		scrambletotal = playercount;
		teamscramble = (INT16)cv_teamscramble.value;

		if (!(gamestate == GS_INTERMISSION && cv_scrambleonchange.value))
			CONS_Printf(M_GetText("Teams will be scrambled next round.\n"));
	}
}

static void Command_Showmap_f(void)
{
	if (gamestate == GS_LEVEL)
	{
		if (mapheaderinfo[gamemap-1]->zonttl[0] && !(mapheaderinfo[gamemap-1]->levelflags & LF_NOZONE))
		{
			if (mapheaderinfo[gamemap-1]->actnum[0])
				CONS_Printf("%s (%d): %s %s %s\n", G_BuildMapName(gamemap), gamemap, mapheaderinfo[gamemap-1]->lvlttl, mapheaderinfo[gamemap-1]->zonttl, mapheaderinfo[gamemap-1]->actnum);
			else
				CONS_Printf("%s (%d): %s %s\n", G_BuildMapName(gamemap), gamemap, mapheaderinfo[gamemap-1]->lvlttl, mapheaderinfo[gamemap-1]->zonttl);
		}
		else
		{
			if (mapheaderinfo[gamemap-1]->actnum[0])
				CONS_Printf("%s (%d): %s %s\n", G_BuildMapName(gamemap), gamemap, mapheaderinfo[gamemap-1]->lvlttl, mapheaderinfo[gamemap-1]->actnum);
			else
				CONS_Printf("%s (%d): %s\n", G_BuildMapName(gamemap), gamemap, mapheaderinfo[gamemap-1]->lvlttl);
		}
	}
	else
		CONS_Printf(M_GetText("You must be in a level to use this.\n"));
}

static void Command_Mapmd5_f(void)
{
	if (gamestate == GS_LEVEL)
	{
		INT32 i;
		char md5tmp[33];
		for (i = 0; i < 16; ++i)
			sprintf(&md5tmp[i*2], "%02x", mapmd5[i]);
		CONS_Printf("%s: %s\n", G_BuildMapName(gamemap), md5tmp);
	}
	else
		CONS_Printf(M_GetText("You must be in a level to use this.\n"));
}

static void Command_ExitLevel_f(void)
{
	if (!(netgame || multiplayer) && !cv_debug)
		CONS_Printf(M_GetText("This only works in a netgame.\n"));
	else if (!(server || (IsPlayerAdmin(consoleplayer))))
		CONS_Printf(M_GetText("Only the server or a remote admin can use this.\n"));
	else if (( gamestate != GS_LEVEL && gamestate != GS_CREDITS ) || demo.playback)
		CONS_Printf(M_GetText("You must be in a level to use this.\n"));
	else
		SendNetXCmd(XD_EXITLEVEL, NULL, 0);
}

static void Got_ExitLevelcmd(UINT8 **cp, INT32 playernum)
{
	(void)cp;

	// Ignore duplicate XD_EXITLEVEL commands.
	if (gameaction == ga_completed)
		return;

	if (playernum != serverplayer && !IsPlayerAdmin(playernum))
	{
		CONS_Alert(CONS_WARNING, M_GetText("Illegal exitlevel command received from %s\n"), player_names[playernum]);
		if (server)
			SendKick(playernum, KICK_MSG_CON_FAIL);
		return;
	}

	G_ExitLevel();
}

static void Got_SetupVotecmd(UINT8 **cp, INT32 playernum)
{
	INT32 i;
	UINT8 gt, secondgt;

	if (playernum != serverplayer && !IsPlayerAdmin(playernum))
	{
		CONS_Alert(CONS_WARNING, M_GetText("Illegal vote setup received from %s\n"), player_names[playernum]);
		if (server)
			SendKick(playernum, KICK_MSG_CON_FAIL);
		return;
	}

	gt = (UINT8)READUINT8(*cp);
	secondgt = (UINT8)READUINT8(*cp);

	for (i = 0; i < 4; i++)
	{
		votelevels[i][0] = (UINT16)READUINT16(*cp);
		votelevels[i][1] = gt;
		if (!mapheaderinfo[votelevels[i][0]])
			P_AllocMapHeader(votelevels[i][0]);
	}

	votelevels[2][1] = secondgt;

	G_SetGamestate(GS_VOTING);
	Y_StartVote();
}

static void Got_ModifyVotecmd(UINT8 **cp, INT32 playernum)
{
	SINT8 voted = READSINT8(*cp);
	UINT8 p = READUINT8(*cp);

	(void)playernum;
	votes[p] = voted;
}

static void Got_PickVotecmd(UINT8 **cp, INT32 playernum)
{
	SINT8 pick = READSINT8(*cp);
	SINT8 level = READSINT8(*cp);

	if (playernum != serverplayer && !IsPlayerAdmin(playernum))
	{
		CONS_Alert(CONS_WARNING, M_GetText("Illegal vote setup received from %s\n"), player_names[playernum]);
		if (server)
			SendKick(playernum, KICK_MSG_CON_FAIL);
		return;
	}

	Y_SetupVoteFinish(pick, level);
}

static void Got_GiveItemcmd(UINT8 **cp, INT32 playernum)
{
	int item;
	int  amt;

	item = READSINT8 (*cp);
	amt  = READUINT8 (*cp);

	if (
			( netgame && ! cv_kartallowgiveitem.value ) ||
			( item < KITEM_SAD || item >= NUMKARTITEMS )
	)
	{
		CONS_Alert(CONS_WARNING,
				M_GetText ("Illegal give item received from %s\n"),
				player_names[playernum]);
		if (server)
			SendKick(playernum, KICK_MSG_CON_FAIL);
		return;
	}

	players[playernum].itemtype   = item;
	players[playernum].itemamount = amt;
}

static void Got_ScheduleTaskcmd(UINT8 **cp, INT32 playernum)
{
	char command[MAXTEXTCMD];
	INT16 seconds;

	seconds = READINT16(*cp);
	READSTRING(*cp, command);

	if (playernum != serverplayer && !IsPlayerAdmin(playernum))
	{
		CONS_Alert(CONS_WARNING,
				M_GetText ("Illegal schedule task received from %s\n"),
				player_names[playernum]);
		if (server)
			SendKick(playernum, KICK_MSG_CON_FAIL);
		return;
	}

	Schedule_Add(seconds, seconds, (const char *)command);

	if (server || consoleplayer == playernum)
	{
		CONS_Printf(
			"OK! Running \"" "\x82" "%s" "\x80" "\" every " "\x82" "%d" "\x80" " seconds.\n",
			command,
			seconds
		);
	}
}

static void Got_ScheduleClearcmd(UINT8 **cp, INT32 playernum)
{
	(void)cp;

	if (playernum != serverplayer && !IsPlayerAdmin(playernum))
	{
		CONS_Alert(CONS_WARNING,
				M_GetText ("Illegal schedule clear received from %s\n"),
				player_names[playernum]);
		if (server)
			SendKick(playernum, KICK_MSG_CON_FAIL);
		return;
	}

	Schedule_Clear();

	if (server || consoleplayer == playernum)
	{
		CONS_Printf("All scheduled tasks have been cleared.\n");
	}
}

static void Got_Automatecmd(UINT8 **cp, INT32 playernum)
{
	UINT8 eventID;
	char command[MAXTEXTCMD];

	eventID = READUINT8(*cp);
	READSTRING(*cp, command);

	if (
		(playernum != serverplayer && !IsPlayerAdmin(playernum))
		|| (eventID >= AEV__MAX)
	)
	{
		CONS_Alert(CONS_WARNING,
				M_GetText ("Illegal automate received from %s\n"),
				player_names[playernum]);
		if (server)
			SendKick(playernum, KICK_MSG_CON_FAIL);
		return;
	}

	Automate_Set(eventID, command);

	if (server || consoleplayer == playernum)
	{
		if (command[0] == '\0')
		{
			CONS_Printf(
				"Removed the %s automate command.\n",
				automate_names[eventID]
			);
		}
		else
		{
			CONS_Printf(
				"Set the %s automate command to \"" "\x82" "%s" "\x80" "\".\n",
				automate_names[eventID],
				command
			);
		}
	}
}

/** Prints the number of displayplayers[0].
  *
  * \todo Possibly remove this; it was useful for debugging at one point.
  */
static void Command_Displayplayer_f(void)
{
	int playernum;
	int i;
	for (i = 0; i <= splitscreen; ++i)
	{
		playernum = g_localplayers[i];
		CONS_Printf(
				"local   player %d: \x84(%d) \x83%s\x80\n",
				i,
				playernum,
				player_names[playernum]
		);
	}
	CONS_Printf("\x83----------------------------------------\x80\n");
	for (i = 0; i <= r_splitscreen; ++i)
	{
		playernum = displayplayers[i];
		CONS_Printf(
				"display player %d: \x84(%d) \x83%s\x80\n",
				i,
				playernum,
				player_names[playernum]
		);
	}
}

/** Quits a game and returns to the title screen.
  *
  */
void Command_ExitGame_f(void)
{
	INT32 i;

	LUA_HookBool(false, HOOK(GameQuit));

	D_QuitNetGame();
	CL_Reset();
	CV_ClearChangedFlags();

	for (i = 0; i < MAXPLAYERS; i++)
		CL_ClearPlayer(i);

	for (i = 0; i < MAXSPLITSCREENPLAYERS; i++)
	{
		players[g_localplayers[i]].availabilities = R_GetSkinAvailabilities();
	}

	splitscreen = 0;
	SplitScreen_OnChange();

	cv_debug = 0;
	emeralds = 0;
	memset(&luabanks, 0, sizeof(luabanks));

	if (dirmenu)
		closefilemenu(true);

	if (!modeattacking)
		D_StartTitle();
}

void Command_Retry_f(void)
{
	if (!(gamestate == GS_LEVEL || gamestate == GS_INTERMISSION))
	{
		CONS_Printf(M_GetText("You must be in a level to use this.\n"));
	}
	else if (grandprixinfo.gp == false && bossinfo.boss == false)
	{
		CONS_Printf(M_GetText("This only works in Grand Prix or Mission Mode.\n"));
	}
	else if (grandprixinfo.gp == true && grandprixinfo.eventmode != GPEVENT_NONE)
	{
		CONS_Printf(M_GetText("You can't retry right now!\n"));
	}
	else
	{
		M_ClearMenus(true);
		G_SetRetryFlag();
	}
}

#ifdef NETGAME_DEVMODE
// Allow the use of devmode in netgames.
static void Fishcake_OnChange(void)
{
	cv_debug = cv_fishcake.value;
	// consvar_t's get changed to default when registered
	// so don't make modifiedgame always on!
	if (cv_debug)
	{
		G_SetGameModified(multiplayer, true);
	}

	else if (cv_debug != cv_fishcake.value)
		CV_SetValue(&cv_fishcake, cv_debug);
}
#endif

/** Reports to the console whether or not the game has been modified.
  *
  * \todo Make it obvious, so a console command won't be necessary.
  * \sa modifiedgame
  * \author Graue <graue@oceanbase.org>
  */
static void Command_Isgamemodified_f(void)
{
	if (majormods)
		CONS_Printf("The game has been modified with major addons, so you cannot play Record Attack.\n");
	else if (savemoddata)
		CONS_Printf("The game has been modified with an addon with its own save data, so you can play Record Attack and earn medals.\n");
	else if (modifiedgame)
		CONS_Printf("The game has been modified with only minor addons. You can play Record Attack, earn medals and unlock extras.\n");
	else
		CONS_Printf("The game has not been modified. You can play Record Attack, earn medals and unlock extras.\n");
}

static void Command_Cheats_f(void)
{
	if (COM_CheckParm("off"))
	{
		if (!(server || (IsPlayerAdmin(consoleplayer))))
			CONS_Printf(M_GetText("Only the server or a remote admin can use this.\n"));
		else
			CV_ResetCheatNetVars();
		return;
	}

	if (CV_CheatsEnabled())
	{
		CONS_Printf(M_GetText("At least one CHEAT-marked variable has been changed -- Cheats are enabled.\n"));
		if (server || (IsPlayerAdmin(consoleplayer)))
			CONS_Printf(M_GetText("Type CHEATS OFF to reset all cheat variables to default.\n"));
	}
	else
		CONS_Printf(M_GetText("No CHEAT-marked variables are changed -- Cheats are disabled.\n"));
}

#ifdef _DEBUG
static void Command_Togglemodified_f(void)
{
	modifiedgame = !modifiedgame;
}

extern UINT8 *save_p;
static void Command_Archivetest_f(void)
{
	UINT8 *buf;
	UINT32 i, wrote;
	thinker_t *th;
	if (gamestate != GS_LEVEL)
	{
		CONS_Printf("This command only works in-game, you dummy.\n");
		return;
	}

	// assign mobjnum
	i = 1;
	for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
		if (th->function.acp1 != (actionf_p1)P_RemoveThinkerDelayed)
			((mobj_t *)th)->mobjnum = i++;

	// allocate buffer
	buf = save_p = ZZ_Alloc(1024);

	// test archive
	CONS_Printf("LUA_Archive...\n");
	LUA_Archive(&save_p);
	WRITEUINT8(save_p, 0x7F);
	wrote = (UINT32)(save_p-buf);

	// clear Lua state, so we can really see what happens!
	CONS_Printf("Clearing state!\n");
	LUA_ClearExtVars();

	// test unarchive
	save_p = buf;
	CONS_Printf("LUA_UnArchive...\n");
	LUA_UnArchive(&save_p);
	i = READUINT8(save_p);
	if (i != 0x7F || wrote != (UINT32)(save_p-buf))
		CONS_Printf("Savegame corrupted. (write %u, read %u)\n", wrote, (UINT32)(save_p-buf));

	// free buffer
	Z_Free(buf);
	CONS_Printf("Done. No crash.\n");
}
#endif

/** Give yourself an, optional quantity or one of, an item.
  *
  * \sa cv_kartallowgiveitem
*/
static void Command_KartGiveItem_f(void)
{
	char         buf[2];

	int           ac;
	const char *name;
	int         item;

	const char * str;

	int i;

	/* Allow always in local games. */
	if (! netgame || cv_kartallowgiveitem.value)
	{
		ac = COM_Argc();
		if (ac < 2)
		{
			CONS_Printf(
"kartgiveitem <item> [amount]: Give yourself an item\n"
			);
		}
		else
		{
			item = NUMKARTITEMS;

			name = COM_Argv(1);

			if (isdigit(*name) || *name == '-')
			{
				item = atoi(name);
			}
			else
			{
				for (i = 0; ( str = kartdebugitem_cons_t[i].strvalue ); ++i)
				{
					if (strcasecmp(name, str) == 0)
					{
						item = kartdebugitem_cons_t[i].value;
						break;
					}
				}
			}

			if (item < NUMKARTITEMS)
			{
				buf[0] = item;

				if (ac > 2)
					buf[1] = atoi(COM_Argv(2));
				else
					buf[1] = 1;/* default to one quantity */

				SendNetXCmd(XD_GIVEITEM, buf, 2);
			}
			else
			{
				CONS_Alert(CONS_WARNING,
						"No item matches '%s'\n",
						name);
			}
		}
	}
	else
	{
		CONS_Alert(CONS_NOTICE,
				"The server does not allow this.\n");
	}
}

static void Command_Schedule_Add(void)
{
	UINT8 buf[MAXTEXTCMD];
	UINT8 *buf_p = buf;

	size_t ac;
	INT16 seconds;
	const char *command;

	if (!(server || IsPlayerAdmin(consoleplayer)))
	{
		CONS_Printf("Only the server or a remote admin can use this.\n");
		return;
	}

	ac = COM_Argc();
	if (ac < 3)
	{
		CONS_Printf("schedule <seconds> <...>: runs the specified commands on a recurring timer\n");
		return;
	}

	seconds = atoi(COM_Argv(1));

	if (seconds <= 0)
	{
		CONS_Printf("Timer must be at least 1 second.\n");
		return;
	}

	command = COM_Argv(2);

	WRITEINT16(buf_p, seconds);
	WRITESTRING(buf_p, command);

	SendNetXCmd(XD_SCHEDULETASK, buf, buf_p - buf);
}

static void Command_Schedule_Clear(void)
{
	if (!(server || IsPlayerAdmin(consoleplayer)))
	{
		CONS_Printf("Only the server or a remote admin can use this.\n");
		return;
	}

	SendNetXCmd(XD_SCHEDULECLEAR, NULL, 0);
}

static void Command_Schedule_List(void)
{
	size_t i;

	if (!(server || IsPlayerAdmin(consoleplayer)))
	{
		// I set it up in a way that this information could be available
		// to everyone, but HOSTMOD has it server/admin-only too, so eh?
		CONS_Printf("Only the server or a remote admin can use this.\n");
		return;
	}

	if (schedule_len == 0)
	{
		CONS_Printf("No tasks are scheduled.\n");
		return;
	}

	for (i = 0; i < schedule_len; i++)
	{
		scheduleTask_t *task = schedule[i];

		CONS_Printf(
			"In " "\x82" "%d" "\x80" " second%s: " "\x82" "%s" "\x80" "\n",
			task->timer,
			(task->timer > 1) ? "s" : "",
			task->command
		);
	}
}

static void Command_Automate_Set(void)
{
	UINT8 buf[MAXTEXTCMD];
	UINT8 *buf_p = buf;

	size_t ac;

	const char *event;
	size_t eventID;

	const char *command;

	if (!(server || IsPlayerAdmin(consoleplayer)))
	{
		CONS_Printf("Only the server or a remote admin can use this.\n");
		return;
	}

	ac = COM_Argc();
	if (ac < 3)
	{
		CONS_Printf("automate_set <event> <command>: sets the command to run each time a event triggers\n");
		return;
	}

	event = COM_Argv(1);

	for (eventID = 0; eventID < AEV__MAX; eventID++)
	{
		if (strcasecmp(event, automate_names[eventID]) == 0)
		{
			break;
		}
	}

	if (eventID == AEV__MAX)
	{
		CONS_Printf("Unknown event type \"%s\".\n", event);
		return;
	}

	command = COM_Argv(2);

	WRITEUINT8(buf_p, eventID);
	WRITESTRING(buf_p, command);

	SendNetXCmd(XD_AUTOMATE, buf, buf_p - buf);
}

/** Makes a change to ::cv_forceskin take effect immediately.
  *
  * \sa Command_SetForcedSkin_f, cv_forceskin, forcedskin
  * \author Graue <graue@oceanbase.org>
  */
static void ForceSkin_OnChange(void)
{
	// NOT in SP, silly!
	if (!(netgame || multiplayer))
		return;

	if (cv_forceskin.value < 0)
		CONS_Printf("The server has lifted the forced skin restrictions.\n");
	else
	{
		CONS_Printf("The server is restricting all players to skin \"%s\".\n",cv_forceskin.string);
		ForceAllSkins(cv_forceskin.value);
	}
}

//Allows the player's name to be changed if cv_mute is off.
static void Name_OnChange(void)
{
	if (cv_mute.value && !(server || IsPlayerAdmin(consoleplayer)))
	{
		CONS_Alert(CONS_NOTICE, M_GetText("You may not change your name when chat is muted.\n"));
		CV_StealthSet(&cv_playername[0], player_names[consoleplayer]);
	}
	else
		SendNameAndColor(0);

}

static void Name2_OnChange(void)
{
	if (cv_mute.value) //Secondary player can't be admin.
	{
		CONS_Alert(CONS_NOTICE, M_GetText("You may not change your name when chat is muted.\n"));
		CV_StealthSet(&cv_playername[1], player_names[g_localplayers[1]]);
	}
	else
		SendNameAndColor(1);
}

static void Name3_OnChange(void)
{
	if (cv_mute.value) //Third player can't be admin.
	{
		CONS_Alert(CONS_NOTICE, M_GetText("You may not change your name when chat is muted.\n"));
		CV_StealthSet(&cv_playername[2], player_names[g_localplayers[2]]);
	}
	else
		SendNameAndColor(2);
}

static void Name4_OnChange(void)
{
	if (cv_mute.value) //Secondary player can't be admin.
	{
		CONS_Alert(CONS_NOTICE, M_GetText("You may not change your name when chat is muted.\n"));
		CV_StealthSet(&cv_playername[3], player_names[g_localplayers[3]]);
	}
	else
		SendNameAndColor(3);
}

// sends the follower change for players
static void Follower_OnChange(void)
{
	char str[SKINNAMESIZE+1], cpy[SKINNAMESIZE+1];
	INT32 num;

	// there is a slight chance that we will actually use a string instead so...
	// let's investigate the string...
	strcpy(str, cv_follower[0].string);
	strcpy(cpy, cv_follower[0].string);
	strlwr(str);
	if (stricmp(cpy,"0") !=0 && !atoi(cpy))	// yep, that's a string alright...
	{
		if (stricmp(cpy, "None") == 0)
		{
			CV_StealthSet(&cv_follower[0], "-1");

			if (!Playing())
				return; // don't send anything there.

			SendNameAndColor(0);
			return;
		}

		num = K_FollowerAvailable(str);

		if (num == -1) // that's an error.
			CONS_Alert(CONS_WARNING, M_GetText("Follower '%s' not found\n"), str);

		CV_StealthSet(&cv_follower[0], str);
		cv_follower[0].value = num;
	}

	if (!Playing())
		return; // don't send anything there.

	SendNameAndColor(0);
}

// About the same as Color_OnChange but for followers.
static void Followercolor_OnChange(void)
{
	if (!Playing())
		return; // do whatever you want if you aren't in the game or don't have a follower.

	if (!P_PlayerMoving(consoleplayer))
	{
		// Color change menu scrolling fix is no longer necessary
		SendNameAndColor(0);
	}
}

// repeat for the 3 other players

static void Follower2_OnChange(void)
{
	char str[SKINNAMESIZE+1], cpy[SKINNAMESIZE+1];
	INT32 num;

	// there is a slight chance that we will actually use a string instead so...
	// let's investigate the string...
	strcpy(str, cv_follower[1].string);
	strcpy(cpy, cv_follower[1].string);
	strlwr(str);
	if (stricmp(cpy,"0") !=0 && !atoi(cpy))	// yep, that's a string alright...
	{
		if (stricmp(cpy, "None") == 0)
		{
			CV_StealthSet(&cv_follower[1], "-1");

			if (!Playing())
				return; // don't send anything there.

			SendNameAndColor(1);
			return;
		}

		num = K_FollowerAvailable(str);

		if (num == -1) // that's an error.
			CONS_Alert(CONS_WARNING, M_GetText("Follower '%s' not found\n"), str);

		CV_StealthSet(&cv_follower[1], str);
		cv_follower[1].value = num;
	}

	if (!Playing())
		return; // don't send anything there.

	SendNameAndColor(1);
}

static void Followercolor2_OnChange(void)
{
	if (!Playing())
		return; // do whatever you want if you aren't in the game or don't have a follower.

	if (!P_PlayerMoving(g_localplayers[1]))
	{
		// Color change menu scrolling fix is no longer necessary
		SendNameAndColor(1);
	}
}

static void Follower3_OnChange(void)
{
	char str[SKINNAMESIZE+1], cpy[SKINNAMESIZE+1];
	INT32 num;

	// there is a slight chance that we will actually use a string instead so...
	// let's investigate the string...
	strcpy(str, cv_follower[2].string);
	strcpy(cpy, cv_follower[2].string);
	strlwr(str);
	if (stricmp(cpy,"0") !=0 && !atoi(cpy))	// yep, that's a string alright...
	{
		if (stricmp(cpy, "None") == 0)
		{
			CV_StealthSet(&cv_follower[2], "-1");

			if (!Playing())
				return; // don't send anything there.

			SendNameAndColor(2);
			return;
		}

		num = K_FollowerAvailable(str);

		if (num == -1) // that's an error.
			CONS_Alert(CONS_WARNING, M_GetText("Follower '%s' not found\n"), str);

		CV_StealthSet(&cv_follower[2], str);
		cv_follower[2].value = num;
	}

	if (!Playing())
		return; // don't send anything there.

	SendNameAndColor(2);
}

static void Followercolor3_OnChange(void)
{
	if (!Playing())
		return; // do whatever you want if you aren't in the game or don't have a follower.

	if (!P_PlayerMoving(g_localplayers[2]))
	{
		// Color change menu scrolling fix is no longer necessary
		SendNameAndColor(2);
	}
}

static void Follower4_OnChange(void)
{
	char str[SKINNAMESIZE+1], cpy[SKINNAMESIZE+1];
	INT32 num;

	// there is a slight chance that we will actually use a string instead so...
	// let's investigate the string...
	strcpy(str, cv_follower[3].string);
	strcpy(cpy, cv_follower[3].string);
	strlwr(str);
	if (stricmp(cpy,"0") !=0 && !atoi(cpy))	// yep, that's a string alright...
	{
		if (stricmp(cpy, "None") == 0)
		{
			CV_StealthSet(&cv_follower[3], "-1");

			if (!Playing())
				return; // don't send anything there.

			SendNameAndColor(3);
			return;
		}

		num = K_FollowerAvailable(str);

		if (num == -1) // that's an error.
			CONS_Alert(CONS_WARNING, M_GetText("Follower '%s' not found\n"), str);

		CV_StealthSet(&cv_follower[3], str);
		cv_follower[3].value = num;
	}

	if (!Playing())
		return; // don't send anything there.

	SendNameAndColor(3);
}

static void Followercolor4_OnChange(void)
{
	if (!Playing())
		return; // do whatever you want if you aren't in the game or don't have a follower.

	if (!P_PlayerMoving(g_localplayers[3]))
	{
		// Color change menu scrolling fix is no longer necessary
		SendNameAndColor(3);
	}
}

/** Sends a skin change for the console player, unless that player is moving.
  * \sa cv_skin, Skin2_OnChange, Color_OnChange
  * \author Graue <graue@oceanbase.org>
  */
static void Skin_OnChange(void)
{
	if (!Playing())
		return; // do whatever you want

	if (!(cv_debug || devparm) && !(multiplayer || netgame) // In single player.
		&& (gamestate != GS_WAITINGPLAYERS)) // allows command line -warp x +skin y
	{
		CV_StealthSet(&cv_skin[0], skins[players[consoleplayer].skin].name);
		return;
	}

	if (CanChangeSkin(consoleplayer) && !P_PlayerMoving(consoleplayer))
		SendNameAndColor(0);
	else
	{
		CONS_Alert(CONS_NOTICE, M_GetText("You can't change your skin at the moment.\n"));
		CV_StealthSet(&cv_skin[0], skins[players[consoleplayer].skin].name);
	}
}

/** Sends a skin change for the secondary splitscreen player, unless that
  * player is moving.
  * \sa cv_skin2, Skin_OnChange, Color2_OnChange
  * \author Graue <graue@oceanbase.org>
  */
static void Skin2_OnChange(void)
{
	if (!Playing() || !splitscreen)
		return; // do whatever you want

	if (CanChangeSkin(g_localplayers[1]) && !P_PlayerMoving(g_localplayers[1]))
		SendNameAndColor(1);
	else
	{
		CONS_Alert(CONS_NOTICE, M_GetText("You can't change your skin at the moment.\n"));
		CV_StealthSet(&cv_skin[1], skins[players[g_localplayers[1]].skin].name);
	}
}

static void Skin3_OnChange(void)
{
	if (!Playing() || splitscreen < 2)
		return; // do whatever you want

	if (CanChangeSkin(g_localplayers[2]) && !P_PlayerMoving(g_localplayers[2]))
		SendNameAndColor(2);
	else
	{
		CONS_Alert(CONS_NOTICE, M_GetText("You can't change your skin at the moment.\n"));
		CV_StealthSet(&cv_skin[2], skins[players[g_localplayers[2]].skin].name);
	}
}

static void Skin4_OnChange(void)
{
	if (!Playing() || splitscreen < 3)
		return; // do whatever you want

	if (CanChangeSkin(g_localplayers[3]) && !P_PlayerMoving(g_localplayers[3]))
		SendNameAndColor(3);
	else
	{
		CONS_Alert(CONS_NOTICE, M_GetText("You can't change your skin at the moment.\n"));
		CV_StealthSet(&cv_skin[3], skins[players[g_localplayers[3]].skin].name);
	}
}

/** Sends a color change for the console player, unless that player is moving.
  * \sa cv_playercolor, Color2_OnChange, Skin_OnChange
  * \author Graue <graue@oceanbase.org>
  */
static void Color_OnChange(void)
{
	if (!Playing())
	{
		if (!cv_playercolor[0].value || !skincolors[cv_playercolor[0].value].accessible)
			CV_StealthSetValue(&cv_playercolor[0], lastgoodcolor[0]);
	}
	else
	{
		if (!(cv_debug || devparm) && !(multiplayer || netgame)) // In single player.
		{
			CV_StealthSet(&cv_skin[0], skins[players[consoleplayer].skin].name);
			return;
		}

		if (!P_PlayerMoving(consoleplayer) && skincolors[players[consoleplayer].skincolor].accessible == true)
		{
			// Color change menu scrolling fix is no longer necessary
			SendNameAndColor(0);
		}
		else
		{
			CV_StealthSetValue(&cv_playercolor[0],
				players[consoleplayer].skincolor);
		}
	}
	lastgoodcolor[0] = cv_playercolor[0].value;
}

/** Sends a color change for the secondary splitscreen player, unless that
  * player is moving.
  * \sa cv_playercolor2, Color_OnChange, Skin2_OnChange
  * \author Graue <graue@oceanbase.org>
  */
static void Color2_OnChange(void)
{
	if (!Playing() || splitscreen < 1)
	{
		if (!cv_playercolor[1].value || !skincolors[cv_playercolor[1].value].accessible)
			CV_StealthSetValue(&cv_playercolor[1], lastgoodcolor[1]);
	}
	else
	{
		if (!P_PlayerMoving(g_localplayers[1]) && skincolors[players[g_localplayers[1]].skincolor].accessible == true)
		{
			// Color change menu scrolling fix is no longer necessary
			SendNameAndColor(1);
		}
		else
		{
			CV_StealthSetValue(&cv_playercolor[1],
				players[g_localplayers[1]].skincolor);
		}
	}
	lastgoodcolor[1] = cv_playercolor[1].value;
}

static void Color3_OnChange(void)
{
	if (!Playing() || splitscreen < 2)
	{
		if (!cv_playercolor[2].value || !skincolors[cv_playercolor[2].value].accessible)
			CV_StealthSetValue(&cv_playercolor[2], lastgoodcolor[2]);
	}
	else
	{
		if (!P_PlayerMoving(g_localplayers[2]) && skincolors[players[g_localplayers[2]].skincolor].accessible == true)
		{
			// Color change menu scrolling fix is no longer necessary
			SendNameAndColor(2);
		}
		else
		{
			CV_StealthSetValue(&cv_playercolor[2],
				players[g_localplayers[2]].skincolor);
		}
	}
	lastgoodcolor[2] = cv_playercolor[2].value;
}

static void Color4_OnChange(void)
{
	if (!Playing() || splitscreen < 3)
	{
		if (!cv_playercolor[3].value || !skincolors[cv_playercolor[3].value].accessible)
			CV_StealthSetValue(&cv_playercolor[3], lastgoodcolor[3]);
	}
	else
	{
		if (!P_PlayerMoving(g_localplayers[3]) && skincolors[players[g_localplayers[3]].skincolor].accessible == true)
		{
			// Color change menu scrolling fix is no longer necessary
			SendNameAndColor(3);
		}
		else
		{
			CV_StealthSetValue(&cv_playercolor[3],
				players[g_localplayers[3]].skincolor);
		}
	}
	lastgoodcolor[3] = cv_playercolor[3].value;
}

/** Displays the result of the chat being muted or unmuted.
  * The server or remote admin should already know and be able to talk
  * regardless, so this is only displayed to clients.
  *
  * \sa cv_mute
  * \author Graue <graue@oceanbase.org>
  */
static void Mute_OnChange(void)
{
	/*if (server || (IsPlayerAdmin(consoleplayer)))
		return;*/
	// Kinda dumb IMO, you should be able to see confirmation for having muted the chat as the host or admin.

	if (leveltime <= 1)
		return;	// avoid having this notification put in our console / log when we boot the server.

	if (cv_mute.value)
		HU_AddChatText(M_GetText("\x82*Chat has been muted."), false);
	else
		HU_AddChatText(M_GetText("\x82*Chat is no longer muted."), false);
}

/** Hack to clear all changed flags after game start.
  * A lot of code (written by dummies, obviously) uses COM_BufAddText() to run
  * commands and change consvars, especially on game start. This is problematic
  * because CV_ClearChangedFlags() needs to get called on game start \b after
  * all those commands are run.
  *
  * Here's how it's done: the last thing in COM_BufAddText() is "dummyconsvar
  * 1", so we end up here, where dummyconsvar is reset to 0 and all the changed
  * flags are set to 0.
  *
  * \todo Fix the aforementioned code and make this hack unnecessary.
  * \sa cv_dummyconsvar
  * \author Graue <graue@oceanbase.org>
  */
static void DummyConsvar_OnChange(void)
{
	if (cv_dummyconsvar.value == 1)
	{
		CV_SetValue(&cv_dummyconsvar, 0);
		CV_ClearChangedFlags();
	}
}

static void Command_ShowScores_f(void)
{
	UINT8 i;

	if (!(netgame || multiplayer))
	{
		CONS_Printf(M_GetText("This only works in a netgame.\n"));
		return;
	}

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (playeringame[i])
			// FIXME: %lu? what's wrong with %u? ~Callum (produces warnings...)
			CONS_Printf(M_GetText("%s's score is %u\n"), player_names[i], players[i].score);
	}
	CONS_Printf(M_GetText("The pointlimit is %d\n"), cv_pointlimit.value);

}

static void Command_ShowTime_f(void)
{
	if (!(netgame || multiplayer))
	{
		CONS_Printf(M_GetText("This only works in a netgame.\n"));
		return;
	}

	CONS_Printf(M_GetText("The current time is %f.\nThe timelimit is %f\n"), (double)leveltime/TICRATE, (double)timelimitintics/TICRATE);
}

// SRB2Kart: On change messages
static void NumLaps_OnChange(void)
{
	if (K_CanChangeRules() == false)
	{
		return;
	}

	if (leveltime < starttime)
	{
		CONS_Printf(M_GetText("Number of laps have been set to %d.\n"), cv_numlaps.value);
		numlaps = (UINT8)cv_numlaps.value;
	}
	else
	{
		CONS_Printf(M_GetText("Number of laps will be set to %d next round.\n"), cv_numlaps.value);
	}
}

static void KartFrantic_OnChange(void)
{
	if (K_CanChangeRules() == false)
	{
		return;
	}

	if (leveltime < starttime)
	{
		CONS_Printf(M_GetText("Frantic items has been set to %s.\n"), cv_kartfrantic.value ? M_GetText("on") : M_GetText("off"));
		franticitems = (boolean)cv_kartfrantic.value;
	}
	else
	{
		CONS_Printf(M_GetText("Frantic items will be turned %s next round.\n"), cv_kartfrantic.value ? M_GetText("on") : M_GetText("off"));
	}
}

static void KartSpeed_OnChange(void)
{
	if (!M_SecretUnlocked(SECRET_HARDSPEED) && cv_kartspeed.value == KARTSPEED_HARD)
	{
		CONS_Printf(M_GetText("You haven't earned this yet.\n"));
		CV_StealthSet(&cv_kartspeed, cv_kartspeed.defaultvalue);
		return;
	}

	if (K_CanChangeRules() == false)
	{
		return;
	}

	if (leveltime < starttime && cv_kartspeed.value != KARTSPEED_AUTO)
	{
		CONS_Printf(M_GetText("Game speed has been changed to \"%s\".\n"), cv_kartspeed.string);
		gamespeed = (UINT8)cv_kartspeed.value;
	}
	else
	{
		CONS_Printf(M_GetText("Game speed will be changed to \"%s\" next round.\n"), cv_kartspeed.string);
	}
}

static void KartEncore_OnChange(void)
{
	if (K_CanChangeRules() == false)
	{
		return;
	}

	CONS_Printf(M_GetText("Encore Mode will be set to %s next round.\n"), cv_kartencore.string);
}

static void KartComeback_OnChange(void)
{
	if (K_CanChangeRules() == false)
	{
		return;
	}

	if (leveltime < starttime)
	{
		CONS_Printf(M_GetText("Karma Comeback has been turned %s.\n"), cv_kartcomeback.value ? M_GetText("on") : M_GetText("off"));
		comeback = (boolean)cv_kartcomeback.value;
	}
	else
	{
		CONS_Printf(M_GetText("Karma Comeback will be turned %s next round.\n"), cv_kartcomeback.value ? M_GetText("on") : M_GetText("off"));
	}
}

static void KartEliminateLast_OnChange(void)
{
	if (K_CanChangeRules() == false)
	{
		CV_StealthSet(&cv_karteliminatelast, cv_karteliminatelast.defaultvalue);
	}

	P_CheckRacers();
}

static void KartRings_OnChange(void)
{
	if (K_CanChangeRules() == false)
	{
		return;
	}

	if (ringsdisabled)
	{
		CONS_Printf(M_GetText("Rings will be turned %s Next Round .\n"), cv_kartrings.string);
	}
	else
	{
		CONS_Printf(M_GetText("Rings will be turned %s Next Round .\n"), cv_kartrings.string);
	}
}

static void Schedule_OnChange(void)
{
	size_t i;

	if (cv_schedule.value)
	{
		return;
	}

	if (schedule_len == 0)
	{
		return;
	}

	// Reset timers when turning off.
	for (i = 0; i < schedule_len; i++)
	{
		scheduleTask_t *task = schedule[i];
		task->timer = task->basetime;
	}
}

void Got_DiscordInfo(UINT8 **p, INT32 playernum)
{
	if (playernum != serverplayer /*&& !IsPlayerAdmin(playernum)*/)
	{
		// protect against hacked/buggy client
		CONS_Alert(CONS_WARNING, M_GetText("Illegal Discord info command received from %s\n"), player_names[playernum]);
		if (server)
			SendKick(playernum, KICK_MSG_CON_FAIL);
		return;
	}

	// Don't do anything with the information if we don't have Discord RP support
#ifdef HAVE_DISCORDRPC
	discordInfo.maxPlayers = READUINT8(*p);
	discordInfo.joinsAllowed = (boolean)READUINT8(*p);
	discordInfo.everyoneCanInvite = (boolean)READUINT8(*p);

	DRPC_UpdatePresence();
#else
	(*p) += 3;
#endif
}
