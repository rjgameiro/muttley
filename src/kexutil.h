// kexutil.h
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

#include <sys/types.h>

#ifndef KEXUTILS_H
#define KEXUTILS_H

// load a kernel extension located at kex_path
mid_t kex_load( char * kex_path );

// single load a kernel extension located at kex_path (i.e. load if not
// already loaded)
mid_t kex_single_load( char * kex_path );

// verify if a kernel extension is loaded (note the it may show with
// 'genkex' but not be loaded), in this case 'slibclean' will remove
// it from memory
mid_t kex_status( char * kex_path );

// initialize a kernel extension, passing it a data bufer block
// data may be NULL with length 0
int kex_init( mid_t kmid, void * data, int length );

// terminate a kernel extension, passing it a data bufer block
// data may be NULL with length 0
int kex_term( mid_t kmid, void * data, int length );

// unload kernel extension (shouldn't be in use)
mid_t kex_unload( mid_t kmid );


#endif // ifndef KEXUTILS_H
