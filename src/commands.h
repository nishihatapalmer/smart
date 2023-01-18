//
// Created by matt on 01/12/22.
//

#ifndef SMART_COMMANDS_H
#define SMART_COMMANDS_H

/****************************************************
 * Commands
 */

/*
 * Main commands.
 */

static char *const RUN_COMMAND = "run";
static char *const SELECT_COMMAND = "select";
static char *const TEST_COMMAND = "test";
static char *const CONFIG_COMMAND = "config";

/*
 * Help options.
 */
static char *const OPTION_SHORT_HELP = "-h";
static char *const OPTION_LONG_HELP = "--help";

/*
 * Options for which smart subcommand to process.
 */
typedef struct smart_opts
{
    const char *subcommand;
    const smart_config_t *smart_config;
    void *opts;
} smart_subcommand_t;

/*
 * Prints help for subcommands.
 */
void print_subcommand_usage_and_exit(const char *command)
{
    print_logo();

    printf("\n usage: %s [run | test | select | config]\n\n", command);

    printf("\t- run: executes benchmarks on one or more algorithms\n");
    printf("\t- test: test the correctness of one or more algorithms\n");
    printf("\t- select: select one or more algorithms to run or test and manage lists of saved algorithms\n");
    printf("\t- config: prints the run-time configuration of smart.\n");

    printf("\n\tRun smart followed by the command to get additional help on run, test and select.\n\n");

    exit(0);
}


/************************************
 * Shared definitions between test and run.
 */

/*
 * Which set of algorithms are to be tested or benchmarked.
 */
enum algo_sources {ALGO_REGEXES, ALL_ALGOS, SELECTED_ALGOS, NAMED_SET_ALGOS};

/*
 * A struct to hold information about what pattern lengths to benchmark or test with.
 */
typedef struct pattern_len_info {
    int pattern_min_len;
    int pattern_max_len;
    char increment_operator;
    int increment_by;
} pattern_len_info_t;

/*
 * Shared commands
 */
static char *const OPTION_SHORT_SEED = "-rs";
static char *const OPTION_LONG_SEED = "--rand-seed";
static char *const OPTION_SHORT_USE_NAMED = "-use";
static char *const OPTION_LONG_USE_NAMED = "--use-algos";
static char *const FLAG_SHORT_ALL_ALGOS = "-all";
static char *const FLAG_LONG_ALL_ALGOS = "--all-algos";
static char *const OPTION_SHORT_PATTERN_LEN = "-plen";
static char *const OPTION_LONG_PATTERN_LEN = "--patt-len";
static char *const OPTION_SHORT_INCREMENT= "-inc";
static char *const OPTION_LONG_INCREMENT = "--increment";

/*
 * Returns the next pattern length given the pattern length increment options and the current length.
 * Guarantees that the next pattern length will be at least one greater than the current length.
 * Does not enforce any maximum size on what the next pattern length can be.
 */
int next_pattern_length(const pattern_len_info_t *pattern_info, int current_length)
{
    int next_length = current_length;
    if (pattern_info->increment_operator == INCREMENT_MULTIPLY_OPERATOR)
    {
        next_length *= pattern_info->increment_by;
    }
    else if (pattern_info->increment_operator == INCREMENT_ADD_OPERATOR)
    {
        next_length += pattern_info->increment_by;
    }
    else
    {
        error_and_exit("Unknown pattern length increment operator was set: %c", pattern_info->increment_operator);
    }

    if (next_length <= current_length)
    {
        next_length = current_length + 1;
    }

    return next_length;
}

/*
 * Returns the number of different pattern lengths to be tested or benchmarked, given the pattern length info and a maximum size.
 */
int get_num_pattern_lengths(const pattern_len_info_t *pattern_info, int text_size)
{
    int num_patterns = 0;
    int value = pattern_info->pattern_min_len;
    int max_size = MIN(text_size, pattern_info->pattern_max_len); // max len guaranteed to be >= min len by parser stage.
    while (value <= max_size)
    {
        value = next_pattern_length(pattern_info, value);
        num_patterns++;
    }

    return num_patterns;
}

