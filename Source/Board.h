
/*----------------------------------------------------------------------+
 |                                                                      |
 |      Board.h                                                         |
 |                                                                      |
 +----------------------------------------------------------------------*/

/*
 *  Copyright (C) 1998-2015, Marcel van Kervinck
 *  All rights reserved
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *  1. Redistributions of source code must retain the above copyright
 *  notice, this list of conditions and the following disclaimer.
 *
 *  2. Redistributions in binary form must reproduce the above copyright
 *  notice, this list of conditions and the following disclaimer in the
 *  documentation and/or other materials provided with the distribution.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

/*----------------------------------------------------------------------+
 |      Definitions                                                     |
 +----------------------------------------------------------------------*/

#include "geometry.h"

static const int rankStep = rank2 - rank1;
static const int fileStep = fileB - fileA;

typedef struct board *Board_t;

struct side {
        unsigned char attacks[boardSize];
        int king;
};

/*
 *      +-----+-----+-----+-----+-----+-----+-----+-----+
 *      |   Pawns   |   Minors  |   Rooks   |Queen|King |
 *      +-----+-----+-----+-----+-----+-----+-----+-----+
 *           7..6        5..4        3..2      1     0
 */

enum {
        attackKing   = 1,
        attackQueen  = 2,
        attackRook   = 4,
        attackMinor  = 16,
        attackPawn   = 64,
};

#define maxMoves 256
#define maxMoveSize sizeof("a7-a8=N+")
#define maxFenSize 128

struct board {
        signed char squares[boardSize];

        signed char castleFlags;
        signed char enPassantPawn;
        signed char halfmoveClock; // TODO: update this field

        int plyNumber; // holds both side to move and full move number

        int eloDiff;

        /*
         *  Side data
         */
        struct side *side, *xside;
        struct side whiteSide;
        struct side blackSide;
        int sideInfoPlyNumber; // for auto-update

        /*
         *  Move undo administration
         */
        signed char undoStack[2048];
        int undoLen;
        int *movePtr; // For in move generation
};

/*
 *  Chess pieces
 */

enum piece {
        empty,
        whiteKing, whiteQueen, whiteRook, whiteBishop, whiteKnight, whitePawn,
        blackKing, blackQueen, blackRook, blackBishop, blackKnight, blackPawn
};

enum pieceColor { white = 0, black = 1 };

#define pieceColor(piece) ((piece) >= blackKing) // piece must not be 'empty'

/*
 *  Game state
 */

enum castleFlag {
        castleFlagWhiteKside = 1 << 0,
        castleFlagWhiteQside = 1 << 1,
        castleFlagBlackKside = 1 << 2,
        castleFlagBlackQside = 1 << 3
};

#define sideToMove(board) ((board)->plyNumber & 1)

/*
 *  Moves
 */

/*
 *  Move integer bits are as follows:
 *  0-5         to square
 *  6-11        from square
 *  12          special flag (castling, promotion, en passant capture, double pawn push)
 *  13-14       promotion: Q=0, R=1, B=2, N=3
 */

#define boardBits  6

#define move(from, to)          (((from) << boardBits) | (to))
enum moveFlags {
        specialMoveFlag         = 1 << (2*boardBits),
        promotionBits           = 2*boardBits + 1,
        queenPromotionFlags     = 0 << promotionBits,
        rookPromotionFlags      = 1 << promotionBits,
        bishopPromotionFlags    = 2 << promotionBits,
        knightPromotionFlags    = 3 << promotionBits
};
#define specialMove(from, to)   (specialMoveFlag | move(from, to))

#define from(move)              (((move) >> boardBits) & ~(~0<<boardBits))
#define to(move)                ( (move)               & ~(~0<<boardBits))

/*----------------------------------------------------------------------+
 |      Data                                                            |
 +----------------------------------------------------------------------*/

extern const char startpos[];

/*----------------------------------------------------------------------+
 |      Functions                                                       |
 +----------------------------------------------------------------------*/

/*
 *  Setup chess board from position description in FEN notation
 *  Return the length of the FEN on success, or 0 on failure.
 */
int setupBoard(Board_t self, const char *fen);

/*
 *  Update attack tables and king locations. To be used after
 *  setupBoard or makeMove. Used by generateMoves, inCheck.
 *  Can be invalidated by moveToStandardAlgebraic,
 *  getCheckMark, isLegalMove, normalizeEnPassantStatus.
 */
void updateSideInfo(Board_t self);

/*
 *  Convert the current position to FEN
 */
void boardToFen(Board_t self, char *fen);

/*
 *  Compute a 64-bit hash for the current position using Polyglot-Zobrist hashing
 */
unsigned long long hash64(Board_t self);

/*
 *  Generate all pseudo-legal moves for the position and return the move count
 */
int generateMoves(Board_t self, int moveList[maxMoves]);

/*
 *  Make the move on the board
 */
void makeMove(Board_t self, int move);

/*
 *  Check if last pseudo move was indeed legal
 */
static inline bool wasLegalMove(Board_t self)
{
        updateSideInfo(self);
        return self->side->attacks[self->xside->king] == 0;
}

/*
 *  Retract the last move and restore the previous position
 *  All moves can be undone all the way back to the setup position
 */
void undoMove(Board_t self);

/*
 *  Convert move to standard algebraic notation, without checkmark
 *  A movelist must be prepared by the caller for disambiguation,
 *  which may include the move itself.
 */
char *moveToStandardAlgebraic(Board_t self, char moveString[maxMoveSize], int move, int xmoves[maxMoves], int xlen);

/*
 *  Convert move to long algebraic notation, without checkmark
 */
char *moveToLongAlgebraic(Board_t self, char moveString[maxMoveSize], int move);

/*
 *  Convert move to computer notation (UCI)
 */
char *moveToUci(Board_t self, char moveString[maxMoveSize], int move);

/*
 *  Determine the check status for the current position ("", "+" or "#")
 */
const char *getCheckMark(Board_t self);

/*
 *  Parse move input, disambiguate abbreviated notations
 *  A movelist must be prepared by the caller for disambiguation.
 *  Return the length of the move on success, or <= 0 on failure:
 *   0 : Invalid move syntax
 *  -1 : Not a legal move in this position
 *  -2 : Ambiguous move
 */
extern int parseMove(Board_t self, const char *line, int xmoves[maxMoves], int xlen, int *move);

// Clear the ep flag if there are not legal moves
extern void normalizeEnPassantStatus(Board_t self);

// Side to move in check?
extern int inCheck(Board_t self);

// Is move legal? Move must come from generateMoves, so be safe to make.
extern bool isLegalMove(Board_t self, int move);

// Is the move a pawn promotion?
extern bool isPromotion(Board_t self, int from, int to);

static inline bool repetition(Board_t self)
{
        return false; // TODO: dummy
}

/*----------------------------------------------------------------------+
 |                                                                      |
 +----------------------------------------------------------------------*/

