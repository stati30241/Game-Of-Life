/*
* Conway's Game of Life
* Conway's Game of life is a cellular automaton by the mathematicatian
* John Conway. This program lets the user edit the inital state and 
* then plays the game of life based on the seed provided.
* 
* Instructions:
*	Use the left mouse button to set cells as alive
*	Use the right mouse button to set cells as dead
*	Use the middle mouse button to pan around
*	Use the mouse wheel to zoom in and out
*	Press the space bar to set all cells as dead
*	Press the enter key to start doing the generations
* 
* Author:  stati30241
* Date:    June 6, 2021
* Youtube: https://www.youtube.com/channel/UC7Lx000LLDX6CU0qsyHGfQw
*/


#include <SFML/Graphics.hpp>

#include <array>
#include <bitset>


// Main class to handle the application
template <size_t gridSizeX, size_t gridSizeY>
class GameOfLife {
private:
	sf::RenderWindow* m_window;                         // The window that everything is rendered onto
	sf::Clock m_clock;                                  // Clock to keep track of the time elapsed between each frame

	const float m_tickSpeed = 0.1f;                     // The time between each generation
	float m_timer = 0.0f;                               // Timer to keep track of the time elapsed between each generation

	const sf::Vector2f m_viewSize = { 100.0f, 100.0f }; // The default size of the view
	sf::Vector2f m_oldMousePos;                         // The position of the mouse cursor in the last frame
	bool m_panning = false;                             // Is set to true when the user is panning

	const float m_zoomFactor = 1.10f;                   // The factor by which the user will be able to zoom in and out
	float m_totalZoom = 0.0f;                           // The total amount the user has zoomed in or out

	sf::Font m_font;                                    // The font used to render text onto the window
													    
	std::bitset<gridSizeX * gridSizeY>* m_cellMap;      // The main bitset containing the cell information
	std::bitset<gridSizeX * gridSizeY>* m_cellMapCopy;  // The copy of the main bitset to do generations
	bool m_startFlag = false;                           // Start flag to determine if we should start doing generations
	size_t m_numGenerations = 0;                        // Keeps track of the number of generations

private:
	// Zooms at a specific pixel in the window
	void zoomAtPixel(const sf::Vector2i& pixel, float zoom);
	// Pans the view according to the pan offset
	void panView(const sf::Vector2f& oldMousePos, const sf::Vector2f& newMousePos);

	// Calculates the position of the cell and sets it as dead or alive
	void setCell(const sf::Vector2i& mousePos, bool state);

	// Calculates and returns the neighbors for the given cell
	std::array<sf::Vector2u, 8> getNeighbors(const sf::Vector2u& cell);
	// Updates the cells based on the rules
	bool doGeneration();

	// Renders the cells onto the window
	void renderCells();
	// Renders the grid onto the window
	void renderGrid(const sf::Color& lineColor = sf::Color::Black);
	// Renders the text onto the window
	void renderText();

	// Handles the inputs from the user
	void handleInputs();
	// Updates the window
	void update();
	// Renders the entities onto the window
	void render();

public:
	// Constructor
	GameOfLife();
	// Destructor
	~GameOfLife();

	// Starts the application and cointains the main game
	// loop, executes all the other functions
	void run();
};


template <size_t gridSizeX, size_t gridSizeY>
GameOfLife<gridSizeX, gridSizeY>::GameOfLife()
	: m_window{ new sf::RenderWindow{ sf::VideoMode{ 800u, 800u }, "Conway's Game of Life", sf::Style::Close } },
	m_cellMap{ new std::bitset<gridSizeX * gridSizeY> }, m_cellMapCopy{ new std::bitset<gridSizeX * gridSizeY> } {
	// Load the font
	m_font.loadFromFile("cour.ttf");

	// Zoom in so only a part of the grid is visible
	sf::View view = { sf::Vector2f{ m_window->getSize() } / 2.0f, m_viewSize };
	m_totalZoom = m_viewSize.x / m_window->getSize().x;
	m_window->setView(view);
}


