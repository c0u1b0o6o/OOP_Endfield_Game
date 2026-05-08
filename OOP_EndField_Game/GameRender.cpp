#include "Game.h"
#include <cmath>
#include <sstream>

namespace ark {

	void Game::renderButton(float x, float y, float w, float h, const std::string& label, bool hover) {
		renderColorButton(x, y, w, h, label, hover, Colors::accent());
	}

	void Game::renderColorButton(float x, float y, float w, float h, const std::string& label, bool hover, sf::Color bgColor) {
		sf::RectangleShape bg(sf::Vector2f(w, h));
		bg.setPosition(sf::Vector2f(x, y));

		if (hover) {
			bgColor.r = (uint8_t)std::min<int>(255, bgColor.r + 20);
			bgColor.g = (uint8_t)std::min<int>(255, bgColor.g + 20);
			bgColor.b = (uint8_t)std::min<int>(255, bgColor.b + 20);
		}

		bg.setFillColor(bgColor);
		bg.setOutlineColor(sf::Color(255, 255, 255, 40));
		bg.setOutlineThickness(1.f);
		window_.draw(bg);

		sf::Text txt(font_, label, (unsigned int)(h * 0.5f));
		txt.setFillColor(sf::Color(20, 20, 30));
		auto bounds = txt.getLocalBounds();
		txt.setPosition(sf::Vector2f(x + (w - bounds.size.x) / 2 - bounds.position.x,
			y + (h - bounds.size.y) / 2 - bounds.position.y));
		window_.draw(txt);
	}

	void Game::renderBoard(const Board& board, float ox, float oy, float cs) {
		int R = board.rows(), C = board.cols();
		// Grid background
		sf::RectangleShape gridBg(sf::Vector2f(C * cs, R * cs));
		gridBg.setPosition(sf::Vector2f(ox, oy));
		gridBg.setFillColor(Colors::grid());
		gridBg.setOutlineColor(Colors::gridLine());
		gridBg.setOutlineThickness(2.f);
		window_.draw(gridBg);

		// Cells
		for (int r = 0; r < R; ++r) {
			for (int c = 0; c < C; ++c) {
				sf::RectangleShape cell(sf::Vector2f(cs - 2, cs - 2));
				cell.setPosition(sf::Vector2f(ox + c * cs + 1, oy + r * cs + 1));
				int t = board.cellType(r, c);
				if (t == cell::BLOCK) {
					cell.setFillColor(Colors::blocked());
					// Draw X pattern
					window_.draw(cell);
					sf::RectangleShape line1(sf::Vector2f(cs * 0.7f, 2.f));
					line1.setOrigin(sf::Vector2f(cs * 0.35f, 1.f));
					line1.setPosition(sf::Vector2f(ox + c * cs + cs / 2, oy + r * cs + cs / 2));
					line1.setRotation(sf::degrees(45.f));
					line1.setFillColor(sf::Color(100, 100, 110));
					window_.draw(line1);
					line1.setRotation(sf::degrees(-45.f));
					window_.draw(line1);
					continue;
				}
				else if (t == cell::FIXED) {
					int clr = board.cellColor(r, c);
					auto fc = Colors::partColor(clr);
					fc.r = (uint8_t)std::min(255, fc.r + 40);
					fc.g = (uint8_t)std::min(255, fc.g + 40);
					fc.b = (uint8_t)std::min(255, fc.b + 40);
					cell.setFillColor(fc);
					cell.setOutlineColor(sf::Color(255, 255, 255, 80));
					cell.setOutlineThickness(1.f);
					window_.draw(cell);
					// Lock icon (= symbol)
					sf::Text eq(font_, "=", (unsigned int)(cs * 0.5f));
					eq.setFillColor(sf::Color(255, 255, 255, 200));
					auto eb = eq.getLocalBounds();
					eq.setPosition(sf::Vector2f(ox + c * cs + (cs - eb.size.x) / 2 - eb.position.x,
						oy + r * cs + (cs - eb.size.y) / 2 - eb.position.y));
					window_.draw(eq);
					continue;
				}
				else if (t == cell::EMPTY) {
					cell.setFillColor(sf::Color(30, 33, 45));
				}
				else {
					// Player part
					int clr = board.cellColor(r, c);
					cell.setFillColor(Colors::partColor(clr));
					cell.setOutlineColor(sf::Color(255, 255, 255, 30));
					cell.setOutlineThickness(1.f);
				}
				window_.draw(cell);
			}
		}

		// Grid lines
		for (int r = 0; r <= R; ++r) {
			sf::RectangleShape line(sf::Vector2f(C * cs, 1.f));
			line.setPosition(sf::Vector2f(ox, oy + r * cs));
			line.setFillColor(Colors::gridLine());
			window_.draw(line);
		}
		for (int c = 0; c <= C; ++c) {
			sf::RectangleShape line(sf::Vector2f(1.f, R * cs));
			line.setPosition(sf::Vector2f(ox + c * cs, oy));
			line.setFillColor(Colors::gridLine());
			window_.draw(line);
		}
	}

