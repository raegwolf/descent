/*
 * The original shareware recorder omits the laser-level event writer even
 * though common gameplay calls the public hook.  Full builds use the original
 * NEWDEMO.C macOS implementation.  The graphics-first MACOS shareware build keeps
 * the event unavailable and supplies the missing ABI entry point here.
 */
#include "types.h"

#if defined(MACOS) && defined(SHAREWARE)
void newdemo_record_laser_level(byte old_level, byte new_level)
{
	(void)old_level;
	(void)new_level;
}
#endif
