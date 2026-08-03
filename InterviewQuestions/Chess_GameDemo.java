/*
Chess Game
------------------------------

Functional Requirements:
    Two player chess game
    Initialize the board with pieces
    Piece movement strategy: every piece has its own rules
    Move a piece
    Validate a legal move
    Capture opponent pieces
    Detect Check
    Detect Checkmate
    Detect Stalemate

Pieces in a Chess Game:
    KING, QUEEN, PAWN, KNIGHT, BISHOP, ROOK

Entities:
    Player
    Game
    Board
    Move [Source Cell, Destination Cell, PieceMoved, capturedPiece]
    Piece [Abstract]: Color, PieceType, getPossibleMoves()
    King, Queen, Rook, Bishop, Knight, Pawn

Out of scope: castling, en passant, draw by repetition/50-move rule.
*/

import java.util.ArrayList;
import java.util.List;

enum Color {
    WHITE, BLACK;

    Color opposite() {
        return this == WHITE ? BLACK : WHITE;
    }
}

enum PieceType {
    KING, QUEEN, ROOK, BISHOP, KNIGHT, PAWN
}

enum GameStatus {
    ONGOING, CHECK, CHECKMATE, STALEMATE
}

// (row, col), row 0 = rank 1 (white side), col 0 = file 'a'
final class Position {
    final int row;
    final int col;

    Position(int row, int col) {
        this.row = row;
        this.col = col;
    }

    @Override
    public boolean equals(Object other) {
        if (!(other instanceof Position)) {
            return false;
        }
        Position position = (Position) other;
        return row == position.row && col == position.col;
    }

    @Override
    public int hashCode() {
        return row * 31 + col;
    }
}

abstract class Piece {
    Color color;
    PieceType type;

    Piece(Color color, PieceType type) {
        this.color = color;
        this.type = type;
    }

    // Pseudo-legal moves: obeys movement/blocking/capture rules, ignores check safety.
    abstract List<Position> getPossibleMoves(Position position, Board board);

    // Squares this piece threatens; used for check detection. Same as possible
    // moves for every piece except the pawn, whose diagonal attacks aren't
    // conditional on an enemy being there yet.
    List<Position> getAttackedSquares(Position position, Board board) {
        return getPossibleMoves(position, board);
    }

    char symbol() {
        char letter;
        switch (type) {
            case KING: letter = 'K'; break;
            case QUEEN: letter = 'Q'; break;
            case ROOK: letter = 'R'; break;
            case BISHOP: letter = 'B'; break;
            case KNIGHT: letter = 'N'; break;
            default: letter = 'P';
        }
        return color == Color.WHITE ? letter : Character.toLowerCase(letter);
    }

    List<Position> slide(Position position, Board board, int[][] directions) {
        List<Position> moves = new ArrayList<>();
        for (int[] direction : directions) {
            int r = position.row + direction[0];
            int c = position.col + direction[1];
            while (board.isValidCell(r, c)) {
                Piece occupant = board.getPiece(r, c);
                if (occupant == null) {
                    moves.add(new Position(r, c));
                } else {
                    if (occupant.color != color) {
                        moves.add(new Position(r, c));
                    }
                    break;
                }
                r += direction[0];
                c += direction[1];
            }
        }
        return moves;
    }
}

class Rook extends Piece {
    Rook(Color color) {
        super(color, PieceType.ROOK);
    }

    @Override
    List<Position> getPossibleMoves(Position position, Board board) {
        return slide(position, board, new int[][]{{1, 0}, {-1, 0}, {0, 1}, {0, -1}});
    }
}

class Bishop extends Piece {
    Bishop(Color color) {
        super(color, PieceType.BISHOP);
    }

    @Override
    List<Position> getPossibleMoves(Position position, Board board) {
        return slide(position, board, new int[][]{{1, 1}, {1, -1}, {-1, 1}, {-1, -1}});
    }
}

class Queen extends Piece {
    Queen(Color color) {
        super(color, PieceType.QUEEN);
    }

    @Override
    List<Position> getPossibleMoves(Position position, Board board) {
        return slide(position, board, new int[][]{
                {1, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, 1}, {1, -1}, {-1, 1}, {-1, -1}
        });
    }
}

class Knight extends Piece {
    private static final int[][] OFFSETS = {
            {2, 1}, {2, -1}, {-2, 1}, {-2, -1}, {1, 2}, {1, -2}, {-1, 2}, {-1, -2}
    };

    Knight(Color color) {
        super(color, PieceType.KNIGHT);
    }

    @Override
    List<Position> getPossibleMoves(Position position, Board board) {
        List<Position> moves = new ArrayList<>();
        for (int[] offset : OFFSETS) {
            int r = position.row + offset[0];
            int c = position.col + offset[1];
            if (board.isValidCell(r, c)) {
                Piece occupant = board.getPiece(r, c);
                if (occupant == null || occupant.color != color) {
                    moves.add(new Position(r, c));
                }
            }
        }
        return moves;
    }
}

class King extends Piece {
    King(Color color) {
        super(color, PieceType.KING);
    }

