/*
 * Copyright 2010 Andrea Mazzoleni. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY ANDREA MAZZOLENI AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL ANDREA MAZZOLENI OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
 
/**
 * Tommy benchmark program.
 *
 * Simply run it without any options. It will generate a set of *.lst files
 * suitable for producing graphs with gnuplot or another program.
 *
 * This program is supposed to be also compiled with a C++ compiler,
 * to be able to use the Google dense_hash_map templates.
 * For this reason it contains a lot of unnecessary casting for C of void* pointers.
 */

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <time.h>
#include <errno.h>
#include <assert.h>

#if defined(__linux)
#include <stdint.h>
#include <unistd.h>
#include <sys/time.h>
#endif

#if defined(__MACH__)
#include <mach/mach_time.h>
#endif

#if defined(_WIN32)
#include <windows.h>
#if defined(_M_IX86) || defined(__i386__)
#define USE_JUDY
#endif
#endif

#ifdef __cplusplus
#if !defined(__MACH__)
#define USE_CCGOOGLE
#endif
#else
#define USE_CGOOGLE
#endif

/******************************************************************************/
/* data structure */

/* Tommy data structures */
/* We directly include the C file to have functions automatically */
/* expanded inline by the compiler like other implementations */
#include "tommy.h"
#include "tommy.c"

/* Google C dense hash table */
/* http://code.google.com/p/google-sparsehash/ in the experimental/ directory */
/* Disabled by default because it's superseeded by the C++ version. */
/* Note that it has a VERY BAD performance on the "Change" test. Consider to use khash if you need a C implementation. */
#ifdef USE_CGOOGLE
#define htonl(x) 0
#define ntohl(x) 0
#define Table(x) Dense##x /* Use google dense tables */
#include "benchmark/lib/google/libchash.c" 
#endif

/* Google C++ dense hash table. */
/* http://code.google.com/p/google-sparsehash/ */
/* Note that after erasing we always call resize(0) to possibly trigger a table resize to free some space */
/* Otherwise, it would be an unfair advantage never shrinking on deletion. */
/* The shrink is triggered only sometimes, so the performance doesn't suffer to much */
#ifdef USE_CCGOOGLE
#include <google/dense_hash_map>
#endif

/* UTHASH */
/* http://uthash.sourceforge.net/ */
#include "benchmark/lib/uthash/uthash.h"

/* Nedtries */
/* http://www.nedprod.com/programs/portable/nedtries/ */
/* Note that I fixed a crash bug when inserting objects with 0 key */
/* This fix is now in the nedtries github */
/* https://github.com/ned14/nedtries/commit/21039696f27db4ffac70a82f89dc5d00ae74b332 */
/* We do not use the C++ implementation as it doesn't compile with gcc 4.4.3 */
#include "benchmark/lib/nedtries/nedtrie.h"

/* KHash */
/* http://attractivechaos.awardspace.com/ */
/* It has the unfair advantage on remove by never shrinking when deleting objects. */
#define inline __inline
#include "benchmark/lib/khash/khash.h"

/* RBtree */
/* http://www.canonware.com/rb/ */
/* We are using the most recent implementation rb_never/rb.h. Not rb_old/ or rb_new/ */
#define RB_COMPACT
#define ssize_t size_t
#define bool int
#define true 1
#define false 0
#include "benchmark/lib/rb/rb_newer.h"

/* Judy */
/* http://judy.sourceforge.net/ */
/* Disabled on 64 bits platform in Windows as it's not supported */
#ifdef USE_JUDY
#include "benchmark/lib/judy/judy.h"
#endif

/******************************************************************************/
/* objects */

#define PAYLOAD 16 /**< Size of payload data for objects */

struct rbt_object {
	rb_node(struct rbt_object) link;
	unsigned value;
	char payload[PAYLOAD];
};

struct hashtable_object {
	tommy_node node;
	unsigned value;
	char payload[PAYLOAD];
};

struct uthash_object {
	UT_hash_handle hh;
	unsigned value;
	char payload[PAYLOAD];
};

struct khash_object {
	unsigned value;
	char payload[PAYLOAD];
};

struct google_object {
	unsigned value;
	char payload[PAYLOAD];
};

struct trie_object {
	tommy_trie_node node;
	unsigned value;
	char payload[PAYLOAD];
};

struct trie_inplace_object {
	tommy_trie_inplace_node node;
	unsigned value;
	char payload[PAYLOAD];
};

struct judy_object {
	unsigned value;
	char payload[PAYLOAD];
};

struct nedtrie_object {
	NEDTRIE_ENTRY(nedtrie_object) link;
	unsigned value;   
	char payload[PAYLOAD];
};

struct hashnedtrie_object {
	NEDTRIE_ENTRY(hashnedtrie_object) link;
	unsigned value;
	tommy_hash_t hash;
	char payload[PAYLOAD];
};

struct rbt_object* RBTREE;
struct hashtable_object* HASHTABLE;
struct hashtable_object* HASHDYN;
struct hashtable_object* HASHLIN;
struct trie_object* TRIE;
struct trie_inplace_object* TRIE_INPLACE;
struct khash_object* KHASH;
struct google_object* GOOGLE;
struct uthash_object* UTHASH;
struct nedtrie_object* NEDTRIE;
#ifdef USE_JUDY
struct judy_object* JUDY;
#endif

/******************************************************************************/
/* containers */

int rbt_compare(const void* void_a, const void* void_b)
{
	const struct rbt_object* a = (const struct rbt_object*)void_a;
	const struct rbt_object* b = (const struct rbt_object*)void_b;

	int va = a->value;
	int vb = b->value;

	if (va < vb)
		return -1;
	if (va > vb)
		return 1;
	return 0;
}

int tommy_hashtable_compare(const void* void_arg, const void* void_obj)
{
	const unsigned* arg = (const unsigned*)void_arg;
	const struct hashtable_object* obj = (const struct hashtable_object*)void_obj;

	if (*arg == obj->value)
		return 0;

	return 1;
}

typedef rbt(struct rbt_object) rbtree_t;

rb_gen(static, rbt_, rbtree_t, struct rbt_object, link, rbt_compare)

NEDTRIE_HEAD(nedtrie_t, nedtrie_object);

static size_t nedtrie_func(const struct nedtrie_object* r)
{
	return r->value;
}

NEDTRIE_GENERATE(static, nedtrie_t, nedtrie_object, link, nedtrie_func, NEDTRIE_NOBBLEZEROS(nedtrie_t));

KHASH_MAP_INIT_INT(word, struct khash_object*)

