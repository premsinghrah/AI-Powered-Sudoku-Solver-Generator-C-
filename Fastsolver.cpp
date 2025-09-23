// sudoku_fixed.cpp
// C++11/C++17 compatible single-file Sudoku solver + generator
// Compile: g++ -std=c++11 sudoku_fixed.cpp -O2 -o sudoku

#include <bits/stdc++.h>
using namespace std;

using Board = array<array<int,9>,9>;

static inline int blockIndex(int r, int c) { return (r/3)*3 + (c/3); }

// Pretty print board
void printBoard(const Board &b) {
    for (int r = 0; r < 9; ++r) {
        if (r % 3 == 0) cout << "+-------+-------+-------+\n";
        for (int c = 0; c < 9; ++c) {
            if (c % 3 == 0) cout << "| ";
            if (b[r][c] == 0) cout << ". ";
            else cout << b[r][c] << ' ';
        }
        cout << "|\n";
    }
    cout << "+-------+-------+-------+\n";
}

// Parse board from 81-char string of digits or '.'; returns false if format wrong
bool parseBoard(const string &s, Board &b) {
    string t;
    for (char ch : s) if (!isspace((unsigned char)ch)) t.push_back(ch);
    if (t.size() != 81) return false;
    for (int i=0;i<81;++i) {
        char ch = t[i];
        int r = i/9, c = i%9;
        if (ch == '.' || ch == '0') b[r][c] = 0;
        else if (ch >= '1' && ch <= '9') b[r][c] = ch - '0';
        else return false;
    }
    return true;
}

// Solver class: supports solve and counting solutions up to a limit
struct Solver {
    Board board;
    array<int,9> rowMask, colMask, blockMask; // bit masks: bit d-1 set if digit d is used
    vector<pair<int,int>> empties; // list of empty cells (r,c)

    Solver() { reset(); }

    void reset() {
        board = {};
        rowMask.fill(0);
        colMask.fill(0);
        blockMask.fill(0);
        empties.clear();
    }

    // Load board and init masks; returns false if invalid (conflict)
    bool loadBoard(const Board &b) {
        reset();
        board = b;
        for (int r=0;r<9;++r) for (int c=0;c<9;++c) {
            int v = board[r][c];
            if (v == 0) continue;
            int bit = 1 << (v-1);
            if (rowMask[r] & bit) return false;
            if (colMask[c] & bit) return false;
            int bi = blockIndex(r,c);
            if (blockMask[bi] & bit) return false;
            rowMask[r] |= bit;
            colMask[c] |= bit;
            blockMask[bi] |= bit;
        }
        for (int r=0;r<9;++r) for (int c=0;c<9;++c) if (board[r][c]==0) empties.emplace_back(r,c);
        return true;
    }

    // returns bitmask of possible digits (bits 0..8)
    inline int candidatesMask(int r, int c) const {
        int used = rowMask[r] | colMask[c] | blockMask[blockIndex(r,c)];
        return (~used) & 0x1FF; // 9 bits
    }

    // Solve with backtracking; count solutions up to countLimit; outCount will contain number found (<= countLimit)
    // IMPORTANT: outCount must be provided by caller.
    bool solve(int countLimit, int &outCount) {
        outCount = 0;
        Board savedBoard;
        bool saved = false;

        // DFS returns true if search should stop (i.e., we've reached countLimit)
        function<bool()> dfs = [&]() -> bool {
            if (outCount >= countLimit) return true; // stop
            // Find cell with minimum candidates (MRV)
            int bestIdx = -1, bestCount = 10, bestMask = 0;
            for (int i = 0; i < (int)empties.size(); ++i) {
                int r = empties[i].first;
                int c = empties[i].second;
                if (board[r][c] != 0) continue;
                int mask = candidatesMask(r,c);
                if (mask == 0) return false; // dead end on this path
                int cnt = __builtin_popcount(mask);
                if (cnt < bestCount) { bestCount = cnt; bestIdx = i; bestMask = mask; if (cnt==1) break; }
            }
            if (bestIdx == -1) {
                // Found a full solution
                ++outCount;
                if (!saved) { saved = true; savedBoard = board; } // save first found solution
                return outCount >= countLimit; // if we've reached limit -> tell callers to stop
            }
            int r = empties[bestIdx].first, c = empties[bestIdx].second;
            int m = bestMask;
            while (m && outCount < countLimit) {
                int lowbit = m & -m;
                int d = __builtin_ctz(lowbit) + 1; // digit to try
                m -= lowbit;
                int bit = 1 << (d-1);
                // place d
                board[r][c] = d;
                rowMask[r] |= bit;
                colMask[c] |= bit;
                blockMask[blockIndex(r,c)] |= bit;
                bool stop = dfs();
                // undo
                board[r][c] = 0;
                rowMask[r] &= ~bit;
                colMask[c] &= ~bit;
                blockMask[blockIndex(r,c)] &= ~bit;
                if (stop) return true;
            }
            return false;
        };

        dfs();
        if (saved) board = savedBoard; // restore the first solution into board so caller can print it
        return outCount >= 1;
    }

