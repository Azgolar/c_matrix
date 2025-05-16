#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

// Erzeugt ein Array mit Zahlen im Bereich [start, ende] mit adaptiver Schrittweite
void konvertieren(int start, int ende, int **arr, int *len)
{
    size_t max_steps = 100;
    int *temp = malloc(max_steps * sizeof(int));

    int letztes = start;
    int zweier = 1;
    int i = 0;

    temp[i] = start;
    i = i + 1;

	int schritt;
	int naechstes;
	
    while (letztes < ende) {
        if      (letztes <= 9)     schritt = 4;
        else if (letztes <= 99)    schritt = 6;
        else if (letztes <= 999)   schritt = 100;
        else if (letztes <= 9999)  schritt = 500;
        else                       schritt = 1000;

        naechstes = letztes + schritt;

		if (zweier > letztes && zweier <= ende) {
			if (zweier < naechstes) {
				temp[i] = zweier;
				i = i + 1;
				letztes       = zweier;
				zweier = zweier * 2;
				continue;
			}
		}

        if (naechstes >= ende) {
            temp[i] = ende;
            i = i + 1;
            break;
        }

        temp[i] = naechstes;
        i = i + 1;
        letztes = naechstes;
		
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
void speichern(const char *filename, int *n, double *laufzeit, int len, int threads) {
	FILE *cpu = fopen("/proc/cpuinfo", "r");

	char line[256];
	char model_name[256] = "";
	int logical = 0, physical = 0;

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

	int hyperthreading = logical / physical;

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
    for (int i = 0; i < len; i++) {
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