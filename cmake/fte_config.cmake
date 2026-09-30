
# general rebranding
set(DISTRIBUTION "FTE" CACHE STRING "") #should be kept short. 8 or less letters is good, with no spaces.
set(DISTRIBUTIONLONG "Forethought Entertainment" CACHE STRING "") #think of this as your company name. It isn't shown too often, so can be quite long.
set(FULLENGINENAME "FTE Quake" CACHE STRING "") #nominally user-visible name.
set(ENGINEWEBSITE "http://fte.triptohell.info" CACHE STRING "") #for shameless self-promotion purposes.
set(BRANDING_ICON "fte_eukara.ico" CACHE STRING "") #The file to use in windows' resource files - for linux your game should include an icon.[png|ico] file in the game's data.

# filesystem rebranding
set(GAME_SHORTNAME "quake" CACHE STRING "") #short alphanumeric description
set(GAME_FULLNAME "${FULLENGINENAME}" CACHE STRING "") #full name of the game we're playing
set(GAME_BASEGAMES FALSE CACHE STRING "") #comma-separate list of basegame strings to use
set(GAME_PROTOCOL "FTE-Quake" CACHE STRING "") #so other games won't show up in the server browser
set(GAME_DEFAULTPORT "27500" CACHE STRING "") #slightly reduces the chance of people connecting to the wrong type of server
set(GAME_IDENTIFYINGFILES FALSE CACHE STRING "") #with multiple games, this string-list gives verification that the basedir is actually valid. if null, will just be assumed correct.
set(GAME_DOWNLOADSURL FALSE CACHE STRING "") #url for the package manger to update from
set(GAME_DEFAULTCMDS FALSE CACHE STRING "") #a string containing the things you want to exec in order to override default.cfg

set(ENGINE_HAS_ZIP TRUE CACHE STRING "") #when defined, the engine is effectively a self-extrating zip with the gamedata zipped onto the end (if it in turn contains nested packages then they should probably be STOREd pk3s)

# Misc Renderer stuff
set(PSET_CLASSIC TRUE CACHE STRING "") #support the 'classic' particle system, for that classic quake feel.
set(PSET_SCRIPT TRUE CACHE STRING "") #scriptable particles (both fte's and importing effectinfo)
set(RTLIGHTS TRUE CACHE STRING "") #
set(RUNTIMELIGHTING TRUE CACHE STRING "") #automatic generation of .lit files

# Extra misc features
set(MULTITHREAD TRUE CACHE STRING "") #misc basic multithreading - dsound, downloads, basic stuff that's unlikely to have race conditions.
set(LOADERTHREAD TRUE CACHE STRING "") #worker threads for loading misc stuff. falls back on main thread if not supported.
if(WIN32)
	set(AVAIL_DINPUT TRUE CACHE STRING "") #
else()
	set(AVAIL_DINPUT FALSE CACHE STRING "") #
endif()
set(SIDEVIEWS 4 CACHE STRING "") #enable secondary/reverse views.
set(MAX_SPLITS 4u CACHE STRING "") #
set(VERTEXINDEXBYTES 2 CACHE STRING "") #16bit indexes work everywhere but may break some file types, 32bit indexes are optional in gles<=2 and d3d<=9 and take more memory/copying but allow for bigger batches/models. Plugins need to be compiled the same way so this is no longer set per-renderer.
set(TEXTEDITOR TRUE CACHE STRING "") #my funky text editor! its awesome!
set(PLUGINS TRUE CACHE STRING "") #support for external plugins (like huds or fancy menus or whatever)
set(USE_SQLITE TRUE CACHE STRING "") #sql-database-as-file support
set(IPLOG TRUE CACHE STRING "") #track player's ip addresses (any decent server will hide ip addresses, so this probably isn't that useful, but nq players expect it)

# Filesystem formats
set(PACKAGE_PK3 TRUE CACHE STRING "") #aka zips. we support utf8,zip64,spans,weakcrypto,(deflate),(bzip2),symlinks. we do not support strongcrypto nor any of the other compression schemes.
set(PACKAGE_Q1PAK TRUE CACHE STRING "") #also q2
set(PACKAGE_DOOMWAD FALSE CACHE STRING "") #doom wad support (generates various file names, and adds support for doom's audio, sprites, etc)
set(AVAIL_XZDEC TRUE CACHE STRING "") #.xz decompression
set(AVAIL_GZDEC TRUE CACHE STRING "") #.gz decompression
set(AVAIL_BZLIB FALSE CACHE STRING "") #whether pk3s can use bz2 compression
set(PACKAGE_DZIP TRUE CACHE STRING "") #.dzip support for smaller demos (which are actually more like pak files and can store ANY type of file)