rbtree_t tree;
tommy_hashtable hashtable;
tommy_hashdyn hashdyn;
tommy_hashlin hashlin;
tommy_allocator trie_allocator;
tommy_trie trie;
tommy_trie_inplace trie_inplace;
struct uthash_object* uthash = 0;
struct nedtrie_t nedtrie;
khash_t(word)* khash;
#ifdef USE_CGOOGLE
struct HashTable* cgoogle;
#endif
#ifdef USE_CCGOOGLE
/* use a specialized hash, otherwise the performance depends on the STL implementation used. */
class custom_hash_compare {
public:
	size_t operator()(const tommy_key_t key) const { return tommy_inthash_u32(key); }
};
typedef google::dense_hash_map<unsigned, struct google_object*, custom_hash_compare> ccgoogle_t;
ccgoogle_t* ccgoogle;
#endif
#ifdef USE_JUDY
Pvoid_t judy = 0;
#endif

/******************************************************************************/
/* time */

#if defined(_WIN32)
static LARGE_INTEGER win_frequency; 
#endif

static void nano_init(void)
{
#if defined(_WIN32)
	if (!QueryPerformanceFrequency(&win_frequency)) {
		win_frequency.QuadPart = 0;
	}
#endif
}

static tommy_uint64_t nano(void)
{
	tommy_uint64_t ret;
#if defined(_WIN32)   
	LARGE_INTEGER t;

	if (!QueryPerformanceCounter(&t))
		return 0;

	ret = (t.QuadPart / win_frequency.QuadPart) * 1000000000;

	ret += (t.QuadPart % win_frequency.QuadPart) * 1000000000 / win_frequency.QuadPart;
#elif defined(__MACH__)
	mach_timebase_info_data_t info;
	kern_return_t r;
	tommy_uint64_t t;

	t = mach_absolute_time();

	r = mach_timebase_info(&info);
	if (r != 0) {
		abort();
	}

	ret = (t / info.denom) * info.numer;
	
	ret += (t % info.denom) * info.numer / info.denom;
#elif defined(__linux)
	struct timespec ts;
	int r;

	r = clock_gettime(CLOCK_MONOTONIC, &ts);
	if (r != 0) {
		abort();
	}

	ret = ts.tv_sec * (tommy_uint64_t)1000000000 + ts.tv_nsec;
#else
	struct timeval tv;
	int r;

	r = gettimeofday(&tv, 0);
	if (r != 0) {
		abort();
	}

	ret = tv.tv_sec * (tommy_uint64_t)1000000000 + tv.tv_usec * 1000;
#endif
	return ret;
}

/******************************************************************************/
/* random */

/**
 * Pseudo random number generator.
 * Note that using (rand() % max) in Visual C results in totally bogus values, 
 * with *strong* cache effects when accessing elements in a not really random order.
 * This happen because Visual C uses a simple linear congruential generator with only 32 bits.
 */
tommy_uint64_t SEED = 0;

unsigned rnd(unsigned max) 
{
	unsigned r;
	tommy_uint64_t divider;
    
loop:    
	/* linear congruential generator from MMIX by Donald Knuth, http://en.wikipedia.org/wiki/Linear_congruential_generator */
#ifdef _MSC_VER
	divider = 0xFFFFFFFFFFFFFFFF / max;
	SEED = SEED * 6364136223846793005 + 1442695040888963407;
#else
	divider = 0xFFFFFFFFFFFFFFFFULL / max;
	SEED = SEED * 6364136223846793005LL + 1442695040888963407LL;
#endif
 
	r = (unsigned)(SEED / divider);

	/* it may happen as the divider is approximated down */
	if (r >= max)
		goto loop;

	return r;
}

/******************************************************************************/
/* test */

#ifdef _DEBUG
#define MAX 100000
#else
#define MAX 10000000
#endif

/**
 * Max number of retries.
 */
#define RETRY_MAX 3

/**
 * Operations.
 */
#define OPERATION_INSERT 0
#define OPERATION_HIT 1
#define OPERATION_MISS 2
#define OPERATION_SIZE 3
#define OPERATION_CHANGE 4
#define OPERATION_REMOVE 5
#define OPERATION_MAX 6

const char* OPERATION_NAME[OPERATION_MAX] = {
	"insert",
	"hit",
	"miss",
	"size",
	"change",
	"remove",
};

/**
 * Orders.
 */
#define ORDER_FORWARD 0
#define ORDER_RANDOM 1
#define ORDER_MAX 2

const char* ORDER_NAME[ORDER_MAX] = {
	"forward",
	"random",
};

/**
 * Data structures.
 */
#define DATA_HASHTABLE 0
#define DATA_HASHDYN 1
#define DATA_HASHLIN 2
#define DATA_TRIE 3
#define DATA_TRIE_INPLACE 4
#define DATA_TREE 5
#define DATA_KHASH 6
#define DATA_CGOOGLE 7
#define DATA_CCGOOGLE 8
#define DATA_UTHASH 9
#define DATA_NEDTRIE 10
#define DATA_JUDY 11
#define DATA_MAX 12

const char* DATA_NAME[DATA_MAX] = {
	"tommy-hashtable",
	"tommy-hashdyn",
	"tommy-hashlin",
	"tommy-trie",
	"tommy-trie-inplace",
	"rbtree",
	"khash",
	"cgoogledensehash",
	"googledensehash",
	"uthash",
	"nedtrie",
	"judy",
};

/** 
 * Logged data.
 */
unsigned LOG[RETRY_MAX][DATA_MAX][ORDER_MAX][OPERATION_MAX];

/**
 * Time limit in nanosecond. 
 * We stop measuring degenerated cases after this limit.
 */
#define TIME_MAX_NS 1500

/**
 * Last measure.
 * Used to stop degenerated cases.
 */
unsigned LAST[DATA_MAX][ORDER_MAX];

/**
 * Test state.
 */
unsigned the_retry; /**< Number of retry. */
unsigned the_data;  /**< Data struture in test. */
unsigned the_operation; /**< Operation in test. */
unsigned the_order; /**< Order in test. */
unsigned the_max; /**< Number of elements in test. */
tommy_uint64_t the_time; /**< Start time of the test. */
unsigned the_start_data; /**< Saved data structure in the last START command, simply to avoid to repeat it for STOP. */

/**
 * Control flow state.
 */
tommy_bool_t the_log;

/** 
 * If the data structure should be in the graph, even if with no data.
 * It allows to have equal graphs also if in some platforms a data structure is not available.
 */
tommy_bool_t is_listed(unsigned data)
{
	(void)data;

	/* always have all the columns, we exclude them in the graphs */
	return 1;
}

/**
 * If the data structure should be measured.
 */
tommy_bool_t is_select(unsigned data)
{
	return the_data == data;
}

