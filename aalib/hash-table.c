#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>


#include "hashtools.h"

/** forward declaration */
static HashAlgorithm lookupNamedHashStrategy(const char *name);
static HashProbe lookupNamedProbingStrategy(const char *name);

/** Custom forward declaration for function created by Lukas*/
static int deleteKeys(AssociativeArray *);

/**
 * Create a hash table of the given size,
 * which will use the given algorithm to create hash values,
 * and the given probing strategy
 *
 *  @param  hash  the HashAlgorithm to use
 *  @param  probingStrategy algorithm used for probing in the case of
 *				collisions
 *  @param  newHashSize  the size of the table (will be rounded up
 *				to the next-nearest larger prime, but see exception)
 *  @see         HashAlgorithm
 *  @see         HashProbe
 *  @see         Primes
 *
 *  @throws java.lang.IndexOutOfBoundsException if no prime number larger
 *				than newHashSize can be found (currently only primes
 *				less than 5000 are known)
 */
AssociativeArray *
aaCreateAssociativeArray(
		size_t size,
		char *probingStrategy,
		char *hashPrimary,
		char *hashSecondary
	)
{
	AssociativeArray *newTable;

	newTable = (AssociativeArray *) malloc(sizeof(AssociativeArray));

	newTable->hashAlgorithmPrimary = lookupNamedHashStrategy(hashPrimary);
	newTable->hashNamePrimary = strdup(hashPrimary);
	newTable->hashAlgorithmSecondary = lookupNamedHashStrategy(hashSecondary);
	newTable->hashNameSecondary = strdup(hashSecondary);
	newTable->hashProbe = lookupNamedProbingStrategy(probingStrategy);
	newTable->probeName = strdup(probingStrategy);

	newTable->size = getLargerPrime(size);

	if (newTable->size < 1) {
		fprintf(stderr, "Cannot create table of size %ld\n", size);
		free(newTable);
		return NULL;
	}

	newTable->table = (KeyDataPair *) malloc(newTable->size * sizeof(KeyDataPair));

	/** initialize everything with zeros */
	memset(newTable->table, 0, newTable->size * sizeof(KeyDataPair));

	newTable->nEntries = 0;

	newTable->insertCost = newTable->searchCost = newTable->deleteCost = 0;

	return newTable;
}

/**
 * deallocate all the memory in the store -- the keys (which we allocated),
 * and the store itself.
 * The user * code is responsible for managing the memory for the values
 */
void
aaDeleteAssociativeArray(AssociativeArray *aarray)
{
	/**
	 * DONE:  clean up the memory managed by our utility
	 *
	 * Note that memory for keys are managed, values are the
	 * responsibility of the user
	 */
	//dealloc all the keys
	deleteKeys(aarray);

	//dealloc the array
	free(aarray->table);

	//dealloc the strings
	free(aarray->hashNamePrimary);
	free(aarray->hashNameSecondary);
	free(aarray->probeName);

	//now nuke the aa struct itself
	free(aarray);

}

/**
 * iterate over the array, calling the user function on each valid value
 */
int aaIterateAction(
		AssociativeArray *aarray,
		int (*userfunction)(AAKeyType key, size_t keylen, void *datavalue, void *userdata),
		void *userdata
	)
{
	int i;

	for (i = 0; i < aarray->size; i++) {
		if (aarray->table[i].validity == HASH_USED) {
			if ((*userfunction)(
					aarray->table[i].key,
					aarray->table[i].keylen,
					aarray->table[i].value,
					userdata) < 0) {
				return -1;
			}
		}
	}
	return 1;
}

/** utilities to change names into functions, used in the function above */
static HashAlgorithm lookupNamedHashStrategy(const char *name)
{
	if (strncmp(name, "sum", 3) == 0) {
		return hashBySum;
	} else if (strncmp(name, "len", 3) == 0) {
		return hashByLength;		
	}
	else if (strncmp(name, "pri", 3) == 0)
	{ // DONE: add in your own strategy here
		return hashByPrime;
	}

	fprintf(stderr, "Invalid hash strategy '%s' - using 'sum'\n", name);
	return hashBySum;
}

static HashProbe lookupNamedProbingStrategy(const char *name)
{
	if (strncmp(name, "lin", 3) == 0) {
		return linearProbe;
	} else if (strncmp(name, "qua", 3) == 0) {
		return quadraticProbe;
	} else if (strncmp(name, "dou", 3) == 0) {
		return doubleHashProbe;
	}

	fprintf(stderr, "Invalid hash probe strategy '%s' - using 'linear'\n", name);
	return linearProbe;
}

