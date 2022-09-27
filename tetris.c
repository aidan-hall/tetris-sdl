#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_video.h>
#include <SDL2/SDL_pixels.h>
#include <SDL_surface.h>
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef struct SDL_Context {
  SDL_Window *window;
  SDL_Surface *window_surface;
  SDL_Renderer *renderer;
  TTF_Font *font;

} SDL_Context;

SDL_Context make_sdl_context() {
  // Load SDL
  if (SDL_Init(SDL_INIT_VIDEO) < 0)
    exit(1);

  // Load SDL_image
  const int MY_IMG_FLAGS = IMG_INIT_PNG | IMG_INIT_JPG;
  if ((IMG_Init(MY_IMG_FLAGS) & MY_IMG_FLAGS) == 0)
    exit(1);

  // Load SDL_mixer
  const int MY_MIXER_FLAGS = MIX_INIT_OGG;
  if (Mix_Init(MY_MIXER_FLAGS) < 0)
    exit(1);

  if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0)
    exit(1);

  // Create window
  SDL_Context context;
  context.window =
      SDL_CreateWindow("Tetris", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                       500, 500, SDL_WINDOW_SHOWN);
  if (context.window == NULL)
    exit(1);

  // Texture Filtering
  if (!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1")) {
    SDL_LogWarn(SDL_LOG_CATEGORY_RENDER, "Couldn't enable linear filtering.\n");
  }

  // Create renderer for window.
  context.renderer =
      SDL_CreateRenderer(context.window, -1, SDL_RENDERER_ACCELERATED);
  if (context.renderer == NULL)
    exit(1);

  context.window_surface = SDL_GetWindowSurface(context.window);
  if (context.window_surface == NULL)
    exit(1);


  // TTF
  if (TTF_Init() == -1)
    exit(1);

  context.font = TTF_OpenFont("art/GroovetasticRegular.ttf", 28);
  if (context.font == NULL)
    exit(1);

  return context;
}

void close_sdl_context(SDL_Context context) {
  SDL_DestroyWindow(context.window);
  SDL_DestroyRenderer(context.renderer);

  TTF_CloseFont(context.font);

  TTF_Quit();
  Mix_Quit();
  IMG_Quit();
  SDL_Quit();
}


SDL_Texture *load_texture(SDL_Context ctx, const char *path) {
  SDL_Surface *surface = IMG_Load(path);
  SDL_Surface *optimised_surface = SDL_ConvertSurface(surface, ctx.window_surface->format, 0);
  SDL_Texture *texture = SDL_CreateTextureFromSurface(ctx.renderer, optimised_surface);
  SDL_FreeSurface(surface);
  SDL_FreeSurface(optimised_surface);
  return texture;
}

int main() {
  SDL_Context ctx = make_sdl_context();

  SDL_Texture * tiles = load_texture(ctx, "art/tiles.png");

  bool quit = false;
  while (!quit) {
    SDL_Event event;
    while (SDL_PollEvent(&event) != 0) {
      switch ((SDL_EventType)event.type) {
      case SDL_QUIT:
        quit = true;
        break;
      default:
        break;
      }
    }
  }

  SDL_DestroyTexture(tiles);
  close_sdl_context(ctx);
}
