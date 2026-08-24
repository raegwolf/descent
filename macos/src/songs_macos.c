/*
 * Graphics-only macOS port: music is intentionally disabled.
 *
 * This replaces MAIN/SONGS.C in the macOS target.  The original module
 * parses descent.sng and starts MIDI playback.  Neither the parser nor the
 * song data is needed while the first port milestone is graphics-only.
 * Keeping the public API and Songs table lets the original game and menu
 * code run unchanged; a future sound port can restore MAIN/SONGS.C.
 */
#include "songs.h"

song_info Songs[MAX_SONGS];

void songs_play_song(int songnum, int repeat)
{
	(void)songnum;
	(void)repeat;
}

void songs_play_level_song(int levelnum)
{
	(void)levelnum;
}
