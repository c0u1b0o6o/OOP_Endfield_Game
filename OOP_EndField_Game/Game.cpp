#include "Game.h"
#define NOMINMAX
#include <windows.h>
#include <commdlg.h>
#pragma comment(lib, "Comdlg32.lib")
#include <iostream>
#include <algorithm>
#include <cmath>

namespace ark {

	// 建構子：初始化視窗與遊戲資源
	// 設計說明：
	// - 在建構子中進行基本資源載入（字型、音效）與關卡掃描，確保遊戲介面可即時顯示。
	// - 使用固定視窗大小（1280x800）與 FPS 上限來保持畫面穩定。
	// - 編輯器的預設 part shape 以 editorPartH_ / editorPartW_ 為基準生成空格陣列，便於使用者直接編輯。
	Game::Game() {
		window_.create(sf::VideoMode({ 1280u, 800u }), "Originium Circuit Repair",
			sf::Style::Close | sf::Style::Resize);
		window_.setFramerateLimit(60);
		loadFont();
		loadSounds();
		scanLevels();
		editorPartShape_.assign(editorPartH_, std::vector<uint8_t>(editorPartW_, 0));
	}

	// 載入音效與背景音樂
	// 詳細說明：
	// - 為避免遊戲在資源缺失時崩潰，載入操作以 if 判斷並在失敗時忽略特定音效。
	// - BGM 使用 sf::Music 並設定為 loop，使遊戲背景音自動循環播放，音量設定為 40 以免過大。
	// - 使用 std::optional 或類似容器（在 header 定義）以延遲初始化音效物件，避免在無檔案時使用未初始化的資源。
	void Game::loadSounds() {
		if (sbPlace_.loadFromFile("Assets/sfx/place.wav")) sndPlace_.emplace(sbPlace_);
		if (sbError_.loadFromFile("Assets/sfx/error.wav")) sndError_.emplace(sbError_);
		if (sbPick_.loadFromFile("Assets/sfx/pick.wav")) sndPick_.emplace(sbPick_);
		if (sbRotate_.loadFromFile("Assets/sfx/rotate.wav")) sndRotate_.emplace(sbRotate_);
		if (sbWin_.loadFromFile("Assets/sfx/win.wav")) sndWin_.emplace(sbWin_);

		// Click 音效載入
		if (sbClick_.loadFromFile("Assets/sfx/click.wav")) {
			sndClick_.emplace(sbClick_);
		}

		// 背景音樂：若載入成功則播放並循環
		bgm_.emplace(); // optional 實例化

		if (bgm_->openFromFile("Assets/music/bgm.wav")) {
			bgm_->setLooping(true);
			bgm_->setVolume(40.f);
			bgm_->play();
		}
		else {
			statusMsg_ = "Warning: BGM load failed!"; // 顯示警告但不阻斷遊戲
		}
	}

	// 載入字型：嘗試多個路徑以兼容不同系統配置
	// 設計說明：遊戲可在開發機與使用者電腦間移植，因此嘗試本地資源路徑與系統字型目錄。
	void Game::loadFont() {
		std::vector<std::string> paths = {
			"Assets/fonts/font.ttf",
			"C:/Windows/Fonts/msjh.ttf",
			"C:/Windows/Fonts/msyh.ttf",
			"C:/Windows/Fonts/simhei.ttf",
			"C:/Windows/Fonts/consola.ttf",
			"C:/Windows/Fonts/arial.ttf",
			"C:/Windows/Fonts/segoeui.ttf"
		};
		for (auto& p : paths) {
			if (font_.openFromFile(p)) return; // 成功載入後立即返回
		}
		std::cerr << "Warning: No font loaded\n"; // 若都失敗則在 stderr 提醒
	}

	// 掃描關卡檔案：尋找 Levels 目錄或相對位置
	// 設計說明：嘗試多個路徑以提高尋找成功率，排序結果以便在介面顯示穩定順序。
	void Game::scanLevels() {
		levelFiles_.clear();
		std::vector<std::string> dirs = { "Levels", ".", "../Levels" };
		for (auto& dir : dirs) {
			try {
				for (auto& entry : std::filesystem::directory_iterator(dir)) {
					if (entry.path().extension() == ".txt")
						levelFiles_.push_back(entry.path().string());
				}
				if (!levelFiles_.empty()) break;
			}
			catch (...) {} // 不拋出例外，繼續嘗試其他路徑
		}
		std::sort(levelFiles_.begin(), levelFiles_.end());
	}

