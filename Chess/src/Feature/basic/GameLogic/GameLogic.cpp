#include "GameLogic.h"

// Initialize static members
bool GameLogic::enPassantAvailable = false;
ChessSquare GameLogic::enPassantSquare(0, 0);
int GameLogic::enPassantVictimIndex = -1;

bool GameLogic::isValidSquare(int file, int rank) {
    return file >= 0 && file < 8 && rank >= 0 && rank < 8;
}

bool GameLogic::isSquareOccupied(int file, int rank, const std::vector<Piece>& pieces, int excludePiece) {
    for(size_t i = 0; i < pieces.size(); ++i) {
        if (i != excludePiece && pieces[i].file == file && pieces[i].rank == rank) {
            return true;
        }
    }
    return false;
}

bool GameLogic::isSquareOccupiedByEnemy(int file, int rank, bool isWhite, const std::vector<Piece>& pieces, int excludePiece) {
    for(size_t i = 0; i < pieces.size(); ++i) {
        if (i != excludePiece && pieces[i].file == file && pieces[i].rank == rank && pieces[i].isWhite != isWhite) {
            return true;
        }
    }
    return false;
}

std::vector<ChessSquare> GameLogic::getAllowableMoves(int pieceIndex, const std::vector<Piece>& pieces) {
    std::vector<ChessSquare> allowableMoves;

    if (pieceIndex < 0 || pieceIndex >= pieces.size()) return allowableMoves;

    const Piece& piece = pieces[pieceIndex];
    int currentFile = piece.file;
    int currentRank = piece.rank;
    bool isWhite = piece.isWhite;

    auto squareOccupiedByFriend = [&](int nf, int nr){
        for (const auto& p : pieces) if (p.file == nf && p.rank == nr) return p.isWhite == isWhite; return false;
    };
    auto squareOccupiedByEnemy = [&](int nf, int nr){
        for (const auto& p : pieces) if (p.file == nf && p.rank == nr) return p.isWhite != isWhite; return false;
    };

    switch (piece.type) {
        case PieceType::PAWN: {
            int dir = isWhite ? 1 : -1;
            int one = currentRank + dir;
            if (isValidSquare(currentFile, one) && !isSquareOccupied(currentFile, one, pieces)) {
                allowableMoves.push_back(ChessSquare(currentFile, one));
                if (!piece.hasMoved) {
                    int two = currentRank + 2 * dir;
                    if (isValidSquare(currentFile, two) && !isSquareOccupied(currentFile, two, pieces)) 
                        allowableMoves.push_back(ChessSquare(currentFile, two));
                }
            }
            // captures
            for (int df : {-1, 1}) {
                int nf = currentFile + df; int nr = currentRank + dir;
                if (!isValidSquare(nf, nr)) continue;
                if (squareOccupiedByEnemy(nf, nr)) allowableMoves.push_back(ChessSquare(nf, nr));
            }
            // en-passant
            if (enPassantAvailable) {
                for (int df : {-1, 1}) {
                    int nf = currentFile + df; int nr = currentRank + dir;
                    if (nf == enPassantSquare.file && nr == enPassantSquare.rank) 
                        allowableMoves.push_back(enPassantSquare);
                }
            }
            break;
        }
        case PieceType::ROOK: {
            auto addLine = [&](int sf, int sr){
                for (int s = 1; s < 8; ++s) {
                    int nf = currentFile + s*sf, nr = currentRank + s*sr;
                    if (!isValidSquare(nf, nr)) break;
                    if (!isSquareOccupied(nf, nr, pieces)) allowableMoves.push_back(ChessSquare(nf, nr));
                    else { if (squareOccupiedByEnemy(nf, nr)) allowableMoves.push_back(ChessSquare(nf, nr)); break; }
                }
            };
            addLine(1,0); addLine(-1,0); addLine(0,1); addLine(0,-1);
            break;
        }
        case PieceType::BISHOP: {
            auto addLine = [&](int sf, int sr){
                for (int s = 1; s < 8; ++s) {
                    int nf = currentFile + s*sf, nr = currentRank + s*sr;
                    if (!isValidSquare(nf, nr)) break;
                    if (!isSquareOccupied(nf, nr, pieces)) allowableMoves.push_back(ChessSquare(nf, nr));
                    else { if (squareOccupiedByEnemy(nf, nr)) allowableMoves.push_back(ChessSquare(nf, nr)); break; }
                }
            };
            addLine(1,1); addLine(1,-1); addLine(-1,1); addLine(-1,-1);
            break;
        }
        case PieceType::QUEEN: {
            auto addLine = [&](int sf, int sr){
                for (int s = 1; s < 8; ++s) {
                    int nf = currentFile + s*sf, nr = currentRank + s*sr;
                    if (!isValidSquare(nf, nr)) break;
                    if (!isSquareOccupied(nf, nr, pieces)) allowableMoves.push_back(ChessSquare(nf, nr));
                    else { if (squareOccupiedByEnemy(nf, nr)) allowableMoves.push_back(ChessSquare(nf, nr)); break; }
                }
            };
            addLine(1,0); addLine(-1,0); addLine(0,1); addLine(0,-1);
            addLine(1,1); addLine(1,-1); addLine(-1,1); addLine(-1,-1);
            break;
        }
        case PieceType::KNIGHT: {
            int moves[8][2] = {{-2,-1},{-2,1},{-1,-2},{-1,2},{1,-2},{1,2},{2,-1},{2,1}};
            for (auto &m : moves) {
                int nf = currentFile + m[0], nr = currentRank + m[1];
                if (!isValidSquare(nf, nr)) continue;
                if (!squareOccupiedByFriend(nf, nr)) allowableMoves.push_back(ChessSquare(nf, nr));
            }
            break;
        }
        case PieceType::KING: {
            for (int df = -1; df <= 1; ++df) for (int dr = -1; dr <= 1; ++dr) {
                if (df == 0 && dr == 0) continue;
                int nf = currentFile + df, nr = currentRank + dr;
                if (!isValidSquare(nf, nr)) continue;
                if (!squareOccupiedByFriend(nf, nr)) allowableMoves.push_back(ChessSquare(nf, nr));
            }
            // castling (simplified; ignores check)
            if (!piece.hasMoved) {
                // king side
                int rf = 7; int rr = currentRank; int rookIdx = -1;
                for (int i=0;i<(int)pieces.size();++i) if (pieces[i].type==PieceType::ROOK && pieces[i].isWhite==isWhite && pieces[i].rank==rr && pieces[i].file==rf) { rookIdx = i; break; }
                if (rookIdx>=0 && !pieces[rookIdx].hasMoved && !isSquareOccupied(currentFile+1, rr, pieces) && !isSquareOccupied(currentFile+2, rr, pieces))
                    allowableMoves.push_back(ChessSquare(currentFile+2, rr));
                // queen side
                rf = 0; rr = currentRank; rookIdx = -1;
                for (int i=0;i<(int)pieces.size();++i) if (pieces[i].type==PieceType::ROOK && pieces[i].isWhite==isWhite && pieces[i].rank==rr && pieces[i].file==rf) { rookIdx = i; break; }
                if (rookIdx>=0 && !pieces[rookIdx].hasMoved && !isSquareOccupied(currentFile-1, rr, pieces) && !isSquareOccupied(currentFile-2, rr, pieces) && !isSquareOccupied(currentFile-3, rr, pieces))
                    allowableMoves.push_back(ChessSquare(currentFile-2, rr));
            }
            break;
        }
    }

    return allowableMoves;
}

