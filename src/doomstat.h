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
/// \file  doomstat.h
/// \brief All the global variables that store the internal state.
///
///        Theoretically speaking, the internal state of the engine
///        should be found by looking at the variables collected
///        here, and every relevant module will have to include
///        this header file. In practice... things are a bit messy.

#ifndef __DOOMSTAT__
#define __DOOMSTAT__

// We need globally shared data structures, for defining the global state variables.
#include "doomdata.h"

// We need the player data structure as well.
#include "d_player.h"

// =============================
// Selected map etc.
// =============================

// Selected by user.
extern INT16 gamemap;
extern char mapmusname[7];
extern UINT16 mapmusflags;
extern UINT32 mapmusposition;
extern UINT32 mapmusresume;
extern UINT8 mapmusrng;
#define MUSIC_TRACKMASK   0x0FFF // ----************
#define MUSIC_RELOADRESET 0x8000 // *---------------
#define MUSIC_FORCERESET  0x4000 // -*--------------
// Use other bits if necessary.

extern UINT32 maptol;

extern INT32 cursaveslot;
//extern INT16 lastmapsaved;
extern INT16 lastmaploaded;
extern UINT8 gamecomplete;

// Extra abilities/settings for skins (combinable stuff)
typedef enum
{
	MA_RUNNING     = 1,    // In action
	MA_INIT        = 1<<1, // Initialisation
	MA_NOCUTSCENES = 1<<2, // No cutscenes
	MA_INGAME      = 1<<3  // Timer ignores loads
} marathonmode_t;

extern marathonmode_t marathonmode;
extern tic_t marathontime;

#define maxgameovers 13
extern UINT8 numgameovers;
extern SINT8 startinglivesbalance[maxgameovers+1];

#define NUMPRECIPFREESLOTS 64

typedef enum
{
	PRECIP_NONE = 0,

	PRECIP_RAIN,
	PRECIP_SNOW,
	PRECIP_BLIZZARD,
	PRECIP_STORM,
	PRECIP_STORM_NORAIN,
	PRECIP_STORM_NOSTRIKES,

	PRECIP_FIRSTFREESLOT,
	PRECIP_LASTFREESLOT = PRECIP_FIRSTFREESLOT + NUMPRECIPFREESLOTS - 1,

	MAXPRECIP
} preciptype_t;

typedef enum
{
	PRECIPFX_THUNDER = 1,
	PRECIPFX_LIGHTNING = 1<<1,
	PRECIPFX_WATERPARTICLES = 1<<2
} precipeffect_t;

typedef struct
{
	const char *name;
	mobjtype_t type;
	precipeffect_t effects;
} precipprops_t;

extern precipprops_t precipprops[MAXPRECIP];
extern preciptype_t precip_freeslot;

extern preciptype_t globalweather;
extern preciptype_t curWeather;

// Set if homebrew PWAD stuff has been added.
extern boolean modifiedgame;
extern boolean majormods;
extern UINT16 mainwads;
extern boolean savemoddata; // This mod saves time/emblem data.
extern boolean imcontinuing; // Temporary flag while continuing
extern boolean metalrecording;

#define ATTACKING_NONE     0
#define ATTACKING_TIME     1
#define ATTACKING_CAPSULES 2
extern UINT8 modeattacking;

// menu demo things
extern UINT8  numDemos;
extern UINT32 demoDelayTime;
extern UINT32 demoIdleTime;

// Netgame? only true in a netgame
extern boolean netgame;
extern boolean addedtogame; // true after the server has added you
// Only true if >1 player. netgame => multiplayer but not (multiplayer=>netgame)
extern boolean multiplayer;

extern INT16 gametype;

extern UINT32 gametyperules;
extern INT16 gametypecount;

extern UINT8 splitscreen;
extern int r_splitscreen;

extern boolean circuitmap; // Does this level have 'circuit mode'?
extern boolean fromlevelselect;
extern boolean forceresetplayers, deferencoremode;

// ========================================
// Internal parameters for sound rendering.
// ========================================

extern boolean sound_disabled;
extern boolean digital_disabled;

// =========================
// Status flags for refresh.
// =========================
//

