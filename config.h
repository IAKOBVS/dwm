/* See LICENSE file for copyright and license details. */

#ifndef CONFIG_H
#define CONFIG_H 1

#include <X11/Xlib.h>
#include <X11/XF86keysym.h>
#include <sys/types.h>

#include "dwm.h"

/* constants */
#define TERMINAL     "alacritty"
#define SHELL        "zsh"
#define SHELL_BACKUP "bash"
#define VEDITOR      "dav"
#define BROWSER      "brave"
#define TORRENT      "qbittorrent"
#define SCREENSHOT   "scnow"
#define DISCORD      "discord"

/* scripts from from dwmblocks-fast */
#define MIC_MUTE     "dwmblocks-fast-mic-mute"
#define MIC_MUTE     "dwmblocks-fast-mic-mute"
#define VOLUME_UP    "dwmblocks-fast-audio-vol-up"
#define VOLUME_DOWN  "dwmblocks-fast-audio-vol-down"
#define AUDIO_MUTE   "dwmblocks-fast-audio-mute"
#define COLOR_PICKER "colpick"
#define OBS          "dwmblocks-fast-obs"
#define OBS_RECORD   "dwmblocks-fast-obs-record"
#define CFAN_MEDIUM  "dwmblocks-fast-cfan-medium"
#define CFAN_HIGH    "dwmblocks-fast-cfan-high"

/* appearance */
static char normbordercolor[]       = "#222222";
static char normfgcolor[]           = "#dad2c8";
static char normbgcolor[]           = "#222222";
static char selbordercolor[]        = "#988165";
static char selbgcolor[]            = "#988165";
static char selfgcolor[]            = "#222222";

static const unsigned int borderpx  = 5;        /* border pixel of windows */
static const Gap default_gap        = {
	.realgap = 17,
	.gappx = 17,
	.isgap = 1
};
static const unsigned int snap      = 32;       /* snap pixel */
static int swallowfloating    = 0;        /* 1 means swallow floating windows by default */
static const unsigned char showbar            = 1;        /* 0 means no bar */
static const unsigned char topbar             = 1;        /* 0 means bottom bar */

static const char *fonts[]          = {
	"monospace:size=11:antialias=true:autohint=true",
	"Noto Color Emoji:size=11:antialias=true:autohint=true:width=ultracondensed"
};
static const char dmenufont[]       = "monospace:size=11";

static const char *colors[][3]      = {
      /*               fg               bg               border   */
      [SchemeNorm] = { normfgcolor, normbgcolor, normbordercolor },
      [SchemeSel]  = { selfgcolor,  selbgcolor,  selbordercolor  },
};

/* tagging */
static const char *tags[] = { "1", "2", "3", "4", "5", "6", "7", "8", "9" };