bool GameLogic::isValidMove(int pieceIndex, int targetFile, int targetRank, const std::vector<Piece>& pieces) {
    if (!isValidSquare(targetFile, targetRank)) return false;
    const std::vector<ChessSquare> allowableMoves = getAllowableMoves(pieceIndex, pieces);
    for (const ChessSquare& m : allowableMoves) {
        if (m.file == targetFile && m.rank == targetRank) return true;
    }
    return false;
}

void GameLogic::markCaptured(std::vector<Piece>& pieces, int idx) {
    if (idx < 0 || idx >= (int)pieces.size()) return;
    pieces[idx].file = -999;
    pieces[idx].rank = -999;
    pieces[idx].model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -10.0f, 0.0f));
}

int GameLogic::findKingIndex(const std::vector<Piece>& pcs, bool isWhite) {
    for (int i=0;i<(int)pcs.size();++i) if (pcs[i].type==PieceType::KING && pcs[i].file>=0 && pcs[i].rank>=0 && pcs[i].isWhite==isWhite) return i;
    return -1;
}

static bool squareOccupied(const std::vector<Piece>& pcs, int f, int r) {
    for (auto& p: pcs) if (p.file==f && p.rank==r) return true; return false;
}

bool GameLogic::isSquareAttacked(const std::vector<Piece>& pcs, int f, int r, bool byWhite) {
    // Pawns
    int dir = byWhite ? 1 : -1;
    for (auto& p: pcs) {
        if (p.file<0||p.rank<0||p.isWhite!=byWhite) continue;
        switch (p.type) {
            case PieceType::PAWN: {
                if ((p.file+1==f && p.rank+dir==r) || (p.file-1==f && p.rank+dir==r)) return true; break;
            }
            case PieceType::KNIGHT: {
                int mv[8][2]={{-2,-1},{-2,1},{-1,-2},{-1,2},{1,-2},{1,2},{2,-1},{2,1}}; for (auto&m:mv){ if (p.file+m[0]==f && p.rank+m[1]==r) return true; } break;
            }
            case PieceType::BISHOP: case PieceType::ROOK: case PieceType::QUEEN: {
                auto ray=[&](int sf,int sr){ for(int s=1;s<8;++s){int nf=p.file+s*sf,nr=p.rank+s*sr; if(nf<0||nf>7||nr<0||nr>7) break; if(nf==f&&nr==r) return true; if(squareOccupied(pcs,nf,nr)) break;} return false;};
                if (p.type==PieceType::BISHOP||p.type==PieceType::QUEEN){ if(ray(1,1)||ray(1,-1)||ray(-1,1)||ray(-1,-1)) return true; }
                if (p.type==PieceType::ROOK||p.type==PieceType::QUEEN){ if(ray(1,0)||ray(-1,0)||ray(0,1)||ray(0,-1)) return true; }
                break;
            }
            case PieceType::KING: {
                for(int df=-1;df<=1;++df)for(int dr=-1;dr<=1;++dr){ if(df==0&&dr==0)continue; if(p.file+df==f&&p.rank+dr==r) return true; } break;
            }
        }
    }
    return false;
}