# Map formats
set(Q1BSPS TRUE CACHE STRING "") #Quake1
set(Q2BSPS TRUE CACHE STRING "") #Quake2
set(Q3BSPS TRUE CACHE STRING "") #Quake3, as well as a load of other games too...
set(RFBSPS TRUE CACHE STRING "") #qfusion's bsp format / jk2o etc.
set(TERRAIN TRUE CACHE STRING "") #FTE's terrain, as well as .map support
set(DOOMWADS FALSE CACHE STRING "") #map support, filesystem support is separate.
set(MAP_PROC FALSE CACHE STRING "") #doom3...

# Model formats
set(SPRMODELS TRUE CACHE STRING "") #Quake's sprites
set(SP2MODELS TRUE CACHE STRING "") #Quake2's models
set(DSPMODELS TRUE CACHE STRING "") #Doom sprites!
set(MD1MODELS TRUE CACHE STRING "") #Quake's models.
set(MD2MODELS TRUE CACHE STRING "") #Quake2's models
set(MD3MODELS TRUE CACHE STRING "") #Quake3's models, also often used for q1 etc too.
set(MD5MODELS TRUE CACHE STRING "") #Doom3 models.
set(ZYMOTICMODELS TRUE CACHE STRING "") #nexuiz uses these, for some reason.
set(DPMMODELS TRUE CACHE STRING "") #these keep popping up, despite being a weak format.
set(PSKMODELS TRUE CACHE STRING "") #unreal's interchange format. Undesirable in terms of load times.
set(HALFLIFEMODELS TRUE CACHE STRING "") #horrible format that doesn't interact well with the rest of FTE's code. Unusable tools (due to license reasons).
set(INTERQUAKEMODELS TRUE CACHE STRING "") #Preferred model format, at least from an idealism perspective.
set(MODELFMT_MDX TRUE CACHE STRING "") #kingpin's format (for hitboxes+geomsets).
set(MODELFMT_OBJ TRUE CACHE STRING "") #lame mesh-only format that needs far too much processing and even lacks a proper magic identifier too
set(MODELFMT_GLTF TRUE CACHE STRING "") #khronos 'transmission format'. .gltf or .glb extension. PBR. Version 2 only, for now.
set(MODELFMT_DNXM FALSE CACHE STRING "") #3d realms duke nukem extended model
set(MODELFMT_CPJ FALSE CACHE STRING "") #3d realms cannibal project model
set(MODELFMT_GLM FALSE CACHE STRING "") #raven software ghoul 2 model
set(RAGDOLL TRUE CACHE STRING "") #ragdoll support. requires RBE support (via a plugin...).

# Image formats
set(IMAGEFMT_KTX TRUE CACHE STRING "") #Khronos TeXture. common on gles3 devices for etc2 compression
set(IMAGEFMT_PKM TRUE CACHE STRING "") #file format generally written by etcpack or android's etc1tool. doesn't support mips.
set(IMAGEFMT_ASTC TRUE CACHE STRING "") #lame simple header around a single astc image. not needed for astc in ktx files etc. its better to use ktx files.
set(IMAGEFMT_PBM TRUE CACHE STRING "") #pbm/ppm/pgm/pfm family formats.
set(IMAGEFMT_PSD TRUE CACHE STRING "") #baselayer only.
set(IMAGEFMT_XCF TRUE CACHE STRING "") #flattens, most of the time
set(IMAGEFMT_HDR TRUE CACHE STRING "") #an RGBE format.
set(IMAGEFMT_DDS TRUE CACHE STRING "") #.dds files embed mipmaps and texture compression. faster to load.
set(IMAGEFMT_TGA TRUE CACHE STRING "") #somewhat mandatory
set(IMAGEFMT_LMP TRUE CACHE STRING "") #mandatory for quake
set(IMAGEFMT_PNG TRUE CACHE STRING "") #common in quakeworld, useful for screenshots.
set(IMAGEFMT_JPG TRUE CACHE STRING "") #common in quake3, useful for screenshots.
set(IMAGEFMT_GIF FALSE CACHE STRING "") #for the luls. loads as a texture2DArray
set(IMAGEFMT_BLP FALSE CACHE STRING "") #legacy crap
set(IMAGEFMT_BMP TRUE CACHE STRING "") #windows bmp. yuck. also includes .ico for the luls
set(IMAGEFMT_PCX TRUE CACHE STRING "") #paletted junk. required for qw player skins, q2 and a few old skyboxes.
set(IMAGEFMT_EXR TRUE CACHE STRING "") #openexr, via Industrial Light & Magic's rgba api, giving half-float data.
set(IMAGEFMT_PVR TRUE CACHE STRING "") #powervr texture, used by various dreamcast games including HL and Q3
set(PACKAGE_TEXWAD TRUE CACHE STRING "") #quake's image wad support
set(DECOMPRESS_ETC2 TRUE CACHE STRING "") #decompress etc2(core in gles3/gl4.3) if the graphics driver doesn't support it (eg d3d or crappy gpus with vulkan).
set(DECOMPRESS_S3TC TRUE CACHE STRING "") #allows bc1-3 to work even when drivers don't support it. This is probably only an issue on mobile chips. WARNING: not entirely sure if all patents expired yet...
set(DECOMPRESS_RGTC TRUE CACHE STRING "") #bc4+bc5
set(DECOMPRESS_BPTC TRUE CACHE STRING "") #bc6+bc7
set(DECOMPRESS_ASTC TRUE CACHE STRING "") #ASTC, for drivers that don't support it properly.

