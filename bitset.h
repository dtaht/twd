#ifndef _TWD_BITSET_H
#define _TWD_BITSET_H
// Basic bitset handling
// I miss boost::std_bitset
// Not for the first time I regret not doing this in C++

enum word_t { WORD_SIZE = sizeof(size_t) * 8 };

word_t data[N / 32 + 1];

typedef struct { word_t *words; size_t nwords; } bitset_t;

static inline int bitset_index(size_t b) { return b / WORD_SIZE; }
static inline int bitset_boffset(size_t b) { return b % WORD_SIZE; }

static inline void bitset_set(bitset_t *data, size_t b) { 
    data[bindex(b)] |= 1 << (bitset_boffset(b)); 
}

static inline void bitset_clear(bitset_t *data, size_t b) { 
    data[bindex(b)] &= ~(1 << (bitset_boffset(b)));
}
static inline int bitset_get(bitset_t *data, size_t b) { 
    return data[bindex(b)] & (1 << (bitset_boffset(b));
}

/* set all elements of data to zero */ 

void bitset_clear_all()
{
}

/* set all elements of data to one */

void bitset_set_all()
{ 
}

void bitset_ffs()
{
}

bitset_t *bitset_calloc(size_t nbits) {
    bitset_t *bitset = malloc(sizeof(*bitset));
    bitset->nwords = (n / WORD_SIZE + 1);
    bitset->words = malloc(sizeof(*bitset->words) * bitset->nwords);
    bitset_clear(bitset);
    return bitset;
}

void bitset_free(struct bitset *bitset) {
    free(bitset->words);
    free(bitset);
}

#endif
