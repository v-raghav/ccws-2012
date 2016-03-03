// This file created from cuda_runtime_api.h distributed with CUDA 1.1
// Changes Copyright 2009,  Tor M. Aamodt, Ali Bakhoda and George L. Yuan
// University of British Columbia

/* 
 * cuda_runtime_api.cc
 *
 * Copyright © 2009 by Tor M. Aamodt, Wilson W. L. Fung, Ali Bakhoda, 
 * George L. Yuan and the University of British Columbia, Vancouver, 
 * BC V6T 1Z4, All Rights Reserved.
 * 
 * THIS IS A LEGAL DOCUMENT BY DOWNLOADING GPGPU-SIM, YOU ARE AGREEING TO THESE
 * TERMS AND CONDITIONS.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNERS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 * 
 * NOTE: The files libcuda/cuda_runtime_api.c and src/cuda-sim/cuda-math.h
 * are derived from the CUDA Toolset available from http://www.nvidia.com/cuda
 * (property of NVIDIA).  The files benchmarks/BlackScholes/ and 
 * benchmarks/template/ are derived from the CUDA SDK available from 
 * http://www.nvidia.com/cuda (also property of NVIDIA).  The files from 
 * src/intersim/ are derived from Booksim (a simulator provided with the 
 * textbook "Principles and Practices of Interconnection Networks" available 
 * from http://cva.stanford.edu/books/ppin/). As such, those files are bound by 
 * the corresponding legal terms and conditions set forth separately (original 
 * copyright notices are left in files from these sources and where we have 
 * modified a file our copyright notice appears before the original copyright 
 * notice).  
 * 
 * Using this version of GPGPU-Sim requires a complete installation of CUDA 
 * which is distributed seperately by NVIDIA under separate terms and 
 * conditions.  To use this version of GPGPU-Sim with OpenCL requires a
 * recent version of NVIDIA's drivers which support OpenCL.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 
 * 3. Neither the name of the University of British Columbia nor the names of
 * its contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 * 
 * 4. This version of GPGPU-SIM is distributed freely for non-commercial use only.  
 *  
 * 5. No nonprofit user may place any restrictions on the use of this software,
 * including as modified by the user, by any other authorized user.
 * 
 * 6. GPGPU-SIM was developed primarily by Tor M. Aamodt, Wilson W. L. Fung, 
 * Ali Bakhoda, George L. Yuan, at the University of British Columbia, 
 * Vancouver, BC V6T 1Z4
 */

/*
 * Copyright 1993-2007 NVIDIA Corporation.  All rights reserved.
 *
 * NOTICE TO USER:   
 *
 * This source code is subject to NVIDIA ownership rights under U.S. and 
 * international Copyright laws.  Users and possessors of this source code 
 * are hereby granted a nonexclusive, royalty-free license to use this code 
 * in individual and commercial software.
 *
 * NVIDIA MAKES NO REPRESENTATION ABOUT THE SUITABILITY OF THIS SOURCE 
 * CODE FOR ANY PURPOSE.  IT IS PROVIDED "AS IS" WITHOUT EXPRESS OR 
 * IMPLIED WARRANTY OF ANY KIND.  NVIDIA DISCLAIMS ALL WARRANTIES WITH 
 * REGARD TO THIS SOURCE CODE, INCLUDING ALL IMPLIED WARRANTIES OF 
 * MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE.
 * IN NO EVENT SHALL NVIDIA BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL, 
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS 
 * OF USE, DATA OR PROFITS,  WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE 
 * OR OTHER TORTIOUS ACTION,  ARISING OUT OF OR IN CONNECTION WITH THE USE 
 * OR PERFORMANCE OF THIS SOURCE CODE.  
 *
 * U.S. Government End Users.   This source code is a "commercial item" as 
 * that term is defined at  48 C.F.R. 2.101 (OCT 1995), consisting  of 
 * "commercial computer  software"  and "commercial computer software 
 * documentation" as such terms are  used in 48 C.F.R. 12.212 (SEPT 1995) 
 * and is provided to the U.S. Government only as a commercial end item.  
 * Consistent with 48 C.F.R.12.212 and 48 C.F.R. 227.7202-1 through 
 * 227.7202-4 (JUNE 1995), all U.S. Government End Users acquire the 
 * source code with only those rights set forth herein. 
 *
 * Any use of this source code in individual and commercial software must 
 * include, in the user documentation and internal comments to the code,
 * the above Disclaimer and U.S. Government End Users Notice.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <stdarg.h>
#include <sys/mman.h>
#ifdef OPENGL_SUPPORT
#define GL_GLEXT_PROTOTYPES
#ifdef __APPLE__
#include <GLUT/glut.h> // Apple's version of GLUT is here
#else
#include <GL/gl.h>
#endif
#endif

#define __CUDA_RUNTIME_API_H__

#include "host_defines.h"
#include "builtin_types.h"
#include "driver_types.h"
#include "__cudaFatFormat.h"
#include "../src/abstract_hardware_model.h"
#include "../src/gpgpu-sim/gpu-sim.h"
#include "../src/cuda-sim/ptx_loader.h"
#include "../src/cuda-sim/cuda-sim.h"
#include "../src/cuda-sim/ptx_ir.h"
#include "../src/cuda-sim/ptx_parser.h"
#include "../src/gpgpusim_entrypoint.h"
#include "../src/stream_manager.h"

#include <pthread.h>
#include <semaphore.h>

extern void synchronize();
extern void exit_simulation();

static int load_static_globals( symbol_table *symtab, unsigned min_gaddr, unsigned max_gaddr, gpgpu_t *gpu );
static int load_constants( symbol_table *symtab, addr_t min_gaddr, gpgpu_t *gpu );

static kernel_info_t *gpgpu_cuda_ptx_sim_init_grid( const char *kernel_key, 
		gpgpu_ptx_sim_arg_list_t args,
		struct dim3 gridDim,
		struct dim3 blockDim,
		struct CUctx_st* context );

/*DEVICE_BUILTIN*/
struct cudaArray
{
	void *devPtr;
	int devPtr32;
	struct cudaChannelFormatDesc desc;
	int width;
	int height;
	int size; //in bytes
	unsigned dimensions;
};

#if !defined(__dv)
#if defined(__cplusplus)
#define __dv(v) \
		= v
#else /* __cplusplus */
#define __dv(v)
#endif /* __cplusplus */
#endif /* !__dv */

cudaError_t g_last_cudaError = cudaSuccess;

extern stream_manager *g_stream_manager;

void register_ptx_function( const char *name, function_info *impl )
{
	// no longer need this
}

#if defined __APPLE__
#   define __my_func__    __PRETTY_FUNCTION__
#else
# if defined __cplusplus ? __GNUC_PREREQ (2, 6) : __GNUC_PREREQ (2, 4)
#   define __my_func__    __PRETTY_FUNCTION__
# else
#  if defined __STDC_VERSION__ && __STDC_VERSION__ >= 199901L
#   define __my_func__    __func__
#  else
#   define __my_func__    ((__const char *) 0)
#  endif
# endif
#endif

struct _cuda_device_id {
	_cuda_device_id(gpgpu_sim* gpu) {m_id = 0; m_next = NULL; m_gpgpu=gpu;}
	struct _cuda_device_id *next() { return m_next; }
	unsigned num_shader() const { return m_gpgpu->get_config().num_shader(); }
	int num_devices() const {
		if( m_next == NULL ) return 1;
		else return 1 + m_next->num_devices();
	}
	struct _cuda_device_id *get_device( unsigned n )
	{
		assert( n < (unsigned)num_devices() );
		struct _cuda_device_id *p=this;
		for(unsigned i=0; i<n; i++)
			p = p->m_next;
		return p;
	}
	const struct cudaDeviceProp *get_prop() const
	{
		return m_gpgpu->get_prop();
	}
	unsigned get_id() const { return m_id; }

	gpgpu_sim *get_gpgpu() { return m_gpgpu; }
private:
	unsigned m_id;
	class gpgpu_sim *m_gpgpu;
	struct _cuda_device_id *m_next;
};

struct CUctx_st {
	CUctx_st( _cuda_device_id *gpu ) { m_gpu = gpu; }

	_cuda_device_id *get_device() { return m_gpu; }

	void add_binary( symbol_table *symtab, unsigned fat_cubin_handle )
	{
		m_code[fat_cubin_handle] = symtab;
		m_last_fat_cubin_handle = fat_cubin_handle;
	}

	void add_ptxinfo( const char *deviceFun, const struct gpgpu_ptx_sim_kernel_info &info )
	{
		symbol *s = m_code[m_last_fat_cubin_handle]->lookup(deviceFun);
		assert( s != NULL );
		function_info *f = s->get_pc();
		assert( f != NULL );
		f->set_kernel_info(info);
	}

	void register_function( unsigned fat_cubin_handle, const char *hostFun, const char *deviceFun )
	{
		if( m_code.find(fat_cubin_handle) != m_code.end() ) {
			symbol *s = m_code[fat_cubin_handle]->lookup(deviceFun);
			assert( s != NULL );
			function_info *f = s->get_pc();
			assert( f != NULL );
			m_kernel_lookup[hostFun] = f;
		} else {
			m_kernel_lookup[hostFun] = NULL;
		}
	}