template <size_t gridSizeX, size_t gridSizeY>
GameOfLife<gridSizeX, gridSizeY>::~GameOfLife() {
	// Delete the pointers that were allocated
	delete m_window;
	delete m_cellMap;
	delete m_cellMapCopy;
}


template <size_t gridSizeX, size_t gridSizeY>
void GameOfLife<gridSizeX, gridSizeY>::zoomAtPixel(const sf::Vector2i& pixel, float zoom) {
	// Calculate the position of the pixel in world space before zooming
	const sf::Vector2f oldPixelPos = m_window->mapPixelToCoords(pixel);

	// Zoom the view and set it
	sf::View view = m_window->getView();
	view.zoom(zoom);
	m_window->setView(view);

	// Calculate the positon of the pixel in the world space after zooming
	// and the determine the offset
	const sf::Vector2f newPixelPos = m_window->mapPixelToCoords(pixel);
	const sf::Vector2f offset = oldPixelPos - newPixelPos;

	// Move the view according to the offset and set it
	view.move(offset);
	m_window->setView(view);
}


template <size_t gridSizeX, size_t gridSizeY>
void GameOfLife<gridSizeX, gridSizeY>::panView(const sf::Vector2f& oldMousePos, const sf::Vector2f& newMousePos) {
	// Calculate the offset
	const sf::Vector2f offset = oldMousePos - newMousePos;

	// Move the view according to the offset and set it
	sf::View view = m_window->getView();
	view.move(offset * m_totalZoom);
	m_window->setView(view);
}


template <size_t gridSizeX, size_t gridSizeY>
void GameOfLife<gridSizeX, gridSizeY>::setCell(const sf::Vector2i& mousePos, bool state) {
	// Calculate the position of the cell in the world space
	const sf::Vector2i cellPos = sf::Vector2i{ m_window->mapPixelToCoords(mousePos) } / 2;

	// Check if the position is inside the grid
	if (cellPos.x >= 0 && cellPos.x < gridSizeX &&
		cellPos.y >= 0 && cellPos.y < gridSizeY) {
		// Calculate the index of the bitset that corresponds to the cell's position
		const size_t cellIndex = cellPos.y * gridSizeX + cellPos.x;
		// Set the bit based on the cell's state
		m_cellMap->set(cellIndex, state);
	}
}


template <size_t gridSizeX, size_t gridSizeY>
std::array<sf::Vector2u, 8> GameOfLife<gridSizeX, gridSizeY>::getNeighbors(const sf::Vector2u& cell) {
	size_t im = (cell.x - 1) % gridSizeX;
	size_t ip = (cell.x + 1) % gridSizeX;
	size_t jm = (cell.y - 1) % gridSizeY;
	size_t jp = (cell.y + 1) % gridSizeY;

	std::array<sf::Vector2u, 8> neighbors = { sf::Vector2u{im, jm}, sf::Vector2u{im, cell.y}, sf::Vector2u{im, jp}, sf::Vector2u{cell.x, jm},
											sf::Vector2u{cell.x, jp}, sf::Vector2u{ip, jm}, sf::Vector2u{ip, cell.y}, sf::Vector2u{ip, jp} };
	return neighbors;
}


