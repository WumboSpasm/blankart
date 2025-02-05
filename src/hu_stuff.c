// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  hu_stuff.c
/// \brief Heads up display

#include "doomdef.h"
#include "byteptr.h"
#include "hu_stuff.h"
#include "font.h"

#include "m_menu.h" // gametype_cons_t
#include "m_cond.h" // emblems
#include "m_misc.h" // word jumping

#include "d_clisrv.h"

#include "g_game.h"
#include "g_input.h"

#include "i_video.h"
#include "i_system.h"

#include "st_stuff.h" // ST_HEIGHT
#include "r_local.h"

#include "keys.h"
#include "v_video.h"

#include "w_wad.h"
#include "z_zone.h"

#include "console.h"
#include "am_map.h"
#include "d_main.h"

#include "p_local.h" // camera[]
#include "p_tick.h"

#ifdef HWRENDER
#include "hardware/hw_main.h"
#endif

#include "lua_hud.h"
#include "lua_hudlib_drawlist.h"
#include "lua_hook.h"

// SRB2Kart
#include "s_sound.h" // song credits
#include "k_kart.h"
#include "k_boss.h"
#include "k_color.h"
#include "k_hud.h"
#include "r_fps.h"

// coords are scaled
#define HU_INPUTX 0
#define HU_INPUTY 0

typedef enum
{
	HU_SHOUT		= 1,		// Shout message
	HU_CSAY			= 1<<1,		// Middle-of-screen server message
} sayflags_t;

//-------------------------------------------
//              heads up font
//-------------------------------------------

// ping font
// Note: I'd like to adress that at this point we might *REALLY* want to work towards a common drawString function that can take any font we want because this is really turning into a MESS. :V -Lat'
patch_t *pinggfx[5];	// small ping graphic
patch_t *mping[5]; // smaller ping graphic
patch_t *pingmeasure[2]; // ping measurement graphic
patch_t *pinglocal[2]; // mindelay indecator

patch_t *framecounter;
patch_t *frameslash;	// framerate stuff. Used in screen.c

static player_t *plr;
boolean hu_keystrokes; // :)
boolean chat_on; // entering a chat message?
static char w_chat[HU_MAXMSGLEN + 1];
static size_t c_input = 0; // let's try to make the chat input less shitty.
static boolean headsupactive = false;
boolean hu_showscores; // draw rankings
static char hu_tick;

//-------------------------------------------
//              misc vars
//-------------------------------------------

patch_t *missingpat;

// song credits
static patch_t *songcreditbg;

// -------
// protos.
// -------
static void HU_DrawRankings(void);

//======================================================================
//                 KEYBOARD LAYOUTS FOR ENTERING TEXT
//======================================================================

char *shiftxform;

char english_shiftxform[] =
{
	0,
	1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
	11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
	21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
	31,
	' ', '!', '"', '#', '$', '%', '&',
	'"', // shift-'
	'(', ')', '*', '+',
	'<', // shift-,
	'_', // shift--
	'>', // shift-.
	'?', // shift-/
	')', // shift-0
	'!', // shift-1
	'@', // shift-2
	'#', // shift-3
	'$', // shift-4
	'%', // shift-5
	'^', // shift-6
	'&', // shift-7
	'*', // shift-8
	'(', // shift-9
	':',
	':', // shift-;
	'<',
	'+', // shift-=
	'>', '?', '@',
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
	'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
	'{', // shift-[
	'|', // shift-backslash - OH MY GOD DOES WATCOM SUCK
	'}', // shift-]
	'"', '_',
	'~', // shift-`
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
	'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
	'{', '|', '}', '~', 127
};

static char cechotext[1024];
static tic_t cechotimer = 0;
static tic_t cechoduration = 5*TICRATE;
static INT32 cechoflags = 0;

static tic_t resynch_ticker = 0;

static huddrawlist_h luahuddrawlist_scores;

//======================================================================
//                          HEADS UP INIT
//======================================================================

#ifndef NONET
// just after
static void Command_Say_f(void);
static void Command_Sayto_f(void);
static void Command_Sayteam_f(void);
static void Command_CSay_f(void);
static void Command_Shout(void);
static void Got_Saycmd(UINT8 **p, INT32 playernum);
#endif

void HU_LoadGraphics(void)
{
	INT32 i;

	if (dedicated)
		return;

	Font_Load();

	HU_UpdatePatch(&songcreditbg, "K_SONGCR");

	// cache ping gfx:
	for (i = 0; i < 5; i++)
	{
		HU_UpdatePatch(&pinggfx[i], "PINGGFX%d", i+1);
		HU_UpdatePatch(&mping[i], "MPING%d", i+1);
	}

	HU_UpdatePatch(&pingmeasure[0], "PINGD");
	HU_UpdatePatch(&pingmeasure[1], "PINGMS");

	HU_UpdatePatch(&pinglocal[0], "PINGGFXL");
	HU_UpdatePatch(&pinglocal[1], "MPINGL");

	// fps stuff
	HU_UpdatePatch(&framecounter, "FRAMER");
	HU_UpdatePatch(&frameslash, "FRAMESL");
}

// Initialise Heads up
// once at game startup.
//
void HU_Init(void)
{
	font_t font;

#ifndef NONET
	COM_AddCommand("say", Command_Say_f);
	COM_AddCommand("sayto", Command_Sayto_f);
	COM_AddCommand("sayteam", Command_Sayteam_f);
	COM_AddCommand("csay", Command_CSay_f);
	COM_AddCommand("shout", Command_Shout);
	RegisterNetXCmd(XD_SAY, Got_Saycmd);
#endif

	// only allocate if not present, to save us a lot of headache
	if (missingpat == NULL)
	{
		lumpnum_t missingnum = W_GetNumForName("MISSING");
		if (missingnum == LUMPERROR)
			I_Error("HU_LoadGraphics: \"MISSING\" patch not present in resource files.");

		missingpat = W_CachePatchNum(missingnum, PU_STATIC);
	}

	// set shift translation table
	shiftxform = english_shiftxform;

	/*
	Setup fonts
	*/

	if (!dedicated)
	{
#define  DIM( s, n ) ( font.start = s, font.size = n )
#define ADIM( name )        DIM (name ## _FONTSTART, name ## _FONTSIZE)
#define   PR( s )           strcpy(font.prefix, s)
#define  DIG( n )           ( font.digits = n )
#define  REG                Font_DumbRegister(&font)

		DIG  (3);

		ADIM (HU);

		PR   ("STCFN");
		REG;

		PR   ("TNYFN");
		REG;

		ADIM (KART);
		PR   ("MKFNT");
		REG;

		ADIM (LT);
		PR   ("LTFNT");
		REG;

		ADIM (CRED);
		PR   ("CRFNT");
		REG;

		DIG  (3);

		ADIM (LT);

		PR   ("GTOL");
		REG;

		PR   ("GTFN");
		REG;

		DIG  (1);

		DIM  (0, 10);

		PR   ("STTNUM");
		REG;

		PR   ("NGTNUM");
		REG;

		PR   ("PINGN");
		REG;

#undef  REG
#undef  DIG
#undef  PR
#undef  ADMIN
#undef  DIM
	}

	HU_LoadGraphics();

	luahuddrawlist_scores = LUA_HUD_CreateDrawList();
}

patch_t *HU_UpdateOrBlankPatch(patch_t **user, boolean required, const char *format, ...)
{
	va_list ap;
	char buffer[9];

	lumpnum_t lump = INT16_MAX;
	patch_t *patch;

	va_start (ap, format);
	vsnprintf(buffer, sizeof buffer, format, ap);
	va_end   (ap);

	if (user && partadd_earliestfile != UINT16_MAX)
	{
		UINT16 fileref = numwadfiles;
		lump = INT16_MAX;

		while ((lump == INT16_MAX) && ((--fileref) >= partadd_earliestfile))
		{
			lump = W_CheckNumForNamePwad(buffer, fileref, 0);
		}

		/* no update in this wad */
		if (fileref < partadd_earliestfile)
			return *user;

		CONS_Printf("pe = %d, fr = %d\n", partadd_earliestfile, fileref);

		lump |= (fileref << 16);
	}
	else
	{
		lump = W_CheckNumForName(buffer);

		if (lump == LUMPERROR)
		{
			if (required == true)
				*user = missingpat;

			return *user;
		}
	}

	patch = W_CachePatchNum(lump, PU_HUDGFX);

	if (user)
	{
		if (*user)
			Patch_Free(*user);

		*user = patch;
	}

	return patch;
}

static inline void HU_Stop(void)
{
	headsupactive = false;
}

//
// Reset Heads up when consoleplayer spawns
//
void HU_Start(void)
{
	if (headsupactive)
		HU_Stop();

	plr = &players[consoleplayer];

	headsupactive = true;
}

//======================================================================
//                            EXECUTION
//======================================================================

#ifndef NONET

// EVERY CHANGE IN THIS SCRIPT IS LOL XD! BY VINCYTM

static UINT32 chat_nummsg_log = 0;
static UINT32 chat_nummsg_min = 0;
static UINT32 chat_scroll = 0;
static tic_t chat_scrolltime = 0;

static UINT32 chat_maxscroll = 0; // how far can we scroll?

//static chatmsg_t chat_mini[CHAT_BUFSIZE]; // Display the last few messages sent.
//static chatmsg_t chat_log[CHAT_BUFSIZE]; // Keep every message sent to us in memory so we can scroll n shit, it's cool.

static char chat_log[CHAT_BUFSIZE][255]; // hold the last 48 or so messages in that log.
static char chat_mini[8][255]; // display up to 8 messages that will fade away / get overwritten
static tic_t chat_timers[8];

static boolean chat_scrollmedown = false; // force instant scroll down on the chat log. Happens when you open it / send a message.

// remove text from minichat table

static INT16 addy = 0; // use this to make the messages scroll smoothly when one fades away

static void HU_removeChatText_Mini(void)
{
	// MPC: Don't create new arrays, just iterate through an existing one
	size_t i;
	for(i=0;i<chat_nummsg_min-1;i++) {
		strcpy(chat_mini[i], chat_mini[i+1]);
		chat_timers[i] = chat_timers[i+1];
	}
	chat_nummsg_min--; // lost 1 msg.

	// use addy and make shit slide smoothly af.
	addy += (vid.width < 640) ? 8 : 6;

}

