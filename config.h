/* See LICENSE file for copyright and license details. */

#ifndef CONFIG_H
#define CONFIG_H 1

#include <X11/Xlib.h>
#include <sys/types.h>

#include "dwm.h"

/* constants */
#define TERMINAL "alacritty"
#define SHELL "zsh"
#define SHELL_BACKUP "bash"
#define VEDITOR "dav"
#define BROWSER "$BROWSER"
#define TORRENT "qbittorrent" 
#define SCREENSHOT "scnow"
#define DISCORD "discord"

#define SIG_AUDIO     10
#define SIG_OBS       9
#define SIG_MIC       8
#define SIG_RECORDING 7

#define STRINGIFY_(n) #n
#define STRINGIFY(n) STRINGIFY_(n)
#define SIG_CMD(n) "pkill -RTMIN+" STRINGIFY(n) " dwmblocks"

/* appearance */
static char normbordercolor[]       = "#222222";
static char normfgcolor[]           = "#dad2c8";
static char normbgcolor[]           = "#222222";
static char selbordercolor[]        = "#988165";
static char selbgcolor[]            = "#988165";
static char selfgcolor[]            = "#222222";

static const unsigned int borderpx  = 5;        /* border pixel of windows */
static const Gap default_gap        = {
	.isgap = 1,
	.realgap = 17,
	.gappx = 17
};
static const unsigned int snap      = 32;       /* snap pixel */
static const int swallowfloating    = 1;        /* 1 means swallow floating windows by default */
static const int showbar            = 1;        /* 0 means no bar */
static const int topbar             = 1;        /* 0 means bottom bar */

static const char *fonts[]          = {
	"monospace:size=11:antialias=true:autohint=true",
	"Noto Color Emoji:size=11:antialias=true:autohint=true"
};
static const char dmenufont[]       = "monospace:size=11";

static const char *colors[][3]      = {
      /*               fg         bg         border   */
      [SchemeNorm] = { normfgcolor, normbgcolor, normbordercolor },
      [SchemeSel]  = { selfgcolor, selbgcolor,  selbordercolor  },
};

/* tagging */
static const char *tags[] = { "1", "2", "3", "4", "5", "6", "7", "8", "9" };

static const Rule rules[] = {
    /* xprop(1):
         *WM_CLASS(STRING) = instance, class
           *WM_NAME(STRING) = title
             */
    	/* classinstance title tags  mask    isfloating isterminal noswallow monitor */
	{ "st-256color", NULL, NULL, 0,      0,         1,         0,        1 },
	{ "Alacritty",   NULL, NULL, 0,      0,         1,         0,        1 },
	{ "resolve",     NULL, NULL, 1 << 1, 0,         0,         0,        1 },
	{ "discord",     NULL, NULL, 1 << 1, 0,         0,         0,        1 },
	{ "Audacity",    NULL, NULL, 1 << 1, 0,         0,         0,        1 },
	{ "audacity",    NULL, NULL, 1 << 1, 0,         0,         0,        1 },
	{ "kdenlive",    NULL, NULL, 1 << 1, 0,         0,         0,        1 },
	{ "Gimp",        NULL, NULL, 1 << 1, 0,         0,         0,        1 },
	{ "jamesdsp",    NULL, NULL, 1 << 7, 0,         0,         0,        1 },
	{ "obs",         NULL, NULL, 1 << 8, 0,         0,         0,        1 },
	{ "qBittorrent", NULL, NULL, 1 << 8, 0,         0,         0,        1 },
};

/* layout(s) */
static const float mfact     = 0.55; /* factor of master area size [0.05..0.95] */
static const int nmaster     = 1;    /* number of clients in master area */
static const int resizehints = 1;    /* 1 means respect size hints in tiled resizals */
static const int lockfullscreen = 1; /* 1 will force focus on the fullscreen window */

static const Layout layouts[] = {
	/* symbol     arrange function */
	{ "[]=",      tile },    /* first entry is default */
	{ "><>",      NULL },    /* no layout function means floating behavior */
	/* { "[M]",      monocle }, */
};

