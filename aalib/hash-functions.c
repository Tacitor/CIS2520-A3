#include <stdio.h>
#include <stdlib.h>
#include <string.h> // for strcmp()
#include <ctype.h> // for isprint()

#include "hashtools.h"

/** check if the two keys are the same */
int
doKeysMatch(AAKeyType key1, size_t key1len, AAKeyType key2, size_t key2len)
{
	/** if the lengths don't match, the keys can't match */
	if (key1len != key2len)
		return 0;

	return memcmp(key1, key2, key1len) == 0;
}

/* provide the hex representation of a value */
static char toHex(int val)
{
	if (val < 10) return (char) ('0' + val);
	return (char) ('a' + (val - 10));
}

/**
 * Provide the key in a printable form.  Uses a static buffer,
 * which means that not only is this not thread-safe, but
 * even runs into trouble if called twice in the same printf().
 *
 * That said, is does provide a memory clean way to give a 
 * printable string return value to the calling code
 */
int
printableKey(char *buffer, int bufferlen, AAKeyType key, size_t printlen)
{
	int i, allChars = 1;
	char *loadptr;


	for (i = 0; allChars && i < printlen; i++) {
		if ( ! isprint(key[i])) allChars = 0;
	}

	if (allChars) {
		snprintf(buffer, bufferlen, "char key:[%s]", (char *) key);
	} else {
		snprintf(buffer, bufferlen, "hex key:[0x");
		loadptr = &buffer[strlen(buffer)];
		for (i = 0; i < printlen && loadptr - buffer < bufferlen - 4; i++) {
			*loadptr++ = toHex((key[i] & 0xf0) >> 4); // top nybble -> first hext digit
			*loadptr++ = toHex(key[i] & 0x0f);        // bottom nybble -> second digit
		}
		*loadptr++ = ']';
		*loadptr++ = 0;
	}
	return 1;
}

/**
 * Calculate a hash value based on the length of the key
 *
 * Calculate an integer index in the range [0...size-1] for
 * 		the given string key
 *
 *  @param  key  key to calculate mapping upon
 *  @param  size boundary for range of allowable return values
 *  @return      integer index associated with key
 *
 *  @see    HashAlgorithm
 */
HashIndex hashByLength(AAKeyType key, size_t keyLength, HashIndex size)
{
	return keyLength % size;
}



/**
 * Calculate a hash value based on the sum of the values in the key
 *
 * Calculate an integer index in the range [0...size-1] for
 * 		the given string key, based on the sum of the values
 *		in the key
 *
 *  param  key  key to calculate mapping upon
 *  param  size boundary for range of allowable return values
 *  return      integer index associated with key
 */
HashIndex hashBySum(AAKeyType key, size_t keyLength, HashIndex size)
{
	HashIndex sum = 0;

	/**
	 * DONE: you will need to implement a summation based
	 * hashing algorithm here, using a sum-of-bytes
	 * strategy such as that discussed in class.  Take
	 * a look at HashByLength if you want an example
	 * of a "working" (but not very smart) hashing
	 * algorithm.
	 */
	for (int i = 0; i < keyLength; i++) {
		sum = (sum + (HashIndex)(key[i])) % size;
	}

	return sum;
}


/**
 * Locate an empty position in the given array, starting the
 * search at the indicated index, and restricting the search
 * to locations in the range [0...size-1]
 *
 *  @param  index where to begin the search
 *  @param  AssociativeArray associated AssociativeArray we are probing
 *  @param  invalidEndsSearch should the identification of a
 *				KeyDataPair marked invalid end our search?
 *				This is true if we are looking for a location
 *				to insert new data
 *  @return index of location where search stopped, or -1 if
 *				search failed
 *
 *  @see    HashProbe
 */