// same but w the log. TODO: optimize this and maybe merge in a single func? im bad at C.
static void HU_removeChatText_Log(void)
{
	// MPC: Don't create new arrays, just iterate through an existing one
	size_t i;
	for(i=0;i<chat_nummsg_log-1;i++) {
		strcpy(chat_log[i], chat_log[i+1]);
	}
	chat_nummsg_log--; // lost 1 msg.
}
#endif

void HU_AddChatText(const char *text, boolean playsound)
{
#ifndef NONET
	if (playsound && cv_consolechat.value != 2)	// Don't play the sound if we're using hidden chat.
		S_StartSound(NULL, sfx_radio);
	// reguardless of our preferences, put all of this in the chat buffer in case we decide to change from oldchat mid-game.

	if (chat_nummsg_log >= CHAT_BUFSIZE) // too many messages!
		HU_removeChatText_Log();

	strcpy(chat_log[chat_nummsg_log], text);
	chat_nummsg_log++;

	if (chat_nummsg_min >= 8)
		HU_removeChatText_Mini();

	strcpy(chat_mini[chat_nummsg_min], text);
	chat_timers[chat_nummsg_min] = TICRATE*cv_chattime.value;
	chat_nummsg_min++;

	if (OLDCHAT) // if we're using oldchat, print directly in console
		CONS_Printf("%s\n", text);
	else			// if we aren't, still save the message to log.txt
		CON_LogMessage(va("%s\n", text));
#else
	(void)playsound;
	CONS_Printf("%s\n", text);
#endif
}

#ifndef NONET

/** Runs a say command, sending an ::XD_SAY message.
  * A say command consists of a signed 8-bit integer for the target, an
  * unsigned 8-bit flag variable, and then the message itself.
  *
  * The target is 0 to say to everyone, 1 to 32 to say to that player, or -1
  * to -32 to say to everyone on that player's team. Note: This means you
  * have to add 1 to the player number, since they are 0 to 31 internally.
  *
  * The flag HU_SHOUT will be set if it is the dedicated server speaking.
  *
  * This function obtains the message using COM_Argc() and COM_Argv().
  *
  * \param target    Target to send message to.
  * \param usedargs  Number of arguments to ignore.
  * \param flags     Set HU_CSAY for server/admin to CECHO everyone.
  * \sa Command_Say_f, Command_Sayteam_f, Command_Sayto_f, Got_Saycmd
  * \author Graue <graue@oceanbase.org>
  */

static void DoSayCommand(SINT8 target, size_t usedargs, UINT8 flags)
{
	char buf[2 + HU_MAXMSGLEN + 1];
	size_t numwords, ix;
	char *msg = &buf[2];
	const size_t msgspace = sizeof buf - 2;

	numwords = COM_Argc() - usedargs;
	I_Assert(numwords > 0);

	if (CHAT_MUTE) // TODO: Per Player mute.
	{
		HU_AddChatText(va("%s>ERROR: The chat is muted. You can't say anything.", "\x85"), false);
		return;
	}

	// Only servers/admins can shout or CSAY.
	if (!server && !IsPlayerAdmin(consoleplayer))
	{
		flags &= ~(HU_SHOUT|HU_CSAY);
	}

	// Enforce shout for the dedicated server.
	if (dedicated && !(flags & HU_CSAY))
	{
		flags |= HU_SHOUT;
	}

	buf[0] = target;
	buf[1] = flags;
	msg[0] = '\0';

	for (ix = 0; ix < numwords; ix++)
	{
		if (ix > 0)
			strlcat(msg, " ", msgspace);
		strlcat(msg, COM_Argv(ix + usedargs), msgspace);
	}

	if (strlen(msg) > 4 && strnicmp(msg, "/pm", 3) == 0) // used /pm
	{
		// what we're gonna do now is check if the player exists
		// with that logic, characters 4 and 5 are our numbers:
		const char *newmsg;
		char playernum[3];
		INT32 spc = 1; // used if playernum[1] is a space.

		strncpy(playernum, msg+3, 3);

		// check for undesirable characters in our "number"
		if (((playernum[0] < '0') || (playernum[0] > '9')) || ((playernum[1] < '0') || (playernum[1] > '9')))
		{
			// check if playernum[1] is a space
			if (playernum[1] == ' ')
				spc = 0;
			// let it slide
			else
			{
				HU_AddChatText("\x82NOTICE: \x80Invalid command format. Correct format is \'/pm<playernum> \'.", false);
				return;
			}
		}
		// I'm very bad at C, I swear I am, additional checks eww!
		if (spc != 0 && msg[5] != ' ')
		{
			HU_AddChatText("\x82NOTICE: \x80Invalid command format. Correct format is \'/pm<playernum> \'.", false);
			return;
		}

		target = atoi(playernum); // turn that into a number
		//CONS_Printf("%d\n", target);

		// check for target player, if it doesn't exist then we can't send the message!
		if (target < MAXPLAYERS && playeringame[target]) // player exists
			target++; // even though playernums are from 0 to 31, target is 1 to 32, so up that by 1 to have it work!
		else
		{
			HU_AddChatText(va("\x82NOTICE: \x80Player %d does not exist.", target), false); // same
			return;
		}
		buf[0] = target;
		newmsg = msg+5+spc;
		strlcpy(msg, newmsg, HU_MAXMSGLEN + 1);
	}

	SendNetXCmd(XD_SAY, buf, strlen(msg) + 1 + msg-buf);
}

/** Send a message to everyone.
  * \sa DoSayCommand, Command_Sayteam_f, Command_Sayto_f
  * \author Graue <graue@oceanbase.org>
  */
static void Command_Say_f(void)
{
	if (COM_Argc() < 2)
	{
		CONS_Printf(M_GetText("say <message>: send a message\n"));
		return;
	}

	// Autoshout is handled by HU_queueChatChar.
	// If you're using the say command, you can use the shout command, lol.
	DoSayCommand(0, 1, 0);
}

/** Send a message to a particular person.
  * \sa DoSayCommand, Command_Sayteam_f, Command_Say_f
  * \author Graue <graue@oceanbase.org>
  */
static void Command_Sayto_f(void)
{
	INT32 target;

	if (COM_Argc() < 3)
	{
		CONS_Printf(M_GetText("sayto <playername|playernum> <message>: send a message to a player\n"));
		return;
	}

	target = nametonum(COM_Argv(1));
	if (target == -1)
	{
		CONS_Alert(CONS_NOTICE, M_GetText("No player with that name!\n"));
		return;
	}
	target++; // Internally we use 0 to 31, but say command uses 1 to 32.

	DoSayCommand((SINT8)target, 2, 0);
}

/** Send a message to members of the player's team.
  * \sa DoSayCommand, Command_Say_f, Command_Sayto_f
  * \author Graue <graue@oceanbase.org>
  */
static void Command_Sayteam_f(void)
{
	if (COM_Argc() < 2)
	{
		CONS_Printf(M_GetText("sayteam <message>: send a message to your team\n"));
		return;
	}

	if (dedicated)
	{
		CONS_Alert(CONS_NOTICE, M_GetText("Dedicated servers can't send team messages. Use \"say\".\n"));
		return;
	}

	if (G_GametypeHasTeams())	// revert to normal say if we don't have teams in this gametype.
		DoSayCommand(-1, 1, 0);
	else
		DoSayCommand(0, 1, 0);
}

/** Send a message to everyone, to be displayed by CECHO. Only
  * permitted to servers and admins.
  */
static void Command_CSay_f(void)
{
	if (COM_Argc() < 2)
	{
		CONS_Printf(M_GetText("csay <message>: send a message to be shown in the middle of the screen\n"));
		return;
	}

	if (!server && !IsPlayerAdmin(consoleplayer))
	{
		CONS_Alert(CONS_NOTICE, M_GetText("Only servers and admins can use csay.\n"));
		return;
	}

	DoSayCommand(0, 1, HU_CSAY);
}

static void Command_Shout(void)
{
	if (COM_Argc() < 2)
	{
		CONS_Printf(M_GetText("shout <message>: send a message with special alert sound, name, and color\n"));
		return;
	}

	if (!server && !IsPlayerAdmin(consoleplayer))
	{
		CONS_Alert(CONS_NOTICE, M_GetText("Only servers and admins can use shout.\n"));
		return;
	}

	DoSayCommand(0, 1, HU_SHOUT);
}

static tic_t stop_spamming[MAXPLAYERS];

/** Receives a message, processing an ::XD_SAY command.
  * \sa DoSayCommand
  * \author Graue <graue@oceanbase.org>
  */