    @Override
    List<Position> getPossibleMoves(Position position, Board board) {
        List<Position> moves = new ArrayList<>();
        for (int dRow = -1; dRow <= 1; dRow++) {
            for (int dCol = -1; dCol <= 1; dCol++) {
                if (dRow == 0 && dCol == 0) {
                    continue;
                }
                int r = position.row + dRow;
                int c = position.col + dCol;
                if (board.isValidCell(r, c)) {
                    Piece occupant = board.getPiece(r, c);
                    if (occupant == null || occupant.color != color) {
                        moves.add(new Position(r, c));
                    }
                }
            }
        }
        return moves;
    }
}

class Pawn extends Piece {
    Pawn(Color color) {
        super(color, PieceType.PAWN);
    }

    @Override
    List<Position> getPossibleMoves(Position position, Board board) {
        int direction = color == Color.WHITE ? 1 : -1;
        int startRow = color == Color.WHITE ? 1 : 6;
        List<Position> moves = new ArrayList<>();

        int oneStepRow = position.row + direction;
        if (board.isValidCell(oneStepRow, position.col) && board.getPiece(oneStepRow, position.col) == null) {
            moves.add(new Position(oneStepRow, position.col));
            int twoStepRow = position.row + 2 * direction;
            if (position.row == startRow && board.getPiece(twoStepRow, position.col) == null) {
                moves.add(new Position(twoStepRow, position.col));
            }
        }

        for (int dCol : new int[]{-1, 1}) {
            int r = position.row + direction;
            int c = position.col + dCol;
            if (board.isValidCell(r, c)) {
                Piece occupant = board.getPiece(r, c);
                if (occupant != null && occupant.color != color) {
                    moves.add(new Position(r, c));
                }
            }
        }
        return moves;
    }

    @Override
    List<Position> getAttackedSquares(Position position, Board board) {
        int direction = color == Color.WHITE ? 1 : -1;
        List<Position> squares = new ArrayList<>();
        for (int dCol : new int[]{-1, 1}) {
            int r = position.row + direction;
            int c = position.col + dCol;
            if (board.isValidCell(r, c)) {
                squares.add(new Position(r, c));
            }
        }
        return squares;
    }
}

final class Move {
    final Position source;
    final Position destination;
    final Piece pieceMoved;
    final Piece capturedPiece;

    Move(Position source, Position destination, Piece pieceMoved, Piece capturedPiece) {
        this.source = source;
        this.destination = destination;
        this.pieceMoved = pieceMoved;
        this.capturedPiece = capturedPiece;
    }
}

class Board {
    static final int SIZE = 8;
    private final Piece[][] cells = new Piece[SIZE][SIZE];

    Board() {
        initialize();
    }

    void initialize() {
        PieceType[] backRank = {
                PieceType.ROOK, PieceType.KNIGHT, PieceType.BISHOP, PieceType.QUEEN,
                PieceType.KING, PieceType.BISHOP, PieceType.KNIGHT, PieceType.ROOK
        };
        for (int col = 0; col < SIZE; col++) {
            cells[0][col] = makePiece(backRank[col], Color.WHITE);
            cells[7][col] = makePiece(backRank[col], Color.BLACK);
        }
        for (int col = 0; col < SIZE; col++) {
            cells[1][col] = new Pawn(Color.WHITE);
            cells[6][col] = new Pawn(Color.BLACK);
        }
    }

    private static Piece makePiece(PieceType type, Color color) {
        switch (type) {
            case ROOK: return new Rook(color);
            case KNIGHT: return new Knight(color);
            case BISHOP: return new Bishop(color);
            case QUEEN: return new Queen(color);
            case KING: return new King(color);
            default: throw new IllegalArgumentException("Unsupported back-rank piece type");
        }
    }

    boolean isValidCell(int row, int col) {
        return row >= 0 && row < SIZE && col >= 0 && col < SIZE;
    }

    boolean isValidCell(Position position) {
        return isValidCell(position.row, position.col);
    }

    Piece getPiece(int row, int col) {
        return cells[row][col];
    }

    Piece getPiece(Position position) {
        return cells[position.row][position.col];
    }

    void setPiece(Position position, Piece piece) {
        cells[position.row][position.col] = piece;
    }

    Piece movePiece(Position source, Position destination) {
        Piece captured = getPiece(destination);
        setPiece(destination, getPiece(source));
        setPiece(source, null);
        return captured;
    }

    Position findKing(Color color) {
        for (int row = 0; row < SIZE; row++) {
            for (int col = 0; col < SIZE; col++) {
                Piece piece = cells[row][col];
                if (piece != null && piece.type == PieceType.KING && piece.color == color) {
                    return new Position(row, col);
                }
            }
        }
        return null;
    }

    List<Object[]> allPieces(Color color) {
        List<Object[]> result = new ArrayList<>();
        for (int row = 0; row < SIZE; row++) {
            for (int col = 0; col < SIZE; col++) {
                Piece piece = cells[row][col];
                if (piece != null && piece.color == color) {
                    result.add(new Object[]{new Position(row, col), piece});
                }
            }
        }
        return result;
    }

