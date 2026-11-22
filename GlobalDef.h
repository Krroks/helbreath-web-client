#ifndef GLOBALDEF_H
#define GLOBALDEF_H

// Logical screen: 16:9 at the original 480 lines. The 640x480 art (menus, HUD panel) is centred in it.
#define SCREENX			854
#define SCREENY			480
#define UISHIFTX		((SCREENX-640)/2)	// the HUD and dialogs are laid out on a 640x480 board centred in the frame
#define VIEWY			427	// height of the world view (the icon panel takes the rest)
#define PLAYER_VIEW_TX	13	// tile column the player stands on, from the left edge of the world view (was 10)
#define VIEW_TILES_X	28	// width of the map window the server sends: x-PLAYER_VIEW_TX .. x+14 (was 21)

#define NAME_WORLDNAME1			"WS1" // Name for default (1st) server
#define MSG_WORLDNAME1			"180/200 Low-Rate Server" // Long name for 1st server...
#define NAME_WORLDNAME2			"WS2"
#define MSG_WORLDNAME2			"High-Rate Server"
#define MSG_HOMEPAGE			""

//#define SERVER_IP			"74.55.185.122"	// dedi
//#define SERVER_IP			"65.175.174.30" //test-Server
//#define SERVER_IP			"192.168.1.11"
#define SERVER_IP			"192.168.1.4"
#define SERVER_PORT			321

#define LANG	ENGLISH
//#define LANG	SPANISH

/*** Some more compilation options ***/
//#define USING_WIN_IME		// Used for unicode characters (unchecked)
#define MAKE_ACCOUNT		// Enable acc creation. Missing sprites, fixed!
#define SELECTSERVER		// Show/Hides server selection screen.

#define COMMA_GOLD		// Allows to show commas number (ie: 1,200,000)

//#define DEF_HTMLCOMMOM

//#define  _DEBUG				// Multi-Client, Instant-logout, Version check, Badwords,
								// Mics debug msgs, CtrlQ for admins, Show status, Cursor position

#ifdef _DEBUG
	#define SPEEDHAX_RUN	1.5
	#define SPEEDHAX_ATK 2
	#define SPEEDHAX_HIT	3
	#define LOGINCFG
	#define _SCDEBUG
	#define WINDOWED_MODE	// Shows HB in a windowed mode (for debug purpose only, sprite will bug....)
#else
	#define SPEEDHAX_RUN	1
	#define SPEEDHAX_ATK 1
	#define SPEEDHAX_HIT	1
#define LOGINCFG
#endif
		
#endif