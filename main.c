#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

#ifdef _WIN32
#include <windows.h>
#endif

typedef struct {
    char **terrain;
    char **paint;
    int x, y;
} State;

// для очистки терминала
void clear_screen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear"); // clear в винде тоже работает
#endif
}

// для вывода поля
void print_field(FILE *out, char **terrain, char **paint, int y, int x, int height, int width, bool to_file) {
    /*

    */
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            char c;
            if (i == y && j == x) {
                c = '#';
            } else if (terrain[i][j] == '_' && paint[i][j] != '\0') {
                c = paint[i][j];
            } else {
                c = terrain[i][j];
            }
            if (to_file) {
                fprintf(out, "%c ", c);
            } else {
                printf("%c ", c);
            }
        }
        if (to_file) {
            fprintf(out, "\n");
        } else {
            printf("\n");
        }
    }
    if (to_file) {
        fprintf(out, "\n");
    } else {
        printf("\n");
    }
}

// сохраняем историю
void push_state(State **history, int *size, int *cap, char **terr, char **pai, int px, int py, int hh, int ww) {
    if (*size == *cap) {
        *cap = *cap ? *cap * 2 : 16;
        *history = realloc(*history, *cap * sizeof(State)); // увеличить размер массива
        if (!*history) return; // мягкий выход
    }
    State *ns = &(*history)[*size]; // 2Д массивы с препятствиями, отдельный с цветами, координаты текущей позиции
    ns->terrain = malloc(hh * sizeof(char *));
    if (!ns->terrain) return;
    for (int i = 0; i < hh; i++) {
        ns->terrain[i] = malloc(ww);
        if (!ns->terrain[i]) { return; }
        memcpy(ns->terrain[i], terr[i], ww);
    }
    ns->paint = malloc(hh * sizeof(char *));
    if (!ns->paint) return;
    for (int i = 0; i < hh; i++) {
        ns->paint[i] = malloc(ww);
        if (!ns->paint[i]) return;
        memcpy(ns->paint[i], pai[i], ww);
    }
    ns->x = px; ns->y = py;
    (*size)++;
}

// undo
void pop_state(State **history, int *size, char ***terr, char ***pai, int *px, int *py, int hh, int ww) {
    /*
    1 Если история пуста или одна запись — выход.
    2 Уменьшает размер истории.
    3 Освобождает текущие массивы terr и pai.
    4 Выделяет новые массивы для terr и pai, копирует данные из предыдущего состояния (prev).
    5 Обновляет координаты px и py из prev.
    6 Освобождает копии массивов в prev, обнуляет указатели.
    */

    if (*size <= 1) return;
    (*size)--;
    State *prev = &(*history)[*size];
    // освободили память для текущего
    for (int i = 0; i < hh; i++) {
        free((*terr)[i]);
        free((*pai)[i]);
    }
    free(*terr); free(*pai);

    
    // Восстановили предыдущий
    *terr = malloc(hh * sizeof(char *));
    if (!*terr) return;
    for (int i = 0; i < hh; i++) {
        (*terr)[i] = malloc(ww);
        if (!(*terr)[i]) { return; }
        memcpy((*terr)[i], prev->terrain[i], ww);
    }
    *pai = malloc(hh * sizeof(char *));
    if (!*pai) return;
    for (int i = 0; i < hh; i++) {
        (*pai)[i] = malloc(ww);
        if (!(*pai)[i]) return;
        memcpy((*pai)[i], prev->paint[i], ww);
    }
    *px = prev->x; *py = prev->y;
    // Free prev copy
    for (int i = 0; i < hh; i++) { free(prev->terrain[i]); free(prev->paint[i]); }
    free(prev->terrain); free(prev->paint);
    prev->terrain = NULL; prev->paint = NULL;
}

// очистка истории
void cleanup(State **history, int *hsize, char ***terrain, char ***paint, int height) {
    if (*terrain) {
        for (int i = 0; i < height; i++) { free((*terrain)[i]); free((*paint)[i]); }
        free(*terrain); free(*paint);
        *terrain = NULL; *paint = NULL;
    }
    if (*history) {
        for (int k = 0; k < *hsize; k++) {
            if ((*history)[k].terrain) {
                for (int i = 0; i < height; i++) { free((*history)[k].terrain[i]); free((*history)[k].paint[i]); }
                free((*history)[k].terrain); free((*history)[k].paint);
            }
        }
        free(*history); *history = NULL;
    }
    *hsize = 0;
}


