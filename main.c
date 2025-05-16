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
                summe = summe + a[i][k] * b[k][j];
            }
            c[i][j] = summe;
        }
    }
}

void* parallel_multiplizieren(void *arg) {
    daten *inhalt = (daten*)arg;
    for (int i = inhalt->anfang; i < inhalt->ende; i++) {
        for (int j = 0; j < inhalt->n; j++) {
            int summe = 0;
            for (int k = 0; k < inhalt->n; k++) {
                summe = summe + inhalt->a[i][k] * inhalt->b[k][j];
            }
            inhalt->c[i][j] = summe;
        }
    }
    return NULL;
}

void parallel_multiply(int **a, int **b, int **c, int n, int num_threads) {
    pthread_t threads[num_threads];
    daten args[num_threads];
    int base = n / num_threads;
    int rest  = n % num_threads;
    int start = 0;

    for (int i = 0; i < num_threads; i++) {
		
		if (rest > 0) {
			args[i].ende = start + base + 1;
			rest = rest - 1;
		} else {
			args[i].ende = start + base;
		}
        args[i].anfang = start;
        args[i].n      = n;
        args[i].a      = a;
        args[i].b      = b;
        args[i].c      = c;
        start = args[i].ende;
    }
    for (int j = 0; j < num_threads; j++) {
        pthread_create(&threads[j], NULL, parallel_multiplizieren, &args[j]);
	}
    for (int k = 0; k < num_threads; k++) {
        pthread_join(threads[k], NULL);
	}
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
            int **a =  malloc(aktuell * sizeof(int*));
            int **b =  malloc(aktuell * sizeof(int*));
            int **c =  malloc(aktuell * sizeof(int*));
			
			// Speicher für Reihen allokieren
			for (int i = 0; i < aktuell; i++) {
				a[i] = malloc(aktuell * sizeof *a[i]);
				b[i] = malloc(aktuell * sizeof *b[i]);
				c[i] = malloc(aktuell * sizeof *c[i]);
			}


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
                int **c_sequentiell = malloc(aktuell * sizeof *c_sequentiell);
				for (int r = 0; r < aktuell; r++) {
					c_sequentiell[r] = malloc(aktuell * sizeof *c_sequentiell[r]);
				}
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
					
                for (int i = 0; i < aktuell; ++i) {
					free(c_sequentiell[i]); 
				}
				free(c_sequentiell); 
				}
            }

            for (int i = 0; i < aktuell; ++i) {
				free(a[i]);
				free(b[i]); 
				free(c[i]); 				
			}
			free(a);
			free(b);
			free(c);
        }

        speichern(datei, n, dauern, n_laenge, threads);
        free(dauern);
    }

    free(n);
    free(datei);
    return 0;
}
