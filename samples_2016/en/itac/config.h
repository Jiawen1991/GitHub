#define INT64 long
#define INT16 short
#define INT32 int
#define INTMPI int
#define FLOAT32 float
#define FLOAT64 double

#define UINT32    unsigned INT32
#define UINT64    unsigned INT64
typedef unsigned char U1;
typedef UINT32        U4;
typedef UINT64        U8;
typedef INT32         I4;
typedef INT64         I8;
typedef FLOAT64       F8;
typedef union { U1 bytes[8]; U8 word; F8 float64; } O8; /**< opaque 8 bytes */
#define BOOL      int
#define TRUE      1
#define FALSE     0

#define PAL_MERGE_TYPE_ANSI 1