tommy_bool_t start(unsigned data)
{
	the_start_data = data;

	if (!is_select(the_start_data))
		return 0;

	if (!the_log)
		printf("%10s, %10s, %12s, ", ORDER_NAME[the_order], OPERATION_NAME[the_operation], DATA_NAME[data]);

	the_time = nano();
	return 1;
}

void stop(void)
{
	tommy_uint64_t elapsed = nano() - the_time;

	if (!is_select(the_start_data))
		return;

	if (!the_log) {
		printf("%4u [ns]\n", (unsigned)(elapsed / the_max));
	} 

	LOG[the_retry][the_data][the_order][the_operation] = (unsigned)(elapsed / the_max);
}

void mem(unsigned data, tommy_size_t v)
{
	if (!the_log) {
		printf("%10s, %10s, %12s, ", ORDER_NAME[the_order], OPERATION_NAME[the_operation], DATA_NAME[data]);
		printf("%4u [byte]\n", (unsigned)(v / the_max));
	} 

	LOG[the_retry][the_data][the_order][the_operation] = (unsigned)(v / the_max);
}

#define COND(s) if (is_select(s))
#define START(s) if (start(s)) for(i=0;i<the_max;++i)
#define STOP() stop()
#define MEM(s, v) if (is_select(s)) mem(s, v)
#define OPERATION(operation) the_operation = operation
#define ORDER(order) the_order = order
#define DATA(data) the_data = data

FILE* open(const char* mode)
{
	char buf[128];
	sprintf(buf, "dat_%s_%s.lst", ORDER_NAME[the_order], OPERATION_NAME[the_operation]);
	return fopen(buf, mode);
}

/******************************************************************************/
/* test */

/* default hash function used */
#define hash(v) tommy_inthash_u32(v)

/**
 * Cache clearing buffer.
 */
unsigned char CACHE[8*1024*1024];

void cache_clear(void)
{
	unsigned i;

	/* read & write */
	for(i=0;i<sizeof(CACHE);i += 32)
		CACHE[i] += 1;

#ifdef WIN32
	Sleep(0);
#endif
}

/**
 * Different orders used.
 */ 
unsigned* FORWARD;
unsigned* RAND0;
unsigned* RAND1;

void order_init(int max, int sparse)
{
	int i;

	FORWARD = (unsigned*)malloc(max * sizeof(unsigned));
	RAND0 = (unsigned*)malloc(max * sizeof(unsigned));
	RAND1 = (unsigned*)malloc(max * sizeof(unsigned));

	/* generated forward orders */
	if (sparse) {
		for(i=0;i<max;++i) {
			FORWARD[i] = 0xffffffff / max * i;
			RAND0[i] = FORWARD[i];
			RAND1[i] = FORWARD[i];
		}
	} else {
		for(i=0;i<max;++i) {
			FORWARD[i] = 0x80000000 + i * 2;
			RAND0[i] = FORWARD[i];
			RAND1[i] = FORWARD[i];
		}	
	}

	/* randomize using the Fisher�Yates shuffle, http://en.wikipedia.org/wiki/Fisher-Yates_shuffle */
	for(i=max-1;i>=0;--i) {
		unsigned j, t;
		j = rnd(i+1);
		t = RAND0[i];
		RAND0[i] = RAND0[j];
		RAND0[j] = t;

		j = rnd(i+1);
		t = RAND1[i];
		RAND1[i] = RAND1[j];
		RAND1[j] = t;
	}
}

void order_done(void)
{
	free(FORWARD);
	free(RAND0); 
	free(RAND1); 
}

void test_alloc(void)
{
	COND(DATA_TREE) {
		rbt_new(&tree);
		RBTREE = (struct rbt_object*)malloc(sizeof(struct rbt_object) * the_max);
	}

	COND(DATA_HASHTABLE) {
		tommy_hashtable_init(&hashtable, 2 * the_max);
		HASHTABLE = (struct hashtable_object*)malloc(sizeof(struct hashtable_object) * the_max);
	}

	COND(DATA_HASHDYN) {
		tommy_hashdyn_init(&hashdyn);
		HASHDYN = (struct hashtable_object*)malloc(sizeof(struct hashtable_object) * the_max);
	}

	COND(DATA_HASHLIN) {
		tommy_hashlin_init(&hashlin);
		HASHLIN = (struct hashtable_object*)malloc(sizeof(struct hashtable_object) * the_max);
	}

	COND(DATA_TRIE) {
		tommy_allocator_init(&trie_allocator, TOMMY_TRIE_BLOCK_SIZE, TOMMY_TRIE_BLOCK_SIZE);
		tommy_trie_init(&trie, &trie_allocator);
		TRIE = (struct trie_object*)malloc(sizeof(struct trie_object) * the_max);
	}

	COND(DATA_TRIE_INPLACE) {
		tommy_trie_inplace_init(&trie_inplace);
		TRIE_INPLACE = (struct trie_inplace_object*)malloc(sizeof(struct trie_inplace_object) * the_max);
	}

	COND(DATA_KHASH) {
		KHASH = (struct khash_object*)malloc(sizeof(struct khash_object) * the_max);
		khash = kh_init(word);
	}

#ifdef USE_CGOOGLE
	COND(DATA_CGOOGLE) {
		GOOGLE = (struct google_object*)malloc(sizeof(struct google_object) * the_max);
		cgoogle = AllocateHashTable(sizeof(void*), 0);
	}
#endif

#ifdef USE_CCGOOGLE
	COND(DATA_CCGOOGLE) {
		GOOGLE = (struct google_object*)malloc(sizeof(struct google_object) * the_max);
		ccgoogle = new ccgoogle_t;
		ccgoogle->set_empty_key(-1);
		ccgoogle->set_deleted_key(-2);
	}
#endif

	COND(DATA_UTHASH) {
		UTHASH = (struct uthash_object*)malloc(sizeof(struct uthash_object) * the_max);
	}

	COND(DATA_NEDTRIE) {
		NEDTRIE_INIT(&nedtrie);
		NEDTRIE = (struct nedtrie_object*)malloc(sizeof(struct nedtrie_object) * the_max);
	}

#ifdef USE_JUDY
	COND(DATA_JUDY) {
		JUDY = (struct judy_object*)malloc(sizeof(struct judy_object) * the_max);
	}
#endif
}

