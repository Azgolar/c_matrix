#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

// Erzeugt ein Array mit Zahlen im Bereich [start, ende] mit adaptiver Schrittweite
void konvertieren(int start, int ende, int **arr, int *len)
{
	if (ende == 0) {
		printf("Fehler: -n <zahl> muss angegeben sein. Benutzung siehe -h\n");
        exit(1);
    }
	
    if (ende < 6) {
        printf("Fehler: -n muss >= 6 sein\n");
        exit(1);
    }
	
	// maximal 100 Messwerte
    int *temp = malloc(100 * sizeof(int));

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
void speichern(const char *dateiname, int *n, double *laufzeit, int len, int threads) {
	FILE *cpu = fopen("/proc/cpuinfo", "r");

	char line[256];
	char name[80] = "";
	int logical = 0, physical = 0;

	// Lesen aller zeilen
	while (fgets(line, sizeof(line), cpu)) {
		// Prozessormodell lesen
		if (!name[0] && strncmp(line, "model name", 10) == 0) {
			char *p = strchr(line, ':') + 2;  
			strcpy(name, p);
			name[strcspn(name, "\n")] = '\0';
		}
		// vorkommen "processor" ist identisch zur Anzahl der logischen Kerne 
		if (strncmp(line, "processor", 9) == 0) {
			logical = logical + 1;
		}
		// Anzahl der physischen Kerne aus Zeile lesen
		if (!physical && strncmp(line, "cpu cores", 9) == 0) {
			physical = atoi(strchr(line, ':') + 1);
		}
	}

	fclose(cpu);

	int hyperthreading = 0;
	if (physical > 0)
	{
		hyperthreading = logical / physical;
	}

    // Prüfen, ob Datei existiert
	// stat ist in stat.h enthalt und hat die Metadaten der Datei
    struct stat metadaten;
    int exists = (stat(dateiname, &metadaten) == 0);

    FILE *f = fopen(dateiname, "a");
    if (!f) {
        printf("Fehler beim Öffnen der Datei zum Speichern der Daten");
        exit(1);
    }

    // Kopfzeile nur schreiben, wenn Datei neu
    if (!exists) {
        fprintf(f, "%s,%u,%u,%u\n", dateiname, logical, physical, hyperthreading);
    }

    // Messdaten anhängen
    for (int i = 0; i < len; i++) {
        fprintf(f, "%u,%u,%.6f\n", threads, n[i], laufzeit[i]);
    }
    fclose(f);

    if (exists)
	{
		printf("Daten erfolgreich angehängt an %s.\n", dateiname);
	}
    else
	{
		printf("Daten erfolgreich in %s geschrieben.\n", dateiname);
	}
}