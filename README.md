# CIS*2520 F23 Assignment 3

## Name 

    Lukas Krampitz

## Student Number

    1192170

## Resource Used for Assistance

    - None

## State of Implementation

    Complete

## Summary of New Hashing Algorithm

    The new hashing algorithm is called hashByPrime and is similar to hashBySum, with the key distinction it that it
    makes use of prime numbers close to the sum of the key. Instead of simple adding the integer value of the current
    byte of a key is instead makes a call to the privided function getLargerPrime() and passes it the value of the
    integer value of the byte. This returns a prime value that is always larger or equal to the passed value. These
    primes are summed and then the modulus operator is used to ensure the index stays within bounds of the hash table.

    Goals:
	- Must: This algorithm is able to locate values again is provied the same key. This is beucase primes numbers
	  are constant so the same index can always be computed.
	- Should: The full table is able to be used, this hash does not prevent any values from being produced
	- Ideally: It does a fairly good job of avoiding clusters. The amount and size of clusters is comparible
	  to hashBySum.