extern boolean menuactive; // Menu overlaid?
extern UINT8 paused; // Game paused?
extern UINT8 window_notinfocus; // are we in focus? (backend independant -- handles auto pausing and display of "focus lost" message)
extern INT32 window_x;
extern INT32 window_y;

extern boolean nodrawers;
extern boolean noblit;
extern boolean lastdraw;
extern postimg_t postimgtype[MAXSPLITSCREENPLAYERS];
extern INT32 postimgparam[MAXSPLITSCREENPLAYERS];

extern INT32 viewwindowx, viewwindowy;
extern INT32 viewwidth, scaledviewwidth;

extern boolean gamedataloaded;

// Player taking events, and displaying.
extern INT32 consoleplayer;
extern INT32 displayplayers[MAXSPLITSCREENPLAYERS];
/* g_localplayers[0] = consoleplayer */
extern INT32 g_localplayers[MAXSPLITSCREENPLAYERS];

/* spitscreen players sync */
extern INT32 splitscreen_original_party_size[MAXPLAYERS];
extern INT32 splitscreen_original_party[MAXPLAYERS][MAXSPLITSCREENPLAYERS];

/* parties */
extern INT32 splitscreen_invitations[MAXPLAYERS];
extern INT32 splitscreen_party_size[MAXPLAYERS];
extern INT32 splitscreen_party[MAXPLAYERS][MAXSPLITSCREENPLAYERS];

/* the only local one */
extern boolean splitscreen_partied[MAXPLAYERS];

// Maps of special importance
extern INT16 spstage_start, spmarathon_start;
extern INT16 sstage_start, sstage_end, smpstage_start, smpstage_end;

extern INT16 titlemap;
extern boolean hidetitlepics;
extern INT16 bootmap; //bootmap for loading a map on startup

extern INT16 tutorialmap; // map to load for tutorial
extern boolean tutorialmode; // are we in a tutorial right now?
extern INT32 tutorialgcs; // which control scheme is loaded?

extern boolean looptitle;

// CTF colors.
extern UINT16 skincolor_redteam, skincolor_blueteam, skincolor_redring, skincolor_bluering;

extern tic_t countdowntimer;
extern boolean countdowntimeup;
extern boolean exitfadestarted;

typedef struct
{
	UINT8 numpics;
	char picname[8][8];
	UINT8 pichires[8];
	char *text;
	UINT16 xcoord[8];
	UINT16 ycoord[8];
	UINT16 picduration[8];
	UINT8 musicloop;
	UINT16 textxpos;
	UINT16 textypos;

	char   musswitch[7];
	UINT16 musswitchflags;
	UINT32 musswitchposition;

	UINT8 fadecolor; // Color number for fade, 0 means don't do the first fade
	UINT8 fadeinid;  // ID of the first fade, to a color -- ignored if fadecolor is 0
	UINT8 fadeoutid; // ID of the second fade, to the new screen
} scene_t; // TODO: It would probably behoove us to implement subsong/track selection here, too, but I'm lazy -SH

typedef struct
{
	scene_t scene[128]; // 128 scenes per cutscene.
	INT32 numscenes; // Number of scenes in this cutscene
} cutscene_t;

extern cutscene_t *cutscenes[128];

// Reserve prompt space for tutorials
#define TUTORIAL_PROMPT 201 // one-based
#define TUTORIAL_AREAS 6
#define TUTORIAL_AREA_PROMPTS 5
#define MAX_PROMPTS (TUTORIAL_PROMPT+TUTORIAL_AREAS*TUTORIAL_AREA_PROMPTS*3) // 3 control modes
#define MAX_PAGES 128

