#include "tui.h"

#include <ncurses.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int linhas;
    int colunas;
    int reps;
    float densidade;
    MatrizTriplas mt;
    CSR *csr;
    float *densa;
} AppState;

typedef struct {
    WINDOW *header;
    WINDOW *menu;
    WINDOW *content;
    WINDOW *status;
    int width;
    int height;
} Ui;

static void liberar_estado(AppState *st) {
    if (!st) return;
    liberar_triplas(&st->mt);
    liberar_csr(st->csr);
    free(st->densa);
    st->csr = NULL;
    st->densa = NULL;
    st->mt.dados = NULL;
    st->mt.total = 0;
}

static void construir_estado(AppState *st, unsigned int seed) {
    if (!st) return;
    liberar_estado(st);
    st->mt = gerar_triplas_aleatorias(st->linhas, st->colunas, st->densidade, seed);
    st->csr = construir_csr(&st->mt);
    st->densa = triplas_para_densa(&st->mt);
}

static void ui_println(WINDOW *win, int *y, int max_y, const char *fmt, ...) {
    if (*y >= max_y) return;
    va_list args;
    va_start(args, fmt);
    wmove(win, *y, 1);
    wclrtoeol(win);
    vw_printw(win, fmt, args);
    va_end(args);
    (*y)++;
}

static void ui_title(WINDOW *win, const char *title) {
    int w = getmaxx(win);
    wattron(win, A_BOLD);
    mvwprintw(win, 0, 2, " %s ", title);
    mvwhline(win, 1, 1, ACS_HLINE, w - 2);
    wattroff(win, A_BOLD);
}

static void ui_status(WINDOW *win, const char *msg) {
    werase(win);
    wbkgd(win, COLOR_PAIR(6));
    mvwprintw(win, 0, 2, "%s", msg);
    wrefresh(win);
}

static void mostrar_csr_tui(const AppState *st, WINDOW *content) {
    int h = getmaxy(content);
    int y = 2;
    werase(content);
    wbkgd(content, COLOR_PAIR(4));
    box(content, 0, 0);
    ui_title(content, "Vetores CSR");
    ui_println(content, &y, h - 2, "valores:");
    int x = 1;
    wmove(content, y, x);
    for (int k = 0; k < st->csr->total_nao_nulos; k++) {
        if (x + 7 >= getmaxx(content)) {
            y++;
            if (y >= h - 2) break;
            x = 1;
            wmove(content, y, x);
        }
        wprintw(content, "%.2f ", st->csr->valores[k]);
        x += 7;
    }
    y++;
    ui_println(content, &y, h - 2, "col_indices:");
    x = 1;
    wmove(content, y, x);
    for (int k = 0; k < st->csr->total_nao_nulos; k++) {
        if (x + 6 >= getmaxx(content)) {
            y++;
            if (y >= h - 2) break;
            x = 1;
            wmove(content, y, x);
        }
        wprintw(content, "%d ", st->csr->col_indices[k]);
        x += 6;
    }
    y++;
    ui_println(content, &y, h - 2, "row_ptr:");
    x = 1;
    wmove(content, y, x);
    for (int i = 0; i < st->csr->linhas + 1; i++) {
        if (x + 6 >= getmaxx(content)) {
            y++;
            if (y >= h - 2) break;
            x = 1;
            wmove(content, y, x);
        }
        wprintw(content, "%d ", st->csr->row_ptr[i]);
        x += 6;
    }
    wrefresh(content);
}

static void mostrar_densa_tui(const AppState *st, WINDOW *content) {
    int h = getmaxy(content);
    int w = getmaxx(content);
    int y = 2;
    werase(content);
    wbkgd(content, COLOR_PAIR(4));
    box(content, 0, 0);
    ui_title(content, "Matriz densa");
    ui_println(content, &y, h - 2, "Tamanho: %dx%d", st->linhas, st->colunas);
    for (int i = 0; i < st->linhas && y < h - 2; i++) {
        int x = 1;
        wmove(content, y, x);
        for (int j = 0; j < st->colunas; j++) {
            float v = st->densa[i * st->colunas + j];
            if (x + 7 >= w) break;
            wprintw(content, "%6.2f ", v);
            x += 7;
        }
        y++;
    }
    wrefresh(content);
}

static void desenhar_barra(WINDOW *win, int y, int x, int largura, double valor, double maxv, short pair) {
    int fill = 0;
    if (maxv > 0.0) {
        double ratio = valor / maxv;
        if (ratio > 1.0) ratio = 1.0;
        fill = (int)(ratio * largura);
    }
    for (int i = 0; i < largura; i++) {
        if (i < fill) {
            wattron(win, COLOR_PAIR(pair) | A_REVERSE);
            mvwaddch(win, y, x + i, ' ');
            wattroff(win, COLOR_PAIR(pair) | A_REVERSE);
        } else {
            wattron(win, COLOR_PAIR(4));
            mvwaddch(win, y, x + i, '.');
            wattroff(win, COLOR_PAIR(4));
        }
    }
}

