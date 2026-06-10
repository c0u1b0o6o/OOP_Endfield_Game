#include "AutoSolver.h"
#include <algorithm>
#include <iostream>
#include <set>
#include <utility>

namespace ark {

    // 剪枝檢查（pruneCheck）
    // 詳細說明：
    // 這個檢查用來在 DFS 搜尋過程中早期排除不可能的分支。
    // 每個顏色都有目標的每行/列數量 (targetRow/targetCol)，以及目前已經被放置（或固定）的數量 (currentRow/currentCol)。
    // 若某一顏色在任一行或列上的當前數量已經超過該目標值，則無論之後如何放置其他零件都不可能回復到合法目標，
    // 因此可以立刻剪掉此分支以節省搜尋時間。這是一種常見的約束回溯 pruning 策略，用於降低狀態空間。
    bool AutoSolver::pruneCheck(const Board& board) const {
        for (int color = 0; color < board.colorCount(); ++color) {
            // 檢查每一行是否超出目標
            for (int r = 0; r < board.rows(); ++r)
                if (board.currentRow(color, r) > board.targetRow(color, r))
                    return false;
            // 檢查每一列是否超出目標
            for (int c = 0; c < board.cols(); ++c)
                if (board.currentCol(color, c) > board.targetCol(color, c))
                    return false;
        }
        return true; // 所有顏色、行、列皆未超標，通過剪枝
    }

    // 深度優先搜尋 (DFS) 找到一個解
    // 詳細說明：
    // - 使用回溯法：對於尚未放置的零件，嘗試所有旋轉與所有可能錨點位置。
    // - 每次放置後先呼叫 pruneCheck() 進行簡單的約束檢查，若通過才繼續遞迴。
    // - 這裡選擇直覺的變數順序（找到第一個未使用的零件 idx）而非更複雜的啟發式（例如最少可用位置優先），
    //   原因是程式碼簡潔且對中小尺寸關卡通常效能足夠；若要提高效率可以改用啟發式選擇 idx。
    // - 旋轉去重的註解說明：理論上可以預先對零件的不同旋轉形狀做唯一化來避免重複嘗試，但目前實作直接在迴圈中嘗試 4 次旋轉即可，
    //   這樣程式碼簡單且不改變結果。若遇到效能瓶頸再引入旋轉去重。
    bool AutoSolver::dfs(Board& board, std::vector<Part>& parts,
                         std::vector<bool>& used, int placedCount,
                         std::vector<SolverPlacement>& solution) {
        // 若已放置所有零件，檢查是否滿足勝利條件
        if (placedCount == (int)parts.size()) {
            return board.checkWinCondition((int)parts.size(), placedCount);
        }

        // 選出下一個未使用的零件索引
        // 設計考量：直接取第一個未使用的零件，簡單且確保每次會減少問題規模。
        int idx = -1;
        for (int i = 0; i < (int)parts.size(); ++i)
            if (!used[i]) { idx = i; break; }
        if (idx < 0) return false;

        used[idx] = true;

        // 備份原始零件狀態，回溯時還原
        Part original = parts[idx];

        // 嘗試 4 種旋轉：0, 90, 180, 270
        // 理由：方塊是矩形格子表示，四向旋轉覆蓋所有情形；若零件具有鏡射也需額外考慮（本專案未使用鏡射）
        for (int rot = 0; rot < 4; ++rot) {
            if (rot > 0) parts[idx].rotateRight();

            // 以零件的 pivot 為基準，計算放置錨點範圍。允許負值是為了處理 pivot 位於形狀內部的情況。
            const Part& p = parts[idx];
            for (int r = -p.pivotRow(); r <= board.rows() - 1; ++r) {
                for (int c = -p.pivotCol(); c <= board.cols() - 1; ++c) {
                    if (board.canPlace(p, r, c)) {
                        // 實際放置零件
                        board.placePart(p, r, c);

                        // 重要：先做快速剪枝，避免深入無效分支
                        if (pruneCheck(board)) {
                            solution.push_back({p.id(), r, c, rot});
                            if (dfs(board, parts, used, placedCount + 1, solution))
                                return true; // 若找到解則立刻回傳，結束整個搜尋
                            solution.pop_back(); // 回溯 solution 記錄
                        }

                        // 回溯：移除剛放的零件，恢復 board 狀態
                        board.removePart(p);
                    }
                }
            }
        }

        // 旋轉回原本狀態並釋放 used 標記
        parts[idx] = original;
        used[idx] = false;
        return false;
    }

