#include "AutoSolver.h"
#include <algorithm>
#include <iostream>

namespace ark {

    bool AutoSolver::pruneCheck(const Board& board) const {
        for (int color = 0; color < board.colorCount(); ++color) {
            for (int r = 0; r < board.rows(); ++r)
                if (board.currentRow(color, r) > board.targetRow(color, r))
                    return false;
            for (int c = 0; c < board.cols(); ++c)
                if (board.currentCol(color, c) > board.targetCol(color, c))
                    return false;
        }
        return true;
    }

    bool AutoSolver::dfs(Board& board, std::vector<Part>& parts,
                         std::vector<bool>& used, int placedCount,
                         std::vector<SolverPlacement>& solution) {
        if (placedCount == (int)parts.size()) {
            return board.checkWinCondition((int)parts.size(), placedCount);
        }

        // 找下一個未放置的零件
        int idx = -1;
        for (int i = 0; i < (int)parts.size(); ++i)
            if (!used[i]) { idx = i; break; }
        if (idx < 0) return false;

        used[idx] = true;

        // 備份原始 shape 以便恢復
        Part original = parts[idx];

        // 嘗試 4 種旋轉
        for (int rot = 0; rot < 4; ++rot) {
            if (rot > 0) parts[idx].rotateRight();

            // 去重：如果旋轉後的形狀與之前的某個旋轉相同，跳過
            // (簡單實作：不做去重，DFS 足夠快)

            const Part& p = parts[idx];
            for (int r = -p.pivotRow(); r <= board.rows() - 1; ++r) {
                for (int c = -p.pivotCol(); c <= board.cols() - 1; ++c) {
                    if (board.canPlace(p, r, c)) {
                        board.placePart(p, r, c);

                        if (pruneCheck(board)) {
                            solution.push_back({p.id(), r, c, rot});
                            if (dfs(board, parts, used, placedCount + 1, solution))
                                return true;
                            solution.pop_back();
                        }

                        board.removePart(p);
                    }
                }
            }
        }

        // 恢復原始形狀
        parts[idx] = original;
        used[idx] = false;
        return false;
    }

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

    bool AutoSolver::solve(Board board, std::vector<Part> parts,
                           std::vector<SolverPlacement>& solution) {
        solution.clear();
        std::vector<bool> used(parts.size(), false);
        return dfs(board, parts, used, 0, solution);
    }

    std::vector<std::vector<SolverPlacement>> AutoSolver::solveAll(Board board, std::vector<Part> parts) {
        std::vector<std::vector<SolverPlacement>> allSolutions;
        std::vector<SolverPlacement> currentSolution;
        std::vector<bool> used(parts.size(), false);
        dfsAll(board, parts, used, 0, currentSolution, allSolutions);
        return allSolutions;
    }

} // namespace ark