/* key definitions */
#define MODKEY Mod4Mask
#define ALT Mod1Mask
#define TAGKEYS(KEY,TAG) \
	{ MODKEY,                       KEY,      view,           {.ui = 1 << TAG} }, \
	{ MODKEY|ControlMask,           KEY,      toggleview,     {.ui = 1 << TAG} }, \
	{ ALT,                          KEY,      tag,            {.ui = 1 << TAG} }, \
	{ MODKEY|ShiftMask,             KEY,      tag,            {.ui = 1 << TAG} }, \
	{ MODKEY|ControlMask|ShiftMask, KEY,      toggletag,      {.ui = 1 << TAG} },

/* helper for spawning binaries */
#define CMD(cmd) { .v = (const char*[]){ cmd, NULL } }
/* helper for spawning shell commands in the pre dwm-5.0 fashion */
#define SHCMD(cmd) { .v = (const char*[]){ "/bin/sh", "-c", cmd, NULL } }

/* commands */
static char dmenumon[2] = "0"; /* component of dmenucmd, manipulated in spawn() */
static const char *dmenucmd[] = {
	"dmenu_run", "-m", dmenumon, "-fn", dmenufont, "-nb", selfgcolor, "-nf", normfgcolor, "-sb", selbgcolor, "-sf", selfgcolor, NULL
};

static const char *termcmd[]  = { TERMINAL " msg create-window || " TERMINAL , NULL };

