#include <obs-module.h>

OBS_DECLARE_MODULE()

OBS_MODULE_USE_DEFAULT_LOCALE("cellcamera", "en-US")

extern struct obs_source_info ios_source;

bool obs_module_load(void)
{
	obs_register_source(&ios_source);
	return true;
}
