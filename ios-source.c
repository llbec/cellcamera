#include <obs.h>
#include "obs-module.h"

#include "airrcv/airrcv.h"

struct ios_source
{
    obs_source_t *source;
};

static const char *ios_source_getname(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("Apple");
}

static void ios_source_destroy(void *data)
{
	struct ios_source *ios = data;
	if (ios) {
		airplay_recv_stop();
		bfree(ios);
	}
}

static void *ios_source_create(obs_data_t *settings, obs_source_t *source)
{
	struct ios_source *ios = bzalloc(sizeof(struct ios_source));
	ios->source = source;

	int ret = airplay_recv_start();
	if (ret != 0) {
		ios_source_destroy(ios);
		return NULL;
	}

	UNUSED_PARAMETER(settings);
	return ios;
}

struct obs_source_info ios_source = {
	.id           = "ios-source",
	.type         = OBS_SOURCE_TYPE_INPUT,
	.output_flags = OBS_SOURCE_ASYNC_VIDEO,
	.get_name     = ios_source_getname,
	.create       = ios_source_create,
	.destroy      = ios_source_destroy,
	.icon_type    = OBS_ICON_TYPE_CAMERA
};
