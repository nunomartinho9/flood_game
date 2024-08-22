
typedef struct Range1f {
  float min;
  float max;
} Range1f;
// ...

typedef struct Range2f {
  Vector2 min;
  Vector2 max;
} Range2f;

inline Range2f range2f_make(Vector2 min, Vector2 max) { return (Range2f) { min, max }; }

Range2f range2f_shift(Range2f r, Vector2 shift) {
  r.min = v2_add(r.min, shift);
  r.max = v2_add(r.max, shift);
  return r;
}

Range2f range2f_make_bottom_center(Vector2 size) {
  Range2f range = {0};
  range.max = size;
  range = range2f_shift(range, v2(size.x * -0.5, 0.0));
  return range;
}

Vector2 range2f_size(Range2f range) {
  Vector2 size = {0};
  size = v2_sub(range.min, range.max);
  size.x = fabsf(size.x);
  size.y = fabsf(size.y);
  return size;
}

bool range2f_contains(Range2f range, Vector2 v) {
  return v.x >= range.min.x && v.x <= range.max.x && v.y >= range.min.y && v.y <= range.max.y;
}

// :example


					/* 					
										Range2f bounds = range2f_make_bottom_center(sprite->size);
										bounds = range2f_shift(bounds, en->pos);
										bounds.min = v2_sub(bounds.min, v2(10.0, 10.0));
										bounds.max = v2_add(bounds.max, v2(10.0, 10.0));
										Vector4 col = COLOR_RED;
										col.a = 0.4;
										if (range2f_contains(bounds, mouse_pos_world))
										{
											col.a = 1.0;
										}
										draw_rect(bounds.min, range2f_size(bounds), col); */