    // Solve and fill board with one solution (fast single-solution fill)
    bool solveOne() {
        int cnt = 0;
        bool ok = solve(1, cnt);
        return ok;
    }

    // Count solutions up to limit (returns number found, at most limit)
    int countSolutions(int limit=2) {
        int cnt = 0;
        solve(limit, cnt);
        return cnt;
    }
};

// (The rest of your generator code left mostly unchanged)

// Generate a full solved board via randomized backtracking
bool fillFullBoard(Board &b, mt19937 &rng) {
    b = {};
    vector<pair<int,int>> cells;
    cells.reserve(81);
    for (int r=0;r<9;++r) for (int c=0;c<9;++c) cells.emplace_back(r,c);

    function<bool(int)> backtrack = [&](int idx)->bool {
        if (idx == 81) return true;
        int r = cells[idx].first, c = cells[idx].second;
        int used = 0;
        for (int cc=0; cc<9; ++cc) if (b[r][cc]) used |= 1 << (b[r][cc]-1);
        for (int rr=0; rr<9; ++rr) if (b[rr][c]) used |= 1 << (b[rr][c]-1);
        int bi = blockIndex(r,c);
        int br = (bi/3)*3, bc = (bi%3)*3;
        for (int rr = br; rr < br+3; ++rr) for (int cc = bc; cc < bc+3; ++cc) if (b[rr][cc]) used |= 1 << (b[rr][cc]-1);
        int mask = (~used) & 0x1FF;
        if (mask == 0) return false;
        vector<int> digs;
        while (mask) {
            int lb = mask & -mask;
            digs.push_back(__builtin_ctz(lb) + 1);
            mask -= lb;
        }
        shuffle(digs.begin(), digs.end(), rng);
        for (int d : digs) {
            b[r][c] = d;
            if (backtrack(idx+1)) return true;
            b[r][c] = 0;
        }
        return false;
    };

    return backtrack(0);
}

Board generateFullSolution(mt19937 &rng) {
    Board b{};
    array<int,9> rowMask{}, colMask{}, blockMask{};
    vector<pair<int,int>> empties;
    for (int r=0;r<9;++r) for (int c=0;c<9;++c) empties.emplace_back(r,c);

    function<bool()> dfs = [&]() -> bool {
        int bestIdx=-1, bestCnt=10, bestMask=0;
        for (int i=0;i<(int)empties.size(); ++i) {
            int r=empties[i].first, c=empties[i].second;
            if (b[r][c]!=0) continue;
            int used = rowMask[r] | colMask[c] | blockMask[blockIndex(r,c)];
            int mask = (~used) & 0x1FF;
            int cnt = __builtin_popcount(mask);
            if (cnt==0) return false;
            if (cnt < bestCnt) { bestCnt = cnt; bestIdx = i; bestMask = mask; if (cnt==1) break; }
        }
        if (bestIdx == -1) return true;
        int r = empties[bestIdx].first, c = empties[bestIdx].second;
        vector<int> digits;
        int m = bestMask;
        while (m) { int lb = m & -m; digits.push_back(__builtin_ctz(lb)+1); m -= lb; }
        shuffle(digits.begin(), digits.end(), rng);
        for (int d : digits) {
            int bit = 1 << (d-1);
            b[r][c] = d;
            rowMask[r] |= bit;
            colMask[c] |= bit;
            blockMask[blockIndex(r,c)] |= bit;
            if (dfs()) return true;
            b[r][c] = 0;
            rowMask[r] &= ~bit;
            colMask[c] &= ~bit;
            blockMask[blockIndex(r,c)] &= ~bit;
        }
        return false;
    };

    bool ok = dfs();
    if (!ok) return generateFullSolution(rng);
    return b;
}

