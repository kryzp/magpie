
internal CameraDriver
CameraDriverInit(const CameraDriverConfig *config)
{
	CameraDriver driver = {0};
	driver.config = *config;

	return driver;
}

internal void
CameraDriverShake(CameraDriver *driver, f32 amount)
{
	// TODO
}

internal void
CameraDriverDrive(CameraDriver *driver, R_Camera *camera)
{
	// TODO
}