static void mostrar_benchmark_tui(const AppState *st, WINDOW *content) {
    int h = getmaxy(content);
    int y = 2;
    double t_csr = medir_acesso_csr(st->csr, st->reps, st->linhas, st->colunas);
    double t_tri = medir_acesso_triplas(&st->mt, st->reps, st->linhas, st->colunas);
    double t_den = medir_acesso_densa(st->densa, st->reps, st->linhas, st->colunas);
    werase(content);
    wbkgd(content, COLOR_PAIR(4));
    box(content, 0, 0);
    ui_title(content, "Benchmark de acesso (i,j)");
    ui_println(content, &y, h - 2, "reps=%d, densidade=%.0f%%", st->reps, st->densidade * 100.0f);
    ui_println(content, &y, h - 2, "CSR    : %.6f s", t_csr);
    ui_println(content, &y, h - 2, "Triplas: %.6f s", t_tri);
    ui_println(content, &y, h - 2, "Densa  : %.6f s", t_den);

    double maxv = t_csr;
    if (t_tri > maxv) maxv = t_tri;
    if (t_den > maxv) maxv = t_den;
    if (y < h - 4) y++;
    int w = getmaxx(content);
    int bar_w = w - 14;
    if (bar_w < 10) bar_w = 10;
    ui_println(content, &y, h - 2, "Grafico (mais rapido = menor barra)");
    if (y < h - 2) {
        mvwprintw(content, y, 2, "CSR   ");
        desenhar_barra(content, y, 10, bar_w, t_csr, maxv, 7);
        y++;
    }
    if (y < h - 2) {
        mvwprintw(content, y, 2, "Tri   ");
        desenhar_barra(content, y, 10, bar_w, t_tri, maxv, 7);
        y++;
    }
    if (y < h - 2) {
        mvwprintw(content, y, 2, "Densa ");
        desenhar_barra(content, y, 10, bar_w, t_den, maxv, 7);
        y++;
    }
    wrefresh(content);
}

static void ler_linha(WINDOW *win, int y, const char *msg, char *buf, size_t n) {
    mvwprintw(win, y, 2, "%s", msg);
    wclrtoeol(win);
    echo();
    wgetnstr(win, buf, (int)n - 1);
    noecho();
}

static void alterar_parametros_tui(AppState *st, WINDOW *content) {
    char buf[64];
    int h = getmaxy(content);
    int y = 2;
    werase(content);
    wbkgd(content, COLOR_PAIR(4));
    box(content, 0, 0);
    ui_title(content, "Alterar parametros");
    ui_println(content, &y, h - 2, "Atual: linhas=%d colunas=%d reps=%d densidade=%.0f%%",
               st->linhas, st->colunas, st->reps, st->densidade * 100.0f);
    ler_linha(content, y++, "Linhas (enter para manter): ", buf, sizeof(buf));
    st->linhas = parse_int_or_default(buf, st->linhas);
    ler_linha(content, y++, "Colunas (enter para manter): ", buf, sizeof(buf));
    st->colunas = parse_int_or_default(buf, st->colunas);
    ler_linha(content, y++, "Repeticoes (enter para manter): ", buf, sizeof(buf));
    st->reps = parse_int_or_default(buf, st->reps);
    ler_linha(content, y++, "Densidade (1, 5, 20 ou 0.01/0.05/0.20): ", buf, sizeof(buf));
    st->densidade = parse_densidade(buf, st->densidade);
    construir_estado(st, 1234u);
}

static void draw_header(const AppState *st, Ui *ui) {
    werase(ui->header);
    wbkgd(ui->header, COLOR_PAIR(1));
    wattron(ui->header, A_BOLD);
    mvwprintw(ui->header, 0, 2, "CSR Explorer");
    mvwprintw(ui->header, 0, ui->width - 28, "linhas=%d colunas=%d", st->linhas, st->colunas);
    wattroff(ui->header, A_BOLD);
    wrefresh(ui->header);
}

static void draw_menu(const AppState *st, Ui *ui, int selected) {
    (void)st;
    werase(ui->menu);
    wbkgd(ui->menu, COLOR_PAIR(2));
    box(ui->menu, 0, 0);
    wattron(ui->menu, A_BOLD);
    mvwprintw(ui->menu, 1, 2, "Menu");
    wattroff(ui->menu, A_BOLD);
    const char *items[] = {
        "Vetores CSR",
        "Matriz densa",
        "Benchmark",
        "Alterar parametros",
        "Ajuda",
        "Sair"
    };
    for (int i = 0; i < 6; i++) {
        if (i == selected) wattron(ui->menu, COLOR_PAIR(5) | A_BOLD);
        mvwprintw(ui->menu, 3 + i * 2, 2, "%d) %s", i + 1, items[i]);
        if (i == selected) wattroff(ui->menu, COLOR_PAIR(5) | A_BOLD);
    }
    wrefresh(ui->menu);
}

