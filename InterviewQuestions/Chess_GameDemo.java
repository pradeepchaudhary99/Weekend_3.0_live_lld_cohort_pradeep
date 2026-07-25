
/*
Functional Requirements:
1.  Two player chess game 
2.  Intialize the board with pieces 
3.  Piece movements startegy : every piece has its own rules 
4.  Move a piece 
5.  validate a legal moves 
6.  Capture opponent pieces 
7.  Detect Check 
8.  Detect [[Checkmate]]
9.  Detect Stalemate

Pieces in a chess Game:
KING 
QUEEN 
PAWN 
KNIGHT
BISHOP
ROOK

*/



/*

Core Entities: 
Player 
Game 
Board 
Cell 
Move    [Source Cell, Destination Cell, PieceMoved, capturedPiece]
Piece [Abstract]: Color, PieceType, moveStrategy()
King 
Queen 
Knight 
Pawn 

*/

abstract class Piece{
    Color color;
    PieceType type;
    validMove();
}



class Game{
    Board 
    Player white; 
    Player black;
    Player currentPlayer;

    GameStatus status; 
    MoveHistory history;

    // Methods 
    start();
        initilazeBoard();
    move();
    switchTurn();

    isCheck();
    isCheckMate();
    isStaleMate();

}


class Board{
    Cell[][] cells;
    initilize();
    getPiece();
    movePiece();
    isValidCell();
}





public class Chess_GameDemo {
    
}
