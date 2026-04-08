#include <math.h>

#include <obs-frontend-api.h>
#include <obs-module.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("obs-fit-selected-source", "en-US")

#define PLUGIN_NAME "obs-fit-selected-source"
#define ROTATION_EPSILON 0.01f

struct fit_stats {
	uint32_t applied;
	uint32_t skipped_locked;
	uint32_t skipped_group;
	uint32_t skipped_group_transform;
	uint32_t skipped_rotation;
	uint32_t skipped_invalid_size;
	uint32_t skipped_not_selected;
};

struct fit_context {
	struct fit_stats *stats;
	uint32_t canvas_width;
	uint32_t canvas_height;
	obs_sceneitem_t *parent_group;
	bool parent_group_unsafe;
	struct vec2 parent_group_offset;
};

static bool nearly_zero(float value)
{
	return fabsf(value) < ROTATION_EPSILON;
}

static float normalize_rotation_degrees(float value)
{
	float normalized = fmodf(value, 360.0f);

	if (normalized < 0.0f)
		normalized += 360.0f;
	if (normalized > 180.0f)
		normalized -= 360.0f;

	return normalized;
}

static float preserve_flip_scale(float current, float fit_scale)
{
	return signbit(current) ? -fit_scale : fit_scale;
}

static bool group_transform_is_translation_only(obs_sceneitem_t *group_item,
		struct vec2 *group_pos)
{
	struct vec2 pos;
	struct vec2 scale;
	struct obs_sceneitem_crop crop;
	float rotation;

	obs_sceneitem_get_pos(group_item, &pos);
	obs_sceneitem_get_scale(group_item, &scale);
	obs_sceneitem_get_crop(group_item, &crop);
	rotation = normalize_rotation_degrees(obs_sceneitem_get_rot(group_item));

	if (!nearly_zero(scale.x - 1.0f) || !nearly_zero(scale.y - 1.0f))
		return false;
	if (!nearly_zero(rotation))
		return false;
	if (crop.left || crop.top || crop.right || crop.bottom)
		return false;
	if (obs_sceneitem_get_bounds_type(group_item) != OBS_BOUNDS_NONE)
		return false;
	if (obs_sceneitem_get_alignment(group_item) !=
	    (OBS_ALIGN_LEFT | OBS_ALIGN_TOP))
		return false;
	if (group_pos)
		*group_pos = pos;

	return true;
}

static bool fit_scene_item_to_canvas(obs_sceneitem_t *item, uint32_t canvas_width,
		uint32_t canvas_height, obs_sceneitem_t *parent_group,
		const struct vec2 *parent_group_offset)
{
	obs_source_t *source = obs_sceneitem_get_source(item);
	struct obs_sceneitem_crop crop;
	struct vec2 current_scale;
	struct vec2 scale;
	struct vec2 pos;
	uint32_t source_width;
	uint32_t source_height;
	uint32_t cropped_width;
	uint32_t cropped_height;
	float fit_scale;

	if (!source)
		return false;

	obs_sceneitem_get_crop(item, &crop);
	source_width = obs_source_get_width(source);
	source_height = obs_source_get_height(source);

	if ((uint32_t)(crop.left + crop.right) >= source_width ||
	    (uint32_t)(crop.top + crop.bottom) >= source_height) {
		return false;
	}

	cropped_width = source_width - (uint32_t)(crop.left + crop.right);
	cropped_height = source_height - (uint32_t)(crop.top + crop.bottom);

	if (!cropped_width || !cropped_height || !canvas_width || !canvas_height)
		return false;

	fit_scale = fminf((float)canvas_width / (float)cropped_width,
		(float)canvas_height / (float)cropped_height);

	obs_sceneitem_get_scale(item, &current_scale);
	scale.x = preserve_flip_scale(current_scale.x, fit_scale);
	scale.y = preserve_flip_scale(current_scale.y, fit_scale);
	pos.x = ((float)canvas_width - (float)cropped_width * fit_scale) * 0.5f;
	pos.y = ((float)canvas_height - (float)cropped_height * fit_scale) * 0.5f;
	if (signbit(scale.x))
		pos.x += (float)cropped_width * fit_scale;
	if (signbit(scale.y))
		pos.y += (float)cropped_height * fit_scale;
	if (parent_group_offset) {
		pos.x -= parent_group_offset->x;
		pos.y -= parent_group_offset->y;
	}

	if (parent_group)
		obs_sceneitem_defer_group_resize_begin(parent_group);
	obs_sceneitem_defer_update_begin(item);
	obs_sceneitem_set_bounds_type(item, OBS_BOUNDS_NONE);
	obs_sceneitem_set_alignment(item, OBS_ALIGN_LEFT | OBS_ALIGN_TOP);
	obs_sceneitem_set_scale(item, &scale);
	obs_sceneitem_set_pos(item, &pos);
	obs_sceneitem_defer_update_end(item);
	if (parent_group)
		obs_sceneitem_defer_group_resize_end(parent_group);

	blog(LOG_INFO,
		"[%s] fitted '%s' to %ux%u canvas (cropped source %ux%u, scale %.4f, flip_x=%s, flip_y=%s)",
		PLUGIN_NAME, obs_source_get_name(source), canvas_width, canvas_height,
		cropped_width, cropped_height, (double)fit_scale,
		signbit(scale.x) ? "true" : "false",
		signbit(scale.y) ? "true" : "false");
	return true;
}