/*
 * Returns the maximum pattern length to be tested or benchmarked, given the pattern length info and the text size.
 */
int get_max_pattern_length(const pattern_len_info_t *pattern_info, int text_size)
{
    int current_length = pattern_info->pattern_min_len;      // guaranteed to be within the text size by the parser stage.
    int max_size = MIN(text_size, pattern_info->pattern_max_len); // max len guaranteed to be >= min len by parser stage.
    while (current_length <= max_size)
    {
        int next_length = next_pattern_length(pattern_info, current_length);
        if (next_length > max_size)
        {
            break;
        }
        current_length = next_length;
    }

    return current_length;
}

/************************************
 * Run command
 */

/*
 * Run command options.
 */
static char *const OPTION_SHORT_NUM_RUNS = "-runs";
static char *const OPTION_LONG_NUM_RUNS = "--num-runs";
static char *const OPTION_SHORT_TEXT_SIZE = "-ts";
static char *const OPTION_LONG_TEXT_SIZE = "--text-size";
static char *const OPTION_SHORT_MAX_TIME = "-tb";
static char *const OPTION_LONG_MAX_TIME = "--time-bound";
static char *const OPTION_SHORT_TEXT_SOURCE = "-text";
static char *const OPTION_LONG_TEXT_SOURCE = "--text-files";
static char *const OPTION_SHORT_RANDOM_TEXT = "-rand";
static char *const OPTION_LONG_RANDOM_TEXT = "--rand-text";
static char *const OPTION_SHORT_PATTERN = "-pat";
static char *const OPTION_LONG_PATTERN = "--pattern";
static char *const OPTION_SHORT_SEARCH_DATA = "-data";
static char *const OPTION_LONG_SEARCH_DATA = "--data-to-search";
static char *const OPTION_SHORT_CPU_PIN = "-pin";
static char *const OPTION_LONG_CPU_PIN = "--pin-cpu";
static char *const PARAM_CPU_PINNING_OFF = "off";
static char *const PARAM_CPU_PIN_LAST = "last";
static char *const OPTION_SHORT_GET_CPU_STATS = "-cstats";
static char *const OPTION_LONG_GET_CPU_STATS = "--cpu-stats";
static char *const PARAM_CPU_STATS_FIRST_LEVEL_CACHE = "first";
static char *const PARAM_CPU_STATS_LAST_LEVEL_CACHE = "last";
static char *const PARAM_CPU_STATS_BRANCHING = "branch";

/*
 * Run command flags.
 */
static char *const FLAG_SHORT_OCCURRENCE = "-occ";
static char *const FLAG_LONG_OCCURRENCE = "--occurrences";
static char *const FLAG_SHORT_PREPROCESSING_TIME = "-pre";
static char *const FLAG_LONG_PREPROCESSING_TIME = "--pre-time";
static char *const FLAG_SHORT_FILL_BUFFER = "-fb";
static char *const FLAG_LONG_FILL_BUFFER = "--fill-buffer";
static char *const FLAG_SHORT_PATTERN_LENGTHS_SHORT = "-short";
static char *const FLAG_LONG_PATTERN_LENGTHS_SHORT = "--short-patterns";
static char *const FLAG_SHORT_PATTERN_LENGTHS_VERY_SHORT = "-vshort";
static char *const FLAG_LONG_PATTERN_LENGTHS_VERY_SHORT = "--very-short";


static char *const FLAG_TEXT_OUTPUT = "-txt";     //TODO: output of results.
static char *const FLAG_LATEX_OUTPUT = "-tex";    //TODO: output of results.
static char *const FLAG_PHP_OUTPUT = "-php";      //TODO: output of results.

/*
 * Type of data source to use for benchmarking.
 */
enum data_source_type{DATA_SOURCE_NOT_DEFINED, DATA_SOURCE_FILES, DATA_SOURCE_RANDOM, DATA_SOURCE_USER};

