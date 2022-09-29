#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>
#include <sys/stat.h>


#ifdef WIN32

#include <windows.h>

#pragma warning (disable: 4996)
#define _CRT_SECURE_NO_DEPRECATE  // for latest SDK compilers

#define sleep(x) Sleep(1000*x)
#define strcasecmp _stricmp
#define snprintf _snprintf

/*	char* strtok_r(char *s, const char *delim, char **ptrptr)
{
return strtok(s, delim);
}
*/

#else // UNIX

#include <unistd.h>

#endif

#if HAVE_STDBOOL_H 
#include <stdbool.h> 
#else 
#define bool unsigned char  
#define false 0 
#define true 1 
#endif


#define DRMAA_TEST_SUITE_VERSION "1.7.2"

#include "drmaa.h"

#define JOB_CHUNK 2
#define NTHREADS 3
#define NBULKS 3

// DRMAA_TIMEOUT_WAIT_FOREVER is anyway useless in a test suite
// we assume some local setting, fast enough for the single job runs
long max_wait_timeout = 3600;

#define NEXT_ARGV(argc, argv) \
	((*argc)--, (*argv)++, (*argv)[0])

enum {
	ALL_TESTS = 0,    

	ST_SUBMIT_WAIT,    
	/* - one thread 
	- submit jobs 
	- wait for jobend */

	MT_SUBMIT_WAIT,        
	/* - multiple submission threads
	- wait is done by main thread */

	MT_SUBMIT_BEFORE_INIT_WAIT,
	/* - no drmaa_init() was called
	- multiple threads try to submit but fail
	- when drmaa_init() is called by main thread
	submission proceed
	- wait is done by main thread */

	ST_MULT_INIT,
	/* - drmaa_init() is called multiple times 
	- first time it must succeed - second time it must fail
	- then drmaa_exit() is called */

	ST_MULT_EXIT,
	/* - drmaa_init() is called
	- then drmaa_exit() is called multiple times
	- first time it must succeed - second time it must fail */

	MT_EXIT_DURING_SUBMIT,
	/* - drmaa_init() is called
	- multiple submission threads submitting (delayed) a series 
	of jobs
	- during submission main thread does drmaa_exit() */

	MT_SUBMIT_MT_WAIT,
	/* - drmaa_init() is called
	- multiple submission threads submit jobs and wait these jobs
	- when all threads are finished main thread calls drmaa_exit() */

	MT_EXIT_DURING_SUBMIT_OR_WAIT,
	/* - drmaa_init() is called
	- multiple submission threads submit jobs and wait these jobs
	- while submission threads are waiting their jobs the main 
	thread calls drmaa_exit() */

	ST_BULK_SUBMIT_WAIT,
	/* - drmaa_init() is called
	- a bulk job is submitted and waited 
	- then drmaa_exit() is called */

	ST_BULK_SINGLESUBMIT_WAIT_INDIVIDUAL,
	/* - drmaa_init() is called
	- bulk and sequential jobs are submitted
	- all jobs are waited individually
	- then drmaa_exit() is called */

	ST_SUBMITMIXTURE_SYNC_ALL_DISPOSE,
	/* - drmaa_init() is called
	- submit a mixture of single and bulk jobs
	- do drmaa_synchronize(DRMAA_JOB_IDS_SESSION_ALL, dispose) 
	to wait for all jobs to finish
	- then drmaa_exit() is called */

	ST_SUBMITMIXTURE_SYNC_ALL_NODISPOSE,
	/* - drmaa_init() is called
	- submit a mixture of single and bulk jobs
	- do drmaa_synchronize(DRMAA_JOB_IDS_SESSION_ALL, no-dispose) 
	to wait for all jobs to finish
	- do drmaa_wait(DRMAA_JOB_IDS_SESSION_ANY) until 
	DRMAA_ERRNO_INVALID_JOB to reap all jobs
	- then drmaa_exit() is called */

	ST_SUBMITMIXTURE_SYNC_ALLIDS_DISPOSE,
	/* - drmaa_init() is called
	- submit a mixture of single and bulk jobs
	- do drmaa_synchronize(all_jobids, dispose) 
	to wait for all jobs to finish
	- then drmaa_exit() is called */

	ST_SUBMITMIXTURE_SYNC_ALLIDS_NODISPOSE,
	/* - drmaa_init() is called
	- submit a mixture of single and bulk jobs
	- do drmaa_synchronize(all_jobids, no-dispose) 
	to wait for all jobs to finish
	- do drmaa_wait(DRMAA_JOB_IDS_SESSION_ANY) until 
	DRMAA_ERRNO_INVALID_JOB to reap all jobs
	- then drmaa_exit() is called */

	ST_INPUT_FILE_FAILURE,
	ST_OUTPUT_FILE_FAILURE,
	ST_ERROR_FILE_FAILURE,
	/* - drmaa_init() is called
	- a job is submitted with input/output/error path specification 
	that must cause the job to fail
	- use drmaa_synchronize() to ensure job was started
	- drmaa_job_ps() must return DRMAA_PS_FAILED
	- drmaa_wait() must report drmaa_wifaborted() -> true
	- then drmaa_exit() is called */

	ST_SUBMIT_IN_HOLD_RELEASE,
	/* - drmaa_init() is called
	- a job is submitted with a user hold 
	- use drmaa_job_ps() to verify user hold state
	- hold state is released using drmaa_control()
	- the job is waited
	- then drmaa_exit() is called
	(still requires manual testing)
	*/

	ST_SUBMIT_IN_HOLD_DELETE,
	/* - drmaa_init() is called
	- a job is submitted with a user hold 
	- use drmaa_job_ps() to verify user hold state
	- job is terminated using drmaa_control()
	- the job is waited and it is checked if wifaborted is true
	- then drmaa_exit() is called
	(still requires manual testing)
	*/

	ST_BULK_SUBMIT_IN_HOLD_SINGLE_RELEASE,
	/* - drmaa_init() is called
	- a bulk job is submitted with a user hold 
	- hold state is released separately for each task using drmaa_control()
	- the job ids are waited
	- then drmaa_exit() is called
	(still requires manual testing)
	*/

	ST_BULK_SUBMIT_IN_HOLD_SESSION_RELEASE,
	/* - drmaa_init() is called
	- a bulk job is submitted with a user hold 
	- hold state is released for the session using drmaa_control()
	- the job ids are waited
	- then drmaa_exit() is called
	(still requires manual testing)
	*/

	ST_BULK_SUBMIT_IN_HOLD_SESSION_DELETE,
	/* - drmaa_init() is called
	- a bulk job is submitted with a user hold 
	- use drmaa_job_ps() to verify user hold state
	- all session jobs are terminated using drmaa_control()
	- the job ids are waited
	- then drmaa_exit() is called
	(still requires manual testing)
	*/

	ST_BULK_SUBMIT_IN_HOLD_SINGLE_DELETE,
	/* - drmaa_init() is called
	- a bulk job is submitted with a user hold 
	- use drmaa_job_ps() to verify user hold state
	- all session jobs are terminated using drmaa_control()
	- the job ids are waited
	- then drmaa_exit() is called
	(still requires manual testing)
	*/

	ST_EXIT_STATUS,
	/* - drmaa_init() is called
	- 255 job are submitted
	- job i returns i as exit status (8 bit)
	- drmaa_wait() verifies each job returned the 
	correct exit status
	- then drmaa_exit() is called */

	ST_SUPPORTED_ATTR,
	/* - drmaa_init() is called
	- drmaa_get_attribute_names() is called
	- the names of all supported non vector attributes are printed
	- then drmaa_exit() is called */

	ST_SUPPORTED_VATTR,
	/* - drmaa_init() is called
	- drmaa_get_vector_attribute_names() is called
	- the names of all supported vector attributes are printed
	- then drmaa_exit() is called */

	ST_VERSION,
	/* - drmaa_version() is called 
	- version information is printed */

	ST_CONTACT,
	/* - drmaa_get_contact() is called
	- the contact string is printed
	- drmaa_init() is called 
	- drmaa_get_contact() is called
	- the contact string is printed
	- then drmaa_exit() is called */

	ST_DRM_SYSTEM,
	/* - drmaa_get_DRM_system() is called
	- the contact string is printed
	- drmaa_init() is called 
	- drmaa_get_DRM_system() is called
	- the DRM system name is printed
	- then drmaa_exit() is called */

	ST_DRMAA_IMPL,
	/* - drmaa_get_DRM_system() is called
	- the contact string is printed
	- drmaa_init() is called 
	- drmaa_get_DRMAA_implementation() is called
	- the DRMAA implemention name is printed
	- then drmaa_exit() is called */

	ST_EMPTY_SESSION_WAIT,
	/* - drmaa_init() is called
	- drmaa_wait() must return DRMAA_ERRNO_INVALID_JOB
	- then drmaa_exit() is called */

	ST_EMPTY_SESSION_SYNCHRONIZE_DISPOSE,
	/* - drmaa_init() is called
	- drmaa_synchronize(DRMAA_JOB_IDS_SESSION_ALL, dispose=true) must return DRMAA_ERRNO_SUCCESS
	- then drmaa_exit() is called */

	ST_EMPTY_SESSION_SYNCHRONIZE_NODISPOSE,
	/* - drmaa_init() is called
	- drmaa_synchronize(DRMAA_JOB_IDS_SESSION_ALL, dispose=false) must return DRMAA_ERRNO_SUCCESS
	- then drmaa_exit() is called */

	ST_EMPTY_SESSION_CONTROL,
	/* - drmaa_init() is called
	- drmaa_control(DRMAA_JOB_IDS_SESSION_ALL, <passed control operation>) must return DRMAA_ERRNO_SUCCESS
	- then drmaa_exit() is called */

	ST_SUBMIT_SUSPEND_RESUME_WAIT,
	/*  - drmaa_init() is called
	- a single job is submitted 
	- drmaa_job_ps() is used to actively wait until job is running
	- drmaa_control() is used to suspend the job
	- drmaa_job_ps() is used to verify job was suspended
	- drmaa_control() is used to resume the job
	- drmaa_job_ps() is used to verify job was resumed
	- drmaa_wait() is used to wait for the jobs regular end
	- then drmaa_exit() is called */

	ST_SUBMIT_POLLING_WAIT_TIMEOUT, 
	/*  - drmaa_init() is called
	- a single job is submitted 
	- repeatedly drmaa_wait() with a timeout is used until job is finished
	- then drmaa_exit() is called */

	ST_SUBMIT_POLLING_WAIT_ZEROTIMEOUT,
	/*  - drmaa_init() is called
	- a single job is submitted 
	- repeatedly do drmaa_wait(DRMAA_TIMEOUT_NO_WAIT) + sleep() until job is finished
	- then drmaa_exit() is called */

	ST_SUBMIT_POLLING_SYNCHRONIZE_TIMEOUT,
	/*  - drmaa_init() is called
	- a single job is submitted 
	- repeatedly drmaa_synchronize() with a timeout is used until job is finished
	- then drmaa_exit() is called */

	ST_SUBMIT_POLLING_SYNCHRONIZE_ZEROTIMEOUT,
	/*  - drmaa_init() is called
	- a single job is submitted 
	- repeatedly do drmaa_synchronize(DRMAA_TIMEOUT_NO_WAIT) + sleep() until job is finished
	- then drmaa_exit() is called */

	ST_ATTRIBUTE_CHANGE,
	/*  - all attributes a written with different values for two times
	- check if the JT is correct afterwards */

	ST_USAGE_CHECK,
	/* - one thread 
	- submit jobs 
	- wait for jobend
	- print job usage */

	// ST_UNSUPPORTED_ATTR,
	/* - drmaa_init() is called
	- drmaa_set_attribute() is called for an invalid attribute
	- then drmaa_exit() is called */

	// ST_UNSUPPORTED_VATTR,
	/* - drmaa_init() is called
	- drmaa_set_vector_attribute() is called for an invalid attribute
	- then drmaa_exit() is called */

	ST_SUBMIT_KILL_SIG,
	/* - drmaa_init() is called
	- one job is submitted
	- job is killed via SIGKILL and SIGINT
	- drmaa_wtermsig() is used to validate if the correct termination signals where reported
	- drmaa_exit_is_called */

	ST_GET_NUM_JOBIDS,
	/* drmaa_init() ist called
	- bulk job is submitted
	- functionality of drmaa_get_num_jobids is tested
	- drmaa_exit is called*/

	ST_BULK_SUBMIT_INCRPH
	/* drmaa_init() ist called
	- bulk job is submitted
	- drmaa_wd_ph and drmaa_incr_ph placeholders are used in output file name
	- existence of files is checked
	- drmaa_exit is called*/
};

struct test_name2number_map {
	char *test_name;       /* name of the test                                    */
	int test_number;       /* number the test is internally mapped to             */
	int nargs;             /* number of test case arguments required              */
	char *opt_arguments;   /* description of test case arguments for usage output */
};

const struct test_name2number_map test_map[] = {

	/* all automated tests - ST_* and MT_* tests */
#ifdef WIN32
	{ "ALL_AUTOMATED",                            ALL_TESTS,                              2, "<sleeper_job> <exit_arg_job> <email_addr>" },
#else
	{ "ALL_AUTOMATED",                            ALL_TESTS,                              3, "<sleeper_job> <exit_arg_job> <kill_arg_job> <email_addr>" },
#endif

	/* one application thread - automated tests only */
	{ "ST_MULT_INIT",                              ST_MULT_INIT,                              0, "" },
	{ "ST_MULT_EXIT",                              ST_MULT_EXIT,                              0, "" },
	{ "ST_SUPPORTED_ATTR",                         ST_SUPPORTED_ATTR,                         0, "" },
	{ "ST_SUPPORTED_VATTR",                        ST_SUPPORTED_VATTR,                        0, "" },
	{ "ST_VERSION",                                ST_VERSION,                                0, "" },
	{ "ST_DRM_SYSTEM",                             ST_DRM_SYSTEM,                             0, "" },
	{ "ST_DRMAA_IMPL",                             ST_DRMAA_IMPL,                             0, "" },
	{ "ST_CONTACT",                                ST_CONTACT,                                0, "" },
	{ "ST_EMPTY_SESSION_WAIT",                     ST_EMPTY_SESSION_WAIT,                     0, "" },
	{ "ST_EMPTY_SESSION_SYNCHRONIZE_DISPOSE",      ST_EMPTY_SESSION_SYNCHRONIZE_DISPOSE,      0, "" },
	{ "ST_EMPTY_SESSION_SYNCHRONIZE_NODISPOSE",    ST_EMPTY_SESSION_SYNCHRONIZE_NODISPOSE,    0, "" },
	{ "ST_EMPTY_SESSION_CONTROL",                  ST_EMPTY_SESSION_CONTROL,                  1, "DRMAA_CONTROL_*" },
	{ "ST_SUBMIT_WAIT",                            ST_SUBMIT_WAIT,                            1, "<sleeper_job>" },
	{ "ST_BULK_SUBMIT_WAIT",                       ST_BULK_SUBMIT_WAIT,                       1, "<sleeper_job>" },
	{ "ST_BULK_SINGLESUBMIT_WAIT_INDIVIDUAL",      ST_BULK_SINGLESUBMIT_WAIT_INDIVIDUAL,      1, "<sleeper_job>" },
	{ "ST_SUBMITMIXTURE_SYNC_ALL_DISPOSE",         ST_SUBMITMIXTURE_SYNC_ALL_DISPOSE,         1, "<sleeper_job>" },
	{ "ST_SUBMITMIXTURE_SYNC_ALL_NODISPOSE",       ST_SUBMITMIXTURE_SYNC_ALL_NODISPOSE,       1, "<sleeper_job>" },
	{ "ST_SUBMITMIXTURE_SYNC_ALLIDS_DISPOSE",      ST_SUBMITMIXTURE_SYNC_ALLIDS_DISPOSE,      1, "<sleeper_job>" },
	{ "ST_SUBMITMIXTURE_SYNC_ALLIDS_NODISPOSE",    ST_SUBMITMIXTURE_SYNC_ALLIDS_NODISPOSE,    1, "<sleeper_job>" },
	{ "ST_EXIT_STATUS",                            ST_EXIT_STATUS,                            1, "<exit_arg_job>" },
	{ "ST_SUBMIT_KILL_SIG",                        ST_SUBMIT_KILL_SIG,                         1, "<kill_arg_job>" },
	{ "ST_INPUT_FILE_FAILURE",                     ST_INPUT_FILE_FAILURE,                     1, "<sleeper_job>" },
	{ "ST_OUTPUT_FILE_FAILURE",                    ST_OUTPUT_FILE_FAILURE,                    1, "<sleeper_job>" },
	{ "ST_ERROR_FILE_FAILURE",                     ST_ERROR_FILE_FAILURE,                     1, "<sleeper_job>" },
	{ "ST_SUBMIT_IN_HOLD_RELEASE",                 ST_SUBMIT_IN_HOLD_RELEASE,                 1, "<sleeper_job>" },
	{ "ST_SUBMIT_IN_HOLD_DELETE",                  ST_SUBMIT_IN_HOLD_DELETE,                  1, "<sleeper_job>" },
	{ "ST_BULK_SUBMIT_IN_HOLD_SESSION_RELEASE",    ST_BULK_SUBMIT_IN_HOLD_SESSION_RELEASE,    1, "<sleeper_job>" },
	{ "ST_BULK_SUBMIT_IN_HOLD_SINGLE_RELEASE",     ST_BULK_SUBMIT_IN_HOLD_SINGLE_RELEASE,     1, "<sleeper_job>" },
	{ "ST_BULK_SUBMIT_IN_HOLD_SESSION_DELETE",     ST_BULK_SUBMIT_IN_HOLD_SESSION_DELETE,     1, "<sleeper_job>" },
	{ "ST_BULK_SUBMIT_IN_HOLD_SINGLE_DELETE",      ST_BULK_SUBMIT_IN_HOLD_SINGLE_DELETE,      1, "<sleeper_job>" },
	{ "ST_SUBMIT_POLLING_WAIT_TIMEOUT",            ST_SUBMIT_POLLING_WAIT_TIMEOUT,            1, "<sleeper_job>" },
	{ "ST_SUBMIT_POLLING_WAIT_ZEROTIMEOUT",        ST_SUBMIT_POLLING_WAIT_ZEROTIMEOUT,        1, "<sleeper_job>" },
	{ "ST_SUBMIT_POLLING_SYNCHRONIZE_TIMEOUT",     ST_SUBMIT_POLLING_SYNCHRONIZE_TIMEOUT,     1, "<sleeper_job>" },
	{ "ST_SUBMIT_POLLING_SYNCHRONIZE_ZEROTIMEOUT", ST_SUBMIT_POLLING_SYNCHRONIZE_ZEROTIMEOUT, 1, "<sleeper_job>" },
	{ "ST_ATTRIBUTE_CHANGE",                       ST_ATTRIBUTE_CHANGE,                       0, "" },
	{ "ST_SUBMIT_SUSPEND_RESUME_WAIT",             ST_SUBMIT_SUSPEND_RESUME_WAIT,             1, "<sleeper_job>" },
	{ "ST_USAGE_CHECK",                            ST_USAGE_CHECK,                            1, "<exit_arg_job>" },
	//   { "ST_UNSUPPORTED_ATTR",                       ST_UNSUPPORTED_ATTR,                       0, "" },
	//   { "ST_UNSUPPORTED_VATTR",                      ST_UNSUPPORTED_VATTR,                      0, "" },

	/* multiple application threads - automated tests only */
	{ "MT_SUBMIT_WAIT",                           MT_SUBMIT_WAIT,                             1, "<sleeper_job>" },
	{ "MT_SUBMIT_BEFORE_INIT_WAIT",               MT_SUBMIT_BEFORE_INIT_WAIT,                 1, "<sleeper_job>" },
	{ "MT_EXIT_DURING_SUBMIT",                    MT_EXIT_DURING_SUBMIT,                      1, "<sleeper_job>" },
	{ "MT_SUBMIT_MT_WAIT",                        MT_SUBMIT_MT_WAIT,                          1, "<sleeper_job>" },
	{ "MT_EXIT_DURING_SUBMIT_OR_WAIT",            MT_EXIT_DURING_SUBMIT_OR_WAIT,              1, "<sleeper_job>" },
	{ "ST_GET_NUM_JOBIDS",                        ST_GET_NUM_JOBIDS,                          1, "<sleeper_job>" },
	{ "ST_BULK_SUBMIT_INCRPH",                    ST_BULK_SUBMIT_INCRPH,                      1, "<sleeper_job>" },
	/* ------------------------------------------------------------------------------------ */
	{ NULL,                                       0 }
};

