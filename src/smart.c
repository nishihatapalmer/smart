/*
 * SMART: string matching algorithms research tool.
 * Copyright (C) 2012  Simone Faro and Thierry Lecroq
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 *
 * contact the authors at: faro@dmi.unict.it and thierry.lecroq@univ-rouen.fr
 * download the tool at: http://www.dmi.unict.it/~faro/smart/
 */

#define PATTERN_SIZE_MAX 4200 // maximal length of the pattern

#define SIGMA 256 // constant alphabet size
#define NumAlgo 500
#define NumPatt 17	  // maximal number of pattern lengths
#define NumSetting 15 // number of text buffers
#define MAXTIME 999.00

/* DEFINITION OF VARIABLES */
unsigned int MINLEN = 1, MAXLEN = 4200; // min length and max length of pattern size

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>
#include <sys/stat.h>
#include "timer.h"
#include "sets.h"
#include "output.h"
#include "parser.h"
#include "select.c"
#include <libgen.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <dirent.h>

#define TEXT_SIZE_DEFAULT 1048576

#define TOP_EDGE_WIDTH 60

void print_top_edge(int len)
{
	int i;
	fprintf(stdout, "\t");
	for (i = 0; i < len; i++)
		fprintf(stdout, "%c", '_');
	fprintf(stdout, "\n");
}

void print_percentage(int perc)
{
	if (perc < 10)
		printf("\b\b\b\b[%d%%]", perc);
	else if (perc < 100)
		printf("\b\b\b\b\b[%d%%]", perc);
	else
		printf("\b\b\b\b[%d%%]", perc);
	fflush(stdout);
}

int load_text(const char *filename, char *T, int max_len)
{
	printf("\tLoading the file %s\n", filename);

	FILE *input = fopen(filename, "r");
	if (input == NULL)
		return -1;

	int i = 0;
	char c;
	while (i < max_len && (c = getc(input)) != EOF)
		T[i++] = c;

	fclose(input);
	return i;
}

int merge_all_texts(const char *path, char *T, int max_text_size)
{
	int curr_size = 0;

	DIR *dir = dir = opendir(path);
	if (dir != NULL)
	{

		struct dirent *entry;
		while ((entry = readdir(dir)) != NULL)
		{
			if (entry->d_type == DT_REG)
			{
				// TODO: extract function join_path();
				char full_path[STR_BUF];
				memset(full_path, 0, sizeof(char) * STR_BUF);

				strcat(full_path, path);
				strcat(full_path, "/");
				strcat(full_path, entry->d_name);

				int size = load_text(full_path, T + curr_size, max_text_size - curr_size);
				if (size < 0)
					return 0;

				curr_size += size;
			}
		}
		closedir(dir);
	}

	T[curr_size < max_text_size ? curr_size : max_text_size - 1] = '\0';

	return curr_size;
}

int gen_search_text(const char *path, unsigned char *T, int max_text_size)
{
	return merge_all_texts(path, T, max_text_size);
}

#define MAX_ALLOC_TRIALS 10

int alloc_shared_mem(int size, void **buffer, key_t *tkey)
{
	int shmid = -1;
	for (int n = 0; shmid < 0 && n < MAX_ALLOC_TRIALS; n++)
	{
		*tkey = rand() % 1000;
		shmid = shmget(*tkey, sizeof(char) * size, IPC_CREAT | 0666);
	}

	if (shmid < 0)
		return shmid;

	if ((*buffer = shmat(shmid, NULL, 0)) == (unsigned char *)-1)
	{
		printf("\nShared memory allocation failed!\nYou need at least 12Mb of shared memory\nPlease, change your system settings and try again.\n");
		perror("shmat");
		shmctl(shmid, IPC_RMID, 0);
		exit(1);
	}
	return shmid;
}

void gen_random_patterns(unsigned char **patterns, int m, unsigned char *T, int n, int num_patterns)
{
	for (int i = 0; i < num_patterns; i++)
	{
		int j;
		int k = random() % (n - m);
		for (j = 0; j < m; j++)
			patterns[i][j] = T[k + j];

		patterns[i][j] = '\0';
	}
}