void test_free(void)
{
	COND(DATA_TREE) {
		free(RBTREE);
	}

	COND(DATA_HASHTABLE) {
		if (tommy_hashtable_count(&hashtable) != 0)
			abort();
		tommy_hashtable_done(&hashtable);
		free(HASHTABLE);
	}

	COND(DATA_HASHDYN) {
		if (tommy_hashdyn_count(&hashdyn) != 0)
			abort();
		tommy_hashdyn_done(&hashdyn);
		free(HASHDYN);
	}

	COND(DATA_HASHLIN) {
		if (tommy_hashlin_count(&hashlin) != 0)
			abort();
		tommy_hashlin_done(&hashlin);
		free(HASHLIN);
	}

	COND(DATA_TRIE) {
		if (tommy_trie_count(&trie) != 0)
			abort();
		tommy_allocator_done(&trie_allocator);
		tommy_trie_done(&trie);
		free(TRIE);
	}

	COND(DATA_TRIE_INPLACE) {
		if (tommy_trie_inplace_count(&trie_inplace) != 0)
			abort();
		free(TRIE_INPLACE);
	}

	COND(DATA_KHASH) {
		kh_destroy(word, khash);
		free(KHASH);
	}

#ifdef USE_CGOOGLE
	COND(DATA_CGOOGLE) {
		FreeHashTable(cgoogle);
		free(GOOGLE);
	}
#endif

#ifdef USE_CCGOOGLE
	COND(DATA_CCGOOGLE) {
		free(GOOGLE);
		delete ccgoogle;
	}
#endif

	COND(DATA_UTHASH) {
		free(UTHASH);
	}

	COND(DATA_NEDTRIE) {
		free(NEDTRIE);
	}

#ifdef USE_JUDY
	COND(DATA_JUDY) {
		free(JUDY);
	}
#endif
}

void test_insert(unsigned* INSERT)
{
	unsigned i;

	START(DATA_TREE) {
		unsigned key = INSERT[i];
		RBTREE[i].value = key;
		rbt_insert(&tree, &RBTREE[i]);
	} STOP();

	START(DATA_HASHTABLE) {
		unsigned key = INSERT[i];
		unsigned hash_key = hash(key);
		HASHTABLE[i].value = key;
		tommy_hashtable_insert(&hashtable, &HASHTABLE[i].node, &HASHTABLE[i], hash_key);
	} STOP();

	START(DATA_HASHDYN) {
		unsigned key = INSERT[i];
		unsigned hash_key = hash(key);
		HASHDYN[i].value = key;
		tommy_hashdyn_insert(&hashdyn, &HASHDYN[i].node, &HASHDYN[i], hash_key);
	} STOP();

	START(DATA_HASHLIN) {
		unsigned key = INSERT[i];
		unsigned hash_key = hash(key);
		HASHLIN[i].value = key;
		tommy_hashlin_insert(&hashlin, &HASHLIN[i].node, &HASHLIN[i], hash_key);
	} STOP();

	START(DATA_TRIE) {
		unsigned key = INSERT[i];
		TRIE[i].value = key;
		tommy_trie_insert(&trie, &TRIE[i].node, &TRIE[i], key);
	} STOP();

	START(DATA_TRIE_INPLACE) {
		unsigned key = INSERT[i];
		TRIE_INPLACE[i].value = key;
		tommy_trie_inplace_insert(&trie_inplace, &TRIE_INPLACE[i].node, &TRIE_INPLACE[i], key);
	} STOP();

	/* for khash we hash the key because internally khash doesn't use any hash for integer keys */
	START(DATA_KHASH) {
		unsigned key = INSERT[i];
		unsigned hash_key = hash(key);
		khiter_t k;
		int r;
		KHASH[i].value = key;
		k = kh_put(word, khash, hash_key, &r);
		if (!r)
			abort();
		kh_value(khash, k) = &KHASH[i];
	} STOP();

#ifdef USE_CGOOGLE
	START(DATA_CGOOGLE) {
		unsigned key = INSERT[i];
		HTItem* r;
		u_long ptr_value = (u_long)&GOOGLE[i];
		GOOGLE[i].value = key;
		r = HashInsert(cgoogle, key, ptr_value);
		if (!r)
			abort();
	} STOP();
#endif

#ifdef USE_CCGOOGLE
	START(DATA_CCGOOGLE) {
		unsigned key = INSERT[i];
		struct google_object* obj = &GOOGLE[i];
		GOOGLE[i].value = key;
		(*ccgoogle)[key] = obj;
	} STOP();
#endif

	START(DATA_UTHASH) {
		unsigned key = INSERT[i];
		struct uthash_object* obj = &UTHASH[i];
		obj->value = key;
		HASH_ADD_INT(uthash, value, obj);
	} STOP();

	START(DATA_NEDTRIE) {
		unsigned key = INSERT[i];
		NEDTRIE[i].value = key;
		NEDTRIE_INSERT(nedtrie_t, &nedtrie, &NEDTRIE[i]);
	} STOP();

#ifdef USE_JUDY
	START(DATA_JUDY) {
		unsigned key = INSERT[i];
		Pvoid_t PValue;
		JUDY[i].value = key;
		JLI(PValue, judy, key);
		*(struct judy_object**)PValue = &JUDY[i];
	} STOP();
#endif
}

