#include <SFML/Graphics.hpp>

int main() {
	// 1. 建立一個 800x600 的視窗，標題為 "SFML Test"
	sf::RenderWindow window(sf::VideoMode(800, 600), "SFML Test");

	// 2. 設定一個圓形形狀，半徑為 100
	sf::CircleShape shape(100.f);
	shape.setFillColor(sf::Color::Green); // 設定顏色為綠色

	// 將圓形稍微移動到視窗中央的感覺
	shape.setPosition(300.f, 200.f);

	// 3. 遊戲主迴圈
	while (window.isOpen()) {
		sf::Event event;

		// 檢查所有事件（如按下關閉按鈕）
		while (window.pollEvent(event)) {
			if (event.type == sf::Event::Closed)
				window.close();
		}

		// --- 繪製區 ---
		window.clear();          // 清除上一幀的畫面
		window.draw(shape);      // 畫出圓形
		window.display();        // 顯示到視窗上
	}

	return 0;
}