#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_video.h>
#include <SDL_keycode.h>
#include <SDL_surface.h>
#include <SDL_timer.h>
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef struct SDL_Context {
  SDL_Window *window;
  SDL_Surface *window_surface;
  SDL_Renderer *renderer;
  TTF_Font *font;

} SDL_Context;

const int WINDOW_WIDTH = 500;
const int WINDOW_HEIGHT = 500;

const int TILE_SIZE = 24;

#define PLAY_SPACE_HEIGHT (20)
#define PLAY_SPACE_WIDTH (10)
#define PLAY_SPACE_PIXEL_HEIGHT (PLAY_SPACE_HEIGHT * TILE_SIZE)
#define PLAY_SPACE_PIXEL_WIDTH (PLAY_SPACE_WIDTH * TILE_SIZE)
#define PLAY_SPACE_X ((WINDOW_WIDTH - PLAY_SPACE_PIXEL_WIDTH) / 2)
#define PLAY_SPACE_Y (6)

/* Include (1 pixel) padding around play space. */
const SDL_Rect PLAY_SPACE_DIMENSIONS = {
    PLAY_SPACE_X - 1,
    PLAY_SPACE_Y - 1,
    PLAY_SPACE_WIDTH *TILE_SIZE + 1,
    PLAY_SPACE_HEIGHT *TILE_SIZE + 1,
};

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
                       WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
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
  SDL_Surface *optimised_surface =
      SDL_ConvertSurface(surface, ctx.window_surface->format, 0);
  SDL_Texture *texture =
      SDL_CreateTextureFromSurface(ctx.renderer, optimised_surface);
  SDL_FreeSurface(surface);
  SDL_FreeSurface(optimised_surface);
  return texture;
}

enum Tetromino {
  TET_I = 0,
  TET_O,
  TET_T,
  TET_J,
  TET_L,
  TET_S,
  TET_Z,
  TET_NUM,
};

enum Tetromino pick_piece() { return rand() % TET_NUM; }

struct Piece {
  /** [0-3] */
  uint8_t rotation : 2;
  enum Tetromino tetromino : 3;
  uint8_t x;
  uint8_t y;
};

typedef bool PieceTiles[4][4];
typedef bool PlaySpace[PLAY_SPACE_HEIGHT][PLAY_SPACE_WIDTH];

const PieceTiles orientations[TET_NUM][4] = {
    /* I */
    {
        {
            {0, 0, 0, 0},
            {0, 0, 0, 0},
            {1, 1, 1, 1},
            {0, 0, 0, 0},
        },
        {
            {0, 1, 0, 0},
            {0, 1, 0, 0},
            {0, 1, 0, 0},
            {0, 1, 0, 0},
        },
        {
            {0, 0, 0, 0},
            {1, 1, 1, 1},
            {0, 0, 0, 0},
            {0, 0, 0, 0},
        },
        {
            {0, 0, 1, 0},
            {0, 0, 1, 0},
            {0, 0, 1, 0},
            {0, 0, 1, 0},
        },
    },
    /* O */
    {
        {
            {0, 0, 0, 0},
            {0, 1, 1, 0},
            {0, 1, 1, 0},
            {0, 0, 0, 0},
        },
        {
            {0, 0, 0, 0},
            {0, 1, 1, 0},
            {0, 1, 1, 0},
            {0, 0, 0, 0},
        },
        {
            {0, 0, 0, 0},
            {0, 1, 1, 0},
            {0, 1, 1, 0},
            {0, 0, 0, 0},
        },
        {
            {0, 0, 0, 0},
            {0, 1, 1, 0},
            {0, 1, 1, 0},
            {0, 0, 0, 0},
        },
    },
    /* T */
    {
        {
            {0, 1, 0, 0},
            {1, 1, 1, 0},
            {0, 0, 0, 0},
            {0, 0, 0, 0},
        },
        {
            {0, 1, 0, 0},
            {0, 1, 1, 0},
            {0, 1, 0, 0},
            {0, 0, 0, 0},
        },
        {
            {0, 0, 0, 0},
            {1, 1, 1, 0},
            {0, 1, 0, 0},
            {0, 0, 0, 0},
        },
        {
            {0, 1, 0, 0},
            {1, 1, 0, 0},
            {0, 1, 0, 0},
            {0, 0, 0, 0},
        },
    },
    /* J */
    {
        {
            {1, 0, 0, 0},
            {1, 1, 1, 0},
            {0, 0, 0, 0},
            {0, 0, 0, 0},
        },
        {
            {0, 1, 1, 0},
            {0, 1, 0, 0},
            {0, 1, 0, 0},
            {0, 0, 0, 0},
        },
        {
            {0, 0, 0, 0},
            {1, 1, 1, 0},
            {0, 0, 1, 0},
            {0, 0, 0, 0},
        },
        {
            {0, 1, 0, 0},
            {0, 1, 0, 0},
            {1, 1, 0, 0},
            {0, 0, 0, 0},
        },
    },
    /* L */
    {
        {
            {0, 0, 1, 0},
            {1, 1, 1, 0},
            {0, 0, 0, 0},
            {0, 0, 0, 0},
        },
        {
            {0, 1, 0, 0},
            {0, 1, 0, 0},
            {0, 1, 1, 0},
            {0, 0, 0, 0},
        },
        {
            {0, 0, 0, 0},
            {1, 1, 1, 0},
            {1, 0, 0, 0},
            {0, 0, 0, 0},
        },
        {
            {1, 1, 0, 0},
            {0, 1, 0, 0},
            {0, 1, 0, 0},
            {0, 0, 0, 0},
        },
    },
    /* S */
    {
        {
            {0, 0, 0, 0},
            {0, 1, 1, 0},
            {1, 1, 0, 0},
            {0, 0, 0, 0},
        },
        {
            {1, 0, 0, 0},
            {1, 1, 0, 0},
            {0, 1, 0, 0},
            {0, 0, 0, 0},
        },
        {
            {0, 1, 1, 0},
            {1, 1, 0, 0},
            {0, 0, 0, 0},
            {0, 0, 0, 0},
        },
        {
            {0, 1, 0, 0},
            {0, 1, 1, 0},
            {0, 0, 1, 0},
            {0, 0, 0, 0},
        },
    },
    /* Z */
    {
        {
            {0, 0, 0, 0},
            {1, 1, 0, 0},
            {0, 1, 1, 0},
            {0, 0, 0, 0},
        },
        {
            {0, 1, 0, 0},
            {1, 1, 0, 0},
            {1, 0, 0, 0},
            {0, 0, 0, 0},
        },
        {
            {1, 1, 0, 0},
            {0, 1, 1, 0},
            {0, 0, 0, 0},
            {0, 0, 0, 0},
        },
        {
            {0, 0, 1, 0},
            {0, 1, 1, 0},
            {0, 1, 0, 0},
            {0, 0, 0, 0},
        },
    },
};

