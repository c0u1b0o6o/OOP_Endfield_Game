#pragma once
// =============================================================================
//  Board.h  -  盤面狀態與規則驗證
//  cellType / cellColor 雙陣列設計
// =============================================================================
#include "Part.h"
#include <vector>
#include <string>

namespace ark {

    namespace cell {
        constexpr int EMPTY = -1;
        constexpr int BLOCK = -2;   // 不可放置格 (X)
        constexpr int FIXED = -3;   // 固定零件格 (=)
    }

    class Board {
    public:
        Board() = default;
        Board(int rows, int cols, int colorCount);

        int rows()       const { return rows_; }
        int cols()       const { return cols_; }
        int colorCount() const { return colorCount_; }

        // ---- 盤面初始化 ----
        void setEmptyCell(int r, int c);
        void setBlockedCell(int r, int c);
        void setFixedCell(int r, int c, int colorIndex);
        void setTargetRowCount(int colorIndex, int row, int count);
        void setTargetColCount(int colorIndex, int col, int count);

        // ---- 查詢 ----
        int  cellType(int r, int c)  const { return cellType_[r][c]; }
        int  cellColor(int r, int c) const { return cellColor_[r][c]; }
        int  targetRow(int color, int row) const { return targetRow_[color][row]; }
        int  targetCol(int color, int col) const { return targetCol_[color][col]; }
        int  currentRow(int color, int row) const { return curRow_[color][row]; }
        int  currentCol(int color, int col) const { return curCol_[color][col]; }

        bool inBounds(int r, int c) const {
            return r >= 0 && r < rows_ && c >= 0 && c < cols_;
        }

        // ---- 放置 / 移除 ----
        bool canPlace(const Part& part, int anchorRow, int anchorCol,
                      std::string* errorMsg = nullptr) const;
        bool placePart(const Part& part, int anchorRow, int anchorCol);
        void removePart(const Part& part);

        // ---- 勝利判定 ----
        bool checkWinCondition(int totalPartCount, int placedPartCount) const;

        // ---- 重置 ----
        void resetPlayerParts();

        // ---- Console 輸出 ----
        void printSolution() const;

    private:
        int rows_ = 0;
        int cols_ = 0;
        int colorCount_ = 0;
        std::vector<std::vector<int>> cellType_;
        std::vector<std::vector<int>> cellColor_;
        std::vector<std::vector<int>> targetRow_;
        std::vector<std::vector<int>> targetCol_;
        std::vector<std::vector<int>> curRow_;
        std::vector<std::vector<int>> curCol_;
    };

} // namespace ark

