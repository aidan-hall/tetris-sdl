#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_video.h>
#include <SDL_hints.h>
#include <SDL_keyboard.h>
#include <SDL_keycode.h>
#include <SDL_surface.h>
#include <SDL_timer.h>
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define LENGTH(X) ((sizeof(X)) / (sizeof(X[0])))
#define FPS (60)
#define FRAME_LENGTH (1000 / FPS)
#define DROP_FRAMES (30)
#define SOFT_DROP_FRAMES (10)

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
  context.renderer = SDL_CreateRenderer(
      context.window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
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
  TET_EMPTY = TET_NUM,
};

#define PIECE_REGION(I)                                                        \
  { I *TILE_SIZE, 0, TILE_SIZE, TILE_SIZE }
/* Can't be const due to SDL's API. */
SDL_Rect piece_regions[TET_NUM] = {
    PIECE_REGION(TET_I), PIECE_REGION(TET_O), PIECE_REGION(TET_T),
    PIECE_REGION(TET_J), PIECE_REGION(TET_L), PIECE_REGION(TET_S),
    PIECE_REGION(TET_Z),
};

enum Tetromino pick_piece() { return rand() % TET_NUM; }

struct Piece {
  /** [0-3] */
  uint8_t rotation : 2;
  enum Tetromino tetromino : 3;
  int8_t x;
  int8_t y;
};

typedef enum Tetromino PieceTiles[4][4];
typedef enum Tetromino PlaySpace[PLAY_SPACE_HEIGHT][PLAY_SPACE_WIDTH];

const PieceTiles orientations[TET_NUM][4] = {
    /* I */
    {
        {
            {7, 7, 7, 7},
            {7, 7, 7, 7},
            {0, 0, 0, 0},
            {7, 7, 7, 7},
        },
        {
            {7, 0, 7, 7},
            {7, 0, 7, 7},
            {7, 0, 7, 7},
            {7, 0, 7, 7},
        },
        {
            {7, 7, 7, 7},
            {0, 0, 0, 0},
            {7, 7, 7, 7},
            {7, 7, 7, 7},
        },
        {
            {7, 7, 0, 7},
            {7, 7, 0, 7},
            {7, 7, 0, 7},
            {7, 7, 0, 7},
        },
    },
    /* O */
    {
        {
            {7, 7, 7, 7},
            {7, 1, 1, 7},
            {7, 1, 1, 7},
            {7, 7, 7, 7},
        },
        {
            {7, 7, 7, 7},
            {7, 1, 1, 7},
            {7, 1, 1, 7},
            {7, 7, 7, 7},
        },
        {
            {7, 7, 7, 7},
            {7, 1, 1, 7},
            {7, 1, 1, 7},
            {7, 7, 7, 7},
        },
        {
            {7, 7, 7, 7},
            {7, 1, 1, 7},
            {7, 1, 1, 7},
            {7, 7, 7, 7},
        },
    },
    /* T */
    {
        {
            {7, 2, 7, 7},
            {2, 2, 2, 7},
            {7, 7, 7, 7},
            {7, 7, 7, 7},
        },
        {
            {7, 2, 7, 7},
            {7, 2, 2, 7},
            {7, 2, 7, 7},
            {7, 7, 7, 7},
        },
        {
            {7, 7, 7, 7},
            {2, 2, 2, 7},
            {7, 2, 7, 7},
            {7, 7, 7, 7},
        },
        {
            {7, 2, 7, 7},
            {2, 2, 7, 7},
            {7, 2, 7, 7},
            {7, 7, 7, 7},
        },
    },
    /* J */
    {
        {
            {3, 7, 7, 7},
            {3, 3, 3, 7},
            {7, 7, 7, 7},
            {7, 7, 7, 7},
        },
        {
            {7, 3, 3, 7},
            {7, 3, 7, 7},
            {7, 3, 7, 7},
            {7, 7, 7, 7},
        },
        {
            {7, 7, 7, 7},
            {3, 3, 3, 7},
            {7, 7, 3, 7},
            {7, 7, 7, 7},
        },
        {
            {7, 3, 7, 7},
            {7, 3, 7, 7},
            {3, 3, 7, 7},
            {7, 7, 7, 7},
        },
    },
    /* L */
    {
        {
            {7, 7, 4, 7},
            {4, 4, 4, 7},
            {7, 7, 7, 7},
            {7, 7, 7, 7},
        },
        {
            {7, 4, 7, 7},
            {7, 4, 7, 7},
            {7, 4, 4, 7},
            {7, 7, 7, 7},
        },
        {
            {7, 7, 7, 7},
            {4, 4, 4, 7},
            {4, 7, 7, 7},
            {7, 7, 7, 7},
        },
        {
            {4, 4, 7, 7},
            {7, 4, 7, 7},
            {7, 4, 7, 7},
            {7, 7, 7, 7},
        },
    },
    /* S */
    {
        {
            {7, 7, 7, 7},
            {7, 5, 5, 7},
            {5, 5, 7, 7},
            {7, 7, 7, 7},
        },
        {
            {5, 7, 7, 7},
            {5, 5, 7, 7},
            {7, 5, 7, 7},
            {7, 7, 7, 7},
        },
        {
            {7, 5, 5, 7},
            {5, 5, 7, 7},
            {7, 7, 7, 7},
            {7, 7, 7, 7},
        },
        {
            {7, 5, 7, 7},
            {7, 5, 5, 7},
            {7, 7, 5, 7},
            {7, 7, 7, 7},
        },
    },
    /* Z */
    {
        {
            {7, 7, 7, 7},
            {6, 6, 7, 7},
            {7, 6, 6, 7},
            {7, 7, 7, 7},
        },
        {
            {7, 6, 7, 7},
            {6, 6, 7, 7},
            {6, 7, 7, 7},
            {7, 7, 7, 7},
        },
        {
            {6, 6, 7, 7},
            {7, 6, 6, 7},
            {7, 7, 7, 7},
            {7, 7, 7, 7},
        },
        {
            {7, 7, 6, 7},
            {7, 6, 6, 7},
            {7, 6, 7, 7},
            {7, 7, 7, 7},
        },
    },
};

