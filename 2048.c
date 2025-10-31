
#include <furi.h>
#include <furi_hal.h>
#include <gui/gui.h>
#include <stdlib.h>
#include <storage/storage.h>
#include <time.h>

typedef struct {
  unsigned int highscore;
  unsigned int highblock;
} Records;

typedef struct {
  int grid[4][4];
  unsigned int score;
  int running;
  Records records;
} GameState;

int check_gameover(GameState *);

unsigned int get_highest(GameState *gs) {
  unsigned int highest = 0;
  for (int i = 0; i < 16; i++)
    if (gs->grid[i / 4][i % 4] >= (int)highest) {
      highest = gs->grid[i / 4][i % 4];
    }
  return highest;
}

void update_gamestate(GameState *state) {
  if (!state)
    return;

  int full = 1;
  for (int i = 0; i < 16; i++) {
    if (state->grid[i / 4][i % 4] == -1) {
      full = 0;
      break;
    }
  }
  if (full) {
    state->running = 0;
    return;
  }

  int new;
  do {
    new = rand() % 16;
  } while (state->grid[new / 4][new % 4] != -1);
  state->grid[new / 4][new % 4] = rand() % 2;
  if (state->score > state->records.highscore)
    state->records.highscore = state->score;
  if (get_highest(state) > state->records.highblock)
    state->records.highblock = get_highest(state);
}

static int handle_left_shift(GameState *gs) {
  int res = 0;
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      /* Handle non-empty cells */
      if (gs->grid[i][j] >= 0) {
        for (int k = i; k > 0; k--) {
          /* If it's empty, just shift it over */
          if (gs->grid[k - 1][j] == -1) {
            gs->grid[k - 1][j] = gs->grid[k][j];
            gs->grid[k][j] = -1;
            res = 1;
          }
          /* Collision cases */
          else {
            /* Combine blocks if they're the same value */
            if (gs->grid[k - 1][j] == gs->grid[k][j]) {
              gs->grid[k - 1][j]++;
              gs->grid[k][j] = -1;
              /* Preserves 2048-like scoring even with the display differences
               */
              gs->score += (1 << gs->grid[k - 1][j]);
              res = 1;
            }
            break;
          }
        }
      }
    }
  }
  return res;
}

static int handle_right_shift(GameState *gs) {
  int res = 0;
  for (int i = 3; i >= 0; i--) {
    for (int j = 0; j < 4; j++) {
      /* Handle non-empty cells */
      if (gs->grid[i][j] >= 0) {
        for (int k = i; k < 3; k++) {
          /* If it's empty, just shift it over */
          if (gs->grid[k + 1][j] == -1) {
            gs->grid[k + 1][j] = gs->grid[k][j];
            gs->grid[k][j] = -1;
            res = 1;
          }
          /* Collision cases */
          else {
            /* Combine blocks if they're the same value */
            if (gs->grid[k + 1][j] == gs->grid[k][j]) {
              gs->grid[k + 1][j]++;
              gs->grid[k][j] = -1;
              /* Preserves 2048-like scoring even with the display differences
               */
              gs->score += (1 << gs->grid[k + 1][j]);
              res = 1;
            }
            break;
          }
        }
      }
    }
  }
  return res;
}

static int handle_up_shift(GameState *gs) {
  int res = 0;
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      /* Handle non-empty cells */
      if (gs->grid[j][i] >= 0) {
        for (int k = i; k > 0; k--) {
          /* If it's empty, just shift it over */
          if (gs->grid[j][k - 1] == -1) {
            gs->grid[j][k - 1] = gs->grid[j][k];
            gs->grid[j][k] = -1;
            res = 1;
          }
          /* Collision cases */
          else {
            /* Combine blocks if they're the same value */
            if (gs->grid[j][k - 1] == gs->grid[j][k]) {
              gs->grid[j][k - 1]++;
              gs->grid[j][k] = -1;
              /* Preserves 2048-like scoring even with the display differences
               */
              gs->score += (1 << gs->grid[j][k - 1]);
              res = 1;
            }
            break;
          }
        }
      }
    }
  }
  return res;
}

static int handle_down_shift(GameState *gs) {
  int res = 0;
  for (int i = 3; i >= 0; i--) {
    for (int j = 0; j < 4; j++) {
      /* Handle non-empty cells */
      if (gs->grid[j][i] >= 0) {
        for (int k = i; k < 3; k++) {
          /* If it's empty, just shift it over */
          if (gs->grid[j][k + 1] == -1) {
            gs->grid[j][k + 1] = gs->grid[j][k];
            gs->grid[j][k] = -1;
            res = 1;
          }
          /* Collision cases */
          else {
            /* Combine blocks if they're the same value */
            if (gs->grid[j][k + 1] == gs->grid[j][k]) {
              gs->grid[j][k + 1]++;
              gs->grid[j][k] = -1;
              /* Preserves 2048-like scoring even with the display differences
               */
              gs->score += (1 << gs->grid[j][k + 1]);
              res = 1;
            }
            break;
          }
        }
      }
    }
  }
  return res;
}

static void input_callback(InputEvent *event, void *ctx) {
  UNUSED(ctx);
  GameState *gs = ctx;

  int update = 0;
  if (event->type == InputTypeShort) {
    switch (event->key) {
    case InputKeyUp:
      update = handle_up_shift(gs);
      break;
    case InputKeyDown:
      update = handle_down_shift(gs);
      break;
    case InputKeyLeft:
      update = handle_left_shift(gs);
      break;
    case InputKeyRight:
      update = handle_right_shift(gs);
      break;
    case InputKeyBack:
      gs->running = 0;
      break;
    default:
      break;
    }
  }
  if (update)
    update_gamestate(gs);
  if (check_gameover(gs))
    gs->running = 0;
}