static const Rule rules[] = {
    /* xprop(1):
         *WM_CLASS(STRING) = instance, class
           *WM_NAME(STRING) = title
             */
	{ .class = "st-256color",             .instance = NULL, .title = NULL,       .tags = 0,      .isfloating = 0, .isterminal = 1, .noswallow = 0, .monitor = 1 },
	{ .class = "Alacritty",               .instance = NULL, .title = NULL,       .tags = 0,      .isfloating = 0, .isterminal = 1, .noswallow = 0, .monitor = 1 },
	{ .class = "resolve",                 .instance = NULL, .title = NULL,       .tags = 1 << 1, .isfloating = 0, .isterminal = 0, .noswallow = 0, .monitor = 1 },
	{ .class = "Resolve",                 .instance = NULL, .title = NULL,       .tags = 1 << 1, .isfloating = 0, .isterminal = 0, .noswallow = 0, .monitor = 1 },
	{ .class = "Gimp",                    .instance = NULL, .title = NULL,       .tags = 1 << 1, .isfloating = 0, .isterminal = 0, .noswallow = 0, .monitor = 1 },
	{ .class = "Mail",                    .instance = NULL, .title = NULL,       .tags = 1 << 7, .isfloating = 0, .isterminal = 0, .noswallow = 0, .monitor = 1 },
	{ .class = "discord",                 .instance = NULL, .title = NULL,       .tags = 1 << 7, .isfloating = 0, .isterminal = 0, .noswallow = 0, .monitor = 1 },
	{ .class = "Discord",                 .instance = NULL, .title = NULL,       .tags = 1 << 7, .isfloating = 0, .isterminal = 0, .noswallow = 0, .monitor = 1 },
	{ .class = "org.mozilla.Thunderbird", .instance = NULL, .title = NULL,       .tags = 1 << 7, .isfloating = 0, .isterminal = 0, .noswallow = 0, .monitor = 1 },
	{ .class = "Audacity",                .instance = NULL, .title = NULL,       .tags = 1 << 8, .isfloating = 0, .isterminal = 0, .noswallow = 0, .monitor = 1 },
	{.class =  "audacity",               .instance = NULL, .title = NULL,       .tags = 1 << 8, .isfloating = 0, .isterminal = 0, .noswallow = 0, .monitor = 1 },
	{.class =  "obs",                    .instance = NULL, .title = NULL,       .tags = 1 << 8, .isfloating = 0, .isterminal = 0, .noswallow = 0, .monitor = 1 },
	{.class =  "qBittorrent",            .instance = NULL, .title = NULL,       .tags = 1 << 8, .isfloating = 0, .isterminal = 0, .noswallow = 0, .monitor = 1 },
	{.class =  "qbittorrent",            .instance = NULL, .title = NULL,       .tags = 1 << 8, .isfloating = 0, .isterminal = 0, .noswallow = 0, .monitor = 1 },
	{.class =  "Steam",                  .instance = NULL, .title = NULL,       .tags = 1 << 8, .isfloating = 0, .isterminal = 0, .noswallow = 0, .monitor = 1 },
	{.class =  "steam",                  .instance = NULL, .title = NULL,       .tags = 1 << 8, .isfloating = 0, .isterminal = 0, .noswallow = 0, .monitor = 1 },
	{.class =  "steamwebhelper",         .instance = NULL, .title = NULL,       .tags = 1 << 8, .isfloating = 0, .isterminal = 0, .noswallow = 0, .monitor = 1 },
	{.class =  NULL,                     .instance = NULL, .title = "WhatsApp", .tags = 1 << 8, .isfloating = 0, .isterminal = 0, .noswallow = 0, .monitor = 1 },
	{.class =  NULL,                     .instance = NULL, .title = "Gmail",    .tags = 1 << 8, .isfloating = 0, .isterminal = 0, .noswallow = 0, .monitor = 1 },
	/* { "kdenlive",     NULL, NULL, 1 << 1, 0, 0, 0, 1 }, */
	/* { "jamesdsp",    NULL, NULL, 1 << 7, 0, 0, 0, 1 }, */
#ifdef DWM_TEST
	{ "TestClass", "TestInstance", NULL, 1 << 2, 0, 0, 0, -1 },  /* instance match coverage (line 100) */
#endif
};

/* layout(s) */
static const float mfact     = 0.55f; /* factor of master area size [0.05..0.95] */
static const int nmaster     = 1;    /* number of clients in master area */
static const int resizehints = 1;    /* 1 means respect size hints in tiled resizals */
static const int lockfullscreen = 1; /* 1 will force focus on the fullscreen window */
static const int optimizefullscreen = 1; /* 1 will skip bar drawing during fullscreen (saves CPU) */

static const Layout layouts[] = {
	/* symbol     arrange function */
	{ "[]=",      tile },    /* first entry is default */
	{ "><>",      NULL },    /* no layout function means floating behavior */
	/* { "[M]",      monocle }, */
};

/* key definitions */
#define MODKEY Mod1Mask
#define ALTKEY Mod4Mask
#define TAGKEYS(KEY,TAG) \
	{ MODKEY,                       KEY,             view,           {.ui = 1 << TAG} }, \
	{ MODKEY|ControlMask,           KEY,             toggleview,     {.ui = 1 << TAG} }, \
	{ ALTKEY,                       KEY,             tag,            {.ui = 1 << TAG} }, \
	{ MODKEY|ControlMask|ShiftMask, KEY,             toggletag,      {.ui = 1 << TAG} },

/* helper for spawning binaries */
#define CMD(cmd) { .v = (const char*[]){ cmd, NULL } }
#define CMD_ARGS(cmd, ...) { .v = (const char*[]){ cmd, __VA_ARGS__, NULL } }
/* helper for spawning shell commands in the pre dwm-5.0 fashion */
#define SHCMD(cmd) { .v = (const char*[]){ "/bin/sh", "-c", cmd, NULL } }

/* commands */
static char dmenumon[2] = "0"; /* component of dmenucmd, manipulated in spawn() */
static const char *dmenucmd[] = {
	"dmenu_run", "-m", dmenumon, "-fn", dmenufont, "-nb", selfgcolor, "-nf", normfgcolor, "-sb", selbgcolor, "-sf", selfgcolor, NULL
};