static void Got_Saycmd(UINT8 **p, INT32 playernum)
{
	SINT8 target;
	UINT8 flags;
	const char *dispname;
	char *msg;
	boolean action = false;
	char *ptr;
	INT32 spam_eatmsg = 0;

	CONS_Debug(DBG_NETPLAY,"Received SAY cmd from Player %d (%s)\n", playernum+1, player_names[playernum]);

	target = READSINT8(*p);
	flags = READUINT8(*p);
	msg = (char *)*p;
	SKIPSTRINGL(*p, HU_MAXMSGLEN + 1);

	if ((cv_mute.value || flags & (HU_CSAY|HU_SHOUT)) && playernum != serverplayer && !(IsPlayerAdmin(playernum)))
	{
		CONS_Alert(CONS_WARNING, cv_mute.value ?
			M_GetText("Illegal say command received from %s while muted\n") : M_GetText("Illegal csay command received from non-admin %s\n"),
			player_names[playernum]);
		if (server)
			SendKick(playernum, KICK_MSG_CON_FAIL);
		return;
	}

	//check for invalid characters (0x80 or above)
	{
		size_t i;
		const size_t j = strlen(msg);
		for (i = 0; i < j; i++)
		{
			if (msg[i] & 0x80)
			{
				CONS_Alert(CONS_WARNING, M_GetText("Illegal say command received from %s containing invalid characters\n"), player_names[playernum]);
				if (server)
					SendKick(playernum, KICK_MSG_CON_FAIL);
				return;
			}
		}
	}

	// before we do anything, let's verify the guy isn't spamming, get this easier on us.

	//if (stop_spamming[playernum] != 0 && cv_chatspamprotection.value && !(flags & HU_CSAY))
	if (stop_spamming[playernum] != 0 && consoleplayer != playernum && cv_chatspamprotection.value && !(flags & (HU_CSAY|HU_SHOUT)))
	{
		CONS_Debug(DBG_NETPLAY,"Received SAY cmd too quickly from Player %d (%s), assuming as spam and blocking message.\n", playernum+1, player_names[playernum]);
		stop_spamming[playernum] = 4;
		spam_eatmsg = 1;
	}
	else
		stop_spamming[playernum] = 4; // you can hold off for 4 tics, can you?

	// run the lua hook even if we were supposed to eat the msg, netgame consistency goes first.

	if (LUA_HookPlayerMsg(playernum, target, flags, msg, spam_eatmsg))
		return;

	if (spam_eatmsg)
		return; // don't proceed if we were supposed to eat the message.

	// If it's a CSAY, just CECHO and be done with it.
	if (flags & HU_CSAY)
	{
		HU_SetCEchoDuration(5);
		I_OutputMsg("Server message: ");
		HU_DoCEcho(msg);
		return;
	}

	// Handle "/me" actions, but only in messages to everyone.
	if (target == 0 && strlen(msg) > 4 && strnicmp(msg, "/me ", 4) == 0)
	{
		msg += 4;
		action = true;
	}

	if (flags & HU_SHOUT)
		dispname = cv_shoutname.zstring;
	else
		dispname = player_names[playernum];

	// Clean up message a bit
	// If you use a \r character, you can remove your name
	// from before the text and then pretend to be someone else!
	ptr = msg;
	while (*ptr != '\0')
	{
		if (*ptr == '\r')
			*ptr = ' ';

		ptr++;
	}

	// Show messages sent by you, to you, to your team, or to everyone:
	if (playernum == consoleplayer // By you
	|| (target == -1 && ST_SameTeam(&players[consoleplayer], &players[playernum])) // To your team
	|| target == 0 // To everyone
	|| consoleplayer == target-1) // To you
	{
		const char *prefix = "", *cstart = "", *cend = "", *adminchar = "\x82~\x83", *remotechar = "\x82@\x83", *fmt2, *textcolor = "\x80";
		char *tempchar = NULL;
		char color_prefix[2];

		if (flags & HU_SHOUT)
		{
			if (cv_shoutcolor.value == -1)
			{
				UINT16 chatcolor = skincolors[players[playernum].skincolor].chatcolor;

				if (chatcolor > V_TANMAP)
				{
					sprintf(color_prefix, "%c", '\x80');
				}
				else
				{
					sprintf(color_prefix, "%c", '\x80' + (chatcolor >> V_CHARCOLORSHIFT));
				}
			}
			else
			{
				sprintf(color_prefix, "%c", '\x80' + cv_shoutcolor.value);
			}

			// Colorize full text
			cstart = textcolor = color_prefix;
		}
		else if (players[playernum].spectator)
		{
			// grey text
			cstart = textcolor = "\x86";
		}
		else if (target == -1) // say team
		{
			if (players[playernum].ctfteam == 1)
			{
				// red text
				cstart = textcolor = "\x85";
			}
			else
			{
				// blue text
				cstart = textcolor = "\x84";
			}
		}
		else
		{
			UINT16 chatcolor = skincolors[players[playernum].skincolor].chatcolor;

			if (chatcolor > V_TANMAP)
			{
				sprintf(color_prefix, "%c", '\x80');
			}
			else
			{
				sprintf(color_prefix, "%c", '\x80' + (chatcolor >> V_CHARCOLORSHIFT));
			}

			cstart = color_prefix;
		}
		prefix = cstart;

		// Give admins and remote admins their symbols.
		if (playernum == serverplayer)
			tempchar = (char *)Z_Calloc(strlen(cstart) + strlen(adminchar) + 1, PU_STATIC, NULL);
		else if (IsPlayerAdmin(playernum))
			tempchar = (char *)Z_Calloc(strlen(cstart) + strlen(remotechar) + 1, PU_STATIC, NULL);

		if (tempchar)
		{
			if (playernum == serverplayer)
				strcat(tempchar, adminchar);
			else
				strcat(tempchar, remotechar);
			strcat(tempchar, cstart);
			cstart = tempchar;
		}

		// Choose the proper format string for display.
		// Each format includes four strings: color start, display
		// name, color end, and the message itself.
		// '\4' makes the message yellow and beeps; '\3' just beeps.
		if (action)
			fmt2 = "* %s%s%s%s \x82%s%s";
		else if (target-1 == consoleplayer) // To you
		{
			prefix = "\x82[PM]";
			cstart = "\x82";
			textcolor = "\x82";
			fmt2 = "%s<%s%s>%s\x80 %s%s";
		}
		else if (target > 0) // By you, to another player
		{
			// Use target's name.
			dispname = player_names[target-1];
			prefix = "\x82[TO]";
			cstart = "\x82";
			fmt2 = "%s<%s%s>%s\x80 %s%s";

		}
		else // To everyone or sayteam, it doesn't change anything.
			fmt2 = "%s<%s%s%s>\x80 %s%s";
		/*else // To your team
		{
			if (players[playernum].ctfteam == 1) // red
				prefix = "\x85[TEAM]";
			else if (players[playernum].ctfteam == 2) // blue
				prefix = "\x84[TEAM]";
			else
				prefix = "\x83"; // makes sure this doesn't implode if you sayteam on non-team gamemodes

			fmt2 = "%s<%s%s>\x80%s %s%s";
		}*/

		HU_AddChatText(va(fmt2, prefix, cstart, dispname, cend, textcolor, msg), (cv_chatnotifications.value) && !(flags & HU_SHOUT)); // add to chat

		if ((cv_chatnotifications.value) && (flags & HU_SHOUT))
			S_StartSound(NULL, sfx_sysmsg);

		if (tempchar)
			Z_Free(tempchar);
	}
#ifdef _DEBUG
	// I just want to point out while I'm here that because the data is still
	// sent to all players, techincally anyone can see your chat if they really
	// wanted to, even if you used sayto or sayteam.
	// You should never send any sensitive info through sayto for that reason.
	else
		CONS_Printf("Dropped chat: %d %d %s\n", playernum, target, msg);
#endif
}

// Handles key input and string input
//
static inline boolean HU_keyInChatString(char *s, char ch)
{
	size_t l;

	if ((ch >= HU_FONTSTART && ch <= HU_FONTEND && fontv[HU_FONT].font[ch-HU_FONTSTART])
	  || ch == ' ') // Allow spaces, of course
	{
		l = strlen(s);
		if (l < HU_MAXMSGLEN - 1)
		{
			if (c_input >= strlen(s)) // don't do anything complicated
			{
				s[l++] = ch;
				s[l]=0;
			}
			else
			{
				// move everything past c_input for new characters:
				size_t m = HU_MAXMSGLEN-1;
				while (m>=c_input)
				{
					if (s[m])
						s[m+1] = (s[m]);
					if (m == 0) // prevent overflow
						break;
					m--;
				}
				s[c_input] = ch; // and replace this.
			}
			c_input++;
			return true;
		}
		return false;
	}
	else if (ch == KEY_BACKSPACE)
	{
		size_t i = c_input;

		if (c_input <= 0)
			return false;

		if (!s[i-1])
			return false;

		if (i >= strlen(s)-1)
		{
			s[strlen(s)-1] = 0;
			c_input--;
			return false;
		}

		for (; (i < HU_MAXMSGLEN); i++)
		{
			s[i-1] = s[i];
		}
		c_input--;
	}
	else if (ch != KEY_ENTER)
		return false; // did not eat key

	return true; // ate the key
}

#endif

//
//
static void HU_TickSongCredits(void)
{
	if (cursongcredit.def == NULL) // No def
	{
		cursongcredit.x = cursongcredit.old_x = 0;
		cursongcredit.anim = 0;
		cursongcredit.trans = NUMTRANSMAPS;
		return;
	}

	cursongcredit.old_x = cursongcredit.x;

	if (cursongcredit.anim > 0)
	{
		INT32 len = V_ThinStringWidth(cursongcredit.text, V_ALLOWLOWERCASE|V_6WIDTHSPACE);
		fixed_t destx = (len+7) * FRACUNIT;

		if (cursongcredit.trans > 0)
		{
			cursongcredit.trans--;
		}

		if (cursongcredit.x < destx)
		{
			cursongcredit.x += (destx - cursongcredit.x) / 2;
		}

		if (cursongcredit.x > destx)
		{
			cursongcredit.x = destx;
		}

		cursongcredit.anim--;
	}
	else
	{
		if (cursongcredit.trans < NUMTRANSMAPS)
		{
			cursongcredit.trans++;
		}

		if (cursongcredit.x > 0)
		{
			cursongcredit.x /= 2;
		}

		if (cursongcredit.x < 0)
		{
			cursongcredit.x = 0;
		}
	}
}

void HU_Ticker(void)
{
	if (dedicated)
		return;

	hu_tick++;
	hu_tick &= 7; // currently only to blink chat input cursor

	if (PlayerInputDown(1, gc_scores))
		hu_showscores = !chat_on;
	else
		hu_showscores = false;

	hu_keystrokes = false;

	if (chat_on)
	{
		// count down the scroll timer.
		if (chat_scrolltime > 0)
			chat_scrolltime--;
	}

	if (netgame)
	{
		size_t i = 0;

		// handle spam while we're at it:
		for(; (i<MAXPLAYERS); i++)
		{
			if (stop_spamming[i] > 0)
				stop_spamming[i]--;
		}

		// handle chat timers
		for (i=0; (i<chat_nummsg_min); i++)
		{
			if (chat_timers[i] > 0)
				chat_timers[i]--;
			else
				HU_removeChatText_Mini();
		}
	}

	if (cechotimer)
		cechotimer--;

	if (gamestate != GS_LEVEL)
	{
		return;
	}

	resynch_ticker++;

	HU_TickSongCredits();
}

