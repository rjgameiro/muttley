// muttley.c
// AIX Device WatchDog Kernel Extension Controller
//
// Copyright (C) 2010 Ricardo Gameiro
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <nlist.h>
#include <dlfcn.h>
#include <sys/mode.h>
#include <sys/sysconfig.h>


#include "kexutil.h"
#include "muttley.kex.h"

#define MUTTLEY_NAME "muttley"

const char * author  = "(c) 2010 Ricardo Gameiro\n\n";

const char * application =
    MUTTLEY_NAME " v0.1 - released under: The MIT License,\n"
    "find it at: http://www.opensource.org/licenses/mit-license.php\n";

const char * help_message =
    "muttley is a kernel device monitoring and watch dog extension allowing\n"
    "load, status, start, display, stop and unload actions to be performed.\n"
    "The extension monitors a device and (optionally) forces a kernel panic\n"
    "and dump, upon loss of access to that device.\n"
    "The monitored device will usually be a raw hard disk or logical volume."
    "\n\n"
    "usage:\n"
    "  "MUTTLEY_NAME " -h\n"
    "  "MUTTLEY_NAME " load\n"
    "  "MUTTLEY_NAME " status\n"
    "  "MUTTLEY_NAME " [-d device] [-c checks] [-s success] [-r runs]\\\n"
    "          [-i run_int] [-b behaviour] [-f] start\n"
    "  "MUTTLEY_NAME " [-i disp_int] [-t times] display\n"
    "  "MUTTLEY_NAME " stop\n"
    "  "MUTTLEY_NAME " unload\n\n"
    "options:\n"
    "  -h             displays this help message\n"
    "  -d device      path to device to monitor (default %s)\n"
    "  -c checks      number of checks to perform on each run (default %d)\n"
    "  -s success     number of successful checks per run to consider the\n"
    "                 device as available (default %d)\n"
    "  -r runs        consecutive failed run threshold to execute the defined\n"
    "                 behaviour (default %d)\n"
    "  -i run_int     interval between check runs in seconds (default %d)\n"
    "  -b behaviour   behaviour on monitoring failure - none, panic\n"
    "                 (default %s)\n"
    "  -f             allow 'device' to be any type of file, if using a file\n"
    "                 it must be at least 512 bytes in size (useful for\n "
    "                 test purposes, i.e. with a file which is removable)\n"
    "  -v disp_int    statistics display interval in seconds (default %d)\n"
    "  -t times       number of times the statistics will be display \n"
    "                 (default %d)\n"
    "arguments:\n"
    "  load           loads the kernel extension\n"
    "  status         query kernel extension status\n"
    "  start          starts monitoring (and executes the defined action on\n"
    "                 threshold)\n"
    "  display        display monitoring statistics\n"
    "  stop           stops monitoring\n"
    "  unload         unloads the kernel extension\n\n"
    "Notes:\n"
    "The muttley command line controller must be executed in the same\n"
    "same directory where the 'muttley.kex' kernel extension is located.\n"
    "\n"
    "DISCLAIMER\nAll sorts of strange and random 'features' may develop in\n"
    "your system by the simple though of using this program.\n\n";

// the muttley's kernel extension name
const char * muttley_kex = "muttley.kex";

// true and false ends up being so much prettier than 1 and 0
enum boolean {
    false = 0,
    true = 1
};

// our exit codes
enum exit_codes {
    exit_ok = 0,
    exit_err_acs = EACCES,
    exit_err_inv = EINVAL,
    exit_err_sys,
    exit_err_int,
    exit_not_rdy
};

// muttley's possible behaviours in english to parse from the command line
const char * behaviour_str[ mtl_behaviour_sz ] = {
    "none",
    "panic"
};

// muttley controller's actions enum
enum actions {
    action_load = 0,
    action_status,
    action_start,
    action_display,
    action_stop,
    action_unload,
    action_sz
};

// and in english to simplify parsing from the command line
const char * actions_str[ action_sz ] = {
    "load",
    "status",
    "start",
    "display",
    "stop",
    "unload"
};

// initialize execution options with some sane defaults
// overridden by the command line options when supplied
struct {
    char * device;
    int action;
    int checks;
    int successes;
    int runs;
    int run_int;
    int behaviour;
    int force;
    int disp_int;
    int times;
} muttley_opt = {
    "/dev/rhd4", 0, 3, 1, 2, 5, mtl_behaviour_none, false, 2, 1
};

// function pointer to the kernel extension's statistics system call
// this is initialized in run time if the kernel extension is loaded upon
// invocation of the command line tool
muttley_query_syscall_t muttley_query_syscall = NULL;


