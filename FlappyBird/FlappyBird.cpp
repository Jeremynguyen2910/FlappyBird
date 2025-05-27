#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <SFML/System.hpp>
#include <iostream>
#include <optional>
#include <vector>
#include <cstdlib> 
#include <ctime>

bool checkCollision(const sf::Sprite& bird, const std::vector<sf::Sprite>& pipes, float windowHeight, float groundHeight) {
	sf::FloatRect birdBounds = bird.getGlobalBounds();

	// Va chạm với ống
	for (const auto& pipe : pipes) {
		if (birdBounds.findIntersection(pipe.getGlobalBounds())) {
			return true;
		}
	}

	// Va chạm mặt đất
	if (bird.getPosition().y + 24 / 2 >= windowHeight - groundHeight) {
		return true;
	}

	// Va chạm trần
	if (bird.getPosition().y - 24 / 2 <= 0) {
		return true;
	}

	return false;  // Không va chạm
}
void updateScoreSprites(int score, std::vector<sf::Sprite>& scoreSprites, sf::Texture digitTextures[10], float windowWidth) {
	scoreSprites.clear();
	std::string scoreStr = std::to_string(score);
	float totalWidth = 0.0f;

	// Tính tổng chiều rộng điểm số
	for (char c : scoreStr) {
		int digit = c - '0';
		sf::Vector2u size = digitTextures[digit].getSize();
		totalWidth += size.x;
	}

	float startX = (windowWidth - totalWidth) / 2.0f;
	for (char c : scoreStr) {
		int digit = c - '0';
		sf::Sprite digitSprite(digitTextures[digit]);
		digitSprite.setPosition({ startX, 80 });  // Vị trí y = 50 px từ trên xuống
		scoreSprites.push_back(digitSprite);
		startX += digitTextures[digit].getSize().x;
	}
}


