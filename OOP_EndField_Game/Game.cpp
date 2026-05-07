#include "Game.h"
#include <iostream>
#include <algorithm>
#include <cmath>

namespace ark {

Game::Game() {
    window_.create(sf::VideoMode({1280u, 800u}), "Originium Circuit Repair",
                   sf::Style::Close | sf::Style::Resize);
    window_.setFramerateLimit(60);
    loadFont();
    scanLevels();
    editorPartShape_.assign(editorPartH_, std::vector<uint8_t>(editorPartW_, 0));
}

void Game::loadFont() {
    // Try multiple font paths
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
        if (font_.openFromFile(p)) return;
    }
    std::cerr << "Warning: No font loaded\n";
}

void Game::scanLevels() {
    levelFiles_.clear();
    std::vector<std::string> dirs = {"Levels", ".", "../Levels"};
    for (auto& dir : dirs) {
        try {
            for (auto& entry : std::filesystem::directory_iterator(dir)) {
                if (entry.path().extension() == ".txt")
                    levelFiles_.push_back(entry.path().string());
            }
            if (!levelFiles_.empty()) break;
        } catch (...) {}
    }
    std::sort(levelFiles_.begin(), levelFiles_.end());
}

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
        computeLayout();
    } catch (std::exception& e) {
        statusMsg_ = std::string("Load error: ") + e.what();
        statusTimer_ = 3.f;
    }
}

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

void Game::computeLayout() {
    float maxBoardW = 650.f, maxBoardH = 600.f;
    float marginTop = 80.f;
    float csW = maxBoardW / (board_.cols() + 2);
    float csH = maxBoardH / (board_.rows() + 2);
    cellSize_ = std::min(csW, csH);
    cellSize_ = std::min(cellSize_, 70.f);
    boardOffX_ = 60.f + cellSize_; // leave room for row targets
    boardOffY_ = marginTop + cellSize_; // leave room for col targets
}

void Game::selectPart(int idx) {
    if (idx < 0 || idx >= (int)parts_.size()) return;
    if (placements_[idx].placed) {
        // Pick up from board
        board_.removePart(parts_[idx]);
        placements_[idx].placed = false;
        placedCount_--;
    }
    selectedPart_ = idx;
    dragging_ = true;
    idleTimer_ = 0.f;
}

void Game::deselectPart() {
    selectedPart_ = -1;
    dragging_ = false;
}

void Game::tryPlace() {
    if (selectedPart_ < 0) return;
    auto& p = parts_[selectedPart_];
    int ar = ghostRow_ - p.pivotRow();
    int ac = ghostCol_ - p.pivotCol();
    std::string err;
    if (board_.canPlace(p, ar, ac, &err)) {
        board_.placePart(p, ar, ac);
        placements_[selectedPart_] = {true, ar, ac, 0};
        placedCount_++;
        if (board_.checkWinCondition((int)parts_.size(), placedCount_)) {
            scene_ = Scene::Victory;
            // Print solution to console
            std::cout << "=== Solution ===" << std::endl;
            board_.printSolution();
        }
        deselectPart();
        idleTimer_ = 0.f;
    } else {
        statusMsg_ = err;
        statusTimer_ = 2.f;
    }
}

void Game::rotateCurrent() {
    if (selectedPart_ < 0) return;
    parts_[selectedPart_].rotateRight();
    rotating_ = true;
    rotAnimAngle_ = 90.f;
    idleTimer_ = 0.f;
}

void Game::solveInBackground() {
    if (solutionSearched_) return;
    solutionSearched_ = true;
    // Solve from initial state
    solutionFound_ = solver_.solve(initialBoard_, initialParts_, solution_);
    if (solutionFound_) {
        std::cout << "=== Auto-Solver Found Solution ===" << std::endl;
        // Print it
        Board temp = initialBoard_;
        std::vector<Part> tempParts = initialParts_;
        for (auto& sp : solution_) {
            Part p = tempParts[sp.partId].rotated(sp.rotation);
            temp.placePart(p, sp.anchorRow, sp.anchorCol);
        }
        temp.printSolution();
    } else {
        std::cout << "No solution found." << std::endl;
    }
}