    // 深度優先搜尋（收集所有解）
    // 詳細說明：與 dfs 類似，但不在找到第一個解時停止，而是把所有滿足條件的排列加入 allSolutions。
    // 注意：為避免記憶體暴增，對大型狀態空間請慎用；本函數後續有額外的等價解去重處理。
    void AutoSolver::dfsAll(Board& board, std::vector<Part>& parts,
                         std::vector<bool>& used, int placedCount,
                         std::vector<SolverPlacement>& currentSolution,
                         std::vector<std::vector<SolverPlacement>>& allSolutions) {
        if (placedCount == (int)parts.size()) {
            if (board.checkWinCondition((int)parts.size(), placedCount)) {
                allSolutions.push_back(currentSolution);
            }
            return;
        }

        int idx = -1;
        for (int i = 0; i < (int)parts.size(); ++i)
            if (!used[i]) { idx = i; break; }
        if (idx < 0) return;

        used[idx] = true;
        Part original = parts[idx];

        for (int rot = 0; rot < 4; ++rot) {
            if (rot > 0) parts[idx].rotateRight();

            const Part& p = parts[idx];
            for (int r = -p.pivotRow(); r <= board.rows() - 1; ++r) {
                for (int c = -p.pivotCol(); c <= board.cols() - 1; ++c) {
                    if (board.canPlace(p, r, c)) {
                        board.placePart(p, r, c);

                        if (pruneCheck(board)) {
                            currentSolution.push_back({p.id(), r, c, rot});
                            dfsAll(board, parts, used, placedCount + 1, currentSolution, allSolutions);
                            currentSolution.pop_back();
                        }

                        board.removePart(p);
                    }
                }
            }
        }

        parts[idx] = original;
        used[idx] = false;
    }

    // 外部介面：尋找單一解
    // 備註：Board 與 parts 都以值傳入（copy），能保證呼叫者的物件不會被修改。
    bool AutoSolver::solve(Board board, std::vector<Part> parts,
                           std::vector<SolverPlacement>& solution) {
        solution.clear();
        std::vector<bool> used(parts.size(), false);
        return dfs(board, parts, used, 0, solution);
    }

    // 尋找所有解並移除等價解（簽名去重）
    // 詳細說明：
    // 1. 先用 dfsAll 收集所有原始解 (rawSolutions)。這些解之間可能因為零件順序或旋轉等導致等價，但在 placements 上看起來不同。
    // 2. 對每個解，計算一個不依賴零件放置順序的簽名：對每個零件收集其最終佔據的格子座標集合以及顏色，
    //    並將 (partId, color, sorted cells) 作為唯一描述，整個解簽名為這些描述的集合。
    // 3. 使用 set 去重，這樣可以把不同放置順序或等價旋轉導致的重複解合併，只保留一個代表解。
    std::vector<std::vector<SolverPlacement>> AutoSolver::solveAll(Board board, std::vector<Part> parts) {
        std::vector<std::vector<SolverPlacement>> rawSolutions;
        std::vector<SolverPlacement> currentSolution;
        std::vector<bool> used(parts.size(), false);
        dfsAll(board, parts, used, 0, currentSolution, rawSolutions);

        std::vector<std::vector<SolverPlacement>> allSolutions;
        // 簽名結構：set of (partId, (color, vector<cells>))
        std::set<std::set<std::pair<int, std::pair<int, std::vector<std::pair<int, int>>>>>> seenSignatures;

        for (auto& sol : rawSolutions) {
            std::set<std::pair<int, std::pair<int, std::vector<std::pair<int, int>>>>> solSignature;
            for (auto& sp : sol) {
                // 根據儲存的 part id 與 rotation 來取得該零件的最終形狀
                Part p = parts[sp.partId].rotated(sp.rotation);
                std::vector<std::pair<int, int>> cells;
                // 計算該零件佔據的所有格子座標，這樣簽名與放置順序無關
                for (int pr = 0; pr < p.height(); ++pr) {
                    for (int pc = 0; pc < p.width(); ++pc) {
                        if (p.shape()[pr][pc]) {
                            cells.push_back({sp.anchorRow + pr, sp.anchorCol + pc});
                        }
                    }
                }
                std::sort(cells.begin(), cells.end());
                solSignature.insert({sp.partId, {p.colorIndex(), cells}});
            }

            if (seenSignatures.find(solSignature) == seenSignatures.end()) {
                seenSignatures.insert(solSignature);
                allSolutions.push_back(sol);
            }
        }
        return allSolutions;
    }

} // namespace ark