	// 載入關卡並初始化遊戲狀態
	// 詳細說明：
	// - 使用 ark::loadLevel 將檔案解析成 Board 與 Part 的資料結構。
	// - 將 initialBoard_/initialParts_ 保存為初始狀態，方便重設與求解。
	// - 檢查是否在初始狀態就已經滿足勝利條件，若是則直接切換到勝利畫面。
	void Game::loadLevel(const std::string& path) {
		try {
			auto data = ark::loadLevel(path);
			board_ = data.board;
			parts_ = data.parts;
			initialBoard_ = board_;
			initialParts_ = parts_;
			placements_.assign(parts_.size(), PlacementInfo{});
			placedCount_ = 0;
			selectedPart_ = -1;
			dragging_ = false;
			solutionSearched_ = false;
			solutionFound_ = false;
			showingHint_ = false;
			hintAvailable_ = false;
			idleTimer_ = 0.f;
			statusMsg_.clear();
			currentLevelPath_ = path;
			scene_ = Scene::Playing;

			// 若初始配置已滿足條件，直接進入勝利畫面（方便測試或特殊關卡）
			if (board_.checkWinCondition((int)parts_.size(), placedCount_)) {
				if (sndWin_) sndWin_->play();
				scene_ = Scene::Victory;
				std::cout << "Instant Win: Initial configuration satisfies all conditions." << std::endl;
			}

			computeLayout();
		}
		catch (std::exception& e) {
			statusMsg_ = std::string("Load error: ") + e.what();
			statusTimer_ = 3.f;
		}
	}

	// 重設關卡至初始狀態
	// 設計說明：直接使用初始快照來恢復 board/parts，比逐個 reverse 動作更可靠且簡潔。
	void Game::resetLevel() {
		board_ = initialBoard_;
		parts_ = initialParts_;
		placements_.assign(parts_.size(), PlacementInfo{});
		placedCount_ = 0;
		selectedPart_ = -1;
		dragging_ = false;
		showingHint_ = false;
		idleTimer_ = 0.f;
	}

	// 計算 UI 佈局（格子大小與偏移）
	// 設計考量：根據棋盤大小動態調整格子尺寸使其在畫面內顯示良好，同時限制最大方格大小以避免過大
	void Game::computeLayout() {
		float maxBoardW = 650.f, maxBoardH = 600.f;
		float marginTop = 80.f;
		float csW = maxBoardW / (board_.cols() + 2);
		float csH = maxBoardH / (board_.rows() + 2);
		cellSize_ = std::min(csW, csH);
		cellSize_ = std::min(cellSize_, 70.f);
		boardOffX_ = 60.f + cellSize_; // 留空間顯示列目標
		boardOffY_ = marginTop + cellSize_; // 留空間顯示行目標
	}

	// 選取零件：若該零件已放置則先移除，再啟動拖曳狀態
	// 設計說明：允許玩家從盤面上再次點取已放置的零件以重新放置，故先移除再標為選取
	void Game::selectPart(int idx) {
		if (idx < 0 || idx >= (int)parts_.size()) return;
		if (placements_[idx].placed) {
			board_.removePart(parts_[idx]);
			placements_[idx].placed = false;
			placedCount_--;
		}
		if (sndPick_) sndPick_->play();
		selectedPart_ = idx;
		dragging_ = true;
	}

	// 取消選取
	void Game::deselectPart() {
		selectedPart_ = -1;
		dragging_ = false;
	}