bool GameLogic::isSideInCheck(const std::vector<Piece>& pcs, bool sideIsWhite) {
    int k = findKingIndex(pcs, sideIsWhite);
    if (k < 0) return false;
    return isSquareAttacked(pcs, pcs[k].file, pcs[k].rank, !sideIsWhite);
}

bool GameLogic::canCommitMove(const std::vector<Piece>& pcs, int moverIdx, ChessSquare from, ChessSquare to, bool sideToMove) {
    if (moverIdx < 0 || moverIdx >= (int)pcs.size()) return false;
    if (pcs[moverIdx].isWhite != sideToMove) return false;
    // Final legality check including king safety and special cases
    return wouldBeLegalMove(pcs, moverIdx, from, to, enPassantAvailable, enPassantSquare);
}

bool GameLogic::wouldBeLegalMove(const std::vector<Piece>& piecesIn, int moverIdx, ChessSquare from, ChessSquare to, bool enPassAvail, ChessSquare enPassSq) {
    std::vector<Piece> pcs = piecesIn; if (moverIdx<0||moverIdx>=(int)pcs.size()) return false; Piece& m = pcs[moverIdx]; bool side=m.isWhite;
    // First ensure the move obeys the piece's movement rules (pseudo-legal)
    {
        const std::vector<ChessSquare> pseudo = getAllowableMoves(moverIdx, piecesIn);
        bool found = false; for (const auto& mv : pseudo) if (mv.file==to.file && mv.rank==to.rank) { found = true; break; }
        if (!found) return false;
    }
    // capture target or en-passant victim
    int captured=-1;
    for (int i=0;i<(int)pcs.size();++i) if (i!=moverIdx && pcs[i].file==to.file && pcs[i].rank==to.rank && pcs[i].isWhite!=side) { captured=i; break; }
    if (m.type==PieceType::PAWN && enPassAvail && to.file==enPassSq.file && to.rank==enPassSq.rank) {
        int dir = side?1:-1; int victimRank = to.rank - dir; for (int i=0;i<(int)pcs.size();++i) if (pcs[i].file==to.file && pcs[i].rank==victimRank && pcs[i].type==PieceType::PAWN && pcs[i].isWhite!=side) { captured=i; break; }
    }
    if (captured>=0) { pcs[captured].file=-999; pcs[captured].rank=-999; }
    // castling rook move in simulation
    if (m.type==PieceType::KING && abs(to.file-from.file)==2) {
        // Additional rules: king cannot castle out of, through, or into check
        // 1) current square not attacked
        if (isSquareAttacked(piecesIn, from.file, from.rank, !side)) return false;
        // 2) square it passes through not attacked
        int step = (to.file>from.file)? 1 : -1; int midFile = from.file + step;
        if (isSquareAttacked(piecesIn, midFile, from.rank, !side)) return false;
        int rr = from.rank; if (to.file>from.file) { // king side
            for(int i=0;i<(int)pcs.size();++i) if(pcs[i].type==PieceType::ROOK && pcs[i].isWhite==side && pcs[i].rank==rr && pcs[i].file==7) { pcs[i].file=5; break; }
        } else { // queen side
            for(int i=0;i<(int)pcs.size();++i) if(pcs[i].type==PieceType::ROOK && pcs[i].isWhite==side && pcs[i].rank==rr && pcs[i].file==0) { pcs[i].file=3; break; }
        }
    }
    // move king/piece
    pcs[moverIdx].file=to.file; pcs[moverIdx].rank=to.rank;
    int kingIdx = findKingIndex(pcs, side);
    if (kingIdx<0) return false;
    return !isSquareAttacked(pcs, pcs[kingIdx].file, pcs[kingIdx].rank, !side);
}

