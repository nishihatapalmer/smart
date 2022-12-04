
#ifndef SMART_ALGORITHMS_H
#define SMART_ALGORITHMS_H

#define SEARCH_FUNC_NAME "internal_search"

#include <dlfcn.h>

#include "string_set.h"
#include "config.h"
#include "utils.h"

static const char *ALGO_COLUMN_FORMAT = "%-18s ";
static const int ALGO_NUM_COLUMNS = 6; //TODO: calculate from algo name len?  Max algo name lens?

/*
 * Function signature typedef of the internal search function called to benchmark algorithms.
 */
typedef int search_function(unsigned char *, int, unsigned char *, int, double *, double *);

/*
 * Struct containing information about the algorithms to open and their shared object handles and function pointers.
 * Initialise the struct with the number of algo_names and the names of the algo_names in algo_names.
 * A call to load_algo_shared_libraries passing in this struct will dynamically load the shared objects, populating the handles and
 * function pointers to them.  A call to unload_algos will unload them, and set the handles to -1 and the function pointers to NULL.
 */
typedef struct algo_info
{
    int num_algos;
    char algo_names[MAX_SELECT_ALGOS][ALGO_NAME_LEN];
    search_function *algo_functions[MAX_SELECT_ALGOS];
    long shared_object_handles[MAX_SELECT_ALGOS];
} algo_info_t;

/*
 * Sorts the names of the algorithms.
 *
 * WARNING: if you sort the names *after* calling load_algo_shared_libraries(), the relationship between the name
 * of the algorithm and the search function pointer to it will be broken.  Only sort names *before* loading algorithms.
 */
void sort_algorithm_names(algo_info_t *to_sort)
{
    qsort(to_sort->algo_names, to_sort->num_algos, sizeof(char) * ALGO_NAME_LEN, str_compare);
}

/*
 * Loads the algorithm names from a text file specified by algo_filename which exists in the smart config folder.
 */
void read_algo_names_from_file(const smart_config_t *smart_config, algo_info_t *algorithms, const char *algo_filename)
{
    char fullpath[MAX_PATH_LENGTH];
    set_full_path_or_exit(fullpath, smart_config->smart_config_dir, algo_filename);
    algorithms->num_algos = read_valid_algo_names_from_filename(algorithms->algo_names, fullpath, MAX_SELECT_ALGOS);
}

/*
 * Writes the algorithms in the set of algorithms to a file in the config dir.
 */
int write_algo_names_to_file(const smart_config_t *smart_config, algo_info_t *algorithms, const char *filename)
{
    char write_file_name[MAX_PATH_LENGTH];
    set_full_path_or_exit(write_file_name, smart_config->smart_config_dir, filename);

    char tmp_file_name[MAX_PATH_LENGTH];
    set_filename_suffix_or_exit(tmp_file_name, write_file_name, ".tmp");

    char lower_case_algo_name[ALGO_NAME_LEN];

    FILE *tmp_fp = fopen(tmp_file_name, "w");
    for (int i = 0; i < algorithms->num_algos; i++)
    {
        set_lower_case_algo_name(lower_case_algo_name, algorithms->algo_names[i]);
        fprintf(tmp_fp, "%s\n", lower_case_algo_name);
    }
    fclose(tmp_fp);

    rename(tmp_file_name, write_file_name);
    remove(tmp_file_name);
}

/*
 * Gets all the algorithm names in all the algo search paths, and trims off the .so suffix.
 */
void get_all_algo_names(const smart_config_t *smart_config, algo_info_t *algorithms)
{
    //TODO: does not specify max number of algo names we can have.
    algorithms->num_algos = add_and_trim_filenames_with_suffix_in_paths(algorithms->algo_names, ".so",
                                                                        smart_config->num_algo_search_paths,
                                                                        smart_config->smart_algo_search_paths);
}

/*
 * Filters out any names in the set of algorithms that do not match the regexes supplied.
 * Stores the names of any which are removed from algorithms in the filtered_out algorithm info,
 * although you can set this to NULL if you don't care which ones were removed.
 * Returns the number of names filtered out.
 */
