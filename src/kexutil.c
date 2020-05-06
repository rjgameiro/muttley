// kexutil.c
// Wrapper functions to load/unload/start/stop/query AIX kernel extensions.
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
#include <errno.h>
#include <string.h>
#include <sys/buf.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sysconfig.h>
#include <sys/device.h>

#include "kexutil.h"

// load a kernel extension, on success return it's kmid, otherwise
// return 0
mid_t kex_load( char * kex_path ) {

    struct cfg_load load;

    load.kmid = 0;
    load.path = (char *)kex_path;
    load.libpath = NULL;

    if( sysconfig( SYS_KLOAD, (void *)&load, (int)sizeof( load ) ) ) {
        fprintf( stderr, "sysconfig(SYS_KLOAD,%s): %s\n", kex_path,
                 strerror( errno ) );
        return( 0 );
    }
    return( load.kmid );

}

// single load a kernel extension, on success return it's kmid, otherwise
// return 0
mid_t kex_single_load( char * kex_path ) {

    struct cfg_load load;

    load.kmid = 0;
    load.path = (char *)kex_path;
    load.libpath = NULL;

    if( sysconfig( SYS_SINGLELOAD, (void *)&load, (int)sizeof( load ) ) ) {
        fprintf( stderr, "sysconfig(SYS_SINGLELOAD,%s): %s\n", kex_path,
                 strerror( errno ) );
        return( 0 );
    }
    return( load.kmid );

}


// check if a kernel extension is loaded, return (-1) on check failure, 0
// if not loaded and it's kmid if it is loaded
mid_t kex_status( char * kex_path ) {

    struct cfg_load query;

    query.kmid = 0;
    query.path = (char *)kex_path;
    query.libpath = NULL;

    if( sysconfig( SYS_QUERYLOAD, (void *)&query, (int)sizeof( query ) ) ) {
        fprintf( stderr, "sysconfig(SYS_QUERYLOAD,%s): %s\n", kex_path,
                 strerror( errno ) );
        return( -1 );         // probably shouldn't use (-1) has a return code
        // int this case, but for now, it does the job
    }
    return( query.kmid );

}


// initialize a kernel extension (i.e. call it's entry point with CFG_INIT),
// passing it a data buffer with length (may be NULL, 0)
int kex_init( mid_t kmid, void * data, int length ) {

    struct cfg_kmod control;

    control.kmid = kmid;
    control.cmd = CFG_INIT;
    control.mdiptr = data;
    control.mdilen = length;

    if( sysconfig( SYS_CFGKMOD, (void *)&control, (int)sizeof( control ) ) ) {
        fprintf( stderr, "sysconfig(SYS_CFGKMOD,0x%x): %s\n", kmid, strerror( errno ) );
        return( 0 );
    }
    return( 1 );
}


// terminate a kernel extension (i.e. call it's entry point with CFG_TERM),
// passing it a data buffer with length (may be NULL, 0)
int kex_term( mid_t kmid, void * data, int length ) {

    struct cfg_kmod control;

    control.kmid = kmid;
    control.cmd = CFG_TERM;
    control.mdiptr = data;
    control.mdilen = length;

    if( sysconfig( SYS_CFGKMOD, (void *)&control, (int)sizeof( control ) ) ) {
        fprintf( stderr, "sysconfig(SYS_CFGKMOD,0x%x): %s\n", kmid, strerror( errno ) );
        return( 0 );
    }
    return( 1 );
}


// unload a kernel extension, shouldn't be called with the extension in
// use - note that 'genkex' will still reference the extension, to remove it from
// memory, an 'slibclean' on the shell is required
mid_t kex_unload( mid_t kmid ) {

    struct cfg_load unload;

    unload.path = NULL;
    unload.kmid = kmid;
    unload.libpath = NULL;

    if( unload.kmid && sysconfig( SYS_KULOAD, (void *)&unload, (int)sizeof( unload ) ) ) {
        fprintf( stderr, "sysconfig(SYS_KULOAD,0x%x): %s\n", kmid, strerror( errno ) );
        return( 0 );
    }
    return( 1 );

}
