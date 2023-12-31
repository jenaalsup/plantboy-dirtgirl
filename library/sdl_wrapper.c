#include "sdl_wrapper.h"
#include "body.h"
#include "scene.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <SDL2/SDL_image.h>
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <time.h> 
#include <SDL2/SDL_mixer.h>

#define MUS_PATH "images/Powerful-Trap-.wav"
#define WAV_PATH_1 "images/Powerful-Trap-.wav"
#define PORTAL_SOUND "images/Portal_new.wav"
#define WIN_SOUND "images/Win.wav"
#define DIED_SOUND "images/Died.wav"
#define STAR_OFM_SOUND "images/Star_ofM.wav"
#define FERTILISER_SOUND "images/Fertiliser.wav"
#define TRAMPLOINE_SOUND "images/Trampoline_new.wav"
#define ICE_SOUND "images/ice_sound.wav"

typedef enum {
  PORTAL = 1,
  WIN = 2,
  DIED = 3,
  STAR_OFM = 4,
  FERTILIZER = 5,
  TRAMPOLINE = 6,
  ICE = 7,
} sound_t;

//SOUND STUFF
Mix_Music *music = NULL;
Mix_Chunk *wave = NULL;

typedef enum {
  PLANT_BOY = 1,
  DIRT_GIRL = 2,
  BOY_FERTILIZER = 13,
  GIRL_FERTILIZER = 14,
  STAR = 15,
  TREE = 16,
} info_t;

const char WINDOW_TITLE[] = "CS 3";
const int WINDOW_WIDTH = 1000;
const int WINDOW_HEIGHT = 500;
const double MS_PER_S = 1e3;

//BACKGROUND
const char *BG = "images/background_2.jpeg";
const char *BG_START = "images/start_screen.jpg";
const char *BG_WIN = "images/end_screen.jpeg";

//SPRITES:
const char *DIRT_GIRL_SPRITE = "images/dirt_girl_front_facing.png";
const char *PLANT_BOY_SPRITE = "images/plant_boy_front_facing-removebg-preview.png";
const char *TREE_SPRITE = "images/tree.png";

//POWERUPS/BOOSTS:
const char *DIRT_GIRL_FERTILIZER = "images/dirt_girl_fertilizer.png"; 
const char *PLANT_BOY_FERTILIZER = "images/plant_boy_fertilizer.png";
const char *STAR_OF_MASTERY = "images/star_of_mastery.png";

//DEATH COUNT TREES:
const char *DEATH_COUNT_1 = "images/death_count_1.png";
const char *DEATH_COUNT_2 = "images/death_count_2.png";
const char *DEATH_COUNT_3 = "images/death_count_3.png";
const char *DEATH_COUNT_3_PLUS = "images/death_count_3_plus.png";

//POSITIONS:
const SDL_Rect SPRITE_RECT = {500,500,80,80};
const SDL_Rect FERTILIZER_RECT = {0,0,50,50};
const SDL_Rect TREE_RECT = {0,0,140,140};
const SDL_Rect OBJECT_RECT = {0,0,50,50};

//OTHER CONSTANTS:
const int ADJUSTMENT = 35; //in pixels
const int SCENE_SCALE = 140;
const int COLOR_CONSTANT = 255;

/**
 * The coordinate at the center of the screen.
 */
vector_t center;
/**
 * The coordinate difference from the center to the top right corner.
 */
vector_t max_diff;
/**
 * The SDL window where the scene is rendered.
 */
SDL_Window *window;
/**
 * The renderer used to draw the scene.
 */
SDL_Renderer *renderer;
/**
 * The keypress handler, or NULL if none has been configured.
 */
key_handler_t key_handler = NULL;
/**
 * SDL's timestamp when a key was last pressed or released.
 * Used to mesasure how long a key has been held.
 */
uint32_t key_start_timestamp;
/**
 * The value of clock() when time_since_last_tick() was last called.
 * Initially 0.
 */
clock_t last_clock = 0;

/** Computes the center of the window in pixel coordinates */
vector_t get_window_center(void) {
  int *width = malloc(sizeof(*width)), *height = malloc(sizeof(*height));
  assert(width != NULL);
  assert(height != NULL);
  SDL_GetWindowSize(window, width, height);
  vector_t dimensions = {.x = *width, .y = *height};
  free(width);
  free(height);
  return vec_multiply(0.5, dimensions);
}

/**
 * Computes the scaling factor between scene coordinates and pixel coordinates.
 * The scene is scaled by the same factor in the x and y dimensions,
 * chosen to maximize the size of the scene while keeping it in the window.
 */