static void draw_callback(Canvas *canvas, void *ctx) {
  UNUSED(ctx);
  canvas_clear(canvas);
  GameState *gs = ctx;

  /* Draw the grid */
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      canvas_draw_frame(canvas, i * 16, j * 16, 16, 16);
    }
  }

  /* Draw in the values */
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      if (gs->grid[i][j] >= 0) {
        char buff[16] = "";
        snprintf(buff, sizeof(buff), "%d", gs->grid[i][j] + 1);
        int x = i * 16 + 7;
        int y = j * 16 + 11;
        if (gs->grid[i][j] + 1 >= 10)
          x -= 3;
        canvas_draw_str(canvas, x, y, buff);
      }
    }
  }
  canvas_draw_frame(canvas, 63, 0, 65, 64);

  /* Draw current score */
  char buff[32] = "";
  snprintf(buff, sizeof(buff), "Score: %d", gs->score);
  canvas_draw_str(canvas, 67, 10, buff);
  memset(buff, 0, sizeof(buff));

  /* Draw highest block */
  unsigned int highest = get_highest(gs);
  snprintf(buff, sizeof(buff), "Highest: %d", 1 << highest);
  canvas_draw_str(canvas, 67, 20, buff);
  memset(buff, 0, sizeof(buff));

  /* Draw high score */
  snprintf(buff, sizeof(buff), "HScore: %d", gs->records.highscore);
  canvas_draw_str(canvas, 67, 30, buff);

  /* Draw high block */
  snprintf(buff, sizeof(buff), "HBlock: %d", 1 << gs->records.highblock);
  canvas_draw_str(canvas, 67, 40, buff);
}

void init_records(GameState *state) {
  UNUSED(state);
  Storage *storage = furi_record_open(RECORD_STORAGE);
  File *file = storage_file_alloc(storage);
  if (storage_file_open(file, "/ext/apps_data/2048/records.bin", FSAM_READ,
                        FSOM_OPEN_EXISTING)) {
    storage_file_read(file, &state->records, sizeof(Records));
    storage_file_close(file);
  }
  storage_file_free(file);
  furi_record_close(RECORD_STORAGE);
}

void save_records(GameState *state) {
  UNUSED(state);
  Storage *storage = furi_record_open(RECORD_STORAGE);
  File *file = storage_file_alloc(storage);
  if (storage_file_open(file, "/ext/apps_data/2048/records.bin", FSAM_WRITE,
                        FSOM_CREATE_ALWAYS)) {
    storage_file_write(file, &state->records, sizeof(Records));
    storage_file_close(file);
  }
  storage_file_free(file);
  furi_record_close(RECORD_STORAGE);
}

void init_gamestate(GameState *state) {
  UNUSED(state);
  uint32_t idxs[2];
  idxs[0] = rand() % 16;
  do {
    idxs[1] = rand() % 16;
  } while (idxs[1] == idxs[0]);

  for (uint32_t i = 0; i < 16; i++) {
    if (i == idxs[0] || i == idxs[1]) {
      int is_1 = rand() % 2;
      if (is_1) {
        state->grid[i / 4][i % 4] = 1;
      } else {
        state->grid[i / 4][i % 4] = 0;
      }
    } else {
      state->grid[i / 4][i % 4] = -1;
    }
  }
  state->running = 1;
  state->score = 0;
  state->records.highscore = 0;
  state->records.highblock = 1;
  init_records(state);
}

int can_move(GameState *gs, int x, int y) {
  UNUSED(gs);
  if (gs->grid[x][y] == -1)
    return 1;
  if (x > 0) {
    int val = gs->grid[x - 1][y];
    if (val == -1 || val == gs->grid[x][y])
      return 1;
  }
  if (x < 3) {
    int val = gs->grid[x + 1][y];
    if (val == -1 || val == gs->grid[x][y])
      return 1;
  }
  if (y > 0) {
    int val = gs->grid[x][y - 1];
    if (val == -1 || val == gs->grid[x][y])
      return 1;
  }
  if (y < 3) {
    int val = gs->grid[x][y + 1];
    if (val == -1 || val == gs->grid[x][y])
      return 1;
  }
  return 0;
}

int check_gameover(GameState *gs) {
  UNUSED(gs);

  for (int i = 0; i < 4; i++)
    for (int j = 0; j < 4; j++)
      if (can_move(gs, i, j))
        return 0;

  return 1;
}

int32_t app_2048(void *p) {
  UNUSED(p);

  /* Open GUI */
  Gui *gui = furi_record_open(RECORD_GUI);

  /* Create ViewPort */
  ViewPort *viewport = view_port_alloc();

  /* Initialize random initial game state */
  GameState init;
  init_gamestate(&init);

  /* Set callbacks */
  view_port_draw_callback_set(viewport, draw_callback, &init);
  view_port_input_callback_set(viewport, input_callback, &init);

  /* Add ViewPort to GUI */
  gui_add_view_port(gui, viewport, GuiLayerFullscreen);

  /* Keep running until user closes app */
  while (init.running) {
    view_port_update(viewport);
    furi_delay_ms(50);
  }

  save_records(&init);

  furi_delay_ms(2500);

  /* Clean up */
  gui_remove_view_port(gui, viewport);
  view_port_free(viewport);
  furi_record_close(RECORD_GUI);

  return 0;
}
