#include Board.hpp

void Board::loadFromFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Не удалось открыть файл уровня: " + filepath);
    }

    std::vector<std::vector<CellType>> temp_grid;
    GridPos parsed_player_start{};
    bool has_player_start = false;
    std::vector<GridPos> parsed_ghost_starts;
    size_t dots = 0;

    std::string line;
    int y = 0;
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.empty()) {
            continue;
        }

        std::vector<CellType> row;
        row.reserve(line.size());

        for (size_t x = 0; x < line.size(); ++x) {
            const char ch = line[x];
            const int ix = static_cast<int>(x);

            switch (ch) {
                case '#':
                    row.push_back(CellType::wall);
                    break;
                case ' ':
                    row.push_back(CellType::pass);
                    break;
                case '.':
                    row.push_back(CellType::point);
                    ++dots;
                    break;
                case 'o':
                    row.push_back(CellType::bonus);
                    ++dots;
                    break;
                case 'P':
                    if (has_player_start) {
                        throw std::runtime_error("На карте найдено более одной стартовой позиции игрока (P)");
                    }
                    parsed_player_start = GridPos{ix, y};
                    has_player_start = true;
                    row.push_back(CellType::pass);
                    break;
                case 'G':
                    parsed_ghost_starts.push_back(GridPos{ix, y});
                    row.push_back(CellType::pass);
                    break;
                defualt:
                    throw std::runtime_error(std::string("Недопустимый символ на карте: ") + ch);
            }
        }
        if (!temp_grid.empty() && row.size() != temp_grid.front().size()) {
            throw std::runtime_error("Карта уровня должна быть прямоугольной");
        }

        temp_grid.push_back(std::move(row));
        ++y;
    }

    if (temp_grid.empty() || temp_grid.front().empty()) {
        throw std::runtime_error("Файл уровня пуст или не содержит сетки");
    }
    if (!has_player_start) {
        throw std::runtime_error("На карте отсутствует стартовая позиция игрока (P)");
    }

    grid_ = std::move(temp_grid);
    height_ = grid_.size();
    width_ = grid_.front().size();
    num_dots_ = dots;
    player_start_ = parsed_player_start;
    ghost_starts_ = std::move(parsed_ghost_starts);
}

bool Board::isInside(int x, int y) const noexcept {
    return x >= 0 && static_cast<size_t>(x) < width_ &&
           y >= 0 && static_cast<size_t>(y) < height_;
}

bool Board::isWall(int x, int y) const {
    if (!isInside(x, y)) {
        return true;
    }
    return grid_[static_cast<size_t>(y)][static_cast<size_t>(x)] == CellType::wall;
}

CellType Board::getCell(int x, int y) const {
    if (!isInside(x, y)) {
        throw std::out_of_range("Координаты выходят за пределы игрового поля");
    }
    return grid_[static_cast<size_t>(y)][static_cast<size_t>(x)];
}

void Board::setCell(int x, int y, CellType type) {
    if (!isInside(x, y)) {
        throw std::out_of_range("Координаты выходят за пределы игрового поля");
    }
    grid_[static_cast<size_t>(y)][static_cast<size_t>(x)] = type;
}

void Board::decrementDots() noexcept {
    if (num_dots_ > 0) {
        --num_dots_;
    }
}