HashIndex linearProbe(AssociativeArray *hashTable,
		AAKeyType key, size_t keylength,
		int index, int invalidEndsSearch, int *cost
	)
{
	/**
	 * DONE: you will need to implement an algorithm
	 * that probes until it finds an "empty" slot in
	 * the hashTable.  Note that because of tombstones,
	 * there are multiple ways of a slot being empty.
	 * Additionally, the code in HashTable depends on
	 * this code to find an actually empty slot, so
	 * this code is called with the results of the
	 * hash -- this means that the "index" value may
	 * already be valid on entry.
	 *
	 * Note that if an empty place cannot be found,
	 * you are to return (-1).  If a zero or positive
	 * value is returned, the calling code <i>will</i>
	 * use it, so be sure your return values are correct!
	 *
	 * For this routine, implement a "linear" probing
	 * strategy, such as that discussed in class.
	 */
	HashIndex j = index;

	//set up the stopping condition
	int contSearch = 1;
	int keyDataPairValidity;

	//Debug the lookup process
	/* 
	if (!invalidEndsSearch) {
		printf("\nSearching for key: %s starting at: %d\n", (char*)key, index);	
	}
	*/

	//loop until a spot has been found
	while (contSearch) {
		keyDataPairValidity = (hashTable->table)[j].validity;
		//count this itteration towards the total cost
		(*cost)++;
		
		//Debug the lookup process
		/* 
		if (!invalidEndsSearch) {
			printf("Mem address: %p\n", (hashTable->table)[j].key);
		}
		*/

		// test to see if this index has the provided key in it
		if ((hashTable->table)[j].key != NULL 
			&& (hashTable->table)[j].validity == HASH_USED 
			&& doKeysMatch((hashTable->table)[j].key, (hashTable->table)[j].keylen, key, keylength) == 1)
		{
			contSearch = 0;
			return j;
		}

		//if this is not the key we are looking for, or we are inserting and it won't be in here
		//test to see if need to continue searching or if we have hit a dead end that does not inlcude finding the desired key
		if (keyDataPairValidity == HASH_EMPTY) { //the second we find an empty spot we know that the given key is not here AND that it <i>could</i> be added here is insterting
			contSearch = 0; //stop the search

			return j;
		} else if (invalidEndsSearch && keyDataPairValidity == HASH_DELETED) {
			//if we are insterting then we can also stop at the first tombstone and overwite it
			contSearch = 0; //stop the search

			//ensure that the key in this tombstone is freed since is it about to be overwitten by a new insetion
			if (key != NULL)
			{
				free((hashTable->table)[j].key);
			}

			return j;
		}

		//if we have not reached an empty spot linearly probe the next spot (use step size of 1)
		j = (j + 1) % hashTable->size;

		if (j == index) { //if we have wrapped around again to the stating position
			//the hash table is full :(
			
			return -1;
		}
	}

	//if the loop is broken out of without returning the function some error has occoured
	fprintf(stderr, "Invalid call to linearProbe returning -1\n");
	return -1;

}


/**
 * Locate an empty position in the given array, starting the
 * search at the indicated index, and restricting the search
 * to locations in the range [0...size-1]
 *
 *  @param  index where to begin the search
 *  @param  hashTable associated HashTable we are probing
 *  @param  invalidEndsSearch should the identification of a
 *				KeyDataPair marked invalid end our search?
 *				This is true if we are looking for a location
 *				to insert new data
 *  @return index of location where search stopped, or -1 if
 *				search failed
 *
 *  @see    HashProbe
 */
HashIndex quadraticProbe(AssociativeArray *hashTable, AAKeyType key, size_t keylen,
		int startIndex, int invalidEndsSearch,
		int *cost
	)
{
	/**
	 * DONE: you will need to implement an algorithm
	 * that probes until it finds an "empty" slot in
	 * the hashTable.  Note that because of tombstones,
	 * there are multiple ways of a slot being empty.
	 * Additionally, the code in HashTable depends on
	 * this code to find an actually empty slot, so
	 * this code is called with the results of the
	 * hash -- this means that the "index" value may
	 * already be valid on entry.
	 *
	 * Note that if an empty place cannot be found,
	 * you are to return (-1).  If a zero or positive
	 * value is returned, the calling code <i>will</i>
	 * use it, so be sure your return values are correct!
	 *
	 * For this routine, implement a "quadratic" probing
	 * strategy, such as that discussed in class.
	 */

	int step = 0;
	HashIndex j = startIndex;

	//set up the stopping condition
	int contSearch = 1;
	int keyDataPairValidity;

	//Debug the lookup process
	/* 
	if (!invalidEndsSearch) {
		printf("\nSearching for key: %s starting at: %d\n", (char*)key, index);	
	}
	*/

	//loop until a spot has been found
	while (contSearch) {
		keyDataPairValidity = (hashTable->table)[j].validity;
		//count this itteration towards the total cost
		(*cost)++;
		
		//Debug the lookup process
		/* 
		if (!invalidEndsSearch) {
			printf("Mem address: %p\n", (hashTable->table)[j].key);
		}
		*/

		// test to see if this index has the provided key in it
		if ((hashTable->table)[j].key != NULL 
			&& (hashTable->table)[j].validity == HASH_USED 
			&& doKeysMatch((hashTable->table)[j].key, (hashTable->table)[j].keylen, key, keylen) == 1)
		{
			contSearch = 0;
			return j;
		}

		//if this is not the key we are looking for, or we are inserting and it won't be in here
		//test to see if need to continue searching or if we have hit a dead end that does not inlcude finding the desired key
		if (keyDataPairValidity == HASH_EMPTY) { //the second we find an empty spot we know that the given key is not here AND that it <i>could</i> be added here is insterting
			contSearch = 0; //stop the search

			return j;
		} else if (invalidEndsSearch && keyDataPairValidity == HASH_DELETED) {
			//if we are insterting then we can also stop at the first tombstone and overwite it
			contSearch = 0; //stop the search

			//ensure that the key in this tombstone is freed since is it about to be overwitten by a new insetion
			if (key != NULL)
			{
				free((hashTable->table)[j].key);
			}

			return j;
		}

		//if we have not reached an empty spot quaddratically probe the next spot
		step++;
		j = (startIndex + (step * step)) % hashTable->size;
		
		if (step == hashTable->size) { //if a single step is larger than the table there is no room left
			//the hash table is full :(
			contSearch = 0;
			
			return -1;
		}
	}

	//if the loop is broken out of without returning the function some error has occoured
	fprintf(stderr, "Invalid call to quadraticProbe returning -1\n");
	return -1;
}