	function_info *get_kernel(const char *hostFun)
	{
		std::map<const void*,function_info*>::iterator i=m_kernel_lookup.find(hostFun);
		assert( i != m_kernel_lookup.end() );
		return i->second;
	}

private:
	_cuda_device_id *m_gpu; // selected gpu
	std::map<unsigned,symbol_table*> m_code; // fat binary handle => global symbol table
	unsigned m_last_fat_cubin_handle;
	std::map<const void*,function_info*> m_kernel_lookup; // unique id (CUDA app function address) => kernel entry point
};

class kernel_config {
public:
	kernel_config( dim3 GridDim, dim3 BlockDim, size_t sharedMem, struct CUstream_st *stream )
	{
		m_GridDim=GridDim;
		m_BlockDim=BlockDim;
		m_sharedMem=sharedMem;
		m_stream = stream;
	}
	void set_arg( const void *arg, size_t size, size_t offset )
	{
		m_args.push_front( gpgpu_ptx_sim_arg(arg,size,offset) );
	}
	dim3 grid_dim() const { return m_GridDim; }
	dim3 block_dim() const { return m_BlockDim; }
	gpgpu_ptx_sim_arg_list_t get_args() { return m_args; }
	struct CUstream_st *get_stream() { return m_stream; }

private:
	dim3 m_GridDim;
	dim3 m_BlockDim;
	size_t m_sharedMem;
	struct CUstream_st *m_stream;
	gpgpu_ptx_sim_arg_list_t m_args;
};

class _cuda_device_id *GPGPUSim_Init()
{
	static _cuda_device_id *the_device = NULL;
	if( !the_device ) {
		gpgpu_sim *the_gpu = gpgpu_ptx_sim_init_perf();

		cudaDeviceProp *prop = (cudaDeviceProp *) calloc(sizeof(cudaDeviceProp),1);
		snprintf(prop->name,256,"GPGPU-Sim_v%s", g_gpgpusim_version_string );
		prop->major = 2;
		prop->minor = 0;
		prop->totalGlobalMem = 0x40000000 /* 1 GB */;
		prop->memPitch = 0;
		prop->maxThreadsPerBlock = 512;
		prop->maxThreadsDim[0] = 512;
		prop->maxThreadsDim[1] = 512;
		prop->maxThreadsDim[2] = 512;
		prop->maxGridSize[0] = 0x40000000;
		prop->maxGridSize[1] = 0x40000000;
		prop->maxGridSize[2] = 0x40000000;
		prop->totalConstMem = 0x40000000;
		prop->textureAlignment = 0;
		prop->sharedMemPerBlock = the_gpu->shared_mem_size();
		prop->regsPerBlock = the_gpu->num_registers_per_core();
		prop->warpSize = the_gpu->wrp_size();
		prop->clockRate = the_gpu->shader_clock();
#if (CUDART_VERSION >= 2010)
		prop->multiProcessorCount = the_gpu->get_config().num_shader();
#endif
		the_gpu->set_prop(prop);
		the_device = new _cuda_device_id(the_gpu);
	}
	start_sim_thread(1);
	return the_device;
}

static CUctx_st* GPGPUSim_Context()
{
	static CUctx_st *the_context = NULL;
	if( the_context == NULL ) {
		_cuda_device_id *the_gpu = GPGPUSim_Init();
		the_context = new CUctx_st(the_gpu);
	}
	return the_context;
}

extern "C" void ptxinfo_addinfo()
{
	if( !strcmp("__cuda_dummy_entry__",get_ptxinfo_kname()) ) {
		// this string produced by ptxas for empty ptx files (e.g., bandwidth test)
		clear_ptxinfo();
		return;
	}
	CUctx_st *context = GPGPUSim_Context();
	print_ptxinfo();
	context->add_ptxinfo( get_ptxinfo_kname(), get_ptxinfo_kinfo() );
	clear_ptxinfo();
}

void cuda_not_implemented( const char* func, unsigned line )
{
	fflush(stdout);
	fflush(stderr);
	printf("\n\nGPGPU-Sim PTX: Execution error: CUDA API function \"%s()\" has not been implemented yet.\n"
			"                 [$GPGPUSIM_ROOT/libcuda/%s around line %u]\n\n\n",
			func,__FILE__, line );
	fflush(stdout);
	abort();
}


#define gpgpusim_ptx_error(msg, ...) gpgpusim_ptx_error_impl(__func__, __FILE__,__LINE__, msg, ##__VA_ARGS__)
#define gpgpusim_ptx_assert(cond,msg, ...) gpgpusim_ptx_assert_impl((cond),__func__, __FILE__,__LINE__, msg, ##__VA_ARGS__)

void gpgpusim_ptx_error_impl( const char *func, const char *file, unsigned line, const char *msg, ... )
{
	va_list ap;
	char buf[1024];
	va_start(ap,msg);
	vsnprintf(buf,1024,msg,ap);
	va_end(ap);

	printf("GPGPU-Sim CUDA API: %s\n", buf);
	printf("                    [%s:%u : %s]\n", file, line, func );
	abort();
}

void gpgpusim_ptx_assert_impl( int test_value, const char *func, const char *file, unsigned line, const char *msg, ... )
{
	va_list ap;
	char buf[1024];
	va_start(ap,msg);
	vsnprintf(buf,1024,msg,ap);
	va_end(ap);

	if ( test_value == 0 )
		gpgpusim_ptx_error_impl(func, file, line, msg);
}


typedef std::map<unsigned,CUevent_st*> event_tracker_t;

int CUevent_st::m_next_event_uid;
event_tracker_t g_timer_events;
int g_active_device = 0; //active gpu that runs the code
std::list<kernel_config> g_cuda_launch_stack;

/*******************************************************************************
 *                                                                              *
 *                                                                              *
 *                                                                              *
 *******************************************************************************/