	void Game::renderTargets(const Board& board, float ox, float oy, float cs) {
		int R = board.rows(), C = board.cols(), CC = board.colorCount();
		unsigned int fontSize = (unsigned int)(cs * 0.32f);
		if (CC > 1) fontSize = (unsigned int)(cs * 0.24f);

		// Row targets (left side)
		for (int r = 0; r < R; ++r) {
			float ty = oy + r * cs;
			for (int color = 0; color < CC; ++color) {
				int target = board.targetRow(color, r);
				int current = board.currentRow(color, r);
				sf::Color col = Colors::partColor(color);
				if (current == target && target > 0) col = sf::Color(255, 255, 100);
				else if (current > target) col = Colors::error();

				std::string s = std::to_string(target);
				sf::Text txt(font_, s, fontSize);
				txt.setFillColor(col);
				float tx = ox - cs + color * (cs / CC);
				auto b = txt.getLocalBounds();
				txt.setPosition(sf::Vector2f(tx + (cs / CC - b.size.x) / 2,
					ty + (cs - b.size.y) / 2 - b.position.y));
				window_.draw(txt);

				// Current count (smaller, below)
				sf::Text cur(font_, std::to_string(current), (unsigned int)(fontSize * 0.7f));
				cur.setFillColor(sf::Color(col.r, col.g, col.b, 120));
				auto cb = cur.getLocalBounds();
				cur.setPosition(sf::Vector2f(tx + (cs / CC - cb.size.x) / 2,
					ty + cs * 0.65f));
				window_.draw(cur);
			}
		}

		// Column targets (top)
		for (int c = 0; c < C; ++c) {
			float tx = ox + c * cs;
			for (int color = 0; color < CC; ++color) {
				int target = board.targetCol(color, c);
				int current = board.currentCol(color, c);
				sf::Color col = Colors::partColor(color);
				if (current == target && target > 0) col = sf::Color(255, 255, 100);
				else if (current > target) col = Colors::error();

				std::string s = std::to_string(target);
				sf::Text txt(font_, s, fontSize);
				txt.setFillColor(col);
				float ty2 = oy - cs + color * (cs / CC);
				auto b = txt.getLocalBounds();
				txt.setPosition(sf::Vector2f(tx + (cs - b.size.x) / 2 - b.position.x,
					ty2 + (cs / CC - b.size.y) / 2 - b.position.y));
				window_.draw(txt);

				sf::Text cur(font_, std::to_string(current), (unsigned int)(fontSize * 0.7f));
				cur.setFillColor(sf::Color(col.r, col.g, col.b, 120));
				auto cb = cur.getLocalBounds();
				cur.setPosition(sf::Vector2f(tx + cs * 0.65f,
					ty2 + (cs / CC - cb.size.y) / 2 - cb.position.y));
				window_.draw(cur);
			}
		}
	}

