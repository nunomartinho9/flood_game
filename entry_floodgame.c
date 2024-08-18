bool almost_equals(float a, float b, float epsilon) {
 return fabs(a - b) <= epsilon;
}

bool animate_f32_to_target(float* value, float target, float delta_t, float rate) {
	*value += (target - *value) * (1.0 - pow(2.0f, -rate * delta_t));
	if (almost_equals(*value, target, 0.001f))
	{
		*value = target;
		return true; // reached
	}
	return false;
}

void animate_v2_to_target(Vector2* value, Vector2 target, float delta_t, float rate) {
	animate_f32_to_target(&(value->x), target.x, delta_t, rate);
	animate_f32_to_target(&(value->y), target.y, delta_t, rate);
}


typedef struct Sprite
{
	Gfx_Image *image;
	Vector2 size;

} Sprite;
typedef enum SpriteID
{
	SPRITE_nil,
	SPRITE_player,
	SPRITE_tree0,
	SPRITE_tree1,
	SPRITE_rock0,
	SPRITE_MAX,
} SpriteID;
// nuno: maybe we make this an X macro?? https://chatgpt.com/share/260222eb-2738-4d1e-8b1d-4973a097814d
Sprite sprites[SPRITE_MAX];

Sprite* get_sprite(SpriteID id) {
	if (id >= 0 && id < SPRITE_MAX) {
		return &sprites[id];
	}

	return &sprites[0];
}
typedef enum EntityArchetype
{
	arch_nil = 0,
	arch_rock = 1,
	arch_tree = 2,
	arch_player = 3,
} EntityArchetype;

typedef struct Entity
{
	Vector2 pos;
	bool is_valid;
	EntityArchetype arch;

	bool render_sprite;
	SpriteID sprite_id;

} Entity;
#define MAX_ENTITY_COUNT 1024
typedef struct World
{
	Entity entities[MAX_ENTITY_COUNT];

} World;
World *world = 0;

Entity *entity_create()
{
	Entity *entity_found = 0;
	for (int i = 0; i < MAX_ENTITY_COUNT; i++)
	{
		Entity *existing_entity = &world->entities[i];
		if (!existing_entity->is_valid)
		{
			entity_found = existing_entity;
			break;
		}
	}
	assert(entity_found, "No more free entities!");
	entity_found->is_valid = true;
	return (entity_found);
}

void entity_destroy(Entity *entity)
{
	memset(entity, 0, sizeof(Entity));
}

void setup_rock(Entity *entity)
{
	entity->arch = arch_rock;
	entity->sprite_id = SPRITE_rock0;
	//....
}

void setup_tree(Entity *entity)
{
	entity->arch = arch_tree;
	entity->sprite_id = SPRITE_tree0;
	//....
}

void setup_player(Entity *entity)
{
	entity->arch = arch_player;
	entity->sprite_id = SPRITE_player;
	//....
}


