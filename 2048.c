
#include <furi.h>
#include <furi_hal.h>
#include <gui/gui.h>
#include <stdlib.h>
#include <storage/storage.h>
#include <time.h>

#define SCREEN_HEIGHT 64
#define SCREEN_WIDTH 128
#define TEXT_HEIGHT 7

typedef enum { STOPPED = 0, RUNNING = 1, GAMEOVER = 2 } State;

typedef struct {
  unsigned int highscore;
  unsigned int highblock;
} Records;

typedef struct {
  int grid[4][4];
  unsigned int score;
  State state;
  Records records;
} GameState;

int check_gameover(GameState *);

void save_records(GameState *);

ViewPort *viewport;

unsigned int get_highest(GameState *gs) {
  unsigned int highest = 0;
  for (int i = 0; i < 16; i++)
    if (gs->grid[i / 4][i % 4] >= (int)highest) {
      highest = gs->grid[i / 4][i % 4];
    }
  return highest;
}

void update_gamestate(GameState *gs) {
  if (!gs)
    return;

  if (check_gameover(gs)) {
    gs->state = GAMEOVER;
    return;
  }

  int new;
  do {
    new = rand() % 16;
  } while (gs->grid[new / 4][new % 4] != -1);
  gs->grid[new / 4][new % 4] = rand() % 2;
  int pb = 0;
  if (gs->score > gs->records.highscore) {
    gs->records.highscore = gs->score;
    pb = 1;
  }
  if (get_highest(gs) > gs->records.highblock) {
    gs->records.highblock = get_highest(gs);
    pb = 1;
  }
  if (pb)
    save_records(gs);
}

static int combine(int *arr, int *score) {
  *score = 0;
  int res = 0;
  for (int i = 0; i < 3; i++) {
    if (arr[i] == -1)
      continue;
    for (int j = i + 1; j < 4; j++) {
      if (arr[j] == -1)
        continue;
      if (arr[i] == arr[j]) {
        arr[i]++;
        arr[j] = -1;
        *score += 1 << arr[i];
        res = 1;
      }
      break;
    }
  }
  int move = 0;
  while (arr[move] != -1)
    move++;
  for (int i = move + 1; i < 4; i++) {
    if (arr[i] != -1) {
      arr[move] = arr[i];
      arr[i] = -1;
      res = 1;
      move++;
    }
  }
  return res;
}

static int handle_left_shift(GameState *gs) {
  int res = 0;
  for (int i = 0; i < 4; i++) {
    int temp[4] = {0};
    int score = 0;
    for (int j = 0; j < 4; j++)
      temp[j] = gs->grid[j][i];

    res |= combine(temp, &score);
    gs->score += score;
    for (int j = 0; j < 4; j++) {
      gs->grid[j][i] = temp[j];
    }
  }
  return res;
}

static int handle_right_shift(GameState *gs) {
  int res = 0;
  for (int i = 0; i < 4; i++) {
    int temp[4] = {0};
    int score = 0;
    for (int j = 3; j >= 0; j--)
      temp[3 - j] = gs->grid[j][i];

    res |= combine(temp, &score);
    gs->score += score;
    for (int j = 3; j >= 0; j--) {
      gs->grid[j][i] = temp[3 - j];
    }
  }
  return res;
}

static int handle_up_shift(GameState *gs) {
  int res = 0;
  for (int i = 0; i < 4; i++) {
    int temp[4] = {0};
    int score = 0;
    for (int j = 0; j < 4; j++)
      temp[j] = gs->grid[i][j];

    res |= combine(temp, &score);
    gs->score += score;
    for (int j = 0; j < 4; j++) {
      gs->grid[i][j] = temp[j];
    }
  }
  return res;
}

static int handle_down_shift(GameState *gs) {
  int res = 0;
  for (int i = 0; i < 4; i++) {
    int temp[4] = {0};
    int score = 0;
    for (int j = 0; j < 4; j++)
      temp[j] = gs->grid[i][3 - j];

    res |= combine(temp, &score);
    gs->score += score;
    for (int j = 0; j < 4; j++) {
      gs->grid[i][3 - j] = temp[j];
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
      gs->state = GAMEOVER;
      view_port_update(viewport);
      return;
    default:
      break;
    }
  }
  if (update)
    update_gamestate(gs);
  if (check_gameover(gs))
    gs->state = GAMEOVER;
}

#define GAMEOVER_TEXT "Game Over."
#define GAMEOVER_X                                                             \
  (SCREEN_WIDTH / 2 - ((sizeof(GAMEOVER_TEXT) - 1) * 5 - 1) / 2)
#define GAMEOVER_Y (SCREEN_HEIGHT / 2 - TEXT_HEIGHT + 2)

void draw_gameover(Canvas *canvas, GameState *state) {
  UNUSED(state);
  canvas_clear(canvas);
  canvas_draw_str(canvas, GAMEOVER_X, GAMEOVER_Y, GAMEOVER_TEXT);

  char buff[32] = "";
  snprintf(buff, sizeof(buff), "Score: %d", state->score);
  int32_t score_x = 0, score_y = 42;
  score_x = (SCREEN_WIDTH / 2) - (strlen(buff) * 5 - 1) / 2;
  score_y = (SCREEN_HEIGHT / 2) + TEXT_HEIGHT;
  canvas_draw_str(canvas, score_x, score_y, buff);
}

static void draw_callback(Canvas *canvas, void *ctx) {
  UNUSED(ctx);
  canvas_clear(canvas);
  GameState *gs = ctx;
  if (gs->state == GAMEOVER) {
    draw_gameover(canvas, gs);
    return;
  }

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
  snprintf(buff, sizeof(buff), "PB: %d", gs->records.highscore);
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
  /* Fills two random cells with a 1 or 2 */
  for (int i = 0; i < 16; i++) {
    state->grid[i / 4][i % 4] = -1;
  }
  /* Each call adds a single new random cell */
  update_gamestate(state);
  update_gamestate(state);
  state->state = RUNNING;
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
  viewport = view_port_alloc();

  /* Initialize random initial game state */
  GameState init;
  init_gamestate(&init);

  /* Set callbacks */
  view_port_draw_callback_set(viewport, draw_callback, &init);
  view_port_input_callback_set(viewport, input_callback, &init);

  /* Add ViewPort to GUI */
  gui_add_view_port(gui, viewport, GuiLayerFullscreen);

  /* Keep running until user closes app */
  while (init.state != STOPPED) {
    view_port_update(viewport);
    furi_delay_ms(50);
    if (init.state == GAMEOVER) {
      furi_delay_ms(1950);
      init.state = STOPPED;
    }
  }

  save_records(&init);

  /* Clean up */
  gui_remove_view_port(gui, viewport);
  view_port_free(viewport);
  furi_record_close(RECORD_GUI);

  return 0;
}
