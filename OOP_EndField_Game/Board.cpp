#include "Board.h"
#include <stdexcept>
#include <iostream>
#include <iomanip>

namespace ark {

    // 建構子：初始化棋盤內部資料結構
    // 詳細說明：
    // - 使用二維陣列存放每格的類型與顏色。
    // - 為了快速檢查每行/列的顏色數量，維護 curRow_ / curCol_ 兩個計數矩陣，
    //   這樣在放置或移除零件時可以 O(1) 更新相關行列的計數，而不是每次掃描整列或整行。
    Board::Board(int rows, int cols, int colorCount)
        : rows_(rows), cols_(cols), colorCount_(colorCount),
          cellType_(rows, std::vector<int>(cols, cell::EMPTY)),
          cellColor_(rows, std::vector<int>(cols, -1)),
          targetRow_(colorCount, std::vector<int>(rows, 0)),
          targetCol_(colorCount, std::vector<int>(cols, 0)),
          curRow_(colorCount, std::vector<int>(rows, 0)),
          curCol_(colorCount, std::vector<int>(cols, 0))
    {}

    // 設為空格：如果原本是固定格（FIXED），需調整 curRow_ / curCol_ 的計數
    // 詳細說明：固定格代表初始化時已存在的色塊，當將其變為空格時，必須同步調整當前記錄。
    void Board::setEmptyCell(int r, int c) {
        if (cellType_[r][c] == cell::FIXED) {
            int color = cellColor_[r][c];
            if (color >= 0) {
                curRow_[color][r]--;
                curCol_[color][c]--;
            }
        }
        cellType_[r][c] = cell::EMPTY;
        cellColor_[r][c] = -1;
    }

    // 設為阻擋格（無法放置）
    // 詳細說明：阻擋格直接標記為 BLOCK，顏色欄位設為 -1
    void Board::setBlockedCell(int r, int c) {
        cellType_[r][c] = cell::BLOCK;
        cellColor_[r][c] = -1;
    }

    // 設為固定顏色格：同時更新當前行/列的顏色計數
    // 詳細說明：固定格通常來自關卡檔案。在初始化階段使用此函式可同時建立 curRow_/curCol_ 的初始值，
    // 因為這些固定格也會影響後續的剪枝檢查與勝利判斷。
    void Board::setFixedCell(int r, int c, int colorIndex) {
        cellType_[r][c] = cell::FIXED;
        cellColor_[r][c] = colorIndex;
        curRow_[colorIndex][r]++;
        curCol_[colorIndex][c]++;
    }

    // 設定目標行數與列數
    // 詳細說明：targetRow_ / targetCol_ 是關卡目標，用於勝利條件和 pruneCheck 裡的判斷。
    void Board::setTargetRowCount(int color, int row, int count) {
        targetRow_[color][row] = count;
    }
    void Board::setTargetColCount(int color, int col, int count) {
        targetCol_[color][col] = count;
    }

    // 檢查是否能在 (ar,ac) 放置 part（不修改棋盤）
    // 詳細說明：此函式只做局部合法性檢查：
    // - 檢查 shape 的每個實格是否在棋盤範圍內
    // - 檢查該格是否為阻擋或固定格
    // - 檢查是否與已放置的玩家零件重疊（cellType_ >= 0 表示某零件 id）
    // 備註：這裡不檢查放置後的行/列計數是否超出目標，該檢查由呼叫端 (pruneCheck) 處理以便早期剪枝
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

    // 實際放置零件：將格子標上零件 id 與顏色，並更新 curRow_ / curCol_
    // 設計理由：放置時同步更新計數能讓 pruneCheck 與勝利檢查在 O(1) 時間內獲取行/列的顏色數量
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

    // 移除零件：遍歷整個棋盤移除與該 id 相同的格子，並更新計數
    // 設計理由：此實作選擇簡潔的做法（遍歷全盤）來移除指定 id 的格子，雖然在大型棋盤上不是最有效率，
    // 但程式簡單且對目標規模通常可接受。如果需要優化，可在放置時記錄該零件佔據的格子，便能 O(k) 移除，其中 k 為方塊格數。
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

    // 檢查勝利條件：所有零件已放且每行/列的顏色計數等於目標
    // 詳細說明：先確認放置的零件數量與總零件數相等，再比較每個顏色的行/列計數是否與 target 相符。
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

    // 重置玩家放置的方塊（不影響固定格）
    // 詳細說明：此函式會把所有 cellType_ >= 0 的格子視為玩家放置的方塊並清除它們，同時更新 curRow_/curCol_。
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

    // 將棋盤輸出為文字（用於除錯或 Console 模式顯示）
    // 輸出符號說明：'=' 固定格, 'X' 阻擋格, '_' 空格, 其它數字表示放置的零件 id
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