#if 0
static const char *termcmd[] = {
	"alacritty" , "msg", "create-window", NULL
};
#endif

/* processes to kill on fullscreen enter, restart on fullscreen exit */
#ifdef DWM_TEST
static const char *killatfullscreen[] = {
	"test-target" /* lets tests assert the STOP/CONT/HUP invocations */
};
#else
static const char *killatfullscreen[] = {
	"dwmblocks-fast"
	/* "unclutter" */
};
#endif

static const Key keys[] = {
	{ .mod = MODKEY,            .keysym = XK_p,                    .func = spawn,         .arg = { .v = dmenucmd } },
	{ .mod = MODKEY,            .keysym = XK_Return,               .func = spawn,         .arg = SHCMD("alacritty msg create-window || alacritty") },
	/* {  .mod = MODKEY,            .keysym = XK_Return,               .func = spawn,         .arg = CMD("st") }, */
	/* {  .mod = MODKEY,            .keysym = XK_Return,               .func = spawn,         .arg = CMD("gaming-unkill") }, */
	/* {  .mod = ALTKEY,            .keysym =    XK_Return,            .func = spawn,         .arg = CMD("st") }, */
	{ .mod = MODKEY,            .keysym = XK_b,                    .func = togglebar,     .arg = {0} },
	{ .mod = MODKEY,            .keysym = XK_j,                    .func = focusstack,    .arg = { .i= +1 } },
	{ .mod = MODKEY,            .keysym = XK_k,                    .func = focusstack,    .arg = { .i= -1 } },
	{ .mod = MODKEY,            .keysym = XK_f,                    .func = togglefullscr, .arg = {0} },
	{ .mod = MODKEY,            .keysym = XK_h,                    .func = setmfact,      .arg = { .f= -0.05f} },
	{ .mod = MODKEY,            .keysym = XK_l,                    .func = setmfact,      .arg = { .f= +0.05f} },
	{ .mod = MODKEY,            .keysym = XK_q,                    .func = killclient,    .arg = {0} },
	/* {  .mod = MODKEY,            .keysym = K_q,                     .func = spawn,         .arg = CMD("gaming-kill-when-steam-running") }, */
	{ .mod = MODKEY,            .keysym = XK_space,                .func = zoom,          .arg = {0} },
	{.mod =  MODKEY,           .keysym = XK_Tab,                  .func = view,          .arg = {0} },
	/* {  .mod =  MODKEY,           .keysym = XK_space,                .func = setlayout,     .arg = { .v = &layouts[0]} }, */ /* collided with zoom */
	{.mod =  MODKEY,           .keysym = XK_comma,                .func = focusmon,      .arg = { .i= -1 } },
	{.mod =  MODKEY,           .keysym = XK_period,               .func = focusmon,      .arg = { .i= +1 } },
	/* {  .mod =  ALTKEY,           .keysym = XK_comma,                .func = tagmon,        .arg = { .i= -1 } }, */
	/* {  .mod =  ALTKEY,           .keysym = XK_period,               .func = tagmon,        .arg = { .i= +1 } }, */
	{.mod =  MODKEY,           .keysym = XK_w,                    .func = spawn,         .arg = CMD(BROWSER) },
	{.mod =  MODKEY,           .keysym = XK_s,                    .func = spawn,         .arg = CMD("dsite") },
	{.mod =  MODKEY,           .keysym = XK_c,                    .func = spawn,         .arg = CMD("dclip") },
	{.mod =  MODKEY,           .keysym = XK_m,                    .func = spawn,         .arg = CMD(MIC_MUTE) },
	{.mod =  MODKEY|ShiftMask, .keysym = XK_m,                    .func = spawn,         .arg = CMD(AUDIO_MUTE) },
	{.mod =  MODKEY|ShiftMask, .keysym = XK_0,                    .func = spawn,         .arg = CMD(OBS) },
	{.mod =  MODKEY|ShiftMask, .keysym = XK_grave,                .func = spawn,         .arg = CMD(OBS_RECORD) },
	/* {  .mod =  MODKEY,           .keysym = XK_c,                    .func = spawn,         .arg = CMD(COLOR_PICKER) }, */ /* collided with dclip */
	{.mod =  MODKEY|ShiftMask, .keysym = XK_a,                    .func = spawn,         .arg = CMD("switchaudioport") },
	/* {  .mod =  MODKEY,           .keysym = XK_s,                    .func = spawn,         .arg = CMD(SCREENSHOT) }, */ /* collided with dsite */
	{.mod =  MODKEY,           .keysym = XK_minus,                .func = spawn,         .arg = CMD_ARGS(VOLUME_DOWN, "1") },
	{.mod =  ALTKEY,           .keysym = XK_minus,                .func = spawn,         .arg = CMD_ARGS(VOLUME_DOWN, "2") },
	{.mod =  MODKEY,           .keysym = XK_equal,                .func = spawn,         .arg = CMD_ARGS(VOLUME_UP,   "1") },
	{.mod =  ALTKEY,           .keysym = XK_equal,                .func = spawn,         .arg = CMD_ARGS(VOLUME_UP,   "2") },
	{.mod =  ALTKEY,           .keysym = XK_F9,                   .func = spawn,         .arg = CMD("cfan") },
	/* {  .mod =  MODKEY,           .keysym = XF86XK_AudioLowerVolume, .func = spawn,         .arg = CMD_ARGS("sudo", CFAN_MEDIUM) }, */
	/* {  .mod =  MODKEY,           .keysym = XF86XK_AudioRaiseVolume, .func = spawn,         .arg = CMD_ARGS("sudo", CFAN_HIGH) }, */
	{.mod =  MODKEY,           .keysym = XK_u,                    .func = spawn,         .arg = SHCMD(TERMINAL " -e system-update")},
	/* {  .mod =  MODKEY,           .keysym = XK_u,                    .func = spawn,         .arg = SHCMD(TERMINAL " -e system-update-aur")}, */
	{.mod =  MODKEY,           .keysym = XK_a,                    .func = spawn,         .arg = SHCMD("pgrep 'audacity' || auda")},
	{.mod =  MODKEY|ShiftMask, .keysym = XK_equal,                .func = setgaps,       .arg = { .i= -1 } },
	{.mod =  MODKEY|ShiftMask, .keysym = XK_minus,                .func = setgaps,       .arg = { .i= +1 } },
	{.mod =  ALTKEY|ShiftMask, .keysym = XK_equal,                .func = setgaps,       .arg = { .i= -5 } },
	{.mod =  ALTKEY|ShiftMask, .keysym = XK_minus,                .func = setgaps,       .arg = { .i= +5 } },
	{.mod =  ALTKEY|ShiftMask, .keysym = XK_0,                    .func = setgaps,       .arg = { .i= GAP_RESET } },
	{.mod =  MODKEY|ShiftMask, .keysym = XK_9,                    .func = setgaps,       .arg = { .i= GAP_TOGGLE} },
	/* { ALTKEY|ShiftMask,          .keysym = XK_q,                    .func = quit,          .arg = {0} }, */
	TAGKEYS(XK_1, 0)
	TAGKEYS(XK_2, 1)
	TAGKEYS(XK_3, 2)
	TAGKEYS(XK_4, 3)
	TAGKEYS(XK_5, 4)
	TAGKEYS(XK_6, 5)
	TAGKEYS(XK_7, 6)
	TAGKEYS(XK_8, 7)
	TAGKEYS(XK_9, 8)
#ifdef DWM_TEST
	{ 0 },  /* sentinel: test null-func skip in cachekeys */
#endif
};

