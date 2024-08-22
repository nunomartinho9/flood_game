#include "./range_n.c"
const int tile_width = 8;
const float entity_selection_radius = 16.0f;
const int rock_health = 5;
const int tree_health = 3;

inline float v2_dist(Vector2 a, Vector2 b)
{
	return v2_length(v2_sub(a, b));
}


int world_pos_to_tile_pos(float world_pos)
{
	return roundf(world_pos / (float)tile_width);
}

float tile_pos_to_world_pos(int tile_pos)
{
	return ((float)tile_pos * (float)tile_width);
}

Vector2 round_v2_to_tile(Vector2 world_pos)
{
	world_pos.x = tile_pos_to_world_pos(world_pos_to_tile_pos(world_pos.x));
	world_pos.y = tile_pos_to_world_pos(world_pos_to_tile_pos(world_pos.y));
	return world_pos;
}

bool almost_equals(float a, float b, float epsilon)
{
	return fabs(a - b) <= epsilon;
}

bool animate_f32_to_target(float *value, float target, float delta_t, float rate)
{
	*value += (target - *value) * (1.0 - pow(2.0f, -rate * delta_t));
	if (almost_equals(*value, target, 0.001f))
	{
		*value = target;
		return true; // reached
	}
	return false;
}

void animate_v2_to_target(Vector2 *value, Vector2 target, float delta_t, float rate)
{
	animate_f32_to_target(&(value->x), target.x, delta_t, rate);
	animate_f32_to_target(&(value->y), target.y, delta_t, rate);
}

typedef struct Sprite
{
	Gfx_Image *image;

} Sprite;
typedef enum SpriteID
{
	SPRITE_nil,
	SPRITE_player,
	SPRITE_tree0,
	SPRITE_tree1,
	SPRITE_rock0,
	SPRITE_item_soul,
	SPRITE_MAX,
} SpriteID;
// nuno: maybe we make this an X macro?? https://chatgpt.com/share/260222eb-2738-4d1e-8b1d-4973a097814d
Sprite sprites[SPRITE_MAX];

Sprite *get_sprite(SpriteID id)
{
	if (id >= 0 && id < SPRITE_MAX)
	{
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
	arch_item_soul = 4,
	ARCH_MAX
} EntityArchetype;

typedef struct Entity
{
	Vector2 pos;
	bool is_valid;
	EntityArchetype arch;

	bool render_sprite;
	SpriteID sprite_id;
	int health;
	//int item_count;
	bool destroyable_world_item;

} Entity;
#define MAX_ENTITY_COUNT 1024
typedef struct World
{
	Entity entities[MAX_ENTITY_COUNT];

} World;
World *world = 0;

typedef struct WorldFrame
{
	Entity *selected_entity;
} WorldFrame;
WorldFrame world_frame;

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
	entity->health = rock_health;
	entity->destroyable_world_item = true;
	//....
}

void setup_tree(Entity *entity)
{
	entity->arch = arch_tree;
	entity->sprite_id = SPRITE_tree0;
	entity->health = tree_health;
	entity->destroyable_world_item = true;
	//....
}

void setup_player(Entity *entity)
{
	entity->arch = arch_player;
	entity->sprite_id = SPRITE_player;
	entity->destroyable_world_item = false;
	//....
}

void setup_item_soul(Entity *entity) {
	entity->arch = arch_item_soul;
	entity->sprite_id = SPRITE_item_soul;
	entity->destroyable_world_item = false;
}

Vector2 screen_to_world()
{
	float mouse_x = input_frame.mouse_x;
	float mouse_y = input_frame.mouse_y;

	Matrix4 proj = draw_frame.projection;
	Matrix4 view = draw_frame.camera_xform;
	float window_w = window.width;
	float window_h = window.height;

	// Normalize the mouse coordinates
	float ndc_x = (mouse_x / (window_w * 0.5f)) - 1.0f;
	float ndc_y = (mouse_y / (window_h * 0.5f)) - 1.0f;

	// Transform to world coords
	Vector4 world_pos = v4(ndc_x, ndc_y, 0, 1);
	world_pos = m4_transform(m4_inverse(proj), world_pos);
	world_pos = m4_transform(view, world_pos);
	// log("%f, %f", world_pos.x, world_pos.y);

	// Return the 2d vector
	return (Vector2){world_pos.x, world_pos.y};
}
bool in_debug = false;
void toggle_game_debug()
{
	in_debug = !in_debug;
}

