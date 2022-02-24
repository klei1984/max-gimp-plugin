/* Copyright (c) 2022 M.A.X. Port Team
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"),\ to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#define MAX_COLOR(r, g, b) (r), (g), (b)

#define PALETTE_INIT                                                                                                  \
    MAX_COLOR(255, 0, 255), MAX_COLOR(252, 0, 0), MAX_COLOR(0, 252, 0), MAX_COLOR(0, 0, 252), MAX_COLOR(252, 252, 0), \
        MAX_COLOR(252, 168, 0), MAX_COLOR(128, 128, 160), MAX_COLOR(252, 68, 0), MAX_COLOR(252, 252, 144),            \
        MAX_COLOR(168, 168, 224), MAX_COLOR(96, 88, 220), MAX_COLOR(168, 168, 224), MAX_COLOR(200, 200, 252),         \
        MAX_COLOR(240, 168, 100), MAX_COLOR(252, 252, 156), MAX_COLOR(240, 168, 100), MAX_COLOR(232, 48, 48),         \
        MAX_COLOR(40, 60, 72), MAX_COLOR(20, 96, 132), MAX_COLOR(40, 60, 72), MAX_COLOR(12, 12, 12),                  \
        MAX_COLOR(72, 56, 36), MAX_COLOR(180, 100, 0), MAX_COLOR(72, 56, 36), MAX_COLOR(12, 12, 12),                  \
        MAX_COLOR(12, 12, 12), MAX_COLOR(24, 24, 24), MAX_COLOR(40, 40, 40), MAX_COLOR(52, 52, 52),                   \
        MAX_COLOR(68, 64, 64), MAX_COLOR(84, 80, 80), MAX_COLOR(0, 252, 0), MAX_COLOR(128, 184, 24),                  \
        MAX_COLOR(108, 168, 12), MAX_COLOR(92, 156, 8), MAX_COLOR(76, 144, 4), MAX_COLOR(64, 116, 4),                 \
        MAX_COLOR(48, 92, 4), MAX_COLOR(36, 64, 4), MAX_COLOR(24, 40, 4), MAX_COLOR(184, 184, 4),                     \
        MAX_COLOR(176, 132, 4), MAX_COLOR(168, 84, 4), MAX_COLOR(160, 44, 4), MAX_COLOR(252, 252, 252),               \
        MAX_COLOR(100, 4, 120), MAX_COLOR(120, 52, 4), MAX_COLOR(144, 184, 12), MAX_COLOR(104, 156, 184),             \
        MAX_COLOR(68, 132, 168), MAX_COLOR(44, 112, 148), MAX_COLOR(20, 96, 132), MAX_COLOR(12, 76, 108),             \
        MAX_COLOR(8, 56, 84), MAX_COLOR(4, 40, 64), MAX_COLOR(4, 24, 40), MAX_COLOR(184, 120, 84),                    \
        MAX_COLOR(172, 96, 52), MAX_COLOR(160, 76, 24), MAX_COLOR(148, 56, 4), MAX_COLOR(120, 44, 4),                 \
        MAX_COLOR(96, 36, 4), MAX_COLOR(68, 24, 4), MAX_COLOR(36, 12, 4), MAX_COLOR(220, 116, 48),                    \
        MAX_COLOR(204, 112, 48), MAX_COLOR(216, 104, 44), MAX_COLOR(204, 104, 44), MAX_COLOR(196, 104, 48),           \
        MAX_COLOR(188, 104, 48), MAX_COLOR(188, 104, 40), MAX_COLOR(196, 96, 48), MAX_COLOR(196, 100, 40),            \
        MAX_COLOR(188, 96, 48), MAX_COLOR(188, 96, 40), MAX_COLOR(180, 96, 48), MAX_COLOR(180, 96, 40),               \
        MAX_COLOR(172, 96, 40), MAX_COLOR(192, 88, 40), MAX_COLOR(180, 88, 40), MAX_COLOR(172, 88, 48),               \
        MAX_COLOR(172, 88, 40), MAX_COLOR(164, 88, 40), MAX_COLOR(180, 80, 36), MAX_COLOR(172, 76, 36),               \
        MAX_COLOR(164, 80, 40), MAX_COLOR(164, 80, 32), MAX_COLOR(156, 80, 40), MAX_COLOR(156, 80, 32),               \
        MAX_COLOR(144, 80, 40), MAX_COLOR(164, 72, 32), MAX_COLOR(156, 72, 40), MAX_COLOR(156, 72, 32),               \
        MAX_COLOR(144, 72, 40), MAX_COLOR(148, 72, 32), MAX_COLOR(156, 64, 32), MAX_COLOR(120, 136, 188),             \
        MAX_COLOR(116, 136, 184), MAX_COLOR(120, 136, 188), MAX_COLOR(120, 140, 192), MAX_COLOR(124, 144, 196),       \
        MAX_COLOR(128, 148, 200), MAX_COLOR(120, 140, 192), MAX_COLOR(116, 128, 180), MAX_COLOR(116, 132, 184),       \
        MAX_COLOR(120, 136, 188), MAX_COLOR(120, 140, 192), MAX_COLOR(124, 144, 196), MAX_COLOR(128, 148, 200),       \
        MAX_COLOR(120, 140, 192), MAX_COLOR(108, 124, 160), MAX_COLOR(104, 116, 144), MAX_COLOR(108, 124, 160),       \
        MAX_COLOR(112, 128, 176), MAX_COLOR(116, 136, 192), MAX_COLOR(120, 140, 208), MAX_COLOR(112, 128, 176),       \
        MAX_COLOR(172, 88, 40), MAX_COLOR(172, 88, 40), MAX_COLOR(132, 152, 92), MAX_COLOR(128, 148, 196),            \
        MAX_COLOR(100, 160, 156), MAX_COLOR(180, 140, 72), MAX_COLOR(128, 136, 180), MAX_COLOR(136, 148, 188),        \
        MAX_COLOR(160, 160, 200), MAX_COLOR(140, 140, 184), MAX_COLOR(120, 128, 172), MAX_COLOR(228, 196, 136),       \
        MAX_COLOR(232, 192, 128), MAX_COLOR(236, 192, 120), MAX_COLOR(240, 188, 112), MAX_COLOR(244, 188, 104),       \
        MAX_COLOR(248, 184, 96), MAX_COLOR(252, 180, 88), MAX_COLOR(240, 176, 92), MAX_COLOR(232, 172, 96),           \
        MAX_COLOR(220, 168, 100), MAX_COLOR(208, 164, 100), MAX_COLOR(200, 156, 104), MAX_COLOR(188, 152, 104),       \
        MAX_COLOR(180, 148, 104), MAX_COLOR(168, 140, 104), MAX_COLOR(160, 136, 104), MAX_COLOR(144, 120, 88),        \
        MAX_COLOR(132, 104, 72), MAX_COLOR(120, 92, 60), MAX_COLOR(108, 80, 48), MAX_COLOR(96, 68, 40),               \
        MAX_COLOR(76, 72, 68), MAX_COLOR(68, 72, 72), MAX_COLOR(104, 52, 8), MAX_COLOR(52, 60, 72),                   \
        MAX_COLOR(12, 76, 104), MAX_COLOR(72, 56, 36), MAX_COLOR(76, 40, 8), MAX_COLOR(44, 36, 32),                   \
        MAX_COLOR(52, 28, 8), MAX_COLOR(36, 20, 4), MAX_COLOR(20, 12, 0), MAX_COLOR(252, 248, 244),                   \
        MAX_COLOR(240, 220, 208), MAX_COLOR(240, 216, 184), MAX_COLOR(220, 196, 172), MAX_COLOR(220, 192, 152),       \
        MAX_COLOR(216, 180, 140), MAX_COLOR(196, 164, 124), MAX_COLOR(180, 160, 128), MAX_COLOR(168, 152, 120),       \
        MAX_COLOR(156, 148, 136), MAX_COLOR(172, 164, 144), MAX_COLOR(188, 168, 148), MAX_COLOR(196, 184, 172),       \
        MAX_COLOR(204, 160, 104), MAX_COLOR(188, 152, 100), MAX_COLOR(168, 136, 92), MAX_COLOR(160, 136, 104),        \
        MAX_COLOR(152, 132, 96), MAX_COLOR(144, 132, 112), MAX_COLOR(128, 124, 116), MAX_COLOR(120, 112, 100),        \
        MAX_COLOR(128, 112, 88), MAX_COLOR(136, 120, 96), MAX_COLOR(144, 116, 80), MAX_COLOR(156, 124, 72),           \
        MAX_COLOR(168, 128, 72), MAX_COLOR(176, 136, 80), MAX_COLOR(192, 144, 80), MAX_COLOR(196, 136, 64),           \
        MAX_COLOR(176, 124, 56), MAX_COLOR(164, 112, 52), MAX_COLOR(144, 108, 56), MAX_COLOR(128, 104, 56),           \
        MAX_COLOR(120, 96, 68), MAX_COLOR(112, 96, 56), MAX_COLOR(112, 84, 40), MAX_COLOR(100, 80, 44),               \
        MAX_COLOR(88, 76, 56), MAX_COLOR(80, 68, 48), MAX_COLOR(80, 60, 40), MAX_COLOR(72, 56, 36),                   \
        MAX_COLOR(64, 56, 40), MAX_COLOR(56, 48, 36), MAX_COLOR(48, 40, 28), MAX_COLOR(40, 36, 32),                   \
        MAX_COLOR(36, 32, 28), MAX_COLOR(28, 24, 20), MAX_COLOR(12, 12, 12), MAX_COLOR(52, 28, 28),                   \
        MAX_COLOR(44, 40, 40), MAX_COLOR(52, 48, 48), MAX_COLOR(60, 56, 56), MAX_COLOR(72, 68, 68),                   \
        MAX_COLOR(84, 80, 80), MAX_COLOR(92, 88, 88), MAX_COLOR(100, 96, 96), MAX_COLOR(108, 104, 104),               \
        MAX_COLOR(112, 100, 80), MAX_COLOR(104, 92, 72), MAX_COLOR(96, 84, 64), MAX_COLOR(84, 64, 32),                \
        MAX_COLOR(72, 40, 40), MAX_COLOR(44, 40, 56), MAX_COLOR(128, 96, 40), MAX_COLOR(128, 104, 72),                \
        MAX_COLOR(204, 128, 104), MAX_COLOR(168, 108, 88), MAX_COLOR(184, 80, 52), MAX_COLOR(120, 76, 64),            \
        MAX_COLOR(152, 60, 44), MAX_COLOR(112, 36, 32), MAX_COLOR(72, 28, 20), MAX_COLOR(28, 12, 12),                 \
        MAX_COLOR(136, 168, 96), MAX_COLOR(112, 144, 76), MAX_COLOR(84, 144, 56), MAX_COLOR(92, 112, 64),             \
        MAX_COLOR(64, 104, 44), MAX_COLOR(56, 80, 32), MAX_COLOR(40, 64, 24), MAX_COLOR(20, 24, 12),                  \
        MAX_COLOR(116, 108, 156), MAX_COLOR(96, 84, 128), MAX_COLOR(56, 64, 136), MAX_COLOR(64, 64, 104),             \
        MAX_COLOR(44, 48, 104), MAX_COLOR(64, 56, 76), MAX_COLOR(28, 32, 72), MAX_COLOR(12, 16, 40),                  \
        MAX_COLOR(180, 100, 0), MAX_COLOR(132, 72, 0), MAX_COLOR(88, 48, 0), MAX_COLOR(152, 152, 0),                  \
        MAX_COLOR(108, 108, 0), MAX_COLOR(64, 64, 0), MAX_COLOR(252, 252, 252)