double get_scene_scale(vector_t window_center) {
  double x_scale = window_center.x / max_diff.x,
         y_scale = window_center.y / max_diff.y;
  return x_scale < y_scale ? x_scale : y_scale;
}

/** Maps a scene coordinate to a window coordinate */
vector_t get_window_position(vector_t scene_pos, vector_t window_center) {
  vector_t scene_center_offset = vec_subtract(scene_pos, center);
  double scale = get_scene_scale(window_center);
  vector_t pixel_center_offset = vec_multiply(scale, scene_center_offset);
  vector_t pixel = {.x = round(window_center.x + pixel_center_offset.x),
                    .y = round(window_center.y - pixel_center_offset.y)};
  return pixel;
}

/**
 * Converts an SDL key code to a char.
 * 7-bit ASCII characters are just returned
 * and arrow keys are given special character codes.
 */
char get_keycode(SDL_Keycode key) {
  switch (key) {
  case SDLK_LEFT:
    return LEFT_ARROW;
  case SDLK_UP:
    return UP_ARROW;
  case SDLK_RIGHT:
    return RIGHT_ARROW;
  case SDLK_DOWN:
    return DOWN_ARROW;
  case SDLK_w:
    return W_KEY;
  case SDLK_a:
    return A_KEY;
  case SDLK_d:
    return D_KEY;
  default:
    // Only process 7-bit ASCII characters
    return key == (SDL_Keycode)(char)key ? key : '\0';
  }
}

void sdl_init(vector_t min, vector_t max) {
  assert(min.x < max.x);
  assert(min.y < max.y);

  center = vec_multiply(0.5, vec_add(min, max));
  max_diff = vec_subtract(max, center);
  SDL_Init(SDL_INIT_EVERYTHING);
  window = SDL_CreateWindow(WINDOW_TITLE, SDL_WINDOWPOS_CENTERED,
                            SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT,
                            SDL_WINDOW_RESIZABLE);
  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
}

bool sdl_is_done(state_t *state) {
  SDL_Event *event = malloc(sizeof(*event));
  assert(event != NULL);
  while (SDL_PollEvent(event)) {
    switch (event->type) {
    case SDL_QUIT:
      free(event);
      return true;
    case SDL_KEYDOWN:
    case SDL_KEYUP:
      if (key_handler == NULL)
        break;
      char key = get_keycode(event->key.keysym.sym);
      if (key == '\0')
        break;

      uint32_t timestamp = event->key.timestamp;
      if (!event->key.repeat) {
        key_start_timestamp = timestamp;
      }
      key_event_type_t type =
          event->type == SDL_KEYDOWN ? KEY_PRESSED : KEY_RELEASED;
      double held_time = (timestamp - key_start_timestamp) / MS_PER_S;
      key_handler(key, type, held_time, state);
      break;
    }
  }
  free(event);
  return false;
}

void sdl_clear(void) {
  SDL_SetRenderDrawColor(renderer, COLOR_CONSTANT, COLOR_CONSTANT, COLOR_CONSTANT, COLOR_CONSTANT);
  SDL_RenderClear(renderer);
}

void sdl_draw_polygon(list_t *points, rgb_color_t color) {
  size_t n = list_size(points);
  assert(n >= 3);
  assert(0 <= color.r && color.r <= 1);
  assert(0 <= color.g && color.g <= 1);
  assert(0 <= color.b && color.b <= 1);

  vector_t window_center = get_window_center();

  // Convert each vertex to a point on screen
  int16_t *x_points = malloc(sizeof(*x_points) * n),
          *y_points = malloc(sizeof(*y_points) * n);
  assert(x_points != NULL);
  assert(y_points != NULL);
  for (size_t i = 0; i < n; i++) {
    vector_t *vertex = list_get(points, i);
    vector_t pixel = get_window_position(*vertex, window_center);
    x_points[i] = pixel.x;
    y_points[i] = pixel.y;
  }

  // Draw polygon with the given color
  filledPolygonRGBA(renderer, x_points, y_points, n, color.r * COLOR_CONSTANT,
                    color.g * COLOR_CONSTANT, color.b * COLOR_CONSTANT, COLOR_CONSTANT);
  free(x_points);
  free(y_points);
}