int main() {
	const int windowWidth = 288;
	const int windowHeight = 512;
	float gap = 110.0f;
	bool gameOver = false;
	sf::Clock restartClock;
	bool canRestart = false;
	bool gameStarted = false;
	bool freezeRotation = false;
	int score = 0;

	// Load number
	sf::Texture digitTextures[10];
	std::vector<sf::Sprite> scoreSprites;
	for (int i = 0; i < 10; ++i) {
		std::string filename = "assets/" + std::to_string(i) + ".png";
		digitTextures[i].loadFromFile(filename);
	}


	sf::RenderWindow window(sf::VideoMode({ windowWidth, windowHeight }), "Flappy Bird");
	window.setFramerateLimit(60);

	// Lấy giờ hệ thống
	std::time_t now = std::time(nullptr);
	std::tm localTime;
	localtime_s(&localTime, &now); // nếu bạn dùng Linux thì dùng localtime()
	int currentHour = localTime.tm_hour;

	// Load background depend on time of day
	sf::Texture backgroundTexture;
	if (currentHour >= 6 && currentHour < 18) {
		if (!backgroundTexture.loadFromFile("assets/background.png")) {
			std::cerr << "Failed to load background.png\n";
			return -1;
		}
	}
	else {
		if (!backgroundTexture.loadFromFile("assets/background-night.png")) {
			std::cerr << "Failed to load background-night.png\n";
			return -1;
		}
	}

	sf::Sprite background(backgroundTexture);

	// Load bird
	sf::Texture birdTexture;
	if (!birdTexture.loadFromFile("assets/bird.png")) {
		std::cerr << "Failed to load bird.png\n";
		return -1;
	}
	sf::Texture birdTextureU;
	if (!birdTextureU.loadFromFile("assets/birdu.png")) {
		std::cerr << "Failed to load bird.png\n";
		return -1;
	}
	sf::Texture birdTextureD;
	if (!birdTextureD.loadFromFile("assets/birdd.png")) {
		std::cerr << "Failed to load bird.png\n";
		return -1;
	}

	std::vector<sf::Texture*> birdFrames = {
	&birdTexture,
	&birdTextureD,
	&birdTextureU
	};

	int birdFrameIndex = 0;
	float birdAnimationTimer = 0.0f;
	float birdAnimationInterval = 0.1f;


	sf::Sprite bird(*birdFrames[birdFrameIndex]);
	bird.setOrigin({ 17, 12 });  // Đặt origin ở giữa chim
	bird.setPosition({ windowWidth / 2.0f - 15, windowHeight / 2.0f - 50 });  // Đặt vị trí chim

	float birdVelocity = 0.0f;
	float gravity = 0.5f;
	const float jumpStrength = -8.0f;

	//load ground
	sf::Texture groundTexture;
	if (!groundTexture.loadFromFile("assets/ground.png")) {
		std::cerr << "Failed to load ground.png\n";
		return -1;
	}

	sf::Sprite ground1(groundTexture);
	sf::Sprite ground2(groundTexture);

	ground1.setPosition({ 0, windowHeight - 112 });
	ground2.setPosition({ 336, windowHeight - 112 });

	const float groundSpeed = 2.0f; // tốc độ cuộn mặt đất

	srand(static_cast<unsigned>(time(nullptr)));  // Khởi tạo random 1 lần
	// load pipe
	sf::Texture pipeTexture;
	if (!pipeTexture.loadFromFile("assets/pipe.png")) {
		std::cerr << "Failed to load pipe.png\n";
		return -1;
	}

	std::vector<sf::Sprite> pipes;
	std::vector<bool> passedPipes;

	float pipeSpawnTimer = 0.0f;
	sf::Clock clock;
	float idleTime = 0.0f; //float 


	//load game over texture
	sf::Texture gameOverTexture;
	if (!gameOverTexture.loadFromFile("assets/gameover.png")) {
		std::cerr << "Failed to load gameover.png\n";
		return -1;
	}
	sf::Sprite gameOverSprite(gameOverTexture);
	gameOverSprite.setOrigin({ gameOverTexture.getSize().x / 2.f, gameOverTexture.getSize().y / 2.f });
	gameOverSprite.setPosition({ windowWidth / 2.f, windowHeight / 2.f - 60 });

	while (window.isOpen()) {
		if (gameOver && restartClock.getElapsedTime().asSeconds() >= 1.0f) {
			canRestart = true;  // Cho phép bấm Space để chơi lại
		}
		float deltaTime = clock.restart().asSeconds();
		if (!gameOver) {
			birdAnimationTimer += deltaTime;
			if (birdAnimationTimer >= birdAnimationInterval) {
				birdAnimationTimer = 0.0f;
				birdFrameIndex = (birdFrameIndex + 1) % birdFrames.size();
				bird.setTexture(*birdFrames[birdFrameIndex]);
			}
		}

		pipeSpawnTimer += deltaTime;


		//Xử lí sự kiện
		while (const std::optional event = window.pollEvent()) {
			if (event->is<sf::Event::Closed>())
				window.close();
			if (event->is<sf::Event::KeyPressed>()) {
				if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space)) {
					if (gameOver && canRestart) {
						// Reset trạng thái game
						bird.setPosition({ windowWidth / 2.0f - 15, windowHeight / 2.0f - 50 });
						birdVelocity = 0.0f;
						gravity = 0.5f;  // Reset trọng lực

						bird.setRotation(sf::degrees(0));
						freezeRotation = false;


						pipes.clear();
						pipeSpawnTimer = 0.0f;
						score = 0;
						scoreSprites.clear();
						passedPipes.clear();  // << Thêm dòng này

						ground1.setPosition({ 0, windowHeight - 112 });
						ground2.setPosition({ 336, windowHeight - 112 });

						gameOver = false;
						canRestart = false;
						gameStarted = false;  // ← Quan trọng: chưa bắt đầu vội
					}
					else if (!gameStarted && !gameOver) {
						gameStarted = true;
						birdVelocity = jumpStrength;  // Bắt đầu chơi
					}
					else if (!gameOver) {
						birdVelocity = jumpStrength; // Chim nhảy lên bình thường
					}
				}
			}
		}

		if (gameStarted) {
			birdVelocity += gravity;
			bird.move({ 0, birdVelocity });
			if (!gameOver || !freezeRotation) {
				// Cập nhật góc xoay của chim
				float rawAngle;
				if (-8 <= birdVelocity && birdVelocity <= 0) {
					rawAngle = std::clamp(birdVelocity, -30.0f, -30.0f);
				}
				else if (0 <= birdVelocity && birdVelocity <= 8) {
					rawAngle = std::clamp(birdVelocity * 3.75f - 30.0f, -30.0f, 0.0f);
				}
				else {
					rawAngle = std::clamp(birdVelocity * 12.0f - 102.0f, 0.0f, 90.0f);
				}
				bird.setRotation(sf::degrees(rawAngle));
			}


			sf::FloatRect birdBounds = bird.getGlobalBounds();
			if (gameOver && bird.getPosition().y + 24 / 2 >= windowHeight - groundTexture.getSize().y) {
				float groundY = windowHeight - groundTexture.getSize().y;
				// Đặt chim nằm đúng trên mặt đất
				bird.setPosition({ bird.getPosition().x, groundY - 24 / 2 });
				birdVelocity = 0;
				gravity = 0;
				freezeRotation = true; // ✨ Giữ nguyên góc hiện tại

			}
			// Kiểm tra va chạm (gọi hàm bạn đã có)
			if (!gameOver) {
				if (pipeSpawnTimer >= 2.0f) {
					pipeSpawnTimer = 0.0f;

					float minHeight = 50.0f;
					float maxHeight = 250.0f;
					float topPipeHeight = minHeight + static_cast<float>(rand()) / RAND_MAX * (maxHeight - minHeight);

					//top pipe
					sf::Sprite topPipe(pipeTexture);
					topPipe.setOrigin({ 52 / 2.f, 320 });  // Đặt origin đáy giữa
					topPipe.setScale({ 1.0f, -1.0f });  // Lật ngược pipe
					topPipe.setPosition({ windowWidth + 26, topPipeHeight - pipeTexture.getSize().y });

					//bottom pipe
					sf::Sprite bottomPipe(pipeTexture);
					bottomPipe.setPosition({ windowWidth, topPipeHeight + gap });

					pipes.push_back(topPipe);
					pipes.push_back(bottomPipe);
					passedPipes.push_back(false);  // top pipe
					passedPipes.push_back(false);  // bottom pipe
				}

				for (auto& pipe : pipes) {
					pipe.move({ -100.0f * deltaTime, 0 }); // tốc độ di chuyển pipe
				}

				if (passedPipes.size() != pipes.size()) {
					passedPipes.resize(pipes.size(), false);
				}

				for (size_t i = 0; i < pipes.size(); ++i) {
					// Chỉ kiểm tra với ống dưới
					if (i % 2 == 1 && !passedPipes[i]) {
						if (pipes[i].getPosition().x + pipes[i].getGlobalBounds().size.x / 2.0f < bird.getPosition().x) {
							score++;
							passedPipes[i] = true;
						}
					}
				}


				for (int i = static_cast<int>(pipes.size()) - 1; i >= 0; --i) {
					if (pipes[i].getPosition().x + pipes[i].getGlobalBounds().size.x < 0) {
						pipes.erase(pipes.begin() + i);
						passedPipes.erase(passedPipes.begin() + i);
					}
				}

				if (checkCollision(bird, pipes, windowHeight, groundTexture.getSize().y)) {
					gameOver = true;
					restartClock.restart();  // Bắt đầu đếm thời gian delay
					canRestart = false;
				}

				// Cập nhật điểm số
				updateScoreSprites(score, scoreSprites, digitTextures, windowWidth);

			}

		}
		// Di chuyển mặt đất sang trái
		if (!gameOver) {
			ground1.move({ -groundSpeed, 0 });
			ground2.move({ -groundSpeed, 0 });



			// Nếu mặt đất ra khỏi màn hình thì đưa nó về phía sau sprite còn lại
			if (ground1.getPosition().x + groundTexture.getSize().x < 0)
				ground1.setPosition({ ground2.getPosition().x + groundTexture.getSize().x, ground1.getPosition().y });

			if (ground2.getPosition().x + groundTexture.getSize().x < 0)
				ground2.setPosition({ ground1.getPosition().x + groundTexture.getSize().x, ground2.getPosition().y });
		}

		// Vẽ
		window.clear();
		window.draw(background);

		for (const auto& pipe : pipes) {
			window.draw(pipe);
		}



		window.draw(bird);
		for (auto& sprite : scoreSprites) {
			window.draw(sprite);
		}
		if (gameOver) {
			window.draw(gameOverSprite);
		}
		window.draw(ground1);
		window.draw(ground2);

		window.display();
	}

	return 0;
}