#define PROMPT_PIC_PERSIST 0
#define PROMPT_PIC_LOOP 1
#define PROMPT_PIC_DESTROY 2
#define MAX_PROMPT_PICS 8
typedef struct
{
	UINT8 numpics;
	UINT8 picmode; // sequence mode after displaying last pic, 0 = persist, 1 = loop, 2 = destroy
	UINT8 pictoloop; // if picmode == loop, which pic to loop to?
	UINT8 pictostart; // initial pic number to show
	char picname[MAX_PROMPT_PICS][8];
	UINT8 pichires[MAX_PROMPT_PICS];
	UINT16 xcoord[MAX_PROMPT_PICS]; // gfx
	UINT16 ycoord[MAX_PROMPT_PICS]; // gfx
	UINT16 picduration[MAX_PROMPT_PICS];

	char   musswitch[7];
	UINT16 musswitchflags;
	UINT8 musicloop;

	char tag[33]; // page tag
	char name[34]; // narrator name, extra char for color
	char iconname[8]; // narrator icon lump
	boolean rightside; // narrator side, false = left, true = right
	boolean iconflip; // narrator flip icon horizontally
	UINT8 hidehud; // hide hud, 0 = show all, 1 = hide depending on prompt position (top/bottom), 2 = hide all
	UINT8 lines; // # of lines to show. If name is specified, name takes one of the lines. If 0, defaults to 4.
	INT32 backcolor; // see CON_SetupBackColormap: 0-11, INT32_MAX for user-defined (CONS_BACKCOLOR)
	UINT8 align; // text alignment, 0 = left, 1 = right, 2 = center
	UINT8 verticalalign; // vertical text alignment, 0 = top, 1 = bottom, 2 = middle
	UINT8 textspeed; // text speed, delay in tics between characters.
	sfxenum_t textsfx; // sfx_ id for printing text
	UINT8 nextprompt; // next prompt to jump to, one-based. 0 = current prompt
	UINT8 nextpage; // next page to jump to, one-based. 0 = next page within prompt->numpages
	char nexttag[33]; // next tag to jump to. If set, this overrides nextprompt and nextpage.
	INT32 timetonext; // time in tics to jump to next page automatically. 0 = don't jump automatically
	char *text;
} textpage_t;

typedef struct
{
	textpage_t page[MAX_PAGES];
	INT32 numpages; // Number of pages in this prompt
} textprompt_t;

extern textprompt_t *textprompts[MAX_PROMPTS];

// For the Custom Exit linedef.
extern INT16 nextmapoverride;
extern UINT8 skipstats;

extern UINT32 ssspheres; //  Total # of spheres in a level

// Fun extra stuff
extern INT16 lastmap; // Last level you were at (returning from special stages).
extern mobj_t *redflag, *blueflag; // Pointers to physical flags
extern mapthing_t *rflagpoint, *bflagpoint; // Pointers to the flag spawn locations
#define GF_REDFLAG 1
#define GF_BLUEFLAG 2

// A single point in space.
typedef struct
{
	fixed_t x, y, z;
} mappoint_t;

extern struct quake
{
	// camera offsets and duration
	fixed_t x,y,z;
	UINT16 time;

	// location, radius, and intensity...
	mappoint_t *epicenter;
	fixed_t radius, intensity;
} quake;

// NiGHTS grades
typedef struct
{
	UINT32 grade[6]; // D, C, B, A, S, X (F: failed to reach any of these)
} nightsgrades_t;

// Custom Lua values
// (This is not ifdeffed so the map header structure can stay identical, just in case.)
typedef struct
{
	char option[32]; // 31 usable characters
	char value[256]; // 255 usable characters. If this seriously isn't enough then wtf.
} customoption_t;

typedef struct 
{
	UINT8 light_contrast;				///< Range of wall lighting. 0 is no lighting.
	SINT8 sprite_backlight;				///< Subtract from wall lighting for sprites only.
	boolean use_light_angle;			///< When false, wall lighting is evenly distributed. When true, wall lighting is directional.
	angle_t light_angle;				///< Angle of directional wall lighting.
} mapheader_lighting_t;

#define MAXMUSNAMES 3 // maximum definable music tracks per level

/** Map header information.
  */