/* button definitions */
/* click can be ClkTagBar, ClkLtSymbol, ClkStatusText, ClkWinTitle, ClkClientWin, or ClkRootWin */
static const Button buttons[] = {
	/*    click                   event mask      button             function            argument */
	/* {  .click = ClkLtSymbol,   .mask = 0,      .button = Button1, .func = setlayout,  .arg = {0} }, */
	/* {  .click = ClkLtSymbol,   .mask = 0,      .button = Button3, .func = setlayout,  .arg = { .v = &layouts[2]} }, */
	/* {  .click = ClkStatusText, .mask = 0,      .button = Button2, .func = spawn,      .arg = { .v = termcmd } }, */
	{ .click = ClkClientWin,  .mask = MODKEY, .button = Button1, .func = movemouse,  .arg = {0} },
	{ .click = ClkTagBar,     .mask = MODKEY, .button = Button1, .func = view,       .arg = {0} },
	/* {  .click = ClkTagBar,     .mask = 0,      .button = Button3, .func = toggleview, .arg = {0} }, */
	/* {  .click = ClkTagBar,     .mask = MODKEY, .button = Button1, .func = tag,        .arg = {0} }, */
	/* {  .click = ClkTagBar,     .mask = MODKEY, .button = Button3, .func = toggletag,  .arg = {0} }, */
#ifdef DWM_TEST
	{ 0 },  /* sentinel: test null-func skip in cachebuttons */
#endif
};

#endif /* CONFIG_H */
