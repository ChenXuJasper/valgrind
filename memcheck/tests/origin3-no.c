
/* This test case was originally written by Nicholas Nethercote. */

// This test demonstrates cases the piggybacking algorithm cannot handle,
// but which are handled ok by the instrumentation based algorithm.

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include "../memcheck.h"

int x = 0;

int main(void)
{
   assert(4 == sizeof(int));

   // 8-bit undefined value.  When compared it's loaded from memory, so will
   // never work.
   {
      char* ptr_to_undef_char = malloc(sizeof(char));
      char  undef_char = *ptr_to_undef_char;
      fprintf(stderr, "\nUndef 1 of 8 (8 bit undef)\n");
      x += (undef_char == 0x12 ? 11 : 22);
   }

   // Stack, 8-bit from (recently) 32-bit.  But the load only loads 8-bits
   // of the value, so it'll never work.
   {
      int undef_stack_int;
      register char undef_stack_char = (char)undef_stack_int;
      fprintf(stderr, "\nUndef 2 of 8 (8 bits of 32 undef)\n");
      x += (undef_stack_char == 0x12 ? 11 : 22);
   }

   // 32-bit undefined value.  This one is identified, and is here for
   // sanity-checking.
   {
      int* ptr_to_undef_int = malloc(sizeof(int));
      int  undef_int = *ptr_to_undef_int;
      fprintf(stderr, "\nUndef 3 of 8 (32 bit undef)\n");
      x += (undef_int == 0x12345678 ? 13 : 24);
   }

   // Unaligned 32-bit value.
   {
      int* ptr_to_undef_int = malloc(sizeof(int) + 1);
      int  undef_unaligned_int = *(int*)((long)ptr_to_undef_int + 1);
      fprintf(stderr, "\nUndef 4 of 8 (32 bit undef, unaligned)\n");
      x += (undef_unaligned_int == 0x12345678 ? 14 : 25);
   }

   // Modified 32-bit value.
   {
      int* ptr_to_undef_int3 = malloc(sizeof(int));
      int  modified_undef_int = *ptr_to_undef_int3;
      fprintf(stderr, "\nUndef 5 of 8 (32 bit undef, modified)\n");
      modified_undef_int++;
      x += (modified_undef_int == 0x12345678 ? 15 : 26);
   }

   // Uninitialised 32-bit value (middle of 3) is made undefined in two
   // unaligned pieces:
   //   |....|....|....|   three 4-byte integers
   //    XXXX-YY           first MAKE_MEM_UNDEFINED
   //           YY-XXXX    second MAKE_MEM_UNDEFINED
   // Because the YY parts don't get marked (they're not 32-bit and aligned)
   // the middle byte keeps its original value, which is zero (from calloc).
   // So even though it's been marked as undefined, it doesn't have an
   // origin-tracking value and so cannot be identified.  We also check the
   // first and third ints (which are identified) for sanity-checking.
   {
      int* ptr_to_3_undef_ints = calloc(3, sizeof(int));
      int* ptr_to_middle       = (int*)((long)ptr_to_3_undef_ints + 6);
      VALGRIND_MAKE_MEM_UNDEFINED(ptr_to_3_undef_ints, 6);
      VALGRIND_MAKE_MEM_UNDEFINED(ptr_to_middle,       6);
      fprintf(stderr, "\nUndef 6 of 8 (32 bit undef, unaligned, strange, #1)\n");
      x += (*(ptr_to_3_undef_ints + 0)  == 0x12345678 ? 16 : 27);
      fprintf(stderr, "\nUndef 7 of 8 (32 bit undef, unaligned, strange, #2)\n");
      x += (*(ptr_to_3_undef_ints + 1)  == 0x12345678 ? 17 : 28);
      fprintf(stderr, "\nUndef 8 of 8 (32 bit undef, unaligned, strange, #3)\n");
      x += (*(ptr_to_3_undef_ints + 2)  == 0x12345678 ? 18 : 29);
   }

   return x;
}