typedef struct
{
	// The original eight, plus one.
	char lvlttl[22];       ///< Level name without "Zone". (21 character limit instead of 32, 21 characters can display on screen max anyway)
	char subttl[33];       ///< Subtitle for level
	char zonttl[22];       ///< "ZONE" replacement name
	char actnum[3];        ///< SRB2Kart: Now a 2 character long string.
	UINT32 typeoflevel;    ///< Combination of typeoflevel flags.
	INT16 nextlevel;       ///< Map number of next level, or 1100-1102 to end.
	INT16 marathonnext;    ///< See nextlevel, but for Marathon mode. Necessary to support hub worlds ala SUGOI.
	char keywords[33];     ///< Keywords separated by space to search for. 32 characters.
	char forcecharacter[17];  ///< (SKINNAMESIZE+1) Skin to switch to or "" to disable.
	UINT8 weather;         ///< 0 = sunny day, 1 = storm, 2 = snow, 3 = rain, 4 = blank, 5 = thunder w/o rain, 6 = rain w/o lightning, 7 = heat wave.
	char skytexture[9];    ///< Sky texture to use.
	INT16 skybox_scalex;   ///< Skybox X axis scale. (0 = no movement, 1 = 1:1 movement, 16 = 16:1 slow movement, -4 = 1:4 fast movement, etc.)
	INT16 skybox_scaley;   ///< Skybox Y axis scale.
	INT16 skybox_scalez;   ///< Skybox Z axis scale.

	// Extra information.
	char interscreen[8];  ///< 320x200 patch to display at intermission.
	char runsoc[33];      ///< SOC to execute at start of level (32 character limit instead of 63)
	char scriptname[33];  ///< Script to use when the map is switched to. (32 character limit instead of 191)
	UINT8 precutscenenum; ///< Cutscene number to play BEFORE a level starts.
	UINT8 cutscenenum;    ///< Cutscene number to use, 0 for none.
	INT16 countdown;      ///< Countdown until level end?
	UINT16 palette;       ///< PAL lump to use on this map
	UINT16 encorepal;     ///< PAL for encore mode
	UINT8 numlaps;        ///< Number of laps in circuit mode, unless overridden.
	SINT8 unlockrequired; ///< Is an unlockable required to play this level? -1 if no.
	UINT8 levelselect;    ///< Is this map available in the level select? If so, which map list is it available in?
	SINT8 bonustype;      ///< What type of bonus does this level have? (-1 for null.)
	SINT8 maxbonuslives;  ///< How many bonus lives to award at Intermission? (-1 for unlimited.)

	UINT16 levelflags;     ///< LF_flags:  merged booleans into one UINT16 for space, see below
	UINT8 menuflags;      ///< LF2_flags: options that affect record attack / nights mode menus

	char selectheading[22]; ///< Level select heading. Allows for controllable grouping.
	UINT16 startrings;      ///< Number of rings players start with.
	INT32 sstimer;          ///< Timer for special stages.
	UINT32 ssspheres;       ///< Sphere requirement in special stages.
	fixed_t gravity;        ///< Map-wide gravity.

	// Title card.
	char ltzzpatch[8];      ///< Zig zag patch.
	char ltzztext[8];       ///< Zig zag text.
	char ltactdiamond[8];   ///< Act diamond.

	// Freed animals stuff.
	UINT8 numFlickies;     ///< Internal. For freed flicky support.
	mobjtype_t *flickies;  ///< List of freeable flickies in this level. Allocated dynamically for space reasons. Be careful.

	// NiGHTS stuff.
	UINT8 numGradedMares;   ///< Internal. For grade support.
	nightsgrades_t *grades; ///< NiGHTS grades. Allocated dynamically for space reasons. Be careful.

	// SRB2kart
	fixed_t mobj_scale; ///< Replacement for TOL_ERZ3
	fixed_t default_waypoint_radius; ///< 0 is a special value for DEFAULT_WAYPOINT_RADIUS, but scaled with mobjscale

	mapheader_lighting_t lighting;			///< Wall and sprite lighting
	mapheader_lighting_t lighting_encore;	///< Alternative lighting for Encore mode
	boolean use_encore_lighting;			///< Whether to use separate Encore lighting

	// Music stuff.
	UINT32 musinterfadeout;  ///< Fade out level music on intermission screen in milliseconds
	char musintername[7];    ///< Intermission screen music.
	// Music information
	char musname[MAXMUSNAMES][7];		///< Music tracks to play. First dimension is the track number, second is the music string. "" for no music.
	UINT16 mustrack;					///< Subsong to play. Only really relevant for music modules and specific formats supported by GME. 0 to ignore.
	UINT32 muspos;						///< Music position to jump to.
	UINT8 musname_size;					///< Number of music tracks defined

	char muspostbossname[7];    ///< Post-bossdeath music.
	UINT16 muspostbosstrack;    ///< Post-bossdeath track.
	UINT32 muspostbosspos;      ///< Post-bossdeath position
	UINT32 muspostbossfadein;   ///< Post-bossdeath fade-in milliseconds.

	SINT8 musforcereset; ///< Force resetmusic (-1 for default; 0 for force off; 1 for force on)

	// SRB2Kart: Keeps track of if a map lump exists, so we can tell when a map is being replaced.
	boolean alreadyExists;

	// Lua stuff.
	// (This is not ifdeffed so the map header structure can stay identical, just in case.)
	UINT8 numCustomOptions;     ///< Internal. For Lua custom value support.
	customoption_t *customopts; ///< Custom options. Allocated dynamically for space reasons. Be careful.
} mapheader_t;