void Game::showHint() {
    if (!solutionFound_ || solution_.empty()) return;
    // Find first unplaced part in solution
    for (auto& sp : solution_) {
        if (!placements_[sp.partId].placed) {
            hintPartIdx_ = sp.partId;
            showingHint_ = true;
            return;
        }
    }
}

sf::Vector2f Game::mousePos() const {
    auto mp = sf::Mouse::getPosition(window_);
    return sf::Vector2f(static_cast<float>(mp.x), static_cast<float>(mp.y));
}

bool Game::isMouseOver(float x, float y, float w, float h) const {
    auto m = mousePos();
    return m.x >= x && m.x <= x+w && m.y >= y && m.y <= y+h;
}

// ---- UPDATE ----
void Game::update(float dt) {
    if (statusTimer_ > 0) { statusTimer_ -= dt; if (statusTimer_ <= 0) statusMsg_.clear(); }
    if (rotating_) { rotAnimAngle_ -= dt * 500.f; if (rotAnimAngle_ <= 0) rotating_ = false; }

    switch(scene_) {
        case Scene::MainMenu:    updateMainMenu(dt); break;
        case Scene::LevelSelect: updateLevelSelect(dt); break;
        case Scene::Playing:     updatePlaying(dt); break;
        case Scene::Editor:      updateEditor(dt); break;
        case Scene::Victory:     updateVictory(dt); break;
    }
}

void Game::updateMainMenu(float) { }
void Game::updateLevelSelect(float) { }
void Game::updatePlaying(float dt) {
    // Update ghost position
    if (selectedPart_ >= 0) {
        auto m = mousePos();
        ghostRow_ = (int)std::floor((m.y - boardOffY_) / cellSize_);
        ghostCol_ = (int)std::floor((m.x - boardOffX_) / cellSize_);
    }
    // Idle timer for hint
    idleTimer_ += dt;
    if (idleTimer_ > 30.f && !hintAvailable_) {
        hintAvailable_ = true;
        if (!solutionSearched_) solveInBackground();
    }
}
void Game::updateEditor(float) { }
void Game::updateVictory(float) { }

// ---- EVENTS ----
void Game::handleEvent(const sf::Event& ev) {
    switch(scene_) {
        case Scene::MainMenu:    handleMainMenuEvent(ev); break;
        case Scene::LevelSelect: handleLevelSelectEvent(ev); break;
        case Scene::Playing:     handlePlayingEvent(ev); break;
        case Scene::Editor:      handleEditorEvent(ev); break;
        case Scene::Victory:     handleVictoryEvent(ev); break;
    }
}

void Game::handleMainMenuEvent(const sf::Event& ev) {
    if (auto* mp = ev.getIf<sf::Event::MouseButtonPressed>()) {
        if (mp->button == sf::Mouse::Button::Left) {
            float cx = 640.f, bw = 300.f, bh = 55.f;
            if (isMouseOver(cx-bw/2, 300, bw, bh)) scene_ = Scene::LevelSelect;
            else if (isMouseOver(cx-bw/2, 380, bw, bh)) {
                scene_ = Scene::Editor;
                if (editorBoard_.rows() == 0) editorBoard_ = Board(editorRows_, editorCols_, editorColors_);
            }
            else if (isMouseOver(cx-bw/2, 460, bw, bh)) window_.close();
        }
    }
}

void Game::handleLevelSelectEvent(const sf::Event& ev) {
    if (auto* sc = ev.getIf<sf::Event::MouseWheelScrolled>()) {
        float maxScroll = std::max(0.f, levelFiles_.size() * 60.f - 600.f);
        levelScrollY_ += sc->delta * 30.f;
        if (levelScrollY_ > 0) levelScrollY_ = 0;
        if (levelScrollY_ < -maxScroll) levelScrollY_ = -maxScroll;
    }
    if (auto* mp = ev.getIf<sf::Event::MouseButtonPressed>()) {
        if (mp->button == sf::Mouse::Button::Left) {
            // Back button
            if (isMouseOver(40, 380, 100, 40)) { scene_ = Scene::MainMenu; return; }
            // Level buttons
            for (int i = 0; i < (int)levelFiles_.size(); ++i) {
                float y = 100.f + i * 60.f + levelScrollY_;
                if (isMouseOver(390, y, 500, 50)) {
                    loadLevel(levelFiles_[i]);
                    return;
                }
            }
        }
    }
}

