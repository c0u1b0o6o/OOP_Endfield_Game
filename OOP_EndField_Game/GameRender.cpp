#include "Game.h"
#include <cmath>
#include <sstream>

namespace ark {

void Game::renderButton(float x, float y, float w, float h, const std::string& label, bool hover) {
    sf::RectangleShape bg(sf::Vector2f(w, h));
    bg.setPosition(sf::Vector2f(x, y));
    bg.setFillColor(hover ? Colors::accentHover() : Colors::accent());
    bg.setOutlineColor(sf::Color(255,255,255,40));
    bg.setOutlineThickness(1.f);
    window_.draw(bg);

    sf::Text txt(font_, label, (unsigned int)(h * 0.4f));
    txt.setFillColor(sf::Color(20, 20, 30));
    auto bounds = txt.getLocalBounds();
    txt.setPosition(sf::Vector2f(x + (w - bounds.size.x)/2 - bounds.position.x,
                                  y + (h - bounds.size.y)/2 - bounds.position.y));
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
                line1.setPosition(sf::Vector2f(ox + c*cs + cs/2, oy + r*cs + cs/2));
                line1.setRotation(sf::degrees(45.f));
                line1.setFillColor(sf::Color(100, 100, 110));
                window_.draw(line1);
                line1.setRotation(sf::degrees(-45.f));
                window_.draw(line1);
                continue;
            } else if (t == cell::FIXED) {
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
                eq.setPosition(sf::Vector2f(ox + c*cs + (cs-eb.size.x)/2 - eb.position.x,
                                             oy + r*cs + (cs-eb.size.y)/2 - eb.position.y));
                window_.draw(eq);
                continue;
            } else if (t == cell::EMPTY) {
                cell.setFillColor(sf::Color(30, 33, 45));
            } else {
                // Player part
                int clr = board.cellColor(r, c);
                cell.setFillColor(Colors::partColor(clr));
                cell.setOutlineColor(sf::Color(255,255,255,30));
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
            txt.setPosition(sf::Vector2f(tx + (cs/CC - b.size.x)/2,
                                          ty + (cs - b.size.y)/2 - b.position.y));
            window_.draw(txt);

            // Current count (smaller, below)
            sf::Text cur(font_, std::to_string(current), (unsigned int)(fontSize * 0.7f));
            cur.setFillColor(sf::Color(col.r, col.g, col.b, 120));
            auto cb = cur.getLocalBounds();
            cur.setPosition(sf::Vector2f(tx + (cs/CC - cb.size.x)/2,
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
            txt.setPosition(sf::Vector2f(tx + (cs - b.size.x)/2 - b.position.x,
                                          ty2 + (cs/CC - b.size.y)/2 - b.position.y));
            window_.draw(txt);

            sf::Text cur(font_, std::to_string(current), (unsigned int)(fontSize * 0.7f));
            cur.setFillColor(sf::Color(col.r, col.g, col.b, 120));
            auto cb = cur.getLocalBounds();
            cur.setPosition(sf::Vector2f(tx + cs * 0.65f,
                                          ty2 + (cs/CC - cb.size.y)/2 - cb.position.y));
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
        bg.setOutlineColor(isSelected ? Colors::accent() : sf::Color(50,55,70));
        bg.setOutlineThickness(isSelected ? 2.f : 1.f);
        window_.draw(bg);

        // Mini preview
        auto& p = parts_[i];
        float miniCs = std::min(50.f / p.height(), 50.f / p.width());
        miniCs = std::min(miniCs, 12.f);
        for (int r = 0; r < p.height(); ++r) {
            for (int c = 0; c < p.width(); ++c) {
                if (!p.shape()[r][c]) continue;
                sf::RectangleShape mc(sf::Vector2f(miniCs-1, miniCs-1));
                mc.setPosition(sf::Vector2f(palX + 8 + c * miniCs, py + 4 + r * miniCs));
                mc.setFillColor(isPlaced ? sf::Color(60,65,75) : Colors::partColor(p.colorIndex()));
                window_.draw(mc);
            }
        }

        // Label
        std::string label = "#" + std::to_string(i);
        if (isPlaced) label += " [placed]";
        sf::Text txt(font_, label, 13);
        txt.setFillColor(isPlaced ? sf::Color(100,105,120) : Colors::text());
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
    title.setPosition(sf::Vector2f(640 - tb.size.x/2, 140));
    window_.draw(title);

    sf::Text sub(font_, "Arknights: Endfield Puzzle", 18);
    sub.setFillColor(sf::Color(150, 155, 170));
    auto sb = sub.getLocalBounds();
    sub.setPosition(sf::Vector2f(640 - sb.size.x/2, 200));
    window_.draw(sub);

    float cx = 640.f, bw = 300.f, bh = 55.f;
    renderButton(cx-bw/2, 300, bw, bh, "Start Game", isMouseOver(cx-bw/2,300,bw,bh));
    renderButton(cx-bw/2, 380, bw, bh, "Level Editor", isMouseOver(cx-bw/2,380,bw,bh));
    renderButton(cx-bw/2, 460, bw, bh, "Quit", isMouseOver(cx-bw/2,460,bw,bh));

    // Decorative line
    sf::RectangleShape line(sf::Vector2f(400, 2));
    line.setPosition(sf::Vector2f(440, 260));
    line.setFillColor(sf::Color(60, 65, 80));
    window_.draw(line);
}

void Game::renderLevelSelect() {
    sf::Text title(font_, "Select Level", 32);
    title.setFillColor(Colors::accent());
    title.setPosition(sf::Vector2f(200, 30));
    window_.draw(title);

    renderButton(20, 20, 100, 40, "Back", isMouseOver(20,20,100,40));

    if (levelFiles_.empty()) {
        sf::Text none(font_, "No .txt files found in Levels/ folder", 18);
        none.setFillColor(Colors::error());
        none.setPosition(sf::Vector2f(200, 150));
        window_.draw(none);
        return;
    }

    for (int i = 0; i < (int)levelFiles_.size(); ++i) {
        float y = 100.f + i * 60.f;
        bool hov = isMouseOver(200, y, 500, 50);
        sf::RectangleShape bg(sf::Vector2f(500, 50));
        bg.setPosition(sf::Vector2f(200, y));
        bg.setFillColor(hov ? sf::Color(45, 55, 65) : Colors::panel());
        bg.setOutlineColor(hov ? Colors::accent() : sf::Color(50,55,70));
        bg.setOutlineThickness(1.f);
        window_.draw(bg);

        auto path = std::filesystem::path(levelFiles_[i]);
        sf::Text txt(font_, path.filename().string(), 18);
        txt.setFillColor(hov ? Colors::accentHover() : Colors::text());
        txt.setPosition(sf::Vector2f(220, y + 14));
        window_.draw(txt);
    }
}

void Game::renderPlaying() {
    // Header
    auto path = std::filesystem::path(currentLevelPath_);
    sf::Text header(font_, "Level: " + path.filename().string(), 20);
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
    float btnY = boardOffY_ + board_.rows() * cellSize_ + 30.f;
    renderButton(boardOffX_, btnY, 120, 40, "Reset", isMouseOver(boardOffX_, btnY, 120, 40));
    renderButton(boardOffX_+140, btnY, 120, 40, "Solve", isMouseOver(boardOffX_+140, btnY, 120, 40));

    if (hintAvailable_) {
        renderButton(boardOffX_+280, btnY, 120, 40, "Hint",
                     isMouseOver(boardOffX_+280, btnY, 120, 40));
    } else {
        sf::RectangleShape disBtn(sf::Vector2f(120, 40));
        disBtn.setPosition(sf::Vector2f(boardOffX_+280, btnY));
        disBtn.setFillColor(sf::Color(50, 55, 65));
        window_.draw(disBtn);
        sf::Text ht(font_, "Hint (wait)", 14);
        ht.setFillColor(sf::Color(80,85,95));
        ht.setPosition(sf::Vector2f(boardOffX_+290, btnY+12));
        window_.draw(ht);
    }

    renderButton(boardOffX_+420, btnY, 120, 40, "Menu",
                 isMouseOver(boardOffX_+420, btnY, 120, 40));

    // Status message
    if (!statusMsg_.empty()) {
        sf::Text st(font_, statusMsg_, 16);
        st.setFillColor(Colors::error());
        st.setPosition(sf::Vector2f(boardOffX_, btnY + 50));
        window_.draw(st);
    }

    // Controls help
    sf::Text help(font_, "WASD:Move  R:Rotate  Enter/Click:Place  Esc:Deselect  F5:Reset  F1:Hint", 12);
    help.setFillColor(sf::Color(80, 85, 100));
    help.setPosition(sf::Vector2f(20, 775));
    window_.draw(help);
}

void Game::renderEditor() {
    sf::Text title(font_, "Level Editor", 28);
    title.setFillColor(Colors::accent());
    title.setPosition(sf::Vector2f(20, 5));
    window_.draw(title);

    // Tool buttons
    float tx = 20.f;
    renderButton(tx, 40, 80, 30, "Empty", editorTool_==0); tx+=90;
    renderButton(tx, 40, 80, 30, "Block(X)", editorTool_==1); tx+=90;
    for (int c = 0; c < editorColors_; c++) {
        std::string l = "Fixed C" + std::to_string(c);
        renderButton(tx, 40, 80, 30, l, editorTool_==2+c);
        tx += 90;
    }

    // Size controls
    float cy = 80.f;
    sf::Text rl(font_, "Rows:" + std::to_string(editorRows_), 14);
    rl.setFillColor(Colors::text()); rl.setPosition(sf::Vector2f(20, cy)); window_.draw(rl);
    renderButton(90, cy, 25, 20, "-", false); renderButton(120, cy, 25, 20, "+", false);

    sf::Text cl(font_, "Cols:" + std::to_string(editorCols_), 14);
    cl.setFillColor(Colors::text()); cl.setPosition(sf::Vector2f(160, cy)); window_.draw(cl);
    renderButton(230, cy, 25, 20, "-", false); renderButton(260, cy, 25, 20, "+", false);

    sf::Text ccl(font_, "Colors:" + std::to_string(editorColors_), 14);
    ccl.setFillColor(Colors::text()); ccl.setPosition(sf::Vector2f(300, cy)); window_.draw(ccl);
    renderButton(380, cy, 25, 20, "-", false); renderButton(410, cy, 25, 20, "+", false);
    renderButton(450, cy, 60, 20, "Apply", false);

    // Editor board
    float eox = 60.f, eoy = 120.f, ecs = 50.f;
    if (editorBoard_.rows() > 0)
        renderBoard(editorBoard_, eox, eoy, ecs);
    else {
        sf::Text hint(font_, "Click 'Apply' to create board", 16);
        hint.setFillColor(sf::Color(100,105,120));
        hint.setPosition(sf::Vector2f(eox, eoy+20));
        window_.draw(hint);
    }

    // Part creation panel
    float pcx = eox + std::max(editorCols_, 5) * ecs + 80.f;
    sf::Text pt(font_, "Create Part", 18);
    pt.setFillColor(Colors::accent());
    pt.setPosition(sf::Vector2f(pcx, 105));
    window_.draw(pt);

    sf::Text ps(font_, "W:" + std::to_string(editorPartW_) + " H:" + std::to_string(editorPartH_)
                + " Color:" + std::to_string(editorPartColor_), 13);
    ps.setFillColor(Colors::text()); ps.setPosition(sf::Vector2f(pcx, 130)); window_.draw(ps);

    // Part shape grid
    float psy = 160.f;
    for (int r = 0; r < editorPartH_; r++) {
        for (int c = 0; c < editorPartW_; c++) {
            sf::RectangleShape cell(sf::Vector2f(28, 28));
            cell.setPosition(sf::Vector2f(pcx + c*30, psy + r*30));
            bool filled = (r < (int)editorPartShape_.size() && c < (int)editorPartShape_[0].size()
                          && editorPartShape_[r][c]);
            cell.setFillColor(filled ? Colors::partColor(editorPartColor_) : sf::Color(40,44,55));
            cell.setOutlineColor(Colors::gridLine());
            cell.setOutlineThickness(1.f);
            window_.draw(cell);
        }
    }
    renderButton(pcx, psy + editorPartH_*30+10, 100, 30, "Add Part", false);

    // Parts list
    sf::Text pl(font_, "Parts: " + std::to_string(editorParts_.size()), 14);
    pl.setFillColor(Colors::text());
    pl.setPosition(sf::Vector2f(pcx, psy + editorPartH_*30+50));
    window_.draw(pl);

    // Bottom buttons
    float btnY2 = eoy + std::max(editorRows_, 5) * ecs + 30.f;
    renderButton(eox, btnY2, 120, 40, "Export", isMouseOver(eox, btnY2, 120, 40));
    renderButton(eox+140, btnY2, 120, 40, "Test Play", isMouseOver(eox+140, btnY2, 120, 40));
    renderButton(eox+280, btnY2, 120, 40, "Back", isMouseOver(eox+280, btnY2, 120, 40));

    if (!statusMsg_.empty()) {
        sf::Text st(font_, statusMsg_, 14);
        st.setFillColor(Colors::accent());
        st.setPosition(sf::Vector2f(eox, btnY2+50));
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
    congrats.setPosition(sf::Vector2f(640 - cb.size.x/2, 300));
    window_.draw(congrats);

    sf::Text sub(font_, "All parts placed correctly!", 22);
    sub.setFillColor(Colors::text());
    auto sb = sub.getLocalBounds();
    sub.setPosition(sf::Vector2f(640 - sb.size.x/2, 380));
    window_.draw(sub);

    renderButton(440, 450, 200, 50, "Next Level", isMouseOver(440,450,200,50));
    renderButton(440, 520, 200, 50, "Main Menu", isMouseOver(440,520,200,50));
}

void Game::render() {
    window_.clear(Colors::bg());
    switch(scene_) {
        case Scene::MainMenu:    renderMainMenu(); break;
        case Scene::LevelSelect: renderLevelSelect(); break;
        case Scene::Playing:     renderPlaying(); break;
        case Scene::Editor:      renderEditor(); break;
        case Scene::Victory:     renderVictory(); break;
    }
    window_.display();
}

} // namespace ark

