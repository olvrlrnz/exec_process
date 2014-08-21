/**
 * =============================================================================
 *
 *       Filename:  compiler.c
 *
 *    Description:  Collection of common compiler defines
 *
 *        Version:  1.0
 *        Created:  08/21/2014 05:45:49 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Oliver Lorenz (ol), olli@olorenz.org
 *
 * =============================================================================
 */

#ifndef COMPILER_H
#define COMPILER_H


#define likely(x)		__builtin_expect((x),1)
#define unlikely(x)		__builtin_expect((x),0)

#define _used			_unused
#define _unused			__attribute__((__unused__))
#define _sentinel		__attribute__((__sentinel__(0)))
#define _transparent_union	__attribute__((__transparent_union__))

#define ARRAY_SIZE(a)		(sizeof(a)/sizeof((a)[0]) + __must_be_array(a))
#define __must_be_array(a)	BUILD_BUG_ON_ZERO(__same_type((a), &(a)[0]))
#define __same_type(a, b)	__builtin_types_compatible_p(typeof(a), typeof(b))
#define BUILD_BUG_ON_ZERO(e)	(sizeof(struct { int:-!!(e); }))


#endif

