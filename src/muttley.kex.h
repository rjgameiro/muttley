// muttley.kex.h
// AIX Device WatchDog Kernel Extension
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

#ifndef MUTTLEY_KEX_H
#define MUTTLEY_KEX_H

#include <limits.h>

// defines allowable behaviours when the check thresholds are exceeded
enum muttley_behaviour {
    mtl_behaviour_none = 0,     // do nothing
    mtl_behaviour_panic,        // force a kernel panic
    mtl_behaviour_sz
};

// defines the list of possible querys to make to the kernel extension
enum muttley_query {
    mtl_query_running = 0,         // is the kernel process running (0 no, 1 yes)
    mtl_query_last_result,         // result of the last run (0 fail, 1 pass)
    mtl_query_last_time,           // time in seconds since epoch of the last run
    mtl_query_last_successes,      // num of successes in the last run
    mtl_query_last_failures,       // num of failures in the last run
    mtl_query_failed_runs,         // number of consecutive failed runs
    mtl_query_total_successes,     // total number of successes since start
    mtl_query_total_failures,      // total number of failures since start
    mtl_query_sz
};

// structure prototype for the kernel extension's parameters
struct muttley_conf {
    char device[ PATH_MAX ];     // path name of device to monitor
    int behaviour;                       // behaviour on failure (none, panic)
    int runs;                            // num of failed runs to execute behaviour
    int checks;                          // num of checks per run
    int successes;                       // num of successes to consider a run successful
    int interval;                        // interval between runs in seconds
};

// type for the muttley query system call when using run time linking to
// the kernel
typedef int ( *muttley_query_syscall_t )( enum muttley_query query );

// system call prototypes
int muttley_query( enum muttley_query query );

#endif // ifndef MUTTLEY_KEX_H
