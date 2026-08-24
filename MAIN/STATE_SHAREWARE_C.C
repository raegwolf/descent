/*
 * STATE.C deliberately excludes save-game support from SHAREWARE builds, but
 * shared title/briefing code retains this hook.  Keep the original shareware
 * behavior (no save produced) while satisfying the portable link contract.
 */
#if defined(MACOS) && defined(SHAREWARE)
int state_save_all(int between_levels)
{
	(void)between_levels;
	return 0;
}
#endif