static void draw_status(const AppState *st, Ui *ui, const char *msg) {
    char buf[128];
    snprintf(buf, sizeof(buf), "densidade=%.0f%% reps=%d | %s", st->densidade * 100.0f, st->reps, msg);
    ui_status(ui->status, buf);
}

static void mostrar_ajuda_tui(WINDOW *content) {
    int h = getmaxy(content);
    int y = 2;
    werase(content);
    wbkgd(content, COLOR_PAIR(4));
    box(content, 0, 0);
    ui_title(content, "Ajuda e atalhos");
    ui_println(content, &y, h - 2, "Setas: mover no menu");
    ui_println(content, &y, h - 2, "Enter: abrir item selecionado");
    ui_println(content, &y, h - 2, "1-6 : abrir item direto");
    ui_println(content, &y, h - 2, "h   : abrir esta ajuda");
    ui_println(content, &y, h - 2, "q   : sair");
    ui_println(content, &y, h - 2, "");
    ui_println(content, &y, h - 2, "No painel de parametros, use Enter para manter valores.");
    wrefresh(content);
}

void run_ncurses(int linhas, int colunas, int reps) {
    AppState st;
    memset(&st, 0, sizeof(st));
    st.linhas = linhas;
    st.colunas = colunas;
    st.reps = reps;
    st.densidade = 0.05f;
    construir_estado(&st, 1234u);

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    if (has_colors()) {
        start_color();
        use_default_colors();
        init_pair(1, COLOR_WHITE, COLOR_BLUE);
        init_pair(2, COLOR_WHITE, COLOR_BLACK);
        init_pair(3, COLOR_BLACK, COLOR_WHITE);
        init_pair(4, COLOR_WHITE, COLOR_BLACK);
        init_pair(5, COLOR_BLACK, COLOR_CYAN);
        init_pair(6, COLOR_BLACK, COLOR_YELLOW);
        init_pair(7, COLOR_WHITE, COLOR_MAGENTA);
    }

    Ui ui;
    ui.height = LINES;
    ui.width = COLS;
    int header_h = 1;
    int status_h = 1;
    int menu_w = 28;
    ui.header = newwin(header_h, ui.width, 0, 0);
    ui.status = newwin(status_h, ui.width, ui.height - 1, 0);
    ui.menu = newwin(ui.height - header_h - status_h, menu_w, header_h, 0);
    ui.content = newwin(ui.height - header_h - status_h, ui.width - menu_w, header_h, menu_w);
    wbkgd(stdscr, COLOR_PAIR(4));

    int running = 1;
    int selected = 0;
    while (running) {
        draw_header(&st, &ui);
        draw_menu(&st, &ui, selected);
        draw_status(&st, &ui, "Use setas e Enter | h ajuda | q sair");
        int ch = getch();
        switch (ch) {
            case '1':
                mostrar_csr_tui(&st, ui.content);
                draw_status(&st, &ui, "Vetores CSR exibidos");
                break;
            case '2':
                mostrar_densa_tui(&st, ui.content);
                draw_status(&st, &ui, "Matriz densa exibida");
                break;
            case '3':
                mostrar_benchmark_tui(&st, ui.content);
                draw_status(&st, &ui, "Benchmark concluido");
                break;
            case '4':
                alterar_parametros_tui(&st, ui.content);
                draw_status(&st, &ui, "Parametros atualizados");
                break;
            case '5':
                mostrar_ajuda_tui(ui.content);
                draw_status(&st, &ui, "Ajuda exibida");
                break;
            case '6':
            case 'q':
            case 'Q':
                running = 0;
                break;
            case 'h':
            case 'H':
                mostrar_ajuda_tui(ui.content);
                draw_status(&st, &ui, "Ajuda exibida");
                break;
            case KEY_UP:
                if (selected > 0) selected--;
                break;
            case KEY_DOWN:
                if (selected < 5) selected++;
                break;
            case '\n':
            case KEY_ENTER:
                switch (selected) {
                    case 0: mostrar_csr_tui(&st, ui.content); draw_status(&st, &ui, "Vetores CSR exibidos"); break;
                    case 1: mostrar_densa_tui(&st, ui.content); draw_status(&st, &ui, "Matriz densa exibida"); break;
                    case 2: mostrar_benchmark_tui(&st, ui.content); draw_status(&st, &ui, "Benchmark concluido"); break;
                    case 3: alterar_parametros_tui(&st, ui.content); draw_status(&st, &ui, "Parametros atualizados"); break;
                    case 4: mostrar_ajuda_tui(ui.content); draw_status(&st, &ui, "Ajuda exibida"); break;
                    case 5: running = 0; break;
                    default: break;
                }
                break;
            default:
                break;
        }
    }

    delwin(ui.header);
    delwin(ui.menu);
    delwin(ui.content);
    delwin(ui.status);
    endwin();
    liberar_estado(&st);
}
