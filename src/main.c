#include "csr.h"
#include "tui.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
    int linhas = 6;
    int colunas = 6;
    int reps = 30;
    int use_tui = 0;
    int num_idx = 0;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--tui") == 0) {
            use_tui = 1;
        } else if (num_idx == 0) {
            linhas = parse_int_or_default(argv[i], linhas);
            num_idx++;
        } else if (num_idx == 1) {
            colunas = parse_int_or_default(argv[i], colunas);
            num_idx++;
        } else if (num_idx == 2) {
            reps = parse_int_or_default(argv[i], reps);
            num_idx++;
        }
    }

    if (use_tui) {
        run_ncurses(linhas, colunas, reps);
        return 0;
    }

    float densidades[] = {0.01f, 0.05f, 0.20f};
    int num_d = (int)(sizeof(densidades) / sizeof(densidades[0]));

    for (int d = 0; d < num_d; d++) {
        float dens = densidades[d];
        MatrizTriplas mt = gerar_triplas_aleatorias(linhas, colunas, dens, 1234u + (unsigned int)d);
        CSR *csr = construir_csr(&mt);
        float *densa = triplas_para_densa(&mt);

        printf("\n--- Densidade %.0f%% ---\n", dens * 100.0f);
        printf("Total nao-nulos: %d\n", csr->total_nao_nulos);
        printf("Integridade CSR: %s\n", verificar_integridade_csr(csr) ? "OK" : "FALHA");

        imprimir_csr(csr);
        printf("\nMatriz densa reconstruida:\n");
        imprimir_densa(csr);

        printf("\nTriplas (linha, coluna, valor):\n");
        imprimir_triplas(&mt);

        printf("\nExemplo acessar(2,3): %.2f\n", acessar(csr, 2, 3));
        float *linha2 = linha_para_array(csr, 2);
        if (linha2) {
            printf("Linha 2 como vetor: ");
            for (int c = 0; c < colunas; c++) printf("%.2f ", linha2[c]);
            printf("\n");
            free(linha2);
        }
        float *soma = somar_linhas(csr, 1, 3);
        if (soma) {
            printf("Soma linhas 1 e 3: ");
            for (int c = 0; c < colunas; c++) printf("%.2f ", soma[c]);
            printf("\n");
            free(soma);
        }
        printf("Produto escalar linhas 1 e 3: %.2f\n", produto_escalar_linhas(csr, 1, 3));

        MatrizTriplas mt2 = csr_para_triplas(csr);
        printf("CSR -> Triplas (reconstruido):\n");
        imprimir_triplas(&mt2);

        double t_csr = medir_acesso_csr(csr, reps, linhas, colunas);
        double t_tri = medir_acesso_triplas(&mt, reps, linhas, colunas);
        double t_den = medir_acesso_densa(densa, reps, linhas, colunas);
        printf("\nTempo acesso (clock): CSR=%.6f s, Triplas=%.6f s, Densa=%.6f s\n",
               t_csr, t_tri, t_den);

        liberar_triplas(&mt);
        liberar_triplas(&mt2);
        free(densa);
        liberar_csr(csr);
    }

    return 0;
}