void sdl_show(void) {
  // Draw boundary lines
  vector_t window_center = get_window_center();
  vector_t max = vec_add(center, max_diff),
           min = vec_subtract(center, max_diff);
  vector_t max_pixel = get_window_position(max, window_center),
           min_pixel = get_window_position(min, window_center);
  SDL_Rect *boundary = malloc(sizeof(*boundary));
  boundary->x = min_pixel.x;
  boundary->y = max_pixel.y;
  boundary->w = max_pixel.x - min_pixel.x;
  boundary->h = min_pixel.y - max_pixel.y;
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, COLOR_CONSTANT);
  SDL_RenderDrawRect(renderer, boundary);
  free(boundary);

  SDL_RenderPresent(renderer);
}

void sdl_render_scene(scene_t *scene) {
  SDL_Texture *PLANT_BOY_TEXTURE = IMG_LoadTexture(renderer, PLANT_BOY_SPRITE);
  SDL_Rect plant_boy_rect = SPRITE_RECT;

  SDL_Texture *DIRT_GIRL_TEXTURE = IMG_LoadTexture(renderer, DIRT_GIRL_SPRITE);
  SDL_Rect dirt_girl_rect = SPRITE_RECT;

  SDL_Rect tree_rect = TREE_RECT;

  SDL_Texture *DIRT_GIRL_FERTILIZER_TEXTURE = IMG_LoadTexture(renderer, DIRT_GIRL_FERTILIZER);
  SDL_Rect dirt_girl_fertilizer_rect = FERTILIZER_RECT;

  SDL_Texture *PLANT_BOY_FERTILIZER_TEXTURE = IMG_LoadTexture(renderer, PLANT_BOY_FERTILIZER);
  SDL_Rect plant_boy_fertilizer_rect = FERTILIZER_RECT;

  SDL_Texture *STAR_OF_MASTERY_TEXTURE;
  if (star_in_scene(scene)) {
    STAR_OF_MASTERY_TEXTURE = IMG_LoadTexture(renderer, STAR_OF_MASTERY);
  }
  SDL_Rect star_of_mastery_rect = OBJECT_RECT;

  sdl_clear();

  SDL_Texture *DEATH_TREE_TEXTURE;
  if ((int)scene_get_num_deaths(scene) == 1) {
    DEATH_TREE_TEXTURE = IMG_LoadTexture(renderer, DEATH_COUNT_1);
  } 
  if ((int)scene_get_num_deaths(scene) == 2) {
    DEATH_TREE_TEXTURE = IMG_LoadTexture(renderer, DEATH_COUNT_2);
  } 
  if ((int)scene_get_num_deaths(scene) == 3) {
    DEATH_TREE_TEXTURE = IMG_LoadTexture(renderer, DEATH_COUNT_3);
  }
  if ((int)scene_get_num_deaths(scene) > 3) {
    DEATH_TREE_TEXTURE = IMG_LoadTexture(renderer, DEATH_COUNT_3_PLUS);
  }

  SDL_Texture *BG_TEXTURE;
  if ((int)scene_get_screen(scene) == 0) {
    BG_TEXTURE = IMG_LoadTexture(renderer, BG_START);
  } 
  if ((int)scene_get_screen(scene) == 1) {
    BG_TEXTURE = IMG_LoadTexture(renderer, BG);
  } 
  if ((int)scene_get_screen(scene) == 2) {
    BG_TEXTURE = IMG_LoadTexture(renderer, BG_WIN);
  } 

  size_t body_count = scene_bodies(scene);
  for (size_t i = 0; i < body_count; i++) {
    body_t *body = scene_get_body(scene, i);
    list_t *shape = body_get_shape(body);
    vector_t centroid = body_get_centroid(body);
    vector_t window = get_window_position(centroid, get_window_center());

    //SPRITES:
    if ((int)body_get_info(body) == PLANT_BOY) {
      plant_boy_rect.x = window.x - SCENE_SCALE * get_scene_scale(get_window_center()) + ADJUSTMENT;
      plant_boy_rect.y = window.y - SCENE_SCALE * get_scene_scale(get_window_center()) + 10;
    }
    if ((int)body_get_info(body) == DIRT_GIRL) {
      dirt_girl_rect.x = window.x - SCENE_SCALE * get_scene_scale(get_window_center()) + ADJUSTMENT;
      dirt_girl_rect.y = window.y - SCENE_SCALE * get_scene_scale(get_window_center()) + 10;
    }
    if ((int)body_get_info(body) == TREE) {
      tree_rect.x = window.x - SCENE_SCALE * get_scene_scale(get_window_center()) + ADJUSTMENT;
      tree_rect.y = window.y - SCENE_SCALE * get_scene_scale(get_window_center()) + ADJUSTMENT;
    }
    //POWERUPS:
    if ((int)body_get_info(body) == BOY_FERTILIZER) {
      plant_boy_fertilizer_rect.x = window.x - SCENE_SCALE * get_scene_scale(get_window_center()) + ADJUSTMENT;
      plant_boy_fertilizer_rect.y = window.y - SCENE_SCALE * get_scene_scale(get_window_center()) + ADJUSTMENT;
    }
    if ((int)body_get_info(body) == GIRL_FERTILIZER) {
      dirt_girl_fertilizer_rect.x = window.x - SCENE_SCALE * get_scene_scale(get_window_center()) + ADJUSTMENT;
      dirt_girl_fertilizer_rect.y = window.y - SCENE_SCALE * get_scene_scale(get_window_center()) + ADJUSTMENT;
    }
    if ((int)body_get_info(body) == STAR) {
      star_of_mastery_rect.x = window.x - SCENE_SCALE * get_scene_scale(get_window_center());
      star_of_mastery_rect.y = window.y - SCENE_SCALE * get_scene_scale(get_window_center());
    }
    sdl_draw_polygon(shape, body_get_color(body));
    list_free(shape);
  }

  // BACKGROUND IMAGE
  SDL_RenderCopy(renderer, BG_TEXTURE, NULL, NULL); 

  // SPRITES
  SDL_RenderCopy(renderer, PLANT_BOY_TEXTURE, NULL, &plant_boy_rect);
  SDL_RenderCopy(renderer, DIRT_GIRL_TEXTURE, NULL, &dirt_girl_rect);

  // OBJECTS
  SDL_RenderCopy(renderer, DIRT_GIRL_FERTILIZER_TEXTURE, NULL, &dirt_girl_fertilizer_rect);
  SDL_RenderCopy(renderer, PLANT_BOY_FERTILIZER_TEXTURE, NULL, &plant_boy_fertilizer_rect);
  if (star_in_scene(scene)) {
    SDL_RenderCopy(renderer, STAR_OF_MASTERY_TEXTURE, NULL, &star_of_mastery_rect);
  }
  SDL_RenderCopy(renderer, DEATH_TREE_TEXTURE, NULL, &tree_rect);

  sdl_show();

  // destroy every texture created
  SDL_DestroyTexture(BG_TEXTURE);
  SDL_DestroyTexture(PLANT_BOY_TEXTURE);
  SDL_DestroyTexture(DIRT_GIRL_TEXTURE);
  SDL_DestroyTexture(DEATH_TREE_TEXTURE);
  SDL_DestroyTexture(DIRT_GIRL_FERTILIZER_TEXTURE);
  SDL_DestroyTexture(PLANT_BOY_FERTILIZER_TEXTURE);
  if (star_in_scene(scene)) {
    SDL_DestroyTexture(STAR_OF_MASTERY_TEXTURE);
  }
}