bool execute_command(char *line, int lnum, char *ctx, char **terrain, char **paint, int *x, int *y, int h, int w, bool display, int interval, int depth, State **history, int *hsize, int *hcap) {
    size_t len = strlen(line);
    if (len >= 2 && line[0] == '/' && line[1] == '/') return true;
    if (len == 0) return true;
    if (line[0] == ' ') { printf("Error: leading spaces %s line %d\n", ctx, lnum); return false; }

    int dx = 0, dy = 0; char dir[10];
    bool is_undo = (strncmp(line, "UNDO", 4) == 0 && (len == 4 || (line[4] == ' ' && line[5] == '\0')));
    bool success = true;
    bool warning_issued = false;

    if (is_undo) {
        // Проверяем, что после UNDO нет лишних символов
        if (len > 4 && line[4] != ' ') {
            printf("Error: unknown command '%s' %s line %d\n", line, ctx, lnum);
            return false;
        }
        pop_state(history, hsize, &terrain, &paint, x, y, h, w);
    } else if (strncmp(line, "MOVE ", 5) == 0) {
        if (sscanf(line + 5, "%9s", dir) != 1) {
            printf("Error: invalid MOVE command syntax %s line %d\n", ctx, lnum);
            return false;
        }
        // Проверяем, что после направления нет лишних символов
        char extra[100];
        if (sscanf(line + 5 + strlen(dir), "%99s", extra) == 1) {
            printf("Error: extra characters after MOVE command %s line %d\n", ctx, lnum);
            return false;
        }
        if (strcmp(dir, "UP") == 0) dy = -1; 
        else if (strcmp(dir, "DOWN") == 0) dy = 1;
        else if (strcmp(dir, "LEFT") == 0) dx = -1; 
        else if (strcmp(dir, "RIGHT") == 0) dx = 1; 
        else {
            printf("Error: invalid direction '%s' in MOVE command %s line %d\n", dir, ctx, lnum);
            return false;
        }
        int nx = (*x + dx + w) % w; int ny = (*y + dy + h) % h;
        char target_t = terrain[ny][nx];
        if (target_t == '%') { printf("Error: stepped on pit\n"); return false; }
        else if (target_t == '^' || target_t == '&' || target_t == '@') { 
            printf("Warning: cannot step on obstacle\n"); 
            warning_issued = true; 
        }
        else { *x = nx; *y = ny; }
    } else if (strncmp(line, "PAINT ", 6) == 0) {
        char ch; 
        char extra[100];
        if (sscanf(line + 6, " %c%99s", &ch, extra) != 1) {
            printf("Error: invalid PAINT command syntax %s line %d\n", ctx, lnum);
            return false;
        }
        if (ch < 'a' || ch > 'z') {
            printf("Error: invalid paint character '%c' (must be a-z) %s line %d\n", ch, ctx, lnum);
            return false;
        }
        paint[*y][*x] = ch;
    } else if (strncmp(line, "DIG ", 4) == 0) {
        if (sscanf(line + 4, "%9s", dir) != 1) {
            printf("Error: invalid DIG command syntax %s line %d\n", ctx, lnum);
            return false;
        }
        char extra[100];
        if (sscanf(line + 4 + strlen(dir), "%99s", extra) == 1) {
            printf("Error: extra characters after DIG command %s line %d\n", ctx, lnum);
            return false;
        }
        if (strcmp(dir, "UP") == 0) dy = -1; 
        else if (strcmp(dir, "DOWN") == 0) dy = 1;
        else if (strcmp(dir, "LEFT") == 0) dx = -1; 
        else if (strcmp(dir, "RIGHT") == 0) dx = 1; 
        else {
            printf("Error: invalid direction '%s' in DIG command %s line %d\n", dir, ctx, lnum);
            return false;
        }
        int nx = (*x + dx + w) % w; int ny = (*y + dy + h) % h;
        if (terrain[ny][nx] == '^') {
            terrain[ny][nx] = '_';
        } else {
            terrain[ny][nx] = '%';
        }
    } else if (strncmp(line, "MOUND ", 6) == 0) {
        if (sscanf(line + 6, "%9s", dir) != 1) {
            printf("Error: invalid MOUND command syntax %s line %d\n", ctx, lnum);
            return false;
        }
        char extra[100];
        if (sscanf(line + 6 + strlen(dir), "%99s", extra) == 1) {
            printf("Error: extra characters after MOUND command %s line %d\n", ctx, lnum);
            return false;
        }
        if (strcmp(dir, "UP") == 0) dy = -1; 
        else if (strcmp(dir, "DOWN") == 0) dy = 1;
        else if (strcmp(dir, "LEFT") == 0) dx = -1; 
        else if (strcmp(dir, "RIGHT") == 0) dx = 1; 
        else {
            printf("Error: invalid direction '%s' in MOUND command %s line %d\n", dir, ctx, lnum);
            return false;
        }
        int nx = (*x + dx + w) % w; int ny = (*y + dy + h) % h;
        if (terrain[ny][nx] == '%') terrain[ny][nx] = '_'; 
        else terrain[ny][nx] = '^';
    } else if (strncmp(line, "JUMP ", 5) == 0) {
        int n; 
        char extra[100];
        if (sscanf(line + 5, "%9s %d%99s", dir, &n, extra) != 2) {
            printf("Error: invalid JUMP command syntax %s line %d\n", ctx, lnum);
            return false;
        }
        if (n < 0) {
            printf("Error: negative jump distance %s line %d\n", ctx, lnum);
            return false;
        }
        if (strcmp(dir, "UP") == 0) dy = -1; 
        else if (strcmp(dir, "DOWN") == 0) dy = 1;
        else if (strcmp(dir, "LEFT") == 0) dx = -1; 
        else if (strcmp(dir, "RIGHT") == 0) dx = 1; 
        else {
            printf("Error: invalid direction '%s' in JUMP command %s line %d\n", dir, ctx, lnum);
            return false;
        }
        if (n == 0) return true;
        int curr_x = *x, curr_y = *y;
        bool stop_before_mound = false;
        bool ignore_jump = false;
        for (int step = 1; step <= n; step++) {
            int nx = (curr_x + dx + w) % w; int ny = (curr_y + dy + h) % h;
            char next_t = terrain[ny][nx];
            if (next_t == '^') {
                printf("Warning: cannot jump over mound\n");
                stop_before_mound = true;
                warning_issued = true;
                break;
            } else if (next_t == '&' || next_t == '@') {
                printf("Warning: cannot jump over obstacle\n");
                ignore_jump = true;
                warning_issued = true;
                break;
            }
            curr_x = nx; curr_y = ny;
        }
        if (!ignore_jump) {
            char final_t = terrain[curr_y][curr_x];
            if (final_t == '%') { printf("Error: stepped on pit\n"); return false; }
            *x = curr_x; *y = curr_y;
        }
    } else if (strncmp(line, "GROW ", 5) == 0) {
        if (sscanf(line + 5, "%9s", dir) != 1) {
            printf("Error: invalid GROW command syntax %s line %d\n", ctx, lnum);
            return false;
        }
        char extra[100];
        if (sscanf(line + 5 + strlen(dir), "%99s", extra) == 1) {
            printf("Error: extra characters after GROW command %s line %d\n", ctx, lnum);
            return false;
        }
        if (strcmp(dir, "UP") == 0) dy = -1; 
        else if (strcmp(dir, "DOWN") == 0) dy = 1;
        else if (strcmp(dir, "LEFT") == 0) dx = -1; 
        else if (strcmp(dir, "RIGHT") == 0) dx = 1; 
        else {
            printf("Error: invalid direction '%s' in GROW command %s line %d\n", dir, ctx, lnum);
            return false;
        }
        int nx = (*x + dx + w) % w; int ny = (*y + dy + h) % h;
        if (terrain[ny][nx] != '_') { 
            printf("Error: cannot grow tree on non-empty cell %s line %d\n", ctx, lnum); 
            return false; 
        }
        terrain[ny][nx] = '&';
    } else if (strncmp(line, "CUT ", 4) == 0) {
        if (sscanf(line + 4, "%9s", dir) != 1) {
            printf("Error: invalid CUT command syntax %s line %d\n", ctx, lnum);
            return false;
        }
        char extra[100];
        if (sscanf(line + 4 + strlen(dir), "%99s", extra) == 1) {
            printf("Error: extra characters after CUT command %s line %d\n", ctx, lnum);
            return false;
        }
        if (strcmp(dir, "UP") == 0) dy = -1; 
        else if (strcmp(dir, "DOWN") == 0) dy = 1;
        else if (strcmp(dir, "LEFT") == 0) dx = -1; 
        else if (strcmp(dir, "RIGHT") == 0) dx = 1; 
        else {
            printf("Error: invalid direction '%s' in CUT command %s line %d\n", dir, ctx, lnum);
            return false;
        }
        int nx = (*x + dx + w) % w; int ny = (*y + dy + h) % h;
        if (terrain[ny][nx] != '&') { 
            printf("Error: no tree to cut %s line %d\n", ctx, lnum); 
            return false; 
        }
        terrain[ny][nx] = '_';
    } else if (strncmp(line, "MAKE ", 5) == 0) {
        if (sscanf(line + 5, "%9s", dir) != 1) {
            printf("Error: invalid MAKE command syntax %s line %d\n", ctx, lnum);
            return false;
        }
        char extra[100];
        if (sscanf(line + 5 + strlen(dir), "%99s", extra) == 1) {
            printf("Error: extra characters after MAKE command %s line %d\n", ctx, lnum);
            return false;
        }
        if (strcmp(dir, "UP") == 0) dy = -1; 
        else if (strcmp(dir, "DOWN") == 0) dy = 1;
        else if (strcmp(dir, "LEFT") == 0) dx = -1; 
        else if (strcmp(dir, "RIGHT") == 0) dx = 1; 
        else {
            printf("Error: invalid direction '%s' in MAKE command %s line %d\n", dir, ctx, lnum);
            return false;
        }
        int nx = (*x + dx + w) % w; int ny = (*y + dy + h) % h;
        if (terrain[ny][nx] != '_') { 
            printf("Error: cannot make stone on non-empty cell %s line %d\n", ctx, lnum); 
            return false; 
        }
        terrain[ny][nx] = '@';
    } else if (strncmp(line, "PUSH ", 5) == 0) {
        if (sscanf(line + 5, "%9s", dir) != 1) {
            printf("Error: invalid PUSH command syntax %s line %d\n", ctx, lnum);
            return false;
        }
        char extra[100];
        if (sscanf(line + 5 + strlen(dir), "%99s", extra) == 1) {
            printf("Error: extra characters after PUSH command %s line %d\n", ctx, lnum);
            return false;
        }
        if (strcmp(dir, "UP") == 0) dy = -1; 
        else if (strcmp(dir, "DOWN") == 0) dy = 1;
        else if (strcmp(dir, "LEFT") == 0) dx = -1; 
        else if (strcmp(dir, "RIGHT") == 0) dx = 1; 
        else {
            printf("Error: invalid direction '%s' in PUSH command %s line %d\n", dir, ctx, lnum);
            return false;
        }
        int nx = (*x + dx + w) % w; int ny = (*y + dy + h) % h;
        if (terrain[ny][nx] != '@') { 
            printf("Error: no stone to push %s line %d\n", ctx, lnum); 
            return false; 
        }
        int px = (nx + dx + w) % w; int py = (ny + dy + h) % h;
        char tt = terrain[py][px];
        if (tt == '^' || tt == '&' || tt == '@') { 
            printf("Error: cannot push stone into obstacle %s line %d\n", ctx, lnum); 
            return false; 
        }
        terrain[ny][nx] = '_';
        if (tt == '%') terrain[py][px] = '_'; 
        else terrain[py][px] = '@';
        *x = nx;
        *y = ny;
    } else if (strncmp(line, "EXEC ", 5) == 0) {
        char fname[50]; 
        char extra[100];
        if (sscanf(line + 5, "%49s%99s", fname, extra) != 1) {
            printf("Error: invalid EXEC command syntax %s line %d\n", ctx, lnum);
            return false;
        }
        if (depth >= 10) { 
            printf("Error: nesting too deep %s line %d\n", ctx, lnum); 
            return false; 
        }
        FILE *subfp = fopen(fname, "r"); 
        if (!subfp) { 
            printf("Error: cannot open exec file '%s' %s line %d\n", fname, ctx, lnum); 
            return false; 
        }
        char subline[100]; int subln = 0;
        while (fgets(subline, sizeof(subline), subfp)) {
            subln++; 
            size_t slen = strlen(subline); 
            if (slen > 0 && subline[slen - 1] == '\n') subline[slen - 1] = '\0';
            if (slen == 0 || (slen >= 2 && subline[0] == '/' && subline[1] == '/')) continue;
            if (subline[0] == ' ') { 
                printf("Error: leading spaces in %s line %d\n", fname, subln); 
                fclose(subfp); 
                return false; 
            }
            if (!execute_command(subline, subln, fname, terrain, paint, x, y, h, w, display, interval, depth + 1, history, hsize, hcap)) { 
                fclose(subfp); 
                return false; 
            }
        }
        fclose(subfp);
    } else if (strncmp(line, "IF CELL ", 8) == 0) {
        int cx, cy; char isym; char then_cmd[90];
        char extra[100];
        // Более строгая проверка синтаксиса IF CELL
        if (sscanf(line, "IF CELL %d %d IS %c THEN %89[^\n]%99s", &cx, &cy, &isym, then_cmd, extra) != 4) {
            printf("Error: invalid IF CELL command syntax %s line %d\n", ctx, lnum);
            return false;
        }
        cx = (cx % w + w) % w; cy = (cy % h + h) % h;
        char cell_sym = (cx == *x && cy == *y) ? '#' : (terrain[cy][cx] == '_' && paint[cy][cx] >= 'a' && paint[cy][cx] <= 'z' ? paint[cy][cx] : terrain[cy][cx]);
        if (cell_sym == isym) {
            return execute_command(then_cmd, lnum, ctx, terrain, paint, x, y, h, w, display, interval, depth + 1, history, hsize, hcap);
        }
    } else {
        // Любая другая строка, которая не соответствует ни одному известному формату команды
        printf("Error: unknown command '%s' %s line %d\n", line, ctx, lnum);
        return false;
    }

    if (success && !is_undo) push_state(history, hsize, hcap, terrain, paint, *x, *y, h, w);
    
    if (success && !warning_issued && display && interval > 0) { 
        clear_screen(); 
        print_field(stdout, terrain, paint, *y, *x, h, w, false); 
        sleep(interval); 
    }
    
    return true;
}
int main(int argc, char *argv[]) {
    if (argc < 3) { printf("Usage: %s <input.txt> <output.txt> [interval N] [no-display] [no-save]\n", argv[0]); return 1; }

    bool display = true, save = true; int interval = 1;
    for (int i = 3; i < argc; i++) {
        if (strcmp(argv[i], "no-display") == 0) display = false;
        else if (strcmp(argv[i], "no-save") == 0) save = false;
        else if (strcmp(argv[i], "interval") == 0) { i++; if (i >= argc) { printf("Error: missing N for interval\n"); return 1; } interval = atoi(argv[i]); if (interval < 0) interval = 0; }
        else { printf("Error: unknown option '%s'\n", argv[i]); return 1; }
    }

    FILE *fp = fopen(argv[1], "r"); if (!fp) { printf("Error: cannot open '%s'\n", argv[1]); return 1; }

    char **terrain = NULL, **paint = NULL; int width = 0, height = 0, x = 0, y = 0;
    bool size_set = false, start_set = false; int line_num = 0; char line[100]; char ctx[100]; strcpy(ctx, argv[1]);
    State *history = NULL; int hsize = 0, hcap = 0;

    while (fgets(line, sizeof(line), fp)) {
        line_num++; size_t len = strlen(line); if (len > 0 && line[len - 1] == '\n') line[len - 1] = '\0';
        if (len == 0) continue; if (len >= 2 && line[0] == '/' && line[1] == '/') continue;
        if (line[0] == ' ') { printf("Error: leading spaces at line %d\n", line_num); cleanup(&history, &hsize, &terrain, &paint, height); fclose(fp); return 1; }

        if (!size_set) {
            if (strncmp(line, "SIZE ", 5) != 0) { printf("Error: first non-comment must be SIZE at line %d\n", line_num); fclose(fp); return 1; }
            if (sscanf(line + 5, "%d %d", &width, &height) != 2 || width < 10 || width > 100 || height < 10 || height > 100) {
                printf("Error: invalid SIZE (10-100) at line %d\n", line_num); fclose(fp); return 1;
            }
            size_set = true;
            terrain = malloc(height * sizeof(char *)); if (!terrain) { fclose(fp); return 1; }
            paint = malloc(height * sizeof(char *)); if (!paint) { free(terrain); fclose(fp); return 1; }
            for (int i = 0; i < height; i++) {
                terrain[i] = malloc(width); if (!terrain[i]) { /* cleanup */ fclose(fp); return 1; }
                memset(terrain[i], '_', width);
                paint[i] = malloc(width); if (!paint[i]) return 1;
                memset(paint[i], 0, width);
            }
            continue;
        }

        if (!start_set) {
            bool handled = false;
            if (strncmp(line, "LOAD ", 5) == 0) {
                char lfname[50]; if (sscanf(line + 5, "%s", lfname) != 1) { printf("Error: invalid LOAD at line %d\n", line_num); cleanup(&history, &hsize, &terrain, &paint, height); fclose(fp); return 1; }
                FILE *loadf = fopen(lfname, "r"); if (!loadf) { printf("Error: cannot open '%s' at line %d\n", lfname, line_num); cleanup(&history, &hsize, &terrain, &paint, height); fclose(fp); return 1; }
                char row[201]; bool found_dino = false;
                for (int i = 0; i < height; i++) {
                    if (!fgets(row, sizeof(row), loadf)) { printf("Error: incomplete LOAD at line %d\n", line_num); fclose(loadf); cleanup(&history, &hsize, &terrain, &paint, height); fclose(fp); return 1; }
                    char *p = row; for (int j = 0; j < width; j++) {
                        char c; if (sscanf(p, "%c", &c) != 1 || *(p + 1) != ' ') { printf("Error: invalid LOAD format at line %d\n", line_num); fclose(loadf); cleanup(&history, &hsize, &terrain, &paint, height); fclose(fp); return 1; }
                        p += 2; if (c == '#') { x = j; y = i; terrain[i][j] = '_'; paint[i][j] = '\0'; found_dino = true; }
                        else if (c >= 'a' && c <= 'z') { terrain[i][j] = '_'; paint[i][j] = c; } else { terrain[i][j] = c; paint[i][j] = '\0'; }
                    }
                }
                fclose(loadf); if (!found_dino) { printf("Error: no # in LOAD at line %d\n", line_num); cleanup(&history, &hsize, &terrain, &paint, height); fclose(fp); return 1; }
                start_set = true; handled = true; if (display) { clear_screen(); print_field(stdout, terrain, paint, y, x, height, width, false); }
            } else if (strncmp(line, "START ", 6) == 0) {
                if (sscanf(line + 6, "%d %d", &x, &y) != 2 || x < 0 || x >= width || y < 0 || y >= height) {
                    printf("Error: invalid START at line %d\n", line_num); cleanup(&history, &hsize, &terrain, &paint, height); fclose(fp); return 1;
                }
                start_set = true; handled = true; if (display) { clear_screen(); print_field(stdout, terrain, paint, y, x, height, width, false); }
            }
            if (handled) push_state(&history, &hsize, &hcap, terrain, paint, x, y, height, width);
            if (!handled) { printf("Error: LOAD/START expected at line %d\n", line_num); cleanup(&history, &hsize, &terrain, &paint, height); fclose(fp); return 1; }
            continue;
        }

        if (strncmp(line, "SIZE ", 5) == 0 || strncmp(line, "START ", 6) == 0) { printf("Error: repeated %s at line %d\n", strncmp(line, "SIZE ", 5) == 0 ? "SIZE" : "START", line_num); cleanup(&history, &hsize, &terrain, &paint, height); fclose(fp); return 1; }
        if (strncmp(line, "LOAD ", 5) == 0) { printf("Error: LOAD must be first at line %d\n", line_num); cleanup(&history, &hsize, &terrain, &paint, height); fclose(fp); return 1; }

        if (!execute_command(line, line_num, ctx, terrain, paint, &x, &y, height, width, display, interval, 0, &history, &hsize, &hcap)) {
            cleanup(&history, &hsize, &terrain, &paint, height); fclose(fp); return 1;
        }
    }

    if (!size_set) { printf("Error: no SIZE\n"); fclose(fp); return 1; }
    if (!start_set) { printf("Error: no START/LOAD\n"); cleanup(&history, &hsize, &terrain, &paint, height); fclose(fp); return 1; }

    if (save) {
        FILE *outf = fopen(argv[2], "w"); if (outf) { print_field(outf, terrain, paint, y, x, height, width, true); fclose(outf); } else printf("Warning: cannot open '%s'\n", argv[2]);
    }

    cleanup(&history, &hsize, &terrain, &paint, height); fclose(fp);
    return 0;
}