/**
 * Add another key and data value to the table, provided there is room.
 *
 *  @param  key  a string value used for searching later
 *  @param  value a data value associated with the key
 *  @return      the location the data is placed within the hash table,
 *				 or a negative number if no place can be found
 */
int aaInsert(AssociativeArray *aarray, AAKeyType key, size_t keylen, void *value)
{
	/**
	 * DONE:  Search for a location where this key can go, stopping
	 * if we find a value that has been delete and reuse it.
	 *
	 * If a suitable location is found, we then initialize that
	 * slot with the new key and data
	 */

	//will need to use the hash algorithm from aarray, use the primary
	//this gives us the first possible index. Might not store the value here as a collision is possible.
	//will need to run through a probing strategy before storing the value
	HashIndex hasedIndex = (*(aarray->hashAlgorithmPrimary))(key, keylen, aarray->size); //the index in the hash table. Indexing starts at 0

	//then look at the index in the location found above
	//call the probe method to get the index
	HashIndex finalIndex = (*(aarray->hashProbe))(aarray, key, keylen, hasedIndex, 1, &aarray->insertCost);

	//check for a used index
	if (aarray->table[finalIndex].validity == HASH_USED) {
		//this is called when the probe returns an index that would work but is already used
		//such a a case occurs when inserting duplicate keys
		fprintf(stderr, "Error: Failed to probe correctly with: '%s' when inserting\n", aarray->probeName);

		//set the finalIndex to be an error state
		finalIndex = -1;
	} else if (finalIndex >= 0) {

		//add it into the array
		//DONE: Check to see if this strdup call causes issues with null terminator when in useIntKey mode
		//It does cause issues so instead use malloc and memdup
		aarray->table[finalIndex].key = (AAKeyType)malloc(keylen);
		memcpy(aarray->table[finalIndex].key, key, keylen);

		//aarray->table[finalIndex].key = (AAKeyType)strdup((char*)key); //Do not forget to free this later
		aarray->table[finalIndex].keylen = keylen;
		aarray->table[finalIndex].value = value;
		aarray->table[finalIndex].validity = HASH_USED;

		//count the newly added entry
		aarray->nEntries++;
	}

	return finalIndex; //can always return the finalIndex b/c all the probing algos return -1 if they fail, so we can just pass it forward always
}


/**
 * Locates the KeyDataPair associated with the given key, if
 * present in the table.
 *
 *  @param  key  the key to search for
 *  @return      the KeyDataPair containing the key, if the key
 *				 was present in the table, or NULL, if it was not
 *  @see         KeyDataPair
 */
void *aaLookup(AssociativeArray *aarray, AAKeyType key, size_t keylen
	)
{
	/**
	 * DONE: perform a similar search to the insert, but here a
	 * deleted location means we have not found the key
	 */

	// will need to use the hash algorithm from aarray, use the primary
	// this gives us the first possible index. Will begin the search here
	// will need to run through a probing strategy before storing the value
	HashIndex hasedIndex = (*(aarray->hashAlgorithmPrimary))(key, keylen, aarray->size); // the index in the hash table. Indexing starts at 0

	// then look at the index in the location found above
	// call the probe method to get the next index
	HashIndex finalIndex = (*(aarray->hashProbe))(aarray, key, keylen, hasedIndex, 0, &aarray->searchCost);

	//Debug the lookup process
	/* 
	printf("finalIndex: %ld, validity: %d\n", finalIndex, aarray->table[finalIndex].validity);
	*/

	//debug the status of an entry especially after deletion
	//printf("\tLookup for: %s, has finalIndex of: %ld, with validity of: %d\n", (char*)key, finalIndex, aarray->table[finalIndex].validity);

	// see if the finalIndex is in the table
	// check to see if the returned index is used
	if (finalIndex >= 0 && aarray->table[finalIndex].validity == HASH_USED)
	{
		// if the index is in use make sure it is the correct one
		// return NULL if the wrong index is returned
		if ((aarray->table)[finalIndex].key != NULL 
			&& doKeysMatch((aarray->table)[finalIndex].key, (aarray->table)[finalIndex].keylen, key, keylen) == 1)
		{
			return aarray->table[finalIndex].value;
		}
		else
		{
			// this is probably never called
			fprintf(stderr, "Error: Failed to probe correctly with: '%s' when querying", aarray->probeName);
			return NULL;
		}
	}

	//return NULL in all other conditions
	return NULL;
}