int filter_out_names_not_matching_regexes(algo_info_t *algorithms, algo_info_t *filtered_out,
                                          const char * const algo_regexes[MAX_SELECT_ALGOS], int num_regexes)
{
    regex_t *algo_name_regexes[num_regexes];
    compile_algo_name_regexes(algo_name_regexes, algo_regexes, num_regexes);

    int num_filtered = 0;
    int remove_idx = -1;
    for (int idx_to_check = 0; idx_to_check < algorithms->num_algos; idx_to_check++)
    {
        // Check to see if any of the regexes match the current algorithm name.
        if (regexes_match(algo_name_regexes, num_regexes, algorithms->algo_names[idx_to_check]))
        {
            // If we match this one, but a previous one did not match - copy it back to shrink the list.
            if (remove_idx >= 0)
            {
                strncpy(algorithms->algo_names[remove_idx], algorithms->algo_names[idx_to_check], ALGO_NAME_LEN);
                remove_idx++; // next slot is the one to remove to.
            }
        }
        else
        {
            if (filtered_out != NULL)
            {
                strncpy(filtered_out->algo_names[num_filtered], algorithms->algo_names[idx_to_check], ALGO_NAME_LEN);
            }
            if (remove_idx < 0) // If we haven't already starting removing from the list, record the first position to remove from.
            {
                remove_idx = idx_to_check;
            }
            num_filtered++;
        }
    }

    if (filtered_out != NULL)
    {
        filtered_out->num_algos = num_filtered;
    }

    // If we actually removed any, then the remove_idx is the new size of the algorithms.
    if (num_filtered > 0)
    {
        algorithms->num_algos = remove_idx;
    }

    free_regexes(algo_name_regexes, num_regexes);

    return num_filtered;
}

/*
 * Filters out any names in the set of algorithms that match the regexes supplied.
 * Stores the names of any which are removed from algorithms in the filtered_out algorithm info,
 * although you can set this to NULL if you don't care which ones were removed.
 * Returns the number of names filtered out.
 */
int filter_out_names_matching_regexes(algo_info_t *algorithms, algo_info_t *filtered_out, const char * const algo_regexes[MAX_SELECT_ALGOS], int num_regexes)
{
    regex_t *algo_name_regexes[num_regexes];
    compile_algo_name_regexes(algo_name_regexes, algo_regexes, num_regexes);

    int num_filtered = 0;
    int remove_idx = -1;
    for (int idx_to_check = 0; idx_to_check < algorithms->num_algos; idx_to_check++)
    {
        // Check to see if any of the regexes match the current algorithm name.
        if (regexes_match(algo_name_regexes, num_regexes, algorithms->algo_names[idx_to_check]))
        {
            if (filtered_out != NULL)
            {
                strncpy(filtered_out->algo_names[num_filtered], algorithms->algo_names[idx_to_check], ALGO_NAME_LEN);
            }
            if (remove_idx < 0)
            {
                remove_idx = idx_to_check;
            }
            num_filtered++;
        }
        else if (remove_idx >= 0)
        {
            strncpy(algorithms->algo_names[remove_idx], algorithms->algo_names[idx_to_check], ALGO_NAME_LEN);
            remove_idx++; // next slot is the one to remove to.
        }
    }

    if (filtered_out != NULL)
    {
        filtered_out->num_algos = num_filtered;
    }

    // If we actually removed any, then the remove_idx is the new size of the algorithms.
    if (num_filtered > 0)
    {
        algorithms->num_algos = remove_idx;
    }

    free_regexes(algo_name_regexes, num_regexes);

    return num_filtered;
}

/*
 * Initialises a string set with the names defined in an algorithms struct.
 */
void init_set_with_algo_names(str_set_t *set, algo_info_t *algorithms)
{
    str_set_init(set);
    for (int i = 0; i < algorithms->num_algos; i++)
    {
        str_set_add(set, algorithms->algo_names[i]);
    }
}

/*
 * Merges all the algorithms in merge_to with the ones in merge_from, and places any additional names in merge_to.
 * Any names merged will be placed in merged_in, although it can be NULl if you don't want to know which ones were merged in.
 * Will exit with error if the total number of merged algorithms exceeds the maximum selectable number of algorithms.
 * Returns the number of new items merged in.
 */
int merge_algorithms(algo_info_t *merge_to, const algo_info_t *merge_from, algo_info_t *merged_in)
{
    int next_index = merge_to->num_algos;

    str_set_t merge_to_names;
    init_set_with_algo_names(&merge_to_names, merge_to);

    int num_merged = 0;
    for (int i = 0; i < merge_from->num_algos; i++)
    {
        if (!str_set_contains(&merge_to_names, merge_from->algo_names[i]))
        {
            if (next_index >= MAX_SELECT_ALGOS)
            {
                error_and_exit(
                        "Could not merge algorithm sets as it exceeds the maximum number of selectable algorithms: %s",
                        MAX_SELECT_ALGOS);
            }
            strncpy(merge_to->algo_names[next_index], merge_from->algo_names[i], ALGO_NAME_LEN);
            if (merged_in != NULL)
            {
                strncpy(merged_in->algo_names[num_merged], merge_from->algo_names[i], ALGO_NAME_LEN);
            }
            str_set_add(&merge_to_names, merge_from->algo_names[i]);
            next_index++;
            num_merged++;
        }
    }

    if (merged_in != NULL)
    {
        merged_in->num_algos = num_merged;
    }

    merge_to->num_algos = next_index;

    str_set_free(&merge_to_names);

    return num_merged;
}