Vector2 get_sprite_size(Sprite* sprite) {
	return v2(sprite->image->width, sprite->image->height);
}

int entry(int argc, char **argv)
{

	window.title = STR("DANTES INF GAME SURVIVAL");
	window.scaled_width = 1280; // We need to set the scaled size if we want to handle system scaling (DPI)
	window.scaled_height = 720;
	window.x = 200;
	window.y = 90;
	window.clear_color = hex_to_rgba(0x0b0b0bff);

	float64 last_time = os_get_elapsed_seconds();
	const float64 fps_limit = 69000;
	const float64 min_frametime = 1.0 / fps_limit;
	const float speed = 65.0;

	world = alloc(get_heap_allocator(), sizeof(World));
	memset(world, 0, sizeof(World));

	sprites[SPRITE_player] = (Sprite){.image = load_image_from_disk(fixed_string("res/sprites/player.png"), get_heap_allocator()), };
	sprites[SPRITE_tree0] = (Sprite){.image = load_image_from_disk(fixed_string("res/sprites/tree0.png"), get_heap_allocator()), };
	sprites[SPRITE_rock0] = (Sprite){.image = load_image_from_disk(fixed_string("res/sprites/rock0.png"), get_heap_allocator()), };
	sprites[SPRITE_item_soul] = (Sprite){.image = load_image_from_disk(fixed_string("res/sprites/soul.png"), get_heap_allocator()), };

	Gfx_Font *font = load_font_from_disk(STR("C:/windows/fonts/arial.ttf"), get_heap_allocator());
	assert(font, "Failed to load arial.ttf", GetLastError());
	const u32 font_height = 32;

	// :entity setup
	Entity *player_en = entity_create();
	setup_player(player_en);
	for (int i = 0; i < 10; i++)
	{
		Entity *en = entity_create();
		setup_rock(en);
		en->pos = v2(get_random_float32_in_range(-100, 100), get_random_float32_in_range(-100, 100));
		en->pos = round_v2_to_tile(en->pos);
		// en->pos.y -= tile_width * 0.5;
	}

	for (int i = 0; i < 4; i++)
	{
		Entity *en = entity_create();
		setup_tree(en);
		en->pos = v2(get_random_float32_in_range(-100, 100), get_random_float32_in_range(-100, 100));
		en->pos = round_v2_to_tile(en->pos);
		// en->pos.y -= tile_width * 0.5;
	}

	float zoom = 5.3;
	Vector2 camera_pos = v2(0, 0);

	// this while is the game running (every single frame)
	while (!window.should_close)
	{
		reset_temporary_storage();
		world_frame = (WorldFrame){0};
		float64 now = os_get_elapsed_seconds();
		if (in_debug)
		{
			if ((int)now != (int)last_time)
				log("%.2f FPS\n%.2fms", 1.0 / (now - last_time), (now - last_time) * 1000); //show fps
		}

		float64 delta = now - last_time;
		if (delta < min_frametime)
		{
			os_high_precision_sleep((min_frametime - delta) * 1000.0);
			now = os_get_elapsed_seconds();
			delta = now - last_time;
		}

		last_time = now;

		draw_frame.projection = m4_make_orthographic_projection(window.width * -0.5, window.width * 0.5, window.height * -0.5, window.height * 0.5, -1, 10);

		// :camera
		{
			Vector2 target_pos = player_en->pos;
			animate_v2_to_target(&camera_pos, target_pos, delta, 25.0f);

			draw_frame.camera_xform = m4_make_scale(v3(1.0, 1.0, 1.0));
			draw_frame.camera_xform = m4_mul(draw_frame.camera_xform, m4_make_translation(v3(camera_pos.x, camera_pos.y, 0)));
			draw_frame.camera_xform = m4_mul(draw_frame.camera_xform, m4_make_scale(v3(1.0 / zoom, 1.0 / zoom, 1.0)));
		}

		// :select entity
		Vector2 mouse_pos_world = screen_to_world();
		int mouse_tile_x = world_pos_to_tile_pos(mouse_pos_world.x);
		int mouse_tile_y = world_pos_to_tile_pos(mouse_pos_world.y);
		{

			if (in_debug)
			{
				log("%f, %f", mouse_pos_world.x, mouse_pos_world.y);
				draw_text(font, sprint(get_temporary_allocator(), "%f, %f", mouse_pos_world.x, mouse_pos_world.y), font_height, mouse_pos_world, v2(0.1, 0.1), COLOR_RED);
			}

			float smallest_dist = INFINITY;
			for (int i = 0; i < MAX_ENTITY_COUNT; i++)
			{
				Entity *en = &world->entities[i];
				if (en->is_valid && en->destroyable_world_item)
				{
					Sprite *sprite = get_sprite(en->sprite_id);
					int entity_tile_x = world_pos_to_tile_pos(en->pos.x);
					int entity_tile_y = world_pos_to_tile_pos(en->pos.y);
					float dist = fabsf(v2_dist(en->pos, mouse_pos_world));
					if (dist < entity_selection_radius)
					{

						if (!world_frame.selected_entity || (dist < smallest_dist))
						{
							world_frame.selected_entity = en;
							smallest_dist = dist;
						}
					}
				}
			}
		}

		// :tile rendering
		{
			int player_tile_x = world_pos_to_tile_pos(player_en->pos.x);
			int player_tile_y = world_pos_to_tile_pos(player_en->pos.y);
			int tile_radius_x = 40;
			int tile_radius_y = 30;
			for (int x = player_tile_x - tile_radius_x; x < player_tile_x + tile_radius_x; x++)
			{
				for (int y = player_tile_y - tile_radius_y; y < player_tile_y + tile_radius_y; y++)
				{
					if ((x + (y % 2 == 0)) % 2 == 0)
					{
						Vector4 col = v4(0.1, 0.1, 0.1, 0.1);
						float y_pos = y * tile_width;
						float x_pos = x * tile_width;
						draw_rect(v2(x_pos + tile_width * -0.5, y_pos + tile_width * -0.5), v2(tile_width, tile_width), col);
					}
				}
			}

			// draw_rect(v2(tile_pos_to_world_pos(mouse_tile_x) + tile_width * -0.5, tile_pos_to_world_pos(mouse_tile_y) + tile_width * -0.5), v2(tile_width, tile_width), v4(0.5, 0.5, 0.5, 0.5));
		}

		// :click items
		{
			Entity* selected_en = world_frame.selected_entity;
			if (is_key_just_pressed(MOUSE_BUTTON_LEFT))
			{
				log("pressed left mb");
				consume_key_just_pressed(MOUSE_BUTTON_LEFT);

				if (selected_en) {
					selected_en->health -= 1;
					if (selected_en->health <= 0) {
						switch(selected_en->arch) {
							case arch_tree: {
								Entity* en = entity_create();
								setup_item_soul(en);
								// en->item_count = 3;
								en->pos = selected_en->pos;
								break;
							}

							default: break;
						}
						
						
						entity_destroy(selected_en);


					}
				}
			}
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
					xform = m4_translate(xform, v3(0, tile_width * -0.5, 0));
					xform = m4_translate(xform, v3(en->pos.x, en->pos.y, 0));
					xform = m4_translate(xform, v3(sprite->image->width * -0.5, 0.0, 0));

					Vector4 color = COLOR_WHITE;

					if (world_frame.selected_entity == en)
					{
						color = COLOR_RED;
					}
					draw_image_xform(sprite->image, xform, get_sprite_size(sprite), color);

					if (in_debug)
					{
						draw_text(font, sprint(get_temporary_allocator(), "%f, %f", en->pos.x, en->pos.y), font_height, en->pos, v2(0.1, 0.1), COLOR_RED);
					}
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
		if (is_key_just_pressed(KEY_F1))
		{
			toggle_game_debug();
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