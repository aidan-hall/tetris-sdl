#include <assert.h>
#include <raylib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define LENGTH(X) ((sizeof(X)) / (sizeof(X[0])))
#define FPS (60)
#define FRAME_LENGTH (1000 / FPS)
#define DROP_FRAMES (30)
#define DROP_FRAMES_DECREMENT (3)
#define SOFT_DROP_FRAMES (5)

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
const Rectangle PLAY_SPACE_DIMENSIONS = {
    PLAY_SPACE_X - 1,
    PLAY_SPACE_Y - 1,
    PLAY_SPACE_WIDTH *TILE_SIZE + 1,
    PLAY_SPACE_HEIGHT *TILE_SIZE + 1,
};

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

const Rectangle piece_regions[TET_NUM] = {
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
enum Move {
  MOVE_NONE,
  MOVE_LEFT,
  MOVE_RIGHT,
  MOVE_CLOCKWISE,
  MOVE_ANTICLOCKWISE,
};

struct Piece translated(struct Piece piece, int translation) {
  struct Piece moved = piece;
  moved.x += translation;
  return moved;
}

void horizontal_kick(struct Piece *piece, struct Piece *moved,
                     PlaySpace play_space) {
  /* const int kick_dist = */
  /*    (piece->tetromino == TET_I && piece->rotation == 1 && move ==
   * MOVE_CLOCKWISE) ? 2 : 1; */
  const int kick_dist = (piece->tetromino != TET_I) ? 1 : 2;
  printf("kick dist: %d\n", kick_dist);
  struct Piece kicked;
  if (!collides(play_space, (kicked = translated(*moved, kick_dist)))) {
    *piece = kicked;
  } else if (!collides(play_space, (kicked = translated(*moved, -kick_dist)))) {
    *piece = kicked;
  }
}

void maybe_move(struct Piece *piece, enum Move move, PlaySpace play_space) {
  struct Piece moved;
  switch (move) {
  case MOVE_LEFT:
    moved = translated(*piece, -1);
    break;
  case MOVE_RIGHT:
    moved = translated(*piece, 1);
    break;
  case MOVE_CLOCKWISE:
    moved = rotated(piece, 1);
    printf("Piece Rotation: %d\n", moved.rotation);
    break;
  case MOVE_ANTICLOCKWISE:
    moved = rotated(piece, -1);
    printf("Piece Rotation: %d\n", moved.rotation);
    break;
  case MOVE_NONE:
    return;
  }

  if (!collides(play_space, moved)) {
    *piece = moved;
  } else if (move == MOVE_CLOCKWISE) {
    horizontal_kick(piece, &moved, play_space);
  } else if (move == MOVE_ANTICLOCKWISE) {
    horizontal_kick(piece, &moved, play_space);
  }
}

void draw_tilemap(void *tiles, int width, int height,
                  const enum Tetromino map[width * height], int x, int y) {
  Rectangle dest_rect;
  dest_rect.height = dest_rect.width = TILE_SIZE;

  for (int i = 0; i < height; ++i) {
    for (int j = 0; j < width; ++j) {
      {
        /* Don't need to draw blank tiles */
        const enum Tetromino tet = map[width * i + j];
        if (tet != TET_EMPTY) {
          dest_rect.x = x + j * TILE_SIZE;
          dest_rect.y = y + i * TILE_SIZE;
          DrawRectangleRec(dest_rect, RED);
        }
      }
    }
  }
}

void draw_piece(void *tiles, struct Piece piece) {

  draw_tilemap(tiles, 4, 4, (enum Tetromino *)piece_tiles(piece),
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

const int clear_scores[4] = {
    100,
    300,
    500,
    800,
};

int main() {
  srand(time(NULL));

  InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Tetris");
  SetTargetFPS(FPS);

  /* Mix_Chunk *sound_clear = Mix_LoadWAV("sound/clear.wav"); */
  /* if (sound_clear == NULL) { */
  /*   puts("Couldn't load clear sound."); */
  /*   exit(1); */
  /* } */

  /* SDL_Texture *tiles = load_texture(ctx, "art/small-tiles.png"); */

  bool restart = true;

  while (restart) {
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
    const uint8_t shift_speed = 8;
    uint8_t shift_cycle = 0;
    uint32_t drop_cycle_speed = DROP_FRAMES;
    int score = 0;
    int total_clears = 0;
    int level = 0;
    while (!quit) {
      if (WindowShouldClose()) {
        quit = true;
        restart = false;
        break;
      }
      if (total_clears >= 8) {
        level += 1;
        total_clears -= 8;
        printf("Level Up: %d\n", level);
      }

      {
        enum Move move = MOVE_NONE;
        if (IsKeyPressed(KEY_SPACE)) {
          spawn_next_piece(&piece, &next_piece);
        }
        if (IsKeyPressed(KEY_C)) {
          move = MOVE_CLOCKWISE;
        }
        if (IsKeyPressed(KEY_X)) {
          move = MOVE_CLOCKWISE;
        }
        if (IsKeyPressed(KEY_C)) {
          move = MOVE_CLOCKWISE;
        }
        if (IsKeyPressed(KEY_RIGHT)) {
          if (move == MOVE_NONE) {
            shift_cycle = 0;
            move = MOVE_RIGHT;
          }
        }
        if (IsKeyPressed(KEY_LEFT)) {
          if (move == MOVE_NONE) {
            shift_cycle = 0;
            move = MOVE_LEFT;
          }
        }
        if (IsKeyPressed(KEY_R)) {
          quit = true;
        }

        if (IsKeyDown(KEY_DOWN)) {
          drop_cycle_speed = SOFT_DROP_FRAMES;
        } else {
          drop_cycle_speed = DROP_FRAMES - level * DROP_FRAMES_DECREMENT;
        }

        if (shift_cycle >= shift_speed) {
          shift_cycle = 0;
          if (IsKeyDown(KEY_LEFT)) {
            move = MOVE_LEFT;
          }
          if (IsKeyDown(KEY_RIGHT)) {
            move = MOVE_RIGHT;
          }
        } else {
          shift_cycle += 1;
        }

        if (move != MOVE_NONE)
          maybe_move(&piece, move, play_space);
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

      /* Row Clearing */
      int clears[PLAY_SPACE_HEIGHT];
      int n_clears = 0;
      for (int i = 0; i < PLAY_SPACE_HEIGHT; ++i) {
        bool full = true;
        for (int j = 0; j < PLAY_SPACE_WIDTH; ++j) {
          if (play_space[i][j] == TET_EMPTY) {
            full = false;
            break;
          }
        }

        if (full) {
          clears[n_clears] = i;
          n_clears++;
        }
      }

      {
        /* Rendering */
        BeginDrawing();
        ClearBackground(BLACK);

        /* UI */
        DrawRectangleLinesEx(PLAY_SPACE_DIMENSIONS, 1.0f, BLUE);

        void *tiles = NULL;
        /* Game Elements */
        draw_tilemap(tiles, PLAY_SPACE_WIDTH, PLAY_SPACE_HEIGHT,
                     (enum Tetromino *)&play_space, PLAY_SPACE_X, PLAY_SPACE_Y);

        /* Current piece */
        draw_piece(tiles, piece);

        /* Piece Preview */
        draw_piece(tiles, (struct Piece){0, next_piece, -5, 3});

        /* Highlight CLEARED rows, and delete. */
        if (n_clears > 0) {
          assert(n_clears <= 4);

          /* Start at level 0 so multiply by level + 1. */
          score += clear_scores[n_clears - 1] * (level + 1);
          total_clears += n_clears;
          printf("Score: %d\n", score);

          /* Mix_PlayChannel(-1, sound_clear, 0); */

          Rectangle row_rect;
          row_rect.width = PLAY_SPACE_PIXEL_WIDTH;
          row_rect.height = TILE_SIZE;
          row_rect.x = PLAY_SPACE_X;
          for (int i = 0; i < n_clears; ++i) {
            row_rect.y = PLAY_SPACE_Y + TILE_SIZE * clears[i];
            DrawRectangleRec(row_rect, GREEN);

            /* Move all the rows down 1, and clear out the top row. */
            memmove(&play_space[1], &play_space[0],
                    clears[i] * PLAY_SPACE_WIDTH * sizeof(enum Tetromino));
            for (int j = 0; j < PLAY_SPACE_WIDTH; ++j) {
              play_space[0][j] = TET_EMPTY;
            }
          }
        }
        /* Pause to emphasise cleared rows after drawing them. */
        EndDrawing();
        if (n_clears > 0) {
          WaitTime(0.5f);
        }
      }

      /* Game Over Condition */
      if (collides(play_space, piece)) {
        quit = true;
      }
    }

    printf("Game Over!\n");
    WaitTime(0.5f);
  }

  /* Mix_FreeChunk(sound_clear); */
  /* SDL_DestroyTexture(tiles); */
  CloseWindow();
}
