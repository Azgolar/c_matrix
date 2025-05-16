#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

// Erzeugt ein Array mit Zahlen im Bereich [start, ende] mit adaptiver Schrittweite
void konvertieren(unsigned int start, unsigned int ende, unsigned int **arr, size_t *len)
{
    size_t max_steps = 100;
    unsigned int *temp = malloc(max_steps * sizeof(unsigned int));

    unsigned int last = start;
    unsigned int next_pow2 = 1;
    size_t i = 0;

    temp[i] = start;
    i = i + 1;

    while (last < ende) {
        unsigned int step;
        if      (last <= 9)     step = 4;
        else if (last <= 99)    step = 6;
        else if (last <= 999)   step = 100;
        else if (last <= 9999)  step = 500;
        else                    step = 1000;

        unsigned int next = last + step;

		if (next_pow2 > last && next_pow2 <= ende) {
			if (next_pow2 < next) {
				temp[i++] = next_pow2;
				last       = next_pow2;
				next_pow2 <<= 1;
				continue;
			}
		}

        if (next >= ende) {
            temp[i] = ende;
            i++;
            break;
        }

        temp[i] = next;
        i++;
        last = next;
		
		if (i > 100)
		{
			printf("\n darf maximal 100 Messwerte haben\n");
			break;
		}
    }

    *arr = temp;
    *len = i;
}


// Speichert Prozessorinfo und Messdaten in eine Datei
void speichern(const char *filename, unsigned int *n, double *laufzeit, size_t len, unsigned int threads) {
	FILE *cpu = fopen("/proc/cpuinfo", "r");

	char line[256];
	char model_name[256] = "";
	unsigned int logical = 0, physical = 0;

	while (fgets(line, sizeof(line), cpu)) {
		if (!model_name[0] && strncmp(line, "model name", 10) == 0) {
			char *p = strchr(line, ':') + 2;  
			strcpy(model_name, p);
			model_name[strcspn(model_name, "\n")] = '\0';
		}
		if (strncmp(line, "processor", 9) == 0) {
			logical++;
		}
		if (!physical && strncmp(line, "cpu cores", 9) == 0) {
			physical = atoi(strchr(line, ':') + 1);
		}
	}

	fclose(cpu);

	unsigned int hyperthreading = logical / physical;

    // Prüfen, ob Datei existiert
    struct stat st;
    int exists = (stat(filename, &st) == 0);

    FILE *f = fopen(filename, "a");
    if (!f) {
        fprintf(stderr, "Fehler beim Öffnen der Datei %s: %s\n", filename, strerror(errno));
        exit(1);
    }

    // Kopfzeile nur schreiben, wenn Datei neu
    if (!exists) {
        fprintf(f, "%s,%u,%u,%u\n", model_name, logical, physical, hyperthreading);
    }

    // Messdaten anhängen
    for (size_t i = 0; i < len; i++) {
        fprintf(f, "%u,%u,%.6f\n", threads, n[i], laufzeit[i]);
    }
    fclose(f);

    if (exists)
	{
		printf("Daten erfolgreich angehängt an %s.\n", filename);
	}
    else
	{
		printf("Daten erfolgreich in %s geschrieben.\n", filename);
	}
}