	// 嘗試在 ghost 位置放置目前選取的零件，並處理放置後邏輯
	// 詳細說明：
	// - 先用 canPlace 檢查是否可放置，若失敗給予錯誤音效與提示訊息。
	// - 成功放置後更新 placements_、placedCount_，並立刻檢查勝利條件。
	void Game::tryPlace() {
		if (selectedPart_ < 0) return;
		auto& p = parts_[selectedPart_];
		int ar = ghostRow_ - p.pivotRow();
		int ac = ghostCol_ - p.pivotCol();
		std::string err;
		if (board_.canPlace(p, ar, ac, &err)) {
			if (sndPlace_) sndPlace_->play();
			board_.placePart(p, ar, ac);
			placements_[selectedPart_] = { true, ar, ac, 0 };
			placedCount_++;
			if (board_.checkWinCondition((int)parts_.size(), placedCount_)) {
				if (sndWin_) sndWin_->play();
				scene_ = Scene::Victory;
				std::cout << "=== Solution ===" << std::endl;
				board_.printSolution();
			}
			deselectPart();
		}
		else {
			// 放置失敗：播放錯誤音效並顯示錯誤文字
			if (sndError_) sndError_->play();
			statusMsg_ = err;
			statusTimer_ = 2.f;
		}
	}

	// 旋轉目前選取的零件（順時針）
	void Game::rotateCurrent() {
		if (selectedPart_ < 0) return;
		if (sndRotate_) sndRotate_->play();
		parts_[selectedPart_].rotateRight();
		rotating_ = true;
		rotAnimAngle_ = 90.f; // 旋轉動畫角度，用於渲染時顯示動畫
	}

	// 在背景求解所有解並列印（單程式內同步呼叫，非真正分離執行緒）
	// 詳細說明：solveAll 可能花費較久，這裡呼叫時最好是按下求解按鈕或閒置需求觸發。
	void Game::solveInBackground() {
		if (solutionSearched_) return;
		solutionSearched_ = true;

		auto allSols = solver_.solveAll(initialBoard_, initialParts_);
		if (!allSols.empty()) {
			solutionFound_ = true;
			solution_ = allSols[0];
			std::cout << "=== Auto-Solver Found " << allSols.size() << " Solution(s) ===" << std::endl;
			// 列印所有解到 console，方便除錯與驗證
			for (size_t i = 0; i < allSols.size(); ++i) {
				std::cout << "[Solution " << (i + 1) << "]" << std::endl;
				Board temp = initialBoard_;
				std::vector<Part> tempParts = initialParts_;
				for (auto& sp : allSols[i]) {
					Part p = tempParts[sp.partId].rotated(sp.rotation);
					temp.placePart(p, sp.anchorRow, sp.anchorCol);
				}
				temp.printSolution();
			}
		}
		else {
			solutionFound_ = false;
			std::cout << "No solution found." << std::endl;
		}
	}

	// 顯示提示（若已找到解則顯示第一個解的格子）
	void Game::showHint() {
		if (!solutionFound_ || solution_.empty()) return;
		showingHint_ = true;
	}

	// 取得滑鼠位置的輔助函式
	sf::Vector2f Game::mousePos() const {
		auto mp = sf::Mouse::getPosition(window_);
		return sf::Vector2f(static_cast<float>(mp.x), static_cast<float>(mp.y));
	}

	// 判斷滑鼠是否位於指定矩形範圍內的輔助函式
	bool Game::isMouseOver(float x, float y, float w, float h) const {
		auto m = mousePos();
		return m.x >= x && m.x <= x + w && m.y >= y && m.y <= y + h;
	}

	// ---- 更新邏輯 ----
	void Game::update(float dt) {
		// 狀態訊息計時器
		if (statusTimer_ > 0) { statusTimer_ -= dt; if (statusTimer_ <= 0) statusMsg_.clear(); }
		// 旋轉動畫計時
		if (rotating_) { rotAnimAngle_ -= dt * 500.f; if (rotAnimAngle_ <= 0) rotating_ = false; }

		switch (scene_) {
		case Scene::MainMenu:    updateMainMenu(dt); break;
		case Scene::LevelSelect: updateLevelSelect(dt); break;
		case Scene::Playing:     updatePlaying(dt); break;
		case Scene::Editor:      updateEditor(dt); break;
		case Scene::Victory:     updateVictory(dt); break;
		}
	}

	void Game::updateMainMenu(float) {}
	void Game::updateLevelSelect(float) {}

