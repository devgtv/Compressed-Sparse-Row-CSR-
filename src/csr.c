#include "csr.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static void *xmalloc(size_t size) {
    void *p = malloc(size);
    if (!p) {
        fprintf(stderr, "Erro: falta de memoria.\n");
        exit(1);
    }
    return p;
}

static void *xcalloc(size_t n, size_t size) {
    void *p = calloc(n, size);
    if (!p) {
        fprintf(stderr, "Erro: falta de memoria.\n");
        exit(1);
    }
    return p;
}

CSR *construir_csr(const MatrizTriplas *mt) {
    if (!mt) return NULL;

    CSR *csr = (CSR *)xmalloc(sizeof(CSR));
    csr->linhas = mt->linhas;
    csr->colunas = mt->colunas;
    csr->total_nao_nulos = mt->total;

    csr->valores = (float *)xmalloc(sizeof(float) * mt->total);
    csr->col_indices = (int *)xmalloc(sizeof(int) * mt->total);
    csr->row_ptr = (int *)xcalloc((size_t)mt->linhas + 1, sizeof(int));

    for (int k = 0; k < mt->total; k++) {
        int linha = mt->dados[k].l;
        if (linha < 0 || linha >= mt->linhas) {
            fprintf(stderr, "Tripla com linha invalida: %d\n", linha);
            exit(1);
        }
        csr->row_ptr[linha + 1]++;
    }

    for (int i = 0; i < mt->linhas; i++) {
        csr->row_ptr[i + 1] += csr->row_ptr[i];
    }

    for (int k = 0; k < mt->total; k++) {
        csr->valores[k] = mt->dados[k].v;
        csr->col_indices[k] = mt->dados[k].c;
    }

    return csr;
}

float acessar(const CSR *csr, int i, int j) {
    if (!csr) return 0.0f;
    if (i < 0 || i >= csr->linhas || j < 0 || j >= csr->colunas) return 0.0f;

    int inicio = csr->row_ptr[i];
    int fim = csr->row_ptr[i + 1];
    for (int k = inicio; k < fim; k++) {
        if (csr->col_indices[k] == j) return csr->valores[k];
    }
    return 0.0f;
}

float *linha_para_array(const CSR *csr, int i) {
    if (!csr || i < 0 || i >= csr->linhas) return NULL;
    float *linha = (float *)xcalloc((size_t)csr->colunas, sizeof(float));
    int inicio = csr->row_ptr[i];
    int fim = csr->row_ptr[i + 1];
    for (int k = inicio; k < fim; k++) {
        int c = csr->col_indices[k];
        if (c >= 0 && c < csr->colunas) {
            linha[c] = csr->valores[k];
        }
    }
    return linha;
}

float *somar_linhas(const CSR *csr, int i, int j) {
    if (!csr) return NULL;
    float *a = linha_para_array(csr, i);
    float *b = linha_para_array(csr, j);
    if (!a || !b) {
        free(a);
        free(b);
        return NULL;
    }
    for (int c = 0; c < csr->colunas; c++) {
        a[c] += b[c];
    }
    free(b);
    return a;
}

float produto_escalar_linhas(const CSR *csr, int i, int j) {
    if (!csr) return 0.0f;
    float *a = linha_para_array(csr, i);
    float *b = linha_para_array(csr, j);
    if (!a || !b) {
        free(a);
        free(b);
        return 0.0f;
    }
    float soma = 0.0f;
    for (int c = 0; c < csr->colunas; c++) {
        soma += a[c] * b[c];
    }
    free(a);
    free(b);
    return soma;
}

