
int entry(int argc, char **argv) {
	
	window.title = STR("FLOOD GAME SURVIVAL");
	window.scaled_width = 1280; // We need to set the scaled size if we want to handle system scaling (DPI)
	window.scaled_height = 720; 
	window.x = 200;
	window.y = 90;
	window.clear_color = hex_to_rgba(0x0b0b0bff);

	float64 last_time = os_get_current_time_in_seconds();
	const float64 fps_limit = 69000;
	const float64 min_frametime = 1.0 / fps_limit;
	const float speed = 50.0;
	Gfx_Image *player_image = load_image_from_disk(fixed_string("player.png"), get_heap_allocator());
	assert(player_image, "player.png failed to load.");

	Vector2 player_pos = v2(0, 0);

	
	//this while is the game running (every single frame)
	while (!window.should_close) {

		draw_frame.projection = m4_make_orthographic_projection(window.width * -0.5, window.width * 0.5, window.height * -0.5, window.height * 0.5, -1, 10);
		float zoom = 10;
		draw_frame.view = m4_make_scale(v3(1.0/zoom, 1.0/zoom, 1.0));

		float64 now = os_get_current_time_in_seconds();
		if ((int)now != (int)last_time) log("%.2f FPS\n%.2fms", 1.0/(now-last_time), (now-last_time)*1000); //fps
		float64 delta = now - last_time;
		if (delta < min_frametime) {
			os_high_precision_sleep((min_frametime-delta)*1000.0);
			now = os_get_current_time_in_seconds();
			delta = now - last_time;
		}
		last_time = now;
		
		reset_temporary_storage();
		
		if (is_key_just_released(KEY_ESCAPE)) {
			window.should_close = true;
		}

		//getting the input of the player
		Vector2 input_axis = v2(0, 0);
		if (is_key_down('A')) {
			input_axis.x -= 1.0;
		}
		if (is_key_down('D')) {
			input_axis.x += 1.0;
		}
		if (is_key_down('S')) {
			input_axis.y -= 1.0;
		}
		if (is_key_down('W')) {
			input_axis.y += 1.0;
		}
		input_axis = v2_normalize(input_axis);
		//adding to the vector2 (player_pos) the player pos + the input (direction) times the speed
		player_pos = v2_add(player_pos, v2_mulf(input_axis, speed * delta)); //0.0002f is a constant basically the speed of the player movement
		/* 
		Matrix4 rect_xform = m4_scalar(1.0);
		rect_xform         = m4_rotate_z(rect_xform, (f32)now);
		rect_xform         = m4_translate(rect_xform, v3(-.25f, -.25f, 0));
		draw_rect_xform(rect_xform, v2(.5f, .5f), COLOR_GREEN);
		
		draw_rect(v2(sin(now), -.8), v2(.5, .25), COLOR_RED); */
		
        /*float aspect = (f32)window.width/(f32)window.height;
		float mx = (input_frame.mouse_x/(f32)window.width  * 2.0 - 1.0)*aspect;
		float my = input_frame.mouse_y/(f32)window.height * 2.0 - 1.0; */
		
		//draw_line(v2(-.75, -.75), v2(mx, my), 0.005, COLOR_WHITE);

		Vector2 size = v2(14, 17);
		Matrix4 player_xform = m4_scalar(.3);
		player_xform         = m4_translate(player_xform, v3(player_pos.x, player_pos.y, 0));
		draw_image_xform(player_image, player_xform, size, COLOR_WHITE);
		
		os_update(); 
		gfx_update();
	}

	return 0;
}