void test_hit(unsigned* SEARCH)
{
	unsigned i;

	/* always dereference the object found. It has cache effect. */
	/* considering we are dealing with objects, it makes sense to simulate an access to it */
	/* this favorites data structures that store part of the information in the object itself */
	const int dereference = 1; 

	START(DATA_TREE) {
		unsigned key = SEARCH[i];
		struct rbt_object key_obj;
		struct rbt_object* obj;
		key_obj.value = key;
		obj = rbt_search(&tree, &key_obj); 
		if (!obj)
			abort();
		if (dereference) {
			if (obj->value != key)
				abort();
		}
	} STOP();

	START(DATA_HASHTABLE) {
		unsigned key = SEARCH[i];
		unsigned hash_key = hash(key);
		struct hashtable_object* obj;
		obj = (struct hashtable_object*)tommy_hashtable_search(&hashtable, tommy_hashtable_compare, &key, hash_key);
		if (!obj)
			abort();
		if (dereference) {
			if (obj->value != key)
				abort();
		}
	} STOP();

	START(DATA_HASHDYN) {
		unsigned key = SEARCH[i];
		unsigned hash_key = hash(key);
		struct hashtable_object* obj;
		obj = (struct hashtable_object*)tommy_hashdyn_search(&hashdyn, tommy_hashtable_compare, &key, hash_key);
		if (!obj)
			abort();
		if (dereference) {
			if (obj->value != key)
				abort();
		}
	} STOP();

	START(DATA_HASHLIN) {
		unsigned key = SEARCH[i];
		unsigned hash_key = hash(key);
		struct hashtable_object* obj;
		obj = (struct hashtable_object*)tommy_hashlin_search(&hashlin, tommy_hashtable_compare, &key, hash_key);
		if (!obj)
			abort();
		if (dereference) {
			if (obj->value != key)
				abort();
		}
	} STOP();

	START(DATA_TRIE) {
		unsigned key = SEARCH[i];
		struct trie_object* obj;
		obj = (struct trie_object*)tommy_trie_search(&trie, key);
		if (!obj)
			abort();
		if (dereference) {
			if (obj->value != key)
				abort();
		}
	} STOP();

	START(DATA_TRIE_INPLACE) {
		unsigned key = SEARCH[i];
		struct trie_inplace_object* obj;
		obj = (struct trie_inplace_object*)tommy_trie_inplace_search(&trie_inplace, key);
		if (!obj)
			abort();
		if (dereference) {
			if (obj->value != key)
				abort();
		}
	} STOP();

	START(DATA_KHASH) {
		unsigned key = SEARCH[i];
		unsigned hash_key = hash(key);
		khiter_t k;
		k = kh_get(word, khash, hash_key);
		if (k == kh_end(khash))
			abort();
		if (dereference) {
			struct khash_object* obj = kh_value(khash, k);
			if (obj->value != key)
				abort();
		}
	} STOP();

#ifdef USE_CGOOGLE
	START(DATA_CGOOGLE) {
		unsigned key = SEARCH[i];
		HTItem* ptr;
		ptr = HashFind(cgoogle, key);
		if (!ptr)
			abort();
		if (dereference) {
			struct google_object* obj = (void*)ptr->data;
			if (obj->value != key)
				abort();
		}
	} STOP();
#endif

#ifdef USE_CCGOOGLE
	START(DATA_CCGOOGLE) {
		unsigned key = SEARCH[i];
		ccgoogle_t::const_iterator ptr = ccgoogle->find(key);
		if (ptr == ccgoogle->end())
			abort();
		if (dereference) {
			struct google_object* obj = ptr->second;
			if (obj->value != key)
				abort();
		}
	} STOP();
#endif

	START(DATA_UTHASH) {
		unsigned key = SEARCH[i];
		struct uthash_object* obj;
		HASH_FIND_INT(uthash, &key, obj);  
		if (!obj)
			abort();
		if (dereference) {
			if (obj->value != key)
				abort();
		}
	} STOP();

#ifdef USE_JUDY
	START(DATA_JUDY) {
		Word_t key = SEARCH[i];
		Pvoid_t PValue;
		JLG(PValue, judy, key);
		if (!PValue)
			abort();
		if (dereference) {
			struct judy_object* obj = *(struct judy_object**)PValue;
			if (obj->value != key)
				abort();
		}
	} STOP();
#endif

	START(DATA_NEDTRIE) {
		unsigned key = SEARCH[i];
		struct nedtrie_object key_obj;
		struct nedtrie_object* obj;
		key_obj.value = key;
		obj = NEDTRIE_FIND(nedtrie_t, &nedtrie, &key_obj);
		if (!obj)
			abort();
		if (dereference) {
			if (obj->value != key)
				abort();
		}
	} STOP();
}

void test_miss(unsigned* SEARCH, unsigned DELTA)
{
	unsigned i;

	START(DATA_TREE) {
		struct rbt_object key;
		key.value = SEARCH[i] + DELTA;
		if (rbt_search(&tree, &key))
			abort();
	} STOP();

	START(DATA_HASHTABLE) {
		unsigned key = SEARCH[i] + DELTA;
		unsigned hash_key = hash(key);
		struct hashtable_object* obj;
		obj = (struct hashtable_object*)tommy_hashtable_search(&hashtable, tommy_hashtable_compare, &key, hash_key);
		if (obj)
			abort();
	} STOP();

	START(DATA_HASHDYN) {
		unsigned key = SEARCH[i] + DELTA;
		unsigned hash_key = hash(key);
		struct hashtable_object* obj;
		obj = (struct hashtable_object*)tommy_hashdyn_search(&hashdyn, tommy_hashtable_compare, &key, hash_key);
		if (obj)
			abort();
	} STOP();

	START(DATA_HASHLIN) {
		unsigned key = SEARCH[i] + DELTA;
		unsigned hash_key = hash(key);
		struct hashtable_object* obj;
		obj = (struct hashtable_object*)tommy_hashlin_search(&hashlin, tommy_hashtable_compare, &key, hash_key);
		if (obj)
			abort();
	} STOP();

	START(DATA_TRIE) {
		struct trie_object* obj;
		obj = (struct trie_object*)tommy_trie_search(&trie, SEARCH[i] + DELTA);
		if (obj)
			abort();
	} STOP();

	START(DATA_TRIE_INPLACE) {
		struct trie_inplace_object* obj;
		obj = (struct trie_inplace_object*)tommy_trie_inplace_search(&trie_inplace, SEARCH[i] + DELTA);
		if (obj)
			abort();
	} STOP();

	START(DATA_KHASH) {
		unsigned key = SEARCH[i] + DELTA;
		unsigned hash_key = hash(key);
		khiter_t k;
		k = kh_get(word, khash, hash_key);
		if (k != kh_end(khash))
			abort();
	} STOP();

#ifdef USE_CGOOGLE
	START(DATA_CGOOGLE) {
		unsigned key = SEARCH[i] + DELTA;
		HTItem* ptr;
		ptr = HashFind(cgoogle, key);
		if (ptr)
			abort();
	} STOP();
#endif

#ifdef USE_CCGOOGLE
	START(DATA_CCGOOGLE) {
		unsigned key = SEARCH[i] + DELTA;
		ccgoogle_t::const_iterator ptr = ccgoogle->find(key);
		if (ptr != ccgoogle->end())
			abort();
	} STOP();
#endif

	START(DATA_UTHASH) {
		unsigned key = SEARCH[i] + DELTA;
		struct uthash_object* obj;
		HASH_FIND_INT(uthash, &key, obj);  
		if (obj)
			abort();
	} STOP();

#ifdef USE_JUDY
	START(DATA_JUDY) {
		Word_t key = SEARCH[i] + DELTA;
		Pvoid_t PValue;
		JLG(PValue, judy, key);
		if (PValue)
			abort();
	} STOP();
#endif

	START(DATA_NEDTRIE) {
		unsigned key = SEARCH[i] + DELTA;
		struct nedtrie_object key_obj;
		struct nedtrie_object* obj;
		key_obj.value = key;
		obj = NEDTRIE_FIND(nedtrie_t, &nedtrie, &key_obj);
		if (obj)
			abort();
	} STOP();
}