# Game/Gamecode Support
set(CSQC_DAT TRUE CACHE STRING "") #
set(MENU_DAT TRUE CACHE STRING "") #
set(MENU_NATIVECODE FALSE CACHE STRING "") #Use an external dll for menus.
set(VM_Q1 TRUE CACHE STRING "") #q1qvm implementation, to support ktx.
set(VM_LUA FALSE CACHE STRING "") #optionally supports lua instead of ssqc.
set(Q2SERVER TRUE CACHE STRING "") #q2 server+gamecode.
set(Q2CLIENT TRUE CACHE STRING "") #q2 client. file formats enabled separately.
set(Q3CLIENT TRUE CACHE STRING "") #q3 client stuff.
set(Q3SERVER TRUE CACHE STRING "") #q3 server stuff.
set(AVAIL_BOTLIB TRUE CACHE STRING "") #q3 botlib
set(BOTLIB_STATIC FALSE CACHE STRING "") #should normally be set only in the makefile, and only if AVAIL_BOTLIB is defined above.
set(HEXEN2 TRUE CACHE STRING "") #runs hexen2 gamecode, supports hexen2 file formats.
set(HUFFNETWORK TRUE CACHE STRING "") #crappy network compression. probably needs reseeding.
set(NETPREPARSE TRUE CACHE STRING "") #allows for running both nq+qw on the same server (if not, protocol used must match gamecode).
set(SUBSERVERS TRUE CACHE STRING "") #Allows the server to fork itself, each acting as an MMO-style server instance of a single 'realm'.
set(HLCLIENT FALSE CACHE STRING "") #we can run HL gamecode (not protocol compatible, set to 6 or 7)
set(HLSERVER FALSE CACHE STRING "") #we can run HL gamecode (not protocol compatible, set to 138 or 140)
set(SAVEDGAMES TRUE CACHE STRING "") #Can save the game.
set(MVD_RECORDING TRUE CACHE STRING "") #server can record MVDs.
set(ENGINE_ROUTING TRUE CACHE STRING "") #Engine-provided routing logic (possibly threaded)
set(USE_INTERNAL_BULLET FALSE CACHE STRING "") #Statically link against bullet physics plugin (instead of using an external plugin)
set(USE_INTERNAL_ODE FALSE CACHE STRING "") #Statically link against ode physics plugin (instead of using an external plugin)
set(OMIT_QCC FALSE CACHE STRING "") #Don't include QCC

# Networking options
set(NQPROT TRUE CACHE STRING "") #act as an nq client/server, with nq gamecode.
set(HAVE_PACKET TRUE CACHE STRING "") #we can send unreliable messages!
set(HAVE_TCP TRUE CACHE STRING "") #we can create/accept TCP connections.
if(EMSCRIPTEN)
	set(HAVE_GNUTLS FALSE CACHE STRING "") #on linux
	set(HAVE_WINSSPI FALSE CACHE STRING "") #on windows
elseif(WIN32)
	set(HAVE_GNUTLS FALSE CACHE STRING "") #on linux
	set(HAVE_WINSSPI TRUE CACHE STRING "") #on windows
else()
	set(HAVE_GNUTLS TRUE CACHE STRING "") #on linux
	set(HAVE_WINSSPI FALSE CACHE STRING "") #on windows