#define number_of_test_cases (sizeof(test_map)/sizeof(struct test_name2number_map)-1)

static int test(int *argc, char **argv[], int parse_args);
static int submit_and_wait(int n);
static int submit_sleeper(int n);
static int do_submit(drmaa_job_template_t *jt, int n);
static int wait_all_jobs(int n);
static int wait_n_jobs(int n);
static drmaa_job_template_t *create_sleeper_job_template(int seconds, int as_bulk_job, int in_hold);
static drmaa_job_template_t *create_exit_job_template(const char *exit_job,  int as_bulk_job);
static void report_session_key(void);
static void *submit_and_wait_thread (void *v);
static void *submit_sleeper_thread (void *v);

int str2drmaa_state(const char *str);
static int str2drmaa_ctrl(const char *str);
static int str2drmaa_errno(const char *str);
static const char *drmaa_state2str(int state);
static const char *drmaa_ctrl2str(int control);
static const char *drmaa_errno2str(int ctrl);
static bool check_job_state(const char* jobid, int expected);
static bool check_term_details(int stat, int exp_aborted, int exp_exited, int exp_signaled);

static void array_job_run_sequence_adapt(int **sequence, int job_id, int count); 

static int set_path_attribute_plus_colon(drmaa_job_template_t *jt, const char *name, 
										 const char *value, char *error_diagnosis, size_t error_diag_len);


static int set_path_attribute_plus_colon(drmaa_job_template_t *jt, const char *name, 
										 const char *value, char *error_diagnosis, size_t error_diag_len)
{
	char path_buffer[10000];
	strcpy(path_buffer, ":"); 
	strcat(path_buffer, value);
	return drmaa_set_attribute(jt, name, path_buffer, error_diagnosis, error_diag_len);         
}


static void report_wrong_job_finish(const char *comment, const char *jobid, int stat);

typedef struct {
	char *native;
	int time;
} test_job_t;
static int job_run_sequence_verify(int pos, char *all_jobids[], int *order[]);
static int **job_run_sequence_parse(char *jrs_str);

static int test_case;
static int is_sun_grid_engine;

/* global test case parameters */
char *sleeper_job = NULL,
*kill_job = NULL,
*exit_job = NULL,
*mirror_job = NULL,
*input_path = NULL,
*output_path = NULL,
*error_path = NULL,
*email_addr = NULL;
int ctrl_op = -1;


static void usage(void)
{
	int i;
	fprintf(stderr, "usage: test_drmaa <test_case>\n");

	fprintf(stderr, "  <test_case> is one of the keywords below including the enlisted test case arguments\n");
	for (i=0; test_map[i].test_name; i++)
		fprintf(stderr, "\t%-45.45s %s\n", test_map[i].test_name, test_map[i].opt_arguments);

	fprintf(stderr, "  <sleeper_job>  is an executable job that sleeps <argv1> seconds\n");
	fprintf(stderr, "                 the job must be executable at the target machine (e.g. /bin/sleep)\n");
	fprintf(stderr, "  <mirror_job>   is an executable job that returns it's stdin stream to stdout (e.g. /bin/cat)\n");
	fprintf(stderr, "                 the job must be executable at the target machine\n\n");
	fprintf(stderr, "  <exit_arg_job> is an executable job that exits <argv1> as exit status\n");
	fprintf(stderr, "                 the job must be executable at the target machine (e.g. ./test_exit_helper)\n");
	fprintf(stderr, "  <kill_arg_job> is an executable job that sends signal <argv1> to itself\n");
	fprintf(stderr, "                 the job must be executable at the target machine (e.g. ./test_kill_helper)\n");
	fprintf(stderr, "  <input_path>   is the path of an input file\n");
	fprintf(stderr, "                 the user must have read access to this file at the target machine\n");
	fprintf(stderr, "  <output_path>  is the path of an output file\n");
	fprintf(stderr, "                 the user must have write access to this file at the target machine\n");
	fprintf(stderr, "  <email_addr>   is an email address to which to send \n");
	fprintf(stderr, "                 job completion notices\n");
	fprintf(stderr, "  <native_spec0> a native specification\n");
	fprintf(stderr, "  <native_spec1> a native specification\n");
	fprintf(stderr, "  <native_spec2> a native specification\n\n");
	exit(1);
}

int main(int argc, char *argv[])
{
	int failed = 0;
	int i; 
	char diag[DRMAA_ERROR_STRING_BUFFER];

	if (argc == 1) 
		usage();

	/* Print out an adivsory */
	printf ("The DRMAA test suite " DRMAA_TEST_SUITE_VERSION " is now starting.  Once it has begun execution,\n");
	printf ("please do not interrupt (CTRL-C) it.  If the program is interrupted\n");
	printf ("before drmaa_exit() is called, session state information might be\n");
	printf ("left behind in your DRM system (%d).\n", number_of_test_cases);

	/* figure out which DRM system we are using */
	{
		char drm_name[DRMAA_DRM_SYSTEM_BUFFER];
		if (drmaa_get_DRM_system(drm_name, 255, diag, sizeof(diag)-1)!=DRMAA_ERRNO_SUCCESS) {
			fprintf(stderr, "drmaa_get_DRM_system() failed: %s\n", diag);
			return 1;
		}
		printf("Connected to DRM system \"%s\"\n", drm_name);
		if (!strncmp(drm_name, "SGE", 3))
			is_sun_grid_engine = 1;
		else
			is_sun_grid_engine = 0;
	}

	while (argc > 1) {
		/* map test name to test number */
		for (i=0; test_map[i].test_name; i++)
			if (!strcasecmp(argv[1], test_map[i].test_name))
				break;
		if (!test_map[i].test_name) {
			fprintf(stderr, "test_drmaa: %s is not a valid test name\n", argv[1]);
			usage();
		}
		test_case = test_map[i].test_number;
		argc--; argv++;
		if ((argc-1) < test_map[i].nargs)
			usage();

		if (test_case == ALL_TESTS) {
			int success = 1;
			int tests_done = 0;
			bool passed_tests[number_of_test_cases];
			sleeper_job = NEXT_ARGV(&argc, &argv);
			exit_job    = NEXT_ARGV(&argc, &argv);
			kill_job    = NEXT_ARGV(&argc, &argv);
			email_addr  = NEXT_ARGV(&argc, &argv);

			for(i=0; i<number_of_test_cases; i++) passed_tests[i]=false;

			for (i=1; test_map[i].test_name && test_map[i].test_number != number_of_test_cases && success; i++) {
				test_case = test_map[i].test_number;
				printf("---------------------\n");
				printf("starting test #%d (%s)\n", i, test_map[i].test_name);

				switch (test_case) {
case ST_EMPTY_SESSION_CONTROL: 
	{
		int i;
		const int ctrl_ops[] = { DRMAA_CONTROL_SUSPEND, DRMAA_CONTROL_RESUME, 
			DRMAA_CONTROL_HOLD, DRMAA_CONTROL_RELEASE, DRMAA_CONTROL_TERMINATE, -1 };
		for (i=0; ctrl_ops[i] != -1; i++) {
			ctrl_op = ctrl_ops[i]; 
			if (test(&argc, &argv, 0)!=0) {
				printf("test \"%s\" with \"%s\" failed\n", 
					test_map[i].test_name, drmaa_ctrl2str(ctrl_ops[i]));
				failed = 1;
				drmaa_exit(NULL, 0);
				break;
			} else
				printf("successfully finished test \"%s\" with \"%s\"\n", 
				test_map[i].test_name, drmaa_ctrl2str(ctrl_ops[i]));
		}
		break;
	}

default:
	if (test(&argc, &argv, 0)!=0) {
		printf("test #%d failed\n", i);
		failed = 1;
		drmaa_exit(NULL, 0);
		break;
	} else
		printf("successfully finished test #%d\n", i);
	break;
				}

				tests_done=tests_done+1;

				if (failed) {
					success = 0;
					passed_tests[i-1] = false;
				}   
				else
					passed_tests[i-1] = true;
			}
			// Summary
			if (success) {
				printf("\nCongratulations ! Your library passed all tests and is therefore DRMAA1.0-compliant for test suite version " DRMAA_TEST_SUITE_VERSION ".\n");
			}
			else
			{
				printf("\nOne or more tests failed. Please check the log messages for regarding text and consult the source "
					"code of the test suite for further hints. Don't hesitate to ask the DRMAA group "
					"(www.drmaa.org) if you need help or discovered a bug in the test suite.\n\n");
				for (i=0; i<tests_done; i++)  {
					if (!passed_tests[i])
						printf("Test %s failed\n", test_map[i+1].test_name);
				}       
			} 
		} else {
			printf("starting test \"%s\"\n", test_map[i].test_name);
			if (test(&argc, &argv, 1)!=0) {
				printf("test \"%s\" failed\n", test_map[i].test_name);
				failed = 1;
				drmaa_exit(NULL, 0);
				break;
			} else
				printf("successfully finished test \"%s\"\n", test_map[i].test_name);
		}
	} 

	return failed;
}