	// 更新遊玩畫面：計算 ghost 位置並檢查閒置提示觸發
	void Game::updatePlaying(float dt) {
		if (selectedPart_ >= 0) {
			auto m = mousePos();
			ghostRow_ = (int)std::floor((m.y - boardOffY_) / cellSize_);
			ghostCol_ = (int)std::floor((m.x - boardOffX_) / cellSize_);
		}
		idleTimer_ += dt;
		// 若閒置時間超過 30 秒則啟用 Hint 按鈕並在背景自動求解
		if (idleTimer_ > 30.f && !hintAvailable_) {
			hintAvailable_ = true;
			if (!solutionSearched_) solveInBackground();
		}
	}
	void Game::updateEditor(float) {}
	void Game::updateVictory(float) {}

	// ---- 事件處理 ----
	void Game::handleEvent(const sf::Event& ev) {
		switch (scene_) {
		case Scene::MainMenu:    handleMainMenuEvent(ev); break;
		case Scene::LevelSelect: handleLevelSelectEvent(ev); break;
		case Scene::Playing:     handlePlayingEvent(ev); break;
		case Scene::Editor:      handleEditorEvent(ev); break;
		case Scene::Victory:     handleVictoryEvent(ev); break;
		}
	}

	// 處理主選單事件（按鈕切換）
	void Game::handleMainMenuEvent(const sf::Event& ev) {
		if (auto* mp = ev.getIf<sf::Event::MouseButtonPressed>()) {
			if (mp->button == sf::Mouse::Button::Left) {
				float cx = 640.f, bw = 300.f, bh = 55.f;
				if (isMouseOver(cx - bw / 2, 300, bw, bh)) {
					if (sndClick_) sndClick_->play();
					scene_ = Scene::LevelSelect;
				}
				else if (isMouseOver(cx - bw / 2, 380, bw, bh)) {
					if (sndClick_) sndClick_->play();
					scene_ = Scene::Editor;
					if (editorBoard_.rows() == 0) editorBoard_ = Board(editorRows_, editorCols_, editorColors_);
				}
				else if (isMouseOver(cx - bw / 2, 460, bw, bh)) {
					window_.close(); // 退出遊戲
				}
			}
		}
	}

	// 處理關卡選單的事件：滾輪滾動與按鈕點擊
	void Game::handleLevelSelectEvent(const sf::Event& ev) {
		if (auto* sc = ev.getIf<sf::Event::MouseWheelScrolled>()) {
			float maxScroll = std::max(0.f, levelFiles_.size() * 60.f - 450.f);
			levelScrollY_ += sc->delta * 30.f;
			if (levelScrollY_ > 0) levelScrollY_ = 0;
			if (levelScrollY_ < -maxScroll) levelScrollY_ = -maxScroll;
		}

		// 合併 Click 判斷，避免 early return 導致後續按鈕失效
		if (auto* mp = ev.getIf<sf::Event::MouseButtonPressed>()) {
			if (mp->button == sf::Mouse::Button::Left) {
				if (isMouseOver(40, 380, 100, 40)) {
					if (sndClick_) sndClick_->play();
					scene_ = Scene::MainMenu;
					return;
				}
				if (isMouseOver(40, 430, 100, 40)) {
					if (sndClick_) sndClick_->play();

					OPENFILENAMEA ofn;
					CHAR szFile[260] = { 0 };
					ZeroMemory(&ofn, sizeof(OPENFILENAME));
					ofn.lStructSize = sizeof(OPENFILENAME);
					ofn.hwndOwner = NULL;
					ofn.lpstrFile = szFile;
					ofn.nMaxFile = sizeof(szFile);
					ofn.lpstrFilter = "Text Files\0*.txt\0All Files\0*.*\0";
					ofn.nFilterIndex = 1;
					ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
					if (GetOpenFileNameA(&ofn) == TRUE) {
						loadLevel(ofn.lpstrFile);
					}
					return;
				}
				for (int i = 0; i < (int)levelFiles_.size(); ++i) {
					float y = 100.f + i * 60.f + levelScrollY_;
					if (isMouseOver(390, y, 500, 50)) {
						if (sndClick_) sndClick_->play();
						loadLevel(levelFiles_[i]);
						return;
					}
				}
			}
		}
	}