#ifndef NONET

static boolean teamtalk = false;
static boolean justscrolleddown;
static boolean justscrolledup;
static INT16 typelines = 1; // number of drawfill lines we need when drawing the chat. it's some weird hack and might be one frame off but I'm lazy to make another loop.
// It's up here since it has to be reset when we open the chat.

static boolean HU_chatboxContainsOnlySpaces(void)
{
	size_t i;

	for (i = 0; w_chat[i]; i++)
		if (w_chat[i] != ' ')
			return false;

	return true;
}

static void HU_sendChatMessage(void)
{
	char buf[2 + HU_MAXMSGLEN + 1];
	char *msg = &buf[2];
	size_t ci;
	INT32 target = 0;

	// if our message was nothing but spaces, don't send it.
	if (HU_chatboxContainsOnlySpaces())
		return;

	// copy printable characters and terminating '\0' only.
	for (ci = 2; w_chat[ci-2]; ci++)
	{
		char c = w_chat[ci-2];
		if (c >= ' ' && !(c & 0x80))
			buf[ci] = c;
	};
	buf[ci] = '\0';

	memset(w_chat, '\0', sizeof(w_chat));
	c_input = 0;

	// last minute mute check
	if (CHAT_MUTE)
	{
		HU_AddChatText(va("%s>ERROR: The chat is muted. You can't say anything.", "\x85"), false);
		return;
	}

	if (strlen(msg) > 4 && strnicmp(msg, "/pm", 3) == 0) // used /pm
	{
		INT32 spc = 1; // used if playernum[1] is a space.
		char playernum[3];
		const char *newmsg;

		// what we're gonna do now is check if the player exists
		// with that logic, characters 4 and 5 are our numbers:

		// teamtalk can't send PMs, just don't send it, else everyone would be able to see it, and no one wants to see your sex RP sicko.
		if (teamtalk)
		{
			HU_AddChatText(va("%sCannot send sayto in Say-Team.", "\x85"), false);
			return;
		}

		strncpy(playernum, msg+3, 3);
		// check for undesirable characters in our "number"
		if (!(isdigit(playernum[0]) && isdigit(playernum[1])))
		{
			// check if playernum[1] is a space
			if (playernum[1] == ' ')
				spc = 0;
				// let it slide
			else
			{
				HU_AddChatText("\x82NOTICE: \x80Invalid command format. Correct format is \'/pm<player num> \'.", false);
				return;
			}
		}
		// I'm very bad at C, I swear I am, additional checks eww!
		if (spc != 0 && msg[5] != ' ')
		{
			HU_AddChatText("\x82NOTICE: \x80Invalid command format. Correct format is \'/pm<player num> \'.", false);
			return;
		}

		target = atoi(playernum); // turn that into a number

		// check for target player, if it doesn't exist then we can't send the message!
		if (target < MAXPLAYERS && playeringame[target]) // player exists
			target++; // even though playernums are from 0 to 31, target is 1 to 32, so up that by 1 to have it work!
		else
		{
			HU_AddChatText(va("\x82NOTICE: \x80Player %d does not exist.", target), false); // same
			return;
		}

		// we need to get rid of the /pm<player num>
		newmsg = msg+5+spc;
		strlcpy(msg, newmsg, HU_MAXMSGLEN + 1);
	}
	if (ci > 2) // don't send target+flags+empty message.
	{
		if (teamtalk)
			buf[0] = -1; // target
		else
			buf[0] = target;

		buf[1] = ((server || IsPlayerAdmin(consoleplayer)) && cv_autoshout.value) ? HU_SHOUT : 0; // flags
		SendNetXCmd(XD_SAY, buf, 2 + strlen(&buf[2]) + 1);
	}
}
#endif

void HU_clearChatChars(void)
{
	memset(w_chat, '\0', sizeof(w_chat));
	chat_on = false;
	c_input = 0;

	I_UpdateMouseGrab();
}

//
// Returns true if key eaten
//
boolean HU_Responder(event_t *ev)
{
	if (ev->type != ev_keydown)
		return false;

	// only KeyDown events now...

	// Shoot, to prevent P1 chatting from ruining the game for everyone else, it's either:
	// A. completely disallow opening chat entirely in online splitscreen
	// or B. iterate through all controls to make sure it's bound to player 1 before eating
	// You can see which one I chose.
	// (Unless if you're sharing a keyboard, since you probably establish when you start chatting that you have dibs on it...)
	// (Ahhh, the good ol days when I was a kid who couldn't afford an extra USB controller...)

	if (ev->data1 >= KEY_MOUSE1)
	{
		INT32 i;
		for (i = 0; i < num_gamecontrols; i++)
		{
			if (gamecontrol[0][i][0] == ev->data1 || gamecontrol[0][i][1] == ev->data1)
				break;
		}

		if (i == num_gamecontrols)
			return false;
	}

#ifndef NONET
	if (!chat_on)
	{
		// enter chat mode
		if ((ev->data1 == gamecontrol[0][gc_talkkey][0] || ev->data1 == gamecontrol[0][gc_talkkey][1])
			&& netgame && !OLD_MUTE) // check for old chat mute, still let the players open the chat incase they want to scroll otherwise.
		{
			chat_on = true;
			w_chat[0] = 0;
			teamtalk = false;
			chat_scrollmedown = true;
			typelines = 1;
			return true;
		}
		if ((ev->data1 == gamecontrol[0][gc_teamkey][0] || ev->data1 == gamecontrol[0][gc_teamkey][1])
			&& netgame && !OLD_MUTE)
		{
			chat_on = true;
			w_chat[0] = 0;
			teamtalk = G_GametypeHasTeams();	// Don't teamtalk if we don't have teams.
			chat_scrollmedown = true;
			typelines = 1;
			return true;
		}
	}
	else // if chat_on
	{
		INT32 c = (INT32)ev->data1;

		// Ignore modifier keys
		// Note that we do this here so users can still set
		// their chat keys to one of these, if they so desire.
		if (ev->data1 == KEY_LSHIFT || ev->data1 == KEY_RSHIFT
		 || ev->data1 == KEY_LCTRL || ev->data1 == KEY_RCTRL
		 || ev->data1 == KEY_LALT || ev->data1 == KEY_RALT)
			return true;

		// Ignore non-keyboard keys, except when the talk key is bound
		if (ev->data1 >= KEY_MOUSE1
		&& (ev->data1 != gamecontrol[0][gc_talkkey][0]
		&& ev->data1 != gamecontrol[0][gc_talkkey][1]))
			return false;

		c = CON_ShiftChar(c);

		// pasting. pasting is cool. chat is a bit limited, though :(
		if ((c == 'v' || c == 'V') && ctrldown)
		{
			const char *paste;
			size_t chatlen;
			size_t pastelen;

			if (CHAT_MUTE)
				return true;

			paste = I_ClipboardPaste();
			if (paste == NULL)
				return true;

			chatlen = strlen(w_chat);
			pastelen = strlen(paste);
			if (chatlen+pastelen > HU_MAXMSGLEN)
				return true; // we can't paste this!!

			memmove(&w_chat[c_input + pastelen], &w_chat[c_input], pastelen);
			memcpy(&w_chat[c_input], paste, pastelen); // copy all of that.
			c_input += pastelen;
			return true;
		}
		else if (c == KEY_ENTER)
		{
			if (!CHAT_MUTE)
				HU_sendChatMessage();

			chat_on = false;
			c_input = 0; // reset input cursor
			chat_scrollmedown = true; // you hit enter, so you might wanna autoscroll to see what you just sent. :)
			I_UpdateMouseGrab();
		}
		else if (c == KEY_ESCAPE
			|| ((c == gamecontrol[0][gc_talkkey][0] || c == gamecontrol[0][gc_talkkey][1]
			|| c == gamecontrol[0][gc_teamkey][0] || c == gamecontrol[0][gc_teamkey][1])
			&& c >= KEY_MOUSE1)) // If it's not a keyboard key, then the chat button is used as a toggle.
		{
			chat_on = false;
			c_input = 0; // reset input cursor
			I_UpdateMouseGrab();
		}
		else if ((c == KEY_UPARROW || c == KEY_MOUSEWHEELUP) && chat_scroll > 0 && !OLDCHAT) // CHAT SCROLLING YAYS!
		{
			chat_scroll--;
			justscrolledup = true;
			chat_scrolltime = 4;
		}
		else if ((c == KEY_DOWNARROW || c == KEY_MOUSEWHEELDOWN) && chat_scroll < chat_maxscroll && chat_maxscroll > 0 && !OLDCHAT)
		{
			chat_scroll++;
			justscrolleddown = true;
			chat_scrolltime = 4;
		}
		else if (c == KEY_LEFTARROW && c_input != 0 && !OLDCHAT) // i said go back
		{
			if (ctrldown)
				c_input = M_JumpWordReverse(w_chat, c_input);
			else
				c_input--;
		}
		else if (c == KEY_RIGHTARROW && c_input < strlen(w_chat) && !OLDCHAT) // don't need to check for admin or w/e here since the chat won't ever contain anything if it's muted.
		{
			if (ctrldown)
				c_input += M_JumpWord(&w_chat[c_input]);
			else
				c_input++;
		}
		else if ((c >= HU_FONTSTART && c <= HU_FONTEND && fontv[HU_FONT].font[c-HU_FONTSTART])
			|| c == ' ') // Allow spaces, of course
		{
			if (CHAT_MUTE || strlen(w_chat) >= HU_MAXMSGLEN)
				return true;

			memmove(&w_chat[c_input + 1], &w_chat[c_input], strlen(w_chat) - c_input + 1);
			w_chat[c_input] = c;
			c_input++;
		}
		else if (c == KEY_BACKSPACE)
		{
			if (CHAT_MUTE || c_input <= 0)
				return true;

			memmove(&w_chat[c_input - 1], &w_chat[c_input], strlen(w_chat) - c_input + 1);
			c_input--;
		}
		else if (c == KEY_DEL)
		{
			if (CHAT_MUTE || c_input >= strlen(w_chat))
				return true;

			memmove(&w_chat[c_input], &w_chat[c_input + 1], strlen(w_chat) - c_input);
		}

		return true;
	}
#endif

	return false;
}

