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

#include <algorithm>
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

using Position = std::pair<int, int>;  // (row, col), row 0 = rank 1 (white side), col 0 = file 'a'

enum class Color { WHITE, BLACK };

Color opposite(Color color) { return color == Color::WHITE ? Color::BLACK : Color::WHITE; }

enum class PieceType { KING, QUEEN, ROOK, BISHOP, KNIGHT, PAWN };

enum class GameStatus { ONGOING, CHECK, CHECKMATE, STALEMATE };

class Board;

class Piece {
public:
    Piece(Color color, PieceType type) : color_(color), type_(type) {}
    virtual ~Piece() = default;

    // Pseudo-legal moves: obeys movement/blocking/capture rules, ignores check safety.
    virtual std::vector<Position> getPossibleMoves(Position position, const Board& board) const = 0;

    // Squares this piece threatens; used for check detection. Same as possible
    // moves for every piece except the pawn, whose diagonal attacks aren't
    // conditional on an enemy being there yet.
    virtual std::vector<Position> getAttackedSquares(Position position, const Board& board) const {
        return getPossibleMoves(position, board);
    }

    char symbol() const {
        char letter;
        switch (type_) {
            case PieceType::KING: letter = 'K'; break;
            case PieceType::QUEEN: letter = 'Q'; break;
            case PieceType::ROOK: letter = 'R'; break;
            case PieceType::BISHOP: letter = 'B'; break;
            case PieceType::KNIGHT: letter = 'N'; break;
            case PieceType::PAWN: letter = 'P'; break;
        }
        return color_ == Color::WHITE ? letter : static_cast<char>(std::tolower(letter));
    }

    Color color() const { return color_; }
    PieceType type() const { return type_; }

protected:
    std::vector<Position> slide(Position position, const Board& board,
                                 const std::vector<Position>& directions) const;

    Color color_;
    PieceType type_;
};

class Board {
public:
    static constexpr int SIZE = 8;

    Board() : cells_(SIZE, std::vector<std::shared_ptr<Piece>>(SIZE, nullptr)) {}

    void initialize();

    bool isValidCell(Position position) const {
        auto [row, col] = position;
        return row >= 0 && row < SIZE && col >= 0 && col < SIZE;
    }

    std::shared_ptr<Piece> getPiece(Position position) const {
        auto [row, col] = position;
        return cells_[row][col];
    }

    void setPiece(Position position, std::shared_ptr<Piece> piece) {
        auto [row, col] = position;
        cells_[row][col] = std::move(piece);
    }

    std::shared_ptr<Piece> movePiece(Position source, Position destination) {
        auto captured = getPiece(destination);
        setPiece(destination, getPiece(source));
        setPiece(source, nullptr);
        return captured;
    }

    std::optional<Position> findKing(Color color) const {
        for (int row = 0; row < SIZE; ++row) {
            for (int col = 0; col < SIZE; ++col) {
                auto piece = cells_[row][col];
                if (piece && piece->type() == PieceType::KING && piece->color() == color) {
                    return Position{row, col};
                }
            }
        }
        return std::nullopt;
    }

    std::vector<std::pair<Position, std::shared_ptr<Piece>>> allPieces(Color color) const {
        std::vector<std::pair<Position, std::shared_ptr<Piece>>> result;
        for (int row = 0; row < SIZE; ++row) {
            for (int col = 0; col < SIZE; ++col) {
                auto piece = cells_[row][col];
                if (piece && piece->color() == color) {
                    result.push_back({Position{row, col}, piece});
                }
            }
        }
        return result;
    }

    void printBoard() const {
        for (int row = SIZE - 1; row >= 0; --row) {
            std::cout << (row + 1) << " ";
            for (int col = 0; col < SIZE; ++col) {
                auto piece = cells_[row][col];
                std::cout << (piece ? piece->symbol() : '.') << " ";
            }
            std::cout << "\n";
        }
        std::cout << "  a b c d e f g h\n";
    }

private:
    std::vector<std::vector<std::shared_ptr<Piece>>> cells_;
};

std::vector<Position> Piece::slide(Position position, const Board& board,
                                    const std::vector<Position>& directions) const {
    std::vector<Position> moves;
    auto [row, col] = position;
    for (auto [dRow, dCol] : directions) {
        int r = row + dRow;
        int c = col + dCol;
        while (board.isValidCell({r, c})) {
            auto occupant = board.getPiece({r, c});
            if (!occupant) {
                moves.push_back({r, c});
            } else {
                if (occupant->color() != color_) {
                    moves.push_back({r, c});
                }
                break;
            }
            r += dRow;
            c += dCol;
        }
    }
    return moves;
}

class Rook : public Piece {
public:
    explicit Rook(Color color) : Piece(color, PieceType::ROOK) {}

    std::vector<Position> getPossibleMoves(Position position, const Board& board) const override {
        return slide(position, board, {{1, 0}, {-1, 0}, {0, 1}, {0, -1}});
    }
};

class Bishop : public Piece {
public:
    explicit Bishop(Color color) : Piece(color, PieceType::BISHOP) {}