/**
 * Locates the KeyDataPair associated with the given key, if
 * present in the table.
 *
 *  @param  key  the key to search for
 *  @return      the index of the KeyDataPair containing the key,
 *				 if the key was present in the table, or (-1),
 *				 if no key was found
 *  @see         KeyDataPair
 */
void *aaDelete(AssociativeArray *aarray, AAKeyType key, size_t keylen)
{
	/**
	 * DONE: Deletion is closely related to lookup;
	 * you must find where the key is stored before
	 * you delete it, after all.
	 *
	 * Implement a deletion algorithm based on tombstones,
	 * as described in class
	 */

	// will need to use the hash algorithm from aarray, use the primary
	// this gives us the first possible index. Will begin the deletion search here
	// will need to run through a probing strategy before storing the value
	HashIndex hasedIndex = (*(aarray->hashAlgorithmPrimary))(key, keylen, aarray->size); // the index in the hash table. Indexing starts at 0

	// then look at the index in the location found above
	// call the probe method to get the next index
	HashIndex finalIndex = (*(aarray->hashProbe))(aarray, key, keylen, hasedIndex, 0, &aarray->deleteCost);

	// see if the finalIndex is in the table
	// check to see if the returned index is used
	if (finalIndex >= 0 && aarray->table[finalIndex].validity == HASH_USED)
	{
		// if the index is in use make sure it is the correct one
		// return NULL if the wrong index is returned
		if ((aarray->table)[finalIndex].key != NULL 
			&& doKeysMatch((aarray->table)[finalIndex].key, (aarray->table)[finalIndex].keylen, key, keylen) == 1)
		{
			//now need to delete the entry by marking it as a tombstone
			(aarray->table)[finalIndex].validity = HASH_DELETED;
			//keep the key as is so it can be displayed at the print out of the hash table

			//count the newly deleted entry
			aarray->nEntries--;

			return (aarray->table)[finalIndex].value;
		}
		else
		{
			// this is probably never called
			fprintf(stderr, "Error: Failed to probe correctly with: '%s' when deleting", aarray->probeName);
			return NULL;
		}
	}

	//return NULL in all other conditions
	return NULL;
}

/**
 * Print out the entire aarray contents
 */
void aaPrintContents(FILE *fp, AssociativeArray *aarray, char * tag)
{
	char keybuffer[128];
	int i;

	fprintf(fp, "%sDumping aarray of %d entries:\n", tag, aarray->size);
	for (i = 0; i < aarray->size; i++) {
		fprintf(fp, "%s  ", tag);
		if (aarray->table[i].validity == HASH_USED) {
			printableKey(keybuffer, 128,
					aarray->table[i].key,
					aarray->table[i].keylen);
			fprintf(fp, "%d : in use : '%s'\n", i, keybuffer);
		} else {
			if (aarray->table[i].validity == HASH_EMPTY) {
				fprintf(fp, "%d : empty (NULL)\n", i);
			} else if ( aarray->table[i].validity == HASH_DELETED) {
				printableKey(keybuffer, 128,
						aarray->table[i].key,
						aarray->table[i].keylen);
				fprintf(fp, "%d : empty (deleted - was '%s')\n", i, keybuffer);
			} else {
				fprintf(fp, "%d : invalid validity state %d\n", i,
						aarray->table[i].validity);
			}
		}
	}
}


/**
 * Print out a short summary
 */
void aaPrintSummary(FILE *fp, AssociativeArray *aarray)
{
	fprintf(fp, "Associative array contains %d entries in a table of %d size\n",
			aarray->nEntries, aarray->size);
	fprintf(fp, "Strategies used: '%s' hash, '%s' secondary hash and '%s' probing\n",
			aarray->hashNamePrimary, aarray->hashNameSecondary, aarray->probeName);
	fprintf(fp, "Costs accrued due to probing:\n");
	fprintf(fp, "  Insertion : %d\n", aarray->insertCost);
	fprintf(fp, "  Search    : %d\n", aarray->searchCost);
	fprintf(fp, "  Deletion  : %d\n", aarray->deleteCost);
}

//Custom functions created by Lukas
int deleteKey(AAKeyType key)
{
	if (key != NULL)
	{
		free(key);
	}
	return 0;
}

/**
 * iterate over the array, deleting and deallocing keys as it goes
 */
static int deleteKeys(AssociativeArray *aarray)
{
	int i;

	for (i = 0; i < aarray->size; i++)
	{
		//this allows both used and tombstone keys to be dealloc'd
		if (aarray->table[i].validity != HASH_EMPTY)
		{
			deleteKey(aarray->table[i].key);			
		}
	}
	return 1;
}