//======================================================================
//                         HEADS UP DRAWING
//======================================================================

#ifndef NONET

// Precompile a wordwrapped string to any given width.
// This is a muuuch better method than V_WORDWRAP.
// again stolen and modified a bit from video.c, don't mind me, will need to rearrange this one day.
// this one is simplified for the chat drawer.
static char *CHAT_WordWrap(INT32 x, INT32 w, INT32 option, const char *string)
{
	INT32 c;
	size_t chw, i, lastusablespace = 0;
	size_t slen;
	char *newstring = Z_StrDup(string);
	INT32 spacewidth = (vid.width < 640) ? 8 : 4, charwidth = (vid.width < 640) ? 8 : 4;

	slen = strlen(string);
	x = 0;

	for (i = 0; i < slen; ++i)
	{
		c = newstring[i];
		if ((UINT8)c >= 0x80 && (UINT8)c <= 0x8F) //color parsing! -Inuyasha 2.16.09
			continue;

		if (c == '\n')
		{
			x = 0;
			lastusablespace = 0;
			continue;
		}

		if (!(option & V_ALLOWLOWERCASE))
			c = toupper(c);
		c -= HU_FONTSTART;

		if (c < 0 || c >= HU_FONTSIZE || !fontv[HU_FONT].font[c])
		{
			chw = spacewidth;
			lastusablespace = i;
		}
		else
			chw = charwidth;

		x += chw;

		if (lastusablespace != 0 && x > w)
		{
			//CONS_Printf("Wrap at index %d\n", i);
			newstring[lastusablespace] = '\n';
			i = lastusablespace+1;
			lastusablespace = 0;
			x = 0;
		}
	}
	return newstring;
}


// 30/7/18: chaty is now the distance at which the lowest point of the chat will be drawn if that makes any sense.

INT16 chatx = 13, chaty = 169; // let's use this as our coordinates

// chat stuff by VincyTM LOL XD!

// HU_DrawMiniChat

static void HU_drawMiniChat(void)
{
	INT32 x = chatx+2;
	INT32 charwidth = 4, charheight = 6;
	INT32 boxw = cv_chatwidth.value;
	INT32 dx = 0, dy = 0;
	size_t i = chat_nummsg_min;
	boolean prev_linereturn = false; // a hack to prevent double \n while I have no idea why they happen in the first place.

	INT32 msglines = 0;
	// process all messages once without rendering anything or doing anything fancy so that we know how many lines each message has...
	INT32 y;

	if (!chat_nummsg_min)
		return; // needless to say it's useless to do anything if we don't have anything to draw.

	if (r_splitscreen > 1)
		boxw = max(64, boxw/2);

	for (; i>0; i--)
	{
		char *msg = CHAT_WordWrap(x+2, boxw-(charwidth*2), V_SNAPTOBOTTOM|V_SNAPTOLEFT|V_ALLOWLOWERCASE, chat_mini[i-1]);
		size_t j = 0;
		INT32 linescount = 0;

		while(msg[j]) // iterate through msg
		{
			if (msg[j] < HU_FONTSTART) // don't draw
			{
				if (msg[j] == '\n') // get back down.
				{
					++j;
					if (!prev_linereturn)
					{
						linescount += 1;
						dx = 0;
					}
					prev_linereturn = true;
					continue;
				}
				else if (msg[j] & 0x80) // stolen from video.c, nice.
				{
					++j;
					continue;
				}

				++j;
			}
			else
			{
				j++;
			}
			prev_linereturn = false;
			dx += charwidth;
			if (dx >= boxw)
			{
				dx = 0;
				linescount += 1;
			}
		}
		dy = 0;
		dx = 0;
		msglines += linescount+1;

		if (msg)
			Z_Free(msg);
	}

	y = chaty - charheight*(msglines+1);

#ifdef NETSPLITSCREEN
	if (r_splitscreen)
	{
		y -= BASEVIDHEIGHT/2;
		if (r_splitscreen > 1)
			y += 16;
	}
	else
#endif
	{
		y -= (cv_kartspeedometer.value ? 16 : 0);
	}

	dx = 0;
	dy = 0;
	i = 0;
	prev_linereturn = false;

	for (; i<=(chat_nummsg_min-1); i++) // iterate through our hot messages
	{
		INT32 clrflag = 0;
		INT32 timer = ((cv_chattime.value*TICRATE)-chat_timers[i]) - cv_chattime.value*TICRATE+9; // see below...
		INT32 transflag = (timer >= 0 && timer <= 9) ? (timer*V_10TRANS) : 0; // you can make bad jokes out of this one.
		size_t j = 0;
		char *msg = CHAT_WordWrap(x+2, boxw-(charwidth*2), V_SNAPTOBOTTOM|V_SNAPTOLEFT|V_ALLOWLOWERCASE, chat_mini[i]); // get the current message, and word wrap it.
		UINT8 *colormap = NULL;

		while(msg[j]) // iterate through msg
		{
			if (msg[j] < HU_FONTSTART) // don't draw
			{
				if (msg[j] == '\n') // get back down.
				{
					++j;
					if (!prev_linereturn)
					{
						dy += charheight;
						dx = 0;
					}
					prev_linereturn = true;
					continue;
				}
				else if (msg[j] & 0x80) // stolen from video.c, nice.
				{
					clrflag = ((msg[j] & 0x7f) << V_CHARCOLORSHIFT) & V_CHARCOLORMASK;
					colormap = V_GetStringColormap(clrflag);
					++j;
					continue;
				}

				++j;
			}
			else
			{
				if (cv_chatbacktint.value) // on request of wolfy
					V_DrawFillConsoleMap(x + dx + 2, y+dy, charwidth, charheight, 159|V_SNAPTOBOTTOM|V_SNAPTOLEFT);

				V_DrawChatCharacter(x + dx + 2, y+dy, msg[j++] |V_SNAPTOBOTTOM|V_SNAPTOLEFT|transflag, true, colormap);
			}

			dx += charwidth;
			prev_linereturn = false;
			if (dx >= boxw)
			{
				dx = 0;
				dy += charheight;
			}
		}
		dy += charheight;
		dx = 0;

		if (msg)
			Z_Free(msg);
	}

	// decrement addy and make that shit smooth:
	addy /= 2;

}

// HU_DrawChatLog

static void HU_drawChatLog(INT32 offset)
{
	INT32 charwidth = 4, charheight = 6;
	INT32 boxw = cv_chatwidth.value, boxh = cv_chatheight.value;
	INT32 x = chatx+2, y, dx = 0, dy = 0;
	UINT32 i = 0;
	INT32 chat_topy, chat_bottomy;
	INT32 highlight = HU_GetHighlightColor();
	boolean atbottom = false;

	// make sure that our scroll position isn't "illegal";
	if (chat_scroll > chat_maxscroll)
		chat_scroll = chat_maxscroll;

#ifdef NETSPLITSCREEN
	if (r_splitscreen)
	{
		boxh = max(6, boxh/2);
		if (r_splitscreen > 1)
			boxw = max(64, boxw/2);
	}
#endif

	y = chaty - offset*charheight - (chat_scroll*charheight) - boxh*charheight - 12;

#ifdef NETSPLITSCREEN
	if (r_splitscreen)
	{
		y -= BASEVIDHEIGHT/2;

		if (r_splitscreen > 1)
			y += 16;
	}
	else
#endif
	{
		y -= (cv_kartspeedometer.value ? 16 : 0);
	}

	chat_topy = y + chat_scroll*charheight;
	chat_bottomy = chat_topy + boxh*charheight;

	V_DrawFillConsoleMap(chatx, chat_topy, boxw, boxh*charheight +2, 159|V_SNAPTOBOTTOM|V_SNAPTOLEFT); // log box

	for (i=0; i<chat_nummsg_log; i++) // iterate through our chatlog
	{
		INT32 clrflag = 0;
		INT32 j = 0;
		char *msg = CHAT_WordWrap(x+2, boxw-(charwidth*2), V_SNAPTOBOTTOM|V_SNAPTOLEFT|V_ALLOWLOWERCASE, chat_log[i]); // get the current message, and word wrap it.
		UINT8 *colormap = NULL;
		while(msg[j]) // iterate through msg
		{
			if (msg[j] < HU_FONTSTART) // don't draw
			{
				if (msg[j] == '\n') // get back down.
				{
					++j;
					dy += charheight;
					dx = 0;
					continue;
				}
				else if (msg[j] & 0x80) // stolen from video.c, nice.
				{
					clrflag = ((msg[j] & 0x7f) << V_CHARCOLORSHIFT) & V_CHARCOLORMASK;
					colormap = V_GetStringColormap(clrflag);
					++j;
					continue;
				}

				++j;
			}
			else
			{
				if ((y+dy+2 >= chat_topy) && (y+dy < (chat_bottomy)))
					V_DrawChatCharacter(x + dx + 2, y+dy+2, msg[j++] |V_SNAPTOBOTTOM|V_SNAPTOLEFT, true, colormap);
				else
					j++; // don't forget to increment this or we'll get stuck in the limbo.
			}

			dx += charwidth;
			if (dx >= boxw-charwidth-2 && i<chat_nummsg_log && msg[j] >= HU_FONTSTART) // end of message shouldn't count, nor should invisible characters!!!!
			{
				dx = 0;
				dy += charheight;
			}
		}
		dy += charheight;
		dx = 0;

		if (msg)
			Z_Free(msg);
	}


	if (((chat_scroll >= chat_maxscroll) || (chat_scrollmedown)) && !(justscrolleddown || justscrolledup || chat_scrolltime)) // was already at the bottom of the page before new maxscroll calculation and was NOT scrolling.
	{
		atbottom = true; // we should scroll
	}
	chat_scrollmedown = false;

	// getmaxscroll through a lazy hack. We do all these loops,
	// so let's not do more loops that are gonna lag the game more. :P
	chat_maxscroll = max(dy / charheight - cv_chatheight.value, 0);

	// if we're not bound by the time, autoscroll for next frame:
	if (atbottom)
		chat_scroll = chat_maxscroll;

	// draw arrows to indicate that we can (or not) scroll.
	// account for Y = -1 offset in tinyfont
	if (chat_scroll > 0)
		V_DrawCharacter(chatx-9, ((justscrolledup) ? (chat_topy-1) : (chat_topy)), V_SNAPTOBOTTOM | V_SNAPTOLEFT | highlight | '\x1A', false); // up arrow
	if (chat_scroll < chat_maxscroll)
		V_DrawCharacter(chatx-9, chat_bottomy-((justscrolleddown) ? 5 : 6), V_SNAPTOBOTTOM | V_SNAPTOLEFT | highlight | '\x1B', false); // down arrow

	justscrolleddown = false;
	justscrolledup = false;
}