int execute_algo(const char *algo_name, key_t pkey, int m, key_t tkey, int n, key_t rkey, key_t ekey, key_t prekey, int *count, int alpha)
{
	char command[100];
	sprintf(command, "./bin/algos/%s shared %d %d %d %d %d %d %d", str2lower(algo_name), pkey, m, tkey, n, rkey, ekey, prekey);
	int res = system(command);
	if (!res)
		return (*count);
	else
		return -1;
}

/* Free up shared memory allocated by sm execution */
void free_shm(unsigned char *T, unsigned char *P, int *count, double *e_time, double *pre_time,
			  int tshmid, int pshmid, int rshmid, int eshmid, int preshmid)
{
	shmdt(T);
	shmdt(P);
	shmdt(count);
	shmdt(e_time);
	shmdt(pre_time);
	shmctl(tshmid, IPC_RMID, 0);
	shmctl(pshmid, IPC_RMID, 0);
	shmctl(rshmid, IPC_RMID, 0);
	shmctl(eshmid, IPC_RMID, 0);
	shmctl(preshmid, IPC_RMID, 0);
}

double compute_average(double *T, int n)
{
	double avg = 0.0;
	for (int i = 0; i < n; i++)
		avg += T[i];
	return avg / n;
}

double compute_std(double avg, double *T, int n)
{
	double std = 0.0;
	for (int i = 0; i < n; i++)
		std += pow(avg - T[i], 2.0);

	return sqrt(std / n);
}