int entry(int argc, char **argv)
{

	window.title = STR("DANTES INF GAME SURVIVAL");
	window.scaled_width = 1280; // We need to set the scaled size if we want to handle system scaling (DPI)
	window.scaled_height = 720;
	window.x = 200;
	window.y = 90;
	window.clear_color = hex_to_rgba(0x0b0b0bff);

	float64 last_time = os_get_current_time_in_seconds();
	const float64 fps_limit = 69000;
	const float64 min_frametime = 1.0 / fps_limit;
	const float speed = 65.0;

	world = alloc(get_heap_allocator(), sizeof(World));
	memset(world, 0, sizeof(World));

	sprites[SPRITE_player] = (Sprite){.image = load_image_from_disk(fixed_string("player.png"), get_heap_allocator()), .size = v2(14, 17)};
	sprites[SPRITE_tree0] = (Sprite){.image = load_image_from_disk(fixed_string("tree0.png"), get_heap_allocator()), .size = v2(11, 13)};
	sprites[SPRITE_rock0] = (Sprite){.image = load_image_from_disk(fixed_string("rock0.png"), get_heap_allocator()), .size = v2(9, 5)};

	Entity *player_en = entity_create();
	setup_player(player_en);
	for (int i = 0; i < 10; i++)
	{
		Entity *en = entity_create();
		setup_rock(en);
		en->pos = v2(get_random_float32_in_range(-100, 100), get_random_float32_in_range(-100, 100));
	}

	for (int i = 0; i < 4; i++)
	{
		Entity *en = entity_create();
		setup_tree(en);
		en->pos = v2(get_random_float32_in_range(-100, 100), get_random_float32_in_range(-100, 100));
	}


	float zoom = 5.3;
	Vector2 camera_pos = v2(0,0);

	// this while is the game running (every single frame)
	while (!window.should_close)
	{
		reset_temporary_storage();

		float64 now = os_get_current_time_in_seconds();
		if ((int)now != (int)last_time)
			log("%.2f FPS\n%.2fms", 1.0 / (now - last_time), (now - last_time) * 1000); // fps
		float64 delta = now - last_time;
		if (delta < min_frametime)
		{
			os_high_precision_sleep((min_frametime - delta) * 1000.0);
			now = os_get_current_time_in_seconds();
			delta = now - last_time;
		}
		
		last_time = now;

		draw_frame.projection = m4_make_orthographic_projection(window.width * -0.5, window.width * 0.5, window.height * -0.5, window.height * 0.5, -1, 10);

		// :camera
		{
			Vector2 target_pos = player_en->pos;
			animate_v2_to_target(&camera_pos, target_pos, delta, 25.0f);

			draw_frame.view = m4_make_scale(v3(1.0, 1.0, 1.0));
			draw_frame.view = m4_mul(draw_frame.view, m4_make_translation(v3(camera_pos.x, camera_pos.y, 0)));
			draw_frame.view = m4_mul(draw_frame.view, m4_make_scale(v3(1.0 / zoom, 1.0 / zoom, 1.0)));
		}



		// :render images for each arch type
		for (int i = 0; i < MAX_ENTITY_COUNT; i++)
		{
			Entity *en = &world->entities[i];

			if (en->is_valid)
			{
				switch (en->arch)
				{

				default:
				{
					Sprite *sprite = get_sprite(en->sprite_id);
					Matrix4 xform = m4_scalar(1.0);
					xform = m4_translate(xform, v3(en->pos.x, en->pos.y, 0));
					xform = m4_translate(xform, v3(sprite->size.x * -0.5, 0.0, 0));
					draw_image_xform(sprite->image, xform, sprite->size, COLOR_WHITE);
				}
				break;
				}
			}
		}

		if (is_key_just_released(KEY_ESCAPE))
		{
			window.should_close = true;
		}

		// getting the input of the player
		Vector2 input_axis = v2(0, 0);
		if (is_key_down('A'))
		{
			input_axis.x -= 1.0;
		}
		if (is_key_down('D'))
		{
			input_axis.x += 1.0;
		}
		if (is_key_down('S'))
		{
			input_axis.y -= 1.0;
		}
		if (is_key_down('W'))
		{
			input_axis.y += 1.0;
		}
		input_axis = v2_normalize(input_axis);
		// adding to the vector2 (player_pos) the player pos + the input (direction) times the speed
		player_en->pos = v2_add(player_en->pos, v2_mulf(input_axis, speed * delta)); // 0.0002f is a constant basically the speed of the player movement
		/*
		Matrix4 rect_xform = m4_scalar(1.0);
		rect_xform         = m4_rotate_z(rect_xform, (f32)now);
		rect_xform         = m4_translate(rect_xform, v3(-.25f, -.25f, 0));
		draw_rect_xform(rect_xform, v2(.5f, .5f), COLOR_GREEN);

		draw_rect(v2(sin(now), -.8), v2(.5, .25), COLOR_RED); */

		/*float aspect = (f32)window.width/(f32)window.height;
		float mx = (input_frame.mouse_x/(f32)window.width  * 2.0 - 1.0)*aspect;
		float my = input_frame.mouse_y/(f32)window.height * 2.0 - 1.0; */

		// draw_line(v2(-.75, -.75), v2(mx, my), 0.005, COLOR_WHITE);

		os_update();
		gfx_update();
	}

	return 0;
}