// prototypes
int muttley( void );
int muttley_load( mid_t kmid );
int muttley_status( mid_t kmid );
int muttley_start( mid_t kmid );
int muttley_display( mid_t kmid );
int muttley_stop( mid_t kmid );
int muttley_unload( mid_t kmid );


// main just parses and validates the command line
// upon successful parsing and consistent parameters, it passes control to
// muttley_main() where most of the things happen
int main( int argc, char ** argv ) {

    int c, i;
    extern char * optarg;
    extern int optind, optopt;

    // check if the user is authorized to run me
    // do you feel the power? :)
    if( getuid() ) {
        fprintf( stderr, "having an identity crisis, aren't we? you're not root!\n" );
        exit( exit_err_acs );
    }

    // parse the command line
    while( ( c = getopt( argc, argv, "hd:c:s:i:b:fv:t:" ) ) != EOF ) {

        switch( c ) {
            case 'h':
                fprintf( stdout, "\n%s%s", application, author );
                fprintf( stdout, help_message, muttley_opt.device,
                         muttley_opt.checks, muttley_opt.successes,
                         muttley_opt.runs, muttley_opt.run_int,
                         muttley_opt.behaviour ? "panic" : "none",
                         muttley_opt.disp_int, muttley_opt.times );
                exit( exit_ok );
                break;
            case 'd':             // device to monitor
                muttley_opt.device = optarg;
                break;
            case 'c':             // number of checks per run
                muttley_opt.checks = atoi( optarg );
                break;
            case 's':             // successes per run to consider a run successful
                muttley_opt.successes = atoi( optarg );
                break;
            case 'r':             // number of runs that must fail to execute behaviour
                muttley_opt.runs = atoi( optarg );
                break;
            case 'i':             // interval between runs
                muttley_opt.run_int = atoi( optarg );
                break;
            case 'b':             // behaviour, i.e. do nothing or panic on failure
                muttley_opt.behaviour = -1;
                for( i = 0; i < mtl_behaviour_sz; i++ ) {
                    if( strcmp( behaviour_str[ i ], optarg ) == 0 )
                        muttley_opt.behaviour = i;
                }
                break;
            case 'f':             // allow monitoring on non-character devices
                muttley_opt.force = true;
                break;
            case 'v':             // display interval
                muttley_opt.disp_int = atoi( optarg );
                break;
            case 't':             // display times
                muttley_opt.times = atoi( optarg );
                break;
            case '?':
                exit( exit_err_inv );
                break;
        }
    }

    // perform sanity checks on the command line options and arguments

    // check that behaviour is valid
    if( muttley_opt.behaviour == -1 ) {
        fprintf( stderr, "behaviour: invalid value specified (valid values"
                 " are 'none' or 'panic')\n" );
        exit( exit_err_inv );
    }

    // check that an action was specified and it is valid
    if( optind != ( argc - 1 ) ) {
        fprintf( stderr, "action: argument not specified\n" );
        exit( exit_err_inv );
    } else {
        muttley_opt.action = -1;
        for( i = 0; i < action_sz; i++ ) {
            if( strcmp( actions_str[ i ], argv[ optind ] ) == 0 )
                muttley_opt.action = i;
        }
        if( muttley_opt.action == -1 ) {
            fprintf( stderr, "action: invalid value specified\n" );
            exit( exit_err_inv );
        }
    }

    // check that the number of checks is valid
    if( ( muttley_opt.checks < 1 ) || ( muttley_opt.checks > 10 ) ) {
        fprintf( stderr, "checks: failed sanity (valid range is 1..10)\n" );
        exit( exit_err_inv );
    }

    // check that the number of successes is valid
    if( ( muttley_opt.successes < 1 ) ||
        ( muttley_opt.successes > muttley_opt.checks ) ) {
        fprintf( stderr, "successes: failed sanity check (valid range is "
                 "1..checks(%d))\n", muttley_opt.checks );
        exit( exit_err_inv );
    }

    // check that the run interval is valid
    if( ( muttley_opt.run_int < 1 ) || ( muttley_opt.run_int > 60 ) ) {
        fprintf( stderr, "run_int: failed sanity check (valid range is "
                 "1..60)\n" );
        exit( exit_err_inv );
    }

    // check that the display interval is valid
    if( ( muttley_opt.disp_int < 1 ) || ( muttley_opt.disp_int > 300 ) ) {
        fprintf( stderr, "disp_int: failed sanity check (valid range is "
                 "1..300)\n" );
        exit( exit_err_inv );
    }

    // check that the number of times to display is valid
    if( muttley_opt.times < 1 ) {
        fprintf( stderr, "times: failed sanity check (must be >= 1)\n" );
        exit( exit_err_inv );
    }

    // ok, everything looks good - though there may be some unnecessary
    // options in the command line, 'cause we are not validating that
    // ready to execute whatever was requested

    exit( muttley() );
}