//
// HU_DrawChat
//
// Draw chat input
//

static void HU_DrawChat(void)
{
	INT32 charwidth = 4, charheight = 6;
	INT32 boxw = cv_chatwidth.value;
	INT32 t = 0, c = 0, y = chaty - (typelines*charheight);
	UINT32 i = 0, saylen = strlen(w_chat); // You learn new things everyday!
	INT32 cflag = 0;
	const char *ntalk = "Say: ", *ttalk = "Team: ";
	const char *talk = ntalk;
	const char *mute = "Chat has been muted.";

#ifdef NETSPLITSCREEN
	if (r_splitscreen)
	{
		y -= BASEVIDHEIGHT/2;
		if (r_splitscreen > 1)
		{
			y += 16;
			boxw = max(64, boxw/2);
		}
	}
	else
#endif
	{
		y -= (cv_kartspeedometer.value ? 16 : 0);
	}

	if (teamtalk)
	{
		talk = ttalk;
#if 0
		if (players[consoleplayer].ctfteam == 1)
			t = 0x500;  // Red
		else if (players[consoleplayer].ctfteam == 2)
			t = 0x400; // Blue
#endif
	}

	if (CHAT_MUTE)
	{
		talk = mute;
		typelines = 1;
		cflag = V_GRAYMAP; // set text in gray if chat is muted.
	}

	V_DrawFillConsoleMap(chatx, y-1, boxw, (typelines*charheight), 159 | V_SNAPTOBOTTOM | V_SNAPTOLEFT);

	while (talk[i])
	{
		if (talk[i] < HU_FONTSTART)
			++i;
		else
		{
			V_DrawChatCharacter(chatx + c + 2, y, talk[i] |V_SNAPTOBOTTOM|V_SNAPTOLEFT|cflag, true, V_GetStringColormap(talk[i]|cflag));
			i++;
		}

		c += charwidth;
	}

	// if chat is muted, just draw the log and get it over with, no need to draw anything else.
	if (CHAT_MUTE)
	{
		HU_drawChatLog(0);
		return;
	}

	i = 0;
	typelines = 1;

	if ((strlen(w_chat) == 0 || c_input == 0) && hu_tick < 4)
		V_DrawChatCharacter(chatx + 2 + c, y+1, '_' |V_SNAPTOBOTTOM|V_SNAPTOLEFT|t, true, NULL);

	while (w_chat[i])
	{
		boolean skippedline = false;
		if (c_input == (i+1))
		{
			INT32 cursorx = (c+charwidth < boxw-charwidth) ? (chatx + 2 + c+charwidth) : (chatx+1); // we may have to go down.
			INT32 cursory = (cursorx != chatx+1) ? (y) : (y+charheight);
			if (hu_tick < 4)
				V_DrawChatCharacter(cursorx, cursory+1, '_' |V_SNAPTOBOTTOM|V_SNAPTOLEFT|t, true, NULL);

			if (cursorx == chatx+1 && saylen == i) // a weirdo hack
			{
				typelines += 1;
				skippedline = true;
			}
		}

		//Hurdler: isn't it better like that?
		if (w_chat[i] < HU_FONTSTART)
			++i;
		else
			V_DrawChatCharacter(chatx + c + 2, y, w_chat[i++] | V_SNAPTOBOTTOM|V_SNAPTOLEFT | t, true, NULL);

		c += charwidth;
		if (c > boxw-(charwidth*2) && !skippedline)
		{
			c = 0;
			y += charheight;
			typelines += 1;
		}
	}

	// handle /pm list. It's messy, horrible and I don't care.
	if (strnicmp(w_chat, "/pm", 3) == 0 && vid.width >= 400 && !teamtalk) // 320x200 unsupported kthxbai
	{
		INT32 count = 0;
		INT32 p_dispy = chaty - charheight -1;
#ifdef NETSPLITSCREEN
		if (r_splitscreen)
		{
			p_dispy -= BASEVIDHEIGHT/2;
			if (r_splitscreen > 1)
				p_dispy += 16;
		}
		else
#endif
		{
			p_dispy -= (cv_kartspeedometer.value ? 16 : 0);
		}

		i = 0;
		for(i=0; (i<MAXPLAYERS); i++)
		{
			// filter: (code needs optimization pls help I'm bad with C)
			if (w_chat[3])
			{
				char playernum[3];
				UINT32 n;
				// right, that's half important: (w_chat[4] may be a space since /pm0 msg is perfectly acceptable!)
				if ( ( ((w_chat[3] != 0) && ((w_chat[3] < '0') || (w_chat[3] > '9'))) || ((w_chat[4] != 0) && (((w_chat[4] < '0') || (w_chat[4] > '9'))))) && (w_chat[4] != ' '))
					break;

				strncpy(playernum, w_chat+3, 3);
				n = atoi(playernum); // turn that into a number
				// special cases:

				if ((n == 0) && !(w_chat[4] == '0'))
				{
					if (!(i<10))
						continue;
				}
				else if ((n == 1) && !(w_chat[3] == '0'))
				{
					if (!((i == 1) || ((i >= 10) && (i <= 19))))
						continue;
				}
				else if ((n == 2) && !(w_chat[3] == '0'))
				{
					if (!((i == 2) || ((i >= 20) && (i <= 29))))
						continue;
				}
				else if ((n == 3) && !(w_chat[3] == '0'))
				{
					if (!((i == 3) || ((i >= 30) && (i <= 31))))
						continue;
				}
				else	// general case.
				{
					if (i != n)
						continue;
				}
			}

			if (playeringame[i])
			{
				char name[MAXPLAYERNAME+1];
				strlcpy(name, player_names[i], 7); // shorten name to 7 characters.
				V_DrawFillConsoleMap(chatx+ boxw + 2, p_dispy- (6*count), 48, 6, 159 | V_SNAPTOBOTTOM | V_SNAPTOLEFT); // fill it like the chat so the text doesn't become hard to read because of the hud.
				V_DrawSmallString(chatx+ boxw + 4, p_dispy- (6*count), V_SNAPTOBOTTOM|V_SNAPTOLEFT|V_ALLOWLOWERCASE, va("\x82%d\x80 - %s", i, name));
				count++;
			}
		}
		if (count == 0) // no results.
		{
			V_DrawFillConsoleMap(chatx+boxw+2, p_dispy- (6*count), 48, 6, 159 | V_SNAPTOBOTTOM | V_SNAPTOLEFT); // fill it like the chat so the text doesn't become hard to read because of the hud.
			V_DrawSmallString(chatx+boxw+4, p_dispy- (6*count), V_SNAPTOBOTTOM|V_SNAPTOLEFT|V_ALLOWLOWERCASE, "NO RESULT.");
		}
	}

	HU_drawChatLog(typelines-1); // typelines is the # of lines we're typing. If there's more than 1 then the log should scroll up to give us more space.
}


// For anyone who, for some godforsaken reason, likes oldchat.


static void HU_DrawChat_Old(void)
{
	INT32 t = 0, c = 0, y = HU_INPUTY;
	size_t i = 0;
	const char *ntalk = "Say: ", *ttalk = "Say-Team: ";
	const char *talk = ntalk;
	INT32 charwidth = 8 * con_scalefactor; //(fontv[HU_FONT].font['A'-HU_FONTSTART]->width) * con_scalefactor;
	INT32 charheight = 8 * con_scalefactor; //(fontv[HU_FONT].font['A'-HU_FONTSTART]->height) * con_scalefactor;
	if (teamtalk)
	{
		talk = ttalk;
#if 0
		if (players[consoleplayer].ctfteam == 1)
			t = 0x500;  // Red
		else if (players[consoleplayer].ctfteam == 2)
			t = 0x400; // Blue
#endif
	}

	while (talk[i])
	{
		if (talk[i] < HU_FONTSTART)
		{
			++i;
			//charwidth = 4 * con_scalefactor;
		}
		else
		{
			//charwidth = (fontv[HU_FONT].font[talk[i]-HU_FONTSTART]->width) * con_scalefactor;
			V_DrawCharacter(HU_INPUTX + c, y, talk[i++] | cv_constextsize.value | V_NOSCALESTART, true);
		}
		c += charwidth;
	}

	if ((strlen(w_chat) == 0 || c_input == 0) && hu_tick < 4)
		V_DrawCharacter(HU_INPUTX+c, y+2*con_scalefactor, '_' |cv_constextsize.value | V_NOSCALESTART|t, true);

	i = 0;
	while (w_chat[i])
	{

		if (c_input == (i+1) && hu_tick < 4)
		{
			INT32 cursorx = (HU_INPUTX+c+charwidth < vid.width) ? (HU_INPUTX + c + charwidth) : (HU_INPUTX); // we may have to go down.
			INT32 cursory = (cursorx != HU_INPUTX) ? (y) : (y+charheight);
			V_DrawCharacter(cursorx, cursory+2*con_scalefactor, '_' |cv_constextsize.value | V_NOSCALESTART|t, true);
		}

		//Hurdler: isn't it better like that?
		if (w_chat[i] < HU_FONTSTART)
		{
			++i;
			//charwidth = 4 * con_scalefactor;
		}
		else
		{
			//charwidth = (fontv[HU_FONT].font[w_chat[i]-HU_FONTSTART]->width) * con_scalefactor;
			V_DrawCharacter(HU_INPUTX + c, y, w_chat[i++] | cv_constextsize.value | V_NOSCALESTART | t, true);
		}

		c += charwidth;
		if (c >= vid.width)
		{
			c = 0;
			y += charheight;
		}
	}

	if (hu_tick < 4)
		V_DrawCharacter(HU_INPUTX + c, y, '_' | cv_constextsize.value |V_NOSCALESTART|t, true);
}
#endif