void test_change(unsigned* REMOVE, unsigned* INSERT)
{
	unsigned i;

	const unsigned DELTA = 1;

	START(DATA_TREE) {
		unsigned key = REMOVE[i];
		struct rbt_object key_obj;
		struct rbt_object* obj;
		key_obj.value = REMOVE[i];
		obj = rbt_search(&tree, &key_obj); 
		if (!obj)
			abort();
		rbt_remove(&tree, obj);

		key = INSERT[i] + DELTA;  
		obj->value = key;
		rbt_insert(&tree, obj);
	} STOP();

	START(DATA_HASHTABLE) {
		unsigned key = REMOVE[i];
		unsigned hash_key = hash(key);
		struct hashtable_object* obj;
		obj = (struct hashtable_object*)tommy_hashtable_remove(&hashtable, tommy_hashtable_compare, &key, hash_key);
		if (!obj)
			abort();

		key = INSERT[i] + DELTA;
		hash_key = hash(key);
		obj->value = key;
		tommy_hashtable_insert(&hashtable, &obj->node, obj, hash_key);
	} STOP();

	START(DATA_HASHDYN) {
		unsigned key = REMOVE[i];
		unsigned hash_key = hash(key);
		struct hashtable_object* obj;
		obj = (struct hashtable_object*)tommy_hashdyn_remove(&hashdyn, tommy_hashtable_compare, &key, hash_key);
		if (!obj)
			abort();

		key = INSERT[i] + DELTA;
		hash_key = hash(key);
		obj->value = key;
		tommy_hashdyn_insert(&hashdyn, &obj->node, obj, hash_key);
	} STOP();

	START(DATA_HASHLIN) {
		unsigned key = REMOVE[i];
		unsigned hash_key = hash(key);
		struct hashtable_object* obj;
		obj = (struct hashtable_object*)tommy_hashlin_remove(&hashlin, tommy_hashtable_compare, &key, hash_key);
		if (!obj)
			abort();

		key = INSERT[i] + DELTA;
		hash_key = hash(key);
		obj->value = key;
		tommy_hashlin_insert(&hashlin, &obj->node, obj, hash_key);
	} STOP();

	START(DATA_TRIE) {
		unsigned key = REMOVE[i];
		struct trie_object* obj;
		obj = (struct trie_object*)tommy_trie_remove(&trie, key);
		if (!obj)
			abort();

		key = INSERT[i] + DELTA;
		obj->value = key;
		tommy_trie_insert(&trie, &obj->node, obj, key);
	} STOP();

	START(DATA_TRIE_INPLACE) {
		unsigned key = REMOVE[i];
		struct trie_inplace_object* obj;
		obj = (struct trie_inplace_object*)tommy_trie_inplace_remove(&trie_inplace, key);
		if (!obj)
			abort();

		key = INSERT[i] + DELTA;
		obj->value = key;
		tommy_trie_inplace_insert(&trie_inplace, &obj->node, obj, key);
	} STOP();

	START(DATA_KHASH) {
		unsigned key = REMOVE[i];
		unsigned hash_key = hash(key);
		khiter_t k;
		int r;
		struct khash_object* obj;
		k = kh_get(word, khash, hash_key);
		if (k == kh_end(khash))
			abort();
		obj = kh_value(khash, k);
		kh_del(word, khash, k);

		key = INSERT[i] + DELTA;
		hash_key = hash(key);
		obj->value = key;
		k = kh_put(word, khash, hash_key, &r);
		if (!r)
			abort();
		kh_value(khash, k) = obj;
	} STOP();

#ifdef USE_CGOOGLE
	START(DATA_CGOOGLE) {
		unsigned key = REMOVE[i];
		HTItem* ptr;
		struct google_object* obj;
		u_long ptr_value;
		ptr = HashFind(cgoogle, key);
		if (!ptr)
			abort();
		obj = (void*)ptr->data;
		HashDeleteLast(cgoogle);

		key = INSERT[i] + DELTA;
		obj->value = key;
		ptr_value = (u_long)obj;
		ptr = HashInsert(cgoogle, key, ptr_value);
		if (!ptr)
			abort();
	} STOP();
#endif

#ifdef USE_CCGOOGLE
	START(DATA_CCGOOGLE) {
		unsigned key = REMOVE[i];
		ccgoogle_t::iterator ptr = ccgoogle->find(key);
		struct google_object* obj;
		if (ptr == ccgoogle->end())
			abort();
		obj = ptr->second;
		ccgoogle->erase(ptr);

		key = INSERT[i] + DELTA;
		obj->value = key;
		(*ccgoogle)[key] = obj;
	} STOP();
#endif

	START(DATA_UTHASH) {
		unsigned key = REMOVE[i];
		struct uthash_object* obj;
		HASH_FIND_INT(uthash, &key, obj);
		if (!obj)
			abort();
		HASH_DEL(uthash, obj);

		key = INSERT[i] + DELTA;
		obj->value = key;
		HASH_ADD_INT(uthash, value, obj);
	} STOP();

#ifdef USE_JUDY
	START(DATA_JUDY) {
		Word_t key = REMOVE[i];
		struct judy_object* obj;
		int r;
		Pvoid_t PValue;
		JLG(PValue, judy, key);
		if (!PValue)
			abort();
		obj = *(struct judy_object**)PValue;
		JLD(r, judy, key);
		if (r != 1)
			abort();

		key = INSERT[i] + DELTA;
		obj->value = key;
		JLI(PValue, judy, key);
		*(struct judy_object**)PValue = obj;
	} STOP();
#endif

	START(DATA_NEDTRIE) {
		unsigned key = REMOVE[i];
		struct nedtrie_object key_obj;
		struct nedtrie_object* obj;
		key_obj.value = key;
		obj = NEDTRIE_FIND(nedtrie_t, &nedtrie, &key_obj);
		if (!obj)
			abort();
		NEDTRIE_REMOVE(nedtrie_t, &nedtrie, obj);

		key = INSERT[i] + DELTA;
		obj->value = key;
		NEDTRIE_INSERT(nedtrie_t, &nedtrie, obj);
	} STOP();
}