extern "C" {

/*******************************************************************************
 *                                                                              *
 *                                                                              *
 *                                                                              *
 *******************************************************************************/
extern int g_use_host_memory_space;
__host__ cudaError_t CUDARTAPI cudaMalloc(void **devPtr, size_t size) 
{
	CUctx_st* context = GPGPUSim_Context();
	*devPtr = context->get_device()->get_gpgpu()->gpu_malloc(size);
	if(g_debug_execution >= 3)
		printf("GPGPU-Sim PTX: cudaMallocing %zu bytes starting at 0x%llx..\n",size, (unsigned long long) *devPtr);
	if ( *devPtr  ) {
		return g_last_cudaError = cudaSuccess;
	} else {
		return g_last_cudaError = cudaErrorMemoryAllocation;
	}
}

__host__ cudaError_t CUDARTAPI cudaMallocHost(void **ptr, size_t size)
{
	GPGPUSim_Context();
	*ptr = malloc(size);
	if ( *ptr  ) {
		return g_last_cudaError = cudaSuccess;
	} else {
		return g_last_cudaError = cudaErrorMemoryAllocation;
	}
}
__host__ cudaError_t CUDARTAPI cudaMallocPitch(void **devPtr, size_t *pitch, size_t width, size_t height)
{
   unsigned malloc_width_inbytes = width;
   printf("GPGPU-Sim PTX: cudaMallocPitch (width = %d)\n", malloc_width_inbytes);
   CUctx_st* ctx = GPGPUSim_Context();
	*devPtr = ctx->get_device()->get_gpgpu()->gpu_malloc(malloc_width_inbytes*height);
	pitch[0] = malloc_width_inbytes;
	if ( *devPtr  ) {
		return g_last_cudaError = cudaSuccess;
	} else {
		return g_last_cudaError = cudaErrorMemoryAllocation;
	}
}

__host__ cudaError_t CUDARTAPI cudaMallocArray(struct cudaArray **array, const struct cudaChannelFormatDesc *desc, size_t width, size_t height __dv(1))
{
	unsigned size = width * height * ((desc->x + desc->y + desc->z + desc->w)/8);
	CUctx_st* context = GPGPUSim_Context();
	(*array) = (struct cudaArray*) malloc(sizeof(struct cudaArray));
	(*array)->desc = *desc;
	(*array)->width = width;
	(*array)->height = height;
	(*array)->size = size;
	(*array)->dimensions = 2;
	((*array)->devPtr32)= (int) (long long)context->get_device()->get_gpgpu()->gpu_mallocarray(size);
	printf("GPGPU-Sim PTX: cudaMallocArray: devPtr32 = %d\n", ((*array)->devPtr32));
	((*array)->devPtr) = (void*) (long long) ((*array)->devPtr32);
	if ( ((*array)->devPtr) ) {
		return g_last_cudaError = cudaSuccess;
	} else {
		return g_last_cudaError = cudaErrorMemoryAllocation;
	}
}

__host__ cudaError_t CUDARTAPI cudaFree(void *devPtr)
{
    CUctx_st* ctx = GPGPUSim_Context();
    ctx->get_device()->get_gpgpu()->gpu_free(devPtr);
    return g_last_cudaError = cudaSuccess;
}
__host__ cudaError_t CUDARTAPI cudaFreeHost(void *ptr)
{
	free (ptr);  // this will crash the system if called twice
	return g_last_cudaError = cudaSuccess;
}

__host__ cudaError_t CUDARTAPI cudaFreeArray(struct cudaArray *array)
{
	// TODO...  manage g_global_mem space?
	return g_last_cudaError = cudaSuccess;
};


/*******************************************************************************
 *                                                                              *
 *                                                                              *
 *                                                                              *
 *******************************************************************************/

__host__ cudaError_t CUDARTAPI cudaMemcpy(void *dst, const void *src, size_t count, enum cudaMemcpyKind kind)
{
	//CUctx_st *context = GPGPUSim_Context();
	//gpgpu_t *gpu = context->get_device()->get_gpgpu();
	if(g_debug_execution >= 3)
		printf("GPGPU-Sim PTX: cudaMemcpy(): devPtr = %p\n", dst);
	if( kind == cudaMemcpyHostToDevice )
		g_stream_manager->push( stream_operation(src,(size_t)dst,count,0) );
	else if( kind == cudaMemcpyDeviceToHost )
		g_stream_manager->push( stream_operation((size_t)src,dst,count,0) );
	else if( kind == cudaMemcpyDeviceToDevice )
		g_stream_manager->push( stream_operation((size_t)src,(size_t)dst,count,0) );
	else {
		printf("GPGPU-Sim PTX: cudaMemcpy - ERROR : unsupported cudaMemcpyKind\n");
		abort();
	}
	return g_last_cudaError = cudaSuccess;
}

__host__ cudaError_t CUDARTAPI cudaMemcpyToArray(struct cudaArray *dst, size_t wOffset, size_t hOffset, const void *src, size_t count, enum cudaMemcpyKind kind)
{ 
	CUctx_st *context = GPGPUSim_Context();
	gpgpu_t *gpu = context->get_device()->get_gpgpu();
	size_t size = count;
	printf("GPGPU-Sim PTX: cudaMemcpyToArray\n");
	if( kind == cudaMemcpyHostToDevice )
		gpu->memcpy_to_gpu( (size_t)(dst->devPtr), src, size);
	else if( kind == cudaMemcpyDeviceToHost )
		gpu->memcpy_from_gpu( dst->devPtr, (size_t)src, size);
	else if( kind == cudaMemcpyDeviceToDevice )
		gpu->memcpy_gpu_to_gpu( (size_t)(dst->devPtr), (size_t)src, size);
	else {
		printf("GPGPU-Sim PTX: cudaMemcpyToArray - ERROR : unsupported cudaMemcpyKind\n");
		abort();
	}
	dst->devPtr32 = (unsigned) (size_t)(dst->devPtr);
	return g_last_cudaError = cudaSuccess;
}


__host__ cudaError_t CUDARTAPI cudaMemcpyFromArray(void *dst, const struct cudaArray *src, size_t wOffset, size_t hOffset, size_t count, enum cudaMemcpyKind kind)
{
	cuda_not_implemented(__my_func__,__LINE__);
	return g_last_cudaError = cudaErrorUnknown;
}


__host__ cudaError_t CUDARTAPI cudaMemcpyArrayToArray(struct cudaArray *dst, size_t wOffsetDst, size_t hOffsetDst, const struct cudaArray *src, size_t wOffsetSrc, size_t hOffsetSrc, size_t count, enum cudaMemcpyKind kind __dv(cudaMemcpyDeviceToDevice))
{
	cuda_not_implemented(__my_func__,__LINE__);
	return g_last_cudaError = cudaErrorUnknown;
}


__host__ cudaError_t CUDARTAPI cudaMemcpy2D(void *dst, size_t dpitch, const void *src, size_t spitch, size_t width, size_t height, enum cudaMemcpyKind kind)
{
	CUctx_st *context = GPGPUSim_Context();
	gpgpu_t *gpu = context->get_device()->get_gpgpu();
	struct cudaArray *cuArray_ptr;
	size_t size = spitch*height;
	cuArray_ptr = (cudaArray*)dst;
	gpgpusim_ptx_assert( (dpitch==spitch), "different src and dst pitch not supported yet" );
	if( kind == cudaMemcpyHostToDevice )
		gpu->memcpy_to_gpu( (size_t)dst, src, size );
	else if( kind == cudaMemcpyDeviceToHost )
		gpu->memcpy_from_gpu( dst, (size_t)src, size );
	else if( kind == cudaMemcpyDeviceToDevice )
		gpu->memcpy_gpu_to_gpu( (size_t)dst, (size_t)src, size);
	else {
		printf("GPGPU-Sim PTX: cudaMemcpy2D - ERROR : unsupported cudaMemcpyKind\n");
		abort();
	}
	return g_last_cudaError = cudaSuccess;
}


__host__ cudaError_t CUDARTAPI cudaMemcpy2DToArray(struct cudaArray *dst, size_t wOffset, size_t hOffset, const void *src, size_t spitch, size_t width, size_t height, enum cudaMemcpyKind kind)
{
	CUctx_st *context = GPGPUSim_Context();
	gpgpu_t *gpu = context->get_device()->get_gpgpu();
	size_t size = spitch*height;
	size_t channel_size = dst->desc.w+dst->desc.x+dst->desc.y+dst->desc.z;
	gpgpusim_ptx_assert( ((channel_size%8) == 0), "none byte multiple destination channel size not supported (sz=%u)", channel_size );
	unsigned elem_size = channel_size/8;
	gpgpusim_ptx_assert( (dst->dimensions==2), "copy to none 2D array not supported" );
	gpgpusim_ptx_assert( (wOffset==0), "non-zero wOffset not yet supported" );
	gpgpusim_ptx_assert( (hOffset==0), "non-zero hOffset not yet supported" );
	gpgpusim_ptx_assert( (dst->height == (int)height), "partial copy not supported" );
	gpgpusim_ptx_assert( (elem_size*dst->width == width), "partial copy not supported" );
	gpgpusim_ptx_assert( (spitch == width), "spitch != width not supported" );
	if( kind == cudaMemcpyHostToDevice )
		gpu->memcpy_to_gpu( (size_t)(dst->devPtr), src, size);
	else if( kind == cudaMemcpyDeviceToHost )
		gpu->memcpy_from_gpu( dst->devPtr, (size_t)src, size);
	else if( kind == cudaMemcpyDeviceToDevice )
		gpu->memcpy_gpu_to_gpu( (size_t)dst->devPtr, (size_t)src, size);
	else {
		printf("GPGPU-Sim PTX: cudaMemcpy2D - ERROR : unsupported cudaMemcpyKind\n");
		abort();
	}
	dst->devPtr32 = (unsigned) (size_t)(dst->devPtr);
	return g_last_cudaError = cudaSuccess;
}


__host__ cudaError_t CUDARTAPI cudaMemcpy2DFromArray(void *dst, size_t dpitch, const struct cudaArray *src, size_t wOffset, size_t hOffset, size_t width, size_t height, enum cudaMemcpyKind kind)
{
	cuda_not_implemented(__my_func__,__LINE__);
	return g_last_cudaError = cudaErrorUnknown;
}


__host__ cudaError_t CUDARTAPI cudaMemcpy2DArrayToArray(struct cudaArray *dst, size_t wOffsetDst, size_t hOffsetDst, const struct cudaArray *src, size_t wOffsetSrc, size_t hOffsetSrc, size_t width, size_t height, enum cudaMemcpyKind kind __dv(cudaMemcpyDeviceToDevice))
{
	cuda_not_implemented(__my_func__,__LINE__);
	return g_last_cudaError = cudaErrorUnknown;
}


__host__ cudaError_t CUDARTAPI cudaMemcpyToSymbol(const char *symbol, const void *src, size_t count, size_t offset __dv(0), enum cudaMemcpyKind kind __dv(cudaMemcpyHostToDevice))
{
	//CUctx_st *context = GPGPUSim_Context();
	assert(kind == cudaMemcpyHostToDevice);
	printf("GPGPU-Sim PTX: cudaMemcpyToSymbol: symbol = %p\n", symbol);
	//stream_operation( const char *symbol, const void *src, size_t count, size_t offset )
	g_stream_manager->push( stream_operation(src,symbol,count,offset,0) );
	//gpgpu_ptx_sim_memcpy_symbol(symbol,src,count,offset,1,context->get_device()->get_gpgpu());
	return g_last_cudaError = cudaSuccess;
}


__host__ cudaError_t CUDARTAPI cudaMemcpyFromSymbol(void *dst, const char *symbol, size_t count, size_t offset __dv(0), enum cudaMemcpyKind kind __dv(cudaMemcpyDeviceToHost))
{
	//CUctx_st *context = GPGPUSim_Context();
	assert(kind == cudaMemcpyDeviceToHost);
	printf("GPGPU-Sim PTX: cudaMemcpyFromSymbol: symbol = %p\n", symbol);
	g_stream_manager->push( stream_operation(symbol,dst,count,offset,0) );
	//gpgpu_ptx_sim_memcpy_symbol(symbol,dst,count,offset,0,context->get_device()->get_gpgpu());
	return g_last_cudaError = cudaSuccess;
}



/*******************************************************************************
 *                                                                              *
 *                                                                              *
 *                                                                              *
 *******************************************************************************/

__host__ cudaError_t CUDARTAPI cudaMemcpyAsync(void *dst, const void *src, size_t count, enum cudaMemcpyKind kind, cudaStream_t stream)
{
	struct CUstream_st *s = (struct CUstream_st *)stream;
	switch( kind ) {
	case cudaMemcpyHostToDevice: g_stream_manager->push( stream_operation(src,(size_t)dst,count,s) ); break;
	case cudaMemcpyDeviceToHost: g_stream_manager->push( stream_operation((size_t)src,dst,count,s) ); break;
	case cudaMemcpyDeviceToDevice: g_stream_manager->push( stream_operation((size_t)src,(size_t)dst,count,s) ); break;
	default:
		abort();
	}
	return g_last_cudaError = cudaSuccess;
}


__host__ cudaError_t CUDARTAPI cudaMemcpyToArrayAsync(struct cudaArray *dst, size_t wOffset, size_t hOffset, const void *src, size_t count, enum cudaMemcpyKind kind, cudaStream_t stream)
{
	cuda_not_implemented(__my_func__,__LINE__);
	return g_last_cudaError = cudaErrorUnknown;
}


__host__ cudaError_t CUDARTAPI cudaMemcpyFromArrayAsync(void *dst, const struct cudaArray *src, size_t wOffset, size_t hOffset, size_t count, enum cudaMemcpyKind kind, cudaStream_t stream)
{
	cuda_not_implemented(__my_func__,__LINE__);
	return g_last_cudaError = cudaErrorUnknown;
}


__host__ cudaError_t CUDARTAPI cudaMemcpy2DAsync(void *dst, size_t dpitch, const void *src, size_t spitch, size_t width, size_t height, enum cudaMemcpyKind kind, cudaStream_t stream)
{
	cuda_not_implemented(__my_func__,__LINE__);
	return g_last_cudaError = cudaErrorUnknown;
}


__host__ cudaError_t CUDARTAPI cudaMemcpy2DToArrayAsync(struct cudaArray *dst, size_t wOffset, size_t hOffset, const void *src, size_t spitch, size_t width, size_t height, enum cudaMemcpyKind kind, cudaStream_t stream)
{
	cuda_not_implemented(__my_func__,__LINE__);
	return g_last_cudaError = cudaErrorUnknown;
}


__host__ cudaError_t CUDARTAPI cudaMemcpy2DFromArrayAsync(void *dst, size_t dpitch, const struct cudaArray *src, size_t wOffset, size_t hOffset, size_t width, size_t height, enum cudaMemcpyKind kind, cudaStream_t stream)
{
	cuda_not_implemented(__my_func__,__LINE__);
	return g_last_cudaError = cudaErrorUnknown;
}



/*******************************************************************************
 *                                                                              *
 *                                                                              *
 *                                                                              *
 *******************************************************************************/

__host__ cudaError_t CUDARTAPI cudaMemset(void *mem, int c, size_t count)
{
	CUctx_st *context = GPGPUSim_Context();
	gpgpu_t *gpu = context->get_device()->get_gpgpu();
	gpu->gpu_memset((size_t)mem, c, count);
	return g_last_cudaError = cudaSuccess;
}

__host__ cudaError_t CUDARTAPI cudaMemset2D(void *mem, size_t pitch, int c, size_t width, size_t height)
{
	cuda_not_implemented(__my_func__,__LINE__);
	return g_last_cudaError = cudaErrorUnknown;
}



/*******************************************************************************
 *                                                                              *
 *                                                                              *
 *                                                                              *
 *******************************************************************************/

__host__ cudaError_t CUDARTAPI cudaGetSymbolAddress(void **devPtr, const char *symbol)
{
	cuda_not_implemented(__my_func__,__LINE__);
	return g_last_cudaError = cudaErrorUnknown;
}


__host__ cudaError_t CUDARTAPI cudaGetSymbolSize(size_t *size, const char *symbol)
{
	cuda_not_implemented(__my_func__,__LINE__);
	return g_last_cudaError = cudaErrorUnknown;
}



/*******************************************************************************
 *                                                                              *
 *                                                                              *
 *                                                                              *
 *******************************************************************************/
__host__ cudaError_t CUDARTAPI cudaGetDeviceCount(int *count)
{
	_cuda_device_id *dev = GPGPUSim_Init();
	*count = dev->num_devices();
	return g_last_cudaError = cudaSuccess;
}

__host__ cudaError_t CUDARTAPI cudaGetDeviceProperties(struct cudaDeviceProp *prop, int device)
{
	_cuda_device_id *dev = GPGPUSim_Init();
	if (device <= dev->num_devices() )  {
		*prop= *dev->get_prop();
		return g_last_cudaError = cudaSuccess;
	} else {
		return g_last_cudaError = cudaErrorInvalidDevice;
	}
}

__host__ cudaError_t CUDARTAPI cudaChooseDevice(int *device, const struct cudaDeviceProp *prop)
{
	_cuda_device_id *dev = GPGPUSim_Init();
	*device = dev->get_id();
	return g_last_cudaError = cudaSuccess;
}

__host__ cudaError_t CUDARTAPI cudaSetDevice(int device)
{
	//set the active device to run cuda
	if ( device <= GPGPUSim_Init()->num_devices() ) {
		g_active_device = device;
		return g_last_cudaError = cudaSuccess;
	} else {
		return g_last_cudaError = cudaErrorInvalidDevice;
	}
}

__host__ cudaError_t CUDARTAPI cudaGetDevice(int *device)
{
	*device = g_active_device;
	return g_last_cudaError = cudaSuccess;
}

/*******************************************************************************
 *                                                                              *
 *                                                                              *
 *                                                                              *
 *******************************************************************************/

__host__ cudaError_t CUDARTAPI cudaBindTexture(size_t *offset,
		const struct textureReference *texref,
		const void *devPtr,
		const struct cudaChannelFormatDesc *desc,
		size_t size __dv(UINT_MAX))
{
	CUctx_st *context = GPGPUSim_Context();
	gpgpu_t *gpu = context->get_device()->get_gpgpu();
	printf("GPGPU-Sim PTX: in cudaBindTexture: sizeof(struct textureReference) = %zu\n", sizeof(struct textureReference));
	struct cudaArray *array;
	array = (struct cudaArray*) malloc(sizeof(struct cudaArray));
	array->desc = *desc;
	array->size = size;
	array->width = size;
	array->height = 1;
	array->dimensions = 1;
	array->devPtr = (void*)devPtr;
	array->devPtr32 = (int)(long long)devPtr;
	offset = 0;
	printf("GPGPU-Sim PTX:   size = %zu\n", size);
	printf("GPGPU-Sim PTX:   texref = %p, array = %p\n", texref, array);
	printf("GPGPU-Sim PTX:   devPtr32 = %x\n", array->devPtr32);
	printf("GPGPU-Sim PTX:   Name corresponding to textureReference: %s\n", gpu->gpgpu_ptx_sim_findNamefromTexture(texref));
	printf("GPGPU-Sim PTX:   ChannelFormatDesc: x=%d, y=%d, z=%d, w=%d\n", desc->x, desc->y, desc->z, desc->w);
	printf("GPGPU-Sim PTX:   Texture Normalized? = %d\n", texref->normalized);
	gpu->gpgpu_ptx_sim_bindTextureToArray(texref, array);
	devPtr = (void*)(long long)array->devPtr32;
	printf("GPGPU-Sim PTX: devPtr = %p\n", devPtr);
	return g_last_cudaError = cudaSuccess;
}


__host__ cudaError_t CUDARTAPI cudaBindTextureToArray(const struct textureReference *texref, const struct cudaArray *array, const struct cudaChannelFormatDesc *desc)
{
    CUctx_st *context = GPGPUSim_Context();
    gpgpu_t *gpu = context->get_device()->get_gpgpu();
    if (g_use_host_memory_space) {
		cuda_not_implemented(__my_func__, __LINE__);
		return g_last_cudaError = cudaErrorUnknown;
	}
   printf("GPGPU-Sim PTX: in cudaBindTextureToArray: %p %p\n", texref, array);
   printf("GPGPU-Sim PTX:   devPtr32 = %x\n", array->devPtr32);
   printf("GPGPU-Sim PTX:   Name corresponding to textureReference: %s\n", gpu->gpgpu_ptx_sim_findNamefromTexture(texref));
   printf("GPGPU-Sim PTX:   Texture Normalized? = %d\n", texref->normalized);
   gpu->gpgpu_ptx_sim_bindTextureToArray(texref, array);
   return g_last_cudaError = cudaSuccess;
}

__host__ cudaError_t CUDARTAPI cudaUnbindTexture(const struct textureReference *texref)
{
	return g_last_cudaError = cudaSuccess;
}

__host__ cudaError_t CUDARTAPI cudaGetTextureAlignmentOffset(size_t *offset, const struct textureReference *texref)
{
	cuda_not_implemented(__my_func__,__LINE__);
	return g_last_cudaError = cudaErrorUnknown;
}

__host__ cudaError_t CUDARTAPI cudaGetTextureReference(const struct textureReference **texref, const char *symbol)
{
	cuda_not_implemented(__my_func__,__LINE__);
	return g_last_cudaError = cudaErrorUnknown;
}

__host__ cudaError_t CUDARTAPI cudaGetChannelDesc(struct cudaChannelFormatDesc *desc, const struct cudaArray *array)
{
	*desc = array->desc;
	return g_last_cudaError = cudaSuccess;
}


__host__ struct cudaChannelFormatDesc CUDARTAPI cudaCreateChannelDesc(int x, int y, int z, int w, enum cudaChannelFormatKind f)
{
	struct cudaChannelFormatDesc dummy;
	dummy.x = x;
	dummy.y = y;
	dummy.z = z;
	dummy.w = w;
	dummy.f = f;
	return dummy;
}

__host__ cudaError_t CUDARTAPI cudaGetLastError(void)
{
	return g_last_cudaError;
}

__host__ const char* CUDARTAPI cudaGetErrorString(cudaError_t error)
{
	if( g_last_cudaError == cudaSuccess )
		return "no error";
	char buf[1024];
	snprintf(buf,1024,"<<GPGPU-Sim PTX: there was an error (code = %d)>>", g_last_cudaError);
	return strdup(buf);
}

__host__ cudaError_t CUDARTAPI cudaConfigureCall(dim3 gridDim, dim3 blockDim, size_t sharedMem, cudaStream_t stream)
{
	struct CUstream_st *s = (struct CUstream_st *)stream;
	g_cuda_launch_stack.push_back( kernel_config(gridDim,blockDim,sharedMem,s) );
	return g_last_cudaError = cudaSuccess;
}

__host__ cudaError_t CUDARTAPI cudaSetupArgument(const void *arg, size_t size, size_t offset)
{
	gpgpusim_ptx_assert( !g_cuda_launch_stack.empty(), "empty launch stack" );
	kernel_config &config = g_cuda_launch_stack.back();
	config.set_arg(arg,size,offset);

	struct gpgpu_ptx_sim_arg *param = (gpgpu_ptx_sim_arg*) calloc(1,sizeof(struct gpgpu_ptx_sim_arg));
	param->m_start = arg;
	param->m_nbytes = size;
	param->m_offset = offset;

	return g_last_cudaError = cudaSuccess;
}


__host__ cudaError_t CUDARTAPI cudaLaunch( const char *hostFun )
{
	CUctx_st* context = GPGPUSim_Context();
	char *mode = getenv("PTX_SIM_MODE_FUNC");
	if( mode )
		sscanf(mode,"%u", &g_ptx_sim_mode);
	gpgpusim_ptx_assert( !g_cuda_launch_stack.empty(), "empty launch stack" );
	kernel_config config = g_cuda_launch_stack.back();
	struct CUstream_st *stream = config.get_stream();
	printf("\nGPGPU-Sim PTX: cudaLaunch for 0x%p (mode=%s) on stream %u\n", hostFun,
			g_ptx_sim_mode?"functional simulation":"performance simulation", stream?stream->get_uid():0 );
	kernel_info_t *grid = gpgpu_cuda_ptx_sim_init_grid(hostFun,config.get_args(),config.grid_dim(),config.block_dim(),context);
	std::string kname = grid->name();
	dim3 gridDim = config.grid_dim();
	dim3 blockDim = config.block_dim();
	printf("GPGPU-Sim PTX: pushing kernel \'%s\' to stream %u, gridDim= (%u,%u,%u) blockDim = (%u,%u,%u) \n",
			kname.c_str(), stream?stream->get_uid():0, gridDim.x,gridDim.y,gridDim.z,blockDim.x,blockDim.y,blockDim.z );
	stream_operation op(grid,g_ptx_sim_mode,stream);
	g_stream_manager->push(op);
	g_cuda_launch_stack.pop_back();
	return g_last_cudaError = cudaSuccess;
}

/*******************************************************************************
 *                                                                              *
 *                                                                              *
 *                                                                              *
 *******************************************************************************/

__host__ cudaError_t CUDARTAPI cudaStreamCreate(cudaStream_t *stream)
{
	printf("GPGPU-Sim PTX: cudaStreamCreate\n");
#if (CUDART_VERSION >= 3000)
	*stream = new struct CUstream_st();
	g_stream_manager->add_stream(*stream);
#else
	*stream = 0;
	printf("GPGPU-Sim PTX: WARNING: Asynchronous kernel execution not supported (%s)\n", __my_func__);
#endif
	return g_last_cudaError = cudaSuccess;
}

__host__ cudaError_t CUDARTAPI cudaStreamDestroy(cudaStream_t stream)
{
#if (CUDART_VERSION >= 3000)
	g_stream_manager->destroy_stream(stream);
#endif
	return g_last_cudaError = cudaSuccess;
}

__host__ cudaError_t CUDARTAPI cudaStreamSynchronize(cudaStream_t stream)
{
#if (CUDART_VERSION >= 3000)
	if( stream == NULL )
		return g_last_cudaError = cudaErrorInvalidResourceHandle;
	stream->synchronize();
#else
	printf("GPGPU-Sim PTX: WARNING: Asynchronous kernel execution not supported (%s)\n", __my_func__);
#endif
	return g_last_cudaError = cudaSuccess;
}

__host__ cudaError_t CUDARTAPI cudaStreamQuery(cudaStream_t stream)
{
#if (CUDART_VERSION >= 3000)
	if( stream == NULL )
		return g_last_cudaError = cudaErrorInvalidResourceHandle;
	return g_last_cudaError = stream->empty()?cudaSuccess:cudaErrorNotReady;
#else
	printf("GPGPU-Sim PTX: WARNING: Asynchronous kernel execution not supported (%s)\n", __my_func__);
	return g_last_cudaError = cudaSuccess; // it is always success because all cuda calls are synchronous
#endif
}

/*******************************************************************************
 *                                                                              *
 *                                                                              *
 *                                                                              *
 *******************************************************************************/

__host__ cudaError_t CUDARTAPI cudaEventCreate(cudaEvent_t *event)
{
	CUevent_st *e = new CUevent_st(false);
	g_timer_events[e->get_uid()] = e;
#if CUDART_VERSION >= 3000
	*event = e;
#else
	*event = e->get_uid();
#endif
	return g_last_cudaError = cudaSuccess;
}

CUevent_st *get_event(cudaEvent_t event)
{
	unsigned event_uid;
#if CUDART_VERSION >= 3000
	event_uid = event->get_uid();
#else
	event_uid = event;
#endif
	event_tracker_t::iterator e = g_timer_events.find(event_uid);
	if( e == g_timer_events.end() )
		return NULL;
	return e->second;
}

__host__ cudaError_t CUDARTAPI cudaEventRecord(cudaEvent_t event, cudaStream_t stream)
{
	CUevent_st *e = get_event(event);
	if( !e ) return g_last_cudaError = cudaErrorUnknown;
	struct CUstream_st *s = (struct CUstream_st *)stream;
	stream_operation op(e,s);
	g_stream_manager->push(op);
	return g_last_cudaError = cudaSuccess;
}

__host__ cudaError_t CUDARTAPI cudaEventQuery(cudaEvent_t event)
{
	CUevent_st *e = get_event(event);
	if( e == NULL ) {
		return g_last_cudaError = cudaErrorInvalidValue;
	} else if( e->done() ) {
		return g_last_cudaError = cudaSuccess;
	} else {
		return g_last_cudaError = cudaErrorNotReady;
	}
}

__host__ cudaError_t CUDARTAPI cudaEventSynchronize(cudaEvent_t event)
{
	printf("GPGPU-Sim API: cudaEventSynchronize ** waiting for event\n");
	fflush(stdout);
	CUevent_st *e = (CUevent_st*) event;
	while( !e->done() )
		;
	printf("GPGPU-Sim API: cudaEventSynchronize ** event detected\n");
	fflush(stdout);
	return g_last_cudaError = cudaSuccess;
}

__host__ cudaError_t CUDARTAPI cudaEventDestroy(cudaEvent_t event)
{
	CUevent_st *e = get_event(event);
	unsigned event_uid = e->get_uid();
	event_tracker_t::iterator pe = g_timer_events.find(event_uid);
	if( pe == g_timer_events.end() )
		return g_last_cudaError = cudaErrorInvalidValue;
	g_timer_events.erase(pe);
	return g_last_cudaError = cudaSuccess;
}


__host__ cudaError_t CUDARTAPI cudaEventElapsedTime(float *ms, cudaEvent_t start, cudaEvent_t end)
{
	time_t elapsed_time;
	CUevent_st *s = get_event(start);
	CUevent_st *e = get_event(end);
	if( s==NULL || e==NULL )
		return g_last_cudaError = cudaErrorUnknown;
	elapsed_time = e->clock() - s->clock();
	*ms = 1000*elapsed_time;
	return g_last_cudaError = cudaSuccess;
}



/*******************************************************************************
 *                                                                              *
 *                                                                              *
 *                                                                              *
 *******************************************************************************/

__host__ cudaError_t CUDARTAPI cudaThreadExit(void)
{
	exit_simulation();
	return g_last_cudaError = cudaSuccess;
}

__host__ cudaError_t CUDARTAPI cudaThreadSynchronize(void)
{
	//Called on host side
	synchronize();
	return g_last_cudaError = cudaSuccess;
};

int CUDARTAPI __cudaSynchronizeThreads(void**, void*)
{
	return cudaThreadExit();
}





/*******************************************************************************
 *                                                                              *
 *                                                                              *
 *                                                                              *
 *******************************************************************************/

//#include "../../cuobjdump_to_ptxplus/cuobjdump_parser.h"

enum cuobjdumpSectionType {
	PTXSECTION=0,
	ELFSECTION
};

typedef struct cuobjdumpSectionRec {
	enum cuobjdumpSectionType type;
	char* arch;
	char* identifier;
	char* ptxfilename;
	char* elffilename;
	char* sassfilename;
}cuobjdumpSection;



std::list<cuobjdumpSection> cuobjdump;

// sectiontype: 0 for ptx, 1 for elf
void addCuobjdumpSection(int sectiontype){
	cuobjdumpSection x;
	x.type = sectiontype? ELFSECTION: PTXSECTION;
	cuobjdump.push_front(x);
	printf("## Adding new section %s\n", x.type==PTXSECTION?"PTX":"ELF");
}

void setCuobjdumparch(const char* arch){
	printf("Adding arch: %s\n", arch);
	cuobjdump.front().arch = strdup(arch);
}

void setCuobjdumpidentifier(const char* identifier){
	printf("Adding identifier: %s\n", identifier);
	cuobjdump.front().identifier = strdup(identifier);
}

void setCuobjdumpptxfilename(const char* filename){
	printf("Adding ptx filename: %s\n", filename);
	cuobjdump.front().ptxfilename = strdup(filename);
}

void setCuobjdumpelffilename(const char* filename){
	cuobjdump.front().elffilename = strdup(filename);
}

void setCuobjdumpsassfilename(const char* filename){
	cuobjdump.front().sassfilename = strdup(filename);
}
extern "C" int cuobjdump_parse();
extern "C" FILE *cuobjdump_in;

//! Call cuobjdump to extract everything (-elf -sass -ptx) to a file
/*!
 *	This Function extract the whole PTX (for all the files) using cuobjdump
 * */
void extract_code_using_cuobjdump(){
	char command[1000];

	char fname[1024];
	snprintf(fname,1024,"_cuobjdump_complete_output_XXXXXX");
	int fd=mkstemp(fname);
	close(fd);

	char* whole_code;
	//! Running CUobjdump using dynamic link to current process
	snprintf(command,1000,"$CUDA_INSTALL_PATH/bin/cuobjdump -ptx -elf -sass /proc/%d/exe > %s",getpid(),fname);
	printf("Running cuobjdump using \"%s\"\n", command);
	int result = system(command);
	if(result) {printf("ERROR: Failed to execute: %s\n", command); exit(1);}

	printf("Parsing file %s\n", fname);
	cuobjdump_in = fopen(fname, "r");

	cuobjdump_parse();
	fclose(cuobjdump_in);
	printf("Done parsing!!!\n");
}

//! Find the proper sm version to be used
/*!
 * The function finds the highest sm version lower than the option
 * force_max_capability
 */
unsigned get_best_version(std::list<cuobjdumpSection> sectionlist, CUctx_st *context){

	unsigned forced_max_capability = context->get_device()->get_gpgpu()->get_config().get_forced_max_capability();
	if (((forced_max_capability==0)||(forced_max_capability >=20)) &&
			context->get_device()->get_gpgpu()->get_config().convert_to_ptxplus()) {
		printf("GPGPU-Sim: WARNING: Capability >= 20 are not supported in PTXPlus\n"
				"\tSetting forced_max_capability to 19\n");
		forced_max_capability = 19;
	}
	unsigned max_capability=0;

	std::list<cuobjdumpSection>::iterator iter;

	for (	iter = sectionlist.begin();
			iter != sectionlist.end();
			iter++
			){
		unsigned capability = 0;
		sscanf(iter->arch,"sm_%u", &capability);
		if (capability > max_capability &&
				(forced_max_capability == 0 || capability <= forced_max_capability)){
			max_capability = capability;
		}
	}
	return max_capability;
}

//! Find number of files with a certain sm version
/*!
 * Once the sm verison to be used is known (using get_best_version), this
 * function counts the number of files that use this sm version
 */
unsigned get_number_of_ptx(std::list<cuobjdumpSection> sectionlist, unsigned selected_capability){
	int result = 0;

	for (	std::list<cuobjdumpSection>::iterator iter = sectionlist.begin();
			iter != sectionlist.end();
			iter++
			){
		unsigned capability = 0;
		sscanf(iter->arch, "sm_%u", &capability);
		if(capability == selected_capability &&
				iter->type == PTXSECTION){
			printf("Found compatible PTX section with capability %s\n", iter->arch);
			result++;
		}
	}
	return result;
}

char* readfile (const char* filename){
	assert (filename != NULL);
	char str[128];
	FILE* fp = fopen(filename,"r");
	if (!fp) {
		printf("ERROR: Could not open file %s for reading\n", filename);
		assert (0);
	}
	//! finding size of the file
	int filesize= 0;
	fseek (fp , 0 , SEEK_END);

	filesize = ftell (fp);
	fseek (fp, 0, SEEK_SET);
	//! allocate and copy the entire ptx
	char* ret = (char*)malloc((filesize +1)* sizeof(char));
	fread(ret,1,filesize,fp);
	ret[filesize]='\0';
	fclose(fp);
	return ret;
}

cuobjdumpSection* findelfsection(std::list<cuobjdumpSection> sectionlist, unsigned selected_capability, const char* identifier){

	std::list<cuobjdumpSection>::iterator iter;
	for (	iter = sectionlist.begin();
			iter != sectionlist.end();
			iter++
			){
		unsigned capability = 0;
		sscanf(iter->arch,"sm_%u", &capability);
		if(capability <= selected_capability &&
				strcmp(identifier, iter->identifier)==0 &&
				iter->type == ELFSECTION)
			return &(*iter);
	}
	return NULL;
}

void useCuobjdump() {

	CUctx_st *context = GPGPUSim_Context();
	unsigned source_num=1;
	char *sass, *elf;
	extract_code_using_cuobjdump(); //extract all the output of cuobjdump to _cuobjdump_*.*
	unsigned selected_capability = get_best_version(cuobjdump, context); //Find max capability less than forced_max_capability
	unsigned total_ptx_files = get_number_of_ptx(cuobjdump,selected_capability); //Count ptx files for the given capability

	for (	std::list<cuobjdumpSection>::iterator iter = cuobjdump.begin();
			iter != cuobjdump.end();
			iter++
			){
		unsigned capability = 0;
		sscanf(iter->arch,"sm_%u", &capability);
		if((capability == selected_capability) && (iter->type == PTXSECTION)){
			symbol_table *symtab;
			char *ptxcode = readfile(iter->ptxfilename);
			if(context->get_device()->get_gpgpu()->get_config().convert_to_ptxplus() ) {
				cuobjdumpSection* elfsection = findelfsection(cuobjdump, selected_capability, iter->identifier);
				assert (elfsection!= NULL);
				char *ptxplus_str = gpgpu_ptx_sim_convert_ptx_and_sass_to_ptxplus(
						iter->ptxfilename,
						elfsection->elffilename,
						elfsection->sassfilename);
				symtab=gpgpu_ptx_sim_load_ptx_from_string(ptxplus_str,source_num);
				printf("Adding %s with cubin handle %u\n", iter->ptxfilename, source_num);
				context->add_binary(symtab, source_num);
				gpgpu_ptxinfo_load_from_string( ptxcode,total_ptx_files-source_num);
				delete[] ptxplus_str;
			} else {
				symtab=gpgpu_ptx_sim_load_ptx_from_string(ptxcode, source_num);
				context->add_binary(symtab,source_num);
				gpgpu_ptxinfo_load_from_string( ptxcode, total_ptx_files-source_num);
			}
			source_num++;

			load_static_globals(symtab,STATIC_ALLOC_LIMIT,0xFFFFFFFF,context->get_device()->get_gpgpu());
			load_constants(symtab,STATIC_ALLOC_LIMIT,context->get_device()->get_gpgpu());
		}
	}
	if(!keep_intermediate_files()){
		char rm_commandline[1024];

		snprintf(rm_commandline,1024,"rm -f _cuobjdump_*");

		printf("GPGPU-Sim PTX: removing temporary files using \"%s\"\n", rm_commandline);
		int rm_result = system(rm_commandline);
		if( rm_result != 0 ) {
			printf("GPGPU-Sim PTX: ERROR ** while removing temporary files %d\n", rm_result);
			exit(1);
		}
	}

}

void** CUDARTAPI __cudaRegisterFatBinary( void *fatCubin )
{
#if (CUDART_VERSION < 2010)
	printf("GPGPU-Sim PTX: ERROR ** this version of GPGPU-Sim requires CUDA 2.1 or higher\n");
	exit(1);
#endif
	CUctx_st *context = GPGPUSim_Context();
	static unsigned next_fat_bin_handle = 1;
	if(context->get_device()->get_gpgpu()->get_config().use_cuobjdump()) {
		unsigned fat_cubin_handle = next_fat_bin_handle;
		next_fat_bin_handle++;
		printf("GPGPU-Sim PTX: __cudaRegisterFatBinary, fat_cubin_handle = %u\n", fat_cubin_handle);
		/*!
		 * This function extracts all data from all files in first call
		 * then for next calls, only returns the appropriate number
		 */
		assert(fat_cubin_handle >= 1);
		if(fat_cubin_handle == 1)
			useCuobjdump();

		return (void**)fat_cubin_handle;
	} else {
		static unsigned source_num=1;
		unsigned fat_cubin_handle = next_fat_bin_handle++;
		__cudaFatCudaBinary *info =   (__cudaFatCudaBinary *)fatCubin;
		assert( info->version >= 3 );
		unsigned num_ptx_versions=0;
		unsigned max_capability=0;
		unsigned selected_capability=0;
		bool found=false;
		unsigned forced_max_capability = context->get_device()->get_gpgpu()->get_config().get_forced_max_capability();
		while( info->ptx[num_ptx_versions].gpuProfileName != NULL ) {
			unsigned capability=0;
			sscanf(info->ptx[num_ptx_versions].gpuProfileName,"compute_%u",&capability);
			printf("GPGPU-Sim PTX: __cudaRegisterFatBinary found PTX versions for '%s', ", info->ident);
			printf("capability = %s\n", info->ptx[num_ptx_versions].gpuProfileName );
			if( forced_max_capability ) {
				if( capability > max_capability && capability <= forced_max_capability ) {
					found = true;
					max_capability=capability;
					selected_capability = num_ptx_versions;
				}
			} else {
				if( capability > max_capability ) {
					found = true;
					max_capability=capability;
					selected_capability = num_ptx_versions;
				}
			}
			num_ptx_versions++;
		}
		if( found  ) {
			printf("GPGPU-Sim PTX: Loading PTX for %s, capability = %s\n",
					info->ident, info->ptx[selected_capability].gpuProfileName );
			symbol_table *symtab;
			const char *ptx = info->ptx[selected_capability].ptx;
			if(context->get_device()->get_gpgpu()->get_config().convert_to_ptxplus() ) {
				printf("GPGPU-Sim PTX: ERROR ** PTXPlus is only supported through cuobjdump\n"
				"\tEither enable cuobjdump or disable PTXPlus in your configuration file\n");
				exit(1);
			} else {
				symtab=gpgpu_ptx_sim_load_ptx_from_string(ptx,source_num);
				context->add_binary(symtab,fat_cubin_handle);
				gpgpu_ptxinfo_load_from_string( ptx, source_num );
			}
			source_num++;
			load_static_globals(symtab,STATIC_ALLOC_LIMIT,0xFFFFFFFF,context->get_device()->get_gpgpu());
			load_constants(symtab,STATIC_ALLOC_LIMIT,context->get_device()->get_gpgpu());
		} else {
			printf("GPGPU-Sim PTX: warning -- did not find an appropriate PTX in cubin\n");
		}
		return (void**)fat_cubin_handle;
	}
}

void __cudaUnregisterFatBinary(void **fatCubinHandle)
{
	;
}

cudaError_t cudaDeviceReset ( void ) {
	// Should reset the simulated GPU
	return g_last_cudaError = cudaSuccess;
}
cudaError_t CUDARTAPI cudaDeviceSynchronize(void){
	// I don't know what this should do
	return g_last_cudaError = cudaSuccess;
}


void CUDARTAPI __cudaRegisterFunction(
		void   **fatCubinHandle,
		const char    *hostFun,
		char    *deviceFun,
		const char    *deviceName,
		int      thread_limit,
		uint3   *tid,
		uint3   *bid,
		dim3    *bDim,
		dim3    *gDim
)
{
	CUctx_st *context = GPGPUSim_Context();
	unsigned fat_cubin_handle = (unsigned)(unsigned long long)fatCubinHandle;
	printf("GPGPU-Sim PTX: __cudaRegisterFunction %s : hostFun 0x%p, fat_cubin_handle = %u\n",
			deviceFun, hostFun, fat_cubin_handle);
	context->register_function( fat_cubin_handle, hostFun, deviceFun );
}

extern void __cudaRegisterVar(
		void **fatCubinHandle,
		char *hostVar, //pointer to...something
		char *deviceAddress, //name of variable
		const char *deviceName, //name of variable (same as above)
		int ext,
		int size,
		int constant,
		int global )
{
	printf("GPGPU-Sim PTX: __cudaRegisterVar: hostVar = %p; deviceAddress = %s; deviceName = %s\n", hostVar, deviceAddress, deviceName);
	printf("GPGPU-Sim PTX: __cudaRegisterVar: Registering const memory space of %d bytes\n", size);
	fflush(stdout);
	if ( constant && !global && !ext ) {
		gpgpu_ptx_sim_register_const_variable(hostVar,deviceName,size);
	} else if ( !constant && !global && !ext ) {
		gpgpu_ptx_sim_register_global_variable(hostVar,deviceName,size);
	} else cuda_not_implemented(__my_func__,__LINE__);
}


void __cudaRegisterShared(
		void **fatCubinHandle,
		void **devicePtr
)
{
	// we don't do anything here
	printf("GPGPU-Sim PTX: __cudaRegisterShared\n" );
}

void CUDARTAPI __cudaRegisterSharedVar(
		void   **fatCubinHandle,
		void   **devicePtr,
		size_t   size,
		size_t   alignment,
		int      storage
)
{
	// we don't do anything here
	printf("GPGPU-Sim PTX: __cudaRegisterSharedVar\n" );
}

void __cudaRegisterTexture(
		void **fatCubinHandle,
		const struct textureReference *hostVar,
		const void **deviceAddress,
		const char *deviceName,
		int dim,
		int norm,
		int ext
) //passes in a newly created textureReference
{
	CUctx_st *context = GPGPUSim_Context();
	gpgpu_t *gpu = context->get_device()->get_gpgpu();
	printf("GPGPU-Sim PTX: in __cudaRegisterTexture:\n");
	gpu->gpgpu_ptx_sim_bindNameToTexture(deviceName, hostVar);
	printf("GPGPU-Sim PTX:   int dim = %d\n", dim);
	printf("GPGPU-Sim PTX:   int norm = %d\n", norm);
	printf("GPGPU-Sim PTX:   int ext = %d\n", ext);
	printf("GPGPU-Sim PTX:   Execution warning: Not finished implementing \"%s\"\n", __my_func__ );
}

#ifndef OPENGL_SUPPORT
typedef unsigned long GLuint;
#endif

cudaError_t cudaGLRegisterBufferObject(GLuint bufferObj)
{
	printf("GPGPU-Sim PTX: Execution warning: ignoring call to \"%s\"\n", __my_func__ );
	return g_last_cudaError = cudaSuccess;
}

struct glbmap_entry {
	GLuint m_bufferObj;
	void *m_devPtr;
	size_t m_size;
	struct glbmap_entry *m_next;
};
typedef struct glbmap_entry glbmap_entry_t;

glbmap_entry_t* g_glbmap = NULL;

cudaError_t cudaGLMapBufferObject(void** devPtr, GLuint bufferObj) 
{
#ifdef OPENGL_SUPPORT
	GLint buffer_size=0;
	CUctx_st* ctx = GPGPUSim_Context();

	glbmap_entry_t *p = g_glbmap;
	while ( p && p->m_bufferObj != bufferObj )
		p = p->m_next;
	if ( p == NULL ) {
		glBindBuffer(GL_ARRAY_BUFFER,bufferObj);
		glGetBufferParameteriv(GL_ARRAY_BUFFER,GL_BUFFER_SIZE,&buffer_size);
		assert( buffer_size != 0 );
		*devPtr = ctx->get_device()->get_gpgpu()->gpu_malloc(buffer_size);

		// create entry and insert to front of list
		glbmap_entry_t *n = (glbmap_entry_t *) calloc(1,sizeof(glbmap_entry_t));
		n->m_next = g_glbmap;
		g_glbmap = n;

		// initialize entry
		n->m_bufferObj = bufferObj;
		n->m_devPtr = *devPtr;
		n->m_size = buffer_size;

		p = n;
	} else {
		buffer_size = p->m_size;
		*devPtr = p->m_devPtr;
	}

	if ( *devPtr  ) {
		char *data = (char *) calloc(p->m_size,1);
		glGetBufferSubData(GL_ARRAY_BUFFER,0,buffer_size,data);
		memcpy_to_gpu( (size_t) *devPtr, data, buffer_size );
		free(data);
		printf("GPGPU-Sim PTX: cudaGLMapBufferObject %zu bytes starting at 0x%llx..\n", (size_t)buffer_size,
				(unsigned long long) *devPtr);
		return g_last_cudaError = cudaSuccess;
	} else {
		return g_last_cudaError = cudaErrorMemoryAllocation;
	}

	return g_last_cudaError = cudaSuccess;
#else
	fflush(stdout);
	fflush(stderr);
	printf("GPGPU-Sim PTX: GPGPU-Sim support for OpenGL integration disabled -- exiting\n");
	fflush(stdout);
	exit(50);
#endif
}

cudaError_t cudaGLUnmapBufferObject(GLuint bufferObj)
{
#ifdef OPENGL_SUPPORT
	glbmap_entry_t *p = g_glbmap;
	while ( p && p->m_bufferObj != bufferObj )
		p = p->m_next;
	if ( p == NULL )
		return g_last_cudaError = cudaErrorUnknown;

	char *data = (char *) calloc(p->m_size,1);
	memcpy_from_gpu( data,(size_t)p->m_devPtr,p->m_size );
	glBufferSubData(GL_ARRAY_BUFFER,0,p->m_size,data);
	free(data);

	return g_last_cudaError = cudaSuccess;
#else
	fflush(stdout);
	fflush(stderr);
	printf("GPGPU-Sim PTX: support for OpenGL integration disabled -- exiting\n");
	fflush(stdout);
	exit(50);
#endif
}

cudaError_t cudaGLUnregisterBufferObject(GLuint bufferObj) 
{
	printf("GPGPU-Sim PTX: Execution warning: ignoring call to \"%s\"\n", __my_func__ );
	return g_last_cudaError = cudaSuccess;
}

#if (CUDART_VERSION >= 2010)

cudaError_t CUDARTAPI cudaHostAlloc(void **pHost,  size_t bytes, unsigned int flags)
{
	*pHost = malloc(bytes);
	if( *pHost )
		return g_last_cudaError = cudaSuccess;
	else
		return g_last_cudaError = cudaErrorMemoryAllocation;
}

cudaError_t CUDARTAPI cudaHostGetDevicePointer(void **pDevice, void *pHost, unsigned int flags)
{
	cuda_not_implemented(__my_func__,__LINE__);
	return g_last_cudaError = cudaErrorUnknown;
}

cudaError_t CUDARTAPI cudaSetValidDevices(int *device_arr, int len)
{
	cuda_not_implemented(__my_func__,__LINE__);
	return g_last_cudaError = cudaErrorUnknown;
}

cudaError_t CUDARTAPI cudaSetDeviceFlags( int flags )
{
	cuda_not_implemented(__my_func__,__LINE__);
	return g_last_cudaError = cudaErrorUnknown;
}

cudaError_t CUDARTAPI cudaFuncGetAttributes(struct cudaFuncAttributes *attr, const char *hostFun )
{
	CUctx_st *context = GPGPUSim_Context();
	function_info *entry = context->get_kernel(hostFun);
	if( entry ) {
		const struct gpgpu_ptx_sim_kernel_info *kinfo = entry->get_kernel_info();
		attr->sharedSizeBytes = kinfo->smem;
		attr->constSizeBytes  = kinfo->cmem;
		attr->localSizeBytes  = kinfo->lmem;
		attr->numRegs         = kinfo->regs;
		attr->maxThreadsPerBlock = 0; // from pragmas?
#if CUDART_VERSION >= 3000
		attr->ptxVersion      = kinfo->ptx_version;
		attr->binaryVersion   = kinfo->sm_target;
#endif
	}
	return g_last_cudaError = cudaSuccess;
}

cudaError_t CUDARTAPI cudaEventCreateWithFlags(cudaEvent_t *event, int flags)
{
	CUevent_st *e = new CUevent_st(flags==cudaEventBlockingSync);
	g_timer_events[e->get_uid()] = e;
#if CUDART_VERSION >= 3000
	*event = e;
#else
	*event = e->get_uid();
#endif
	return g_last_cudaError = cudaSuccess;
}

cudaError_t CUDARTAPI cudaDriverGetVersion(int *driverVersion)
{
	*driverVersion = CUDART_VERSION;
	return g_last_cudaError = cudaErrorUnknown;
}

cudaError_t CUDARTAPI cudaRuntimeGetVersion(int *runtimeVersion)
{
	*runtimeVersion = CUDART_VERSION;
	return g_last_cudaError = cudaErrorUnknown;
}

#endif

cudaError_t CUDARTAPI cudaGLSetGLDevice(int device)
{
	printf("GPGPU-Sim PTX: Execution warning: ignoring call to \"%s\"\n", __my_func__ );
	return g_last_cudaError = cudaErrorUnknown;
}

typedef void* HGPUNV;

cudaError_t CUDARTAPI cudaWGLGetDevice(int *device, HGPUNV hGpu)
{
	cuda_not_implemented(__my_func__,__LINE__);
	return g_last_cudaError = cudaErrorUnknown;
}

void CUDARTAPI __cudaMutexOperation(int lock)
{
	cuda_not_implemented(__my_func__,__LINE__);
}

void  CUDARTAPI __cudaTextureFetch(const void *tex, void *index, int integer, void *val) 
{
	cuda_not_implemented(__my_func__,__LINE__);
}

}

