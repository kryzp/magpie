
static void GunFire(Gun *gun)
{
	if (gun->ammo_count <= 0)
	{
		DebugPrintT("Click!");
		return;
	}

	if (CH_TimerElapsed(&gun->shoot_timer) < gun->specs.rechamber_time)
	{
		DebugPrintT("Rechambering... %f / %f", CH_TimerElapsed(&gun->shoot_timer), gun->specs.rechamber_time);
		return;
	}

	gun->ammo_count--;

	DebugPrintT("Fired! %u / %u", gun->ammo_count, gun->specs.max_ammo_in_clip);

	CH_TimerRestart(&gun->shoot_timer);
}

static void GunReload(Gun *gun)
{
	gun->ammo_count = gun->specs.max_ammo_in_clip;

	DebugPrintT("Reloaded! %u / %u", gun->ammo_count, gun->specs.max_ammo_in_clip);

	CH_TimerRestart(&gun->shoot_timer);
}
