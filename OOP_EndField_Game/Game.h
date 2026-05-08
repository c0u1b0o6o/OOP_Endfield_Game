#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include "Board.h"
#include "Part.h"
#include "LevelParser.h"
#include "AutoSolver.h"
#include <vector>
#include <string>
#include <memory>
#include <functional>
#include <filesystem>
#include <optional>

namespace ark {

// ---- 遊戲狀態列舉 ----
enum class Scene { MainMenu, LevelSelect, Playing, Editor, Victory };

// ---- 顏色方案 ----
namespace Colors {
    inline sf::Color bg()         { return sf::Color(18, 18, 24); }
    inline sf::Color panel()      { return sf::Color(28, 30, 40); }
    inline sf::Color grid()       { return sf::Color(40, 44, 58); }
    inline sf::Color gridLine()   { return sf::Color(60, 65, 80); }
    inline sf::Color blocked()    { return sf::Color(55, 55, 65); }
    inline sf::Color fixed()      { return sf::Color(180, 160, 60); }
    inline sf::Color text()       { return sf::Color(220, 225, 240); }
    inline sf::Color accent()     { return sf::Color(80, 200, 120); }
    inline sf::Color accentHover(){ return sf::Color(100, 230, 140); }
    inline sf::Color error()      { return sf::Color(220, 60, 60); }
    inline sf::Color hintColor()  { return sf::Color(80, 200, 120, 100); }
    inline sf::Color partColor(int colorIdx) {
        switch(colorIdx % 6) {
            case 0: return sf::Color(80, 200, 120);   // 綠
            case 1: return sf::Color(80, 140, 220);   // 藍
            case 2: return sf::Color(220, 100, 80);   // 紅
            case 3: return sf::Color(200, 180, 60);   // 黃
            case 4: return sf::Color(180, 100, 220);  // 紫
            default:return sf::Color(220, 140, 60);   // 橘
        }
    }
    inline sf::Color partGhost(int colorIdx) {
        auto c = partColor(colorIdx);
        c.a = 120;
        return c;
    }
}

// ---- 放置資訊 ----
struct PlacementInfo {
    bool placed = false;
    int anchorRow = 0, anchorCol = 0;
    int rotation = 0;
};

// ---- 主遊戲類別 ----
class Game {
public:
    Game();
    void run();
    void setStartLevel(const std::string& path) { startLevelPath_ = path; }

private:
    // ---- 視窗 ----
    sf::RenderWindow window_;
    sf::Font font_;
    float cellSize_ = 50.f;
    float boardOffX_ = 0.f, boardOffY_ = 0.f;

    // ---- 音效 ----
    sf::SoundBuffer sbPlace_, sbError_, sbPick_, sbRotate_, sbWin_, sbClick_;
    std::optional<sf::Sound> sndPlace_, sndError_, sndPick_, sndRotate_, sndWin_, sndClick_;
    std::optional<sf::Music> bgm_;

    // ---- 場景 ----
    Scene scene_ = Scene::MainMenu;
    std::string startLevelPath_;

    // ---- 關卡 ----
    Board board_;
    Board initialBoard_;
    std::vector<Part> parts_;
    std::vector<Part> initialParts_;
    std::vector<PlacementInfo> placements_;
    int placedCount_ = 0;
    std::string currentLevelPath_;
    std::vector<std::string> levelFiles_;

    // ---- 操作狀態 ----
    int selectedPart_ = -1;
    int ghostRow_ = 0, ghostCol_ = 0;
    bool dragging_ = false;
    std::string statusMsg_;
    float statusTimer_ = 0.f;

    // ---- 提示 / 解題 ----
    AutoSolver solver_;
    std::vector<SolverPlacement> solution_;
    bool solutionFound_ = false;
    bool solutionSearched_ = false;
    float idleTimer_ = 0.f;
    bool hintAvailable_ = false;
    bool showingHint_ = false;
    bool showingNoSolution_ = false;
    int hintPartIdx_ = -1;
    bool testingCustomLevel_ = false;

    // ---- 動畫 ----
    float rotAnimAngle_ = 0.f;
    bool rotating_ = false;

    // ---- 關卡選擇 UI ----
    int menuHover_ = -1;
    int levelHover_ = -1;
    float levelScrollY_ = 0.f;

    // ---- Editor ----
    int editorRows_ = 5, editorCols_ = 5, editorColors_ = 1;
    Board editorBoard_;
    std::vector<Part> editorParts_;
    int editorTool_ = 0; // 0=empty,1=blocked,2+=fixed color
    int editorPartColor_ = 0;
    int editorTargetColor_ = 0;
    int editorPartW_ = 2, editorPartH_ = 2;
    Shape editorPartShape_;
    bool editorDrawingPart_ = false;
    float editorScrollY_ = 0.f;

    // ---- 方法 ----
    void loadFont();
    void loadSounds();
    void scanLevels();
    void loadLevel(const std::string& path);
    void resetLevel();
    void computeLayout();

    // 遊戲邏輯
    void selectPart(int idx);
    void deselectPart();
    void tryPlace();
    void pickupPart(int partId);
    void rotateCurrent();
    void solveInBackground();
    void showHint();

    // 更新
    void update(float dt);
    void updateMainMenu(float dt);
    void updateLevelSelect(float dt);
    void updatePlaying(float dt);
    void updateEditor(float dt);
    void updateVictory(float dt);

    // 事件
    void handleEvent(const sf::Event& ev);
    void handleMainMenuEvent(const sf::Event& ev);
    void handleLevelSelectEvent(const sf::Event& ev);
    void handlePlayingEvent(const sf::Event& ev);
    void handleEditorEvent(const sf::Event& ev);
    void handleVictoryEvent(const sf::Event& ev);

    // 渲染
    void render();
    void renderMainMenu();
    void renderLevelSelect();
    void renderPlaying();
    void renderEditor();
    void renderVictory();
    void renderBoard(const Board& board, float ox, float oy, float cs);
    void renderParts(float ox, float oy, float cs);
    void renderGhost(float ox, float oy, float cs);
    void renderHints(float ox, float oy, float cs);
    void renderTargets(const Board& board, float ox, float oy, float cs);
    void renderPartPalette();
    void renderButton(float x, float y, float w, float h,
                      const std::string& label, bool hover);
    void renderColorButton(float x, float y, float w, float h,
                           const std::string& label, bool hover, sf::Color bgColor);
    bool isMouseOver(float x, float y, float w, float h) const;

    sf::Vector2f mousePos() const;
};

} // namespace ark