void Game::handlePlayingEvent(const sf::Event& ev) {
    if (auto* kp = ev.getIf<sf::Event::KeyPressed>()) {
        if (selectedPart_ >= 0) {
            if (kp->code == sf::Keyboard::Key::W) ghostRow_--;
            if (kp->code == sf::Keyboard::Key::S) ghostRow_++;
            if (kp->code == sf::Keyboard::Key::A) ghostCol_--;
            if (kp->code == sf::Keyboard::Key::D) ghostCol_++;
            if (kp->code == sf::Keyboard::Key::R) rotateCurrent();
            if (kp->code == sf::Keyboard::Key::Enter) tryPlace();
            if (kp->code == sf::Keyboard::Key::Escape) deselectPart();
            idleTimer_ = 0.f;
        }
        if (kp->code == sf::Keyboard::Key::F5) resetLevel();
        if (kp->code == sf::Keyboard::Key::F1) {
            if (!solutionSearched_) solveInBackground();
            if (solutionFound_) showHint();
        }
    }
    if (auto* mp = ev.getIf<sf::Event::MouseButtonPressed>()) {
        if (mp->button == sf::Mouse::Button::Left) {
            auto m = mousePos();
            // Check part palette (right side)
            float palX = boardOffX_ + board_.cols() * cellSize_ + 60.f;
            for (int i = 0; i < (int)parts_.size(); ++i) {
                float py = 100.f + i * 65.f;
                if (m.x >= palX && m.x <= palX + 200 && m.y >= py && m.y <= py + 60) {
                    selectPart(i);
                    return;
                }
            }
            // Check board click - if part selected, try place
            if (selectedPart_ >= 0) {
                tryPlace();
            } else {
                // Click on board to pick up placed part
                int cr = (int)std::floor((m.y - boardOffY_) / cellSize_);
                int cc = (int)std::floor((m.x - boardOffX_) / cellSize_);
                if (board_.inBounds(cr, cc) && board_.cellType(cr, cc) >= 0) {
                    int pid = board_.cellType(cr, cc);
                    selectPart(pid);
                }
            }
            // Buttons
            float btnY = boardOffY_ + board_.rows() * cellSize_ + 40.f;
            if (isMouseOver(boardOffX_, btnY, 140, 50)) resetLevel();
            if (isMouseOver(boardOffX_+160, btnY, 140, 50)) {
                if (!solutionSearched_) solveInBackground();
                if (solutionFound_) {
                    // Auto place all
                    resetLevel();
                    for (auto& sp : solution_) {
                        Part p = parts_[sp.partId];
                        for (int r = 0; r < sp.rotation; r++) p.rotateRight();
                        parts_[sp.partId] = p;
                        board_.placePart(p, sp.anchorRow, sp.anchorCol);
                        placements_[sp.partId] = {true, sp.anchorRow, sp.anchorCol, sp.rotation};
                        placedCount_++;
                    }
                    if (board_.checkWinCondition((int)parts_.size(), placedCount_))
                        scene_ = Scene::Victory;
                }
            }
            if (hintAvailable_ && isMouseOver(boardOffX_+320, btnY, 140, 50)) {
                if (!solutionSearched_) solveInBackground();
                showHint();
            }
            if (isMouseOver(boardOffX_+480, btnY, 140, 50)) {
                scene_ = Scene::LevelSelect;
            }
        }
        if (mp->button == sf::Mouse::Button::Right) {
            if (selectedPart_ >= 0) deselectPart();
        }
    }
}