struct Piece spawn_piece() {
  struct Piece piece = {0, pick_piece(), 3, 0};
  return piece;
}

struct Piece rotated(struct Piece *piece, uint8_t steps) {
  struct Piece rotated_piece = *piece;
  rotated_piece.rotation = (piece->rotation + steps) % 4;
  return rotated_piece;
}

void draw_piece(SDL_Renderer *renderer, SDL_Texture *tiles,
                struct Piece piece) {
  const SDL_Rect texture_region = {piece.tetromino * TILE_SIZE, 0, TILE_SIZE,
                                   TILE_SIZE};

  PieceTiles piece_tiles;
  memcpy(&piece_tiles, orientations[piece.tetromino][piece.rotation],
         sizeof(PieceTiles));

  SDL_Rect dest_rect;
  dest_rect.w = dest_rect.h = TILE_SIZE;

  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 4; ++j) {
      {
        if (piece_tiles[i][j]) {
          dest_rect.x = PLAY_SPACE_X + (piece.x + j) * TILE_SIZE;
          dest_rect.y = PLAY_SPACE_Y + (piece.y + i) * TILE_SIZE;
          SDL_RenderCopy(renderer, tiles, &texture_region, &dest_rect);
        }
      }
    }
  }
}

int main() {
  srand(time(NULL));

  SDL_Context ctx = make_sdl_context();

  SDL_Texture *tiles = load_texture(ctx, "art/small-tiles.png");

  struct Piece piece = spawn_piece();

  PlaySpace play_space;
  memset(&play_space, 0, sizeof(PlaySpace));


  bool quit = false;
  while (!quit) {
    SDL_Event event;
    while (SDL_PollEvent(&event) != 0) {
      switch ((SDL_EventType)event.type) {
      case SDL_QUIT:
        quit = true;
        break;
      case SDL_KEYDOWN:
        switch ((SDL_KeyCode)event.key.keysym.sym) {
        case SDLK_SPACE:
          piece = spawn_piece();
          break;
        case SDLK_c:
          piece = rotated(&piece, 1);
          break;
        case SDLK_x:
          piece = rotated(&piece, -1);
          break;
        default:
          break;
        }
        break;
      default:
        break;
      }
    }

    {
      /* Rendering */
      SDL_Renderer *r = ctx.renderer;
      SDL_SetRenderDrawColor(r, 0, 0, 0, 0);
      SDL_RenderClear(r);

      /* UI */
      SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
      SDL_RenderDrawRect(r, &PLAY_SPACE_DIMENSIONS);

      /* Piece */
      draw_piece(r, tiles, piece);
      SDL_RenderPresent(r);
    }
  }

  SDL_DestroyTexture(tiles);
  close_sdl_context(ctx);
}