int run_setting(char *filename, key_t tkey, unsigned char *T, int n, const run_command_opts_t *opts,
				char *code, int tshmid, char *time_format)
{
	printf("\tExperimental tests started on %s\n", time_format);

	unsigned char **pattern_list = (unsigned char **)malloc(sizeof(unsigned char *) * opts->num_runs);
	for (int i = 0; i < opts->num_runs; i++)
		pattern_list[i] = (unsigned char *)malloc(sizeof(unsigned char) * (PATTERN_SIZE_MAX + 1));

	unsigned char c, *P;
	FILE *fp, *ip, *stream;
	/*
	char logfile[100];
	sprintf(logfile, "results/%s", code);
	mkdir(logfile, 0775);
	strcat(logfile, "/errorlog.txt");
	stream = freopen(logfile, "w", stderr); // redirect of stderr
	*/

	double *e_time, *pre_time;
	int *count;

	srand(time(NULL));

	// allocate space for running time in shared memory
	key_t ekey;
	int eshmid = alloc_shared_mem(sizeof(double), (void **)&e_time, &ekey);
	if (eshmid < 0)
	{
		perror("alloc_shared_mem");
		free_shm(T, P, count, e_time, pre_time, tshmid, -1, -1, eshmid, -1);
		exit(1);
	}

	key_t prekey;
	int preshmid = alloc_shared_mem(sizeof(double), (void **)&pre_time, &prekey);
	if (preshmid < 0)
	{
		perror("alloc_shared_mem");
		free_shm(T, P, count, e_time, pre_time, tshmid, -1, -1, eshmid, preshmid);
		exit(1);
	}

	// allocate space for pattern in shared memory
	key_t pkey;
	int pshmid = alloc_shared_mem(PATTERN_SIZE_MAX + 1, (void **)&P, &pkey);
	if (pshmid < 0)
	{
		perror("alloc_shared_mem");
		free_shm(T, P, count, e_time, pre_time, tshmid, pshmid, -1, eshmid, preshmid);
		exit(1);
	}

	// allocate space for the result number of occurrences in shared memory
	key_t rkey = rand() % 1000;
	int rshmid = alloc_shared_mem(4, (void **)&count, &rkey);
	if (rshmid < 0)
	{
		perror("alloc_shared_mem");
		free_shm(T, P, count, e_time, pre_time, tshmid, pshmid, rshmid, eshmid, preshmid);
		exit(1);
	}

	// count how many algorithms are going to run
	FILE *algo_file = fopen("selected_algos", "r");
	char algo_names[MAX_SELECT_ALGOS][STR_BUF];
	int num_running = list_algos_from_file(algo_file, algo_names);

	// initializes the vector which will contain running times
	// performs experiments on a text
	double SEARCH_TIME[num_running][NumPatt],
		PRE_TIME[num_running][NumPatt];

	for (int i = 0; i < num_running; i++)
		for (int j = 0; j < NumPatt; j++)
			SEARCH_TIME[i][j] = PRE_TIME[i][j] = 0;

	for (int pattern_size = 0; PATT_SIZE[pattern_size] > 0; pattern_size++)
	{
		if (PATT_SIZE[pattern_size] >= opts->pattern_min_len && PATT_SIZE[pattern_size] <= opts->pattern_max_len)
		{
			int m = PATT_SIZE[pattern_size];
			gen_random_patterns(pattern_list, m, T, n, opts->num_runs);

			printf("\n");
			print_top_edge(TOP_EDGE_WIDTH);

			printf("\tExperimental results on %s: %s\n", filename, code);

			printf("\tSearching for a set of %d patterns with length %d\n", opts->num_runs, m);
			printf("\tTesting %d algorithms\n", num_running);
			printf("\n");

			int current_running = 0;
			for (int algo = 0; algo < num_running; algo++)
			{
				current_running++;

				char output_line[30];
				sprintf(output_line, "\t - [%d/%d] %s ", current_running, num_running, str2upper(algo_names[algo]));
				printf("%s", output_line);
				fflush(stdout);

				for (int i = 0; i < 35 - strlen(output_line); i++)
					printf(".");

				double TIME[opts->num_runs];

				int total_occur = 0;
				for (int k = 1; k <= opts->num_runs; k++)
				{
					print_percentage((100 * k) / opts->num_runs);

					int j;
					for (j = 0; j < m; j++)
						P[j] = pattern_list[k - 1][j];
					P[j] = '\0'; // creates the pattern

					(*e_time) = (*pre_time) = 0.0;
					int occur = execute_algo(algo_names[algo], pkey, m, tkey, n, rkey, ekey, prekey, count, opts->alphabet_size);
					total_occur += occur;

					if (occur <= 0 && !opts->simple)
					{
						SEARCH_TIME[algo][pattern_size] = 0;
						PRE_TIME[algo][pattern_size] = 0;
						total_occur = occur;
						break;
					}

					if (!opts->pre)
						(*e_time) += (*pre_time);

					TIME[k] = (*e_time);
					PRE_TIME[algo][pattern_size] += (*pre_time);

					if ((*e_time) > opts->time_limit_millis)
					{
						SEARCH_TIME[algo][pattern_size] = 0;
						PRE_TIME[algo][pattern_size] = 0;
						total_occur = -2;
						break;
					}
				}

				SEARCH_TIME[algo][pattern_size] = compute_average(TIME, opts->num_runs);
				PRE_TIME[algo][pattern_size] /= (double)opts->num_runs;

				if (total_occur > 0)
				{
					double std = compute_std(SEARCH_TIME[algo][pattern_size], TIME, opts->num_runs);

					printf("\b\b\b\b\b\b\b.[OK]  ");
					if (opts->pre)
						sprintf(output_line, "\t%.2f + [%.2f ± %.2f] ms", PRE_TIME[algo][pattern_size], SEARCH_TIME[algo][pattern_size], std);
					else
						sprintf(output_line, "\t[%.2f ± %.2f] ms", SEARCH_TIME[algo][pattern_size], std);

					printf("%s", output_line);
					for (int i = 0; i < 25 - strlen(output_line); i++)
						printf(" ");

					if (opts->occ)
					{
						if (opts->pre)
							printf("\t\tocc \%d", total_occur / opts->num_runs);
						else
							printf("\tocc \%d", total_occur / opts->num_runs);
					}
					printf("\n");
				}
				else if (total_occur == 0)
					printf("\b\b\b\b\b\b\b\b.[ERROR] \n");
				else if (total_occur == -1)
					printf("\b\b\b\b\b.[--]  \n");
				else if (total_occur == -2)
					printf("\b\b\b\b\b\b.[OUT]  \n");
			}

			printf("\n");
			print_top_edge(TOP_EDGE_WIDTH);

			// TODO: extract method: output_running_times();

			/*
			printf("\tOUTPUT RUNNING TIMES %s\n", code);
			if (opts->txt)
				outputTXT(TIME, opts->alphabet_size, filename, code, time_format);

			outputXML(TIME, opts->alphabet_size, filename, code);

			outputHTML2(PRE_TIME, TIME, BEST, WORST, STD, opts->pre, opts->dif, opts->alphabet_size, n, opts->num_runs, filename, code, time_format);

			if (opts->tex)
				outputLatex(TIME, opts->alphabet_size, filename, code, time_format);

			if (opts->php)
				outputPHP(TIME, BEST, WORST, STD, opts->alphabet_size, filename, code, opts->dif, opts->std);
				*/
		}
	}
	// free shared memory
	free_shm(T, P, count, e_time, pre_time, tshmid, pshmid, rshmid, eshmid, preshmid);

	// free memory allocated for patterns
	for (int i = 0; i < opts->num_runs; i++)
		free(pattern_list[i]);
	free(pattern_list);

	return 0;
}