const PieceTiles *piece_tiles(struct Piece piece) {
  return &orientations[piece.tetromino][piece.rotation];
}

bool collides(PlaySpace play_space, struct Piece piece) {
  const PieceTiles *tiles = piece_tiles(piece);

  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 4; ++j) {
      if ((*tiles)[i][j] != TET_EMPTY) {
        int tile_x = j + piece.x;
        int tile_y = i + piece.y;
        /* Ensures that bounds checking is performed before indexing into
         * play_space memory. */
        if (tile_x < 0 || tile_x >= PLAY_SPACE_WIDTH || tile_y < 0 ||
            tile_y >= PLAY_SPACE_HEIGHT) {
          return true;
        }
        if (play_space[tile_y][tile_x] != TET_EMPTY) {
          return true;
        }
      }
    }
  }

  return false;
}

struct Piece spawn_piece(enum Tetromino tetromino) {
  struct Piece piece = {0, tetromino, 3, 0};
  return piece;
}

struct Piece rotated(struct Piece *piece, uint8_t steps) {
  struct Piece rotated_piece = *piece;
  rotated_piece.rotation = (piece->rotation + steps) % 4;
  return rotated_piece;
}

void maybe_move(struct Piece *piece, struct Piece result,
                PlaySpace play_space) {
  if (!collides(play_space, result)) {
    *piece = result;
  }
}

void draw_tilemap(SDL_Renderer *renderer, SDL_Texture *tiles, int width,
                  int height, const enum Tetromino map[width * height], int x,
                  int y) {
  SDL_Rect dest_rect;
  dest_rect.w = dest_rect.h = TILE_SIZE;

  for (int i = 0; i < height; ++i) {
    for (int j = 0; j < width; ++j) {
      {
        /* Don't need to draw blank tiles */
        const enum Tetromino tet = map[width * i + j];
        if (tet != TET_EMPTY) {
          dest_rect.x = x + j * TILE_SIZE;
          dest_rect.y = y + i * TILE_SIZE;
          SDL_RenderCopy(renderer, tiles, &piece_regions[tet], &dest_rect);
        }
      }
    }
  }
}

void draw_piece(SDL_Renderer *renderer, SDL_Texture *tiles,
                struct Piece piece) {

  draw_tilemap(renderer, tiles, 4, 4, (enum Tetromino *)piece_tiles(piece),
               piece.x * TILE_SIZE + PLAY_SPACE_X,
               piece.y * TILE_SIZE + PLAY_SPACE_Y);
}