namespace cuda_math {

void CUDARTAPI __cudaMutexOperation(int lock)
{
	cuda_not_implemented(__my_func__,__LINE__);
}

void  CUDARTAPI __cudaTextureFetch(const void *tex, void *index, int integer, void *val) 
{
	cuda_not_implemented(__my_func__,__LINE__);
}

int CUDARTAPI __cudaSynchronizeThreads(void**, void*)
{
	//TODO This function should syncronize if we support Asyn kernel calls
	return g_last_cudaError = cudaSuccess;
}

}

////////

extern "C" int ptx_parse();
extern "C" int ptx__scan_string(const char*);
extern "C" FILE *ptx_in;

extern "C" const char *g_ptxinfo_filename;
extern "C" int ptxinfo_parse();
extern "C" int ptxinfo_debug;
extern "C" FILE *ptxinfo_in;

/// static functions

static int load_static_globals( symbol_table *symtab, unsigned min_gaddr, unsigned max_gaddr, gpgpu_t *gpu ) 
{
	printf( "GPGPU-Sim PTX: loading globals with explicit initializers... \n" );
	fflush(stdout);
	int ng_bytes=0;
	symbol_table::iterator g=symtab->global_iterator_begin();

   for ( ; g!=symtab->global_iterator_end(); g++) {
      symbol *global = *g;

      if ( global->has_initializer() ) {
         printf( "GPGPU-Sim PTX:     initializing '%s' ... ", global->name().c_str() ); 
         unsigned addr=global->get_address();
         const type_info *type = global->type();
         type_info_key ti=type->get_key();
         size_t size;
         int t;
         ti.type_decode(size,t);
         int nbytes = size/8;
         int offset=0;
         std::list<operand_info> init_list = global->get_initializer();
         for ( std::list<operand_info>::iterator i=init_list.begin(); i!=init_list.end(); i++ ) {
            operand_info op = *i;
            ptx_reg_t value = op.get_literal_value();
            if (!g_use_host_memory_space) {
               assert( (addr+offset+nbytes) < min_gaddr ); // min_gaddr is start of "heap" for cudaMalloc
            }
            gpu->get_global_memory()->write(addr+offset,nbytes,&value,NULL,NULL); // assuming little endian here
            offset+=nbytes;
            ng_bytes+=nbytes;
         }
         printf(" wrote %u bytes\n", offset ); 
      }
   }
   printf( "GPGPU-Sim PTX: finished loading globals (%u bytes total).\n", ng_bytes );
   fflush(stdout);
   return ng_bytes;
}