	void Game::renderGhost(float ox, float oy, float cs) {
		if (selectedPart_ < 0) return;
		auto& p = parts_[selectedPart_];
		int ar = ghostRow_ - p.pivotRow();
		int ac = ghostCol_ - p.pivotCol();
		auto ghostCol = Colors::partGhost(p.colorIndex());

		for (int r = 0; r < p.height(); ++r) {
			for (int c = 0; c < p.width(); ++c) {
				if (!p.shape()[r][c]) continue;
				float px = ox + (ac + c) * cs + 1;
				float py = oy + (ar + r) * cs + 1;
				sf::RectangleShape cell(sf::Vector2f(cs - 2, cs - 2));
				cell.setPosition(sf::Vector2f(px, py));
				cell.setFillColor(ghostCol);
				cell.setOutlineColor(sf::Color(255, 255, 255, 60));
				cell.setOutlineThickness(1.f);
				window_.draw(cell);
			}
		}
	}

	void Game::renderHints(float ox, float oy, float cs) {
		if (!showingHint_ || !solutionFound_ || hintPartIdx_ < 0) return;
		// Find solution placement for hint part
		for (auto& sp : solution_) {
			if (sp.partId == hintPartIdx_) {
				Part p = initialParts_[sp.partId].rotated(sp.rotation);
				for (int r = 0; r < p.height(); ++r) {
					for (int c = 0; c < p.width(); ++c) {
						if (!p.shape()[r][c]) continue;
						float px = ox + (sp.anchorCol + c) * cs + 1;
						float py = oy + (sp.anchorRow + r) * cs + 1;
						sf::RectangleShape cell(sf::Vector2f(cs - 2, cs - 2));
						cell.setPosition(sf::Vector2f(px, py));
						cell.setFillColor(Colors::hintColor());
						cell.setOutlineColor(sf::Color(80, 200, 120, 180));
						cell.setOutlineThickness(2.f);
						window_.draw(cell);
					}
				}
				break;
			}
		}
	}

	void Game::renderPartPalette() {
		float palX = boardOffX_ + board_.cols() * cellSize_ + 60.f;
		float palY = 80.f;

		sf::Text title(font_, "Parts", 20);
		title.setFillColor(Colors::text());
		title.setPosition(sf::Vector2f(palX, palY - 30));
		window_.draw(title);

		for (int i = 0; i < (int)parts_.size(); ++i) {
			float py = palY + 20.f + i * 65.f;
			bool isSelected = (i == selectedPart_);
			bool isPlaced = placements_[i].placed;

			sf::RectangleShape bg(sf::Vector2f(200, 58));
			bg.setPosition(sf::Vector2f(palX, py));
			sf::Color bgc = isSelected ? sf::Color(60, 80, 60) :
				isPlaced ? sf::Color(35, 40, 50) : Colors::panel();
			bg.setFillColor(bgc);
			bg.setOutlineColor(isSelected ? Colors::accent() : sf::Color(50, 55, 70));
			bg.setOutlineThickness(isSelected ? 2.f : 1.f);
			window_.draw(bg);

			// Mini preview
			auto& p = parts_[i];
			float miniCs = std::min(50.f / p.height(), 50.f / p.width());
			miniCs = std::min(miniCs, 12.f);
			for (int r = 0; r < p.height(); ++r) {
				for (int c = 0; c < p.width(); ++c) {
					if (!p.shape()[r][c]) continue;
					sf::RectangleShape mc(sf::Vector2f(miniCs - 1, miniCs - 1));
					mc.setPosition(sf::Vector2f(palX + 8 + c * miniCs, py + 4 + r * miniCs));
					mc.setFillColor(isPlaced ? sf::Color(60, 65, 75) : Colors::partColor(p.colorIndex()));
					window_.draw(mc);
				}
			}

			// Label
			std::string label = "#" + std::to_string(i);
			if (isPlaced) label += " [placed]";
			sf::Text txt(font_, label, 13);
			txt.setFillColor(isPlaced ? sf::Color(100, 105, 120) : Colors::text());
			txt.setPosition(sf::Vector2f(palX + 70, py + 20));
			window_.draw(txt);
		}
	}