//SOUND CODE:

int initialize_sound() {
  // Initialize SDL.
	if (SDL_Init(SDL_INIT_AUDIO) < 0) {
		return -1;
  }

  //Initialize SDL_mixer 
	if (Mix_OpenAudio(22050, MIX_DEFAULT_FORMAT, 2, 4096) == -1) {
		return -1; 
  }
  return 1;
}

int load_sound_effect(char *filename) {
  wave = Mix_LoadWAV(filename);
	if (wave == NULL) {
    return -1;
  }
  if (Mix_PlayChannel(-1, wave, 0) == -1 ) {
    return -1;  
  }
  return 1;
}

char *get_sound_effect(void *sound) {
  int sound_enum = (int)sound;
  if (sound_enum == PORTAL)
    return PORTAL_SOUND;
  if (sound_enum == WIN)
    return WIN_SOUND;
  if (sound_enum == DIED)
    return DIED_SOUND;
  if (sound_enum == STAR_OFM)
    return STAR_OFM_SOUND;
  if (sound_enum == FERTILIZER)
    return FERTILISER_SOUND;
  if (sound_enum == TRAMPOLINE)
    return TRAMPLOINE_SOUND;
  if (sound_enum == ICE)
    return ICE_SOUND;
  return 0;
}

void sdl_on_key(key_handler_t handler) { key_handler = handler; }

double time_since_last_tick(void) {
  clock_t now = clock();
  double difference = last_clock
                          ? (double)(now - last_clock) / CLOCKS_PER_SEC
                          : 0.0; // return 0 the first time this is called
  last_clock = now;
  return difference;
}

int free_audio() {
	Mix_FreeChunk(wave);
	Mix_FreeMusic(music);
	Mix_CloseAudio();
  return 1;
}