#include "LevelParser.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iostream>

namespace ark {

    LevelData loadLevel(const std::string& filepath) {
        std::ifstream fin(filepath);
        if (!fin.is_open())
            throw std::runtime_error("Cannot open level file: " + filepath);

        int C, M, N;
        fin >> C >> M >> N;

        Board board(M, N, C);

        // 讀取每種顏色的目標與固定格
        for (int color = 0; color < C; ++color) {
            // M 個列目標
            for (int r = 0; r < M; ++r) {
                int val; fin >> val;
                board.setTargetRowCount(color, r, val);
            }
            // N 個欄目標
            for (int c = 0; c < N; ++c) {
                int val; fin >> val;
                board.setTargetColCount(color, c, val);
            }
            // 固定格數量
            int x1; fin >> x1;
            for (int i = 0; i < x1; ++i) {
                int r, c; fin >> r >> c;
                board.setFixedCell(r, c, color);
            }
        }

        // 不可放置格
        int x2; fin >> x2;
        for (int i = 0; i < x2; ++i) {
            int r, c; fin >> r >> c;
            board.setBlockedCell(r, c);
        }

        // 零件列表直到 EOF
        std::vector<Part> parts;
        int colorIdx, m2, n2;
        int partId = 0;
        while (fin >> colorIdx >> m2 >> n2) {
            Shape shape(m2, std::vector<uint8_t>(n2, 0));
            for (int r = 0; r < m2; ++r) {
                std::string line;
                fin >> line;
                for (int c = 0; c < n2 && c < (int)line.size(); ++c)
                    shape[r][c] = (line[c] == '1') ? 1 : 0;
            }
            parts.emplace_back(partId++, colorIdx, shape);
        }

        return LevelData{ std::move(board), std::move(parts) };
    }

    void exportLevel(const std::string& filepath, const Board& board,
                     const std::vector<Part>& parts) {
        std::ofstream fout(filepath);
        if (!fout.is_open())
            throw std::runtime_error("Cannot open file for export: " + filepath);

        const int C = board.colorCount();
        const int M = board.rows();
        const int N = board.cols();
        fout << C << " " << M << " " << N << "\n";

        // 收集固定格
        // fixedCells[color] = vector<(r,c)>
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
        }

        // 收集不可放置格
        std::vector<std::pair<int,int>> blocked;
        for (int r = 0; r < M; ++r)
            for (int c = 0; c < N; ++c)
                if (board.cellType(r, c) == cell::BLOCK)
                    blocked.push_back({r, c});
        fout << blocked.size() << "\n";
        for (auto& [r, c] : blocked)
            fout << r << " " << c << "\n";

        // 零件
        for (auto& part : parts) {
            fout << part.colorIndex() << " " << part.height() << " " << part.width() << "\n";
            for (int r = 0; r < part.height(); ++r) {
                for (int c = 0; c < part.width(); ++c)
                    fout << (int)part.shape()[r][c];
                fout << "\n";
            }
        }
    }

} // namespace ark

