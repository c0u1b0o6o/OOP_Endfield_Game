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
    // Command-line 自動解題模式：使用 --solve <path>
    // 設計考量：提供簡單的 CLI 介面方便在沒有 GUI 或需要快速測試時直接求解並輸出文字結果。
    // 注意：程式會使用 ark::loadLevel 讀取檔案，並把 Board 與 Part 傳給 AutoSolver 的 solve，
    //       solve 以值傳入 (copy) Board 與 parts，這樣不會改變原始資料，方便在 GUI 與 CLI 兩種模式共用讀檔邏輯。
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
            // 注意：solve 接收 board/parts 的拷貝（值傳），因此呼叫後原始 data 不會被修改。
            if (solver.solve(data.board, data.parts, solution)) {
                std::cout << "Solution found!\n";
                ark::Board result = data.board; // 建立結果 board，將解套用方便列印
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

    // GUI 模式：建立遊戲並執行
    // 設計考量：將 CLI 與 GUI 分流處理，若提供參數就當作開啟指定關卡的指令
    ark::Game game;
    if (argc >= 2) {
        game.setStartLevel(argv[1]);
    }
    game.run();
    return 0;
}

