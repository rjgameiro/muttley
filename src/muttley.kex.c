// muttley.kex.c
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

#include <sys/uio.h>
#include <sys/time.h>
#include <sys/m_param.h>
#include <sys/libcsys.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/pin.h>
#include <sys/buf.h>
#include <sys/device.h>
#include <sys/fp_io.h>
#include <sys/proc.h>
#include <sys/timer.h>

#include "muttley.kex.h"


// string to use when calling the panic( s ) system call
#define _MTL_PANIC_STR \
    "\n\n\rmuttley: bark, bark!\n\rI lost what I was watching...\n\n\n\r"

// maximum size of the kernel proc's name (appears in 'ps aux')
#define _MTL_KPROC_NAME_SZ 64
// read buffer size (i.e. number of bytes read when monitoring a device)
#define _MTL_READ_BUF_SZ   512
// time to wait for the kernel proc to terminate
#define _MTL_KPROC_TIMEOUT 20


// watch dog results
enum muttley_watch_res {
    mtl_watch_res_failure = 0,
    mtl_watch_res_success = 1
};

// kernel proc control channel commands
enum muttley_cmds {
    mtl_cmd_stop = 0,
    mtl_cmd_start
};
enum muttley_cmds _muttley_cmd = mtl_cmd_stop;

// private data block where the 'run time' parameters are copied to for use by
// the kernel process
struct muttley_conf _muttley_conf;

// running info (see _mtl_query_* constants in .h file)
int _muttley_info[ mtl_query_sz ];

// used to write to the console when a threshold occurs
struct file * _console_fp;

// the string we send to errpt and to the console
const char * _panic_str = _MTL_PANIC_STR;


// perform one monitoring test, i.e. tries to open, read and close the device,
// usually /dev/rhd4 which contains the / filesystem
enum muttley_watch_res _muttley_watch( char * dev_path ) {

    int r;
    long int b;
    struct file * dev_fp;
    char buf[ _MTL_READ_BUF_SZ ];

    // open the device for reading, return on failure
    if( fp_open( dev_path, O_RDONLY, 0, 0, SYS_ADSPACE, &dev_fp ) )
        return( 0 );

    // read _MTL_READ_BUF_SZ bytes into 'buf'
    r = fp_read( dev_fp, (char *)buf, _MTL_READ_BUF_SZ, 0, SYS_ADSPACE, &b );

    fp_close( dev_fp );

    // return _mtl_watch_res_success if fp_read returned success and the
    // number of bytes requested matches the number of bytes read
    r = ( !r ) && ( b == _MTL_READ_BUF_SZ );
    return( r ? mtl_watch_res_success : mtl_watch_res_failure );
}


// the kernel proc itself,
int _muttley( int flag, void * params, int length ) {

    int check, success;
    struct timestruc_t last_time, curr_time;
    long int b;

    // inform everyone who wants to know that we're running
    _muttley_info[ mtl_query_running ] = 1;

    last_time.tv_sec = 0;
    last_time.tv_nsec = 0;

    // if no one tell's us to stop, then just keep going
    while( _muttley_cmd == mtl_cmd_start ) {

        curtime( &curr_time );

        // if the interval since the previous check has elapsed
        if( curr_time.tv_sec - last_time.tv_sec >= _muttley_conf.interval ) {

            check = 0;
            success = 0;
            // do a full run of checks until we reach _muttley_conf defined
            // success threshold or we reach the maximum checks per run
            while( ( check < _muttley_conf.checks ) &&
                   ( success < _muttley_conf.successes ) ) {
                success += _muttley_watch( _muttley_conf.device );
                check++;
            }

            // rewrite _muttley_info to reflect the latest run
            // update the number of consecutive failed runs (needed to decide
            // when to execute 'behaviour')
            if( success != _muttley_conf.successes ) {
                _muttley_info[ mtl_query_last_result ] = 0;
                _muttley_info[ mtl_query_failed_runs ]++;
            } else {
                _muttley_info[ mtl_query_last_result ] = 1;
                _muttley_info[ mtl_query_failed_runs ] = 0;
            }

            _muttley_info[ mtl_query_last_time ] = curr_time.tv_sec;
            _muttley_info[ mtl_query_last_successes ] = success;
            _muttley_info[ mtl_query_last_failures ] = check - success;
            _muttley_info[ mtl_query_total_successes ] += success;
            _muttley_info[ mtl_query_total_failures ] += check - success;

            last_time = curr_time;

            if( _console_fp &&
                ( _muttley_info[ mtl_query_failed_runs ] == _muttley_conf.runs ) )
                fp_write( _console_fp, (char *)_panic_str, strlen( _panic_str ), 0,
                          SYS_ADSPACE, &b );

        }

        // if the allowable failed runs threshold is reached, then execute
        // the configured behaviour

        if( ( _muttley_conf.behaviour == mtl_behaviour_panic ) &&
            ( _muttley_info[ mtl_query_failed_runs ] >= _muttley_conf.runs ) )
            panic( _panic_str );

        delay( HZ / 4 );
    }

    // inform everyone who wants to know that we've terminated
    _muttley_info[ mtl_query_running ] = 0;
    return( 0 );
}


