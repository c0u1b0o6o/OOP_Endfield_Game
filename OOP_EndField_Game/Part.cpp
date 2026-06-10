#include "Part.h"
#include <stdexcept>

namespace ark {

	// 建構子：初始化 id、顏色、shape，並計算尺寸與 pivot
	Part::Part(int id, int colorIndex, const Shape& shape)
		: id_(id), colorIndex_(colorIndex), shape_(shape)
	{
		if (shape_.empty() || shape_[0].empty())
			throw std::invalid_argument("Part::Part: empty shape");
		height_ = static_cast<int>(shape_.size());
		width_ = static_cast<int>(shape_[0].size());
		recomputePivotAndCount();
	}

	// 重新計算 pivot（第一個非空格位置）與方格數量
	void Part::recomputePivotAndCount() {
		pivotRow_ = -1; pivotCol_ = -1; cellCount_ = 0;
		for (int r = 0; r < height_; ++r)
			for (int c = 0; c < width_; ++c)
				if (shape_[r][c]) {
					if (pivotRow_ < 0) { pivotRow_ = r; pivotCol_ = c; }
					++cellCount_;
				}
		if (pivotRow_ < 0)
			throw std::invalid_argument("Part: shape has no filled cell");
	}

	// 將形狀順時針旋轉 90 度，並回傳 pivot 的位移差值
	Part::RotateDelta Part::rotateRight() {
		const int oldH = height_, oldW = width_;
		const int oldPivotR = pivotRow_, oldPivotC = pivotCol_;

		Shape rotated(oldW, std::vector<uint8_t>(oldH, 0));
		for (int r = 0; r < oldH; ++r)
			for (int c = 0; c < oldW; ++c)
				rotated[c][oldH - 1 - r] = shape_[r][c];

		shape_ = std::move(rotated);
		height_ = oldW;
		width_ = oldH;
		recomputePivotAndCount();

		// 計算旋轉後原 pivot 在新座標系的位置
		const int rotatedOldPivotR = oldPivotC;
		const int rotatedOldPivotC = oldH - 1 - oldPivotR;

		pivotRow_ = rotatedOldPivotR;
		pivotCol_ = rotatedOldPivotC;

		return RotateDelta{
			oldPivotR - rotatedOldPivotR,
			oldPivotC - rotatedOldPivotC
		};
	}

	// 產生一個旋轉指定次數的拷貝（不修改原物件）
	Part Part::rotated(int times) const {
		Part copy = *this;
		for (int i = 0; i < (times % 4); ++i)
			copy.rotateRight();
		return copy;
	}

} // namespace ark