	// ---- SCENE RENDERERS ----

	void Game::renderMainMenu() {
		// Title
		sf::Text title(font_, "Originium Circuit Repair", 42);
		title.setFillColor(Colors::accent());
		auto tb = title.getLocalBounds();
		title.setPosition(sf::Vector2f(640 - tb.size.x / 2, 140));
		window_.draw(title);

		sf::Text sub(font_, "Arknights: Endfield Puzzle", 18);
		sub.setFillColor(sf::Color(150, 155, 170));
		auto sb = sub.getLocalBounds();
		sub.setPosition(sf::Vector2f(640 - sb.size.x / 2, 200));
		window_.draw(sub);

		float cx = 640.f, bw = 300.f, bh = 55.f;
		renderButton(cx - bw / 2, 300, bw, bh, "Start Game", isMouseOver(cx - bw / 2, 300, bw, bh));
		renderButton(cx - bw / 2, 380, bw, bh, "Level Editor", isMouseOver(cx - bw / 2, 380, bw, bh));
		renderButton(cx - bw / 2, 460, bw, bh, "Quit", isMouseOver(cx - bw / 2, 460, bw, bh));

		// Decorative line
		sf::RectangleShape line(sf::Vector2f(400, 2));
		line.setPosition(sf::Vector2f(440, 260));
		line.setFillColor(sf::Color(60, 65, 80));
		window_.draw(line);
	}

	void Game::renderLevelSelect() {
		sf::Text title(font_, "Select Level", 32);
		title.setFillColor(Colors::accent());
		auto bounds = title.getLocalBounds();
		title.setPosition(sf::Vector2f(640.f - bounds.size.x / 2.f - bounds.position.x, 30.f));
		window_.draw(title);

		renderButton(40, 380, 100, 40, "Back", isMouseOver(40, 380, 100, 40));

		if (levelFiles_.empty()) {
			sf::Text none(font_, "No .txt files found in Levels/ folder", 18);
			none.setFillColor(Colors::error());
			none.setPosition(sf::Vector2f(390, 150));
			window_.draw(none);
			return;
		}

		// --- 【新增】1. 儲存預設視角 ---
		sf::View defaultView = window_.getView();

		// --- 【新增】2. 建立一個限定範圍的視角 (從 Y=90 開始，高度 570) ---
		// SFML 3 寫法： sf::FloatRect({left, top}, {width, height})
		sf::View listView(sf::FloatRect({ 0.f, 90.f }, { 1280.f, 570.f }));
		listView.setViewport(sf::FloatRect({ 0.f, 90.f / 800.f }, { 1.f, 570.f / 800.f }));
		window_.setView(listView);

		for (int i = 0; i < (int)levelFiles_.size(); ++i) {
			float y = 100.f + i * 60.f + levelScrollY_;
			bool hov = isMouseOver(390, y, 500, 50);
			sf::RectangleShape bg(sf::Vector2f(500, 50));
			bg.setPosition(sf::Vector2f(390, y));
			bg.setFillColor(hov ? sf::Color(45, 55, 65) : Colors::panel());
			bg.setOutlineColor(hov ? Colors::accent() : sf::Color(50, 55, 70));
			bg.setOutlineThickness(1.f);
			window_.draw(bg);

			auto path = std::filesystem::path(levelFiles_[i]);
			sf::Text txt(font_, sf::String(path.filename().wstring()), 18);
			txt.setFillColor(hov ? Colors::accentHover() : Colors::text());
			txt.setPosition(sf::Vector2f(410, y + 14));
			window_.draw(txt);
		}
		// --- 【新增】3. 畫完列表後，切換回原本的視角 ---
		window_.setView(defaultView);
	}

