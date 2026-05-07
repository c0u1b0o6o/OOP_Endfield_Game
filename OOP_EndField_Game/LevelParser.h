#pragma once
// =============================================================================
//  LevelParser.h  -  關卡載入器
//  解析 .txt 設定檔，產出 Board + vector<Part>
// =============================================================================
#include "Board.h"
#include "Part.h"
#include <string>
#include <vector>

namespace ark {

    struct LevelData {
        Board board;
        std::vector<Part> parts;
    };

    // 從檔案路徑載入關卡
    LevelData loadLevel(const std::string& filepath);

    // 將 LevelData 匯出為 .txt (給關卡設計器用)
    void exportLevel(const std::string& filepath, const Board& board,
                     const std::vector<Part>& parts);

} // namespace ark

