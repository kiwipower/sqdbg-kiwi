#include <cstdio>
#include <cstdlib>
#include <getopt.h>
#include <cstring>
#include <squirrel.h>
#include <sqstdsystem.h>
#include <sqstdstring.h>
#include "sqrdbg.h"
#include <sqstdmath.h>
#include <sqstdblob.h>
#include <cstdarg>
#include <sqstdrex2.h>
#ifdef SQUNICODE
#define scvprintf vwprintf
#else
#define scvprintf vprintf
#endif

int port = 1234;
char *src = NULL;

void _usage(const char *app_name) {
    printf("Usage:\n");
    printf("%s [option <value>] ...\n", app_name);
    printf("\nOptions:\n");
    printf(" -p \t\t # http port, optional - default 1234\n");
    printf("\nExamples:\n");
    printf("%s -p 8888 script.nut\t # Start debugger on port 8888\n",
           app_name);
    exit(1);
}

void _scan_options(int argc, char *argv[]) {
    int opt;

    while ((opt = getopt(argc, argv, "p:")) != -1) {
        switch (opt) {
            case 'p': {
                port = atoi(optarg);
                break;
            }
            default:
                _usage(*argv);
                break;
        }
    }

    if (!argv[optind]) {
        _usage(*argv);
        exit(-1);
    }

    src = static_cast<char *>(malloc(strlen(argv[optind])));
    strcpy(src, argv[optind]);
}

void printfunc(HSQUIRRELVM v, const SQChar *s, ...) {
    va_list arglist;
    va_start(arglist, s);
    scvprintf(s, arglist);
    va_end(arglist);
    fflush(stdout);
}

SQInteger register_math(HSQUIRRELVM v) {
    sq_pushroottable(v);
    sq_pushstring(v, "math", -1);
    sq_newtable(v); //create a new table
    sq_newslot(v, -3, SQFalse);
    sq_pop(v, 1);

    sq_pushstring(v, "math", -1);
    if (SQ_SUCCEEDED(sq_get(v, -2))) {
        sqstd_register_mathlib(v);
    }
    sq_newslot(v, -3, SQFalse);

    sq_pop(v, 1); //pops the root table
    return 0;
}

int main(int argc, char *argv[]) {
    _scan_options(argc, argv);

    FILE *f = fopen(src, "r+b");
    fseek(f, 0, SEEK_END);
    long length = ftell(f);
    fseek(f, 0, SEEK_SET);
    char buf[length];
    fread(buf, 1, length, f);
    fclose(f);

    HSQUIRRELVM vm = sq_open(1024); // creates a VM with initial stack size 1024
    sq_setprintfunc(vm, printfunc, printfunc); //sets the print function
    sq_pushroottable(vm);
    sqstd_register_systemlib(vm);
    register_math(vm);
    sqstd_register_stringlib(vm);
    sqstd_register_mathlib(vm);
    sqstd_register_bloblib(vm);
    sqstd_register_regexp2lib(vm);

    sq_enabledebuginfo(vm, 1);
    HSQREMOTEDBG pServer = sq_rdbg_init(vm, port, false);
    sq_rdbg_waitforconnections(pServer);

    SQRESULT i = sq_compilebuffer(vm, buf, sizeof(buf), src, SQTrue);
    sq_pushroottable(vm);
    SQRESULT call = sq_call(vm, 1, SQFalse, SQTrue);
    printf("%i ... %i", i, call);

    sq_rdbg_shutdown(pServer);
}