void spawn_next_piece(struct Piece *piece, enum Tetromino *next_piece) {
  *piece = spawn_piece(*next_piece);
  *next_piece = pick_piece();
}

void place_piece(PlaySpace play_space, struct Piece piece) {
  const PieceTiles *tiles = piece_tiles(piece);
  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 4; ++j) {
      const enum Tetromino tile = (*tiles)[i][j];
      if (tile != TET_EMPTY) {
        play_space[piece.y + i][piece.x + j] = tile;
      }
    }
  }
}

int main() {
  srand(time(NULL));

  SDL_Context ctx = make_sdl_context();

  SDL_Texture *tiles = load_texture(ctx, "art/small-tiles.png");

  enum Tetromino next_piece = pick_piece();
  struct Piece piece;
  spawn_next_piece(&piece, &next_piece);

  PlaySpace play_space;
  for (int i = 0; i < PLAY_SPACE_HEIGHT; ++i) {
    for (int j = 0; j < PLAY_SPACE_WIDTH; ++j) {
      play_space[i][j] = TET_EMPTY;
    }
  }

  bool quit = false;
  uint8_t drop_cycle = 0;
  uint32_t drop_cycle_speed = DROP_FRAMES;
  while (!quit) {
    uint64_t tick = SDL_GetTicks();
    SDL_Event event;
    {
      struct Piece translated = piece;
      bool translate = false;
      while (SDL_PollEvent(&event) != 0) {
        switch ((SDL_EventType)event.type) {
        case SDL_QUIT:
          quit = true;
          break;
        case SDL_KEYDOWN: {
          switch ((SDL_KeyCode)event.key.keysym.sym) {
          case SDLK_SPACE:
            spawn_next_piece(&piece, &next_piece);
            break;
          case SDLK_c:
            translated = rotated(&piece, 1);
            translate = true;
            break;
          case SDLK_x:
            translated = rotated(&piece, -1);
            translate = true;
            break;
          case SDLK_LEFT:
            translated.x -= 1;
            translate = true;
            break;
          case SDLK_RIGHT:
            translated.x += 1;
            translate = true;
            break;
          default:
            break;
          }
          break;
        default:
          break;
        }
        }
      }

      int32_t n_keys;
      const uint8_t *keys = SDL_GetKeyboardState(&n_keys);
      if (keys[SDL_SCANCODE_DOWN]) {
        drop_cycle_speed = SOFT_DROP_FRAMES;
      } else {
        drop_cycle_speed = DROP_FRAMES;
      }

      if (translate)
        maybe_move(&piece, translated, play_space);
    }

    /* Dropping & Collision */
    if (drop_cycle >= drop_cycle_speed) {
      piece.y += 1;
      drop_cycle = 0;
    } else {
      drop_cycle++;
    }

    /* Piece landing */
    if (collides(play_space, piece)) {
      piece.y -= 1;
      place_piece(play_space, piece);
      spawn_next_piece(&piece, &next_piece);
    }

    if (collides(play_space, piece)) {
      quit = true;
    }

    {
      /* Rendering */
      SDL_Renderer *r = ctx.renderer;
      SDL_SetRenderDrawColor(r, 0, 0, 0, 0);
      SDL_RenderClear(r);

      /* UI */
      SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
      SDL_RenderDrawRect(r, &PLAY_SPACE_DIMENSIONS);

      /* Game Elements */
      draw_tilemap(r, tiles, PLAY_SPACE_WIDTH, PLAY_SPACE_HEIGHT,
                   (enum Tetromino *)&play_space, PLAY_SPACE_X, PLAY_SPACE_Y);
      /* Current piece */
      draw_piece(r, tiles, piece);

      draw_piece(r, tiles, (struct Piece){0, next_piece, -5, 3});
      SDL_RenderPresent(r);
    }

    uint64_t ending_tick = SDL_GetTicks64();
    SDL_Delay(FRAME_LENGTH - (ending_tick - tick));
  }


  printf("Game Over!\n");
  SDL_Delay(500);

  SDL_DestroyTexture(tiles);
  close_sdl_context(ctx);
}
