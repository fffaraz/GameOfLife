// Conway's Game of Life - deterministic unit tests for Grid behavior
// Copyright (c) 2025 Faraz Fallahi <fffaraz@gmail.com>
//
// Tests are fully deterministic (no RNG): they drive the bit-packed Grid through
// known still lifes, oscillators, and spaceships, with deliberate coverage of the
// two things the SWAR rewrite could get wrong -- 64-cell word boundaries and the
// non-toroidal grid edges. Exit code is nonzero if any check fails.

#include "Grid.hpp"

#include <cstdint>
#include <cstdio>
#include <initializer_list>

namespace {

// A 128-wide grid has two 64-bit words per row, so column 64 starts a new word.
// This lets patterns straddle a word boundary (cols 63/64/65) to exercise the
// cross-word carry bit in updateRow.
constexpr int N = 128;
using G = Grid<N>;

int g_checks = 0;
int g_failures = 0;

#define CHECK(cond)                                                       \
    do {                                                                  \
        ++g_checks;                                                       \
        if (!(cond)) {                                                    \
            ++g_failures;                                                 \
            std::printf("    FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        }                                                                 \
    } while (0)

void setCells(G& g, std::initializer_list<Point> pts)
{
    for (const auto& p : pts) {
        g.set(p, true);
    }
}

int countAlive(const G& g)
{
    int c = 0;
    for (int x = 0; x < N; ++x) {
        for (int y = 0; y < N; ++y) {
            c += g.get({ x, y }) ? 1 : 0;
        }
    }
    return c;
}

bool gridsEqual(const G& a, const G& b)
{
    for (int x = 0; x < N; ++x) {
        for (int y = 0; y < N; ++y) {
            if (a.get({ x, y }) != b.get({ x, y })) {
                return false;
            }
        }
    }
    return true;
}

// True iff exactly the listed cells are alive.
bool onlyCellsAlive(const G& g, std::initializer_list<Point> pts)
{
    G ref;
    ref.clear();
    for (const auto& p : pts) {
        ref.set(p, true);
    }
    return gridsEqual(g, ref);
}

// Evolve src by one generation into a fresh grid (updateRow writes every word).
G step(const G& src)
{
    G dst;
    dst.updateGrid(src);
    return dst;
}

G stepN(G g, int n)
{
    for (int i = 0; i < n; ++i) {
        g = step(g);
    }
    return g;
}

// ---------------------------------------------------------------------------

void test_get_set_toggle_clear()
{
    G g;
    g.clear();
    CHECK(countAlive(g) == 0);

    g.set({ 5, 7 }, true);
    CHECK(g.get({ 5, 7 }));
    CHECK(countAlive(g) == 1);

    g.toggle({ 5, 7 });
    CHECK(!g.get({ 5, 7 }));
    CHECK(countAlive(g) == 0);

    g.toggle({ 5, 7 });
    CHECK(g.get({ 5, 7 }));
    g.set({ 5, 7 }, false);
    CHECK(!g.get({ 5, 7 }));

    // Set/get across the word boundary (cols 63 -> 64) and the last column.
    for (int y : { 0, 1, 62, 63, 64, 65, 126, 127 }) {
        g.clear();
        g.set({ 3, y }, true);
        CHECK(g.get({ 3, y }));
        CHECK(countAlive(g) == 1);
    }
}

void test_toggleBlock()
{
    G g;
    g.clear();
    g.toggleBlock({ 10, 10 }); // 3x3 centered at (10,10)
    CHECK(countAlive(g) == 9);
    for (int x = 9; x <= 11; ++x) {
        for (int y = 9; y <= 11; ++y) {
            CHECK(g.get({ x, y }));
        }
    }

    // Corner: the 3x3 is clipped to the 2x2 in-bounds cells.
    g.clear();
    g.toggleBlock({ 0, 0 });
    CHECK(onlyCellsAlive(g, { { 0, 0 }, { 0, 1 }, { 1, 0 }, { 1, 1 } }));

    // Straddling the word boundary at column 64.
    g.clear();
    g.toggleBlock({ 5, 64 });
    CHECK(countAlive(g) == 9);
    for (int x = 4; x <= 6; ++x) {
        for (int y = 63; y <= 65; ++y) {
            CHECK(g.get({ x, y }));
        }
    }
}

void test_countLiveNeighbors()
{
    G g;
    g.clear();
    setCells(g, { { 9, 9 }, { 9, 10 }, { 9, 11 }, { 10, 9 }, { 10, 11 }, { 11, 9 }, { 11, 10 }, { 11, 11 } });
    CHECK(g.countLiveNeighbors({ 10, 10 }) == 8);
    g.set({ 10, 10 }, true); // the cell itself must not be counted
    CHECK(g.countLiveNeighbors({ 10, 10 }) == 8);

    // Corner cell: only 3 of the 8 neighbors are in-bounds.
    g.clear();
    setCells(g, { { 0, 1 }, { 1, 0 }, { 1, 1 } });
    CHECK(g.countLiveNeighbors({ 0, 0 }) == 3);

    // Neighbors span the word boundary: col 64's left neighbor (63) is in the
    // previous word, right neighbor (65) in the same word.
    g.clear();
    setCells(g, { { 4, 63 }, { 5, 63 }, { 6, 63 }, { 4, 64 }, { 6, 64 }, { 4, 65 }, { 5, 65 }, { 6, 65 } });
    CHECK(g.countLiveNeighbors({ 5, 64 }) == 8);
}

void test_still_life_block()
{
    const std::initializer_list<Point> block { { 10, 10 }, { 10, 11 }, { 11, 10 }, { 11, 11 } };
    G g;
    g.clear();
    setCells(g, block);
    CHECK(onlyCellsAlive(step(g), block)); // stable after 1 gen
    CHECK(onlyCellsAlive(stepN(g, 4), block)); // and after 4

    // updateGrid must not mutate its source.
    CHECK(onlyCellsAlive(g, block));
}

void test_blinker_interior()
{
    G g;
    g.clear();
    setCells(g, { { 10, 10 }, { 10, 11 }, { 10, 12 } }); // horizontal
    CHECK(onlyCellsAlive(step(g), { { 9, 11 }, { 10, 11 }, { 11, 11 } })); // vertical
    CHECK(onlyCellsAlive(stepN(g, 2), { { 10, 10 }, { 10, 11 }, { 10, 12 } })); // period 2
}

void test_blinker_word_boundary()
{
    // Horizontal blinker straddling the word boundary (cols 63/64/65).
    G g;
    g.clear();
    setCells(g, { { 20, 63 }, { 20, 64 }, { 20, 65 } });
    // Should oscillate to a vertical bar centered on column 64.
    CHECK(onlyCellsAlive(step(g), { { 19, 64 }, { 20, 64 }, { 21, 64 } }));
    CHECK(onlyCellsAlive(stepN(g, 2), { { 20, 63 }, { 20, 64 }, { 20, 65 } }));
}

// A glider returns to its own shape translated by (+1,+1) every 4 generations.
// We assert exactly that invariant, so there are no hand-computed coordinates to
// get wrong -- only the well-known translation property is encoded.
void checkGliderTranslates(std::initializer_list<Point> glider)
{
    G g;
    g.clear();
    setCells(g, glider);

    G expected4;
    expected4.clear();
    for (const auto& p : glider) {
        expected4.set({ p.x + 1, p.y + 1 }, true);
    }
    const G g4 = stepN(g, 4);
    CHECK(gridsEqual(g4, expected4));
    CHECK(countAlive(g4) == 5);

    G expected8;
    expected8.clear();
    for (const auto& p : glider) {
        expected8.set({ p.x + 2, p.y + 2 }, true);
    }
    CHECK(gridsEqual(stepN(g, 8), expected8));
}

void test_glider_interior()
{
    checkGliderTranslates({ { 10, 11 }, { 11, 12 }, { 12, 10 }, { 12, 11 }, { 12, 12 } });
}

void test_glider_across_word_boundary()
{
    // Positioned so the glider marches through column 64 over its 8-gen run.
    checkGliderTranslates({ { 30, 63 }, { 31, 64 }, { 32, 62 }, { 32, 63 }, { 32, 64 } });
}

void test_birth_and_death_rules()
{
    // Underpopulation: a lone cell dies.
    G lone;
    lone.clear();
    lone.set({ 20, 20 }, true);
    CHECK(countAlive(step(lone)) == 0);

    // Overpopulation: a cell with 4 neighbors dies.
    G plus;
    plus.clear();
    setCells(plus, { { 10, 10 }, { 9, 10 }, { 11, 10 }, { 10, 9 }, { 10, 11 } });
    CHECK(!step(plus).get({ 10, 10 }));

    // Birth: three cells in an L give the empty corner exactly 3 neighbors, and
    // the whole thing settles into a 2x2 block.
    G ell;
    ell.clear();
    setCells(ell, { { 5, 5 }, { 5, 6 }, { 6, 5 } });
    const G born = step(ell);
    CHECK(born.get({ 6, 6 }));
    CHECK(onlyCellsAlive(born, { { 5, 5 }, { 5, 6 }, { 6, 5 }, { 6, 6 } }));
}

void test_non_toroidal_edges()
{
    // A 2x2 block wedged in the top-left corner is a still life.
    const std::initializer_list<Point> cornerBlock { { 0, 0 }, { 0, 1 }, { 1, 0 }, { 1, 1 } };
    G corner;
    corner.clear();
    setCells(corner, cornerBlock);
    CHECK(onlyCellsAlive(step(corner), cornerBlock));

    // A blinker on the top edge (row 0) cannot wrap around; it decays to nothing.
    // If the grid were toroidal it would oscillate forever instead.
    G topBlinker;
    topBlinker.clear();
    setCells(topBlinker, { { 0, 10 }, { 0, 11 }, { 0, 12 } });
    CHECK(onlyCellsAlive(step(topBlinker), { { 0, 11 }, { 1, 11 } }));
    CHECK(countAlive(stepN(topBlinker, 2)) == 0);

    // Same at the far right edge / last word (column 127 must not wrap to 0).
    G rightBlinker;
    rightBlinker.clear();
    setCells(rightBlinker, { { 10, 127 }, { 11, 127 }, { 12, 127 } });
    CHECK(onlyCellsAlive(step(rightBlinker), { { 11, 126 }, { 11, 127 } }));
    CHECK(countAlive(stepN(rightBlinker, 2)) == 0);
}

// Generic helpers for a grid size that actually gets multiple worker bands. The
// pattern tests above run on Grid<N=128>, which the size-aware thread heuristic
// (>=128 rows/band) deliberately runs single-threaded.
template <int S>
Grid<S> evolve(Grid<S> g, int n)
{
    for (int i = 0; i < n; ++i) {
        Grid<S> d;
        d.updateGrid(g);
        g = d;
    }
    return g;
}

template <int S>
bool sameGrid(const Grid<S>& a, const Grid<S>& b)
{
    for (int x = 0; x < S; ++x) {
        for (int y = 0; y < S; ++y) {
            if (a.get({ x, y }) != b.get({ x, y })) {
                return false;
            }
        }
    }
    return true;
}

template <int S>
int aliveCount(const Grid<S>& g)
{
    int c = 0;
    for (int x = 0; x < S; ++x) {
        for (int y = 0; y < S; ++y) {
            c += g.get({ x, y }) ? 1 : 0;
        }
    }
    return c;
}

// The parallel band executor must produce identical results run to run. A 256-row
// grid gets 2 worker bands (boundary at row 128); speckling live cells across every
// row means both bands and their shared halo rows are active, so a data race at a
// band edge would diverge the two runs.
void test_parallel_determinism()
{
    constexpr int S = 256;
    Grid<S> g;
    g.clear();
    // Irregular ~50% "soup" (a hashed fill, not a regular lattice) so it stays busy
    // for the whole run instead of collapsing -- otherwise the two runs would agree
    // only by both dying out.
    for (int x = 0; x < S; ++x) {
        for (int y = 0; y < S; ++y) {
            uint32_t h = (static_cast<uint32_t>(x) * 73856093u) ^ (static_cast<uint32_t>(y) * 19349663u);
            h ^= h >> 13;
            h *= 0x5bd1e995u;
            h ^= h >> 15;
            if (h & 1u) {
                g.set({ x, y }, true);
            }
        }
    }
    const Grid<S> a = evolve(g, 20);
    const Grid<S> b = evolve(g, 20);
    CHECK(sameGrid(a, b));
    CHECK(aliveCount(a) > 0); // guards against a vacuous empty == empty pass
}

struct Test {
    const char* name;
    void (*fn)();
};

const Test kTests[] = {
    { "get/set/toggle/clear", test_get_set_toggle_clear },
    { "toggleBlock", test_toggleBlock },
    { "countLiveNeighbors", test_countLiveNeighbors },
    { "still life (block)", test_still_life_block },
    { "blinker (interior)", test_blinker_interior },
    { "blinker (word boundary)", test_blinker_word_boundary },
    { "glider (interior)", test_glider_interior },
    { "glider (word boundary)", test_glider_across_word_boundary },
    { "birth/death rules", test_birth_and_death_rules },
    { "non-toroidal edges", test_non_toroidal_edges },
    { "parallel determinism", test_parallel_determinism },
};

} // namespace

int main()
{
    std::printf("Grid unit tests (grid size %d)\n", N);
    for (const auto& t : kTests) {
        const int before = g_failures;
        t.fn();
        std::printf("  [%s] %s\n", g_failures == before ? "PASS" : "FAIL", t.name);
    }
    std::printf("\n%d checks, %d failure(s)\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