// kernel module entry point, used to control muttley's monitoring start
// and termination
// - cmd is one of CFG_INIT or CFG_TERM
// - *uiop must reference to a mutley_conf structure with the proper values
int _muttley_ctrl( int cmd, struct uio * uiop ) {

    int i = 0, r = 0;
    pid_t kpid;
    char name[ _MTL_KPROC_NAME_SZ ];


    if( cmd == CFG_INIT ) { // from muttley command line start command

        // pin muttley's code pages to physical memory
        if( !( r = pincode( _muttley_ctrl ) ) ) {

            // get a file handle to the console
            if( fp_open( "/dev/console", O_WRONLY, 0, 0, SYS_ADSPACE,
                         &_console_fp ) )
                _console_fp = NULL;                 // not used if it's NULL

            // clean up info buffer
            for( i = 0; i < mtl_query_sz; i++ ) {
                _muttley_info[ i ] = 0;
            }

            // get parameters from userland buffer into kernel memory
            uiomove( (char *)&_muttley_conf, sizeof( _muttley_conf ),
                     UIO_WRITE, uiop );

            // set up the kernel proc's name to 'muttley:/device'
            strcpy( name, "muttley:" );
            strncat( name, _muttley_conf.device,
                     _MTL_KPROC_NAME_SZ - strlen( name ) - 1 );

            // initialize the control channel to run
            _muttley_cmd = mtl_cmd_start;
            // create and start the kernel proc on the '_mutley' function
            if( ( kpid = creatp() ) != -1 )
                if( initp( kpid, _muttley, NULL, 0, name ) != 0 )
                    r = ( -1 );
        }

    } else if( cmd == CFG_TERM ) { // from muttley comand line stop command

        // tell the kernel proc to stop
        _muttley_cmd = mtl_cmd_stop;

        // wait for the kernel proc to stop for _MTL_KPROC_TIMEOUT seconds
        while( _muttley_info[ mtl_query_running ] &&
               ( i < _MTL_KPROC_TIMEOUT ) ) {
            delay( HZ );
            i++;
        }

        // upon successfull termination, unpin code pages from physical memory
        if( !_muttley_info[ mtl_query_running ] ) {
            if( _console_fp )
                fp_close( _console_fp );
            r = unpincode( _muttley_ctrl );
        } else
            r = ( -1 );

    } else
        r = ( -1 );

    return( r );
}


// exported system call to allow kernel process statistics collection from
// userland processes from the kernel
int muttley_query( enum muttley_query query ) {

    // return the requested info value from _mtl_info
    if( ( query >= 0 ) && ( query < mtl_query_sz ) )
        return( _muttley_info[ query ] );
    return( -1 );

}