static int load_constants( symbol_table *symtab, addr_t min_gaddr, gpgpu_t *gpu ) 
{
	printf( "GPGPU-Sim PTX: loading constants with explicit initializers... " );
	fflush(stdout);
	int nc_bytes = 0;
	symbol_table::iterator g=symtab->const_iterator_begin();

   for ( ; g!=symtab->const_iterator_end(); g++) {
      symbol *constant = *g;
      if ( constant->is_const() ) {

         if (constant->has_initializer() ) {

            // get the constant element data size
            int basic_type;
            size_t num_bits;
            constant->type()->get_key().type_decode(num_bits,basic_type); 

            std::list<operand_info> init_list = constant->get_initializer();
            int nbytes_written = 0;
            for ( std::list<operand_info>::iterator i=init_list.begin(); i!=init_list.end(); i++ ) {
               operand_info op = *i;
               ptx_reg_t value = op.get_literal_value();
               int nbytes = num_bits/8;
               switch ( op.get_type() ) {
               case int_t: assert(nbytes >= 1); break;
               case float_op_t: assert(nbytes == 4); break;
               case double_op_t: assert(nbytes >= 4); break; // account for double DEMOTING
               default:
                  abort();
               }
               unsigned addr=constant->get_address() + nbytes_written;
               if (!g_use_host_memory_space) {
            	   assert( addr+nbytes < min_gaddr );
               }

               gpu->get_global_memory()->write(addr,nbytes,&value,NULL,NULL); // assume little endian (so u8 is the first byte in u32)
               nc_bytes+=nbytes;
               nbytes_written += nbytes;
            }
         }
      }
   }
   //printf( " done.\n");
   fflush(stdout);
   return nc_bytes;
}

