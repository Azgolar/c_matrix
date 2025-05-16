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
    int *a;
    int *b;
    int *c;
} daten;

void sequentiell(int *a, int *b, int *c, int n) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            int summe = 0;
            for (int k = 0; k < n; k++) {
                summe += a[i*n + k] * b[k*n + j];
            }
            c[i*n + j] = summe;
        }
    }
}

void* multiplizieren(void *arg) {
    daten *d = (daten*)arg;
    for (int i = d->anfang; i < d->ende; i++) {
        for (int j = 0; j < d->n; j++) {
            int summe = 0;
            for (int k = 0; k < d->n; k++) {
                summe += d->a[i*d->n + k] * d->b[k*d->n + j];
            }
            d->c[i*d->n + j] = summe;
        }
    }
    return NULL;
}

void parallel_multiply(int *a, int *b, int *c, int n, int num_threads) {
    pthread_t threads[num_threads];
    daten args[num_threads];
    int base = n / num_threads;
    int rem = n % num_threads;
    int start = 0;
    for (int t = 0; t < num_threads; t++) {
        args[t].anfang = start;
        args[t].ende = start + base + (rem-- > 0 ? 1 : 0);
        args[t].n = n;
        args[t].a = a;
        args[t].b = b;
        args[t].c = c;
        start = args[t].ende;
    }
    for (int t = 0; t < num_threads; t++) {
        pthread_create(&threads[t], NULL, multiplizieren, &args[t]);
    }
    for (int t = 0; t < num_threads; t++) {
        pthread_join(threads[t], NULL);
    }
}

int main(int argc, char *argv[]) {
    char *n_opt = "0";
    char *datei = NULL;
    int debug = 1;
    int opt;

    while ((opt = getopt(argc, argv, "n:c:dh")) != -1) {
        switch (opt) {
            case 'n': n_opt = optarg; break;
            case 'c': datei = strdup(optarg); break;
            case 'd': debug = 0; break;
            case 'h':
                printf(
                    "\nOptional:\n"
                    "-n <zahl>  Matrixgrößen im Bereich [6,zahl] erzeugen\n"
                    "-c <datei> Ergebnisdatei, Default: matrix.txt\n"
                    "-d         Debug-Ausgaben\n"
                );
                return 0;
            default:
                printf("Unbekannte Option '-%c'. Benutzung siehe -h\n", opt);
                return 1;
        }
    }

    if (!datei) datei = strdup("matrix.txt");
    if (!strrchr(datei, '.') || strcmp(strrchr(datei, '.'), ".txt") != 0) {
        char *buf = malloc(strlen(datei) + 5);
        sprintf(buf, "%s.txt", datei);
        free(datei);
        datei = buf;
    }
	
    int max_n = strtoul(n_opt, NULL, 10);
	if (max_n == 0) {
        printf("Fehler: -n <zahl> muss angegeben sein. Benutzung siehe -h\n");
        return 1;
    }
    if (max_n < 6) {
        printf("Fehler: -n muss >= 6 sein\n");
        return 1;
    }

    int *n_vec = NULL;
    int n_len = 0;
    konvertieren(6, max_n, &n_vec, &n_len);

    int max_threads =  (int) sysconf(_SC_NPROCESSORS_ONLN);
	printf("Threads: %d", max_threads);
    for (int threads = 2; threads <= max_threads; threads++) {
        /* Array für alle Laufzeiten dieses Thread-Counts */
        double *dauern = malloc(n_len * sizeof *dauern);
        if (!dauern) {
            perror("malloc");
            return 1;
        }

        for (int idx = 0; idx < n_len; idx++) {
            unsigned int curr_n = n_vec[idx];
            size_t mat_elems = (size_t)curr_n * curr_n;

            /* Matrizen allozieren */
            int *a = malloc(mat_elems * sizeof *a);
            int *b = malloc(mat_elems * sizeof *b);
            int *c = malloc(mat_elems * sizeof *c);
            if (!a || !b || !c) {
                perror("malloc");
                return 1;
            }

            /* Initialisieren */
            for (int j = 0; j < mat_elems; j++) {
                a[j] = rand();
                b[j] = rand();
                c[j] = 0;
            }

            /* Zeitmessung */
            struct timespec t0, t1;
            clock_gettime(CLOCK_MONOTONIC, &t0);
            parallel_multiply(a, b, c, curr_n, threads);
            clock_gettime(CLOCK_MONOTONIC, &t1);

            dauern[idx] = (t1.tv_sec - t0.tv_sec) * 1e3 + (t1.tv_nsec - t0.tv_nsec) / 1e6;
						 
			if (debug) {
                int *c_seq = malloc(mat_elems * sizeof(*c_seq));
                sequentiell(a, b, c_seq, curr_n);
				int fehler = 0;
                for (int k = 0; k < mat_elems; k++) {
                    if (c_seq[k] != c[k]) {
						fehler = 1;
                        break;
                    }
                }
				if (fehler == 0)
				{
					printf("\nErgebnis korrekt\n");
				}
				else
				{
				printf("\nErgebnis falsch\n");
				}
                free(c_seq);
            }


            free(a);
            free(b);
            free(c);
        }

        /* Ergebnisse speichern */
        speichern(datei, n_vec, dauern, n_len, threads);
        free(dauern);
    }

    free(n_vec);
    free(datei);
    return 0;
}