endif()
set(FTPSERVER TRUE CACHE STRING "") #sv_ftp cvar.
set(WEBCLIENT TRUE CACHE STRING "") #uri_get+any internal downloads etc
set(HAVE_HTTPSV TRUE CACHE STRING "") #net_enable_http/websocket
set(TCPCONNECT TRUE CACHE STRING "") #support for playing over tcp sockets, instead of just udp. compatible with qizmo.
set(IRCCONNECT FALSE CACHE STRING "") #lame support for routing game packets via irc server. not a good idea.
set(SUPPORT_ICE TRUE CACHE STRING "") #Internet Connectivity Establishment, for use by plugins to establish voice or game connections.
set(CL_MASTER TRUE CACHE STRING "") #Clientside Server Browser functionality.
set(PACKAGEMANAGER TRUE CACHE STRING "") #Allows the user to enable/disable/download(with WEBCLIENT) packages and plugins. Also handles map packages.
set(MAX_CLIENTS 255 CACHE STRING "")

# Audio Drivers
set(AVAIL_OPENAL TRUE CACHE STRING "") #
if(WIN32)
	set(AVAIL_WASAPI TRUE CACHE STRING "") #windows advanced sound api
	set(AVAIL_DSOUND TRUE CACHE STRING "") #
else()
	set(AVAIL_WASAPI FALSE CACHE STRING "") #windows advanced sound api
	set(AVAIL_DSOUND FALSE CACHE STRING "") #
endif()
set(HAVE_MIXER TRUE CACHE STRING "") #support non-openal audio drivers

# Audio Formats
set(AVAIL_MP3_ACM TRUE CACHE STRING "") #.mp3 support (windows only).

# Other Audio Options
set(VOICECHAT TRUE CACHE STRING "") #
set(HAVE_SPEEX TRUE CACHE STRING "") #Support the speex codec.
set(HAVE_OPUS TRUE CACHE STRING "") #Support the opus codec.
set(HAVE_MEDIA_DECODER TRUE CACHE STRING "") #can play cin/roq, more with plugins
set(HAVE_MEDIA_ENCODER TRUE CACHE STRING "") #capture/capturedemo work.
set(HAVE_CDPLAYER TRUE CACHE STRING "") #includes cd playback. actual cds. named/numbered tracks are supported regardless (though you need to use the 'music' command to play them without this).
set(HAVE_JUKEBOX TRUE CACHE STRING "") #includes built-in jukebox crap
set(HAVE_SPEECHTOTEXT TRUE CACHE STRING "") #windows speech-to-text thing

# Features required by vanilla quake/quakeworld...
set(QUAKETC FALSE CACHE STRING "") #
set(QUAKESTATS TRUE CACHE STRING "") #defines STAT_HEALTH etc. if omitted, you'll need to provide that functionality yourself.
set(QUAKEHUD TRUE CACHE STRING "") #support for drawing the vanilla hud. disable this if you're always going to be using csqc (or equivelent)
set(QWSKINS TRUE CACHE STRING "") #disabling this means no qw .pcx skins nor enemy/team skin/colour forcing
set(NOBUILTINMENUS FALSE CACHE STRING "") #
set(NOLEGACY FALSE CACHE STRING "") #just spike trying to kill off crappy crap...
set(USEAREAGRID TRUE CACHE STRING "") #world collision optimisation. REQUIRED for performance with xonotic. hopefully it helps a few other mods too.
set(NOQCDESCRIPTIONS FALSE CACHE STRING "") #if 2, disables writing fteextensions.qc completely. 1 just omits the text. (ignored in debug builds.)

# Outdated stuff
set(SVRANKING TRUE CACHE STRING "") #legacy server-side ranking system.
set(QTERM FALSE CACHE STRING "") #qterm... adds a console command that allows running programs from within quake - bit like xterm.
set(SVCHAT FALSE CACHE STRING "") #ancient lame builtin to support NPC-style chat...
set(SV_MASTER FALSE CACHE STRING "") #Support running the server as a master server. Should probably not be used.
set(QUAKESPYAPI FALSE CACHE STRING "") #define this if you want the engine to be usable via gamespy/quakespy, which has been dead for a long time now. forces the client to use a single port for all outgoing connections, which hurts reconnects.

# configure the options header
configure_file(${FTE_ROOT_DIR}/cmake/config.h.in ${FTE_ENGINE_COMMON_DIR}/config.h @ONLY)
list(APPEND FTE_COMMON_DEFINITIONS CONFIG_FILE_NAME=config.h)