	void Game::renderPlaying() {
		// Header
		auto path = std::filesystem::path(currentLevelPath_);
		sf::String headerStr = L"Level: " + path.filename().wstring();
		sf::Text header(font_, headerStr, 20);
		header.setFillColor(Colors::text());
		header.setPosition(sf::Vector2f(boardOffX_, 10));
		window_.draw(header);

		std::string info = std::to_string(board_.rows()) + "x" + std::to_string(board_.cols())
			+ "  Colors: " + std::to_string(board_.colorCount())
			+ "  Placed: " + std::to_string(placedCount_) + "/" + std::to_string(parts_.size());
		sf::Text infoTxt(font_, info, 14);
		infoTxt.setFillColor(sf::Color(130, 135, 150));
		infoTxt.setPosition(sf::Vector2f(boardOffX_, 38));
		window_.draw(infoTxt);

		renderBoard(board_, boardOffX_, boardOffY_, cellSize_);
		renderTargets(board_, boardOffX_, boardOffY_, cellSize_);
		renderHints(boardOffX_, boardOffY_, cellSize_);
		renderGhost(boardOffX_, boardOffY_, cellSize_);
		renderPartPalette();

		// Buttons
		float btnY = boardOffY_ + board_.rows() * cellSize_ + 40.f;
		renderButton(boardOffX_, btnY, 140, 50, "Reset", isMouseOver(boardOffX_, btnY, 140, 50));
		renderButton(boardOffX_ + 160, btnY, 140, 50, "Solve All", isMouseOver(boardOffX_ + 160, btnY, 140, 50));

		if (hintAvailable_) {
			renderButton(boardOffX_ + 320, btnY, 140, 50, "Hint",
				isMouseOver(boardOffX_ + 320, btnY, 140, 50));
		}
		else {
			sf::RectangleShape disBtn(sf::Vector2f(140, 50));
			disBtn.setPosition(sf::Vector2f(boardOffX_ + 320, btnY));
			disBtn.setFillColor(sf::Color(50, 55, 65));
			window_.draw(disBtn);
			sf::Text ht(font_, "Hint (wait)", 18);
			ht.setFillColor(sf::Color(80, 85, 95));
			ht.setPosition(sf::Vector2f(boardOffX_ + 340, btnY + 12));
			window_.draw(ht);
		}

		renderButton(boardOffX_ + 480, btnY, 140, 50, "Back",
			isMouseOver(boardOffX_ + 480, btnY, 140, 50));

		// Status message
		if (!statusMsg_.empty()) {
			sf::Text st(font_, statusMsg_, 20);
			st.setFillColor(Colors::error());
			st.setPosition(sf::Vector2f(boardOffX_, btnY + 70));
			window_.draw(st);
		}

		// Controls help
		sf::Text help(font_, "R:Rotate  Enter/Click:Place  Esc/Right Click:Deselect  F5:Reset  F1:Hint", 16);
		help.setFillColor(sf::Color(80, 85, 100));
		help.setPosition(sf::Vector2f(20, 760));
		window_.draw(help);

		if (showingNoSolution_) {
			sf::RectangleShape overlay(sf::Vector2f(1280, 800));
			overlay.setFillColor(sf::Color(0, 0, 0, 150));
			window_.draw(overlay);

			sf::Text msg(font_, "NO SOLUTION", 48);
			msg.setFillColor(Colors::error());
			auto cb = msg.getLocalBounds();
			msg.setPosition(sf::Vector2f(640 - cb.size.x / 2.f, 300.f));
			window_.draw(msg);

			renderButton(540, 450, 200, 50, "Back", isMouseOver(540, 450, 200, 50));
		}
	}