MatrizTriplas csr_para_triplas(const CSR *csr) {
    MatrizTriplas mt;
    mt.linhas = csr ? csr->linhas : 0;
    mt.colunas = csr ? csr->colunas : 0;
    mt.total = csr ? csr->total_nao_nulos : 0;
    mt.dados = NULL;
    if (!csr) return mt;

    mt.dados = (Tripla *)xmalloc(sizeof(Tripla) * (size_t)csr->total_nao_nulos);
    int idx = 0;
    for (int i = 0; i < csr->linhas; i++) {
        int inicio = csr->row_ptr[i];
        int fim = csr->row_ptr[i + 1];
        for (int k = inicio; k < fim; k++) {
            mt.dados[idx].l = i;
            mt.dados[idx].c = csr->col_indices[k];
            mt.dados[idx].v = csr->valores[k];
            idx++;
        }
    }
    return mt;
}

void imprimir_csr(const CSR *csr) {
    if (!csr) return;
    printf("valores: ");
    for (int k = 0; k < csr->total_nao_nulos; k++) {
        printf("%.2f ", csr->valores[k]);
    }
    printf("\ncol_indices: ");
    for (int k = 0; k < csr->total_nao_nulos; k++) {
        printf("%d ", csr->col_indices[k]);
    }
    printf("\nrow_ptr: ");
    for (int i = 0; i < csr->linhas + 1; i++) {
        printf("%d ", csr->row_ptr[i]);
    }
    printf("\n");
}

void imprimir_densa(const CSR *csr) {
    if (!csr) return;
    for (int i = 0; i < csr->linhas; i++) {
        int inicio = csr->row_ptr[i];
        int fim = csr->row_ptr[i + 1];
        int k = inicio;
        for (int j = 0; j < csr->colunas; j++) {
            float v = 0.0f;
            if (k < fim && csr->col_indices[k] == j) {
                v = csr->valores[k];
                k++;
            }
            printf("%6.2f ", v);
        }
        printf("\n");
    }
}

void imprimir_triplas(const MatrizTriplas *mt) {
    if (!mt) return;
    for (int k = 0; k < mt->total; k++) {
        printf("(%d, %d, %.2f) ", mt->dados[k].l, mt->dados[k].c, mt->dados[k].v);
    }
    printf("\n");
}

void liberar_csr(CSR *csr) {
    if (!csr) return;
    free(csr->valores);
    free(csr->col_indices);
    free(csr->row_ptr);
    free(csr);
}

int verificar_integridade_csr(const CSR *csr) {
    if (!csr) return 0;
    if (!csr->row_ptr) return 0;
    if (csr->row_ptr[0] != 0) return 0;
    if (csr->row_ptr[csr->linhas] != csr->total_nao_nulos) return 0;
    for (int i = 0; i < csr->linhas; i++) {
        if (csr->row_ptr[i] > csr->row_ptr[i + 1]) return 0;
    }
    return 1;
}

float acessar_triplas(const MatrizTriplas *mt, int i, int j) {
    if (!mt) return 0.0f;
    for (int k = 0; k < mt->total; k++) {
        if (mt->dados[k].l == i && mt->dados[k].c == j) return mt->dados[k].v;
    }
    return 0.0f;
}

float acessar_densa(const float *mat, int linhas, int colunas, int i, int j) {
    (void)linhas;
    if (!mat) return 0.0f;
    return mat[i * colunas + j];
}

static unsigned int lcg_next(unsigned int *state) {
    *state = (*state * 1664525u) + 1013904223u;
    return *state;
}

static float rand_float(unsigned int *state) {
    unsigned int r = lcg_next(state);
    return (float)(r % 1000) / 100.0f + 1.0f;
}