/**
 * Locate an empty position in the given array, starting the
 * search at the indicated index, and restricting the search
 * to locations in the range [0...size-1]
 *
 *  @param  index where to begin the search
 *  @param  hashTable associated HashTable we are probing
 *  @param  invalidEndsSearch should the identification of a
 *				KeyDataPair marked invalid end our search?
 *				This is true if we are looking for a location
 *				to insert new data
 *  @return index of location where search stopped, or -1 if
 *				search failed
 *
 *  @see    HashProbe
 */
HashIndex doubleHashProbe(AssociativeArray *hashTable, AAKeyType key, size_t keylen,
		int startIndex, int invalidEndsSearch,
		int *cost
	)
{
	/**
	 * DONE: you will need to implement an algorithm
	 * that calls a second hash function (listed
	 * in the hashTable) and uses the value obtained
	 * as a result from that as the step size.
	 *
	 * Beyond that, the algorithm proceeds as with
	 * the above strategies.
	 */

	HashIndex step = (*(hashTable->hashAlgorithmSecondary))(key, keylen, hashTable->size); //get the step size
	HashIndex j = startIndex;

	//set up the stopping condition
	int contSearch = 1;
	int keyDataPairValidity;

	//loop until a spot has been found
	while (contSearch) {
		keyDataPairValidity = (hashTable->table)[j].validity;
		//count this itteration towards the total cost
		(*cost)++;

		// test to see if this index has the provided key in it
		if ((hashTable->table)[j].key != NULL 
			&& (hashTable->table)[j].validity == HASH_USED 
			&& doKeysMatch((hashTable->table)[j].key, (hashTable->table)[j].keylen, key, keylen) == 1)
		{
			contSearch = 0;
			return j;
		}

		//if this is not the key we are looking for, or we are inserting and it won't be in here
		//test to see if need to continue searching or if we have hit a dead end that does not inlcude finding the desired key
		if (keyDataPairValidity == HASH_EMPTY) { //the second we find an empty spot we know that the given key is not here AND that it <i>could</i> be added here is insterting
			contSearch = 0; //stop the search

			return j;
		} else if (invalidEndsSearch && keyDataPairValidity == HASH_DELETED) {
			//if we are insterting then we can also stop at the first tombstone and overwite it
			contSearch = 0; //stop the search

			//ensure that the key in this tombstone is freed since is it about to be overwitten by a new insetion
			if (key != NULL)
			{
				free((hashTable->table)[j].key);
			}

			return j;
		}

		//if we have not reached an empty spot linearly probe the next spot (use step size of calculated by the secondary hash)
		j = (j + step) % hashTable->size;

		if (j == startIndex) { //if we have wrapped around again to the starting position
			//the hash table is full :(
			
			return -1;
		}
	}

	//if the loop is broken out of without returning the function some error has occoured
	fprintf(stderr, "Invalid call for doubleHashProbe returning -1\n");
	return -1;
}
