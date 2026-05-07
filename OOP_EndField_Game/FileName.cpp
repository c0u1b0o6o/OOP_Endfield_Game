// =============================================================================
//  FileName.cpp  -  主入口
//  致命規定：不可 hardcode 路徑，支援 Runtime 動態讀檔
//  用法：TEST.exe              → GUI 主畫面
//        TEST.exe <level.txt>  → 直接載入該關卡進入遊戲
//        TEST.exe --solve <level.txt> → Console 自動解題
// =============================================================================
#include "Game.h"
#include "LevelParser.h"
#include "AutoSolver.h"
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    // Console 自動解題模式
    if (argc >= 3 && std::string(argv[1]) == "--solve") {
        std::string path = argv[2];
        try {
            auto data = ark::loadLevel(path);
            std::cout << "Loaded: " << path << "\n";
            std::cout << "Board: " << data.board.rows() << "x" << data.board.cols()
                      << "  Colors: " << data.board.colorCount()
                      << "  Parts: " << data.parts.size() << "\n";

            ark::AutoSolver solver;
            std::vector<ark::SolverPlacement> solution;
            if (solver.solve(data.board, data.parts, solution)) {
                std::cout << "Solution found!\n";
                ark::Board result = data.board;
                for (auto& sp : solution) {
                    ark::Part p = data.parts[sp.partId].rotated(sp.rotation);
                    result.placePart(p, sp.anchorRow, sp.anchorCol);
                }
                result.printSolution();
            } else {
                std::cout << "No solution found.\n";
            }
        } catch (std::exception& e) {
            std::cerr << "Error: " << e.what() << "\n";
            return 1;
        }
        return 0;
    }

    // GUI 模式
    ark::Game game;
    if (argc >= 2) {
        game.setStartLevel(argv[1]);
    }
    game.run();
    return 0;
}

