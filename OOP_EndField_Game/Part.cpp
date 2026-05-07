#include "Part.h"
#include <stdexcept>

namespace ark {

	Part::Part(int id, int colorIndex, const Shape& shape)
		: id_(id), colorIndex_(colorIndex), shape_(shape)
	{
		if (shape_.empty() || shape_[0].empty())
			throw std::invalid_argument("Part::Part: empty shape");
		height_ = static_cast<int>(shape_.size());
		width_ = static_cast<int>(shape_[0].size());
		recomputePivotAndCount();
	}

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

	Part::RotateDelta Part::rotateRight() {
		const int oldH = height_, oldW = width_;
		const int oldPivotR = pivotRow_, oldPivotC = pivotCol_;

		// 90° 順時針：new[c][H-1-r] = old[r][c]
		Shape rotated(oldW, std::vector<uint8_t>(oldH, 0));
		for (int r = 0; r < oldH; ++r)
			for (int c = 0; c < oldW; ++c)
				rotated[c][oldH - 1 - r] = shape_[r][c];

		shape_ = std::move(rotated);
		height_ = oldW;
		width_ = oldH;
		recomputePivotAndCount();

		// 舊 pivot 旋轉後在新 shape 中的位置
		const int rotatedOldPivotR = oldPivotC;
		const int rotatedOldPivotC = oldH - 1 - oldPivotR;
		// 覆蓋 pivot 為舊 pivot 旋轉後的實際位置 (用戶感知的中心)
		pivotRow_ = rotatedOldPivotR;
		pivotCol_ = rotatedOldPivotC;

		return RotateDelta{
			oldPivotR - rotatedOldPivotR,
			oldPivotC - rotatedOldPivotC
		};
	}

	Part Part::rotated(int times) const {
		Part copy = *this;
		for (int i = 0; i < (times % 4); ++i)
			copy.rotateRight();
		return copy;
	}

} // namespace ark