	// 處理遊戲中（Playing）事件：鍵盤操作、滑鼠點擊、按鈕
	void Game::handlePlayingEvent(const sf::Event& ev) {
		if (showingNoSolution_) {
			if (auto* mp = ev.getIf<sf::Event::MouseButtonPressed>()) {
				if (mp->button == sf::Mouse::Button::Left) {
					if (isMouseOver(540, 450, 200, 50)) {
						showingNoSolution_ = false;
						if (sndClick_) sndClick_->play();
					}
				}
			}
			return;
		}

		if (auto* kp = ev.getIf<sf::Event::KeyPressed>()) {
			if (selectedPart_ >= 0) {
				if (kp->code == sf::Keyboard::Key::W) ghostRow_--;
				if (kp->code == sf::Keyboard::Key::S) ghostRow_++;
				if (kp->code == sf::Keyboard::Key::A) ghostCol_--;
				if (kp->code == sf::Keyboard::Key::D) ghostCol_++;
				if (kp->code == sf::Keyboard::Key::R) rotateCurrent();
				if (kp->code == sf::Keyboard::Key::Enter) tryPlace();
				if (kp->code == sf::Keyboard::Key::Escape) deselectPart();
			}
			if (kp->code == sf::Keyboard::Key::F5) resetLevel();
			if (kp->code == sf::Keyboard::Key::F1) {
				if (!solutionSearched_) solveInBackground();
				if (solutionFound_) {
					showHint();
				}
				else {
					showingNoSolution_ = true;
				}
			}
		}
		if (auto* mp = ev.getIf<sf::Event::MouseButtonPressed>()) {
			if (mp->button == sf::Mouse::Button::Left) {
				auto m = mousePos();
				// 右側零件盤檢測
				float palX = boardOffX_ + board_.cols() * cellSize_ + 60.f;
				for (int i = 0; i < (int)parts_.size(); ++i) {
					float py = 100.f + i * 65.f;
					if (m.x >= palX && m.x <= palX + 200 && m.y >= py && m.y <= py + 60) {
						selectPart(i);
						return;
					}
				}
				// 點擊盤面放置或選取已放置的零件
				if (selectedPart_ >= 0) {
					tryPlace();
				}
				else {
					int cr = (int)std::floor((m.y - boardOffY_) / cellSize_);
					int cc = (int)std::floor((m.x - boardOffX_) / cellSize_);
					if (board_.inBounds(cr, cc) && board_.cellType(cr, cc) >= 0) {
						int pid = board_.cellType(cr, cc);
						selectPart(pid);
					}
				}
				// 下方按鈕欄處理
				float btnY = boardOffY_ + board_.rows() * cellSize_ + 40.f;

				if (isMouseOver(boardOffX_, btnY, 140, 50)) {
					if (sndClick_) sndClick_->play();
					resetLevel();
				}
				if (isMouseOver(boardOffX_ + 160, btnY, 140, 50)) {
					if (sndClick_) sndClick_->play();
					if (!solutionSearched_) solveInBackground();
					if (solutionFound_) {
						resetLevel();
						for (auto& sp : solution_) {
							Part p = parts_[sp.partId];
							for (int r = 0; r < sp.rotation; r++) p.rotateRight();
							parts_[sp.partId] = p;
							board_.placePart(p, sp.anchorRow, sp.anchorCol);
							placements_[sp.partId] = { true, sp.anchorRow, sp.anchorCol, sp.rotation };
							placedCount_++;
						}
						if (board_.checkWinCondition((int)parts_.size(), placedCount_))
							scene_ = Scene::Victory;
					}
					else {
						showingNoSolution_ = true;
					}
				}
				if (hintAvailable_ && isMouseOver(boardOffX_ + 320, btnY, 140, 50)) {
					if (sndClick_) sndClick_->play();
					if (!solutionSearched_) solveInBackground();
					if (solutionFound_) {
						showHint();
					}
					else {
						showingNoSolution_ = true;
					}
				}
				if (isMouseOver(boardOffX_ + 480, btnY, 140, 50)) {
					if (sndClick_) sndClick_->play();
					if (testingCustomLevel_) {
						scene_ = Scene::Editor;
						testingCustomLevel_ = false;
					}
					else {
						scene_ = Scene::LevelSelect;
					}
				}
			}
			if (mp->button == sf::Mouse::Button::Right) {
				if (selectedPart_ >= 0) deselectPart();
			}
		}
	}

