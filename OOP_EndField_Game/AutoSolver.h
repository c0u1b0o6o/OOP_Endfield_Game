#pragma once
// =============================================================================
//  AutoSolver.h  -  DFS + Backtracking 自動解題機
// =============================================================================
#include "Board.h"
#include "Part.h"
#include <vector>

namespace ark {

    struct SolverPlacement {
        int partId;
        int anchorRow;
        int anchorCol;
        int rotation;  // 0~3
    };

    class AutoSolver {
    public:
        // 求解：回傳 true 若找到解，solution 會被填入
        bool solve(Board board, std::vector<Part> parts,
                   std::vector<SolverPlacement>& solution);

        // 求解：回傳所有的可行解
        std::vector<std::vector<SolverPlacement>> solveAll(Board board, std::vector<Part> parts);

    private:
        bool dfs(Board& board, std::vector<Part>& parts,
                 std::vector<bool>& used, int placedCount,
                 std::vector<SolverPlacement>& solution);

        void dfsAll(Board& board, std::vector<Part>& parts,
                 std::vector<bool>& used, int placedCount,
                 std::vector<SolverPlacement>& currentSolution,
                 std::vector<std::vector<SolverPlacement>>& allSolutions);

        // 剪枝：檢查是否有任何列/欄已超出目標
        bool pruneCheck(const Board& board) const;
    };

} // namespace ark