void test_remove(unsigned* REMOVE)
{
	unsigned i;

	const unsigned DELTA = 1;

	/* always dereference the object deleted. It has cache effect. */
	/* considering we are dealing with objects, it makes sense to simulate an access to it */
	/* even on deletion, because you have at least to do a free() call. */
	/* this favorites data structures that store part of the information in the object itself */
	const int dereference = 1; 

	START(DATA_TREE) {
		unsigned key = REMOVE[i] + DELTA;
		struct rbt_object key_obj;
		struct rbt_object* obj;
		key_obj.value = key;
		obj = rbt_search(&tree, &key_obj); 
		if (!obj)
			abort();
		if (dereference) {
			if (obj->value != key)
				abort();
		}
		rbt_remove(&tree, obj);
	} STOP();

	START(DATA_HASHTABLE) {
		unsigned key = REMOVE[i] + DELTA;
		unsigned hash_key = hash(key);
		struct hashtable_object* obj;
		obj = (struct hashtable_object*)tommy_hashtable_remove(&hashtable, tommy_hashtable_compare, &key, hash_key);
		if (!obj)
			abort();
		if (dereference) {
			if (obj->value != key)
				abort();
		}
	} STOP();

	START(DATA_HASHDYN) {
		unsigned key = REMOVE[i] + DELTA;
		unsigned hash_key = hash(key);
		struct hashtable_object* obj;
		obj = (struct hashtable_object*)tommy_hashdyn_remove(&hashdyn, tommy_hashtable_compare, &key, hash_key);
		if (!obj)
			abort();
		if (dereference) {
			if (obj->value != key)
				abort();
		}
	} STOP();

	START(DATA_HASHLIN) {
		unsigned key = REMOVE[i] + DELTA;
		unsigned hash_key = hash(key);
		struct hashtable_object* obj;
		obj = (struct hashtable_object*)tommy_hashlin_remove(&hashlin, tommy_hashtable_compare, &key, hash_key);
		if (!obj)
			abort();
		if (dereference) {
			if (obj->value != key)
				abort();
		}
	} STOP();

	START(DATA_TRIE) {
		unsigned key = REMOVE[i] + DELTA;
		struct trie_object* obj;
		obj = (struct trie_object*)tommy_trie_remove(&trie, key);
		if (!obj)
			abort();
		if (dereference) {
			if (obj->value != key)
				abort();
		}
	} STOP();

	START(DATA_TRIE_INPLACE) {
		unsigned key = REMOVE[i] + DELTA;
		struct trie_inplace_object* obj;
		obj = (struct trie_inplace_object*)tommy_trie_inplace_remove(&trie_inplace, key);
		if (!obj)
			abort();
		if (dereference) {
			if (obj->value != key)
				abort();
		}
	} STOP();

	START(DATA_KHASH) {
		unsigned key = REMOVE[i] + DELTA;
		unsigned hash_key = hash(key);
		struct khash_object* obj;
		khiter_t k;
		k = kh_get(word, khash, hash_key);
		if (k == kh_end(khash))
			abort();
		if (dereference) {
			obj = kh_value(khash, k);
		}
		kh_del(word, khash, k);
		if (dereference) {
			if (obj->value != key)
				abort();
		}
	} STOP();

#ifdef USE_CGOOGLE
	START(DATA_CGOOGLE) {
		unsigned key = REMOVE[i] + DELTA;
		HTItem* ptr;
		struct google_object* obj;
		ptr = HashFind(cgoogle, key);
		if (!ptr)
			abort();
		obj = (struct google_object*)ptr->data;
		HashDeleteLast(cgoogle);
		if (dereference) {
			if (obj->value != key)
				abort();
		}
	} STOP();
#endif

#ifdef USE_CCGOOGLE
	START(DATA_CCGOOGLE) {
		unsigned key = REMOVE[i] + DELTA;
		struct google_object* obj;
		ccgoogle_t::iterator ptr = ccgoogle->find(key);
		if (ptr == ccgoogle->end())
			abort();
		obj = ptr->second;
		ccgoogle->erase(ptr);

		/* force a progressive deallocation, it's done when reaching a 20% occupation */
		ccgoogle->resize(0);

		if (dereference) {
			if (obj->value != key)
				abort();
		}
	} STOP();
#endif

	START(DATA_UTHASH) {
		unsigned key = REMOVE[i] + DELTA;
		struct uthash_object* obj;
		HASH_FIND_INT(uthash, &key, obj);  
		if (!obj)
			abort();
		HASH_DEL(uthash, obj);  
		if (dereference) {
			if (obj->value != key)
				abort();
		}
	} STOP();

#ifdef USE_JUDY
	START(DATA_JUDY) {
		Word_t key = REMOVE[i] + DELTA;
		struct judy_object* obj;
		int r;
		if (dereference) {
			Pvoid_t PValue;
			JLG(PValue, judy, key);
			if (!PValue)
				abort();
			obj = *(struct judy_object**)PValue;
		}
		JLD(r, judy, key);
		if (r != 1)
			abort();
		if (dereference) {
			if (obj->value != key)
				abort();
		}
	} STOP();
#endif

	START(DATA_NEDTRIE) {
		unsigned key = REMOVE[i] + DELTA;
		struct nedtrie_object key_obj;
		struct nedtrie_object* obj;
		key_obj.value = key;
		obj = NEDTRIE_FIND(nedtrie_t, &nedtrie, &key_obj);
		if (!obj)
			abort();
		NEDTRIE_REMOVE(nedtrie_t, &nedtrie, obj);
		if (dereference) {
			if (obj->value != key)
				abort();
		}
	} STOP();
}

tommy_size_t uthash_size(struct uthash_object* obj)
{
	UT_hash_table* table;
	if (!obj)
		return 0;
	table = obj->hh.tbl;
	if (!table)
		return 0;
	return table->num_buckets * (tommy_size_t)sizeof(table->buckets[0])
		+ table->num_items * (tommy_size_t)sizeof(UT_hash_handle);
}

tommy_size_t nedtrie_size(struct nedtrie_t* nedtrie)
{
	struct nedtrie_object element;
	return nedtrie->count * sizeof(element.link);
}

tommy_size_t khash_size(khash_t(word)* khash)
{
	return khash->n_buckets * sizeof(void*) /* val */
		+ khash->n_buckets * sizeof(uint32_t) /* key */
		+ (khash->n_buckets >> 4) * sizeof(uint32_t); /* flags */
}

#ifdef USE_CCGOOGLE
tommy_size_t ccgoogle_size(ccgoogle_t* ccgoogle)
{
	ccgoogle_t::value_type element;
	return ccgoogle->bucket_count() * sizeof(element);
}
#endif

tommy_size_t rbt_size(rbtree_t* tree, unsigned count)
{
	struct rbt_object element;
	(void)tree;
	return count * sizeof(element.link);
}

void test_size(void)
{
#ifdef USE_JUDY
	Word_t w;
#endif

	MEM(DATA_TREE, rbt_size(&tree, the_max));
	MEM(DATA_HASHTABLE, tommy_hashtable_memory_usage(&hashtable));
	MEM(DATA_HASHDYN, tommy_hashdyn_memory_usage(&hashdyn));
	MEM(DATA_HASHLIN, tommy_hashlin_memory_usage(&hashlin));
	MEM(DATA_TRIE, tommy_trie_memory_usage(&trie));
	MEM(DATA_TRIE_INPLACE, tommy_trie_inplace_memory_usage(&trie_inplace));
	MEM(DATA_KHASH, khash_size(khash));
#ifdef USE_CCGOOGLE
	MEM(DATA_CCGOOGLE, ccgoogle_size(ccgoogle));
#endif
	MEM(DATA_UTHASH, uthash_size(uthash));
	MEM(DATA_NEDTRIE, nedtrie_size(&nedtrie));
#ifdef USE_JUDY
	JLMU(w, judy);
	MEM(DATA_JUDY, w);
#endif
}