/*
 * The type of cpu pinning to use.
 */
enum cpu_pin_type{PINNING_OFF, PIN_LAST_CPU, PIN_SPECIFIED_CPU};

/*
 * Options for the run subcommand.
 */
typedef struct run_command_opts
{
    enum algo_sources algo_source;               // source of algorithms to benchmark.
    char algo_filename[STR_BUF];                 // The filename in the config directory containing the algorithms to benchmark.
    const char *algo_names[MAX_SELECT_ALGOS];    // algo_names to benchmark, as POSIX regular expressions on the command line.
    int num_algo_names;                          // Number of algo names recorded.
    enum data_source_type data_source;           // What type of data is to be scanned - files or random.
    const char *data_sources[MAX_DATA_SOURCES];  // A list of files/data_sources to load data from.
    int text_size;                               // Size of the text buffer for benchmarking.
    int fill_buffer;                             // Boolean flag - whether to replicate data to fill up a buffer.
    int alphabet_size;                           // Size of the alphabet to use when creating random texts.
    pattern_len_info_t pattern_info;             // Information about what pattern lengths to benchmark.
    int num_runs;                                // Number of patterns of a given length to benchmark.
    int time_limit_millis;                       // Number of milliseconds before an algorithm has timed out.
    long random_seed;                            // Random seed used to generate text or patterns.
    const char *pattern;                         // pattern to use for simple benchmarking.
    const char *data_to_search;                  // text to use for simple benchmarking.  If not specified normal data sources are used.
    enum cpu_pin_type cpu_pinning;               // What kind of cpu pinning to use.
    int cpu_to_pin;                              // The number of the cpu to pin, if specified by the user, -1 otherwise.
    int cpu_stats;                               // A bitmask of cpu stats to acquire.  Zero means no stats to gather, 001 = L1 cache, 010 = last cache, 100 = branches.
    int occ;                                     // Boolean flag - whether to report total occurrences.
    int pre;                                     // Boolean flag - whether to report pre-processing time separately.
    char expcode[STR_BUF];                       // A code generated to identify this benchmarking run.
} run_command_opts_t;

/*
 * Generates a code to identify each local benchmark experiment that is run.
 * The code is not guaranteed to be globally unique - it is simply based on the local time a benchmark was run.
 */
void gen_experiment_code(char *code, int max_len)
{
    snprintf(code, max_len, "EXP%d", (int)time(NULL));
}

/*
 * Initialises the run command options with default values.
 */
void init_run_command_opts(run_command_opts_t *opts)
{
    opts->algo_source = SELECTED_ALGOS; // by default, we just use selected algos, unless otherwise specified.
    strncpy(opts->algo_filename, SELECTED_ALGOS_FILENAME, STR_BUF);
    opts->num_algo_names = 0;
    for (int i = 0; i < MAX_SELECT_ALGOS; i++)
    {
        opts->algo_names[i] = NULL;
    }
    opts->data_source = DATA_SOURCE_NOT_DEFINED;
    for (int i = 0; i < MAX_DATA_SOURCES; i++)
    {
        opts->data_sources[i] = NULL;
    }
    opts->cpu_pinning = CPU_PIN_DEFAULT;
    opts->cpu_to_pin  = -1;
    opts->cpu_stats   = 0; // default to gathering no cpu stats.
    opts->alphabet_size = SIGMA;
    opts->text_size = TEXT_SIZE_DEFAULT;
    opts->pattern_info.pattern_min_len = PATTERN_MIN_LEN_DEFAULT;
    opts->pattern_info.pattern_max_len = PATTERN_MAX_LEN_DEFAULT;
    opts->pattern_info.increment_operator = INCREMENT_MULTIPLY_OPERATOR;
    opts->pattern_info.increment_by = INCREMENT_BY;
    opts->num_runs = NUM_RUNS_DEFAULT;
    opts->time_limit_millis = TIME_LIMIT_MILLIS_DEFAULT;
    opts->random_seed = time(NULL);
    opts->pattern = NULL;
    opts->data_to_search = NULL;
    opts->fill_buffer = FALSE;
    opts->cpu_stats = FALSE;
    opts->pre = FALSE;
    opts->occ = 0;
    gen_experiment_code(opts->expcode, STR_BUF);
}