	void Game::renderEditor() {
		sf::Text title(font_, "Level Editor", 32);
		title.setFillColor(Colors::accent());
		title.setPosition(sf::Vector2f(20, 10));
		window_.draw(title);

		// Tool buttons (Y = 60)
		float tx = 20.f;
		renderButton(tx, 60, 110, 40, "Empty", editorTool_ == 0); tx += 120;
		renderButton(tx, 60, 110, 40, "Block(X)", editorTool_ == 1); tx += 120;
		for (int c = 0; c < editorColors_; c++) {
			std::string l = "Fixed C" + std::to_string(c);
			renderColorButton(tx, 60, 110, 40, l, editorTool_ == 2 + c, Colors::partColor(c));
			tx += 120;
		}

		// Size controls (Y = 120)
		float cy = 120.f;
		sf::Text rl(font_, "Rows: " + std::to_string(editorRows_), 20);
		rl.setFillColor(Colors::text()); rl.setPosition(sf::Vector2f(20, cy + 8)); window_.draw(rl);
		renderButton(120, cy, 40, 40, "-", false); renderButton(170, cy, 40, 40, "+", false);

		sf::Text cl(font_, "Cols: " + std::to_string(editorCols_), 20);
		cl.setFillColor(Colors::text()); cl.setPosition(sf::Vector2f(250, cy + 8)); window_.draw(cl);
		renderButton(350, cy, 40, 40, "-", false); renderButton(400, cy, 40, 40, "+", false);

		sf::Text ccl(font_, "Colors:" + std::to_string(editorColors_), 20);
		ccl.setFillColor(Colors::text()); ccl.setPosition(sf::Vector2f(480, cy + 8)); window_.draw(ccl);
		renderButton(580, cy, 40, 40, "-", false); renderButton(630, cy, 40, 40, "+", false);

		// Target Color panel (Y = 170)
		float tcy = 170.f;
		sf::Text tcl(font_, "Target Color:", 20);
		tcl.setFillColor(Colors::text());
		tcl.setPosition(sf::Vector2f(20, tcy + 8));
		window_.draw(tcl);

		float tcx = 160.f;
		for (int c = 0; c < editorColors_; c++) {
			std::string l = "C" + std::to_string(c);
			renderColorButton(tcx, tcy, 60, 40, l, editorTargetColor_ == c, Colors::partColor(c));
			tcx += 70.f;
		}

		// Editor board (Y = 260)
		float eox = 140.f, eoy = 260.f, ecs = 50.f;
		if (editorBoard_.rows() > 0) {
			renderBoard(editorBoard_, eox, eoy, ecs);

			int tc = editorTargetColor_;
			if (tc < editorBoard_.colorCount()) {
				// Row Target adjustments (left side)
				for (int r = 0; r < editorBoard_.rows(); ++r) {
					float yy = eoy + r * ecs;
					int tg = editorBoard_.targetRow(tc, r);
					renderButton(eox - 95, yy + 10, 25, 30, "-", false);
					sf::Text tt(font_, std::to_string(tg), 18);
					tt.setFillColor(Colors::partColor(tc));
					auto b = tt.getLocalBounds();
					tt.setPosition(sf::Vector2f(eox - 65 + (25 - b.size.x) / 2.f, yy + 15));
					window_.draw(tt);
					renderButton(eox - 35, yy + 10, 25, 30, "+", false);
				}
				// Col Target adjustments (top side)
				for (int c = 0; c < editorBoard_.cols(); ++c) {
					float xx = eox + c * ecs;
					int tg = editorBoard_.targetCol(tc, c);
					renderButton(xx + 10, eoy - 95, 30, 25, "+", false);
					sf::Text tt(font_, std::to_string(tg), 18);
					tt.setFillColor(Colors::partColor(tc));
					auto b = tt.getLocalBounds();
					tt.setPosition(sf::Vector2f(xx + 15 + (20 - b.size.x) / 2.f, eoy - 65));
					window_.draw(tt);
					renderButton(xx + 10, eoy - 35, 30, 25, "-", false);
				}
			}
		}
		else {
			sf::Text hint(font_, "Click 'Apply Size' to create board", 20);
			hint.setFillColor(sf::Color(150, 155, 170));
			hint.setPosition(sf::Vector2f(eox, eoy + 20));
			window_.draw(hint);
		}

		// Part creation panel
		float pcx = eox + std::max(editorCols_, 5) * ecs + 120.f;
		if (pcx < 700.f) pcx = 700.f;

		sf::Text pt(font_, "Create Part", 24);
		pt.setFillColor(Colors::accent());
		pt.setPosition(sf::Vector2f(pcx, 260));
		window_.draw(pt);

		// W, H, Color (Y = 310)
		float psy = 310.f;
		sf::Text pw(font_, "W: " + std::to_string(editorPartW_), 18);
		pw.setFillColor(Colors::text()); pw.setPosition(sf::Vector2f(pcx, psy + 10)); window_.draw(pw);
		renderButton(pcx + 50, psy, 35, 35, "-", false); renderButton(pcx + 90, psy, 35, 35, "+", false);

		sf::Text ph(font_, "H: " + std::to_string(editorPartH_), 18);
		ph.setFillColor(Colors::text()); ph.setPosition(sf::Vector2f(pcx + 140, psy + 10)); window_.draw(ph);
		renderButton(pcx + 190, psy, 35, 35, "-", false); renderButton(pcx + 230, psy, 35, 35, "+", false);

		sf::Text pcol(font_, "Color: " + std::to_string(editorPartColor_), 18);
		pcol.setFillColor(Colors::partColor(editorPartColor_));
		pcol.setPosition(sf::Vector2f(pcx + 280, psy + 10)); window_.draw(pcol);
		renderButton(pcx + 380, psy, 110, 35, "Next Color", false);

		// Part shape grid (Y = 370)
		float psgy = 370.f;
		for (int r = 0; r < editorPartH_; r++) {
			for (int c = 0; c < editorPartW_; c++) {
				sf::RectangleShape cell(sf::Vector2f(38, 38));
				cell.setPosition(sf::Vector2f(pcx + c * 40, psgy + r * 40));
				bool filled = (r < (int)editorPartShape_.size() && c < (int)editorPartShape_[0].size()
					&& editorPartShape_[r][c]);
				cell.setFillColor(filled ? Colors::partColor(editorPartColor_) : sf::Color(40, 44, 55));
				cell.setOutlineColor(Colors::gridLine());
				cell.setOutlineThickness(1.f);
				window_.draw(cell);
			}
		}
		renderButton(pcx, psgy + editorPartH_ * 40 + 20, 140, 45, "Add Part", false);

		sf::Text pl(font_, "Parts Created: " + std::to_string(editorParts_.size()), 20);
		pl.setFillColor(Colors::text());
		pl.setPosition(sf::Vector2f(pcx + 160, psgy + editorPartH_ * 40 + 30));
		window_.draw(pl);

		// Draw little icons of editor parts
		float partsStartY = psgy + editorPartH_ * 40 + 80;

		// Set up a view for clipping the scrollable area
		sf::View defaultView = window_.getView();
		sf::View partsView(sf::FloatRect({ pcx, partsStartY }, { 1280.f - pcx, 800.f - partsStartY }));
		partsView.setViewport(sf::FloatRect({ pcx / 1280.f, partsStartY / 800.f }, { (1280.f - pcx) / 1280.f, (800.f - partsStartY) / 800.f }));
		window_.setView(partsView);

		float lpx = pcx;
		float lpy = partsStartY + editorScrollY_;
		for (size_t i = 0; i < editorParts_.size(); ++i) {
			if (lpx + 60 > 1200) { lpx = pcx; lpy += 60; }

			sf::RectangleShape pBg(sf::Vector2f(50, 50));
			pBg.setPosition(sf::Vector2f(lpx, lpy));
			pBg.setFillColor(Colors::panel());
			pBg.setOutlineThickness(1.f);
			pBg.setOutlineColor(sf::Color(60, 65, 80));
			window_.draw(pBg);

			auto& pt = editorParts_[i];
			float miniCs = std::min(40.f / pt.height(), 40.f / pt.width());
			miniCs = std::min(miniCs, 12.f);
			for (int pr = 0; pr < pt.height(); ++pr) {
				for (int pc = 0; pc < pt.width(); ++pc) {
					if (!pt.shape()[pr][pc]) continue;
					sf::RectangleShape mc(sf::Vector2f(miniCs - 1, miniCs - 1));
					mc.setPosition(sf::Vector2f(lpx + 5 + pc * miniCs, lpy + 5 + pr * miniCs));
					mc.setFillColor(Colors::partColor(pt.colorIndex()));
					window_.draw(mc);
				}
			}

			// Delete Button
			sf::RectangleShape delBtn(sf::Vector2f(12, 12));
			delBtn.setPosition(sf::Vector2f(lpx + 35, lpy + 3));
			delBtn.setFillColor(Colors::error());
			window_.draw(delBtn);
			sf::Text dx(font_, "X", 10);
			dx.setFillColor(sf::Color::White);
			dx.setPosition(sf::Vector2f(lpx + 37, lpy + 1));
			window_.draw(dx);

			lpx += 60;
		}

		window_.setView(defaultView);

		// Bottom buttons
		float btnY2 = eoy + std::max(editorRows_, 5) * ecs + 50.f;
		if (btnY2 < psgy + editorPartH_ * 40 + 100.f) btnY2 = psgy + editorPartH_ * 40 + 100.f;

		renderButton(eox, btnY2, 140, 50, "Export", isMouseOver(eox, btnY2, 140, 50));
		renderButton(eox + 160, btnY2, 140, 50, "Test Play", isMouseOver(eox + 160, btnY2, 140, 50));
		renderButton(eox + 320, btnY2, 140, 50, "Back", isMouseOver(eox + 320, btnY2, 140, 50));

		if (!statusMsg_.empty()) {
			sf::Text st(font_, statusMsg_, 18);
			st.setFillColor(Colors::accent());
			st.setPosition(sf::Vector2f(eox, btnY2 + 70));
			window_.draw(st);
		}
	}