static const Key keys[] = {
	/* modifier                     key        function        argument */
	{ MODKEY,                       XK_p,      spawn,          {.v = dmenucmd } },
	{ MODKEY,                       XK_Return, spawn,          SHCMD(TERMINAL " msg create-window -e " SHELL " || " TERMINAL " -e " SHELL)},
	/* { ALT,                          XK_Return, spawn,          SHCMD(TERMINAL " msg create-window -e " SHELL_BACKUP "|| " TERMINAL " -e " SHELL_BACKUP)}, */
	{ ALT,                          XK_Return, spawn,          CMD("st")},
	/* { MODKEY,                       XK_b,      togglebar,      {0} }, */
	{ MODKEY,                       XK_j,      focusstack,     {.i = +1 } },
	{ MODKEY,                       XK_k,      focusstack,     {.i = -1 } },
	{ MODKEY,                       XK_f,      togglefullscr,  {0} },
	{ MODKEY,                       XK_h,      setmfact,       {.f = -0.05} },
	{ MODKEY,                       XK_l,      setmfact,       {.f = +0.05} },
	{ MODKEY,                       XK_space,  zoom,           {0} },
	{ ALT,                          XK_Tab,    view,           {0} },
	{ MODKEY,                       XK_q,      killclient,     {0} },
	{ MODKEY,                       XK_space,  setlayout,      {.v = &layouts[0]} },
	{ MODKEY,                       XK_comma,  focusmon,       {.i = -1 } },
	{ MODKEY,                       XK_period, focusmon,       {.i = +1 } },
	{ ALT,                          XK_comma,  tagmon,         {.i = -1 } },
	{ ALT,                          XK_period, tagmon,         {.i = +1 } },
	{ MODKEY,                       XK_w,      spawn,          CMD("brave") },
	{ MODKEY,                       XK_s,	   spawn,          CMD("dmenusite") },
	{ MODKEY,                       XK_c,	   spawn,          CMD("dmenuclip") },
	{ ALT,                          XK_u,      spawn,          SHCMD(TERMINAL " -e paru -Syu --needed --skipreview && remaps")},
	/* { MODKEY|ShiftMask,             XK_k,      spawn,          SHCMD("pgrep 'picom' && killall picom || picom -b")}, */
	{ MODKEY,                       XK_minus,  spawn,          SHCMD("pamixer -d 1; gst-play-1.0 /usr/share/sounds/freedesktop/stereo/audio-volume-change.oga; " SIG_CMD(SIG_AUDIO))},
	{ ALT,                          XK_minus,  spawn,          SHCMD("pamixer -d 2; gst-play-1.0 /usr/share/sounds/freedesktop/stereo/audio-volume-change.oga; " SIG_CMD(SIG_AUDIO))},
	{ MODKEY,                       XK_equal,  spawn,          SHCMD("pamixer -i 1; gst-play-1.0 /usr/share/sounds/freedesktop/stereo/audio-volume-change.oga; " SIG_CMD(SIG_AUDIO))},
	{ ALT,                          XK_equal,  spawn,          SHCMD("pamixer -i 2; gst-play-1.0 /usr/share/sounds/freedesktop/stereo/audio-volume-change.oga; " SIG_CMD(SIG_AUDIO))},
	{ ALT,                          XK_m,      spawn,          SHCMD("mutemic; kill -36 $(pidof dwmblocks); " SIG_CMD(SIG_MIC))},
	{ ALT|ShiftMask,                XK_m,      spawn,          SHCMD("mutesound; kill -35 $(pidof dwmblocks)")},
	{ ALT,                          XK_0,      spawn,          SHCMD("kill -37 $(pidof dwmblocks); obs-kill-picom; " SIG_CMD(SIG_OBS))},
	{ ALT,                          XK_grave,  spawn,          SHCMD("kill -38 $(pidof dwmblocks)")},
	{ MODKEY,                       XK_grave,  spawn,          SHCMD("pgrep obs-ffmpeg && sleep 1 && killall obs && obs-kill-picom; " SIG_CMD(SIG_RECORDING))},
	{ MODKEY,                       XK_a,      spawn,          SHCMD("pgrep 'audacity' || auda")},
	{ MODKEY,                       XK_b,      spawn,          CMD("openbrowser")},
	{ MODKEY,                       XK_v,      spawn,          CMD("openvpn")},
	{ MODKEY,                       XK_c,      spawn,          CMD("colpick")},
	{ ALT|ShiftMask,                XK_a,      spawn,          CMD("switchaudioport")},
	{ MODKEY,                       XK_s,      spawn,          CMD(SCREENSHOT)},
	{ ALT|ShiftMask,             	XK_equal,  setgaps,        {.i = -1 } },
	{ ALT|ShiftMask,             	XK_minus,  setgaps,        {.i = +1 } },
	{ MODKEY|ShiftMask,             XK_equal,  setgaps,        {.i = -5 } },
	{ MODKEY|ShiftMask,             XK_minus,  setgaps,        {.i = +5 } },
	{ MODKEY|ShiftMask,             XK_0,      setgaps,        {.i = GAP_RESET } },
	{ MODKEY|ShiftMask,             XK_9,      setgaps,        {.i = GAP_TOGGLE} },
	{ ALT|ShiftMask,             	XK_q,      quit,           {0} },
	TAGKEYS(                        XK_1,                      0)
	TAGKEYS(                        XK_2,                      1)
	TAGKEYS(                        XK_3,                      2)
	TAGKEYS(                        XK_4,                      3)
	TAGKEYS(                        XK_5,                      4)
	TAGKEYS(                        XK_6,                      5)
	TAGKEYS(                        XK_7,                      6)
	TAGKEYS(                        XK_8,                      7)
	TAGKEYS(                        XK_9,                      8)
};

/* button definitions */
/* click can be ClkTagBar, ClkLtSymbol, ClkStatusText, ClkWinTitle, ClkClientWin, or ClkRootWin */
static const Button buttons[] = {
	/* click                event mask      button          function        argument */
	{ ClkLtSymbol,          0,              Button1,        setlayout,      {0} },
	{ ClkLtSymbol,          0,              Button3,        setlayout,      {.v = &layouts[2]} },
	{ ClkStatusText,        0,              Button2,        spawn,          {.v = termcmd } },
	{ ClkClientWin,         MODKEY,         Button1,        movemouse,      {0} },
	{ ClkClientWin,         MODKEY,         Button2,        togglefloating, {0} },
	{ ClkClientWin,         MODKEY,         Button3,        resizemouse,    {0} },
	{ ClkTagBar,            0,              Button1,        view,           {0} },
	{ ClkTagBar,            0,              Button3,        toggleview,     {0} },
	{ ClkTagBar,            MODKEY,         Button1,        tag,            {0} },
	{ ClkTagBar,            MODKEY,         Button3,        toggletag,      {0} },
};

#endif /* CONFIG_H */