// level flags
#define LF_SCRIPTISFILE       (1<<0) ///< True if the script is a file, not a lump.
#define LF_NOZONE             (1<<1) ///< Don't include "ZONE" on level title
#define LF_SECTIONRACE        (1<<2) ///< Section race level
#define LF_SUBTRACTNUM        (1<<3) ///< Use subtractive position number (for bright levels)

#define LF2_HIDEINMENU    (1<<0) ///< Hide in the multiplayer menu
#define LF2_HIDEINSTATS   (1<<1) ///< Hide in the statistics screen
#define LF2_NOTIMEATTACK  (1<<2) ///< Hide this map in Time Attack modes
#define LF2_VISITNEEDED   (1<<3) ///< Not available in Time Attack modes until you visit the level

extern mapheader_t* mapheaderinfo[NUMMAPS];

// This could support more, but is that a good idea?
// Keep in mind that it may encourage people making overly long cups just because they "can", and would be a waste of memory.
#define MAXLEVELLIST 5

typedef struct cupheader_s
{
	UINT16 id;                     ///< Cup ID
	char name[15];                 ///< Cup title (14 chars)
	char icon[9];                  ///< Name of the icon patch
	INT16 levellist[MAXLEVELLIST]; ///< List of levels that belong to this cup
	UINT8 numlevels;               ///< Number of levels defined in levellist
	INT16 bonusgame;               ///< Map number to use for bonus game
	INT16 specialstage;            ///< Map number to use for special stage
	UINT8 emeraldnum;              ///< ID of Emerald to use for special stage (1-7 for Chaos Emeralds, 8-14 for Super Emeralds, 0 for no emerald)
	SINT8 unlockrequired;          ///< An unlockable is required to select this cup. -1 for no unlocking required.
	struct cupheader_s *next;      ///< Next cup in linked list
} cupheader_t;

extern cupheader_t *kartcupheaders; // Start of cup linked list
extern UINT16 numkartcupheaders;

// Gametypes
#define NUMGAMETYPEFREESLOTS 128

enum GameType
{
	GT_RACE = 0,
	GT_BATTLE,

	GT_FIRSTFREESLOT,
	GT_LASTFREESLOT = GT_FIRSTFREESLOT + NUMGAMETYPEFREESLOTS - 1,
	NUMGAMETYPES
};
// If you alter this list, update deh_tables.c, MISC_ChangeGameTypeMenu in m_menu.c, and Gametype_Names in g_game.c

// Gametype rules
enum GameTypeRules
{
	// Race rules
	GTR_CIRCUIT				= 1,     // Enables the finish line, laps, and the waypoint system.
	GTR_BOTS				= 1<<2,  // Allows bots in this gametype. Combine with BotTiccmd hooks to make bots support your gametype.

	// Battle gametype rules
	GTR_BUMPERS				= 1<<3,  // Enables the bumper health system
	GTR_SPHERES				= 1<<4,  // Replaces rings with blue spheres
	GTR_PAPERITEMS			= 1<<5,  // Replaces item boxes with paper item spawners
	GTR_WANTED				= 1<<6,  // unused
	GTR_KARMA				= 1<<7,  // Enables the Karma system if you're out of bumpers
	GTR_ITEMARROWS			= 1<<8,  // Show item box arrows above players
	GTR_CAPSULES			= 1<<9,  // Enables the wanted anti-camping system
	GTR_BATTLESTARTS		= 1<<10, // Use Battle Mode start positions.