template <size_t gridSizeX, size_t gridSizeY>
bool GameOfLife<gridSizeX, gridSizeY>::doGeneration() {
	// Return if none of the cells are alive
	if (m_cellMap->none()) {
		m_numGenerations = 0;
		return false;
	}

	// Copy the contents of the cell map into it's copy
	memcpy(m_cellMapCopy, m_cellMap, gridSizeX * gridSizeY / 8);

	// Iterate through each cell to find it's number of neighbors
	for (size_t i = 0; i < gridSizeX; ++i) {
		for (size_t j = 0; j < gridSizeY; ++j) {
			// If the cell is dead, then go to the next one
			if (!m_cellMap->test(j * gridSizeX + i)) continue;

			// Counts the number of neighbors
			size_t numNeighbors = 0;
			std::array<sf::Vector2u, 8> neighbors = getNeighbors({ i, j });
			for (const sf::Vector2u& neighbor : neighbors) {
				
				if (m_cellMap->test(neighbor.y * gridSizeX + neighbor.x)) {
					// If the neighbor is alive, increment the number of neighbors
					numNeighbors++;
				} else {
					// If the neighbor is dead, check if it has 3 neighbors
					// and can be brought to life
					size_t numNeighborsDead = 0;
					std::array<sf::Vector2u, 8> neighborsDead = getNeighbors(neighbor);;
					// Count the number of neighbors of the neighbor
					for (const sf::Vector2u& neighborDead : neighborsDead) {
						if (m_cellMap->test(neighborDead.y * gridSizeX + neighborDead.x))
							numNeighborsDead++;
					}
					
					// If the neighbor has 3 neighbors, set it to alive
					if (numNeighborsDead == 3) m_cellMapCopy->set(neighbor.y * gridSizeX + neighbor.x);
				}
			}

			// Changes the state of the cell based on it's number of neighbors
			if (numNeighbors < 2) m_cellMapCopy->reset(j * gridSizeX + i);
			else if (numNeighbors > 3) m_cellMapCopy->reset(j * gridSizeX + i);
		}
	}

	// Copy the contents of cell map copy to the original cell map
	memcpy(m_cellMap, m_cellMapCopy, gridSizeX * gridSizeY / 8);

	// Incease the number of generations by one
	m_numGenerations++;

	return true;
}


template <size_t gridSizeX, size_t gridSizeY>
void GameOfLife<gridSizeX, gridSizeY>::renderCells() {
	// Calculates the size of each cell
	sf::Vector2f cellSize;
	cellSize.x = m_window->getSize().x / static_cast<float>(gridSizeX);
	cellSize.y = m_window->getSize().y / static_cast<float>(gridSizeY);

	// Iterates over each cell to draw the ones that are set alive
	for (size_t i = 0; i < m_cellMap->size(); ++i) {
		if (m_cellMap->test(i)) {
			// Creates a drawable rect for the cell
			sf::RectangleShape cellRect{ cellSize };
			cellRect.setPosition(i % gridSizeX * cellSize.x, i / gridSizeY * cellSize.y);
			m_window->draw(cellRect);
		}
	}
}


template <size_t gridSizeX, size_t gridSizeY>
void GameOfLife<gridSizeX, gridSizeY>::renderGrid(const sf::Color& lineColor) {
	// Calculate the size of each cell
	sf::Vector2f cellSize;
	cellSize.x = m_window->getSize().x / static_cast<float>(gridSizeX);
	cellSize.y = m_window->getSize().y / static_cast<float>(gridSizeY);

	// Draw the vertical lines
	for (float i = 0.0f; i <= m_window->getSize().x; i += cellSize.x) {
		sf::Vertex line[2] = { {{i, 0.0f}, lineColor},
							  {{i, static_cast<float>(m_window->getSize().y)}, lineColor} };
		m_window->draw(line, 2, sf::Lines);
	}

	// Draw the horizontal lines
	for (float j = 0.0f; j <= m_window->getSize().y; j += cellSize.y) {
		sf::Vertex line[2] = { {{0.0f, j}, lineColor},
							  {{static_cast<float>(m_window->getSize().x), j}, lineColor} };
		m_window->draw(line, 2, sf::Lines);
	}
}


template <size_t gridSizeX, size_t gridSizeY>
void GameOfLife<gridSizeX, gridSizeY>::renderText() {
	// Calculates the size and position of the text
	sf::Text generationText{ "Generation " + std::to_string(m_numGenerations), m_font, 30u };
	generationText.setPosition(m_window->mapPixelToCoords({ 5, 5 }));
	generationText.setScale(m_totalZoom, m_totalZoom);
	generationText.setFillColor(sf::Color::Yellow);

	// Renders the text
	m_window->draw(generationText);
}


