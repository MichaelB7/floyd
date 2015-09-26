
/*----------------------------------------------------------------------+
 |                                                                      |
 |      Engine.h                                                        |
 |                                                                      |
 +----------------------------------------------------------------------*/

/*----------------------------------------------------------------------+
 |      Definitions                                                     |
 +----------------------------------------------------------------------*/

typedef struct engine *Engine_t;

/*
 *  Transposition table
 */

struct ttSlot {
        uint64_t hash;
        short loScore, hiScore;
        short move;
        unsigned short prio;
};

#define ttPrio(now, depth) ((now << 8) + depth)
#define ttDepth(prio) ((prio) & 0xff)

struct ttable {
        struct ttSlot *slots;
        size_t allocSize;
        size_t mask;
        int searchCount;
        uint64_t baseHash;
};

/*
 *  Chess engine
 */

struct engine {
        struct board board;

        struct ttable tt;

        // last search result
        struct {
                int score;
                int depth;
                intList pv;
                double seconds;
                long long nodeCount;
        };
};

/*
 *  Workaround to hide some of the ugliness, at least until ISO C supports
 *  seamless access to members of the base struct (ref. `-fms-extensions'
 *  or `kenc'). Note that this is a noop (just a type conversion) because
 *  `board' is the first element of `struct engine'.
 */
#define board(engine) (&(engine)->board)

/*----------------------------------------------------------------------+
 |      Functions                                                       |
 +----------------------------------------------------------------------*/

// callback interface for handling of search progress
typedef bool searchInfo_fn(void *infoData);

void rootSearch(Engine_t self, int maxDepth, searchInfo_fn *infoFunction, void *infoData);

/*----------------------------------------------------------------------+
 |                                                                      |
 +----------------------------------------------------------------------*/