static void HU_DrawCEcho(void)
{
	INT32 i = 0;
	INT32 y = (BASEVIDHEIGHT/2)-4;
	INT32 pnumlines = 0;

	UINT32 realflags = cechoflags|V_SPLITSCREEN; // requested as part of splitscreen's stuff

	char *line;
	char *echoptr;
	char temp[1024];

	for (i = 0; cechotext[i] != '\0'; ++i)
		if (cechotext[i] == '\\')
			pnumlines++;

	y -= (pnumlines-1)*6;

	// Prevent crashing because I'm sick of this
	if (y < 0)
	{
		CONS_Alert(CONS_WARNING, "CEcho contained too many lines, not displaying\n");
		cechotimer = 0;
		return;
	}

	strcpy(temp, cechotext);
	echoptr = &temp[0];

	while (*echoptr != '\0')
	{
		line = strchr(echoptr, '\\');

		if (line == NULL)
			break;

		*line = '\0';

		V_DrawCenteredString(BASEVIDWIDTH/2, y, realflags, echoptr);
		y += 12;

		echoptr = line;
		echoptr++;
	}
}

//
// demo info stuff
//
UINT32 hu_demotime;
UINT32 hu_demolap;

static void HU_DrawDemoInfo(void)
{
	if (!multiplayer)/* netreplay */
	{
		V_DrawCenteredString((BASEVIDWIDTH/2), BASEVIDHEIGHT-40, 0, M_GetText("Replay:"));
		V_DrawCenteredString((BASEVIDWIDTH/2), BASEVIDHEIGHT-32, V_ALLOWLOWERCASE, player_names[0]);
	}
	else
	{
		V_DrawRightAlignedThinString(BASEVIDWIDTH-2, BASEVIDHEIGHT-10, V_ALLOWLOWERCASE, demo.titlename);
	}

	if (modeattacking)
	{
		V_DrawRightAlignedString((BASEVIDWIDTH/2)-4, BASEVIDHEIGHT-24, V_YELLOWMAP|V_MONOSPACE, "BEST TIME:");
		if (hu_demotime != UINT32_MAX)
			V_DrawString((BASEVIDWIDTH/2)+4, BASEVIDHEIGHT-24, V_MONOSPACE, va("%i'%02i\"%02i",
				G_TicsToMinutes(hu_demotime,true),
				G_TicsToSeconds(hu_demotime),
				G_TicsToCentiseconds(hu_demotime)));
		else
			V_DrawString((BASEVIDWIDTH/2)+4, BASEVIDHEIGHT-24, V_MONOSPACE, "--'--\"--");

		V_DrawRightAlignedString((BASEVIDWIDTH/2)-4, BASEVIDHEIGHT-16, V_YELLOWMAP|V_MONOSPACE, "BEST LAP:");
		if (hu_demolap != UINT32_MAX)
			V_DrawString((BASEVIDWIDTH/2)+4, BASEVIDHEIGHT-16, V_MONOSPACE, va("%i'%02i\"%02i",
				G_TicsToMinutes(hu_demolap,true),
				G_TicsToSeconds(hu_demolap),
				G_TicsToCentiseconds(hu_demolap)));
		else
			V_DrawString((BASEVIDWIDTH/2)+4, BASEVIDHEIGHT-16, V_MONOSPACE, "--'--\"--");
	}
}


//
// Song credits
//
void HU_DrawSongCredits(void)
{
	fixed_t x;
	fixed_t y = (r_splitscreen ? (BASEVIDHEIGHT/2)-4 : 32) * FRACUNIT;
	INT32 bgt;

	if (!cursongcredit.def || cursongcredit.trans >= NUMTRANSMAPS) // No def
	{
		return;
	}

	bgt = (NUMTRANSMAPS/2) + (cursongcredit.trans / 2);
	x = R_InterpolateFixed(cursongcredit.old_x, cursongcredit.x);

	if (bgt < NUMTRANSMAPS)
	{
		V_DrawFixedPatch(x, y - (2 * FRACUNIT),
			FRACUNIT, V_SNAPTOLEFT|(bgt<<V_ALPHASHIFT),
			songcreditbg, NULL);
	}

	V_DrawRightAlignedThinStringAtFixed(x, y,
		V_ALLOWLOWERCASE|V_6WIDTHSPACE|V_SNAPTOLEFT|(cursongcredit.trans<<V_ALPHASHIFT),
		cursongcredit.text);
}


// Heads up displays drawer, call each frame
//
void HU_Drawer(void)
{
	if (cv_vhseffect.value && (paused || (demo.playback && cv_playbackspeed.value > 1)))
		V_DrawVhsEffect(demo.rewinding);

#ifndef NONET
	// draw chat string plus cursor
	if (chat_on)
	{
		if (!OLDCHAT)
			HU_DrawChat();
		else
			HU_DrawChat_Old();
	}
	else
	{
		typelines = 1;
		chat_scrolltime = 0;

		if (!OLDCHAT && cv_consolechat.value < 2 && netgame) // Don't display minimized chat if you set the mode to Window (Hidden)
			HU_drawMiniChat(); // draw messages in a cool fashion.
	}
#endif

	if (cechotimer)
		HU_DrawCEcho();

	if (!( Playing() || demo.playback )
	 || gamestate == GS_INTERMISSION || gamestate == GS_CUTSCENE
	 || gamestate == GS_CREDITS      || gamestate == GS_EVALUATION
	 || gamestate == GS_ENDING       || gamestate == GS_GAMEEND
	 || gamestate == GS_VOTING || gamestate == GS_WAITINGPLAYERS) // SRB2kart
		return;

	// draw multiplayer rankings
	if (hu_showscores)
	{
		if (netgame || multiplayer)
		{
			if (LUA_HudEnabled(hud_rankings))
				HU_DrawRankings();
			if (renderisnewtic)
			{
				LUA_HUD_ClearDrawList(luahuddrawlist_scores);
				LUA_HookHUD(luahuddrawlist_scores, HUD_HOOK(scores));
			}
			LUA_HUD_DrawList(luahuddrawlist_scores);
		}

		if (demo.playback)
		{
			HU_DrawDemoInfo();
		}
	}

	if (gamestate != GS_LEVEL)
		return;

	// draw song credits
	if (cv_songcredits.value && !( hu_showscores && (netgame || multiplayer) ))
		HU_DrawSongCredits();

	// draw desynch text
	if (hu_redownloadinggamestate)
	{
		char resynch_text[14];
		UINT32 i;

		// Animate the dots
		strcpy(resynch_text, "Resynching");
		for (i = 0; i < (resynch_ticker / 16) % 4; i++)
			strcat(resynch_text, ".");

		V_DrawCenteredString(BASEVIDWIDTH/2, 180, V_YELLOWMAP | V_ALLOWLOWERCASE, resynch_text);
	}

	if (modeattacking && pausedelay > 0 && !pausebreakkey)
	{
		INT32 strength = ((pausedelay - 1 - NEWTICRATE/2)*10)/(NEWTICRATE/3);
		INT32 x = BASEVIDWIDTH/2, y = BASEVIDHEIGHT/2; // obviously incorrect values while we scrap hudinfo

		V_DrawThinString(x, y,
			((leveltime & 4) ? V_SKYMAP : V_BLUEMAP),
			"HOLD TO RETRY...");

		if (strength > 9)
			V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 0);
		else if (strength > 0)
			V_DrawFadeScreen(0, strength);
	}
}

//======================================================================
//                 HUD MESSAGES CLEARING FROM SCREEN
//======================================================================

// Clear old messages from the borders around the view window
// (only for reduced view, refresh the borders when needed)
//
// startline: y coord to start clear,
// clearlines: how many lines to clear.
//
static INT32 oldclearlines;

void HU_Erase(void)
{
	INT32 topline, bottomline;
	INT32 y, yoffset;

#ifdef HWRENDER
	// clear hud msgs on double buffer (OpenGL mode)
	boolean secondframe;
	static INT32 secondframelines;
#endif

	if (con_clearlines == oldclearlines && !con_hudupdate && !chat_on)
		return;

#ifdef HWRENDER
	// clear the other frame in double-buffer modes
	secondframe = (con_clearlines != oldclearlines);
	if (secondframe)
		secondframelines = oldclearlines;
#endif

	// clear the message lines that go away, so use _oldclearlines_
	bottomline = oldclearlines;
	oldclearlines = con_clearlines;
	if (chat_on && OLDCHAT)
		if (bottomline < 8)
			bottomline = 8; // only do it for consolechat. consolechat is gay.

	if (automapactive || viewwindowx == 0) // hud msgs don't need to be cleared
		return;

	// software mode copies view border pattern & beveled edges from the backbuffer
	if (rendermode == render_soft)
	{
		topline = 0;
		for (y = topline, yoffset = y*vid.width; y < bottomline; y++, yoffset += vid.width)
		{
			if (y < viewwindowy || y >= viewwindowy + viewheight)
				R_VideoErase(yoffset, vid.width); // erase entire line
			else
			{
				R_VideoErase(yoffset, viewwindowx); // erase left border
				// erase right border
				R_VideoErase(yoffset + viewwindowx + viewwidth, viewwindowx);
			}
		}
		con_hudupdate = false; // if it was set..
	}
#ifdef HWRENDER
	else if (rendermode != render_none)
	{
		// refresh just what is needed from the view borders
		HWR_DrawViewBorder(secondframelines);
		con_hudupdate = secondframe;
	}
#endif
}

//======================================================================
//                   IN-LEVEL MULTIPLAYER RANKINGS
//======================================================================

static int
Ping_gfx_num (int lag)
{
	if (lag <= 2)
		return 0;
	else if (lag <= 4)
		return 1;
	else if (lag <= 7)
		return 2;
	else if (lag <= 10)
		return 3;
	else
		return 4;
}

