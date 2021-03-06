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
 
/** \file
 * Hash functions for the use with ::tommy_hashtable, ::tommy_hashdyn and ::tommy_hashlin.
 */

#ifndef __TOMMYHASH_H
#define __TOMMYHASH_H

#include "tommytypes.h"

/******************************************************************************/
/* hash */

/**
 * Hash type used in hashtables.
 */
typedef tommy_key_t tommy_hash_t;

/**
 * Hash function.
 * Robert Jenkins "lookup3" hash,
 * from http://www.burtleburtle.net/bob/hash/doobs.html, function hashlittle().
 * \note
 * This function is platform independent as it read always in little endian format.
 * \param init_val Initial value. You can use this to concatenate hash. Otherwise set it at 0.
 * \param void_key Key to hash.
 * \param key_len Length of the key.
 * \return The 32 bit hash value.
 */
tommy_uint32_t tommy_hash_u32(tommy_uint32_t init_val, const void* void_key, tommy_size_t key_len);

/**
 * Hash function for 64 bits.
 * Robert Jenkins "lookup3" hash,
 * from http://www.burtleburtle.net/bob/hash/doobs.html, function hashlittle2().
 * \note
 * This function is platform independent as it read always in little endian format. 
 */
tommy_uint64_t tommy_hash_u64(tommy_uint64_t init_val, const void* void_key, tommy_size_t key_len);

/**
 * Integer hash of 32 bits.
 * Robert Jenkins "4-byte Integer Hashing",
 * from http://burtleburtle.net/bob/hash/integer.html
 */
tommy_inline tommy_uint32_t tommy_inthash_u32(tommy_uint32_t key)
{
	key -= key << 6;
	key ^= key >> 17;
	key -= key << 9;
	key ^= key << 4;
	key -= key << 3;
	key ^= key << 10;
	key ^= key >> 15;

	return key;
}

/**
 * Integer hash of 64 bits.
 * Thomas Wang "Integer Hash Function",
 * from http://www.cris.com/~Ttwang/tech/inthash.htm
 */
tommy_inline tommy_uint64_t tommy_inthash_u64(tommy_uint64_t key)
{
	key = ~key + (key << 21);
	key = key ^ (key >> 24);
	key = key + (key << 3) + (key << 8);
	key = key ^ (key >> 14);
	key = key + (key << 2) + (key << 4);
	key = key ^ (key >> 28);
	key = key + (key << 31);

	return key;
}

#endif