template <size_t gridSizeX, size_t gridSizeY>
void GameOfLife<gridSizeX, gridSizeY>::handleInputs() {
	sf::Event sfmlEvent;
	while (m_window->pollEvent(sfmlEvent)) {
		switch (sfmlEvent.type) {

		// If the user clicked the X button
		case sf::Event::Closed:
			m_window->close();
			break;

		// If the user scrolled the mouse wheel
		case sf::Event::MouseWheelScrolled:
			if (sfmlEvent.mouseWheelScroll.delta > 0) {
				// Don't let the user zoom in too much
				if (m_totalZoom < 0.01f) break;
				// If the user scrolled up, zoom in
				zoomAtPixel(sf::Mouse::getPosition(*m_window), 1.0f / m_zoomFactor);
				m_totalZoom *= 1.0f / m_zoomFactor;
			} else if (sfmlEvent.mouseWheelScroll.delta < 0) {
				// Don't let the user zoom out too much
				if (m_totalZoom > 0.3f) break;
				// If the user scrolled down, zoom out
				zoomAtPixel(sf::Mouse::getPosition(*m_window), m_zoomFactor);
				m_totalZoom *= m_zoomFactor;
			}
			break;

		// If the user pressed a mouse button
		case sf::Event::MouseButtonPressed:
			// If the button released was the middle button, then set
			// m_panning to true and calculate the old mouse position
			if (sfmlEvent.mouseButton.button == sf::Mouse::Middle) {
				m_oldMousePos = sf::Vector2f{ sf::Mouse::getPosition(*m_window) };
				m_panning = true;
			}
			break;

		// If the user released a mouse button
		case sf::Event::MouseButtonReleased:
			// If the button released was the middle button, then set m_panning to false
			if (sfmlEvent.mouseButton.button == sf::Mouse::Middle) {
				m_panning = false;
			}
			break;

		// If the user moved the mouse
		case sf::Event::MouseMoved:
			// Check if the user in panning
			if (m_panning) {
				// If the user is panning calculate the new mouse position and pan
				const sf::Vector2f newMousePos = sf::Vector2f{ sf::Mouse::getPosition(*m_window) };
				panView(m_oldMousePos, newMousePos);
				m_oldMousePos = newMousePos;
			}
			break;

		// If the user pressed a key
		case sf::Event::KeyPressed:
			if (sfmlEvent.key.code == sf::Keyboard::Space) {
				// If the user pressed the space bar, clear the cell map
				m_cellMap->reset();
			} else if (sfmlEvent.key.code == sf::Keyboard::Enter) {
				// If the user pressed the enter key, start doing generations
				m_startFlag = !m_startFlag;
			}
			break;
		}
	}
}


template <size_t gridSizeX, size_t gridSizeY>
void GameOfLife<gridSizeX, gridSizeY>::update() {
	// Calculates the elapsed time between the current and last frame
	float deltaTime = m_clock.restart().asSeconds();

	// Handle the drawing and erasing of cells by the user
	if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
		// If the user is pressing the left mouse button, draw the cells
		setCell(sf::Mouse::getPosition(*m_window), true);
	} else if (sf::Mouse::isButtonPressed(sf::Mouse::Right)) {
		// If the user is pressing the right mouse button, erase the cells
		setCell(sf::Mouse::getPosition(*m_window), false);
	}

	// Increases the timer and checks if it should do generation
	if (m_startFlag) {
		m_timer += deltaTime;
		if (m_timer >= m_tickSpeed) {
			m_startFlag = doGeneration();
			m_timer = 0.0f;
		}
	}
}


template <size_t gridSizeX, size_t gridSizeY>
void GameOfLife<gridSizeX, gridSizeY>::render() {
	// Clears the window
	m_window->clear({ 50, 50, 50 });

	renderCells();
	renderGrid();
	if (m_startFlag) renderText();

	// Displays the current frame
	m_window->display();
}


template <size_t gridSizeX, size_t gridSizeY>
void GameOfLife<gridSizeX, gridSizeY>::run() {
	// Main game loop
	while (m_window->isOpen()) {
		handleInputs();
		update();
		render();
	}
}


// Main function
int main() {
	GameOfLife<400, 400> game;
	game.run();

	return 0;
}