	GTR_POINTLIMIT			= 1<<11,  // Reaching point limit ends the round
	GTR_TIMELIMIT			= 1<<12, // Reaching time limit ends the round
	GTR_OVERTIME			= 1<<13, // Allow overtime behavior

	// Custom gametype rules
	GTR_TEAMS				= 1<<14, // Teams are forced on
	GTR_NOTEAMS				= 1<<15, // Teams are forced off
	GTR_TEAMSTARTS			= 1<<16, // Use team-based start positions

	// Grand Prix rules
	GTR_CAMPAIGN			= 1<<17, // Handles cup-based progression
	GTR_LIVES				= 1<<18, // Lives system, players are forced to spectate during Game Over.
	GTR_SPECIALBOTS			= 1<<19, // Bot difficulty gets stronger between rounds, and the rival system is enabled.
	
	// Misc
	GTR_FREEROAM			= 1<<20, // Disables Countdown timer and control lock at the start of levels.

	// free: to and including 1<<31
};

// String names for gametypes
extern const char *Gametype_Names[NUMGAMETYPES];
extern const char *Gametype_ConstantNames[NUMGAMETYPES];

// Point and time limits for every gametype
extern INT32 pointlimits[NUMGAMETYPES];
extern INT32 timelimits[NUMGAMETYPES];

// TypeOfLevel things
enum TypeOfLevel
{
	// Gametypes
	TOL_RACE	= 0x0001, ///< Race
	TOL_BATTLE	= 0x0002, ///< Battle
	TOL_BOSS	= 0x0004, ///< Boss (variant of battle, but forbidden)

	// Modifiers
	TOL_TV		= 0x0100 ///< Midnight Channel specific: draw TV like overlay on HUD
};

#define MAXTOL             (1<<31)
#define NUMBASETOLNAMES    (4)
#define NUMTOLNAMES        (NUMBASETOLNAMES + NUMGAMETYPEFREESLOTS)

typedef struct
{
	const char *name;
	UINT32 flag;
} tolinfo_t;
extern tolinfo_t TYPEOFLEVEL[NUMTOLNAMES];
extern UINT32 lastcustomtol;

extern tic_t totalplaytime;
extern UINT32 matchesplayed;

extern UINT8 stagefailed;

// Emeralds stored as bits to throw savegame hackers off.
typedef enum
{
	EMERALD_CHAOS1 = 1,
	EMERALD_CHAOS2 = 1<<1,
	EMERALD_CHAOS3 = 1<<2,
	EMERALD_CHAOS4 = 1<<3,
	EMERALD_CHAOS5 = 1<<4,
	EMERALD_CHAOS6 = 1<<5,
	EMERALD_CHAOS7 = 1<<6,
	EMERALD_ALLCHAOS = EMERALD_CHAOS1|EMERALD_CHAOS2|EMERALD_CHAOS3|EMERALD_CHAOS4|EMERALD_CHAOS5|EMERALD_CHAOS6|EMERALD_CHAOS7,

	EMERALD_SUPER1 = 1<<7,
	EMERALD_SUPER2 = 1<<8,
	EMERALD_SUPER3 = 1<<9,
	EMERALD_SUPER4 = 1<<10,
	EMERALD_SUPER5 = 1<<11,
	EMERALD_SUPER6 = 1<<12,
	EMERALD_SUPER7 = 1<<13,
	EMERALD_ALLSUPER = EMERALD_SUPER1|EMERALD_SUPER2|EMERALD_SUPER3|EMERALD_SUPER4|EMERALD_SUPER5|EMERALD_SUPER6|EMERALD_SUPER7,

	EMERALD_ALL = EMERALD_ALLCHAOS|EMERALD_ALLSUPER
} emeraldflags_t;

extern UINT16 emeralds;