	// 編輯器事件處理：工具切換、尺寸調整、匯出與測試
	void Game::handleEditorEvent(const sf::Event& ev) {
		if (auto* kp = ev.getIf<sf::Event::KeyPressed>()) {
			if (kp->code == sf::Keyboard::Key::Escape) scene_ = Scene::MainMenu;
		}
		if (auto* sc = ev.getIf<sf::Event::MouseWheelScrolled>()) {
			auto m = mousePos();
			float eox = 140.f, ecs = 50.f;
			float pcx = eox + std::max(editorCols_, 5) * ecs + 120.f;
			if (pcx < 700.f) pcx = 700.f;
			float partsStartY = 430.f + editorPartH_ * 40.f + 80.f;
			if (m.x >= pcx && m.y >= partsStartY) {
				editorScrollY_ += sc->delta * 30.f;
				if (editorScrollY_ > 0.f) editorScrollY_ = 0.f;
				int iconsPerRow = std::max(1, (int)((1280.f - pcx) / 60.f));
				int maxRows = std::ceil((float)editorParts_.size() / iconsPerRow);
				float maxScroll = -std::max(0.f, (maxRows * 60.f) - (800.f - partsStartY) + 80.f);
				if (editorScrollY_ < maxScroll) editorScrollY_ = maxScroll;
			}
		}
		if (auto* mp = ev.getIf<sf::Event::MouseButtonPressed>()) {
			if (mp->button == sf::Mouse::Button::Left) {
				auto m = mousePos();
				// 工具列切換（不加音效以免吵雜）
				float tx = 20.f;
				if (isMouseOver(tx, 60, 110, 40)) { editorTool_ = 0; } tx += 120;
				if (isMouseOver(tx, 60, 110, 40)) { editorTool_ = 1; } tx += 120;
				for (int c = 0; c < editorColors_; c++) {
					if (isMouseOver(tx, 60, 110, 40)) { editorTool_ = 2 + c; }
					tx += 120;
				}

				float cy = 120.f;
				int oldR = editorRows_, oldC = editorCols_, oldCol = editorColors_;
				if (isMouseOver(120, cy, 40, 40)) { editorRows_ = std::max(2, editorRows_ - 1); }
				if (isMouseOver(170, cy, 40, 40)) { editorRows_ = std::min(10, editorRows_ + 1); }
				if (isMouseOver(350, cy, 40, 40)) { editorCols_ = std::max(2, editorCols_ - 1); }
				if (isMouseOver(400, cy, 40, 40)) { editorCols_ = std::min(10, editorCols_ + 1); }
				if (isMouseOver(580, cy, 40, 40)) { editorColors_ = std::max(1, editorColors_ - 1); }
				if (isMouseOver(630, cy, 40, 40)) { editorColors_ = std::min(4, editorColors_ + 1); }

				if (oldR != editorRows_ || oldC != editorCols_ || oldCol != editorColors_ || editorBoard_.rows() == 0) {
					Board nb(editorRows_, editorCols_, editorColors_);
					if (editorBoard_.rows() > 0) {
						for (int r = 0; r < std::min(editorBoard_.rows(), editorRows_); ++r) {
							for (int c = 0; c < std::min(editorBoard_.cols(), editorCols_); ++c) {
								int t = editorBoard_.cellType(r, c);
								if (t == -1) nb.setEmptyCell(r, c);
								else if (t == -2) nb.setBlockedCell(r, c);
								else if (t == -3) {
									int cCol = editorBoard_.cellColor(r, c);
									if (cCol < editorColors_) nb.setFixedCell(r, c, cCol);
								}
							}
						}
						for (int col = 0; col < std::min(editorBoard_.colorCount(), editorColors_); ++col) {
							for (int r = 0; r < std::min(editorBoard_.rows(), editorRows_); ++r)
								nb.setTargetRowCount(col, r, editorBoard_.targetRow(col, r));
							for (int c = 0; c < std::min(editorBoard_.cols(), editorCols_); ++c)
								nb.setTargetColCount(col, c, editorBoard_.targetCol(col, c));
						}
					}
					editorBoard_ = nb;
					editorPartColor_ = std::min(editorPartColor_, editorColors_ - 1);
					editorTargetColor_ = std::min(editorTargetColor_, editorColors_ - 1);
					auto it = std::remove_if(editorParts_.begin(), editorParts_.end(), [this](const Part& p) {
						return p.colorIndex() >= editorColors_;
						});
					editorParts_.erase(it, editorParts_.end());
				}

				// Target Color 選擇
				float tcy = 170.f;
				float tcx = 160.f;
				for (int c = 0; c < editorColors_; c++) {
					if (isMouseOver(tcx, tcy, 60, 40)) {
						editorTargetColor_ = c;
					}
					tcx += 70.f;
				}

				float eox = 140.f, eoy = 320.f, ecs = 50.f;

				// Target Value 調整
				int tc = editorTargetColor_;
				if (tc < editorBoard_.colorCount()) {
					for (int r = 0; r < editorBoard_.rows(); ++r) {
						float yy = eoy + r * ecs;
						if (isMouseOver(eox - 95, yy + 10, 25, 30)) {
							int tg = editorBoard_.targetRow(tc, r);
							editorBoard_.setTargetRowCount(tc, r, std::max(0, tg - 1));
						}
						if (isMouseOver(eox - 35, yy + 10, 25, 30)) {
							int tg = editorBoard_.targetRow(tc, r);
							editorBoard_.setTargetRowCount(tc, r, tg + 1);
						}
					}
					for (int c = 0; c < editorBoard_.cols(); ++c) {
						float xx = eox + c * ecs;
						if (isMouseOver(xx + 10, eoy - 95, 30, 25)) {
							int tg = editorBoard_.targetCol(tc, c);
							editorBoard_.setTargetColCount(tc, c, tg + 1);
						}
						if (isMouseOver(xx + 10, eoy - 35, 30, 25)) {
							int tg = editorBoard_.targetCol(tc, c);
							editorBoard_.setTargetColCount(tc, c, std::max(0, tg - 1));
						}
					}
				}

				int cr = (int)((m.y - eoy) / ecs);
				int cc = (int)((m.x - eox) / ecs);
				if (m.y >= eoy && m.x >= eox && cr >= 0 && cr < editorRows_ && cc >= 0 && cc < editorCols_) {
					if (editorTool_ == 0) editorBoard_.setEmptyCell(cr, cc);
					else if (editorTool_ == 1) editorBoard_.setBlockedCell(cr, cc);
					else editorBoard_.setFixedCell(cr, cc, editorTool_ - 2);
				}

				float pcx = eox + std::max(editorCols_, 5) * ecs + 120.f;
				if (pcx < 700.f) pcx = 700.f;

				float psx = pcx, psgy = 430.f;
				int pr = (int)((m.y - psgy) / 40.f);
				int pc = (int)((m.x - psx) / 40.f);
				if (m.y >= psgy && m.x >= psx && pr >= 0 && pr < editorPartH_ && pc >= 0 && pc < editorPartW_) {
					if (pr < (int)editorPartShape_.size() && pc < (int)editorPartShape_[0].size())
						editorPartShape_[pr][pc] ^= 1; // 切換格子
				}

				// Add part
				if (isMouseOver(pcx, psgy + editorPartH_ * 40.f + 20, 140, 45)) {
					if (sndClick_) sndClick_->play();
					bool hasCell = false;
					for (auto& row : editorPartShape_)
						for (auto v : row) if (v) hasCell = true;
					if (hasCell) {
						editorParts_.emplace_back((int)editorParts_.size(), editorPartColor_, editorPartShape_);
						editorPartShape_.assign(editorPartH_, std::vector<uint8_t>(editorPartW_, 0));
					}
				}

				// 刪除編輯器中的零件
				float partsStartY = 430.f + editorPartH_ * 40.f + 80.f;
				if (m.x >= pcx && m.x < 1280.f && m.y >= partsStartY && m.y < 800.f) {
					float lpx = pcx;
					float lpy = partsStartY + editorScrollY_;
					for (size_t i = 0; i < editorParts_.size(); ) {
						if (lpx + 60 > 1200) { lpx = pcx; lpy += 60; }

						// Delete Button bounds
						if (m.x >= lpx + 35 && m.x <= lpx + 47 && m.y >= lpy + 3 && m.y <= lpy + 15) {
							editorParts_.erase(editorParts_.begin() + i);
							if (sndClick_) sndClick_->play();
							// 更新 ID
							for (size_t j = i; j < editorParts_.size(); ++j) {
								editorParts_[j] = Part(j, editorParts_[j].colorIndex(), editorParts_[j].shape());
							}
							break; // 防止一次點擊刪除多個
						}

						lpx += 60;
						i++;
					}
				}

				float psy = 370.f;
				if (isMouseOver(pcx + 50, psy, 35, 35)) editorPartW_ = std::max(1, editorPartW_ - 1);
				if (isMouseOver(pcx + 90, psy, 35, 35)) editorPartW_ = std::min(6, editorPartW_ + 1);
				if (isMouseOver(pcx + 190, psy, 35, 35)) editorPartH_ = std::max(1, editorPartH_ - 1);
				if (isMouseOver(pcx + 230, psy, 35, 35)) editorPartH_ = std::min(6, editorPartH_ + 1);
				editorPartShape_.resize(editorPartH_);
				for (auto& r : editorPartShape_) r.resize(editorPartW_, 0);

				if (isMouseOver(pcx + 380, psy, 110, 35)) editorPartColor_ = (editorPartColor_ + 1) % editorColors_;

				// 匯出、測試與返回按鈕
				float btnY2 = eoy + std::max(editorRows_, 5) * ecs + 50.f;
				if (btnY2 < psgy + editorPartH_ * 40 + 100.f) btnY2 = psgy + editorPartH_ * 40 + 100.f;

				if (isMouseOver(eox, btnY2, 140, 50)) { // Export
					if (sndClick_) sndClick_->play();
					try {
						exportLevel("Levels/custom.txt", editorBoard_, editorParts_);
						statusMsg_ = "Exported to Levels/custom.txt";
						statusTimer_ = 3.f;
						scanLevels();
					}
					catch (std::exception& e) {
						statusMsg_ = e.what(); statusTimer_ = 3.f;
					}
				}
				if (isMouseOver(eox + 160, btnY2, 140, 50)) { // Test Play
					if (sndClick_) sndClick_->play();
					try {
						exportLevel("Levels/custom.txt", editorBoard_, editorParts_);
						scanLevels();
						loadLevel("Levels/custom.txt");
						testingCustomLevel_ = true;
					}
					catch (std::exception& e) {
						statusMsg_ = e.what(); statusTimer_ = 3.f;
					}
				}
				if (isMouseOver(eox + 320, btnY2, 140, 50)) { // Back
					if (sndClick_) sndClick_->play();
					scene_ = Scene::MainMenu;
				}
			}
		}
	}