static bool scene_item_callback(obs_scene_t *scene, obs_sceneitem_t *item,
		void *data)
{
	struct fit_context *ctx = data;
	struct fit_stats *stats = ctx->stats;
	struct fit_context child_ctx;
	struct vec2 group_pos;
	float rotation;
	bool unsafe_group;

	UNUSED_PARAMETER(scene);

	if (obs_sceneitem_is_group(item)) {
		unsafe_group = ctx->parent_group_unsafe ||
			!group_transform_is_translation_only(item, &group_pos);

		if (obs_sceneitem_selected(item)) {
			stats->skipped_group++;
			blog(LOG_INFO, "[%s] skipped selected group item", PLUGIN_NAME);
		}

		child_ctx = *ctx;
		child_ctx.parent_group = item;
		child_ctx.parent_group_unsafe = unsafe_group;
		if (!unsafe_group) {
			child_ctx.parent_group_offset.x += group_pos.x;
			child_ctx.parent_group_offset.y += group_pos.y;
		}
		obs_sceneitem_group_enum_items(item, scene_item_callback, &child_ctx);
		return true;
	}

	if (!obs_sceneitem_selected(item)) {
		stats->skipped_not_selected++;
		return true;
	}

	if (obs_sceneitem_locked(item)) {
		stats->skipped_locked++;
		blog(LOG_INFO, "[%s] skipped locked item", PLUGIN_NAME);
		return true;
	}

	if (ctx->parent_group_unsafe) {
		stats->skipped_group_transform++;
		blog(LOG_INFO,
			"[%s] skipped selected item under transformed group for safety",
			PLUGIN_NAME);
		return true;
	}

	rotation = normalize_rotation_degrees(obs_sceneitem_get_rot(item));
	if (!nearly_zero(rotation)) {
		stats->skipped_rotation++;
		blog(LOG_INFO, "[%s] skipped rotated item (rotation=%.3f)",
			PLUGIN_NAME, (double)rotation);
		return true;
	}

	if (fit_scene_item_to_canvas(item, ctx->canvas_width,
		    ctx->canvas_height, ctx->parent_group,
		    &ctx->parent_group_offset)) {
		stats->applied++;
	} else {
		stats->skipped_invalid_size++;
		blog(LOG_WARNING, "[%s] skipped item because effective size is invalid",
			PLUGIN_NAME);
	}

	return true;
}

static void fit_selected_sources(void *private_data)
{
	obs_source_t *scene_source;
	obs_scene_t *scene;
	struct fit_stats stats = {0};
	struct fit_context ctx;
	struct obs_video_info ovi;
	bool studio_mode;

	UNUSED_PARAMETER(private_data);

	if (!obs_get_video_info(&ovi)) {
		blog(LOG_WARNING, "[%s] failed to get OBS video info", PLUGIN_NAME);
		return;
	}

	studio_mode = obs_frontend_preview_program_mode_active();
	scene_source = NULL;
	if (studio_mode)
		scene_source = obs_frontend_get_current_preview_scene();
	if (!scene_source)
		scene_source = obs_frontend_get_current_scene();
	if (!scene_source) {
		blog(LOG_WARNING, "[%s] no target scene source found", PLUGIN_NAME);
		return;
	}

	scene = obs_scene_from_source(scene_source);
	if (!scene) {
		blog(LOG_WARNING, "[%s] failed to resolve target scene", PLUGIN_NAME);
		obs_source_release(scene_source);
		return;
	}

	ctx.stats = &stats;
	ctx.canvas_width = ovi.base_width;
	ctx.canvas_height = ovi.base_height;
	ctx.parent_group = NULL;
	ctx.parent_group_unsafe = false;
	ctx.parent_group_offset.x = 0.0f;
	ctx.parent_group_offset.y = 0.0f;

	obs_scene_enum_items(scene, scene_item_callback, &ctx);
	blog(LOG_INFO,
		"[%s] completed: applied=%u skipped_locked=%u skipped_group=%u skipped_group_transform=%u skipped_rotation=%u skipped_invalid_size=%u",
		PLUGIN_NAME, stats.applied, stats.skipped_locked, stats.skipped_group,
		stats.skipped_group_transform, stats.skipped_rotation,
		stats.skipped_invalid_size);

	if (!stats.applied && !stats.skipped_locked && !stats.skipped_group &&
	    !stats.skipped_group_transform && !stats.skipped_rotation &&
	    !stats.skipped_invalid_size) {
		blog(LOG_INFO, "[%s] no selected scene items found", PLUGIN_NAME);
	}

	obs_source_release(scene_source);
}

bool obs_module_load(void)
{
	obs_frontend_add_tools_menu_item(obs_module_text("Menu.FitSelectedSource"),
		fit_selected_sources, NULL);
	blog(LOG_INFO, "[%s] loaded", PLUGIN_NAME);
	return true;
}