void gen_experiment_code(char *code)
{
	sprintf(code, "EXP%d", (int)time(NULL));
}

void compute_alphabet_info(int *freq, int *alphabet_size, int *max_code)
{
	*alphabet_size = 0;
	*max_code = 0;
	for (int c = 0; c < SIGMA; c++)
	{
		if (freq[c] != 0)
		{
			(*alphabet_size)++;

			if (c > *max_code)
				*max_code = c;
		}
	}
}

void compute_frequency(const char *T, int n, int *freq)
{
	int nalpha = 0;
	int maxcode = 0;
	for (int j = 0; j < SIGMA; j++)
		freq[j] = 0;

	for (int j = 0; j < n; j++)
		freq[T[j]]++;
}

void print_text_info(const char *T, int n)
{
	printf("\tText buffer of dimension %d byte\n", n);

	int freq[SIGMA];
	compute_frequency(T, n, freq);

	int alphabet_size, max_code;
	compute_alphabet_info(freq, &alphabet_size, &max_code);

	printf("\tAlphabet of %d characters.\n", alphabet_size);
	printf("\tGreater chararacter has code %d.\n", max_code);
}

void run_benchmarks(key_t tkey, int tshmid, run_command_opts_t *opts, char *T)
{
	char filename_list[NumSetting][50];
	int num_buffers = split_filename(opts->filename, filename_list);

	srand(time(NULL));

	char expcode[100];
	gen_experiment_code(expcode);

	printf("\tStarting experimental tests with code %s\n", expcode);

	for (int k = 0; k < num_buffers; k++)
	{
		char fullpath[100];
		strcpy(fullpath, "data/");
		strcat(fullpath, filename_list[k]);

		printf("\n\tTry to process archive %s\n", fullpath);

		int n = gen_search_text(fullpath, T, opts->text_size);
		if (n == 0)
		{
			printf("\tunable to generate search text\n");
			return;
		}
		print_text_info(T, n);

		time_t date_timer;
		char time_format[26];
		struct tm *tm_info;
		time(&date_timer);
		tm_info = localtime(&date_timer);
		strftime(time_format, 26, "%Y:%m:%d %H:%M:%S", tm_info);

		run_setting(filename_list[k], tkey, T, n, opts, expcode, tshmid, time_format);
	}

	// outputINDEX(filename_list, num_buffers, expcode);
}

int exec_run(run_command_opts_t *opts)
{
	PATT_SIZE = PATT_LARGE_SIZE; // the set of pattern legths

	srand(time(NULL));

	print_logo();

	key_t tkey;
	unsigned char *T; // text and pattern
	int tshmid = alloc_shared_mem(opts->text_size + PATTERN_SIZE_MAX, (void **)&T, &tkey);

	if (!strcmp(opts->filename, "all"))
	{
		// TODO: replace filename_list with the list of all the buffers
	}

	// TODO: add context_t type to store all shared_memory variables

	run_benchmarks(tkey, tshmid, opts, T);

	shmctl(tshmid, IPC_RMID, 0);

	return 0;
}

int main(int argc, const char *argv[])
{
	smart_subcommand_t subcommand;
	parse_args(argc, argv, &subcommand);

	if (!strcmp(subcommand.subcommand, "select"))
	{
		exec_select((select_command_opts_t *)subcommand.opts);
		exit(0);
	}

	if (!strcmp(subcommand.subcommand, "run"))
	{
		exec_run((run_command_opts_t *)subcommand.opts);
		exit(0);
	}
}