#define ALLCHAOSEMERALDS(v) ((v & EMERALD_ALLCHAOS) == EMERALD_ALLCHAOS)
#define ALLSUPEREMERALDS(v) ((v & EMERALD_ALLSUPER) == EMERALD_ALLSUPER)
#define ALLEMERALDS(v) ((v & EMERALD_ALL) == EMERALD_ALL)

#define NUM_LUABANKS 16 // please only make this number go up between versions, never down. you'll break saves otherwise. also, must fit in UINT8
extern INT32 luabanks[NUM_LUABANKS];

extern INT32 nummaprings; //keep track of spawned rings/coins

/** Time attack information, currently a very small structure.
  */
typedef struct
{
	tic_t time; ///< Time in which the level was finished.
	tic_t lap;  ///< Best lap time for this level.
	//UINT32 score; ///< Score when the level was finished.
	//UINT16 rings; ///< Rings when the level was finished.
} recorddata_t;

/** Setup for one NiGHTS map.
  * These are dynamically allocated because I am insane
  */
#define GRADE_F 0
#define GRADE_E 1
#define GRADE_D 2
#define GRADE_C 3
#define GRADE_B 4
#define GRADE_A 5
#define GRADE_S 6

/*typedef struct
{
	// 8 mares, 1 overall (0)
	UINT8	nummares;
	UINT32	score[9];
	UINT8	grade[9];
	tic_t	time[9];
} nightsdata_t;*/

//extern nightsdata_t *nightsrecords[NUMMAPS];
extern recorddata_t *mainrecords[NUMMAPS];

// mapvisited is now a set of flags that says what we've done in the map.
#define MV_VISITED (1)
#define MV_BEATEN  (1<<1)
#define MV_ENCORE  (1<<2)
#define MV_MAX     (MV_VISITED|MV_BEATEN|MV_ENCORE)
#define MV_MP      ((MV_MAX+1)<<1)
extern UINT8 mapvisited[NUMMAPS];

extern UINT32 token; ///< Number of tokens collected in a level
extern UINT32 tokenlist; ///< List of tokens collected
extern boolean gottoken; ///< Did you get a token? Used for end of act
extern INT32 tokenbits; ///< Used for setting token bits
extern INT32 sstimer; ///< Time allotted in the special stage
extern UINT32 bluescore; ///< Blue Team Scores
extern UINT32 redscore;  ///< Red Team Scores

// Eliminates unnecessary searching.
extern boolean CheckForBustableBlocks;
extern boolean CheckForBouncySector;
extern boolean CheckForQuicksand;
extern boolean CheckForMarioBlocks;
extern boolean CheckForFloatBob;
extern boolean CheckForReverseGravity;

// Powerup durations
extern UINT16 invulntics;
extern UINT16 sneakertics;
extern UINT16 flashingtics;
extern UINT16 tailsflytics;
extern UINT16 underwatertics;
extern UINT16 spacetimetics;
extern UINT16 extralifetics;
extern UINT16 nightslinktics;

// SRB2kart
extern tic_t introtime;
extern tic_t starttime;

extern tic_t raceexittime;
extern tic_t battleexittime;

extern INT32 hyudorotime;
extern INT32 stealtime;
extern INT32 sneakertime;
extern INT32 itemtime;
extern INT32 bubbletime;
extern INT32 comebacktime;
extern INT32 bumptime;
extern INT32 greasetics;
extern INT32 wipeoutslowtime;
extern INT32 wantedreduce;
extern INT32 wantedfrequency;
extern INT32 flameseg;

extern UINT8 introtoplay;
extern UINT8 creditscutscene;
extern UINT8 useBlackRock;

extern UINT8 use1upSound;
extern UINT8 maxXtraLife; // Max extra lives from rings

extern mobj_t *hunt1, *hunt2, *hunt3; // Emerald hunt locations

// For racing
extern tic_t racecountdown, exitcountdown;

#define DEFAULT_GRAVITY (4*FRACUNIT/5)
extern fixed_t gravity;
extern fixed_t mapobjectscale;

extern struct maplighting
{
	UINT8 contrast;
	SINT8 backlight;
	boolean directional;
	angle_t angle;
} maplighting;

