#include "Board.h"
#include <stdexcept>
#include <iostream>
#include <iomanip>

namespace ark {

    Board::Board(int rows, int cols, int colorCount)
        : rows_(rows), cols_(cols), colorCount_(colorCount),
          cellType_(rows, std::vector<int>(cols, cell::EMPTY)),
          cellColor_(rows, std::vector<int>(cols, -1)),
          targetRow_(colorCount, std::vector<int>(rows, 0)),
          targetCol_(colorCount, std::vector<int>(cols, 0)),
          curRow_(colorCount, std::vector<int>(rows, 0)),
          curCol_(colorCount, std::vector<int>(cols, 0))
    {}

    void Board::setBlockedCell(int r, int c) {
        cellType_[r][c] = cell::BLOCK;
        cellColor_[r][c] = -1;
    }

    void Board::setFixedCell(int r, int c, int colorIndex) {
        cellType_[r][c] = cell::FIXED;
        cellColor_[r][c] = colorIndex;
        curRow_[colorIndex][r]++;
        curCol_[colorIndex][c]++;
    }

    void Board::setTargetRowCount(int color, int row, int count) {
        targetRow_[color][row] = count;
    }
    void Board::setTargetColCount(int color, int col, int count) {
        targetCol_[color][col] = count;
    }

    bool Board::canPlace(const Part& part, int ar, int ac, std::string* err) const {
        const auto& s = part.shape();
        for (int r = 0; r < part.height(); ++r) {
            for (int c = 0; c < part.width(); ++c) {
                if (!s[r][c]) continue;
                const int br = ar + r, bc = ac + c;
                if (!inBounds(br, bc)) {
                    if (err) *err = "Out of bounds";
                    return false;
                }
                const int t = cellType_[br][bc];
                if (t == cell::BLOCK) {
                    if (err) *err = "Blocked cell";
                    return false;
                }
                if (t == cell::FIXED) {
                    if (err) *err = "Fixed cell";
                    return false;
                }
                if (t >= 0) {
                    if (err) *err = "Overlap";
                    return false;
                }
            }
        }
        return true;
    }

    bool Board::placePart(const Part& part, int ar, int ac) {
        if (!canPlace(part, ar, ac)) return false;
        const auto& s = part.shape();
        const int color = part.colorIndex();
        for (int r = 0; r < part.height(); ++r)
            for (int c = 0; c < part.width(); ++c)
                if (s[r][c]) {
                    const int br = ar + r, bc = ac + c;
                    cellType_[br][bc]  = part.id();
                    cellColor_[br][bc] = color;
                    curRow_[color][br]++;
                    curCol_[color][bc]++;
                }
        return true;
    }

    void Board::removePart(const Part& part) {
        const int id = part.id();
        const int color = part.colorIndex();
        for (int r = 0; r < rows_; ++r)
            for (int c = 0; c < cols_; ++c)
                if (cellType_[r][c] == id) {
                    cellType_[r][c]  = cell::EMPTY;
                    cellColor_[r][c] = -1;
                    curRow_[color][r]--;
                    curCol_[color][c]--;
                }
    }

    bool Board::checkWinCondition(int totalPartCount, int placedPartCount) const {
        if (placedPartCount != totalPartCount) return false;
        for (int color = 0; color < colorCount_; ++color) {
            for (int r = 0; r < rows_; ++r)
                if (curRow_[color][r] != targetRow_[color][r]) return false;
            for (int c = 0; c < cols_; ++c)
                if (curCol_[color][c] != targetCol_[color][c]) return false;
        }
        return true;
    }

    void Board::resetPlayerParts() {
        for (int r = 0; r < rows_; ++r)
            for (int c = 0; c < cols_; ++c)
                if (cellType_[r][c] >= 0) {
                    const int color = cellColor_[r][c];
                    if (color >= 0) {
                        curRow_[color][r]--;
                        curCol_[color][c]--;
                    }
                    cellType_[r][c]  = cell::EMPTY;
                    cellColor_[r][c] = -1;
                }
    }

    void Board::printSolution() const {
        for (int r = 0; r < rows_; ++r) {
            for (int c = 0; c < cols_; ++c) {
                if (c > 0) std::cout << ' ';
                int t = cellType_[r][c];
                if (t == cell::FIXED)      std::cout << '=';
                else if (t == cell::BLOCK) std::cout << 'X';
                else if (t == cell::EMPTY) std::cout << '_';
                else                       std::cout << t;
            }
            std::cout << '\n';
        }
    }

} // namespace ark