MatrizTriplas gerar_triplas_aleatorias(int linhas, int colunas, float densidade, unsigned int seed) {
    MatrizTriplas mt;
    mt.linhas = linhas;
    mt.colunas = colunas;

    int total = (int)(linhas * colunas * densidade);
    if (total < 1) total = 1;
    if (total > linhas * colunas) total = linhas * colunas;
    mt.total = total;
    mt.dados = (Tripla *)xmalloc(sizeof(Tripla) * (size_t)total);

    unsigned int st = seed;
    int idx = 0;
    for (int i = 0; i < linhas && idx < total; i++) {
        for (int j = 0; j < colunas && idx < total; j++) {
            unsigned int r = lcg_next(&st);
            if ((r % 10000u) < (unsigned int)(densidade * 10000.0f)) {
                mt.dados[idx].l = i;
                mt.dados[idx].c = j;
                mt.dados[idx].v = rand_float(&st);
                idx++;
            }
        }
    }

    while (idx < total) {
        int i = (int)(lcg_next(&st) % (unsigned int)linhas);
        int j = (int)(lcg_next(&st) % (unsigned int)colunas);
        mt.dados[idx].l = i;
        mt.dados[idx].c = j;
        mt.dados[idx].v = rand_float(&st);
        idx++;
    }

    mt.total = idx;
    return mt;
}

float *triplas_para_densa(const MatrizTriplas *mt) {
    if (!mt) return NULL;
    float *mat = (float *)xcalloc((size_t)mt->linhas * (size_t)mt->colunas, sizeof(float));
    for (int k = 0; k < mt->total; k++) {
        int i = mt->dados[k].l;
        int j = mt->dados[k].c;
        if (i >= 0 && i < mt->linhas && j >= 0 && j < mt->colunas) {
            mat[i * mt->colunas + j] = mt->dados[k].v;
        }
    }
    return mat;
}

void liberar_triplas(MatrizTriplas *mt) {
    if (!mt) return;
    free(mt->dados);
    mt->dados = NULL;
    mt->total = 0;
}

double medir_acesso_csr(const CSR *csr, int reps, int linhas, int colunas) {
    clock_t ini = clock();
    volatile float sink = 0.0f;
    for (int r = 0; r < reps; r++) {
        for (int i = 0; i < linhas; i++) {
            for (int j = 0; j < colunas; j++) {
                sink += acessar(csr, i, j);
            }
        }
    }
    clock_t fim = clock();
    (void)sink;
    return (double)(fim - ini) / CLOCKS_PER_SEC;
}

double medir_acesso_triplas(const MatrizTriplas *mt, int reps, int linhas, int colunas) {
    clock_t ini = clock();
    volatile float sink = 0.0f;
    for (int r = 0; r < reps; r++) {
        for (int i = 0; i < linhas; i++) {
            for (int j = 0; j < colunas; j++) {
                sink += acessar_triplas(mt, i, j);
            }
        }
    }
    clock_t fim = clock();
    (void)sink;
    return (double)(fim - ini) / CLOCKS_PER_SEC;
}

double medir_acesso_densa(const float *mat, int reps, int linhas, int colunas) {
    clock_t ini = clock();
    volatile float sink = 0.0f;
    for (int r = 0; r < reps; r++) {
        for (int i = 0; i < linhas; i++) {
            for (int j = 0; j < colunas; j++) {
                sink += acessar_densa(mat, linhas, colunas, i, j);
            }
        }
    }
    clock_t fim = clock();
    (void)sink;
    return (double)(fim - ini) / CLOCKS_PER_SEC;
}

int parse_int_or_default(const char *s, int def) {
    char *end = NULL;
    long v = strtol(s, &end, 10);
    if (!s || s[0] == '\0' || (end && *end != '\0')) return def;
    if (v < 1) return def;
    return (int)v;
}

float parse_densidade(const char *s, float def) {
    if (!s || s[0] == '\0') return def;
    if (strcmp(s, "1") == 0 || strcmp(s, "0.01") == 0 || strcmp(s, "1%") == 0) return 0.01f;
    if (strcmp(s, "5") == 0 || strcmp(s, "0.05") == 0 || strcmp(s, "5%") == 0) return 0.05f;
    if (strcmp(s, "20") == 0 || strcmp(s, "0.2") == 0 || strcmp(s, "0.20") == 0 || strcmp(s, "20%") == 0) return 0.20f;
    return def;
}