//for CTF balancing
extern INT16 autobalance;
extern INT16 teamscramble;
extern INT16 scrambleplayers[MAXPLAYERS]; //for CTF team scramble
extern INT16 scrambleteams[MAXPLAYERS]; //for CTF team scramble
extern INT16 scrambletotal; //for CTF team scramble
extern INT16 scramblecount; //for CTF team scramble

extern INT32 cheats;

// SRB2kart
extern UINT8 numlaps;
extern UINT8 gamespeed;
extern boolean franticitems;
extern boolean encoremode, prevencoremode;
extern boolean comeback;

extern SINT8 battlewanted[4];
extern tic_t wantedcalcdelay;
extern tic_t indirectitemcooldown;
extern tic_t mapreset;
extern boolean thwompsactive;
extern UINT8 lastLowestLap;
extern SINT8 spbplace;
extern boolean startedInFreePlay;

extern tic_t bombflashtimer;	// Used to avoid causing seizures if multiple mines explode close to you :)
extern boolean legitimateexit;
extern boolean comebackshowninfo;
extern tic_t curlap, bestlap;

extern INT16 votelevels[4][2];
extern SINT8 votes[MAXPLAYERS];
extern SINT8 pickedvote;

extern UINT32 timesBeaten; // # of times the game has been beaten.

// ===========================
// Internal parameters, fixed.
// ===========================
// These are set by the engine, and not changed
//  according to user inputs. Partly load from
//  WAD, partly set at startup time.

extern tic_t gametic;
#define localgametic leveltime

// Player spawn spots.
extern mapthing_t *playerstarts[MAXPLAYERS]; // Cooperative
extern mapthing_t *bluectfstarts[MAXPLAYERS]; // CTF
extern mapthing_t *redctfstarts[MAXPLAYERS]; // CTF

#define TUBEWAYPOINTSEQUENCESIZE 256
#define NUMTUBEWAYPOINTSEQUENCES 256
extern mobj_t *tubewaypoints[NUMTUBEWAYPOINTSEQUENCES][TUBEWAYPOINTSEQUENCESIZE];
extern UINT16 numtubewaypoints[NUMTUBEWAYPOINTSEQUENCES];

void P_AddTubeWaypoint(UINT8 sequence, UINT8 id, mobj_t *waypoint);
mobj_t *P_GetFirstTubeWaypoint(UINT8 sequence);
mobj_t *P_GetLastTubeWaypoint(UINT8 sequence);
mobj_t *P_GetPreviousTubeWaypoint(mobj_t *current, boolean wrap);
mobj_t *P_GetNextTubeWaypoint(mobj_t *current, boolean wrap);
mobj_t *P_GetClosestTubeWaypoint(UINT8 sequence, mobj_t *mo);
boolean P_IsDegeneratedTubeWaypointSequence(UINT8 sequence);

// =====================================
// Internal parameters, used for engine.
// =====================================

#if defined (macintosh)
#define DEBFILE(msg) I_OutputMsg(msg)
#else
#define DEBUGFILE
#ifdef DEBUGFILE
#define DEBFILE(msg) { if (debugfile) { fputs(msg, debugfile); fflush(debugfile); } }
#else
#define DEBFILE(msg) {}
#endif
#endif

#ifdef DEBUGFILE
extern FILE *debugfile;
extern INT32 debugload;
#endif

// if true, load all graphics at level load
extern boolean precache;

// wipegamestate can be set to -1
//  to force a wipe on the next draw
extern gamestate_t wipegamestate;
extern INT16 wipetypepre;
extern INT16 wipetypepost;

// debug flag to cancel adaptiveness
extern boolean singletics;

// =============
// Netgame stuff
// =============

#include "d_clisrv.h"

extern consvar_t cv_showinputjoy; // display joystick in time attack
extern consvar_t cv_forceskin; // force clients to use the server's skin
extern consvar_t cv_downloading; // allow clients to downloading WADs.
extern consvar_t cv_nettimeout; // SRB2Kart: Advanced server options menu
extern consvar_t cv_jointimeout;
extern ticcmd_t netcmds[BACKUPTICS][MAXPLAYERS];
extern INT32 serverplayer;
extern INT32 adminplayers[MAXPLAYERS];

/// \note put these in d_clisrv outright?

#endif //__DOOMSTAT__