std::vector<ChessSquare> GameLogic::getLegalMovesConsideringCheck(int pieceIndex, const std::vector<Piece>& pcs) {
    std::vector<ChessSquare> legals;
    if (pieceIndex < 0 || pieceIndex >= (int)pcs.size()) return legals;
    const Piece& p = pcs[pieceIndex];
    auto pseudo = getAllowableMoves(pieceIndex, pcs);
    ChessSquare from(p.file, p.rank);
    for (const auto& mv : pseudo) {
        if (wouldBeLegalMove(pcs, pieceIndex, from, mv, enPassantAvailable, enPassantSquare)) legals.push_back(mv);
    }
    return legals;
}

ChessSquare GameLogic::notationToSquare(const std::string& notation) {
    if (notation.length() != 2) return ChessSquare(0, 0);
    int file = notation[0] - 'a';
    int rank = notation[1] - '1';
    return ChessSquare(file, rank);
}

int GameLogic::squareToPieceIndex(const ChessSquare& square) {
    for (int i = 0; i < 32; ++i) {
        ChessSquare expected = pieceIndexToSquare(i);
        if (expected.file == square.file && expected.rank == square.rank) {
            return i;
        }
    }
    return -1;
}

ChessSquare GameLogic::pieceIndexToSquare(int pieceIndex) {
    if (pieceIndex < 8) return ChessSquare(7 - pieceIndex, 0);            // White back rank
    if (pieceIndex < 16) return ChessSquare(7 - (pieceIndex - 8), 1);     // White pawns
    if (pieceIndex < 24) return ChessSquare(7 - (pieceIndex - 16), 6);    // Black pawns
    if (pieceIndex < 32) return ChessSquare(7 - (pieceIndex - 24), 7);    // Black back rank
    return ChessSquare(0, 0); // Invalid
}