kernel_info_t *gpgpu_cuda_ptx_sim_init_grid( const char *hostFun, 
		gpgpu_ptx_sim_arg_list_t args,
		struct dim3 gridDim,
		struct dim3 blockDim,
		CUctx_st* context )
{
	function_info *entry = context->get_kernel(hostFun);
	kernel_info_t *result = new kernel_info_t(gridDim,blockDim,entry);
	if( entry == NULL ) {
		printf("GPGPU-Sim PTX: ERROR launching kernel -- no PTX implementation found for %p\n", hostFun);
		abort();
	}
	unsigned argcount=args.size();
	unsigned argn=1;
	for( gpgpu_ptx_sim_arg_list_t::iterator a = args.begin(); a != args.end(); a++ ) {
		entry->add_param_data(argcount-argn,&(*a));
		argn++;
	}

	entry->finalize(result->get_param_memory());
	g_ptx_kernel_count++;
	fflush(stdout);

	return result;
}

void virtualMemoryDeclaration( void* start_of_vm,
                           size_t total_size,
                           size_t field_size,
                           size_t tile_width,
                           size_t object_size,
                           size_t page_size,
                           boost::dynamic_bitset<> hotbytes) {
    CUctx_st* context = GPGPUSim_Context();
    context->get_device()->get_gpgpu()->handle_static_virtual_page( start_of_vm, total_size, field_size, tile_width, object_size, page_size, hotbytes );
}
