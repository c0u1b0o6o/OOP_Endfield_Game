#include "LevelParser.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iostream>
#include <filesystem>

namespace ark {

    // 讀取關卡檔案並解析為 Board 與 Part 清單
    LevelData loadLevel(const std::string& filepath) {
        std::ifstream fin(filepath);
        if (!fin.is_open())
            throw std::runtime_error("Cannot open level file: " + filepath);

        int C, M, N;
        fin >> C >> M >> N; // C: 顏色數, M: 列數, N: 欄數

        Board board(M, N, C);

        // 依顏色讀取各行列的目標數量與固定格
        for (int color = 0; color < C; ++color) {
            // 每一行的目標值
            for (int r = 0; r < M; ++r) {
                int val; fin >> val;
                board.setTargetRowCount(color, r, val);
            }
            // 每一列的目標值
            for (int c = 0; c < N; ++c) {
                int val; fin >> val;
                board.setTargetColCount(color, c, val);
            }
            // 該顏色的固定格數量與位置
            int x1; fin >> x1;
            for (int i = 0; i < x1; ++i) {
                int r, c; fin >> r >> c;
                board.setFixedCell(r, c, color);
            }
        }

        // 讀取阻擋格數量與位置
        int x2; fin >> x2;
        for (int i = 0; i < x2; ++i) {
            int r, c; fin >> r >> c;
            board.setBlockedCell(r, c);
        }

        // 讀取零件列表直到 EOF
        std::vector<Part> parts;
        int colorIdx, m2, n2;
        int partId = 0;
        while (fin >> colorIdx >> m2 >> n2) {
            Shape shape(m2, std::vector<uint8_t>(n2, 0));
            for (int r = 0; r < m2; ++r) {
                for (int c = 0; c < n2; ++c) {
                    int val;
                    if (fin >> val) shape[r][c] = val ? 1 : 0;
                }
            }
            parts.emplace_back(partId++, colorIdx, shape);
        }

        return LevelData{ std::move(board), std::move(parts) };
    }

    // 匯出關卡到檔案（會建立必要的目錄）
    void exportLevel(const std::string& filepath, const Board& board,
                     const std::vector<Part>& parts) {
        std::filesystem::path path(filepath);
        if (path.has_parent_path()) {
            std::filesystem::create_directories(path.parent_path());
        }
        std::ofstream fout(filepath);
        if (!fout.is_open())
            throw std::runtime_error("Cannot open file for export: " + filepath);

        const int C = board.colorCount();
        const int M = board.rows();
        const int N = board.cols();
        fout << C << " " << M << " " << N << "\n\n";

        // 收集並輸出每種顏色的目標行列與固定格
        std::vector<std::vector<std::pair<int,int>>> fixedCells(C);
        for (int r = 0; r < M; ++r)
            for (int c = 0; c < N; ++c)
                if (board.cellType(r, c) == cell::FIXED)
                    fixedCells[board.cellColor(r, c)].push_back({r, c});

        for (int color = 0; color < C; ++color) {
            for (int r = 0; r < M; ++r)
                fout << (r ? " " : "") << board.targetRow(color, r);
            fout << "\n";
            for (int c = 0; c < N; ++c)
                fout << (c ? " " : "") << board.targetCol(color, c);
            fout << "\n";
            fout << fixedCells[color].size() << "\n";
            for (auto& [r, c] : fixedCells[color])
                fout << r << " " << c << "\n";
            fout << "\n";
        }

        // 輸出阻擋格
        std::vector<std::pair<int,int>> blocked;
        for (int r = 0; r < M; ++r)
            for (int c = 0; c < N; ++c)
                if (board.cellType(r, c) == cell::BLOCK)
                    blocked.push_back({r, c});
        fout << blocked.size() << "\n\n";
        for (auto& [r, c] : blocked)
            fout << r << " " << c << "\n";

        // 輸出零件資料
        for (auto& part : parts) {
            fout << part.colorIndex() << " " << part.height() << " " << part.width() << "\n";
            for (int r = 0; r < part.height(); ++r) {
                for (int c = 0; c < part.width(); ++c)
                    fout << (int)part.shape()[r][c] << (c == part.width() - 1 ? "" : " ");
                fout << "\n";
            }
            fout << "\n";
        }
    }

} // namespace ark