/*
 * Dynamically loads the algorithms defined in algo_info as shared objects into the smart process.
 * Returns 0 if successful.  Will exit with status 1 if it is unable to load an algorithm.
 */
int load_algo_shared_libraries(const smart_config_t *smart_config, algo_info_t *algo_info)
{
    char lower_case_algo_name[ALGO_NAME_LEN];
    char algo_lib_filename[MAX_PATH_LENGTH];

    for (int i = 0; i < algo_info->num_algos; i++)
    {
        // Build algo filename as lower case algo name with .so suffix.
        set_lower_case_algo_name(lower_case_algo_name, algo_info->algo_names[i]);
        set_filename_suffix_or_exit(algo_lib_filename, lower_case_algo_name, ".so");

        // Locate the algo filename in the algo search paths:
        char valid_path[MAX_PATH_LENGTH];
        if (locate_file_path(valid_path, algo_lib_filename, smart_config->smart_algo_search_paths,
                         smart_config->num_algo_search_paths))
        {
            void *lib_handle = dlopen(valid_path, RTLD_NOW);
            if (lib_handle == NULL)
            {
                error_and_exit("Unable to open the shared object %s from file %s\n%s",
                               algo_info->algo_names[i], valid_path, dlerror());
            }

            search_function *search = dlsym(lib_handle, SEARCH_FUNC_NAME);
            if (search == NULL)
            {
                error_and_exit("Invalid library: %s does not export a search function in file %s\n%s",
                               algo_info->algo_names[i], valid_path, dlerror());
            }

            algo_info->shared_object_handles[i] = (long) lib_handle;
            algo_info->algo_functions[i] = search;
        }
        else
        {
            warn("Could not locate algorithm %s in the defined algo search paths.", algo_info->algo_names[i]);
            //TODO: and what happens when we try to run this?  Should we delete it from algorithms (modifies list as we work on it...)
            //      just exit?
        }
    }

    return 0;
}

/*
 * Closes all the dynamically loaded algorithm shared object handles.
 * Also sets the algo handles to -1, and the function pointers to NULL.
 */
void unload_algos(algo_info_t *algo_info)
{
    for (int i = 0; i < algo_info->num_algos; i++)
    {
        dlclose((void *) algo_info->shared_object_handles[i]);
        algo_info->shared_object_handles[i] = -1;
        algo_info->algo_functions[i] = NULL;
    }
}

/*
 * Prints the algorithms as a comma-delimited list, with an optional header message.
 */
void print_algorithms_as_list(const char *message, const algo_info_t *algorithms)
{
    if (algorithms->num_algos > 0)
    {
        char upper_case_algo_name[ALGO_NAME_LEN];
        set_upper_case_algo_name(upper_case_algo_name, algorithms->algo_names[0]);

        printf("%s%s", message, upper_case_algo_name);

        for (int i = 1; i < algorithms->num_algos; i++)
        {
            set_upper_case_algo_name(upper_case_algo_name, algorithms->algo_names[i]);
            printf(", %s", upper_case_algo_name);
        }

        printf(".\n");
    }
}

/*
 * Prints the algorithms in a tabular format, with multiple columns.
 */
void print_algorithms_in_tabular_format(const algo_info_t *algorithms)
{
    if (algorithms->num_algos > 0)
    {
        char upper_case_algo_name[ALGO_NAME_LEN];
        for (int i = 0; i < algorithms->num_algos - ALGO_NUM_COLUMNS; i += ALGO_NUM_COLUMNS)
        {
            for (int j = i; j < i + ALGO_NUM_COLUMNS; j++)
            {
                set_upper_case_algo_name(upper_case_algo_name, algorithms->algo_names[j]);
                printf(ALGO_COLUMN_FORMAT, upper_case_algo_name);
            }
            printf("\n");
        }

        int remaining_columns = algorithms->num_algos % ALGO_NUM_COLUMNS;
        if (remaining_columns > 0)
        {
            for (int i = algorithms->num_algos - remaining_columns; i < algorithms->num_algos; i++)
            {
                set_upper_case_algo_name(upper_case_algo_name, algorithms->algo_names[i]);
                printf(ALGO_COLUMN_FORMAT, upper_case_algo_name);
            }
            printf("\n");
        }
    }
}

#endif //SMART_ALGORITHMS_H