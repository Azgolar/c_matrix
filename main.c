#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <time.h>
#include <pthread.h>

// für Verarbeitung der Eingabe und Ausgabe
#include "verarbeiten.c"

typedef struct {
    int anfang;
    int ende;
    int n;
    int **a;
    int **b;
    int **c;
} daten;

void sequentiell(int **a, int **b, int **c, int n) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            int summe = 0;
            for (int k = 0; k < n; k++) {
                summe += a[i][k] * b[k][j];
            }
            c[i][j] = summe;
        }
    }
}

void* parallel_multiplizieren(void *arg) {
    daten *d = (daten*)arg;
    for (int i = d->anfang; i < d->ende; i++) {
        for (int j = 0; j < d->n; j++) {
            int summe = 0;
            for (int k = 0; k < d->n; k++) {
                summe += d->a[i][k] * d->b[k][j];
            }
            d->c[i][j] = summe;
        }
    }
    return NULL;
}

void parallel_multiply(int **a, int **b, int **c, int n, int num_threads) {
    pthread_t threads[num_threads];
    daten args[num_threads];
    int base = n / num_threads;
    int rem  = n % num_threads;
    int start = 0;

    for (int t = 0; t < num_threads; t++) {
        args[t].anfang = start;
        args[t].ende   = start + base + (rem-- > 0 ? 1 : 0);
        args[t].n      = n;
        args[t].a      = a;
        args[t].b      = b;
        args[t].c      = c;
        start = args[t].ende;
    }
    for (int t = 0; t < num_threads; t++)
        pthread_create(&threads[t], NULL, parallel_multiplizieren, &args[t]);
    for (int t = 0; t < num_threads; t++)
        pthread_join(threads[t], NULL);
}

static int** alloc_matrix(int n) {
    int **m = malloc(n * sizeof *m);
    if (!m) {
        perror("malloc rows");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < n; i++) {
        m[i] = malloc(n * sizeof *m[i]);
        if (!m[i]) {
            perror("malloc row");
            exit(EXIT_FAILURE);
        }
    }
    return m;
}

static void free_matrix(int **m, int n) {
    for (int i = 0; i < n; i++)
        free(m[i]);
    free(m);
}

int main(int argc, char *argv[]) {
    char *n_opt = "0";
    char *datei = NULL;
    int debug = 1, opt;

    while ((opt = getopt(argc, argv, "n:c:dh")) != -1) {
        switch (opt) {
            case 'n': n_opt = optarg;          break;
            case 'c': datei = strdup(optarg);  break;
            case 'd': debug = 0;               break;
            case 'h':
                printf(
                    "\nOptional:\n"
                    "-n <zahl>  Matrixgrößen im Bereich [6,zahl] erzeugen\n"
                    "-c <datei> Ergebnisdatei, Default: matrix.txt\n"
                    "-d         Debug-Ausgaben\n"
                );
                return 0;
            default:
                fprintf(stderr, "Unbekannte Option '-%c'. Benutzung siehe -h\n", opt);
                return 1;
        }
    }

    if (!datei) datei = strdup("matrix.txt");
    if (!strrchr(datei, '.') || strcmp(strrchr(datei, '.'), ".txt") != 0) {
        char *hinzu = malloc(strlen(datei) + 5);
        sprintf(hinzu, "%s.txt", datei);
        free(datei);
        datei = hinzu;
    }

    int max_n = strtoul(n_opt, NULL, 10);
    int *n = NULL, n_laenge = 0;
    konvertieren(6, max_n, &n, &n_laenge);

    int max_threads = sysconf(_SC_NPROCESSORS_ONLN);
    printf("Threads: %d\n", max_threads);
	max_threads = 6;
	
    for (int threads = 2; threads <= max_threads; threads++) {
        double *dauern = malloc(n_laenge * sizeof *dauern);

        for (int i = 0; i < n_laenge; i++) {
            int aktuell = n[i];
            int **a = alloc_matrix(aktuell);
            int **b = alloc_matrix(aktuell);
            int **c = alloc_matrix(aktuell);

            /* Initialisieren */
            for (int i = 0; i < aktuell; i++)
                for (int j = 0; j < aktuell; j++) {
                    a[i][j] = rand();
                    b[i][j] = rand();
                    c[i][j] = 0;
                }

            /* Zeitmessung */
            struct timespec t0, t1;
            clock_gettime(CLOCK_MONOTONIC, &t0);
            parallel_multiply(a, b, c, aktuell, threads);
            clock_gettime(CLOCK_MONOTONIC, &t1);

            dauern[i] = (t1.tv_sec - t0.tv_sec)*1e3
                        + (t1.tv_nsec - t0.tv_nsec)/1e6;

            if (debug) {
                int **c_sequentiell = alloc_matrix(aktuell);
                sequentiell(a, b, c_sequentiell, aktuell);

                int fehler = 0;
                for (int i = 0; i < aktuell && !fehler; i++) {
                    for (int j = 0; j < aktuell; j++) {
                        if (c_sequentiell[i][j] != c[i][j]) {
                            fehler = 1;
                            break;
                        }
                    }
                }
				if (fehler == 0) {
					printf("Ergebnis ist korrekt\n");
				}
				else {
                    printf("Ergebnis ist falsch");
                free_matrix(c_sequentiell, aktuell);
				}
            }

            free_matrix(a, aktuell);
            free_matrix(b, aktuell);
            free_matrix(c, aktuell);
        }

        speichern(datei, n, dauern, n_laenge, threads);
        free(dauern);
    }

    free(n);
    free(datei);
    return 0;
}