    std::vector<Position> getPossibleMoves(Position position, const Board& board) const override {
        return slide(position, board, {{1, 1}, {1, -1}, {-1, 1}, {-1, -1}});
    }
};

class Queen : public Piece {
public:
    explicit Queen(Color color) : Piece(color, PieceType::QUEEN) {}

    std::vector<Position> getPossibleMoves(Position position, const Board& board) const override {
        return slide(position, board,
                      {{1, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, 1}, {1, -1}, {-1, 1}, {-1, -1}});
    }
};

class Knight : public Piece {
public:
    explicit Knight(Color color) : Piece(color, PieceType::KNIGHT) {}

    std::vector<Position> getPossibleMoves(Position position, const Board& board) const override {
        auto [row, col] = position;
        std::vector<Position> moves;
        static const std::vector<Position> offsets = {
            {2, 1}, {2, -1}, {-2, 1}, {-2, -1}, {1, 2}, {1, -2}, {-1, 2}, {-1, -2},
        };
        for (auto [dRow, dCol] : offsets) {
            Position candidate{row + dRow, col + dCol};
            if (board.isValidCell(candidate)) {
                auto occupant = board.getPiece(candidate);
                if (!occupant || occupant->color() != color_) {
                    moves.push_back(candidate);
                }
            }
        }
        return moves;
    }
};

class King : public Piece {
public:
    explicit King(Color color) : Piece(color, PieceType::KING) {}

    std::vector<Position> getPossibleMoves(Position position, const Board& board) const override {
        auto [row, col] = position;
        std::vector<Position> moves;
        for (int dRow = -1; dRow <= 1; ++dRow) {
            for (int dCol = -1; dCol <= 1; ++dCol) {
                if (dRow == 0 && dCol == 0) continue;
                Position candidate{row + dRow, col + dCol};
                if (board.isValidCell(candidate)) {
                    auto occupant = board.getPiece(candidate);
                    if (!occupant || occupant->color() != color_) {
                        moves.push_back(candidate);
                    }
                }
            }
        }
        return moves;
    }
};

class Pawn : public Piece {
public:
    explicit Pawn(Color color) : Piece(color, PieceType::PAWN) {}

    std::vector<Position> getPossibleMoves(Position position, const Board& board) const override {
        auto [row, col] = position;
        int direction = color_ == Color::WHITE ? 1 : -1;
        int startRow = color_ == Color::WHITE ? 1 : 6;
        std::vector<Position> moves;

        Position oneStep{row + direction, col};
        if (board.isValidCell(oneStep) && !board.getPiece(oneStep)) {
            moves.push_back(oneStep);
            Position twoStep{row + 2 * direction, col};
            if (row == startRow && !board.getPiece(twoStep)) {
                moves.push_back(twoStep);
            }
        }

        for (int dCol : {-1, 1}) {
            Position capture{row + direction, col + dCol};
            if (board.isValidCell(capture)) {
                auto occupant = board.getPiece(capture);
                if (occupant && occupant->color() != color_) {
                    moves.push_back(capture);
                }
            }
        }
        return moves;
    }

    std::vector<Position> getAttackedSquares(Position position, const Board& board) const override {
        auto [row, col] = position;
        int direction = color_ == Color::WHITE ? 1 : -1;
        std::vector<Position> squares;
        for (int dCol : {-1, 1}) {
            Position candidate{row + direction, col + dCol};
            if (board.isValidCell(candidate)) {
                squares.push_back(candidate);
            }
        }
        return squares;
    }
};

void Board::initialize() {
    static const std::vector<PieceType> backRank = {
        PieceType::ROOK, PieceType::KNIGHT, PieceType::BISHOP, PieceType::QUEEN,
        PieceType::KING, PieceType::BISHOP, PieceType::KNIGHT, PieceType::ROOK,
    };
    auto makePiece = [](PieceType type, Color color) -> std::shared_ptr<Piece> {
        switch (type) {
            case PieceType::ROOK: return std::make_shared<Rook>(color);
            case PieceType::KNIGHT: return std::make_shared<Knight>(color);
            case PieceType::BISHOP: return std::make_shared<Bishop>(color);
            case PieceType::QUEEN: return std::make_shared<Queen>(color);
            case PieceType::KING: return std::make_shared<King>(color);
            default: throw std::invalid_argument("Unsupported back-rank piece type");
        }
    };
    for (int col = 0; col < SIZE; ++col) {
        cells_[0][col] = makePiece(backRank[col], Color::WHITE);
        cells_[7][col] = makePiece(backRank[col], Color::BLACK);
    }
    for (int col = 0; col < SIZE; ++col) {
        cells_[1][col] = std::make_shared<Pawn>(Color::WHITE);
        cells_[6][col] = std::make_shared<Pawn>(Color::BLACK);
    }
}

struct Move {
    Position source;
    Position destination;
    std::shared_ptr<Piece> pieceMoved;
    std::shared_ptr<Piece> capturedPiece;
};

struct Player {
    std::string name;
    Color color;
};