void test_operation(unsigned* INSERT, unsigned* SEARCH)
{
	cache_clear();

	OPERATION(OPERATION_INSERT);
	test_insert(INSERT);

	OPERATION(OPERATION_HIT);
	test_hit(SEARCH);

	OPERATION(OPERATION_MISS);
	test_miss(SEARCH, 1);

	OPERATION(OPERATION_CHANGE);
	test_change(SEARCH, INSERT);

	OPERATION(OPERATION_SIZE);
	test_size();

	OPERATION(OPERATION_REMOVE);
	test_remove(SEARCH);
}

void test_order()
{
	test_alloc();
	ORDER(ORDER_RANDOM);
	test_operation(RAND0, RAND1);
	test_free();
}

void test(unsigned size, unsigned data, int log, int sparse)
{
	double b;
	double f;

	b = 1000;
	f = pow(10, 0.1);

	/* log if batch test or requested */
	the_log = (size == 0 && data == DATA_MAX) || log;

	/* write the header */
	if (the_log)
	for(the_order=0;the_order<ORDER_MAX;++the_order) {
		for(the_operation=0;the_operation<OPERATION_MAX;++the_operation) {
			FILE* f = open("wt");
			fprintf(f, "0\t");
			for(the_data=0;the_data<DATA_MAX;++the_data) {
				if (is_listed(the_data))
					fprintf(f, "%s\t", DATA_NAME[the_data]);
			}
			fprintf(f, "\n");
			fclose(f);
		}
	}

	if (size != 0)
		the_max = size;
	else
		the_max = (unsigned)b;

	while (the_max <= MAX) {
		unsigned retry;
	
		/* number of retries to avoid spikes */
		retry = 500000 / the_max;
		if (retry < 1)
			retry = 1;
		if (retry > RETRY_MAX)
			retry = RETRY_MAX;
		if (size != 0)
			retry = 1;

		/* clear the log */
		memset(LOG, 0, sizeof(LOG));

		order_init(the_max, sparse);

		/* run the test */
		for(the_retry=0;the_retry<retry;++the_retry) {
			/* data */
			for(the_data=0;the_data<DATA_MAX;++the_data) {
				if (!is_listed(the_data))
					continue;

				if (data != DATA_MAX && data != the_data)
					continue;

				for(the_order=0;the_order<ORDER_MAX;++the_order) {
					printf("%d %s %s", the_max, DATA_NAME[the_data], ORDER_NAME[the_order]);

					/* skip degenerated cases */
					if (data == DATA_MAX && LAST[the_data][the_order] > TIME_MAX_NS) {
						printf(" (skipped, too slow)\n");
						continue;
					}

					printf("\n");
					
					test_alloc();
					if (the_order == ORDER_FORWARD)
						test_operation(FORWARD, FORWARD);
					else
						test_operation(RAND0, RAND1);
					test_free();
				}
			}
		}

		order_done();

		/* write the data */
		if (the_log)
		for(the_order=0;the_order<ORDER_MAX;++the_order) {
			for(the_operation=0;the_operation<OPERATION_MAX;++the_operation) {
				FILE* f = open("at");

				fprintf(f, "%u\t", the_max);
				
				/* data */
				for(the_data=0;the_data<DATA_MAX;++the_data) {
					unsigned i, v;

					if (!is_listed(the_data))
						continue;

					/* get the minimum */
					v = LOG[0][the_data][the_order][the_operation];
					for(i=1;i<retry;++i) {
						if (LOG[i][the_data][the_order][the_operation] < v)
							v = LOG[i][the_data][the_order][the_operation];
					}

					/* save the *longest* measured time */
					if (the_operation != OPERATION_SIZE) {
						if (v != 0 && LAST[the_data][the_order] < v)
							LAST[the_data][the_order] = v;
					}

					fprintf(f, "%u\t", v);
				}

				fprintf(f, "\n");
				fclose(f);
			}
		}

		if (size != 0)
			break;

		/* new max */
		b *= f;
		the_max = (unsigned)b;
	}
}

void test_cache_miss(void)
{
	unsigned size = 512*1024*1024;
	unsigned char* DATA = (unsigned char*)malloc(size);
	unsigned delta = 512;
	tommy_uint64_t miss_time;
	unsigned i, j;
	tommy_uint64_t result = 0;

	memset(DATA, 0, size);

	for(j=0;j<8;++j) {
		tommy_uint64_t start, stop;
		start = nano();
		for(i=0;i<size;i += delta) {
			++DATA[i];
		}
		stop = nano();

		if (!result || result > stop - start)
			result = stop - start;
	}

	free(DATA);

	miss_time = result * delta / size;

	printf("Cache miss %d [ns]\n", (unsigned)miss_time);
}

int main(int argc, char * argv[])
{
	int i;
	int flag_data = DATA_MAX;
	int flag_size = 0;
	int flag_log = 0;
	int flag_miss = 0;
	int flag_sparse = 0;

	nano_init();

	printf("Tommy benchmark program.\n");

	for(i=1;i<argc;++i) {
		if (strcmp(argv[i], "-l") == 0) {
			flag_log = 1;
		} else if (strcmp(argv[i], "-s") == 0) {
			flag_sparse = 1;
		} else if (strcmp(argv[i], "-m") == 0) {
			flag_miss = 1;
		} else if (strcmp(argv[i], "-n") == 0) {
			flag_size = MAX;
		} else if (strcmp(argv[i], "-N") == 0) {
			if (i+1 >= argc) {
				printf("Missing data in %s\n", argv[i]);
				exit(EXIT_FAILURE);
			}
			flag_size = atoi(argv[i+1]);
			++i;
		} else if (strcmp(argv[i], "-d") == 0) {
			int j;
			if (i+1 >= argc) {
				printf("Missing data in %s\n", argv[i]);
				exit(EXIT_FAILURE);
			}
			flag_data = DATA_MAX;
			for(j=0;j<DATA_MAX;++j) {
				if (strcmp(argv[i+1], DATA_NAME[j]) == 0) {
					flag_data = j;
				}
			}
			if (flag_data == DATA_MAX) {
				printf("Unknown data %s\n", argv[i+1]);
				exit(EXIT_FAILURE);
			}
			++i;
		} else {
			printf("Unknown option %s\n", argv[i]);
			exit(EXIT_FAILURE);
		} 
	}

	if (flag_miss) {
		test_cache_miss();
		return EXIT_SUCCESS;
	}

	test(flag_size, flag_data, flag_log, flag_sparse);

	printf("OK\n");

	return EXIT_SUCCESS;
}

