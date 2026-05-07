#pragma once
// =============================================================================
//  Part.h  -  零件 (Puzzle Piece) 資料結構
//  - 每個零件有：id (依輸入順序)、color、寬高、shape (0/1 陣列)
//  - 支援 90° 順時針旋轉，並維護「Pivot」(第一個值為 1 的實體格子)
//  - 旋轉時保證 Pivot 對應的「世界格子」保持不動
// =============================================================================
#include <vector>
#include <cstdint>

namespace ark {

    using Shape = std::vector<std::vector<uint8_t>>; // 0/1 二維陣列

    class Part {
    public:
        Part(int id, int colorIndex, const Shape& shape);

        // ---- 唯讀屬性 ----
        int id()         const { return id_; }
        int colorIndex() const { return colorIndex_; }
        int width()      const { return width_; }
        int height()     const { return height_; }
        const Shape& shape() const { return shape_; }

        // 第一個值為 1 的格子 (row, col) — 作為旋轉/拖曳中心
        int pivotRow() const { return pivotRow_; }
        int pivotCol() const { return pivotCol_; }

        // 該零件實際佔的格子數 (用於剪枝 / 統計)
        int cellCount() const { return cellCount_; }

        // ---- 操作 ----
        struct RotateDelta { int dRow; int dCol; };
        RotateDelta rotateRight();

        // 取得旋轉 N 次後的副本 (不改動自身)
        Part rotated(int times) const;

    private:
        int id_;
        int colorIndex_;
        int width_;
        int height_;
        Shape shape_;
        int pivotRow_ = 0;
        int pivotCol_ = 0;
        int cellCount_ = 0;

        void recomputePivotAndCount();
    };

} // namespace ark