static int
Ping_gfx_color (int lag)
{
	if (lag <= 2)
		return SKINCOLOR_JAWZ;
	else if (lag <= 4)
		return SKINCOLOR_MINT;
	else if (lag <= 7)
		return SKINCOLOR_GOLD;
	else if (lag <= 10)
		return SKINCOLOR_RASPBERRY;
	else
		return SKINCOLOR_MAGENTA;
}

//
// HU_drawPing
//
void HU_drawPing(INT32 x, INT32 y, UINT32 lag, INT32 flags, boolean offline)
{
	UINT8 *colormap = NULL;
	INT32 measureid = cv_pingmeasurement.value ? 1 : 0;
	INT32 gfxnum; // gfx to draw
	boolean drawlocal = (offline && cv_mindelay.value && lag <= (tic_t)cv_mindelay.value);

	if (!server && lag <= (tic_t)cv_mindelay.value)
	{
		lag = cv_mindelay.value;
		drawlocal = true;
	}

	gfxnum = Ping_gfx_num(lag);

	if (measureid == 1)
		V_DrawScaledPatch(x+11 - pingmeasure[measureid]->width, y+9, flags, pingmeasure[measureid]);

	if (drawlocal)
		V_DrawScaledPatch(x+2, y, flags, pinglocal[0]);
	else
		V_DrawScaledPatch(x+2, y, flags, pinggfx[gfxnum]);

	colormap = R_GetTranslationColormap(TC_RAINBOW, Ping_gfx_color(lag), GTC_CACHE);

	if (servermaxping && lag > servermaxping && hu_tick < 4)
	{
		// flash ping red if too high
		colormap = R_GetTranslationColormap(TC_RAINBOW, SKINCOLOR_WHITE, GTC_CACHE);
	}

	if (cv_pingmeasurement.value)
	{
		lag = (INT32)(lag * (1000.00f / TICRATE));
	}

	x = V_DrawPingNum(x + (measureid == 1 ? 11 - pingmeasure[measureid]->width : 10), y+9, flags, lag, colormap);

	if (measureid == 0)
		V_DrawScaledPatch(x+1 - pingmeasure[measureid]->width, y+9, flags, pingmeasure[measureid]);
}

void
HU_drawMiniPing (INT32 x, INT32 y, UINT32 lag, INT32 flags)
{
	patch_t *patch;
	INT32 w = BASEVIDWIDTH;

	if (r_splitscreen > 1)
	{
		w /= 2;
	}

	// This looks kinda dumb, but basically:
	// Servers with mindelay set modify the ping table.
	// Clients with mindelay unset don't, because they can't.
	// Both are affected by mindelay, but a client's lag value is pre-adjustment.
	if (server && cv_mindelay.value && (tic_t)cv_mindelay.value <= lag)
		patch = pinglocal[1];
	else if (!server && cv_mindelay.value && (tic_t)cv_mindelay.value >= lag)
		patch = pinglocal[1];
	else
		patch = mping[Ping_gfx_num(lag)];

	if (( flags & V_SNAPTORIGHT ))
		x += ( w - SHORT (patch->width) );

	V_DrawScaledPatch(x, y, flags, patch);
}

//
// HU_DrawSpectatorTicker
//
static inline void HU_DrawSpectatorTicker(void)
{
	INT32 i;
	INT32 length = 0, height = 174;
	INT32 totallength = 0, templength = -8;
	INT32 dupadjust = (vid.width/vid.dupx), duptweak = (dupadjust - BASEVIDWIDTH)/2;

	for (i = 0; i < MAXPLAYERS; i++)
		if (playeringame[i] && players[i].spectator)
			totallength += (signed)strlen(player_names[i]) * 8 + 16;

	length -= (leveltime % (totallength + dupadjust+8));
	length += dupadjust;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (playeringame[i] && players[i].spectator)
		{
			char *pos;
			char initial[MAXPLAYERNAME+1];
			char current[MAXPLAYERNAME+1];
			INT32 len;

			len = ((signed)strlen(player_names[i]) * 8 + 16);

			strcpy(initial, player_names[i]);
			pos = initial;

			if (length >= -len)
			{
				if (length < -8)
				{
					UINT8 eatenchars = (UINT8)(abs(length) / 8);

					if (eatenchars <= strlen(initial))
					{
						// Eat one letter off the left side,
						// then compensate the drawing position.
						pos += eatenchars;
						strcpy(current, pos);
						templength = ((length + 8) % 8);
					}
					else
					{
						strcpy(current, " ");
						templength = length;
					}
				}
				else
				{
					strcpy(current, initial);
					templength = length;
				}

				V_DrawString(templength - duptweak, height, V_TRANSLUCENT|V_ALLOWLOWERCASE, current);
			}

			if ((length += len) >= dupadjust+8)
				break;
		}
	}
}

//
// HU_DrawRankings
//
static void HU_DrawRankings(void)
{
	playersort_t tab[MAXPLAYERS];
	INT32 i, j, scorelines, hilicol, numplayersingame = 0;
	boolean completed[MAXPLAYERS];
	UINT32 whiteplayer = MAXPLAYERS;

	V_DrawFadeScreen(0xFF00, 16); // A little more readable, and prevents cheating the fades under other circumstances.

	if (cons_menuhighlight.value)
		hilicol = cons_menuhighlight.value;
	else if (modeattacking)
		hilicol = V_ORANGEMAP;
	else
		hilicol = ((gametype == GT_RACE) ? V_SKYMAP : V_REDMAP);

	// draw the current gametype in the lower right
	if (modeattacking)
		V_DrawString(4, 188, hilicol|V_SNAPTOBOTTOM|V_SNAPTOLEFT, "Record Attack");
	else
		V_DrawString(4, 188, hilicol|V_SNAPTOBOTTOM|V_SNAPTOLEFT, gametype_cons_t[gametype].strvalue);

	if ((gametyperules & (GTR_TIMELIMIT|GTR_POINTLIMIT)) && !bossinfo.boss)
	{
		if ((gametyperules & GTR_TIMELIMIT) && cv_timelimit.value && timelimitintics > 0)
		{
			UINT32 timeval = (timelimitintics + starttime + 1 - leveltime);
			if (timeval > timelimitintics+1)
				timeval = timelimitintics+1;
			timeval /= TICRATE;

			if (leveltime <= (timelimitintics + starttime))
			{
				V_DrawCenteredString(64, 8, 0, "TIME LEFT");
				V_DrawCenteredString(64, 16, hilicol, va("%u", timeval));
			}

			// overtime
			if (!players[consoleplayer].exiting && (leveltime > (timelimitintics + starttime + TICRATE/2)) && cv_overtime.value)
			{
				V_DrawCenteredString(64, 8, 0, "TIME LEFT");
				V_DrawCenteredString(64, 16, hilicol, "OVERTIME");
			}
		}

		if ((gametyperules & GTR_POINTLIMIT) && cv_pointlimit.value > 0)
		{
			V_DrawCenteredString(256, 8, 0, "POINT LIMIT");
			V_DrawCenteredString(256, 16, hilicol, va("%d", cv_pointlimit.value));
		}
	}
	else
	{
		if (circuitmap)
		{
			V_DrawCenteredString(64, 8, 0, "LAP COUNT");
			V_DrawCenteredString(64, 16, hilicol, va("%d", numlaps));
		}

		V_DrawCenteredString(256, 8, 0, "GAME SPEED");
		V_DrawCenteredString(256, 16, hilicol, kartspeed_cons_t[1+gamespeed].strvalue);
	}

	// When you play, you quickly see your score because your name is displayed in white.
	// When playing back a demo, you quickly see who's the view.
	if (!r_splitscreen)
		whiteplayer = demo.playback ? displayplayers[0] : consoleplayer;

	scorelines = 0;
	memset(completed, 0, sizeof (completed));
	memset(tab, 0, sizeof (playersort_t)*MAXPLAYERS);

	for (i = 0; i < MAXPLAYERS; i++)
	{
		tab[i].num = -1;
		tab[i].name = NULL;
		tab[i].count = INT32_MAX;

		if (!playeringame[i] || players[i].spectator || !players[i].mo)
			continue;

		numplayersingame++;
	}

	for (j = 0; j < numplayersingame; j++)
	{
		UINT8 lowestposition = MAXPLAYERS+1;
		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (completed[i] || !playeringame[i] || players[i].spectator || !players[i].mo)
				continue;

			if (players[i].position >= lowestposition)
				continue;

			tab[scorelines].num = i;
			lowestposition = players[i].position;
		}

		i = tab[scorelines].num;

		completed[i] = true;

		tab[scorelines].name = player_names[i];

		if ((gametyperules & GTR_CIRCUIT))
		{
			if (circuitmap)
				tab[scorelines].count = players[i].laps;
			else
				tab[scorelines].count = players[i].realtime;
		}
		else
			tab[scorelines].count = players[i].roundscore;

		scorelines++;

#if MAXPLAYERS > 16
	if (scorelines > 16)
		break; //dont draw past bottom of screen, show the best only
#endif
	}

	K_DrawTabRankings(((scorelines > 8) ? 32 : 40), 33, tab, scorelines, whiteplayer, hilicol);

	// draw spectators in a ticker across the bottom
	if (netgame && G_GametypeHasSpectators())
		HU_DrawSpectatorTicker();
}

// Interface to CECHO settings for the outside world, avoiding the
// expense (and security problems) of going via the console buffer.
void HU_ClearCEcho(void)
{
	cechotimer = 0;
}

void HU_SetCEchoDuration(INT32 seconds)
{
	cechoduration = seconds * TICRATE;
}

void HU_SetCEchoFlags(INT32 flags)
{
	// Don't allow cechoflags to contain any bits in V_PARAMMASK
	cechoflags = (flags & ~V_PARAMMASK);
}

void HU_DoCEcho(const char *msg)
{
	I_OutputMsg("%s\n", msg); // print to log

	strncpy(cechotext, msg, sizeof(cechotext));
	strncat(cechotext, "\\", sizeof(cechotext) - strlen(cechotext) - 1);
	cechotext[sizeof(cechotext) - 1] = '\0';
	cechotimer = cechoduration;
}