    void printBoard() {
        for (int row = SIZE - 1; row >= 0; row--) {
            StringBuilder rank = new StringBuilder();
            rank.append(row + 1).append(" ");
            for (int col = 0; col < SIZE; col++) {
                Piece piece = cells[row][col];
                rank.append(piece != null ? piece.symbol() : '.').append(' ');
            }
            System.out.println(rank.toString().stripTrailing());
        }
        System.out.println("  a b c d e f g h");
    }
}

class Player {
    String name;
    Color color;

    Player(String name, Color color) {
        this.name = name;
        this.color = color;
    }
}

class IllegalMoveError extends RuntimeException {
    IllegalMoveError(String message) {
        super(message);
    }
}

class Game {
    Board board;
    Player white;
    Player black;
    Player currentPlayer;
    GameStatus status;
    List<Move> history = new ArrayList<>();

    Game(String whiteName, String blackName) {
        board = new Board();
        white = new Player(whiteName, Color.WHITE);
        black = new Player(blackName, Color.BLACK);
        currentPlayer = white;
        status = GameStatus.ONGOING;
    }

    void start() {
        board.initialize();
        currentPlayer = white;
        status = GameStatus.ONGOING;
        history.clear();
    }

    void switchTurn() {
        currentPlayer = currentPlayer == white ? black : white;
    }

    boolean isSquareAttacked(Position position, Color byColor) {
        for (Object[] entry : board.allPieces(byColor)) {
            Position piecePosition = (Position) entry[0];
            Piece piece = (Piece) entry[1];
            if (piece.getAttackedSquares(piecePosition, board).contains(position)) {
                return true;
            }
        }
        return false;
    }

    boolean isCheck(Color color) {
        Position kingPosition = board.findKing(color);
        if (kingPosition == null) {
            return false;
        }
        return isSquareAttacked(kingPosition, color.opposite());
    }

    private boolean hasLegalMove(Color color) {
        for (Object[] entry : board.allPieces(color)) {
            Position source = (Position) entry[0];
            Piece piece = (Piece) entry[1];
            for (Position destination : piece.getPossibleMoves(source, board)) {
                Piece captured = board.movePiece(source, destination);
                boolean stillInCheck = isCheck(color);
                board.setPiece(source, piece);
                board.setPiece(destination, captured);
                if (!stillInCheck) {
                    return true;
                }
            }
        }
        return false;
    }

    boolean isCheckmate(Color color) {
        return isCheck(color) && !hasLegalMove(color);
    }

    boolean isStalemate(Color color) {
        return !isCheck(color) && !hasLegalMove(color);
    }

    Move move(Position source, Position destination) {
        if (status == GameStatus.CHECKMATE || status == GameStatus.STALEMATE) {
            throw new IllegalMoveError("Game is already over");
        }

        Piece piece = board.getPiece(source);
        if (piece == null || piece.color != currentPlayer.color) {
            throw new IllegalMoveError("No movable piece of the current player at source");
        }

        if (!piece.getPossibleMoves(source, board).contains(destination)) {
            throw new IllegalMoveError("Illegal move for this piece");
        }

        Piece captured = board.movePiece(source, destination);
        if (isCheck(currentPlayer.color)) {
            // Undo: this move would leave (or keep) our own king in check.
            board.setPiece(source, piece);
            board.setPiece(destination, captured);
            throw new IllegalMoveError("Move leaves own king in check");
        }

        if (piece.type == PieceType.PAWN && (destination.row == 0 || destination.row == Board.SIZE - 1)) {
            piece = new Queen(piece.color);
            board.setPiece(destination, piece);
        }

        Move performedMove = new Move(source, destination, piece, captured);
        history.add(performedMove);

        switchTurn();
        Color opponentColor = currentPlayer.color;
        if (isCheckmate(opponentColor)) {
            status = GameStatus.CHECKMATE;
        } else if (isCheck(opponentColor)) {
            status = GameStatus.CHECK;
        } else if (isStalemate(opponentColor)) {
            status = GameStatus.STALEMATE;
        } else {
            status = GameStatus.ONGOING;
        }

        return performedMove;
    }

    static Position parseSquare(String square) {
        int col = square.charAt(0) - 'a';
        int row = square.charAt(1) - '1';
        return new Position(row, col);
    }
}

public class Chess_GameDemo {
    public static void main(String[] args) {
        Game game = new Game("Alice", "Bob");
        game.start();

        // Fool's Mate -- fastest possible checkmate, four half-moves.
        String[] moves = {"f2f3", "e7e5", "g2g4", "d8h4"};
        for (String moveStr : moves) {
            Position source = Game.parseSquare(moveStr.substring(0, 2));
            Position destination = Game.parseSquare(moveStr.substring(2, 4));
            String mover = game.currentPlayer.name;
            game.move(source, destination);
            System.out.println(mover + " played " + moveStr + " -> status: " + game.status);
        }

        System.out.println();
        game.board.printBoard();
        System.out.println("\nFinal status: " + game.status);
    }
}
