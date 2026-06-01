#ifndef CSR_H
#define CSR_H

#include <stddef.h>

typedef struct {
    int linhas;
    int colunas;
    int total_nao_nulos;
    float *valores;
    int   *col_indices;
    int   *row_ptr;
} CSR;

typedef struct { int l; int c; float v; } Tripla;
typedef struct { int linhas; int colunas; int total; Tripla *dados; } MatrizTriplas;

CSR *construir_csr(const MatrizTriplas *mt);
float acessar(const CSR *csr, int i, int j);
float *linha_para_array(const CSR *csr, int i);
float *somar_linhas(const CSR *csr, int i, int j);
float produto_escalar_linhas(const CSR *csr, int i, int j);
MatrizTriplas csr_para_triplas(const CSR *csr);
void imprimir_csr(const CSR *csr);
void imprimir_densa(const CSR *csr);
void imprimir_triplas(const MatrizTriplas *mt);
void liberar_csr(CSR *csr);
int verificar_integridade_csr(const CSR *csr);

float acessar_triplas(const MatrizTriplas *mt, int i, int j);
float acessar_densa(const float *mat, int linhas, int colunas, int i, int j);

MatrizTriplas gerar_triplas_aleatorias(int linhas, int colunas, float densidade, unsigned int seed);
float *triplas_para_densa(const MatrizTriplas *mt);
void liberar_triplas(MatrizTriplas *mt);

double medir_acesso_csr(const CSR *csr, int reps, int linhas, int colunas);
double medir_acesso_triplas(const MatrizTriplas *mt, int reps, int linhas, int colunas);
double medir_acesso_densa(const float *mat, int reps, int linhas, int colunas);

int parse_int_or_default(const char *s, int def);
float parse_densidade(const char *s, float def);

#endif
