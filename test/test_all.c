#include "CuTest.h"
#include "oss_c_sdk/aos_log.h"
#include "oss_c_sdk/aos_http_io.h"
#include "src/oss_media_define.h"
#include "src/oss_media_client.h"

extern CuSuite *test_client();
extern CuSuite *test_server();
extern CuSuite *test_sts();
extern CuSuite *test_hls();
extern CuSuite *test_hls_stream();

static const struct testlist {
    const char *testname;
    CuSuite *(*func)(void);
} tests[] = {
    {"test_client", test_client},
    {"test_server", test_server},
    {"test_sts", test_sts},
    {"test_hls", test_hls},
    {"test_hls_stream", test_hls_stream},
    {"LastTest", NULL}
};

int run_all_tests(int argc, char *argv[])
{
    int i;
    int exit_code;
    int list_provided = 0;
    CuSuite* suite = NULL;
    int j;
    int found;
    CuSuite *st = NULL;
    CuString *output = NULL;
    
    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-v")) {
            continue;
        }
        if (!strcmp(argv[i], "-l")) {
            for (i = 0; tests[i].func != NULL; i++) {
                printf("%s\n", tests[i].testname);
            }
            exit(0);
        }
        if (argv[i][0] == '-') {
            fprintf(stderr, "invalid option: `%s'\n", argv[i]);
            exit(1);
        }
        list_provided = 1;
    }

    suite = CuSuiteNew();

    if (!list_provided) {
        /* add everything */
        for (i = 0; tests[i].func != NULL; i++) {
            st = tests[i].func();
            CuSuiteAddSuite(suite, st);
            CuSuiteFree(st);
        }
    } else {
        /* add only the tests listed */
        for (i = 1; i < argc; i++) {
            found = 0;
            if (argv[i][0] == '-') {
                continue;
            }
            for (j = 0; tests[j].func != NULL; j++) {
                if (!strcmp(argv[i], tests[j].testname)) {
                    CuSuiteAddSuite(suite, tests[j].func());
                    found = 1;
                }
            }
            if (!found) {
                fprintf(stderr, "invalid test name: `%s'\n", argv[i]);
                exit(1);
            }
        }
    }

    output = CuStringNew();
    CuSuiteRun(suite);
    CuSuiteSummary(suite, output);
    CuSuiteDetails(suite, output);
    printf("%s\n", output->buffer);

    exit_code = suite->failCount > 0 ? 1 : 0;

    CuSuiteFreeDeep(suite);
    CuStringFree(output);

    return exit_code;
}

int main(int argc, char *argv[])
{
    int exit_code;
    if (oss_media_init(AOS_LOG_OFF) != AOSE_OK) {
        exit(1);
    }

    exit_code = run_all_tests(argc, argv);

    oss_media_destroy();
    
    return exit_code;
}