Board generatePuzzle(mt19937 &rng, int targetClues = 30) {
    Board solution = generateFullSolution(rng);
    Board puzzle = solution;
    vector<pair<int,int>> positions;
    for (int r=0;r<9;++r) for (int c=0;c<9;++c) positions.emplace_back(r,c);
    shuffle(positions.begin(), positions.end(), rng);

    Solver solver;
    for (auto pos : positions) {
        int filled = 0;
        for (int r=0;r<9;++r) for (int c=0;c<9;++c) if (puzzle[r][c] != 0) ++filled;
        if (filled <= targetClues) break;

        int r = pos.first, c = pos.second;
        if (puzzle[r][c] == 0) continue;
        int old = puzzle[r][c];
        puzzle[r][c] = 0;

        if (!solver.loadBoard(puzzle)) { puzzle[r][c] = old; continue; }
        int cnt = solver.countSolutions(2);
        if (cnt != 1) {
            puzzle[r][c] = old;
        }
    }
    return puzzle;
}

int difficultyToClues(const string &diff) {
    string d = diff;
    for (auto &ch: d) ch = tolower((unsigned char)ch);
    if (d=="easy") return 40;
    if (d=="medium") return 34;
    if (d=="hard") return 28;
    try {
        int v = stoi(diff);
        if (v < 17) return 17;
        if (v > 81) return 81;
        return v;
    } catch(...) {
        return 30;
    }
}

void menu() {
    cout << "AI-Powered Sudoku - Solver & Generator\n";
    cout << "Options:\n";
    cout << "  1 - Generate puzzle (easy/medium/hard or specify number of clues e.g. 30)\n";
    cout << "  2 - Solve puzzle (enter 81 characters: digits or . for blanks)\n";
    cout << "  0 - Exit\n";
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    mt19937 rng((unsigned)chrono::high_resolution_clock::now().time_since_epoch().count());

    while (true) {
        menu();
        cout << "Choose option: ";
        int opt;
        if (!(cin >> opt)) return 0;
        if (opt == 0) return 0;
        if (opt == 1) {
            cout << "Enter difficulty (easy/medium/hard) or number of clues (17-81). Default 'medium': ";
            string diff;
            if (!(cin >> diff)) diff = "medium";
            int clues = difficultyToClues(diff);
            cout << "Generating puzzle with target ~" << clues << " clues (unique-solutions enforced)...\n";
            Board p = generatePuzzle(rng, clues);
            printBoard(p);
            cout << "Solution? (y/n): ";
            char ans; cin >> ans;
            if (ans == 'y' || ans == 'Y') {
                Solver s;
                if (!s.loadBoard(p)) {
                    cerr << "Invalid puzzle loaded.\n";
                } else {
                    s.solveOne();
                    cout << "Solution:\n";
                    printBoard(s.board);
                }
            }
        } else if (opt == 2) {
            cout << "Enter puzzle as single line (81 chars) or 9 lines of 9 chars. Use digits 1-9 and . for blank.\n";
            string line;
            getline(cin, line); // consume rest of current line
            string input;
            while ((int)input.size() < 81) {
                if (!getline(cin, line)) break;
                bool onlyws = true;
                for (char ch : line) if (!isspace((unsigned char)ch)) onlyws = false;
                if (onlyws) continue;
                input += line;
            }
            string cleaned;
            for (char ch : input) if (!isspace((unsigned char)ch)) cleaned.push_back(ch);
            Board b;
            if (!parseBoard(cleaned, b)) {
                cerr << "Couldn't parse board. Ensure 81 characters (digits or .)\n";
                continue;
            }
            cout << "Input puzzle:\n";
            printBoard(b);
            Solver solver;
            if (!solver.loadBoard(b)) {
                cerr << "Puzzle invalid (contradiction detected).\n";
                continue;
            }
            int sols = solver.countSolutions(2);
            if (sols == 0) {
                cout << "No solutions exist for this puzzle.\n";
            } else if (sols > 1) {
                cout << "Multiple (" << sols << ") solutions found (<=2 checked). Solver will produce one solution:\n";
                solver.solveOne();
                printBoard(solver.board);
            } else {
                cout << "Unique solution found:\n";
                solver.solveOne();
                printBoard(solver.board);
            }
        } else {
            cout << "Unknown option.\n";
        }
    }
    return 0;
}