	void Game::renderVictory() {
		renderBoard(board_, boardOffX_, boardOffY_, cellSize_);
		renderTargets(board_, boardOffX_, boardOffY_, cellSize_);

		// Overlay
		sf::RectangleShape overlay(sf::Vector2f(1280, 800));
		overlay.setFillColor(sf::Color(0, 0, 0, 150));
		window_.draw(overlay);

		sf::Text congrats(font_, "PUZZLE COMPLETE!", 48);
		congrats.setFillColor(Colors::accent());
		auto cb = congrats.getLocalBounds();
		congrats.setPosition(sf::Vector2f(640 - cb.size.x / 2, 300));
		window_.draw(congrats);

		sf::Text sub(font_, "All parts placed correctly!", 22);
		sub.setFillColor(Colors::text());
		auto sb = sub.getLocalBounds();
		sub.setPosition(sf::Vector2f(640 - sb.size.x / 2, 380));
		window_.draw(sub);

		renderButton(540, 450, 200, 50, "Next Game", isMouseOver(540, 450, 200, 50));
		renderButton(540, 520, 200, 50, "Main Menu", isMouseOver(540, 520, 200, 50));
	}

	void Game::render() {
		window_.clear(Colors::bg());
		switch (scene_) {
		case Scene::MainMenu:    renderMainMenu(); break;
		case Scene::LevelSelect: renderLevelSelect(); break;
		case Scene::Playing:     renderPlaying(); break;
		case Scene::Editor:      renderEditor(); break;
		case Scene::Victory:     renderVictory(); break;
		}
		window_.display();
	}

} // namespace ark