// where most of control magic happens :)
int muttley( void ) {

    int r;
    void * kern_handle;
    mid_t kmid;

    // for simplicity, and because we need it in multiple places just check
    // if the kernel extension is loaded and setup the hook to the
    // muttley_query system call

    if( ( kmid = kex_status( (char *)muttley_kex ) ) > 0 ) {     // is muttley_kex loaded?

        // open the kernel :)
        // woa, writing this comment made me feel knowledgeable
        if( !( kern_handle = dlopen( "/unix", RTLD_NOW | RTLD_GLOBAL ) ) ) {
            fprintf( stderr, "dlopen(/unix): %s\n", strerror( errno ) );
            return( exit_err_sys );
        }
        muttley_query_syscall = (muttley_query_syscall_t)
            dlsym( kern_handle, "muttley_query" );
        dlclose( kern_handle );
        if( ( !muttley_query_syscall ) && ( r = errno ) ) {
            // this really should never happen, extension is loaded, but can't
            // find the muttley_query system call - someone made a typo
            fprintf( stderr, "dlsym(muttley_query): %s\n", strerror( r ) );
            return( exit_err_int );
        }
    } else if( kmid == -1 ) {
        fprintf( stderr, "failed getting muttley status\n" );
        return( exit_err_int );
    }

    // branch control depending on the requested action
    switch( muttley_opt.action ) {
        case action_load:         // load the kernel extension
            r = muttley_load( kmid );
            break;
        case action_status:         // print status, the check was made earlier
            r = muttley_status( kmid );
            break;
        case action_start:         // start the muttley kernel proc
            r = muttley_start( kmid );
            break;
        case action_display:         // display current statistics
            r = muttley_display( kmid );
            break;
        case action_stop:         // stop the muttley kernel proc
            r = muttley_stop( kmid );
            break;
        case action_unload:         // unload the kernel extension
            r = muttley_unload( kmid );
            break;
        default:         // this really shouldn't happen, ever...
            r = exit_err_int;
            break;
    }

    return( r );
}


// load the kernel extension
int muttley_load( mid_t kmid ) {

    if( !kmid ) {
        if( !( kmid = kex_load( (char *)muttley_kex ) ) ) {
            fprintf( stderr, "muttley load failed\n" );
            return( exit_err_sys );
        }
        fprintf( stdout, "muttley successfully loaded with "
                 "kmid = 0x%x\n", kmid );
    } else
        fprintf( stdout, "muttley is already loaded with "
                 "kmid = 0x%x\n", kmid );

    return( exit_ok );
}


// display the kernel extension's load status
int muttley_status( mid_t kmid ) {

    if( !kmid )
        fprintf( stdout, "muttley is not loaded\n" );
    else
        fprintf( stdout, "muttley is loaded with"
                 " kmid = 0x%x\n", kmid );

    return( exit_ok );
}


// start the muttley monitoring kernel proc, done in the extension's
// entry function, called trough sysconfig
int muttley_start( mid_t kmid ) {

    struct muttley_conf conf;
    struct stat device_stat;

    // if muttley is not loaded
    if( !kmid ) {
        fprintf( stderr, "muttley is not loaded, load it first\n" );
        return( exit_not_rdy );
    }

    // start muttley if it is not already running
    if( !muttley_query_syscall( mtl_query_running ) ) {

        // check if the file or device really exists
        if( stat( muttley_opt.device, &device_stat ) == EOF ) {
            fprintf( stderr, "stat(%s): %s\n", muttley_opt.device,
                     strerror( errno ) );
            return( exit_not_rdy );
        }

        // check if it is a character device, unless 'force' was specified
        if( ( !muttley_opt.force ) &&
            ( ( device_stat.st_mode & S_IFCHR ) == 0 ) ) {
            fprintf( stderr, "%s: is not a character device\n",
                     muttley_opt.device );
            return( exit_not_rdy );
        }

        // fill in muttley's kex configuration data
        strncpy( (char *)&conf.device, muttley_opt.device, PATH_MAX - 1 );
        conf.behaviour = muttley_opt.behaviour;
        conf.runs = muttley_opt.runs;
        conf.checks = muttley_opt.checks;
        conf.successes = muttley_opt.successes;
        conf.interval = muttley_opt.run_int;

        // and finally start the kernel proc
        if( !kex_init( kmid, (void *)&conf, sizeof( conf ) ) ) {
            fprintf( stderr, "muttley failed to start on '%s'\n",
                     muttley_opt.device );
            return( exit_err_sys );
        }
        fprintf( stdout, "muttley started on '%s'\n", muttley_opt.device );

    } else {
        fprintf( stdout, "muttley is already running\n" );
    }

    return( exit_ok );
}


