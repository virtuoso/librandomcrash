#ifndef STATIC_MALLOC_H
#define STATIC_MALLOC_H

/*
  Static Memory Pool Malloc

  This is just like malloc except it lacks sbrk() facilities.

  Instread, it is given a single, (client-code) pre-allocated block of memory
  to work with.

  Large parts of this code is stolen from Mark B. Hansen, and adapted
  to use static memory pools.  The original is under the malloc() link at 
  http://www.panix.com/~mbh/projects.html.

  Since he didn't specify a license for this code, I cannot 
  (in good conscience) go ahead and GPL it.  Instead I offer it as 
  free public domain.  See COPYRIGHT file for details..

  Calin A. Culianu <calin@ajvar.org>
  April 2003

  Last Updated: July 2008  
*/

#ifdef __cplusplus
extern "C" {
#endif

typedef void * address_t;
typedef unsigned long int size_type;
struct smalloc_t;
typedef struct smalloc_t smalloc_t;

/* The following three functions are just like malloc() and free() */
extern address_t smalloc(smalloc_t *, size_type);
extern void      sfree(smalloc_t *, const address_t);
extern void      scfree(smalloc_t *, const address_t);


typedef int (*printf_fn_t)(const char *, ...) 
#ifdef __GNUC__
         __attribute((format(printf,1,2)))
#endif
;


/* 
   Call the below to initialize the static memory pool allocator.

   Pass it an initial chunk of memory to work with.  Make sure that size
   is at least 512 or else results are undefined (well, NULL is returned).
   
   The printf_fn_t is optional and can be set to NULL.

   Note: Returned smalloc_t is actually *inside* the chunk of memory 
   you passed in!  */
extern smalloc_t * smalloc_init(address_t memory, size_type size, printf_fn_t);
/* Undoes the smalloc_init() above.  For now, calling this is optional */
extern void    smalloc_destroy(smalloc_t *);
/* Add more memory to the allocator dynamically! */
extern void    smalloc_add_memory_chunk(smalloc_t *, address_t memory, size_type size);

/* Returns the number of bytes allocated. */
extern unsigned long smalloc_get_allocated(smalloc_t *);
/* Returns the number of bytes free. */
extern unsigned long smalloc_get_free(smalloc_t *);
/* Returns the number of contiguous areas that are free. */
extern unsigned smalloc_get_n_free_areas(smalloc_t *);
/* Returns the number of individual areas that are allocated. */
extern unsigned smalloc_get_n_allocated_areas(smalloc_t *);

#ifdef __cplusplus
}
#endif

#endif