Position parseSquare(const std::string& square) {
    int col = square[0] - 'a';
    int row = square[1] - '1';
    return {row, col};
}

class IllegalMoveError : public std::runtime_error {
public:
    explicit IllegalMoveError(const std::string& message) : std::runtime_error(message) {}
};

class Game {
public:
    Game(std::string whiteName = "White", std::string blackName = "Black")
        : white_{std::move(whiteName), Color::WHITE},
          black_{std::move(blackName), Color::BLACK},
          currentPlayer_(&white_),
          status_(GameStatus::ONGOING) {}

    void start() {
        board_ = Board();
        board_.initialize();
        currentPlayer_ = &white_;
        status_ = GameStatus::ONGOING;
        history_.clear();
    }

    void switchTurn() { currentPlayer_ = currentPlayer_ == &white_ ? &black_ : &white_; }

    bool isSquareAttacked(Position position, Color byColor) const {
        for (auto& [piecePosition, piece] : board_.allPieces(byColor)) {
            auto attacked = piece->getAttackedSquares(piecePosition, board_);
            if (std::find(attacked.begin(), attacked.end(), position) != attacked.end()) {
                return true;
            }
        }
        return false;
    }

    bool isCheck(Color color) const {
        auto kingPosition = board_.findKing(color);
        if (!kingPosition) return false;
        return isSquareAttacked(*kingPosition, opposite(color));
    }

    bool isCheckmate(Color color) { return isCheck(color) && !hasLegalMove(color); }

    bool isStalemate(Color color) { return !isCheck(color) && !hasLegalMove(color); }

    Move move(Position source, Position destination) {
        if (status_ == GameStatus::CHECKMATE || status_ == GameStatus::STALEMATE) {
            throw IllegalMoveError("Game is already over");
        }

        auto piece = board_.getPiece(source);
        if (!piece || piece->color() != currentPlayer_->color) {
            throw IllegalMoveError("No movable piece of the current player at source");
        }

        auto possible = piece->getPossibleMoves(source, board_);
        if (std::find(possible.begin(), possible.end(), destination) == possible.end()) {
            throw IllegalMoveError("Illegal move for this piece");
        }

        auto captured = board_.movePiece(source, destination);
        if (isCheck(currentPlayer_->color)) {
            // Undo: this move would leave (or keep) our own king in check.
            board_.setPiece(source, piece);
            board_.setPiece(destination, captured);
            throw IllegalMoveError("Move leaves own king in check");
        }

        if (piece->type() == PieceType::PAWN && (destination.first == 0 || destination.first == Board::SIZE - 1)) {
            piece = std::make_shared<Queen>(piece->color());
            board_.setPiece(destination, piece);
        }

        Move performedMove{source, destination, piece, captured};
        history_.push_back(performedMove);

        switchTurn();
        Color opponentColor = currentPlayer_->color;
        if (isCheckmate(opponentColor)) {
            status_ = GameStatus::CHECKMATE;
        } else if (isCheck(opponentColor)) {
            status_ = GameStatus::CHECK;
        } else if (isStalemate(opponentColor)) {
            status_ = GameStatus::STALEMATE;
        } else {
            status_ = GameStatus::ONGOING;
        }

        return performedMove;
    }

    const Player& currentPlayer() const { return *currentPlayer_; }
    GameStatus status() const { return status_; }
    Board& board() { return board_; }

    static std::string statusName(GameStatus status) {
        switch (status) {
            case GameStatus::ONGOING: return "ONGOING";
            case GameStatus::CHECK: return "CHECK";
            case GameStatus::CHECKMATE: return "CHECKMATE";
            case GameStatus::STALEMATE: return "STALEMATE";
        }
        return "UNKNOWN";
    }

private:
    bool hasLegalMove(Color color) {
        for (auto& [source, piece] : board_.allPieces(color)) {
            for (auto destination : piece->getPossibleMoves(source, board_)) {
                auto captured = board_.movePiece(source, destination);
                bool stillInCheck = isCheck(color);
                board_.setPiece(source, piece);
                board_.setPiece(destination, captured);
                if (!stillInCheck) return true;
            }
        }
        return false;
    }

    Board board_;
    Player white_;
    Player black_;
    Player* currentPlayer_;
    GameStatus status_;
    std::vector<Move> history_;
};

int main() {
    Game game("Alice", "Bob");
    game.start();

    // Fool's Mate -- fastest possible checkmate, four half-moves.
    std::vector<std::string> moves = {"f2f3", "e7e5", "g2g4", "d8h4"};
    for (const auto& moveStr : moves) {
        Position source = parseSquare(moveStr.substr(0, 2));
        Position destination = parseSquare(moveStr.substr(2, 2));
        std::string mover = game.currentPlayer().name;
        game.move(source, destination);
        std::cout << mover << " played " << moveStr << " -> status: " << Game::statusName(game.status()) << "\n";
    }

    std::cout << "\n";
    game.board().printBoard();
    std::cout << "\nFinal status: " << Game::statusName(game.status()) << "\n";

    return 0;
}
