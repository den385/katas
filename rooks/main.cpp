#include <iostream>
#include <array>
#include <mutex>
#include <thread>
#include <vector>
#include <memory>
#include <random>
#include <utility>
#include <chrono>
#include <iomanip>

// 0. На шахматной доске находятся в произвольной позиции N ладей (4-6)
// 1. Они все одновременно начинают ходить на случайные позиции (при этом перемещаться они, естественно, могут только горизонтально либо вертикально).
// 2. Между каждыми ходами каждая фигура делает паузу 200-300 миллисекунд.
// 3. Если на пути фигуры оказывается другая, она ждет, пока путь освободится. Если в течение 5 секунд проход не освободился, выбирается другая позиция аналогичным случайным образом.
// 4. Всё заканчивается, когда все фигуры сделают по 50 ходов
// * GUI никакое не требуется, достаточно будет понятного текстового вывода.


using Board = std::array<std::array<std::timed_mutex, 8>, 8>;   // not RAII
using Pos = std::pair<size_t, size_t>;

thread_local uint rook_moves = 0;
std::mutex cout_mx;

constexpr const std::array<char,8> ax1 = { 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H' };
constexpr const std::array<uint,8> ax2 = { 1, 2, 3, 4, 5, 6, 7, 8 };

size_t random_num(const size_t lo, const size_t hi)
{
    std::random_device rd; // obtain a random number from hardware
    std::mt19937 eng(rd()); // seed the generator
    std::uniform_int_distribution<size_t> distr(lo, hi); // define the range

    return distr(eng); // generate numbers
}

Pos random_rook_move(Pos pos)
{
    if (random_num(0, 1)) // random choice of axis
        return {pos.first, random_num(0,7)};
    else
        return {random_num(0,7), pos.second};
}


void rook_func(int rook_id, Pos rook_pos, Board& board)
{
    using msec = std::chrono::milliseconds;
    using sec = std::chrono::seconds;

    Pos new_rook_pos;

    while (rook_moves < 50)
    {
        do
        {
            new_rook_pos = random_rook_move(rook_pos);
        }
        while(!board[new_rook_pos.first][new_rook_pos.second].try_lock_for(sec(5)));    // on success, thread holds 2 mutexes

        board[rook_pos.first][rook_pos.second].unlock();

        {
            std::lock_guard<std::mutex> lk(cout_mx);
            std::cout << "rook" << rook_id << " move" << std::setw(2) << rook_moves << "\t\t"
                      << ax1[rook_pos.first] << ax2[rook_pos.second] << '-'
                      << ax1[new_rook_pos.first] << ax2[new_rook_pos.second] << std::endl;
        }

        rook_pos = new_rook_pos;
        rook_moves++;

        if (rook_moves < 50)
            std::this_thread::sleep_for(msec(random_num(200, 300)));
    }

    board[rook_pos.first][rook_pos.second].unlock();
}


int main(int argc, char* argv[])
{
    int n_rooks = 4;

    if (argc == 2)
    try
    {
        int tmp = std::stoi(argv[1]);
        n_rooks = (4 <= tmp && tmp <= 6) ? tmp : n_rooks;
    }
    catch(...) {}

    Board board;

    std::vector<std::thread> rooks;
    for (int i=0; i<n_rooks; i++)
    {
        Pos i_pos;
        do
        {
            i_pos = std::make_pair(random_num(0,7), random_num(0,7));
        }
        while (!board[i_pos.first][i_pos.second].try_lock());

        std::thread th(rook_func, i, i_pos, std::ref(board));
        rooks.push_back(std::move(th));
    }

    for (auto& rk : rooks)
        rk.join();

    return 0;
}