	// 勝利畫面事件處理：按鈕返回或進入編輯器
	void Game::handleVictoryEvent(const sf::Event& ev) {
		if (auto* mp = ev.getIf<sf::Event::MouseButtonPressed>()) {
			if (mp->button == sf::Mouse::Button::Left) {
				if (isMouseOver(540, 450, 200, 50)) {
					if (sndClick_) sndClick_->play();
					if (testingCustomLevel_) {
						scene_ = Scene::Editor;
						testingCustomLevel_ = false;
					}
					else {
						scene_ = Scene::LevelSelect;
					}
				}
				if (isMouseOver(540, 520, 200, 50)) {
					if (sndClick_) sndClick_->play();
					scene_ = Scene::MainMenu;
				}
			}
		}
	}

	// 主迴圈：處理事件、更新與渲染
	void Game::run() {
		if (!startLevelPath_.empty()) {
			loadLevel(startLevelPath_);
		}
		sf::Clock clock;
		while (window_.isOpen()) {
			// SFML 3.x 抓取事件使用 pollEvent() 回傳 std::optional<sf::Event>
			while (auto ev = window_.pollEvent()) {
				if (ev->is<sf::Event::Closed>()) { window_.close(); return; }
				handleEvent(*ev);
			}
			float dt = clock.restart().asSeconds();
			update(dt);
			render();
		}
	}

} // namespace ark