// display statistics, mostly useful for debugging
int muttley_display( mid_t kmid ) {

    int c, running;

    running = muttley_query_syscall( mtl_query_running );


    if( muttley_opt.times == 1 ) {     // display one time statistics
        fprintf( stdout, "\nmuttley %s running\n\n", running ?
                 "is" : "is not" );
        if( running ) {
            fprintf( stdout, "lastest run\n" );
            fprintf( stdout, "  time:        %12d (s)\n",
                     muttley_query_syscall( mtl_query_last_time ) );
            fprintf( stdout, "  result:      %12s\n",
                     muttley_query_syscall( mtl_query_last_result ) ?
                     "passed" : "failed" );
            fprintf( stdout, "  successes:   %12d\n",
                     muttley_query_syscall( mtl_query_last_successes ) );
            fprintf( stdout, "  failures:    %12d\n\n",
                     muttley_query_syscall( mtl_query_last_failures ) );
            fprintf( stdout, "consecutive\n  failed runs: %12d\n\n",
                     muttley_query_syscall( mtl_query_failed_runs ) );
            fprintf( stdout, "total\n" );
            fprintf( stdout, "  successes:   %12d\n",
                     muttley_query_syscall( mtl_query_total_successes ) );
            fprintf( stdout, "  failures:    %12d\n",
                     muttley_query_syscall( mtl_query_total_failures ) );
            fprintf( stdout, "\n" );
        }

    } else {     // display running statistics

        for( c = 0; c < muttley_opt.times; c++ ) {

            if( c % 10 == 0 )
                fprintf( stdout, "count :      ltime :  lres : lsucc : "
                         "lfail : cfail : tsucc : tfail\n" );

            fprintf( stdout, "%05d : ", c );
            if( muttley_query_syscall( mtl_query_running ) ) {
                fprintf( stdout, "%10d : ",
                         muttley_query_syscall( mtl_query_last_time ) );
                fprintf( stdout, "%5s : ",
                         muttley_query_syscall( mtl_query_last_result ) ?
                         "pass" : "fail" );
                fprintf( stdout, "%5d : ",
                         muttley_query_syscall( mtl_query_last_successes ) );
                fprintf( stdout, "%5d : ",
                         muttley_query_syscall( mtl_query_last_failures ) );
                fprintf( stdout, "%5d : ",
                         muttley_query_syscall( mtl_query_failed_runs ) );
                fprintf( stdout, "%5d : ",
                         muttley_query_syscall( mtl_query_total_successes ) );
                fprintf( stdout, "%5d\n",
                         muttley_query_syscall( mtl_query_total_failures ) );
            } else
                fprintf( stdout, "---------------------- not running"
                         " -----------------------\n", c );

            sleep( muttley_opt.disp_int );
        }
    }

}


// stop the kernel proc, through sysconfig
int muttley_stop( mid_t kmid ) {

    if( !kmid ) {
        fprintf( stderr, "nonsense, muttley is not even loaded\n" );
        return( exit_not_rdy );
    }

    if( !muttley_query_syscall( mtl_query_running ) ) {
        fprintf( stderr, "nonsense, muttley is not running\n" );
        return( exit_not_rdy );
    }

    fprintf( stdout, "stopping muttley... " );
    fflush( stdout );
    if( kex_term( kmid, NULL, 0 ) )
        fprintf( stdout, "success\n" );
    else {
        fprintf( stderr, "failed\n" );
        return( exit_err_int );
    }

    return( exit_ok );
}


// unload the kernel extension, note that it stays resident
// to remove it from memory, execute 'slibclean' on the shell
int muttley_unload( mid_t kmid ) {

    if( kmid ) {

        // if muttley is running, don't unload
        if( muttley_query_syscall &&
            muttley_query_syscall( mtl_query_running ) ) {
            fprintf( stderr, "muttley is running, stop it prior to trying "
                     "unload or bad things will happen\n" );
            return( exit_not_rdy );
        }

        if( !kex_unload( kmid ) ) {
            fprintf( stderr, "muttley unload failed\n" );
            return( exit_err_sys );
        } else
            fprintf( stdout, "muttley was successfully unloaded\n" );

    } else
        fprintf( stdout, "muttley is not loaded, skipping unload\n" );

    return( exit_ok );
}