/*
 * Prints help on params and options for the run subcommand.
 */
void print_run_usage_and_exit(const char *command)
{
    print_logo();

    printf("\n usage: %s [algo names...] [-text | -rand | -data | -plen | -inc | -short | -vshort | -pat | -use | -all | -runs | -ts | -fb | -rs | -pre | -occ | -tb | -pin | -h]\n\n", command);

    printf("\tYou can specify algorithms to benchmark directly as POSIX regular expressions, e.g. smart run bsdm.* hor ...\n");
    printf("\tIf you do not specify any algorithms on the command line or by another command, the default selected algorithms will be used.\n\n");

    print_help_line("Performs experimental results loading all files F specified into a single buffer for benchmarking.", OPTION_SHORT_TEXT_SOURCE, OPTION_LONG_TEXT_SOURCE, "F ...");
    print_help_line("You can specify several individual files, or directories.  If a directory, all files in it will be loaded,", "", "", "");
    print_help_line("up to the maximum buffer size.  SMART will look for files locally, and then in its search", "", "", "");
    print_help_line("path, which defaults to the /data directory in the smart distribution.", "", "", "");
    print_help_line("Performs experimental results using random text with an alphabet A between 1 and 256 inclusive.", OPTION_SHORT_RANDOM_TEXT, OPTION_LONG_RANDOM_TEXT, "A");
    print_help_line("Performs experimental results using text specified in parameter T.", OPTION_SHORT_SEARCH_DATA, OPTION_LONG_SEARCH_DATA, "T");
    print_help_line("Set the minimum and maximum length of random patterns to benchmark between L and U (included).", OPTION_SHORT_PATTERN_LEN, OPTION_LONG_PATTERN_LEN, "L U");
    print_help_line("If you only provide a single parameter L, then only that pattern length will be used.", "", "", "L");
    print_help_line("Increments the pattern lengths with operator O and value V, e.g. '+1'. Default is '*2'.", OPTION_SHORT_INCREMENT, OPTION_LONG_INCREMENT, "O V");
    print_help_line("To add by a fixed amount V, use operator +", "", "", "+ V");
    print_help_line("To multiply by a fixed amount V, use operator *", "", "", "* V");
    print_help_line("Performs experimental results using short length patterns (from 2 to 32 incrementing by 2)", FLAG_SHORT_PATTERN_LENGTHS_SHORT, FLAG_LONG_PATTERN_LENGTHS_SHORT, "");
    print_help_line("Performs experimental results using very short length patterns (from 1 to 16 incrementing by 1)", FLAG_SHORT_PATTERN_LENGTHS_VERY_SHORT, FLAG_LONG_PATTERN_LENGTHS_VERY_SHORT, "");
    print_help_line("Performs experimental results using a single pattern specified in parameter P.", OPTION_SHORT_PATTERN, OPTION_LONG_PATTERN, "P");
    print_help_line("Benchmarks a set of algorithms named N.algos in the config folder, in addition to any algorithms specified directly.", OPTION_SHORT_USE_NAMED, OPTION_LONG_USE_NAMED, "N");
    print_help_line("Benchmarks all the algorithms.", FLAG_SHORT_ALL_ALGOS, FLAG_LONG_ALL_ALGOS, "");
    print_help_line("Computes running times as the mean of N runs (default 500)", OPTION_SHORT_NUM_RUNS, OPTION_LONG_NUM_RUNS, "N");
    print_help_line("Set the upper bound dimension S (in Mb) of the text used for experimental results (default 1Mb).", OPTION_SHORT_TEXT_SIZE, OPTION_LONG_TEXT_SIZE, "S");
    print_help_line("Fills the text buffer up to its maximum size by copying earlier data until full.", FLAG_SHORT_FILL_BUFFER, FLAG_LONG_FILL_BUFFER, "");
    print_help_line("Sets the random seed to integer S, ensuring tests and benchmarks can be precisely repeated.", OPTION_SHORT_SEED, OPTION_LONG_SEED, "S");
    print_help_line("Reports preprocessing times and searching times separately", FLAG_SHORT_PREPROCESSING_TIME, FLAG_LONG_PREPROCESSING_TIME, "");
    print_help_line("Prints the total number of occurrences", FLAG_SHORT_OCCURRENCE, FLAG_LONG_OCCURRENCE, "");
    print_help_line("Set to L the upper bound for any worst case running time (in ms). The default value is 300 ms.", OPTION_SHORT_MAX_TIME, OPTION_LONG_MAX_TIME, "L");
    print_help_line("Pin the benchmark process to a single CPU for lower benchmarking variance via optional parameter [C]: [off | last | {digits}]", OPTION_SHORT_CPU_PIN, OPTION_LONG_CPU_PIN, "[C]");
    print_help_line("If set to 'off', no CPU pinning will be performed.", "", """", "off");
    print_help_line("If set to 'last' (the default), the benchmark will be pinned to the last available CPU.", "", """", "last");
    print_help_line("If set to a number N, the benchmark will be pinned to CPU number N, if available.", "", """", "N");
    print_help_line("Gather CPU statistics for one or more properties [S]: [first | last | branch]", OPTION_SHORT_GET_CPU_STATS, OPTION_LONG_GET_CPU_STATS, "[S]");
    print_help_line("If set to 'first' then cache accesses and misses for the L1 cache will be obtained.", "", "", "first");
    print_help_line("If set to 'last' then cache accesses and misses for the last level cache will be obtained.", "", "", "last");
    print_help_line("If set to 'branch' then branch instructions and prediction misses will be obtained.", "", "", "branch");
    print_help_line("If no parameters are provided, defaults to obtaining L1 cache and branch instructions.", "", "", "");
    print_help_line("Note that the number of CPU stats it is possible to obtain simultaneously varies by CPU.", "", "", "");

    //print_help_line("Output results in txt tabular format", FLAG_TEXT_OUTPUT, "", "");
    //print_help_line("Output results in latex tabular format", FLAG_LATEX_OUTPUT, "", "");
    print_help_line("Gives this help list.", OPTION_SHORT_HELP, OPTION_LONG_HELP, "");

    printf("\n\n");

    exit(0);
}


/************************************
 * Select command
 */

/*
 * Select command options.
 */
static char *const OPTION_SHORT_SHOW_ALL = "-sa";
static char *const OPTION_LONG_SHOW_ALL = "--show-all";
static char *const OPTION_SHORT_SHOW_SELECTED = "-ss";
static char *const OPTION_LONG_SHOW_SELECTED = "--show-selected";
static char *const OPTION_SHORT_SHOW_NAMED = "-sn";
static char *const OPTION_LONG_SHOW_NAMED = "--show-named";

static char *const OPTION_SHORT_ADD = "-a";
static char *const OPTION_LONG_ADD = "--add";
static char *const OPTION_SHORT_REMOVE = "-r";
static char *const OPTION_LONG_REMOVE = "--remove";
static char *const OPTION_SHORT_NO_ALGOS = "-n";
static char *const OPTION_LONG_NO_ALGOS = "--none";

static char *const OPTION_SHORT_SAVE_AS = "-save";
static char *const OPTION_LONG_SAVE_AS = "--save-as";
static char *const OPTION_SHORT_LIST_NAMED = "-ln";
static char *const OPTION_LONG_LIST_NAMED = "--list-named";
static char *const OPTION_SHORT_SET_DEFAULT = "-set";
static char *const OPTION_LONG_SET_DEFAULT = "--set-default";

/*
 * Types of select command.
 */
enum select_command_type {NO_SELECT_COMMAND, ADD, REMOVE, DESELECT_ALL, SAVE_AS, SET_AS_DEFAULT, LIST_NAMED, SHOW_ALL, SHOW_SELECTED, SHOW_NAMED};

/*
 * Options for the select subcommand.
 */
typedef struct select_command_opts
{
    enum select_command_type select_command;
    const char *algos[MAX_SELECT_ALGOS];
    int n_algos;
    const char *named_set;
} select_command_opts_t;

/*
 * Initialises the select command options with default values.
 */
void init_select_command_opts(select_command_opts_t *opts)
{
    opts->select_command = NO_SELECT_COMMAND;
    opts->n_algos = 0;
    opts->named_set = NULL;
}

/*
 * Prints help on params and options for the select subcommand.
 */
void print_select_usage_and_exit(const char *command)
{
    print_logo();

    printf("\n usage: %s select [algo1, algo2, ...] [ -a | -r | -n | -sa | -ss | -sn | -ln | -save | -set | -h ]\n\n", command);

    print_help_line("Add the list of specified algorithms to the set.", OPTION_SHORT_ADD, OPTION_LONG_ADD, "algo...");
    print_help_line("Algorithm names are specified as POSIX extended regular expressions.", "", "", "");
    print_help_line("Remove the list of specified algorithms to the set.", OPTION_SHORT_REMOVE, OPTION_LONG_REMOVE, "algo...");
    print_help_line("Algorithm names are specified as POSIX extended regular expressions.", "", "", "");
    print_help_line("Clears all selected algorithms.", OPTION_SHORT_NO_ALGOS, OPTION_LONG_NO_ALGOS, "");
    print_help_line("Shows all algorithms available on the algo search paths.", OPTION_SHORT_SHOW_ALL, OPTION_LONG_SHOW_ALL, "");
    print_help_line("Shows the default selected algorithms.", OPTION_SHORT_SHOW_SELECTED, OPTION_LONG_SHOW_SELECTED, "");
    print_help_line("Shows the algorithms in the saved algorithm list N.", OPTION_SHORT_SHOW_NAMED, OPTION_LONG_SHOW_NAMED, "N");
    print_help_line("Lists previously saved selected algorithm sets in the config folder.", OPTION_SHORT_LIST_NAMED, OPTION_LONG_LIST_NAMED, "");
    print_help_line("Saves the default algorithms as a named list of algorithms in file N.algos", OPTION_SHORT_SAVE_AS, OPTION_LONG_SAVE_AS, "N");
    print_help_line("Sets the named list of algos as the default, overwriting the current selection.", OPTION_SHORT_SET_DEFAULT, OPTION_LONG_SET_DEFAULT, "N");
    print_help_line("Gives this help list.", OPTION_SHORT_HELP, OPTION_LONG_HELP, "");

    printf("\n\n");

    exit(0);
}


/************************************
 * Test command
 */

/*
 * Test command options
 */
static char *const OPTION_SHORT_TEST_SELECTED = "-sel";
static char *const OPTION_LONG_TEST_SELECTED = "--selected";
static char *const OPTION_SHORT_DEBUG = "-d";
static char *const OPTION_LONG_DEBUG = "--debug";
static char *const OPTION_SHORT_QUICK_TESTS = "-q";
static char *const OPTION_LONG_QUICK_TESTS = "--quick";
static char *const OPTION_SHORT_FAIL_ONLY = "-fo";
static char *const OPTION_LONG_FAIL_ONLY = "--fail-only";

/*
 * Options for the test subcommand.
 */
typedef struct test_command_opts
{
    enum algo_sources algo_source;                 // source of algorithms to test.
    const char *named_set;                         // name of the named set to load algorithms from, if specified.
    const char *algo_names[MAX_SELECT_ALGOS];      // algo_names to test, as POSIX regular expressions.
    int num_algo_names;                            // Number of algo names recorded.
    long random_seed;                              // Random seed used to generate text or patterns.
    pattern_len_info_t pattern_info;               // Info about what pattern sizes to test random patterns with.
    int debug;                                     // If set will re-call a failing search function with the failing parameters.
    int quick;                                     // Whether to run quick tests.
    int fail_only;                                 // Whether to only report failures in the test output.
} test_command_opts_t;

/*
 * Initialises the test command options with default values.
 */
void init_test_command_opts(test_command_opts_t *opts)
{
    opts->algo_source  = ALGO_REGEXES;       // default is just user specified algo_names unless a command says different.
    opts->named_set    = NULL;
    for (int i = 0; i < MAX_SELECT_ALGOS; i++)
        opts->algo_names[i] = NULL;
    opts->num_algo_names = 0;
    opts->random_seed  = time(NULL);   // default is random seed set by the current time, unless -seed option is specified.
    opts->debug = FALSE;
    opts->quick = FALSE;
    opts->fail_only = FALSE;
    opts->pattern_info.pattern_min_len = 0;            // Only set to a real operator if we are specifying pattern lengths for test.
    opts->pattern_info.pattern_max_len = 0;            // Only set to a real operator if we are specifying pattern lengths for test.
    opts->pattern_info.increment_operator = INCREMENT_MULTIPLY_OPERATOR;
    opts->pattern_info.increment_by = INCREMENT_BY;
}

/*
 * Prints help on params and options for the test subcommand.
 */
void print_test_usage_and_exit(const char *command)
{
    print_logo();

    printf("\n usage: %s test [algo1, algo2, ...] | -all | -sel | -use | -plen |-inc | -rs | -q | -d | -h\n\n", command);

    info("Tests a set of smart algorithms for correctness with a variety of fixed and randomized tests.");
    info("You can specify the algorithms to test directly using POSIX extended regular expressions, e.g. test hor wfr.*");
    info("You can also specify that all algorithms, the currently selected set, or another saved set of algorithms are tested.\n");

    print_help_line("Tests all of the algorithms smart finds in its algo search paths.", FLAG_SHORT_ALL_ALGOS, FLAG_LONG_ALL_ALGOS, "");
    print_help_line("Tests the currently selected algorithms in addition to any algorithms specified directly.", OPTION_SHORT_TEST_SELECTED, OPTION_LONG_TEST_SELECTED, "");
    print_help_line("Tests a set of algorithms named N.algos in the config folder, in addition to any algorithms specified directly.", OPTION_SHORT_USE_NAMED, OPTION_LONG_USE_NAMED, "N");
    print_help_line("Set the minimum and maximum length of random patterns to test between L and U (included).", OPTION_SHORT_PATTERN_LEN, OPTION_LONG_PATTERN_LEN, "L U");
    print_help_line("If you only provide a single parameter L, then only that pattern length will be used.", "", "", "L");
    print_help_line("Increments the pattern lengths with operator O and value V, e.g. +1 or *2", OPTION_SHORT_INCREMENT, OPTION_LONG_INCREMENT, "O V");
    print_help_line("To add by a fixed amount V, use operator +", "", "", "+ V");
    print_help_line("To multiply by a fixed amount V, use operator *", "", "", "* V");
    print_help_line("Sets the random seed to integer S, ensuring tests can be precisely repeated.", OPTION_SHORT_SEED, OPTION_LONG_SEED, "S");
    print_help_line("Runs tests faster by testing less exhaustively.", OPTION_SHORT_QUICK_TESTS, OPTION_LONG_QUICK_TESTS, "");
    print_help_line("Report only failures in the test output.", OPTION_SHORT_FAIL_ONLY, OPTION_LONG_FAIL_ONLY, "");
    print_help_line("Useful to get fast feedback, but all tests should pass before benchmarking against other algorithms.", "", "", "");
    print_help_line("Re-runs a failing search - put a breakpoint on debug_search() in test.h", OPTION_SHORT_DEBUG, OPTION_LONG_DEBUG, "");
    print_help_line("Gives this help list.", OPTION_SHORT_HELP, OPTION_LONG_HELP, "");

    printf("\n\n");

    exit(0);
}

#endif //SMART_COMMANDS_H