static int test(int *argc, char **argv[], int parse_args)
{
	int  i;
	int  job_chunk = JOB_CHUNK;
	char diagnosis[DRMAA_ERROR_STRING_BUFFER];
	drmaa_job_template_t *jt = NULL;
	int drmaa_errno=0;

	switch (test_case) {
case ST_MULT_INIT:
	{ 
		/* no test case arguments */

		if (drmaa_init(NULL, diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
			fprintf(stderr, "drmaa_init() failed: %s\n", diagnosis);
			return 1;
		}
		report_session_key();
		if (drmaa_init(NULL, diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_ALREADY_ACTIVE_SESSION) {
			fprintf(stderr, "drmaa_init() failed: %s\n", diagnosis);
			return 1;
		}
		if (drmaa_exit(diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
			fprintf(stderr, "drmaa_exit() failed: %s\n", diagnosis);
			return 1;
		}
	}
	break;

case ST_MULT_EXIT:
	{ 
		/* no test case arguments */

		if (drmaa_init(NULL, diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
			fprintf(stderr, "drmaa_init() failed: %s\n", diagnosis);
			return 1;
		}
		report_session_key();
		if (drmaa_exit(diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
			fprintf(stderr, "drmaa_exit(1) failed: %s\n", diagnosis);
			return 1;
		}
		if (drmaa_exit(diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_NO_ACTIVE_SESSION) {
			fprintf(stderr, "drmaa_exit(2) failed: %s\n", diagnosis);
			return 1;
		}
	}
	break;

case ST_SUBMIT_WAIT:
	{
		int n = (test_case == ST_SUBMIT_WAIT)?JOB_CHUNK:1;
		char jobid[1024];

		if (parse_args)
			sleeper_job = NEXT_ARGV(argc, argv);

		if (drmaa_init(NULL, diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
			fprintf(stderr, "drmaa_init() failed: %s\n", diagnosis);
			return 1;
		}
		report_session_key();

		if (!(jt = create_sleeper_job_template(5, 0, 0))) {
			fprintf(stderr, "create_sleeper_job_template() failed\n");
			return 1;
		}

		for (i=0; i<n; i++) {
			if (drmaa_run_job(jobid, sizeof(jobid)-1, jt, diagnosis, sizeof(diagnosis)-1)!=DRMAA_ERRNO_SUCCESS) {
				fprintf(stderr, "drmaa_run_job() failed: %s\n", diagnosis);
				return 1;
			}
			printf("submitted job \"%s\"\n", jobid);
		}

		drmaa_delete_job_template(jt, NULL, 0);

		if (wait_all_jobs(n) != DRMAA_ERRNO_SUCCESS) {
			return 1;
		}
		if (drmaa_exit(diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
			fprintf(stderr, "drmaa_exit() failed: %s\n", diagnosis);
			return 1;
		}
	}
	break;

case ST_SUBMIT_POLLING_WAIT_TIMEOUT:
case ST_SUBMIT_POLLING_WAIT_ZEROTIMEOUT:
case ST_SUBMIT_POLLING_SYNCHRONIZE_TIMEOUT:
case ST_SUBMIT_POLLING_SYNCHRONIZE_ZEROTIMEOUT:
	{
		char jobid[1024];
		const int timeout = 5;
		const char *session_all[] = { DRMAA_JOB_IDS_SESSION_ALL, NULL };

		if (parse_args)
			sleeper_job = NEXT_ARGV(argc, argv);

		if (drmaa_init(NULL, diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
			fprintf(stderr, "drmaa_init() failed: %s\n", diagnosis);
			return 1;
		}
		report_session_key();

		if (!(jt = create_sleeper_job_template(5, 0, 0))) {
			fprintf(stderr, "create_sleeper_job_template() failed\n");
			return 1;
		}

		if (drmaa_run_job(jobid, sizeof(jobid)-1, jt, diagnosis, sizeof(diagnosis)-1)!=DRMAA_ERRNO_SUCCESS) {
			fprintf(stderr, "drmaa_run_job() failed: %s\n", diagnosis);
			return 1;
		}
		printf("submitted job \"%s\"\n", jobid);

		drmaa_delete_job_template(jt, NULL, 0);

		switch (test_case) {

case ST_SUBMIT_POLLING_WAIT_TIMEOUT:
	while ((drmaa_errno=drmaa_wait(jobid, NULL, 0, NULL, timeout, NULL, 
		diagnosis, sizeof(diagnosis)-1))!=DRMAA_ERRNO_SUCCESS) {
			if (drmaa_errno != DRMAA_ERRNO_EXIT_TIMEOUT) {
				fprintf(stderr, "drmaa_wait(\"%s\", timeout = %d) failed: %s (%s)\n", 
					jobid, timeout, diagnosis, drmaa_strerror(drmaa_errno));
				return 1;
			}
			printf("still waiting for job \"%s\" to finish\n", jobid);
	}
	break;

case ST_SUBMIT_POLLING_WAIT_ZEROTIMEOUT:
	while ((drmaa_errno=drmaa_wait(jobid, NULL, 0, NULL, DRMAA_TIMEOUT_NO_WAIT, NULL, 
		diagnosis, sizeof(diagnosis)-1))!=DRMAA_ERRNO_SUCCESS) {
			if (drmaa_errno != DRMAA_ERRNO_EXIT_TIMEOUT) {
				fprintf(stderr, "drmaa_wait(\"%s\", no timeout) failed: %s (%s)\n", 
					jobid, diagnosis, drmaa_strerror(drmaa_errno));
				return 1;
			}
			printf("still waiting for job \"%s\" to finish\n", jobid);
			sleep(timeout);
			printf("slept %d seconds\n", timeout);
	}
	break;

case ST_SUBMIT_POLLING_SYNCHRONIZE_TIMEOUT:
	while ((drmaa_errno=drmaa_synchronize(session_all, timeout, 1,
		diagnosis, sizeof(diagnosis)-1))!=DRMAA_ERRNO_SUCCESS) {
			if (drmaa_errno != DRMAA_ERRNO_EXIT_TIMEOUT) {
				fprintf(stderr, "drmaa_synchronize(\"%s\", timeout = %d) failed: %s (%s)\n", 
					jobid, timeout, diagnosis, drmaa_strerror(drmaa_errno));
				return 1;
			}
			printf("still trying to synchronize with job \"%s\" to finish\n", jobid);
	}
	break;

case ST_SUBMIT_POLLING_SYNCHRONIZE_ZEROTIMEOUT:
	while ((drmaa_errno=drmaa_synchronize(session_all, DRMAA_TIMEOUT_NO_WAIT, 1, 
		diagnosis, sizeof(diagnosis)-1))!=DRMAA_ERRNO_SUCCESS) {
			if (drmaa_errno != DRMAA_ERRNO_EXIT_TIMEOUT) {
				fprintf(stderr, "drmaa_synchronize(\"%s\", no timeout) failed: %s (%s)\n", 
					jobid, diagnosis, drmaa_strerror(drmaa_errno));
				return 1;
			}
			printf("still trying to synchronize with job \"%s\" to finish\n", jobid);
			sleep(timeout);
			printf("slept %d seconds\n", timeout);
	}
	break;
		}

		if (drmaa_exit(diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
			fprintf(stderr, "drmaa_exit() failed: %s\n", diagnosis);
			return 1;
		}
	}
	break;

case MT_SUBMIT_WAIT:
	{
		pthread_t submitter_threads[NTHREADS]; 
		int n = -1;

		if (parse_args)
			sleeper_job = NEXT_ARGV(argc, argv);

		if (drmaa_init(NULL, diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
			fprintf(stderr, "drmaa_init() failed: %s\n", diagnosis);
			return 1;
		}
		report_session_key();

		for (i=0; i<NTHREADS; i++)
			pthread_create(&submitter_threads[i], NULL, submit_sleeper_thread, &job_chunk);
		for (i=0; i<NTHREADS; i++)
			if (pthread_join(submitter_threads[i], NULL)) 
				printf("pthread_join() returned != 0\n");
		if (wait_all_jobs(n) != DRMAA_ERRNO_SUCCESS) {
			return 1;
		}

		if (drmaa_exit(diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
			fprintf(stderr, "drmaa_exit() failed: %s\n", diagnosis);
			return 1;
		}
	}
	break;

case MT_SUBMIT_BEFORE_INIT_WAIT:
	{
		pthread_t submitter_threads[NTHREADS]; 
		int n = -1;

		if (parse_args)
			sleeper_job = NEXT_ARGV(argc, argv);
		printf("sleeper_job = %s\n", sleeper_job);
		for (i=0; i<NTHREADS; i++) {
			pthread_create(&submitter_threads[i], NULL, submit_sleeper_thread, &job_chunk);
		}

		/* delay drmaa_init() */
		sleep(5);
		if (drmaa_init(NULL, diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
			fprintf(stderr, "drmaa_init() failed: %s\n", diagnosis); 
			return 1;
		}
		report_session_key();

		for (i=0; i<NTHREADS; i++)
			if (pthread_join(submitter_threads[i], NULL))
				printf("pthread_join() returned != 0\n");

		if (wait_all_jobs(n) != DRMAA_ERRNO_SUCCESS) {
			return 1;
		}
		if (drmaa_exit(diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
			fprintf(stderr, "drmaa_exit() failed: %s\n", diagnosis);
			return 1;
		}
	}
	break;

case MT_EXIT_DURING_SUBMIT:
	{
		pthread_t submitter_threads[NTHREADS]; 

		if (parse_args)
			sleeper_job = NEXT_ARGV(argc, argv);

		putenv("SGE_DELAY_AFTER_SUBMIT=20");

		if (drmaa_init(NULL, diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
			fprintf(stderr, "drmaa_init() failed: %s\n", diagnosis);
			return 1;
		}
		report_session_key();

		for (i=0; i<NTHREADS; i++)
			pthread_create(&submitter_threads[i], NULL, submit_sleeper_thread, &job_chunk);
		sleep(1);
		if (drmaa_exit(diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
			fprintf(stderr, "drmaa_exit() failed: %s\n", diagnosis);
			return 1;
		}
		printf("drmaa_exit() succeeded\n");

		putenv("SGE_DELAY_AFTER_SUBMIT=0");

		for (i=0; i<NTHREADS; i++)
			if (pthread_join(submitter_threads[i], NULL))
				printf("pthread_join() returned != 0\n");
	}
	break;

case MT_SUBMIT_MT_WAIT:
	{
		pthread_t submitter_threads[NTHREADS]; 

		if (parse_args)
			sleeper_job = NEXT_ARGV(argc, argv);

		if (drmaa_init(NULL, diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
			fprintf(stderr, "drmaa_init() failed: %s\n", diagnosis);
			return 1;
		}
		report_session_key();

		for (i=0; i<NTHREADS; i++)
			pthread_create(&submitter_threads[i], NULL, submit_and_wait_thread, &job_chunk);

		for (i=0; i<NTHREADS; i++)
			if (pthread_join(submitter_threads[i], NULL))
				printf("pthread_join() returned != 0\n");

		if (drmaa_exit(diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
			fprintf(stderr, "drmaa_exit() failed: %s\n", diagnosis);
			return 1;
		}
	}
	break;

case MT_EXIT_DURING_SUBMIT_OR_WAIT:
	{
		pthread_t submitter_threads[NTHREADS]; 

		if (parse_args)
			sleeper_job = NEXT_ARGV(argc, argv);

		if (drmaa_init(NULL, diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
			fprintf(stderr, "drmaa_init() failed: %s\n", diagnosis);
			return 1;
		}
		report_session_key();

		for (i=0; i<NTHREADS; i++)
			pthread_create(&submitter_threads[i], NULL, submit_and_wait_thread, &job_chunk);
		sleep(20);
		printf("Now calling drmaa_exit(), while submitter threads are waiting ...\n");
		if (drmaa_exit(diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
			fprintf(stderr, "drmaa_exit() failed: %s\n", diagnosis);
			return 1;
		}
		printf("drmaa_exit() succeeded\n");

		for (i=0; i<NTHREADS; i++)
			if (pthread_join(submitter_threads[i], NULL))
				printf("pthread_join() returned != 0\n");
	}
	break;

case ST_BULK_SUBMIT_WAIT:
	{
		if (parse_args)
			sleeper_job = NEXT_ARGV(argc, argv);

		if (drmaa_init(NULL, diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
			fprintf(stderr, "drmaa_init() failed: %s\n", diagnosis);
			return 1;
		}
		report_session_key();
		if (!(jt = create_sleeper_job_template(5, 1, 0))) {
			fprintf(stderr, "create_sleeper_job_template() failed\n");
			return 1;
		}
		for (i=0; i<NBULKS; i++) {
			char jobid[100];
			drmaa_job_ids_t *jobids;
			int j;
			if ((drmaa_errno=drmaa_run_bulk_jobs(&jobids, jt, 1, JOB_CHUNK, 1, diagnosis, sizeof(diagnosis)-1))!=DRMAA_ERRNO_SUCCESS) {
				printf("failed submitting bulk job (%s): %s\n", drmaa_strerror(drmaa_errno), diagnosis);
				return 1;
			} 
			printf("submitted %u bulk job with jobids:\n", JOB_CHUNK);
			for (j=0; j<JOB_CHUNK; j++) {
				drmaa_get_next_job_id(jobids, jobid, sizeof(jobid)-1);
				printf("\t \"%s\"\n", jobid);
			} 
			drmaa_release_job_ids(jobids);
		}
		drmaa_delete_job_template(jt, NULL, 0);

		if (wait_n_jobs(JOB_CHUNK*NBULKS) != DRMAA_ERRNO_SUCCESS) {
			return 1;
		}
		if (drmaa_exit(diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
			fprintf(stderr, "drmaa_exit() failed: %s\n", diagnosis);
			return 1;
		}
	}
	break;

case ST_BULK_SUBMIT_INCRPH:
	{
		char* outfile=":$drmaa_hd_ph$/tmp_out$drmaa_incr_ph$";
		if (parse_args)
			sleeper_job = NEXT_ARGV(argc, argv);

		if (drmaa_init(NULL, diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
			fprintf(stderr, "drmaa_init() failed: %s\n", diagnosis);
			return 1;
		}
		report_session_key();
		if (!(jt = create_sleeper_job_template(5, 1, 0))) {
			fprintf(stderr, "create_sleeper_job_template() failed\n");
			return 1;
		}
		drmaa_set_attribute(jt, DRMAA_OUTPUT_PATH, outfile, NULL, 0);
		char jobid[100];
		drmaa_job_ids_t *jobids;
		unsigned int i;
		if ((drmaa_errno=drmaa_run_bulk_jobs(&jobids, jt, 10, JOB_CHUNK+10, 1, diagnosis, sizeof(diagnosis)-1))!=DRMAA_ERRNO_SUCCESS) {
			printf("failed submitting bulk job (%s): %s\n", drmaa_strerror(drmaa_errno), diagnosis);
			return 1;
		} 
		printf("submitted %u bulk job with jobids:\n", JOB_CHUNK);
		drmaa_release_job_ids(jobids);
		drmaa_delete_job_template(jt, NULL, 0);

		if (wait_n_jobs(JOB_CHUNK) != DRMAA_ERRNO_SUCCESS) {
			return 1;
		}
		// search for the output files and check their names
		struct stat theStat;
		char* fname=malloc(sizeof(char)*1024);
		for(i=10;i<=JOB_CHUNK+10;i++) {
			snprintf(fname, 1024, "%s/tmp_out%u", getenv("HOME"), i);
			if(stat(fname,&theStat) != 0) {
				fprintf(stderr, "Could not find expected output file %s\n", fname);
				return 1;
			} else {
				printf("Found expected output file %s\n",fname);
				if (remove(fname) != 0) {
					printf( "Warning - could not remove temporary file %s: %s\n", fname, strerror( errno ) );
				}
			}
		}

		if (drmaa_exit(diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
			fprintf(stderr, "drmaa_exit() failed: %s\n", diagnosis);
			return 1;
		}
	}
	break;

case ST_BULK_SINGLESUBMIT_WAIT_INDIVIDUAL:
	{
		const char *all_jobids[NBULKS*JOB_CHUNK + JOB_CHUNK+1];
		char jobid[100];
		int pos = 0;

		if (parse_args)
			sleeper_job = NEXT_ARGV(argc, argv);

		if (drmaa_init(NULL, diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
			fprintf(stderr, "drmaa_init() failed: %s\n", diagnosis);
			return 1;
		}
		report_session_key();

		/*
		*   submit some bulk jobs
		*/
		if (!(jt = create_sleeper_job_template(5, 1, 0))) {
			fprintf(stderr, "create_sleeper_job_template() failed\n");
			return 1;
		}
		for (i=0; i<NBULKS; i++) {
			drmaa_job_ids_t *jobids;
			int j;

			while ((drmaa_errno=drmaa_run_bulk_jobs(&jobids, jt, 1, JOB_CHUNK, 1, diagnosis,
				sizeof(diagnosis)-1))==DRMAA_ERRNO_DRM_COMMUNICATION_FAILURE) {
					fprintf(stderr, "drmaa_run_bulk_jobs() failed - retry: %s\n", diagnosis);
					sleep(1);
			} 
			if (drmaa_errno != DRMAA_ERRNO_SUCCESS) {
				fprintf(stderr, "drmaa_run_bulk_jobs() failed: %s\n", diagnosis);
				return 1;
			}

			printf("submitted bulk job with jobids:\n");
			for (j=0; j<JOB_CHUNK; j++) {
				drmaa_get_next_job_id(jobids, jobid, sizeof(jobid)-1);
				all_jobids[pos++] = strdup(jobid);
				printf("\t \"%s\"\n", jobid);
			} 
			drmaa_release_job_ids(jobids);
			sleep(1);
		}
		drmaa_delete_job_template(jt, NULL, 0);

		/*
		*   submit some sequential jobs
		*/
		if (!(jt = create_sleeper_job_template(5, 0, 0))) {
			fprintf(stderr, "create_sleeper_job_template() failed\n");
			return 1;
		}
		for (i=0; i<JOB_CHUNK; i++) {
			while ((drmaa_errno=drmaa_run_job(jobid, sizeof(jobid)-1, jt, diagnosis,
				sizeof(diagnosis)-1)) == DRMAA_ERRNO_DRM_COMMUNICATION_FAILURE) {
					fprintf(stderr, "drmaa_run_job() failed - retry: %s\n", diagnosis);
					sleep(1);
			}
			if (drmaa_errno != DRMAA_ERRNO_SUCCESS) {
				fprintf(stderr, "drmaa_run_job() failed: %s\n", diagnosis);
				return 1;
			}
			all_jobids[pos++] = strdup(jobid);
			printf("\t \"%s\"\n", jobid);
		}

		/* set string array end mark */
		all_jobids[pos] = NULL;

		drmaa_delete_job_template(jt, NULL, 0);

		/*
		*   wait all those jobs
		*/
		for (pos=0; pos<NBULKS*JOB_CHUNK + JOB_CHUNK; pos++) {
			do {
				int stat;
				drmaa_errno = drmaa_wait(all_jobids[pos], jobid, sizeof(jobid)-1, 
					&stat, max_wait_timeout, NULL, diagnosis, sizeof(diagnosis)-1);
				if (drmaa_errno != DRMAA_ERRNO_SUCCESS) {
					fprintf(stderr, "drmaa_wait(%s) failed - retry: %s\n", all_jobids[pos], diagnosis); 
					sleep(1);
				}
			} while (drmaa_errno == DRMAA_ERRNO_DRM_COMMUNICATION_FAILURE);

			if (drmaa_errno != DRMAA_ERRNO_SUCCESS) {
				fprintf(stderr, "drmaa_wait(%s) failed: %s\n", all_jobids[pos], diagnosis);
				return 1;
			}
			printf("waited job \"%s\"\n", all_jobids[pos]);
		}
		if (drmaa_exit(diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
			fprintf(stderr, "drmaa_exit() failed: %s\n", diagnosis);
			return 1;
		}
	}
	break;

case ST_SUBMITMIXTURE_SYNC_ALL_DISPOSE:
	/* - drmaa_init() is called
	- submit a mixture of single and bulk jobs
	- do drmaa_synchronize(DRMAA_JOB_IDS_SESSION_ALL, dispose) 
	to wait for all jobs to finish
	- then drmaa_exit() is called */
	{
		const char *session_all[] = { DRMAA_JOB_IDS_SESSION_ALL, NULL };
		char jobid[100];

		if (parse_args)
			sleeper_job = NEXT_ARGV(argc, argv);

		if (drmaa_init(NULL, diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
			fprintf(stderr, "drmaa_init() failed: %s\n", diagnosis);
			return 1;
		}
		report_session_key();

		/*
		*   submit some bulk jobs
		*/
		if (!(jt = create_sleeper_job_template(5, 1, 0))) {
			fprintf(stderr, "create_sleeper_job_template() failed\n");
			return 1;
		}
		for (i=0; i<NBULKS; i++) {
			drmaa_job_ids_t *jobids;
			int j;

			while ((drmaa_errno=drmaa_run_bulk_jobs(&jobids, jt, 1, JOB_CHUNK, 1, diagnosis,
				sizeof(diagnosis)-1))==DRMAA_ERRNO_DRM_COMMUNICATION_FAILURE) {
					fprintf(stderr, "drmaa_run_bulk_jobs() failed - retry: %s\n", diagnosis);
					sleep(1);
			} 
			if (drmaa_errno != DRMAA_ERRNO_SUCCESS) {
				fprintf(stderr, "drmaa_run_bulk_jobs() failed: %s\n", diagnosis);
				return 1;
			}

			printf("submitted bulk job with jobids:\n");
			for (j=0; j<JOB_CHUNK; j++) {
				drmaa_get_next_job_id(jobids, jobid, sizeof(jobid)-1);
				printf("\t \"%s\"\n", jobid);
			} 
			drmaa_release_job_ids(jobids);
			sleep(1);
		}
		drmaa_delete_job_template(jt, NULL, 0);

		/*
		*   submit some sequential jobs
		*/
		if (!(jt = create_sleeper_job_template(5, 0, 0))) {
			fprintf(stderr, "create_sleeper_job_template() failed\n");
			return 1;
		}
		for (i=0; i<JOB_CHUNK; i++) {
			while ((drmaa_errno=drmaa_run_job(jobid, sizeof(jobid)-1, jt, diagnosis,
				sizeof(diagnosis)-1)) == DRMAA_ERRNO_DRM_COMMUNICATION_FAILURE) {
					fprintf(stderr, "drmaa_run_job() failed - retry: %s\n", diagnosis);
					sleep(1);
			}
			if (drmaa_errno != DRMAA_ERRNO_SUCCESS) {
				fprintf(stderr, "drmaa_run_job() failed: %s\n", diagnosis);
				return 1;
			}
			printf("\t \"%s\"\n", jobid);
		}
		drmaa_delete_job_template(jt, NULL, 0);

		/*
		*   synchronize with all jobs
		*/
		drmaa_errno = drmaa_synchronize(session_all, max_wait_timeout, 1, diagnosis, sizeof(diagnosis)-1);
		if (drmaa_errno != DRMAA_ERRNO_SUCCESS) {
			fprintf(stderr, "drmaa_synchronize(DRMAA_JOB_IDS_SESSION_ALL, dispose) failed: %s\n", diagnosis);
			return 1;
		}
		printf("waited all jobs\n");
		if (drmaa_exit(diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
			fprintf(stderr, "drmaa_exit() failed: %s\n", diagnosis);
			return 1;
		}
		break;
	}

case ST_SUBMITMIXTURE_SYNC_ALL_NODISPOSE:

	/* - drmaa_init() is called
	- submit a mixture of single and bulk jobs
	- do drmaa_synchronize(DRMAA_JOB_IDS_SESSION_ALL, no-dispose) 
	to wait for all jobs to finish
	- do drmaa_wait(DRMAA_JOB_IDS_SESSION_ANY) until 
	DRMAA_ERRNO_INVALID_JOB to reap all jobs
	- then drmaa_exit() is called */

	{
		const char *all_jobids[NBULKS*JOB_CHUNK + JOB_CHUNK+1];
		const char *session_all[] = { DRMAA_JOB_IDS_SESSION_ALL, NULL };
		char jobid[100];
		int pos = 0;

		if (parse_args)
			sleeper_job = NEXT_ARGV(argc, argv);

		if (drmaa_init(NULL, diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
			fprintf(stderr, "drmaa_init() failed: %s\n", diagnosis);
			return 1;
		}
		report_session_key();

		/*
		*   submit some bulk jobs
		*/
		if (!(jt = create_sleeper_job_template(5, 1, 0))) {
			fprintf(stderr, "create_sleeper_job_template() failed\n");
			return 1;
		}
		for (i=0; i<NBULKS; i++) {
			drmaa_job_ids_t *jobids;
			int j;

			while ((drmaa_errno=drmaa_run_bulk_jobs(&jobids, jt, 1, JOB_CHUNK, 1, diagnosis,
				sizeof(diagnosis)-1))==DRMAA_ERRNO_DRM_COMMUNICATION_FAILURE) {
					fprintf(stderr, "drmaa_run_bulk_jobs() failed - retry: %s\n", diagnosis);
					sleep(1);
			} 
			if (drmaa_errno != DRMAA_ERRNO_SUCCESS) {
				fprintf(stderr, "drmaa_run_bulk_jobs() failed: %s\n", diagnosis);
				return 1;
			}

			printf("submitted bulk job with jobids:\n");
			for (j=0; j<JOB_CHUNK; j++) {
				drmaa_get_next_job_id(jobids, jobid, sizeof(jobid)-1);
				all_jobids[pos++] = strdup(jobid);
				printf("\t \"%s\"\n", jobid);
			} 
			drmaa_release_job_ids(jobids);
			sleep(1);
		}
		drmaa_delete_job_template(jt, NULL, 0);

		/*
		*   submit some sequential jobs
		*/
		if (!(jt = create_sleeper_job_template(5, 0, 0))) {
			fprintf(stderr, "create_sleeper_job_template() failed\n");
			return 1;
		}
		for (i=0; i<JOB_CHUNK; i++) {
			while ((drmaa_errno=drmaa_run_job(jobid, sizeof(jobid)-1, jt, diagnosis,
				sizeof(diagnosis)-1)) == DRMAA_ERRNO_DRM_COMMUNICATION_FAILURE) {
					fprintf(stderr, "drmaa_run_job() failed - retry: %s\n", diagnosis);
					sleep(1);
			}
			if (drmaa_errno != DRMAA_ERRNO_SUCCESS) {
				fprintf(stderr, "drmaa_run_job() failed: %s\n", diagnosis);
				return 1;
			}
			printf("\t \"%s\"\n", jobid);
			all_jobids[pos++] = strdup(jobid);
		}

		/* set string array end mark */
		all_jobids[pos] = NULL;

		drmaa_delete_job_template(jt, NULL, 0);

		/*
		*   synchronize with all jobs
		*/
		drmaa_errno = drmaa_synchronize(session_all, max_wait_timeout, 0, diagnosis, sizeof(diagnosis)-1);
		if (drmaa_errno != DRMAA_ERRNO_SUCCESS) {
			fprintf(stderr, "drmaa_synchronize(DRMAA_JOB_IDS_SESSION_ALL, dispose) failed: %s\n", diagnosis);
			return 1;
		}
		printf("synchronized with all jobs\n");

		/*
		*   wait all those jobs
		*/
		for (pos=0; pos<NBULKS*JOB_CHUNK + JOB_CHUNK; pos++) {
			do {
				int stat;
				drmaa_errno = drmaa_wait(all_jobids[pos], jobid, sizeof(jobid)-1, 
					&stat, max_wait_timeout, NULL, diagnosis, sizeof(diagnosis)-1);
				if (drmaa_errno != DRMAA_ERRNO_SUCCESS) {
					fprintf(stderr, "drmaa_wait(%s) failed - retry: %s\n", all_jobids[pos], diagnosis); 
					sleep(1);
				}
			} while (drmaa_errno == DRMAA_ERRNO_DRM_COMMUNICATION_FAILURE);

			if (drmaa_errno != DRMAA_ERRNO_SUCCESS) {
				fprintf(stderr, "drmaa_wait(%s) failed: %s\n", all_jobids[pos], diagnosis);
				return 1;
			}
			printf("waited job \"%s\"\n", all_jobids[pos]);
		}
		if (drmaa_exit(diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
			fprintf(stderr, "drmaa_exit() failed: %s\n", diagnosis);
			return 1;
		}
		break;
	}

case ST_SUBMITMIXTURE_SYNC_ALLIDS_DISPOSE:
	/* - drmaa_init() is called
	- submit a mixture of single and bulk jobs
	- do drmaa_synchronize(all_jobids, dispose) 
	to wait for all jobs to finish
	- then drmaa_exit() is called */
	{
		const char *all_jobids[NBULKS*JOB_CHUNK + JOB_CHUNK+1];
		char jobid[100];
		int pos = 0;

		if (parse_args)
			sleeper_job = NEXT_ARGV(argc, argv);

		if (drmaa_init(NULL, diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
			fprintf(stderr, "drmaa_init() failed: %s\n", diagnosis);
			return 1;
		}
		report_session_key();

		/*
		*   submit some bulk jobs
		*/
		if (!(jt = create_sleeper_job_template(5, 1, 0))) {
			fprintf(stderr, "create_sleeper_job_template() failed\n");
			return 1;
		}
		for (i=0; i<NBULKS; i++) {
			drmaa_job_ids_t *jobids;
			int j;

			while ((drmaa_errno=drmaa_run_bulk_jobs(&jobids, jt, 1, JOB_CHUNK, 1, diagnosis,
				sizeof(diagnosis)-1))==DRMAA_ERRNO_DRM_COMMUNICATION_FAILURE) {
					fprintf(stderr, "drmaa_run_bulk_jobs() failed - retry: %s\n", diagnosis);
					sleep(1);
			} 
			if (drmaa_errno != DRMAA_ERRNO_SUCCESS) {
				fprintf(stderr, "drmaa_run_bulk_jobs() failed: %s\n", diagnosis);
				return 1;
			}

			printf("submitted bulk job with jobids:\n");
			for (j=0; j<JOB_CHUNK; j++) {
				drmaa_get_next_job_id(jobids, jobid, sizeof(jobid)-1);
				printf("\t \"%s\"\n", jobid);
				all_jobids[pos++] = strdup(jobid);
			} 
			drmaa_release_job_ids(jobids);
			sleep(1);
		}
		drmaa_delete_job_template(jt, NULL, 0);

		/*
		*   submit some sequential jobs
		*/
		if (!(jt = create_sleeper_job_template(5, 0, 0))) {
			fprintf(stderr, "create_sleeper_job_template() failed\n");
			return 1;
		}
		for (i=0; i<JOB_CHUNK; i++) {
			while ((drmaa_errno=drmaa_run_job(jobid, sizeof(jobid)-1, jt, diagnosis,
				sizeof(diagnosis)-1)) == DRMAA_ERRNO_DRM_COMMUNICATION_FAILURE) {
					fprintf(stderr, "drmaa_run_job() failed - retry: %s\n", diagnosis);
					sleep(1);
			}
			if (drmaa_errno != DRMAA_ERRNO_SUCCESS) {
				fprintf(stderr, "drmaa_run_job() failed: %s\n", diagnosis);
				return 1;
			}
			printf("\t \"%s\"\n", jobid);
			all_jobids[pos++] = strdup(jobid);
		}

		/* set string array end mark */
		all_jobids[pos] = NULL;

		drmaa_delete_job_template(jt, NULL, 0);

		/*
		*   synchronize with all jobs
		*/
		drmaa_errno = drmaa_synchronize(all_jobids, max_wait_timeout, 1, diagnosis, sizeof(diagnosis)-1);
		if (drmaa_errno != DRMAA_ERRNO_SUCCESS) {
			fprintf(stderr, "drmaa_synchronize(DRMAA_JOB_IDS_SESSION_ALL, dispose) failed: %s\n", diagnosis);
			return 1;
		}
		printf("waited all jobs\n");
		if (drmaa_exit(diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
			fprintf(stderr, "drmaa_exit() failed: %s\n", diagnosis);
			return 1;
		}
		break;
	}

case ST_SUBMITMIXTURE_SYNC_ALLIDS_NODISPOSE:

	/* - drmaa_init() is called
	- submit a mixture of single and bulk jobs
	- do drmaa_synchronize(all_jobids, no-dispose) 
	to wait for all jobs to finish
	- do drmaa_wait(DRMAA_JOB_IDS_SESSION_ANY) until 
	DRMAA_ERRNO_INVALID_JOB to reap all jobs
	- then drmaa_exit() is called */

	{
		const char *all_jobids[NBULKS*JOB_CHUNK + JOB_CHUNK+1];
		char jobid[100];
		int pos = 0;

		if (parse_args)
			sleeper_job = NEXT_ARGV(argc, argv);

		if (drmaa_init(NULL, diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
			fprintf(stderr, "drmaa_init() failed: %s\n", diagnosis);
			return 1;
		}
		report_session_key();

		/*
		*   submit some bulk jobs
		*/
		if (!(jt = create_sleeper_job_template(5, 1, 0))) {
			fprintf(stderr, "create_sleeper_job_template() failed\n");
			return 1;
		}
		for (i=0; i<NBULKS; i++) {
			drmaa_job_ids_t *jobids;
			int j;

			while ((drmaa_errno=drmaa_run_bulk_jobs(&jobids, jt, 1, JOB_CHUNK, 1, diagnosis,
				sizeof(diagnosis)-1))==DRMAA_ERRNO_DRM_COMMUNICATION_FAILURE) {
					fprintf(stderr, "drmaa_run_bulk_jobs() failed - retry: %s\n", diagnosis);
					sleep(1);
			} 
			if (drmaa_errno != DRMAA_ERRNO_SUCCESS) {
				fprintf(stderr, "drmaa_run_bulk_jobs() failed: %s\n", diagnosis);
				return 1;
			}

			printf("submitted bulk job with jobids:\n");
			for (j=0; j<JOB_CHUNK; j++) {
				drmaa_get_next_job_id(jobids, jobid, sizeof(jobid)-1);
				all_jobids[pos++] = strdup(jobid);
				printf("\t \"%s\"\n", jobid);
			} 
			drmaa_release_job_ids(jobids);
		}
		drmaa_delete_job_template(jt, NULL, 0);

		/*
		*   submit some sequential jobs
		*/
		if (!(jt = create_sleeper_job_template(5, 0, 0))) {
			fprintf(stderr, "create_sleeper_job_template() failed\n");
			return 1;
		}
		for (i=0; i<JOB_CHUNK; i++) {
			while ((drmaa_errno=drmaa_run_job(jobid, sizeof(jobid)-1, jt, diagnosis,
				sizeof(diagnosis)-1)) == DRMAA_ERRNO_DRM_COMMUNICATION_FAILURE) {
					fprintf(stderr, "drmaa_run_job() failed - retry: %s\n", diagnosis);
					sleep(1);
			}
			if (drmaa_errno != DRMAA_ERRNO_SUCCESS) {
				fprintf(stderr, "drmaa_run_job() failed: %s\n", diagnosis);
				return 1;
			}
			printf("\t \"%s\"\n", jobid);
			all_jobids[pos++] = strdup(jobid);
		}

		/* set string array end mark */
		all_jobids[pos] = NULL;

		drmaa_delete_job_template(jt, NULL, 0);

		/*
		*   synchronize with all jobs
		*/
		drmaa_errno = drmaa_synchronize(all_jobids, max_wait_timeout, 0, diagnosis, sizeof(diagnosis)-1);
		if (drmaa_errno != DRMAA_ERRNO_SUCCESS) {
			fprintf(stderr, "drmaa_synchronize(DRMAA_JOB_IDS_SESSION_ALL, dispose) failed: %s\n", diagnosis);
			return 1;
		}
		printf("synchronized with all jobs\n");

		/*
		*   wait all those jobs
		*/
		for (pos=0; pos<NBULKS*JOB_CHUNK + JOB_CHUNK; pos++) {
			do {
				int stat;
				drmaa_errno = drmaa_wait(all_jobids[pos], jobid, sizeof(jobid)-1, 
					&stat, max_wait_timeout, NULL, diagnosis, sizeof(diagnosis)-1);
				if (drmaa_errno != DRMAA_ERRNO_SUCCESS) {
					fprintf(stderr, "drmaa_wait(%s) failed - retry: %s\n", all_jobids[pos], diagnosis); 
					sleep(1);
				}
			} while (drmaa_errno == DRMAA_ERRNO_DRM_COMMUNICATION_FAILURE);

			if (drmaa_errno != DRMAA_ERRNO_SUCCESS) {
				fprintf(stderr, "drmaa_wait(%s) failed: %s\n", all_jobids[pos], diagnosis);
				return 1;
			}
			printf("waited job \"%s\"\n", all_jobids[pos]);
		}
		if (drmaa_exit(diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
			fprintf(stderr, "drmaa_exit() failed: %s\n", diagnosis);
			return 1;
		}
		break;
	}

case ST_INPUT_FILE_FAILURE:
case ST_ERROR_FILE_FAILURE:
case ST_OUTPUT_FILE_FAILURE:
	{
		int stat;
		bool submitted;
		char jobid[100];
		const char *session_all[] = { DRMAA_JOB_IDS_SESSION_ALL, NULL };

		if (parse_args)
			sleeper_job = NEXT_ARGV(argc, argv);

		if (drmaa_init(NULL, diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
			fprintf(stderr, "drmaa_init() failed: %s\n", diagnosis);
			return 1;
		}
		report_session_key();

		/* submit a job that must fail */
		drmaa_allocate_job_template(&jt, NULL, 0);
		drmaa_set_attribute(jt, DRMAA_REMOTE_COMMAND, sleeper_job, NULL, 0);

		switch (test_case) {
case ST_OUTPUT_FILE_FAILURE:
	drmaa_set_attribute(jt, DRMAA_JOIN_FILES, "y", NULL, 0);
	drmaa_set_attribute(jt, DRMAA_OUTPUT_PATH, ":/etc/passwd", NULL, 0);
	break;

case ST_ERROR_FILE_FAILURE:
	drmaa_set_attribute(jt, DRMAA_JOIN_FILES, "n", NULL, 0);
	drmaa_set_attribute(jt, DRMAA_ERROR_PATH, ":/etc/passwd", NULL, 0);
	break;

case ST_INPUT_FILE_FAILURE:
	drmaa_set_attribute(jt, DRMAA_INPUT_PATH, ":not_existing_file", NULL, 0);
	break;
		}

		drmaa_errno = drmaa_run_job(jobid, sizeof(jobid)-1, jt, diagnosis, sizeof(diagnosis)-1); 
		submitted = false;
		if (DRMAA_ERRNO_SUCCESS != drmaa_errno) {
			printf("drmaa_run_job() failed because of the wrong parameters. This is acceptable according to the spec.\n");
		}
		else {
			printf("submitted job \"%s\"\n", jobid);
			submitted = true;
		}
		drmaa_delete_job_template(jt, NULL, 0);

		if (submitted) {

			/* synchronize with job to finish but do not dispose job finish information */
			if ((drmaa_errno = drmaa_synchronize(session_all, max_wait_timeout, 0, 
				diagnosis, sizeof(diagnosis)-1))!=DRMAA_ERRNO_SUCCESS) {
					fprintf(stderr, "drmaa_synchronize(DRMAA_JOB_IDS_SESSION_ALL, dispose) failed: %s\n", diagnosis);
					return 1;
			}
			printf("synchronized with job finish\n");

			/* get job state */
			if (!check_job_state(jobid, DRMAA_PS_FAILED)) return 1;

			/* wait job */
			/* this is not essentially part of the intended test, but should work anyway */
			if ((drmaa_errno = drmaa_wait(jobid, NULL, 0, &stat, DRMAA_TIMEOUT_NO_WAIT, NULL, 
				diagnosis, sizeof(diagnosis)-1)) != DRMAA_ERRNO_SUCCESS) {
					printf("drmaa_wait() with NO_WAIT failed for terminated job: %s (%s)\n", drmaa_strerror(drmaa_errno), diagnosis);
					return 1;
			}

		}

		if (drmaa_exit(diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
			fprintf(stderr, "drmaa_exit() failed: %s\n", diagnosis);
			return 1;
		}
	}
	break;

case ST_SUPPORTED_ATTR:
case ST_SUPPORTED_VATTR:
	/* - drmaa_init() is called
	- drmaa_get_attribute_names()/drmaa_get_vector_attribute_names() is called
	- the names of all supported non vector/vector attributes are printed
	- then drmaa_exit() is called */
	{
		drmaa_attr_names_t *vector;
		char attr_name[DRMAA_ATTR_BUFFER];

		if (drmaa_init(NULL, diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
			fprintf(stderr, "drmaa_init() failed: %s\n", diagnosis);
			return 1;
		}

		if (test_case == ST_SUPPORTED_ATTR)
			drmaa_errno = drmaa_get_attribute_names(&vector, diagnosis, sizeof(diagnosis)-1);
		else
			drmaa_errno = drmaa_get_vector_attribute_names(&vector, diagnosis, sizeof(diagnosis)-1);

		if (drmaa_errno != DRMAA_ERRNO_SUCCESS) {
			fprintf(stderr, "drmaa_get_attribute_names()/drmaa_get_vector_attribute_names() failed: %s\n", 
				diagnosis);
			return 1;
		}             

		unsigned int numAttr=0;
		while ((drmaa_errno=drmaa_get_next_attr_name(vector, attr_name, sizeof(attr_name)-1))==DRMAA_ERRNO_SUCCESS) {
			numAttr++;
			printf("%s\n", attr_name);
		}

		if (drmaa_errno != DRMAA_ERRNO_NO_MORE_ELEMENTS) {
			fprintf(stderr, "drmaa_get_next_attr_name() failed, expected was DRMAA_ERRNO_NO_MORE_ELEMENTS: %s\n", 
				diagnosis);
			return 1;
		}

		size_t size;
		if (drmaa_get_num_attr_names(vector, &size) != DRMAA_ERRNO_SUCCESS) {
			fprintf(stderr, "drmaa_get_num_attr_names() failed\n");
			return 1;
		}

		if (size != numAttr) {
			fprintf(stderr, "drmaa_get_num_attr_names() returned %lu, expected %u\n",(unsigned long)size, numAttr);
			return 1;
		}

		if (drmaa_exit(diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
			fprintf(stderr, "drmaa_exit() failed: %s\n", diagnosis);
			return 1;
		}
	}
	break;

case ST_VERSION:
	/* - drmaa_version() is called 
	- version information is printed */
	{
		unsigned int major, minor;
		if (drmaa_version(&major, &minor, diagnosis, sizeof(diagnosis)-1)
			!=DRMAA_ERRNO_SUCCESS) {
				fprintf(stderr, "drmaa_version() failed: %s\n", diagnosis);
				return 1;
		}
		if (major != 1 || minor != 0) {
			fprintf(stderr, "Wrong DRMAA version returned on drmaa_version() (was %d.%d, should be 1.0)\n", major, minor);
			return 1;
		}
		printf("version %d.%d\n", major, minor);
	}
	break;

case ST_CONTACT:
case ST_DRM_SYSTEM:
case ST_DRMAA_IMPL:
	{
		char output_string[1024];

		if (test_case == ST_CONTACT)
			drmaa_errno = drmaa_get_contact(output_string, sizeof(output_string)-1, 
			diagnosis, sizeof(diagnosis)-1);
		else if (test_case == ST_DRM_SYSTEM)
			drmaa_errno = drmaa_get_DRM_system(output_string, sizeof(output_string)-1,
			diagnosis, sizeof(diagnosis)-1);
		else if (test_case == ST_DRMAA_IMPL)
			drmaa_errno = drmaa_get_DRMAA_implementation(output_string, sizeof(output_string)-1,
			diagnosis, sizeof(diagnosis)-1);

		if (drmaa_errno != DRMAA_ERRNO_SUCCESS) {
			fprintf(stderr, "drmaa_get_contact()/drmaa_get_DRM_system() failed: %s\n", diagnosis);
			return 1;
		}

		if (test_case == ST_CONTACT)
			printf("drmaa_get_contact() returned \"%s\" before init\n", output_string);
		else if (test_case == ST_DRM_SYSTEM)
			printf("drmaa_get_DRM_system() returned \"%s\" before init\n", output_string);
		else if (test_case == ST_DRMAA_IMPL)
			printf("drmaa_get_DRMAA_implementation() returned \"%s\" before init\n", output_string);

		if (drmaa_init(NULL, diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
			fprintf(stderr, "drmaa_init() failed: %s\n", diagnosis);
			return 1;
		}

		if (test_case == ST_CONTACT)
			drmaa_errno = drmaa_get_contact(output_string, sizeof(output_string)-1, 
			diagnosis, sizeof(diagnosis)-1);
		else if (test_case == ST_DRM_SYSTEM)
			drmaa_errno = drmaa_get_DRM_system(output_string, sizeof(output_string)-1,
			diagnosis, sizeof(diagnosis)-1);
		else if (test_case == ST_DRMAA_IMPL)
			drmaa_errno = drmaa_get_DRMAA_implementation(output_string, sizeof(output_string)-1,
			diagnosis, sizeof(diagnosis)-1);

		if (drmaa_errno != DRMAA_ERRNO_SUCCESS) {
			fprintf(stderr, "drmaa_get_contact()/drmaa_get_DRM_system() failed: %s\n", diagnosis);
			return 1;
		}

		if (test_case == ST_CONTACT)
			printf("drmaa_get_contact() returned \"%s\" after init\n", output_string);
		else if (test_case == ST_DRM_SYSTEM)
			printf("drmaa_get_DRM_system() returned \"%s\" after init\n", output_string);
		else if (test_case == ST_DRMAA_IMPL)
			printf("drmaa_get_DRMAA_implementation() returned \"%s\" after init\n", output_string);

		if (drmaa_exit(diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
			fprintf(stderr, "drmaa_exit() failed: %s\n", diagnosis);
			return 1;
		}
	}
	break;

case ST_SUBMIT_IN_HOLD_RELEASE:
case ST_SUBMIT_IN_HOLD_DELETE:
	{
		const char *session_all[] = { DRMAA_JOB_IDS_SESSION_ALL, NULL };
		char jobid[1024];
		int stat;

		if (parse_args)
			sleeper_job = NEXT_ARGV(argc, argv);

		if (drmaa_init(NULL, diagnosis, sizeof(diagnosis)-1) 
			!= DRMAA_ERRNO_SUCCESS) {
				fprintf(stderr, "drmaa_init() failed: %s\n", diagnosis);
				return 1;
		}
		report_session_key();
		if (!(jt = create_sleeper_job_template(5, 0, 1))) {
			fprintf(stderr, "create_sleeper_job_template() failed\n");
			return 1;
		}
		if (drmaa_run_job(jobid, sizeof(jobid)-1, jt, diagnosis, 
			sizeof(diagnosis)-1)!=DRMAA_ERRNO_SUCCESS) {
				fprintf(stderr, "drmaa_run_job() failed: %s\n", diagnosis);
				return 1;
		}
		drmaa_delete_job_template(jt, NULL, 0);
		printf("submitted job in hold state \"%s\"\n", jobid);

		if (!check_job_state(jobid, DRMAA_PS_USER_ON_HOLD) &&
			!check_job_state(jobid, DRMAA_PS_USER_SYSTEM_ON_HOLD))
			return 1;

		printf("verified user hold state for job \"%s\"\n", jobid);

		if (test_case == ST_SUBMIT_IN_HOLD_RELEASE) {
			int inresult = drmaa_control(jobid, DRMAA_CONTROL_RELEASE, diagnosis, sizeof(diagnosis)-1);
			if (inresult != DRMAA_ERRNO_SUCCESS) {
				fprintf(stderr, "drmaa_control(%s, DRMAA_CONTROL_RELEASE) failed: %s (%u)\n", 
					jobid, diagnosis, inresult);
				return 1;
			}
			printf("released user hold state for job \"%s\"\n", jobid);
		} else {
			if (drmaa_control(jobid, DRMAA_CONTROL_TERMINATE, diagnosis, sizeof(diagnosis)-1)!=DRMAA_ERRNO_SUCCESS) {
				fprintf(stderr, "drmaa_control(%s, DRMAA_CONTROL_TERMINATE) failed: %s\n", 
					jobid, diagnosis);
				return 1;
			}
			printf("terminated job in hold state \"%s\"\n", jobid);
		}

		// synchronization for terminated jobs can not be expected to work - not demanded by the spec
		if (test_case == ST_SUBMIT_IN_HOLD_RELEASE) {
			/* synchronize with job to finish but do not dispose job finish information */
			if ((drmaa_errno = drmaa_synchronize(session_all, max_wait_timeout, 0, 
				diagnosis, sizeof(diagnosis)-1))!=DRMAA_ERRNO_SUCCESS) {
					fprintf(stderr, "drmaa_synchronize(DRMAA_JOB_IDS_SESSION_ALL, dispose) failed: %s\n", diagnosis);
					return 1;
			}
			printf("synchronized with job finish\n");
			if (!check_job_state(jobid, DRMAA_PS_DONE)) return 1;
			if (wait_n_jobs(1) != DRMAA_ERRNO_SUCCESS)  return 1;
		} else {
			if (!check_job_state(jobid, DRMAA_PS_FAILED)) return 1;
			// Jobs that haven't been run must be marked as aborted (see GFD.133)
			if ((drmaa_errno=drmaa_wait(jobid, NULL, 0, &stat, max_wait_timeout, NULL, diagnosis, sizeof(diagnosis)-1))!=DRMAA_ERRNO_SUCCESS) {
                		fprintf(stderr, "drmaa_wait(\"%s\") failed: %s\n", jobid, diagnosis);
                		return false;
        		}
			if (!check_term_details(stat, 1, 0, 0)) return 1;
		}

		if (drmaa_exit(diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
			fprintf(stderr, "drmaa_exit() failed: %s\n", diagnosis);
			return 1;
		}
	}
	break;

case ST_BULK_SUBMIT_IN_HOLD_SESSION_RELEASE:
case ST_BULK_SUBMIT_IN_HOLD_SINGLE_RELEASE:
case ST_BULK_SUBMIT_IN_HOLD_SESSION_DELETE:
case ST_BULK_SUBMIT_IN_HOLD_SINGLE_DELETE:
	{
		const char *session_all[] = { DRMAA_JOB_IDS_SESSION_ALL, NULL };
		const char *all_jobids[JOB_CHUNK+1];
		int pos = 0; 
		int ctrl_op;

		if (parse_args)
			sleeper_job = NEXT_ARGV(argc, argv);

		if (drmaa_init(NULL, diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
			fprintf(stderr, "drmaa_init() failed: %s\n", diagnosis);
			return 1;
		}
		report_session_key();

		if (!(jt = create_sleeper_job_template(5, 1, 1))) {
			fprintf(stderr, "create_sleeper_job_template() failed\n");
			return 1;
		}

		/* 
		* Submit a bulk job in hold and verify state using drmaa_job_ps() 
		*/
		{
			drmaa_job_ids_t *jobids;
			int j;
			if ((drmaa_errno=drmaa_run_bulk_jobs(&jobids, jt, 1, JOB_CHUNK, 1, diagnosis, sizeof(diagnosis)-1))!=DRMAA_ERRNO_SUCCESS) {
				printf("failed submitting bulk job (%s): %s\n", drmaa_strerror(drmaa_errno), diagnosis);
				return 1;
			} 
			printf("submitted bulk job with jobids:\n");
			for (j=0; j<JOB_CHUNK; j++) {
				char jobid[100];
				drmaa_get_next_job_id(jobids, jobid, sizeof(jobid)-1);
				printf("\t \"%s\"\n", jobid);

				/* copy jobid into jobid array */
				all_jobids[pos++] = strdup(jobid);

				if (!check_job_state(jobid, DRMAA_PS_USER_ON_HOLD) &&
					!check_job_state(jobid, DRMAA_PS_USER_SYSTEM_ON_HOLD))
					return 1;
			} 
			drmaa_release_job_ids(jobids);
		}
		drmaa_delete_job_template(jt, NULL, 0);

		printf("verified user hold state for bulk job\n");

		/*
		* Release or terminate all jobs using drmaa_control() depending on the test case 
		* drmaa_control() is applied muliple times on all tasks or on the whole session
		*/
		if (test_case == ST_BULK_SUBMIT_IN_HOLD_SINGLE_RELEASE || 
			test_case == ST_BULK_SUBMIT_IN_HOLD_SINGLE_DELETE) {
				int ctrl_op;
				if (test_case == ST_BULK_SUBMIT_IN_HOLD_SINGLE_RELEASE)
					ctrl_op = DRMAA_CONTROL_RELEASE;
				else
					ctrl_op = DRMAA_CONTROL_TERMINATE;

				for (pos=0; pos<JOB_CHUNK; pos++) {
					if (drmaa_control(all_jobids[pos], ctrl_op, diagnosis, 
						sizeof(diagnosis)-1)!=DRMAA_ERRNO_SUCCESS) {
							fprintf(stderr, "drmaa_control(%s, %s) failed: %s\n", 
								all_jobids[pos], drmaa_ctrl2str(ctrl_op), diagnosis);
							return 1;
					}
				}
		} else {
			if (test_case == ST_BULK_SUBMIT_IN_HOLD_SESSION_RELEASE)
				ctrl_op = DRMAA_CONTROL_RELEASE;
			else
				ctrl_op = DRMAA_CONTROL_TERMINATE;

			if (drmaa_control(DRMAA_JOB_IDS_SESSION_ALL, ctrl_op, diagnosis, 
				sizeof(diagnosis)-1)!=DRMAA_ERRNO_SUCCESS) {
					fprintf(stderr, "drmaa_control(%s, %s) failed: %s\n", 
						DRMAA_JOB_IDS_SESSION_ALL, drmaa_ctrl2str(ctrl_op), diagnosis);
					return 1;
			}
		}
		printf("released/terminated all jobs\n");

		/* synchronize with job to finish but do not dispose job finish information */
		// for terminated jobs, DRMAA does not expect jobs to be synchronizable
		if (test_case == ST_BULK_SUBMIT_IN_HOLD_SESSION_RELEASE ||  test_case == ST_BULK_SUBMIT_IN_HOLD_SINGLE_RELEASE) {
			if ((drmaa_errno = drmaa_synchronize(session_all, max_wait_timeout, 0, 
				diagnosis, sizeof(diagnosis)-1))!=DRMAA_ERRNO_SUCCESS) {
					fprintf(stderr, "drmaa_synchronize(DRMAA_JOB_IDS_SESSION_ALL, dispose) failed: %s\n", diagnosis);
					return 1;
			}
			printf("synchronized with job finish\n");
		}

		/* 
		* Verify job state of all jobs in the job id array 
		*/
		for (pos=0; pos<JOB_CHUNK; pos++) {
			if (test_case == ST_BULK_SUBMIT_IN_HOLD_SINGLE_RELEASE ||
				test_case == ST_BULK_SUBMIT_IN_HOLD_SESSION_RELEASE)
			{
				if (!check_job_state(all_jobids[pos], DRMAA_PS_DONE)) return 1;
			}
			if (test_case == ST_BULK_SUBMIT_IN_HOLD_SINGLE_DELETE ||
				test_case == ST_BULK_SUBMIT_IN_HOLD_SESSION_DELETE)
			{
				if (!check_job_state(all_jobids[pos], DRMAA_PS_FAILED)) return 1;
			}
		}

		if (wait_n_jobs(JOB_CHUNK) != DRMAA_ERRNO_SUCCESS) {
			return 1;
		}
		if (drmaa_exit(diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
			fprintf(stderr, "drmaa_exit() failed: %s\n", diagnosis);
			return 1;
		}
	}
	break;

case ST_SUBMIT_SUSPEND_RESUME_WAIT:
	{
		int job_state, stat, exited, exit_status;
		char jobid[1024];

		if (parse_args)
			sleeper_job = NEXT_ARGV(argc, argv);

		if (drmaa_init(NULL, diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
			fprintf(stderr, "drmaa_init() failed: %s\n", diagnosis);
			return 1;
		}

		/* submit a job running long enough allowing it to be suspended and resumed */
		if (!(jt = create_sleeper_job_template(30, 0, 0))) {
			fprintf(stderr, "create_sleeper_job_template() failed\n");
			return 1;
		}
		if (drmaa_run_job(jobid, sizeof(jobid)-1, jt, diagnosis, sizeof(diagnosis)-1)!=DRMAA_ERRNO_SUCCESS) {
			fprintf(stderr, "drmaa_run_job() failed: %s\n", diagnosis);
			return 1;
		}
		printf("submitted job \"%s\"\n", jobid);
		drmaa_delete_job_template(jt, NULL, 0);

		/* wait until job is running */
		do {
			if (drmaa_job_ps(jobid, &job_state, diagnosis, sizeof(diagnosis)-1)!=DRMAA_ERRNO_SUCCESS) {
				fprintf(stderr, "drmaa_job_ps() failed: %s\n", diagnosis);
				return 1;
			}
			if (job_state != DRMAA_PS_RUNNING) {
				fprintf(stderr, "Waiting forever to get job state DRMAA_PS_RUNNING ...\n");
				sleep(5);
			}
		} while (job_state != DRMAA_PS_RUNNING);
		printf("job \"%s\" is now running\n", jobid);

		/* drmaa_control() is used to suspend the job */
		if ((drmaa_errno=drmaa_control(jobid, DRMAA_CONTROL_SUSPEND, diagnosis, 
			sizeof(diagnosis)-1))!=DRMAA_ERRNO_SUCCESS) {
				fprintf(stderr, "drmaa_control(\"%s\", DRMAA_CONTROL_SUSPEND) failed: %s (%s)\n", 
					jobid, diagnosis, drmaa_strerror(drmaa_errno));
				return 1;
		}
		printf("suspended job \"%s\"\n", jobid);

		/* drmaa_job_ps() is used to verify job was suspended */
		if (!check_job_state(jobid, DRMAA_PS_USER_SUSPENDED))
			return 1;
		printf("verified suspend was done for job \"%s\"\n", jobid);

		/* drmaa_control() is used to resume the job */
		if ((drmaa_errno=drmaa_control(jobid, DRMAA_CONTROL_RESUME, diagnosis, 
			sizeof(diagnosis)-1))!=DRMAA_ERRNO_SUCCESS) {
				fprintf(stderr, "drmaa_control(\"%s\", DRMAA_CONTROL_RESUME) failed: %s (%s)\n", 
					jobid, diagnosis, drmaa_strerror(drmaa_errno));
				return 1;
		}
		printf("resumed job \"%s\"\n", jobid);

		/* drmaa_job_ps() is used to verify job was resumed */
		// PS_FAILED is valid after resume according to DRMAA, even if the DRM might not be happy about that
		if (!check_job_state(jobid, DRMAA_PS_RUNNING) &&
			!check_job_state(jobid, DRMAA_PS_FAILED))
			return 1;

		printf("verified resume was done for job \"%s\"\n", jobid);

		/* drmaa_wait() is used to wait for the jobs regular end */
		if ((drmaa_errno=drmaa_wait(jobid, NULL, 0, &stat, 
			max_wait_timeout, NULL, diagnosis, sizeof(diagnosis)-1))!=DRMAA_ERRNO_SUCCESS) {
				fprintf(stderr, "drmaa_wait(\"%s\") failed: %s\n", jobid, diagnosis);
				return 1;
		}

		drmaa_wifexited(&exited, stat, NULL, 0); 
		if (!exited || (drmaa_wexitstatus(&exit_status, stat, NULL, 0), exit_status != 0)) {
			report_wrong_job_finish("expected regular job end", jobid, stat);
			return 1;
		}
		printf("job \"%s\" finished as expected\n", jobid);

		if (drmaa_exit(diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
			fprintf(stderr, "drmaa_exit() failed: %s\n", diagnosis);
			return 1;
		}
	}
	break;

case ST_SUBMIT_KILL_SIG:
	{
		int stat, signaled;
		char jobid[1024];
		const char *job_argv[2];
		drmaa_job_template_t *jt = NULL;
		char buffer[100];
		char sigName[1024];
		int ret = DRMAA_ERRNO_SUCCESS;
		int was_dumped;

#ifndef WIN32
		const struct test_signals {
			char* name;
			int number;
		} test_signals_map[] = {
			{"SIGUSR1", SIGUSR1},   
			{"SIGTERM", SIGTERM},
			{"SIGALRM", SIGALRM},
			{"SIGUSR2", SIGUSR2},
			{"SIGSEGV", SIGSEGV},
			{"SIGHUP",  SIGHUP},
			{"SIGQUIT", SIGQUIT},
			{"SIGILL", SIGILL},
			{"SIGABRT", SIGABRT},
			{"SIGFPE", SIGFPE},
			{"SIGKILL", SIGKILL}
		};
		int test_signals_count = 11;
#else
		const struct test_signals {
			char* name;
			int number;
		} *test_signals_map = NULL;
		int test_signals_count = 0;
#endif

#ifdef WIN32
		printf("skipping test ST_SUBMIT_KILL_SIG on Windows\n");
		break;
#endif

		if (parse_args)
			kill_job = NEXT_ARGV(argc, argv);

		if (drmaa_init(NULL, diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
			fprintf(stderr, "drmaa_init() failed: %s\n", diagnosis);
			return 1;
		}

		// create job template
		ret = drmaa_allocate_job_template(&jt, NULL, 0);
		if (ret == DRMAA_ERRNO_SUCCESS) {
			ret = drmaa_set_attribute(jt, DRMAA_WD, DRMAA_PLACEHOLDER_HD, NULL, 0);
		}
		if (ret == DRMAA_ERRNO_SUCCESS) {
			ret = drmaa_set_attribute(jt, DRMAA_REMOTE_COMMAND, kill_job, NULL, 0);
		} 
		if (ret == DRMAA_ERRNO_SUCCESS) {
			ret = drmaa_set_attribute(jt, DRMAA_OUTPUT_PATH, ":/dev/null", NULL, 0);
		}

		for(i=0; i < test_signals_count; i++) {
			printf("Testing with %s\n", test_signals_map[i].name);
			if (ret == DRMAA_ERRNO_SUCCESS) {
				sprintf(buffer, "%d", test_signals_map[i].number);
				job_argv[0] = buffer; 
				job_argv[1] = NULL;
				ret = drmaa_set_vector_attribute(jt, DRMAA_V_ARGV, job_argv, NULL, 0);
			}

			// start job
			if (drmaa_run_job(jobid, sizeof(jobid)-1, jt, diagnosis, sizeof(diagnosis)-1)!=DRMAA_ERRNO_SUCCESS) {
				fprintf(stderr, "drmaa_run_job() failed: %s\n", diagnosis);
				return 1;
			}
			printf("submitted job \"%s\"\n", jobid);

			/* drmaa_wait() is used to wait for the jobs regular end */
			if ((drmaa_errno=drmaa_wait(jobid, NULL, 0, &stat, 
				max_wait_timeout, NULL, diagnosis, sizeof(diagnosis)-1))!=DRMAA_ERRNO_SUCCESS) {
					fprintf(stderr, "drmaa_wait(\"%s\") failed: %s\n", jobid, diagnosis);
					return 1;
			}

			// check that job is marked as signalled
			if (!check_term_details(stat, 0, 0, 1)) return 1;

			// Determine that the signal was the right one
			if (drmaa_wtermsig(sigName, sizeof(sigName)-1, stat, diagnosis, sizeof(diagnosis)-1) == DRMAA_ERRNO_SUCCESS) {
				if (strcmp(sigName, test_signals_map[i].name) != 0) {
					fprintf(stderr, "Reported signal name is %s, expected %s.\n", sigName, test_signals_map[i].name);
					return 1;
				}
			}
			else {
				fprintf(stderr, "drmaa_wtermsig() failed: %s\n", diagnosis);
				return 1;
			}

			printf("job \"%s\" was killed with %s, as expected", 
				jobid, sigName);

			// check for core dump
			// we can not rely on the existence of core files,
			// therefore no dedicated test is possible
			was_dumped=0;
			if (DRMAA_ERRNO_SUCCESS == 
				drmaa_wcoredump (&was_dumped, stat, diagnosis, sizeof(diagnosis)-1))
			{
				if (was_dumped)
					printf(" (with core dump file)\n");
				else
					printf(" (without core dump file)\n");
			} else {
				fprintf(stderr, "drmaa_wcoredump() failed: %s\n", diagnosis);
				return 1;
			} 

		}

		if (drmaa_exit(diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
			fprintf(stderr, "drmaa_exit() failed: %s\n", diagnosis);
			return 1;
		}
	}
	break;

case ST_EMPTY_SESSION_WAIT:
case ST_EMPTY_SESSION_SYNCHRONIZE_DISPOSE:
case ST_EMPTY_SESSION_SYNCHRONIZE_NODISPOSE:
	{
		if (drmaa_init(NULL, diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
			fprintf(stderr, "drmaa_init() failed: %s\n", diagnosis);
			return 1;
		}

		switch (test_case) {
case ST_EMPTY_SESSION_WAIT:
	/* drmaa_wait() must return DRMAA_ERRNO_INVALID_JOB */
	if ((drmaa_errno=drmaa_wait(DRMAA_JOB_IDS_SESSION_ANY, NULL, 0, NULL, 
		max_wait_timeout, NULL, diagnosis, sizeof(diagnosis)-1))!=DRMAA_ERRNO_INVALID_JOB) {
			fprintf(stderr, "drmaa_wait(empty session) failed: %s\n", diagnosis);
			return 1;
	}
	break;
case ST_EMPTY_SESSION_SYNCHRONIZE_DISPOSE:
case ST_EMPTY_SESSION_SYNCHRONIZE_NODISPOSE:
	{
		const char *session_all[] = { DRMAA_JOB_IDS_SESSION_ALL, NULL };
		/* drmaa_synchronize(DRMAA_JOB_IDS_SESSION_ALL) must return DRMAA_ERRNO_SUCCESS */
		if ((drmaa_errno=drmaa_synchronize(session_all, max_wait_timeout, 
			(test_case == ST_EMPTY_SESSION_SYNCHRONIZE_DISPOSE) ? 1 : 0, 
			diagnosis, sizeof(diagnosis)-1))!=DRMAA_ERRNO_SUCCESS) {
				fprintf(stderr, "drmaa_synchronize(empty session) failed: %s\n", diagnosis);
				return 1;
		}
	}
	break;
		}

		if (drmaa_exit(diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
			fprintf(stderr, "drmaa_exit() failed: %s\n", diagnosis);
			return 1;
		}
	}
	break;
case ST_GET_NUM_JOBIDS:
	{
		if (parse_args)
			sleeper_job = NEXT_ARGV(argc, argv);

		if (drmaa_init(NULL, diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
			fprintf(stderr, "drmaa_init() failed: %s\n", diagnosis);
			return 1;
		}
		report_session_key();
		if (!(jt = create_sleeper_job_template(5, 1, 0))) {
			fprintf(stderr, "create_sleeper_job_template() failed\n");
			return 1;
		}
		for (i=0; i<NBULKS; i++) {
			drmaa_job_ids_t *jobids;
			char jobid[100];
			int j;
			size_t size;
			if ((drmaa_errno=drmaa_run_bulk_jobs(&jobids, jt, 1, JOB_CHUNK, 1, diagnosis, sizeof(diagnosis)-1))!=DRMAA_ERRNO_SUCCESS) {
				printf("failed submitting bulk job (%s): %s\n", drmaa_strerror(drmaa_errno), diagnosis);
				return 1;
			} 
			printf("submitted %u bulk job with jobids:\n", JOB_CHUNK);
			if (drmaa_get_num_job_ids(jobids, &size) != DRMAA_ERRNO_SUCCESS) {
				fprintf(stderr, "drmaa_get_num_job_ids() failed");
				return 1;
			}
			if (size != JOB_CHUNK) {
				fprintf(stderr, "drmaa_get_num_job_ids() returned %lu, expected %u\n", (unsigned long)size, JOB_CHUNK);
				return 1;
			}
			for (j=0; j<JOB_CHUNK; j++) {
				drmaa_get_next_job_id(jobids, jobid, sizeof(jobid)-1);
				printf("\t \"%s\"\n", jobid);
			} 
			drmaa_release_job_ids(jobids);
		}
		drmaa_delete_job_template(jt, NULL, 0);

		if (wait_n_jobs(JOB_CHUNK*NBULKS) != DRMAA_ERRNO_SUCCESS) {
			return 1;
		}
		if (drmaa_exit(diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
			fprintf(stderr, "drmaa_exit() failed: %s\n", diagnosis);
			return 1;
		}

	}
	break;

case ST_EMPTY_SESSION_CONTROL:
	{
		const char *s;

		if (drmaa_init(NULL, diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
			fprintf(stderr, "drmaa_init() failed: %s\n", diagnosis);
			return 1;
		}

		/* parse control operation */
		if (parse_args) {
			s = NEXT_ARGV(argc, argv);
			if ((ctrl_op = str2drmaa_ctrl(s)) == -1) {
				fprintf(stderr, "unknown DRMAA control operation \"%s\"\n", s);
				usage();
			}
		}

		// the spec makes no assumption on ehat happens when the session is empty
		if ((drmaa_errno=drmaa_control(DRMAA_JOB_IDS_SESSION_ALL, ctrl_op,
			diagnosis, sizeof(diagnosis)-1))!=DRMAA_ERRNO_SUCCESS) {
				fprintf(stderr, "drmaa_control(empty_session, %s) failed: %s (%s)\n", 
					drmaa_ctrl2str(ctrl_op), diagnosis, drmaa_strerror(drmaa_errno));
				return 1;
		}

		if (drmaa_exit(diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
			fprintf(stderr, "drmaa_exit() failed: %s\n", diagnosis);
			return 1;
		}
	}
	break;

case ST_EXIT_STATUS:
	/* - drmaa_init() is called
	- job are submitted
	- job i returns i as exit status (8 bit)
	- drmaa_wait() verifies each job returned the
	correct exit status
	- then drmaa_exit() is called */
	{
		char diagnosis[1024];
		char *all_jobids[256];
		const char *job_argv[2];
		char jobid[1024];
		char buffer[100];

		if (parse_args)
			exit_job = NEXT_ARGV(argc, argv);

		if (drmaa_init(NULL, diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
			fprintf(stderr, "drmaa_init() failed: %s\n", diagnosis);
			return 1;
		}
		report_session_key();

		/*
		*   submit sequential jobs
		*/
		if (!(jt = create_exit_job_template(exit_job, 0))) {
			fprintf(stderr, "create_exit_job_template() failed\n");
			return 1;
		}
		// test only from 0-125, since larger exit values are not freely available for apps
		// check "man 1posix exit"
		for (i=0; i<126; i++) {

			/* parametrize exit job with job argument */
			sprintf(buffer, "%d", i);
			job_argv[0] = buffer; 
			job_argv[1] = NULL;
			if (DRMAA_ERRNO_SUCCESS != drmaa_set_vector_attribute(jt, 
				DRMAA_V_ARGV, job_argv, diagnosis, sizeof(diagnosis)-1)) {
					fprintf(stderr, "drmaa_set_vector_attribute() failed: %s\n", diagnosis);
					return 1;
			}

			while ((drmaa_errno=drmaa_run_job(jobid, sizeof(jobid)-1, jt, diagnosis,
				sizeof(diagnosis)-1)) == DRMAA_ERRNO_DRM_COMMUNICATION_FAILURE) {
					fprintf(stderr, "drmaa_run_job() failed - retry: %s\n", diagnosis);
					sleep(1);
			}
			if (drmaa_errno != DRMAA_ERRNO_SUCCESS) {
				fprintf(stderr, "drmaa_run_job() failed: %s\n", diagnosis);
				return 1;
			}
			printf("\t \"%s (%s)\"\n", jobid, buffer);
			all_jobids[i] = strdup(jobid);
		}

		/* set string array end mark */
		all_jobids[i] = NULL;

		drmaa_delete_job_template(jt, NULL, 0);

		/*
		*   wait for all jobs and verify exit status
		*/

		for (i=0; i<126; i++) {
			int stat = 0;
			int exit_status = 0;
			int exited = 0;

			do {
				drmaa_errno = drmaa_wait(all_jobids[i], jobid, sizeof(jobid)-1, 
					&stat, max_wait_timeout, NULL, diagnosis, sizeof(diagnosis)-1);
				if (drmaa_errno != DRMAA_ERRNO_SUCCESS) {
					fprintf(stderr, "drmaa_wait(%s) failed - retry: %s\n", all_jobids[i], diagnosis); 
					sleep(1);
				}
			} while (drmaa_errno == DRMAA_ERRNO_DRM_COMMUNICATION_FAILURE);

			printf("job %d with job id %s finished\n", i, all_jobids[i]);
			if (drmaa_errno != DRMAA_ERRNO_SUCCESS) {
				fprintf(stderr, "drmaa_wait(%s) failed: %s\n", all_jobids[i], diagnosis);
				return 1;
			}

			drmaa_wifexited(&exited, stat, NULL, 0);
			if (!exited) {
				fprintf(stderr, "job \"%s\" did not exit cleanly\n", all_jobids[i]);
				return 1;
			}
			drmaa_wexitstatus(&exit_status, stat, NULL, 0);
			if (exit_status != i) {
				fprintf(stderr, "job \"%s\" returned wrong exit status %d instead of %d\n", 
					all_jobids[i], exit_status, i);
				return 1;
			}

		}

		printf("waited all jobs\n");
		if (drmaa_exit(diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
			fprintf(stderr, "drmaa_exit() failed: %s\n", diagnosis);
			return 1;
		}
		break;
	}

case ST_ATTRIBUTE_CHANGE:
	{         
		const char *job_argv[2];
		const char *job_env[2];
		const char *email_addr[2];
		char stringval[100];
		drmaa_attr_values_t *vbuffer = NULL;


		if (drmaa_init(NULL, diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
			fprintf(stderr, "drmaa_init() failed: %s\n", diagnosis);
			return 1;
		}
		report_session_key();

		printf ("Testing change of job template attributes\n");
		printf ("Getting job template\n");

		drmaa_allocate_job_template(&jt, NULL, 0);

		if (jt == NULL) {
			fprintf(stderr, "drmaa_allocate_job_template() failed\n");
			return 1;
		}

		printf ("Filling job template for the first time\n");
		job_argv[0] = "argv1"; 
		job_argv[1] = NULL;
		job_env[0] = "env1"; 
		job_env[1] = NULL;
		email_addr[0] = "email1"; 
		email_addr[1] = NULL;
		drmaa_set_attribute(jt, DRMAA_REMOTE_COMMAND, "job1", NULL, 0);         
		drmaa_set_vector_attribute(jt, DRMAA_V_ARGV, job_argv, NULL, 0);
		drmaa_set_attribute(jt, DRMAA_JS_STATE, "drmaa_hold", NULL, 0);         
		drmaa_set_vector_attribute(jt, DRMAA_V_ENV, job_env, NULL, 0);
		drmaa_set_attribute(jt, DRMAA_WD, "/tmp1", NULL, 0);         
		drmaa_set_attribute(jt, DRMAA_JOB_CATEGORY, "category1", NULL, 0);         
		drmaa_set_attribute(jt, DRMAA_NATIVE_SPECIFICATION, "native1", NULL, 0);         
		drmaa_set_vector_attribute(jt, DRMAA_V_EMAIL, email_addr, NULL, 0);
		drmaa_set_attribute(jt, DRMAA_BLOCK_EMAIL, "1", NULL, 0);         
		drmaa_set_attribute(jt, DRMAA_START_TIME, "11:11", NULL, 0);         
		drmaa_set_attribute(jt, DRMAA_JOB_NAME, "jobname1", NULL, 0);         
		drmaa_set_attribute(jt, DRMAA_INPUT_PATH, ":/dev/null1", NULL, 0);         
		drmaa_set_attribute(jt, DRMAA_OUTPUT_PATH, ":/dev/null1", NULL, 0);         
		drmaa_set_attribute(jt, DRMAA_ERROR_PATH, ":/dev/null1", NULL, 0);         
		drmaa_set_attribute(jt, DRMAA_JOIN_FILES, "y", NULL, 0);         

		printf("Filling job template for the second time\n");
		job_argv[0] = "2"; 
		job_argv[1] = NULL;
		job_env[0] = "env2"; 
		job_env[1] = NULL;
		email_addr[0] = "email2"; 
		email_addr[1] = NULL;
		drmaa_set_attribute(jt, DRMAA_REMOTE_COMMAND, "job2", NULL, 0);         
		drmaa_set_vector_attribute(jt, DRMAA_V_ARGV, job_argv, NULL, 0);
		drmaa_set_attribute(jt, DRMAA_JS_STATE, "drmaa_active", NULL, 0);         
		drmaa_set_vector_attribute(jt, DRMAA_V_ENV, job_env, NULL, 0);
		drmaa_set_attribute(jt, DRMAA_WD, "/tmp2", NULL, 0);         
		drmaa_set_attribute(jt, DRMAA_JOB_CATEGORY, "category2", NULL, 0);         
		drmaa_set_attribute(jt, DRMAA_NATIVE_SPECIFICATION, "native2", NULL, 0);         
		drmaa_set_vector_attribute(jt, DRMAA_V_EMAIL, email_addr, NULL, 0);
		drmaa_set_attribute(jt, DRMAA_BLOCK_EMAIL, "0", NULL, 0);         
		drmaa_set_attribute(jt, DRMAA_START_TIME, "11:22", NULL, 0);         
		drmaa_set_attribute(jt, DRMAA_JOB_NAME, "jobname2", NULL, 0);         
		drmaa_set_attribute(jt, DRMAA_INPUT_PATH, ":/dev/null2", NULL, 0);         
		drmaa_set_attribute(jt, DRMAA_OUTPUT_PATH, ":/dev/null2", NULL, 0);         
		drmaa_set_attribute(jt, DRMAA_ERROR_PATH, ":/dev/null2", NULL, 0);         
		drmaa_set_attribute(jt, DRMAA_JOIN_FILES, "n", NULL, 0);         

		printf("Checking current values of job template\n");

		if(drmaa_get_vector_attribute(jt, DRMAA_V_ARGV, &vbuffer, diagnosis, sizeof(diagnosis)-1)
			!= DRMAA_ERRNO_SUCCESS) {
				fprintf(stderr, "drmaa_get_vector_attribute() failed: %s\n", diagnosis);
				return 1;
		} else {
			size_t size;
			if (drmaa_get_num_attr_values(vbuffer, &size) != DRMAA_ERRNO_SUCCESS) {
				fprintf(stderr, "drmaa_get_num_attr_values() failed\n");
				return 1;
			}
			if (size != 1) {
				fprintf(stderr, "drmaa_get_num_attr_values() returned %lu, expected 2\n",(unsigned long)size);
				return 1;
			}

			if (drmaa_get_next_attr_value(vbuffer, stringval, sizeof(stringval)-1) != DRMAA_ERRNO_SUCCESS) {
				fprintf(stderr, "drmaa_get_vector_attribute() failed: %s\n", diagnosis);
				return 1;
			} else {
				if (strcmp(stringval, "2") != 0) {
					fprintf(stderr, "Incorrect value after change for DRMAA_V_ARGV attribute\n");
					return 1;
				}
			}
		}

		if(drmaa_get_vector_attribute(jt, DRMAA_V_ENV, &vbuffer, diagnosis, sizeof(diagnosis)-1)
			!= DRMAA_ERRNO_SUCCESS) {
				fprintf(stderr, "drmaa_get_vector_attribute() failed: %s\n", diagnosis);
				return 1;
		} else {
			if (drmaa_get_next_attr_value(vbuffer, stringval, sizeof(stringval)-1) != DRMAA_ERRNO_SUCCESS) {
				fprintf(stderr, "drmaa_get_vector_attribute() failed: %s\n", diagnosis);
				return 1;
			} else {
				if (strcmp(stringval, "env2") != 0) {
					fprintf(stderr, "Incorrect value after change for DRMAA_V_ENV attribute\n");
					return 1;
				}
			}
		}

		if(drmaa_get_vector_attribute(jt, DRMAA_V_EMAIL, &vbuffer, diagnosis, sizeof(diagnosis)-1)
			!= DRMAA_ERRNO_SUCCESS) {
				fprintf(stderr, "drmaa_get_vector_attribute() failed: %s\n", diagnosis);
				return 1;
		} else {
			if (drmaa_get_next_attr_value(vbuffer, stringval, sizeof(stringval)-1) != DRMAA_ERRNO_SUCCESS) {
				fprintf(stderr, "drmaa_get_vector_attribute() failed: %s\n", diagnosis);
				return 1;
			} else {
				if (strcmp(stringval, "email2") != 0) {
					fprintf(stderr, "Incorrect value after change for DRMAA_V_EMAIL attribute\n");
					return 1;
				}
			}
		}

		if (drmaa_get_attribute(jt, DRMAA_REMOTE_COMMAND, stringval, 
			sizeof(stringval)-1, diagnosis, 
			sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
				fprintf(stderr, "drmaa_get_attribute() failed: %s\n", diagnosis);
				return 1;
		} else {
			if (strcmp(stringval, "job2") != 0) {
				fprintf(stderr, "Incorrect value after change for DRMAA_REMOTE_COMMAND attribute\n");
				return 1;
			}}

		if (drmaa_get_attribute(jt, DRMAA_JS_STATE, stringval, 
			sizeof(stringval)-1, diagnosis, 
			sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
				fprintf(stderr, "drmaa_get_attribute() failed: %s\n", diagnosis);
				return 1;
		} else {
			if (strcmp(stringval, "drmaa_active") != 0) {
				fprintf(stderr, "Incorrect value after change for DRMAA_JS_STATE attribute\n");
				return 1;
			}}

		if (drmaa_get_attribute(jt, DRMAA_WD, stringval, 
			sizeof(stringval)-1, diagnosis, 
			sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
				fprintf(stderr, "drmaa_get_attribute() failed: %s\n", diagnosis);
				return 1;
		} else {
			if (strcmp(stringval, "/tmp2") != 0) {
				fprintf(stderr, "Incorrect value after change for DRMAA_WD attribute\n");
				return 1;
			}}

		if (drmaa_get_attribute(jt, DRMAA_JOB_CATEGORY, stringval, 
			sizeof(stringval)-1, diagnosis, 
			sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
				fprintf(stderr, "drmaa_get_attribute() failed: %s\n", diagnosis);
				return 1;
		} else {
			if (strcmp(stringval, "category2") != 0) {
				fprintf(stderr, "Incorrect value after change for DRMAA_JOB_CATEGORY attribute\n");
				return 1;
			}}

		if (drmaa_get_attribute(jt, DRMAA_NATIVE_SPECIFICATION, stringval, 
			sizeof(stringval)-1, diagnosis, 
			sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
				fprintf(stderr, "drmaa_get_attribute() failed: %s\n", diagnosis);
				return 1;
		} else {
			if (strcmp(stringval, "native2") != 0) {
				fprintf(stderr, "Incorrect value after change for DRMAA_NATIVE_SPECIFICATION attribute\n");
				return 1;
			}}

		if (drmaa_get_attribute(jt, DRMAA_BLOCK_EMAIL, stringval, 
			sizeof(stringval)-1, diagnosis, 
			sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
				fprintf(stderr, "drmaa_get_attribute() failed: %s\n", diagnosis);
				return 1;
		} else {
			if (strcmp(stringval, "0") != 0) {
				fprintf(stderr, "Incorrect value after change for DRMAA_BLOCK_EMAIL attribute\n");
				return 1;
			}}

		if (drmaa_get_attribute(jt, DRMAA_START_TIME, stringval, 
			sizeof(stringval)-1, diagnosis, 
			sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
				fprintf(stderr, "drmaa_get_attribute() failed: %s\n", diagnosis);
				return 1;
		} else {
			if (strcmp(stringval, "11:22") != 0) {
				fprintf(stderr, "Incorrect value after change for DRMAA_START_TIME attribute\n");
				return 1;
			}}

		if (drmaa_get_attribute(jt, DRMAA_JOB_NAME, stringval, 
			sizeof(stringval)-1, diagnosis, 
			sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
				fprintf(stderr, "drmaa_get_attribute() failed: %s\n", diagnosis);
				return 1;
		} else {
			if (strcmp(stringval, "jobname2") != 0) {
				fprintf(stderr, "Incorrect value after change for DRMAA_JOB_NAME attribute\n");
				return 1;
			}}

		if (drmaa_get_attribute(jt, DRMAA_INPUT_PATH, stringval, 
			sizeof(stringval)-1, diagnosis, 
			sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
				fprintf(stderr, "drmaa_get_attribute() failed: %s\n", diagnosis);
				return 1;
		} else {
			if (strcmp(stringval, ":/dev/null2") != 0) {
				fprintf(stderr, "Incorrect value after change for DRMAA_INPUT_PATH attribute\n");
				return 1;
			}}

		if (drmaa_get_attribute(jt, DRMAA_OUTPUT_PATH, stringval, 
			sizeof(stringval)-1, diagnosis, 
			sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
				fprintf(stderr, "drmaa_get_attribute() failed: %s\n", diagnosis);
				return 1;
		} else {
			if (strcmp(stringval, ":/dev/null2") != 0) {
				fprintf(stderr, "Incorrect value after change for DRMAA_OUTPUT_PATH attribute\n");
				return 1;
			}}

		if (drmaa_get_attribute(jt, DRMAA_ERROR_PATH, stringval, 
			sizeof(stringval)-1, diagnosis, 
			sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
				fprintf(stderr, "drmaa_get_attribute() failed: %s\n", diagnosis);
				return 1;
		} else {
			if (strcmp(stringval, ":/dev/null2") != 0) {
				fprintf(stderr, "Incorrect value after change for DRMAA_ERROR_PATH attribute\n");
				return 1;
			}}

		if (drmaa_get_attribute(jt, DRMAA_JOIN_FILES, stringval, 
			sizeof(stringval)-1, diagnosis, 
			sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
				fprintf(stderr, "drmaa_get_attribute() failed: %s\n", diagnosis);
				return 1;
		} else {
			if (strcmp(stringval, "n") != 0) {
				fprintf(stderr, "Incorrect value after change for DRMAA_JOIN_FILES attribute\n");
				return 1;
			}}

		drmaa_delete_job_template(jt, NULL, 0);

		if (drmaa_exit(diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
			fprintf(stderr, "drmaa_exit() failed: %s\n", diagnosis);
			return 1;
		}

		break;      
	}
case ST_USAGE_CHECK:
	{
		char jobid[1024], value[128], new_jobid[1024];
		drmaa_attr_values_t *rusage = NULL;
		int status;

		if (parse_args)
			exit_job = NEXT_ARGV(argc, argv);

		if (drmaa_init(NULL, diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
			fprintf(stderr, "drmaa_init() failed: %s\n", diagnosis);
			return 1;
		}

		report_session_key();

		jt = create_exit_job_template(exit_job, 0);

		printf ("Running job\n");
		while ((drmaa_errno=drmaa_run_job(jobid, sizeof(jobid)-1, jt, diagnosis,
			sizeof(diagnosis)-1)) == DRMAA_ERRNO_DRM_COMMUNICATION_FAILURE) {
				fprintf(stderr, "drmaa_run_job() failed - retry: %s\n", diagnosis);
				sleep(1);
		}

		if (drmaa_errno != DRMAA_ERRNO_SUCCESS) {
			fprintf(stderr, "drmaa_run_job() failed: %s\n", diagnosis);
			return 1;
		}

		if (jt != NULL) { drmaa_delete_job_template(jt, NULL, 0); jt = NULL; }

		printf ("Waiting for job to complete\n");
		do {
			drmaa_errno = drmaa_wait(jobid, new_jobid, sizeof(jobid)-1, 
				&status, max_wait_timeout, &rusage, diagnosis, sizeof(diagnosis)-1);
			if (drmaa_errno != DRMAA_ERRNO_SUCCESS) {
				fprintf(stderr, "drmaa_wait(%s) failed - retry: %s\n", jobid, diagnosis); 
				sleep(1);
			}
		} while (drmaa_errno == DRMAA_ERRNO_DRM_COMMUNICATION_FAILURE);

		printf("Job with job id %s finished\n", jobid);

		if (drmaa_errno == DRMAA_ERRNO_NO_RUSAGE) {
			fprintf(stderr, "drmaa_wait(%s) did not return usage information.\n", jobid);
			return 1;
		}
		else if (drmaa_errno != DRMAA_ERRNO_SUCCESS) {
			fprintf(stderr, "drmaa_wait(%s) failed: %s\n", jobid, diagnosis);
			return 1;
		}
		else if (rusage == NULL) {
			fprintf (stderr, "drmaa_wait(%s) did not return usage information and did not return DRMAA_ERRNO_NO_RUSAGE\n", jobid);
			return 1;
		}

		while ((drmaa_errno=drmaa_get_next_attr_value(rusage, value, 127))==DRMAA_ERRNO_SUCCESS) {
			printf("%s\n", value);
		}

		if (drmaa_exit(diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) 
		{
			fprintf(stderr, "drmaa_exit() failed: %s\n", diagnosis);
			return 1;
		}


		break;
	}
	/*
	case ST_UNSUPPORTED_ATTR:
	case ST_UNSUPPORTED_VATTR:
	{
	drmaa_job_template_t *jt = NULL;
	const char *values[2];

	values[0] = "blah";
	values[1] = NULL;

	if (drmaa_init(NULL, diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
	fprintf(stderr, "drmaa_init() failed: %s\n", diagnosis);
	return 1;
	}

	// submit a working job from a local directory to the execution host 
	drmaa_allocate_job_template(&jt, NULL, 0);

	if (test_case == ST_UNSUPPORTED_ATTR) {
	drmaa_errno = drmaa_set_attribute(jt, "blah", "blah", diagnosis, sizeof(diagnosis)-1);
	}
	else {
	drmaa_errno = drmaa_set_vector_attribute(jt, "blah", (const char**)values, diagnosis, sizeof(diagnosis)-1);
	}

	if (drmaa_errno != DRMAA_ERRNO_INVALID_ARGUMENT) {
	fprintf(stderr, "drmaa_set_attribute()/drmaa_set_vector_attribute() allowed invalid attribute\n");
	return 1;
	}             

	if (drmaa_exit(diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
	fprintf(stderr, "drmaa_exit() failed: %s\n", diagnosis);
	return 1;
	}
	}
	break;
	*/
default:
	break;
	}

	return 0;
}


static void *submit_and_wait_thread (void *vp) {
	int n;

	if (vp != NULL) {
		n = *(int *)vp;
	}
	else {
		n = 1;
	}

	submit_and_wait (n);

	return (void *)NULL;
}

static int submit_and_wait(int n)
{
	int ret = DRMAA_ERRNO_SUCCESS;

	ret = submit_sleeper(n);

	if (ret == DRMAA_ERRNO_SUCCESS) {
		ret = wait_all_jobs(n);
	}

	return ret;
}

static void *submit_sleeper_thread (void *vp) {
	int n;

	if (vp != NULL) {
		n = *(int *)vp;
	}
	else {
		n = 1;
	}

	submit_sleeper(n);

	return (void *)NULL;
}

static drmaa_job_template_t *create_exit_job_template(const char *exit_job, int as_bulk_job)
{
	char diagnosis[1024];
	const char *job_argv[2];
	drmaa_job_template_t *jt = NULL;
	int ret = DRMAA_ERRNO_SUCCESS;

	ret = drmaa_allocate_job_template(&jt, NULL, 0);

	if (ret == DRMAA_ERRNO_SUCCESS) {
		ret = drmaa_set_attribute(jt, DRMAA_WD, DRMAA_PLACEHOLDER_HD, diagnosis, sizeof(diagnosis)-1);
	}

	if (ret == DRMAA_ERRNO_SUCCESS) {
		ret = drmaa_set_attribute(jt, DRMAA_REMOTE_COMMAND, exit_job,  diagnosis, sizeof(diagnosis)-1);
	}

	if (ret == DRMAA_ERRNO_SUCCESS) {
		job_argv[0] = "0"; 
		job_argv[1] = NULL;
		ret = drmaa_set_vector_attribute(jt, DRMAA_V_ARGV, job_argv,  diagnosis, sizeof(diagnosis)-1);
	}

	if (ret == DRMAA_ERRNO_SUCCESS) {
		ret = drmaa_set_attribute(jt, DRMAA_JOIN_FILES, "y",  diagnosis, sizeof(diagnosis)-1);
	}

#if 0
	if (!as_bulk_job) {
		ret = drmaa_set_attribute(jt, DRMAA_OUTPUT_PATH, ":"DRMAA_PLACEHOLDER_HD"/DRMAA_JOB.$JOB_ID",  diagnosis, sizeof(diagnosis)-1);
	}
	else {
		ret = drmaa_set_attribute(jt, DRMAA_OUTPUT_PATH, ":"DRMAA_PLACEHOLDER_HD"/DRMAA_JOB.$JOB_ID."DRMAA_PLACEHOLDER_INCR,  diagnosis, sizeof(diagnosis)-1);
	}
#else
	if (ret == DRMAA_ERRNO_SUCCESS) {
		/* no output please */
#if !defined(WIN32)
		ret = drmaa_set_attribute(jt, DRMAA_OUTPUT_PATH, ":/dev/null",  diagnosis, sizeof(diagnosis)-1);
#endif
	}
#endif

	if (ret == DRMAA_ERRNO_SUCCESS) {   
		return jt;
	}
	else {
		printf("Error while creating exit job template: %s\n",diagnosis); 
		return NULL;
	}
}

static drmaa_job_template_t *create_sleeper_job_template(int seconds, int as_bulk_job, int in_hold)
{
	const char *job_argv[2];
	drmaa_job_template_t *jt = NULL;
	char buffer[100];
	int ret = DRMAA_ERRNO_SUCCESS;

	ret = drmaa_allocate_job_template(&jt, NULL, 0);

	if (ret == DRMAA_ERRNO_SUCCESS) {
		ret = drmaa_set_attribute(jt, DRMAA_WD, DRMAA_PLACEHOLDER_HD, NULL, 0);
	}

	if (ret == DRMAA_ERRNO_SUCCESS) {
		ret = drmaa_set_attribute(jt, DRMAA_REMOTE_COMMAND, sleeper_job, NULL, 0);
	}

	if (ret == DRMAA_ERRNO_SUCCESS) {
		sprintf(buffer, "%d", seconds);
		job_argv[0] = buffer; 
		job_argv[1] = NULL;
		ret = drmaa_set_vector_attribute(jt, DRMAA_V_ARGV, job_argv, NULL, 0);
	}

	/*   if (ret == DRMAA_ERRNO_SUCCESS) {
	ret = drmaa_set_attribute(jt, DRMAA_JOIN_FILES, "y", NULL, 0);
	}
	*/
	if (ret == DRMAA_ERRNO_SUCCESS) {
#if 0
		if (!as_bulk_job) {
			ret = drmaa_set_attribute(jt, DRMAA_OUTPUT_PATH, ":"DRMAA_PLACEHOLDER_HD"/DRMAA_JOB.$JOB_ID", NULL, 0);
		}
		else {
			ret = drmaa_set_attribute(jt, DRMAA_OUTPUT_PATH, ":"DRMAA_PLACEHOLDER_HD"/DRMAA_JOB.$JOB_ID."DRMAA_PLACEHOLDER_INCR, NULL, 0);
		}
#else
		/* no output please */
#if !defined(WIN32)
		ret = drmaa_set_attribute(jt, DRMAA_OUTPUT_PATH, ":/dev/null", NULL, 0);
#endif
#endif
	}

	if (ret == DRMAA_ERRNO_SUCCESS) {
		if (in_hold) {
			ret = drmaa_set_attribute(jt, DRMAA_JS_STATE, DRMAA_SUBMISSION_STATE_HOLD, NULL, 0);
		}
	}

	if (ret == DRMAA_ERRNO_SUCCESS) {   
		return jt;
	}
	else {
		return NULL;
	}
}

static int submit_sleeper(int n)
{
	drmaa_job_template_t *jt;
	int ret = DRMAA_ERRNO_SUCCESS;

	jt = create_sleeper_job_template(10, 0, 0);

	if (jt != NULL) {
		ret = do_submit(jt, n);

		/* We don't care about the error code from this one.  It doesn't affect
		* anything. */
		drmaa_delete_job_template(jt, NULL, 0);
	}

	return ret;
}

static int do_submit(drmaa_job_template_t *jt, int n)
{
	int i;
	char diagnosis[1024];
	char jobid[100];
	int drmaa_errno = DRMAA_ERRNO_SUCCESS;
	int error = DRMAA_ERRNO_SUCCESS;
	bool done;

	for (i=0; i<n; i++) {
		/* submit job */
		done = false;
		while (!done) {
			drmaa_errno = drmaa_run_job(jobid, sizeof(jobid)-1, jt, diagnosis, sizeof(diagnosis)-1);

			if (drmaa_errno != DRMAA_ERRNO_SUCCESS) {
				printf("failed submitting job (%s)\n", drmaa_strerror(drmaa_errno));
			}

			/* Only retry on "try again" error. */
			if (drmaa_errno == DRMAA_ERRNO_TRY_LATER) {
				printf("retry: %s\n", diagnosis);
				sleep(1);
			} else {
				done = true;
				break; /* while */
			}
		}

		if (drmaa_errno == DRMAA_ERRNO_SUCCESS) {
			printf("submitted job \"%s\"\n", jobid);
		} else {
			printf("unable to submit job\n");
		}

		if (((test_case == MT_EXIT_DURING_SUBMIT_OR_WAIT) || 
			(test_case == MT_EXIT_DURING_SUBMIT)) &&
			(drmaa_errno == DRMAA_ERRNO_NO_ACTIVE_SESSION)) {
				/* It's supposed to do that. */
				drmaa_errno = DRMAA_ERRNO_SUCCESS;
		}

		if (drmaa_errno != DRMAA_ERRNO_SUCCESS) {
			/* If there is ever an error, we will return an error. */
			error = drmaa_errno;
		}
	}

	return error;
}

static int wait_all_jobs(int n)
{
	char jobid[100];
	int drmaa_errno = DRMAA_ERRNO_SUCCESS;
	int stat;
	drmaa_attr_values_t *rusage = NULL;

	do {
		drmaa_errno = drmaa_wait(DRMAA_JOB_IDS_SESSION_ANY, jobid, sizeof(jobid)-1, &stat, max_wait_timeout, &rusage, NULL, 0);

		if (drmaa_errno == DRMAA_ERRNO_SUCCESS) {
			printf("waited job \"%s\"\n", jobid);
			if (n != -1) {
				if (--n == 0) {
					printf("waited for last job\n");
					break;
				}
			}
		} else if (drmaa_errno != DRMAA_ERRNO_INVALID_JOB) {
			printf("drmaa_wait() returned %s\n", drmaa_strerror(drmaa_errno));
		}
	} while (drmaa_errno == DRMAA_ERRNO_SUCCESS);

	/* that means we got all */
	if (drmaa_errno == DRMAA_ERRNO_INVALID_JOB) {
		drmaa_errno = DRMAA_ERRNO_SUCCESS;
	}
	else if ((drmaa_errno == DRMAA_ERRNO_DRM_COMMUNICATION_FAILURE) &&
		(test_case == MT_EXIT_DURING_SUBMIT_OR_WAIT)) {
			/* It's supposed to do that. */
			drmaa_errno = DRMAA_ERRNO_SUCCESS;
	}

	return drmaa_errno;
}

static int wait_n_jobs(int n)
{
	char jobid[100];
	int i, stat;
	int drmaa_errno = DRMAA_ERRNO_SUCCESS;
	int error = DRMAA_ERRNO_SUCCESS;
	bool done;

	for (i=0; i<n; i++) {
		done = false;
		while (!done) {
			drmaa_errno = drmaa_wait(DRMAA_JOB_IDS_SESSION_ANY, jobid,
				sizeof(jobid)-1, &stat,
				max_wait_timeout, NULL, NULL, 0);

			if (drmaa_errno != DRMAA_ERRNO_SUCCESS) {
				printf("failed waiting for any job (%s)\n", drmaa_strerror(drmaa_errno));
			}

			/* Only retry on "try again" error. */
			if (drmaa_errno == DRMAA_ERRNO_TRY_LATER) {
				printf("retry...\n");
				sleep(1);
			} else {
				done = true;
				break;
			}
		}

		if (drmaa_errno == DRMAA_ERRNO_SUCCESS) {
			printf("waiting for any job resulted in finished job \"%s\"\n", jobid);
		} else {
			/* If there is ever an error, we will return an error. */
			error = drmaa_errno;
		}
	}

	return error;
}

static void report_session_key(void) 
{
	if (is_sun_grid_engine) {
		const char *session_key = getenv("SGE_SESSION_KEY");
		if (session_key) 
			printf("got \"%s\" as session key\n", session_key);
		else
			printf("no session key set\n");

	}
}

#if 0
static init_signal_handling()
{
	struct sigaction nact;

	nact.sa_handler = SIG_IGN;

	sigaction(SIGPIPE, &act, NULL);
}
#endif


const struct drmaa_errno_descr_s {
	char *descr;
	int drmaa_errno;
} errno_vector[] = {
	{ "DRMAA_ERRNO_SUCCESS",                      DRMAA_ERRNO_SUCCESS },
	{ "DRMAA_ERRNO_INTERNAL_ERROR",               DRMAA_ERRNO_INTERNAL_ERROR },
	{ "DRMAA_ERRNO_DRM_COMMUNICATION_FAILURE",    DRMAA_ERRNO_DRM_COMMUNICATION_FAILURE },
	{ "DRMAA_ERRNO_AUTH_FAILURE",                 DRMAA_ERRNO_AUTH_FAILURE },
	{ "DRMAA_ERRNO_INVALID_ARGUMENT",             DRMAA_ERRNO_INVALID_ARGUMENT },
	{ "DRMAA_ERRNO_NO_ACTIVE_SESSION",            DRMAA_ERRNO_NO_ACTIVE_SESSION },
	{ "DRMAA_ERRNO_NO_MEMORY",                    DRMAA_ERRNO_NO_MEMORY },
	{ "DRMAA_ERRNO_INVALID_CONTACT_STRING",       DRMAA_ERRNO_INVALID_CONTACT_STRING },
	{ "DRMAA_ERRNO_DEFAULT_CONTACT_STRING_ERROR", DRMAA_ERRNO_DEFAULT_CONTACT_STRING_ERROR },
	{ "DRMAA_ERRNO_DRMS_INIT_FAILED",             DRMAA_ERRNO_DRMS_INIT_FAILED },
	{ "DRMAA_ERRNO_ALREADY_ACTIVE_SESSION",       DRMAA_ERRNO_ALREADY_ACTIVE_SESSION },
	{ "DRMAA_ERRNO_DRMS_EXIT_ERROR",              DRMAA_ERRNO_DRMS_EXIT_ERROR },
	{ "DRMAA_ERRNO_INVALID_ATTRIBUTE_FORMAT",     DRMAA_ERRNO_INVALID_ATTRIBUTE_FORMAT },
	{ "DRMAA_ERRNO_INVALID_ATTRIBUTE_VALUE",      DRMAA_ERRNO_INVALID_ATTRIBUTE_VALUE },
	{ "DRMAA_ERRNO_CONFLICTING_ATTRIBUTE_VALUES", DRMAA_ERRNO_CONFLICTING_ATTRIBUTE_VALUES },
	{ "DRMAA_ERRNO_TRY_LATER",                    DRMAA_ERRNO_TRY_LATER },
	{ "DRMAA_ERRNO_DENIED_BY_DRM",                DRMAA_ERRNO_DENIED_BY_DRM },
	{ "DRMAA_ERRNO_INVALID_JOB",                  DRMAA_ERRNO_INVALID_JOB },
	{ "DRMAA_ERRNO_RESUME_INCONSISTENT_STATE",    DRMAA_ERRNO_RESUME_INCONSISTENT_STATE },
	{ "DRMAA_ERRNO_SUSPEND_INCONSISTENT_STATE",   DRMAA_ERRNO_SUSPEND_INCONSISTENT_STATE },
	{ "DRMAA_ERRNO_HOLD_INCONSISTENT_STATE",      DRMAA_ERRNO_HOLD_INCONSISTENT_STATE },
	{ "DRMAA_ERRNO_RELEASE_INCONSISTENT_STATE",   DRMAA_ERRNO_RELEASE_INCONSISTENT_STATE },
	{ "DRMAA_ERRNO_EXIT_TIMEOUT",                 DRMAA_ERRNO_EXIT_TIMEOUT },
	{ "DRMAA_ERRNO_NO_RUSAGE",                    DRMAA_ERRNO_NO_RUSAGE },
	{ NULL, 0 }
};


/****** test_drmaa/str2drmaa_errno() ********************************************
*  NAME
*     str2drmaa_errno() -- Map string into DRMAA errno constant 
*
*  SYNOPSIS
*     static int str2drmaa_errno(const char *str) 
*
*  FUNCTION
*     Map string into DRMAA errno constant.
*
*  INPUTS
*     const char *str - ??? 
*
*  RESULT
*     static int - DRMAA_ERRNO_* constant or -1 on failre
*******************************************************************************/
static int str2drmaa_errno(const char *str)
{
	int i;
	for (i=0; errno_vector[i].descr != NULL; i++)
		if (!strcmp(errno_vector[i].descr, str))
			return errno_vector[i].drmaa_errno;
	return -1;
}

/****** test_drmaa/drmaa_errno2str() *******************************************
*  NAME
*     drmaa_errno2str() -- Map DRMAA errno constant into string
*
*  SYNOPSIS
*     static const char* drmaa_errno2str(int drmaa_errno) 
*
*  FUNCTION
*     Map DRMAA errno constant into string
*
*  INPUTS
*     int drmaa_errno - Any DRMAA errno
*
*  RESULT
*     static const char* - String representation
*******************************************************************************/
static const char *drmaa_errno2str(int drmaa_errno)
{
	int i;
	for (i=0; errno_vector[i].descr != NULL; i++)
		if (errno_vector[i].drmaa_errno == drmaa_errno)
			return errno_vector[i].descr;
	return "DRMAA_ERRNO_???UNKNOWN???";
}

const struct ctrl_descr_s {
	char *descr;
	int ctrl;
} ctrl_vector[] = {
	{ "DRMAA_CONTROL_SUSPEND",          DRMAA_CONTROL_SUSPEND },
	{ "DRMAA_CONTROL_RESUME",           DRMAA_CONTROL_RESUME },
	{ "DRMAA_CONTROL_HOLD",             DRMAA_CONTROL_HOLD },
	{ "DRMAA_CONTROL_RELEASE",          DRMAA_CONTROL_RELEASE },
	{ "DRMAA_CONTROL_TERMINATE",        DRMAA_CONTROL_TERMINATE },
	{ NULL, 0 }
};

/****** test_drmaa/drmaa_ctrl2str() ********************************************
*  NAME
*     drmaa_ctrl2str() -- Map DRMAA control constant into string 
*
*  SYNOPSIS
*     static const char* drmaa_ctrl2str(int ctrl) 
*
*  FUNCTION
*     Map DRMAA control constant into string 
*
*  INPUTS
*     int ctrl - Any DRMAA_CONTROL_* value
*
*  RESULT
*     static const char* - DRMAA constant name or "unknown" string
*
*******************************************************************************/
static const char *drmaa_ctrl2str(int ctrl)
{
	int i;
	for (i=0; ctrl_vector[i].descr != NULL; i++)
		if (ctrl_vector[i].ctrl == ctrl)
			return ctrl_vector[i].descr;
	return "DRMAA_CONTROL_???UNKNOWN???";
}

/****** test_drmaa/str2drmaa_ctrl() ********************************************
*  NAME
*     str2drmaa_ctrl() -- Map string into DRMAA control constant
*
*  SYNOPSIS
*     static int str2drmaa_ctrl(const char *str) 
*
*  FUNCTION
*     Map string into DRMAA control constant.
*
*  INPUTS
*     const char *str - ??? 
*
*  RESULT
*     static int - DRMAA_CONTROL_* constant or -1 on failure
*******************************************************************************/
static int str2drmaa_ctrl(const char *str)
{
	int i;
	for (i=0; ctrl_vector[i].descr != NULL; i++) {
		if (!strcmp(ctrl_vector[i].descr, str))
			return ctrl_vector[i].ctrl;
	}
	return -1;
}

const struct state_descr_s {
	char *descr;
	int state;
} state_vector[] = {
	{ "DRMAA_PS_UNDETERMINED",          DRMAA_PS_UNDETERMINED },
	{ "DRMAA_PS_QUEUED_ACTIVE",         DRMAA_PS_QUEUED_ACTIVE },
	{ "DRMAA_PS_SYSTEM_ON_HOLD",        DRMAA_PS_SYSTEM_ON_HOLD },
	{ "DRMAA_PS_USER_ON_HOLD ",         DRMAA_PS_USER_ON_HOLD },
	{ "DRMAA_PS_USER_SYSTEM_ON_HOLD",   DRMAA_PS_USER_SYSTEM_ON_HOLD },
	{ "DRMAA_PS_RUNNING",               DRMAA_PS_RUNNING },
	{ "DRMAA_PS_SYSTEM_SUSPENDED",      DRMAA_PS_SYSTEM_SUSPENDED },
	{ "DRMAA_PS_USER_SUSPENDED",        DRMAA_PS_USER_SUSPENDED },
	{ "DRMAA_PS_USER_SYSTEM_SUSPENDED", DRMAA_PS_USER_SYSTEM_SUSPENDED },
	{ "DRMAA_PS_DONE",                  DRMAA_PS_DONE },
	{ "DRMAA_PS_FAILED",                DRMAA_PS_FAILED },
	{ NULL, 0 }
};

/****** test_drmaa/drmaa_state2str() *******************************************
*  NAME
*     drmaa_state2str() -- Map DRMAA state constant into string
*
*  SYNOPSIS
*     static const char* drmaa_state2str(int state) 
*
*  FUNCTION
*     Map DRMAA state constant into string
*
*  INPUTS
*     int state - Any DRMAA_PS_* value.
*
*  RESULT
*     static const char* - 
*******************************************************************************/
static const char *drmaa_state2str(int state)
{
	int i;
	for (i=0; state_vector[i].descr != NULL; i++)
		if (state_vector[i].state == state)
			return state_vector[i].descr;
	return "DRMAA_PS_???UNKNOWN???";
}

/****** test_drmaa/str2drmaa_state() *******************************************
*  NAME
*     str2drmaa_state() -- Map string into DRMAA state constant
*
*  SYNOPSIS
*     int str2drmaa_state(const char *str) 
*
*  FUNCTION
*     Map string into DRMAA state constant.
*
*  INPUTS
*     const char *str - 
*
*  RESULT
*     static int - 
*******************************************************************************/
int str2drmaa_state(const char *str)
{
	int i;
	for (i=0; state_vector[i].descr != NULL; i++)
		if (!strcmp(state_vector[i].descr, str))
			return state_vector[i].state;
	return -1;
}


/****** test_drmaa/report_wrong_job_finish() ***********************************
*  NAME
*     report_wrong_job_finish() -- Report how job finished
*
*  SYNOPSIS
*     static void report_wrong_job_finish(const char *comment, const char 
*     *jobid, int stat) 
*
*  FUNCTION
*     Report how job finished based on the stat value returned by drmaa_wait().
*     The information is printed to stderr.
*
*  INPUTS
*     const char *comment - provided by the caller
*     const char *jobid   - provided by the caller
*     int stat            - stat value as returned by drmaa_wait()
*******************************************************************************/
static void report_wrong_job_finish(const char *comment, const char *jobid, int stat)
{
	int aborted, exited, exit_status, signaled;

	drmaa_wifaborted(&aborted, stat, NULL, 0);
	if (aborted)
		fprintf(stderr, "%s: job \"%s\" never ran\n", comment, jobid);
	else {
		drmaa_wifexited(&exited, stat, NULL, 0);
		if (exited) {
			drmaa_wexitstatus(&exit_status, stat, NULL, 0);
			fprintf(stderr, "%s: job \"%s\" finished regularly with exit status %d\n",
				comment, jobid, exit_status);
		} else {
			drmaa_wifsignaled(&signaled, stat, NULL, 0);
			if (signaled) {
				char termsig[DRMAA_SIGNAL_BUFFER+1];
				drmaa_wtermsig(termsig, DRMAA_SIGNAL_BUFFER, stat, NULL, 0);
				fprintf(stderr, "%s: job \"%s\" finished due to signal %s\n",
					comment, jobid, termsig);
			} else
				fprintf(stderr, "%s: job \"%s\" finished with unclear conditions\n",
				comment, jobid);
		}
	}
}

static bool extract_array_command(char *command_line, int *start, int *end, int *incr) 
{
	bool ret = false;
	char *t_option = NULL;
	char *start_value = NULL;
	char *end_value = NULL;
	char *incr_value = NULL;
	char *end_t_option = NULL;

	*start = 1;
	*end = 1;
	*incr = 1;

	t_option = strstr(command_line, "-t");

	if (t_option != NULL) {
		ret = true;
		start_value = t_option + 3;

		*start = atoi(start_value);

		if (*start <= 0) {
			goto error;
		}

		end_t_option = strstr(start_value, " ");
		end_value = strstr(start_value, "-");
		incr_value = strstr(start_value, ":");

		if ((end_value != NULL) && (end_value < end_t_option)) {
			*end = atoi(end_value+1);

			if (*end <= 0) {
				goto error;
			}

			if ((incr_value != NULL) && (incr_value < end_t_option)) {
				*incr = atoi(incr_value+1);

				if (*incr <= 0) {
					*incr = 1;
				}
			}

		}
		else {
			goto error;
		}

		if (end_t_option != NULL) {
			strcpy(t_option, end_t_option+1);
		}
		else {
			t_option = '\0';
		}
	}/* end if */

	return ret;

error:
	if (end_t_option != NULL) {
		strcpy(t_option, end_t_option);
	}
	else {
		t_option = '\0';
	}   
	*start = 1;
	*end = 1;
	*incr = 1;
	ret = false;
	fprintf(stderr, "could not parse \"%s\" for -t option\n", command_line);

	if (end_t_option != NULL) {
		strcpy(t_option, end_t_option);
	}
	else {
		t_option = '\0';
	}   


	return ret;
}

static int job_run_sequence_verify(int pos, char *all_jobids[], int *order[])
{
	int test_index, i, j;
	int found_group = 0;
	int *group;

	/* search the group this job belongs to */
	for (i=0; order[i]; i++) {
		group = order[i];
		for (j=0; group[j] != -1; j++) {
			if (group[j] == pos) {
				found_group = 1;
				break;
			}
		}
		if (found_group)
			break;
	}
	if (!found_group) {
		fprintf(stderr, "test broken: could not find job index %d in finish order scheme\n", pos);
		return -1;
	}

	/* complain about previous group job that did not finish earlier */
	while (i>0) {
		i--;
		for (j=0; order[i][j] != -1; j++) {
			test_index = order[i][j];
			if (all_jobids[test_index] != NULL) {
				fprintf(stderr, "order broken: job \"%s\" [%d] did not finish before job \"%s\" [%d]\n",
					all_jobids[test_index], test_index, all_jobids[pos], pos);
				return -1;
			}
		}
	}

	return 0;
}


/****** test_drmaa/job_run_sequence_parse() ************************************
*  NAME
*     job_run_sequence_parse() -- ??? 
*
*  SYNOPSIS
*     static int** job_run_sequence_parse(char *jrs_str) 
*
*  FUNCTION
*     Parse job run sequence strings into order data structures. 
*
*  INPUTS
*     char *jrs_str - ??? 
*
*  RESULT
*     static int** - 
*
*  EXAMPLE
*     For exmples the strings "0-1-2", "0,2-1" and "0,1-2-3" are parsed 
*     into data structures like the following ones:
*   
*        int rr0[] = { 0, -1 };
*        int rr1[] = { 1, -1 };
*        int rr2[] = { 2, -1 };
*        int *rr_order[] = { rr0, rr1, rr2, NULL };
*
*        int bf0[] = { 0, 2, -1 };
*        int bf1[] = { 1, -1 };
*        int *bf_order[] = { bf0, bf1, NULL };
*
*        int st0[] = { 0, 1, -1 };
*        int st1[] = { 2, -1 };
*        int st2[] = { 3, -1 };
*        int *st_order[] = { st0, st1, st2, NULL };
*******************************************************************************/
#define GROUP_CHUNK 5
#define NUMBER_CHUNK 10
static int **job_run_sequence_parse(char *jrs_str)
{
	char *s = NULL, *group_str = NULL;

	/* control outer loop */
	char *jrs_str_cp = strdup(jrs_str);
	char  *iter_dash = NULL;
	int **sequence = NULL;
	int groups_total = GROUP_CHUNK;
	int groups_used = 0;
	int i = 0;


	printf("parsing sequence: \"%s\"\n", jrs_str_cp);

	sequence = malloc(sizeof(int *)*(GROUP_CHUNK+1));

	/* groups are delimited by dashes '-' */
	for (group_str=strtok_r(jrs_str_cp, "-", &iter_dash); group_str; group_str=strtok_r(NULL, "-", &iter_dash)) {

		char  *iter_comma = NULL;
		int *group;
		int numbers_total;
		int numbers_used;
		int j = 0;

		if (++groups_used > groups_total) {
			groups_total += GROUP_CHUNK;
			sequence = realloc(sequence, groups_total+1);
		}

		numbers_total = NUMBER_CHUNK;
		numbers_used = 0;

		group = malloc(sizeof(int *)*(NUMBER_CHUNK+1));

		/* sequence numbers within a group are delimited by comma ',' */
		for (s=strtok_r(group_str, ",", &iter_comma); s; s=strtok_r(NULL, ",", &iter_comma)) {
			if (++numbers_used > numbers_total) {
				numbers_total += NUMBER_CHUNK;
				group = realloc(sequence, numbers_total+1);
			}
			printf("%s ", s);
			group[j] = atoi(s);
			j++;
		}
		printf("\n");
		group[j] = -1;

		sequence[i] = group;
		i++;
	}
	sequence[i] = NULL;

	jrs_str_cp = NULL;

	return sequence;
}

static void array_job_run_sequence_adapt(int **sequence, int job_id, int count) 
{
	int x = 0;
	int y = 0;

	if (count <= 1) {
		return;
	}

	printf("modify finish order:\n");

	while (sequence[x] != NULL) { 
		y = 0;
		while (sequence[x][y] != -1) {

			if (sequence[x][y] == job_id) {
				int dy = 0;

				printf("%d ", sequence[x][y]);

				while (sequence[x][y] != -1) {
					y++;
				}

				for (; dy < (count-1); dy++) {
					sequence[x][y+dy] = job_id + dy + 1; 
					sequence[x][y+dy+1] = -1;
					printf("[%d] ", sequence[x][y+dy]);
				}

				y += dy - 1; 
			}
			else if (sequence[x][y] > job_id) {
				sequence[x][y] += (count-1);
				printf("(%d) ", sequence[x][y]);

			}
			else {
				printf("%d ", sequence[x][y]);

			}
			y++;
		}
		printf("\n");
		x++;
	}
}

static bool check_job_state(const char* jobid, int expected) {
	int job_state;
	char diagnosis[1024];
	int unknown_retry = 10;
	int retry_sleep = 6;

	while (unknown_retry > 0) {
		if (drmaa_job_ps(jobid, &job_state, diagnosis, sizeof(diagnosis)-1)!=DRMAA_ERRNO_SUCCESS) {
			fprintf(stderr, "drmaa_job_ps(\"%s\")) failed: %s\n", jobid, diagnosis);
			return false;
		}
		if (job_state != expected) {
			// give DRM a second chance to move to the expected status
			// this fixes a SGE 6.0U8 bug (PS_UNDETERMINED) and delayed drmaa_control() effects
			fprintf(stderr,"Current job state is %s, expected %s, trying to get another answer (%u attempts left)\n", 
				drmaa_state2str(job_state),
				drmaa_state2str(expected),
				unknown_retry);
			unknown_retry--;
			sleep(retry_sleep);
		} else {
			return true;
		}
	}
	fprintf(stderr, "drmaa_job_ps() return wrong status for too many times, while %s was expected. I give up.\n",
		drmaa_state2str(expected));
	return false;
}

// consider that a positive result is considered by a non-zero value
static bool check_term_details(int stat, int exp_aborted, int exp_exited, int exp_signaled) {
	int aborted, exited, signaled, core, drmaa_errno;
	char diagnosis[DRMAA_ERROR_STRING_BUFFER];

	// note that checking wcoredump is not possible, since the creation of core dumps is system dependent

	if ((drmaa_errno=drmaa_wifaborted(&aborted, stat, diagnosis, sizeof(diagnosis)-1))!=DRMAA_ERRNO_SUCCESS) {
		fprintf(stderr, "drmaa_wifaborted failed: %s\n", diagnosis);
		return false;
	}
	if (exp_aborted!=0 && aborted==0) {
		fprintf(stderr,"Expected wifaborted = %u, got %u\n",exp_aborted, aborted);
		return false;
	}

	if ((drmaa_errno=drmaa_wifexited(&exited, stat, diagnosis, sizeof(diagnosis)-1))!=DRMAA_ERRNO_SUCCESS) {
		fprintf(stderr, "drmaa_wifexited failed: %s\n", diagnosis);
		return false;
	}
	if (exp_exited!=0 && exited==0) {
		fprintf(stderr,"Expected wifexited = %u, got %u\n",exp_exited, exited);
		return false;
	}

	if ((drmaa_errno=drmaa_wifsignaled(&signaled, stat, diagnosis, sizeof(diagnosis)-1))!=DRMAA_ERRNO_SUCCESS) {
		fprintf(stderr, "drmaa_wifsignaled failed: %s\n", diagnosis);
		return false;
	}
	if (exp_signaled!=0 && signaled==0) {
		fprintf(stderr,"Expected wifsignaled = %u, got %u\n",exp_signaled, signaled);
		return false;
	}
	return true;
}