void Game::handleEditorEvent(const sf::Event& ev) {
    if (auto* kp = ev.getIf<sf::Event::KeyPressed>()) {
        if (kp->code == sf::Keyboard::Key::Escape) scene_ = Scene::MainMenu;
    }
    if (auto* sc = ev.getIf<sf::Event::MouseWheelScrolled>()) {
        auto m = mousePos();
        float eox = 60.f, ecs = 50.f;
        float pcx = eox + std::max(editorCols_, 5) * ecs + 100.f;
        if (pcx < 650.f) pcx = 650.f;
        float partsStartY = 300.f + editorPartH_*40.f + 80.f;
        if (m.x >= pcx && m.y >= partsStartY) {
            editorScrollY_ += sc->delta * 30.f; // wheel scrolling sensitivity
            if (editorScrollY_ > 0.f) editorScrollY_ = 0.f;

            // Calculate roughly the max required height to prevent endless scroll
            int iconsPerRow = std::max(1, (int)((1280.f - pcx) / 60.f));
            int maxRows = std::ceil((float)editorParts_.size() / iconsPerRow);
            float maxScroll = -std::max(0.f, (maxRows * 60.f) - (800.f - partsStartY) + 80.f);
            if (editorScrollY_ < maxScroll) editorScrollY_ = maxScroll;
        }
    }
    if (auto* mp = ev.getIf<sf::Event::MouseButtonPressed>()) {
        if (mp->button == sf::Mouse::Button::Left) {
            auto m = mousePos();
            // Tool buttons at top
            float tx = 20.f;
            if (isMouseOver(tx, 60, 110, 40)) { editorTool_ = 0; } tx += 120;
            if (isMouseOver(tx, 60, 110, 40)) { editorTool_ = 1; } tx += 120;
            for (int c = 0; c < editorColors_; c++) {
                if (isMouseOver(tx, 60, 110, 40)) { editorTool_ = 2 + c; }
                tx += 120;
            }
            
            // Size controls
            float cy = 120.f;
            int oldR = editorRows_, oldC = editorCols_, oldCol = editorColors_;
            if (isMouseOver(120, cy, 40, 40)) { editorRows_ = std::max(2, editorRows_-1); }
            if (isMouseOver(170, cy, 40, 40)) { editorRows_ = std::min(10, editorRows_+1); }
            if (isMouseOver(350, cy, 40, 40)) { editorCols_ = std::max(2, editorCols_-1); }
            if (isMouseOver(400, cy, 40, 40)) { editorCols_ = std::min(10, editorCols_+1); }
            if (isMouseOver(580, cy, 40, 40)) { editorColors_ = std::max(1, editorColors_-1); }
            if (isMouseOver(630, cy, 40, 40)) { editorColors_ = std::min(4, editorColors_+1); }

            if (oldR != editorRows_ || oldC != editorCols_ || oldCol != editorColors_ || editorBoard_.rows() == 0) {
                Board nb(editorRows_, editorCols_, editorColors_);
                if (editorBoard_.rows() > 0) {
                    for(int r = 0; r < std::min(editorBoard_.rows(), editorRows_); ++r) {
                        for(int c = 0; c < std::min(editorBoard_.cols(), editorCols_); ++c) {
                            int t = editorBoard_.cellType(r,c);
                            if(t == -1) nb.setEmptyCell(r,c);
                            else if(t == -2) nb.setBlockedCell(r,c);
                            else if(t == -3) {
                                int cCol = editorBoard_.cellColor(r,c);
                                if(cCol < editorColors_) nb.setFixedCell(r, c, cCol);
                            }
                        }
                    }
                }
                editorBoard_ = nb;
                editorPartColor_ = std::min(editorPartColor_, editorColors_ - 1);

                // Clear parts that now have invalid colors
                auto it = std::remove_if(editorParts_.begin(), editorParts_.end(), [this](const Part& p) {
                    return p.colorIndex() >= editorColors_;
                });
                editorParts_.erase(it, editorParts_.end());
            }
            
            // Board grid click
            float eox = 60.f, eoy = 190.f, ecs = 50.f;
            int cr = (int)((m.y - eoy) / ecs);
            int cc = (int)((m.x - eox) / ecs);
            if (m.y >= eoy && m.x >= eox && cr >= 0 && cr < editorRows_ && cc >= 0 && cc < editorCols_) {
                if (editorTool_ == 0) {
                    editorBoard_.setEmptyCell(cr, cc);
                } else if (editorTool_ == 1) {
                    editorBoard_.setBlockedCell(cr, cc);
                } else {
                    editorBoard_.setFixedCell(cr, cc, editorTool_ - 2);
                }
            }
            
            // Part creation area
            float pcx = eox + std::max(editorCols_, 5) * ecs + 100.f;
            if (pcx < 650.f) pcx = 650.f;
            
            // Part shape grid
            float psx = pcx, psgy = 300.f;
            int pr = (int)((m.y - psgy) / 40.f);
            int pc = (int)((m.x - psx) / 40.f);
            if (m.y >= psgy && m.x >= psx && pr >= 0 && pr < editorPartH_ && pc >= 0 && pc < editorPartW_) {
                if (pr < (int)editorPartShape_.size() && pc < (int)editorPartShape_[0].size())
                    editorPartShape_[pr][pc] ^= 1;
            }
            
            // Add part button
            if (isMouseOver(pcx, psgy + editorPartH_ * 40.f + 20, 140, 45)) {
                bool hasCell = false;
                for (auto& row : editorPartShape_)
                    for (auto v : row) if (v) hasCell = true;
                if (hasCell) {
                    editorParts_.emplace_back((int)editorParts_.size(), editorPartColor_, editorPartShape_);
                    editorPartShape_.assign(editorPartH_, std::vector<uint8_t>(editorPartW_, 0));
                }
            }
            
            // Part size +/-
            float psy = 240.f;
            if (isMouseOver(pcx+50, psy, 35, 35)) editorPartW_ = std::max(1, editorPartW_-1);
            if (isMouseOver(pcx+90, psy, 35, 35)) editorPartW_ = std::min(6, editorPartW_+1);
            if (isMouseOver(pcx+190, psy, 35, 35)) editorPartH_ = std::max(1, editorPartH_-1);
            if (isMouseOver(pcx+230, psy, 35, 35)) editorPartH_ = std::min(6, editorPartH_+1);
            editorPartShape_.resize(editorPartH_);
            for (auto& r : editorPartShape_) r.resize(editorPartW_, 0); // Resize shape immediately
            
            // Part color
            if (isMouseOver(pcx+380, psy, 110, 35)) editorPartColor_ = (editorPartColor_+1) % editorColors_;
            
            // Bottom Buttons
            float btnY2 = eoy + std::max(editorRows_, 5) * ecs + 50.f;
            if (btnY2 < psgy + editorPartH_*40 + 100.f) btnY2 = psgy + editorPartH_*40 + 100.f;
            
            // Export button
            if (isMouseOver(eox, btnY2, 140, 50)) {
                try {
                    exportLevel("Levels/custom.txt", editorBoard_, editorParts_);
                    statusMsg_ = "Exported to Levels/custom.txt";
                    statusTimer_ = 3.f;
                    scanLevels();
                } catch (std::exception& e) {
                    statusMsg_ = e.what(); statusTimer_ = 3.f;
                }
            }
            // Test Play button
            if (isMouseOver(eox+160, btnY2, 140, 50)) {
                try {
                    exportLevel("Levels/custom.txt", editorBoard_, editorParts_);
                    scanLevels();
                    loadLevel("Levels/custom.txt");
                } catch (std::exception& e) {
                    statusMsg_ = e.what(); statusTimer_ = 3.f;
                }
            }
            // Back button
            if (isMouseOver(eox+320, btnY2, 140, 50)) scene_ = Scene::MainMenu;
        }
    }
}

void Game::handleVictoryEvent(const sf::Event& ev) {
    if (auto* mp = ev.getIf<sf::Event::MouseButtonPressed>()) {
        if (mp->button == sf::Mouse::Button::Left) {
            if (isMouseOver(540, 450, 200, 50)) scene_ = Scene::LevelSelect;
            if (isMouseOver(540, 520, 200, 50)) scene_ = Scene::MainMenu;
        }
    }
}

// ---- MAIN LOOP ----
void Game::run() {
    // 如果有命令列指定的關卡路徑，直接載入
    if (!startLevelPath_.empty()) {
        loadLevel(startLevelPath_);
    }
    sf::Clock clock;
    while (window_.isOpen()) {
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


