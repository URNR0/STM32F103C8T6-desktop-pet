# -*- coding: utf-8 -*-
"""
生成桌面宠物动画帧(pet_frames.h) 和 8x8 ASCII 字库(font8x8.h)。
只生成一次, 产物是 C 数组, 直接拷进 Keil 工程用。
"""
import os

OUT_DIR = os.path.dirname(os.path.abspath(__file__))

# ============================================================
# 1) 基本几何(纯数学, 不依赖第三方库)
# ============================================================
def in_ellipse(px, py, cx, cy, rx, ry):
    ex = (px - cx) / rx
    ey = (py - cy) / ry
    return ex * ex + ey * ey <= 1.0

def in_rect(px, py, x0, y0, x1, y1):
    return x0 <= px <= x1 and y0 <= py <= y1

def in_tri(px, py, ax, ay, bx, by, cx, cy):
    def sign(x1, y1, x2, y2, x3, y3):
        return (x1 - x3) * (y2 - y3) - (x2 - x3) * (y1 - y3)
    d1 = sign(px, py, ax, ay, bx, by)
    d2 = sign(px, py, bx, by, cx, cy)
    d3 = sign(px, py, cx, cy, ax, ay)
    neg = (d1 < 0) or (d2 < 0) or (d3 < 0)
    pos = (d1 > 0) or (d2 > 0) or (d3 > 0)
    return not (neg and pos)

def dist_seg(px, py, x1, y1, x2, y2):
    dx = x2 - x1
    dy = y2 - y1
    L = dx * dx + dy * dy
    if L == 0:
        return ((px - x1) ** 2 + (py - y1) ** 2) ** 0.5
    t = (px - x1) * dx + (py - y1) * dy
    t = max(0.0, min(1.0, t / L))
    cx = x1 + t * dx
    cy = y1 + t * dy
    return ((px - cx) ** 2 + (py - cy) ** 2) ** 0.5

# 宠物脸: 白色(1)为脸, 眼睛/嘴巴用黑色(0)从脸上挖出来
def is_face(px, py):
    head = in_ellipse(px, py, 64, 40, 29, 24)
    earL = in_tri(px, py, 43, 4, 37, 19, 52, 19)
    earR = in_tri(px, py, 85, 4, 76, 19, 91, 19)
    return head or earL or earR

def is_eye(px, py, mode, cx):
    cy = 37
    if mode == "open":        # 睁眼: 圆眼
        return in_ellipse(px, py, cx, cy, 4.5, 6.0)
    if mode == "blink":       # 闭眼: 一条横线
        return in_rect(px, py, cx - 5.0, cy - 1.3, cx + 5.0, cy + 1.3)
    if mode == "sleep":       # 打盹: 闭眼横线(略粗, 更放松)
        return in_rect(px, py, cx - 5.5, cy - 1.8, cx + 5.5, cy + 1.8)
    if mode == "sad":         # 委屈: 无精打采的短横眼
        return in_rect(px, py, cx - 3.5, cy - 1.0, cx + 3.5, cy + 1.2)
    if mode == "happy":       # 开心: ^ ^ 两个小折线
        return (dist_seg(px, py, cx - 5, cy + 3, cx, cy - 4) <= 1.7 or
                dist_seg(px, py, cx, cy - 4, cx + 5, cy + 3) <= 1.7)
    return False

def is_mouth(px, py, mode):
    mx, my, r = 64, 47, 6.5
    d = ((px - mx) ** 2 + (py - my) ** 2) ** 0.5
    if mode == "happy":        # 张嘴笑: 下半个实心圆
        return py >= my and in_ellipse(px, py, mx, my + 2.0, r, r - 1.0)
    if mode == "sleep":        # 打盹: 平静小嘴(一条短横线)
        return in_rect(px, py, mx - 3.0, my - 0.8, mx + 3.0, my + 0.8)
    if mode == "sad":          # 委屈: 嘴角下垂(上半圆弧, 倒U)
        return (abs(d - r) <= 1.9) and (py <= my)
    # 普通/眨眼: 微笑弧线(下半圆弧, 中间低两边高)
    return (abs(d - r) <= 1.9) and (py >= my)

def is_zzz(px, py):
    """打盹符号: 头顶画三个 Z, 从大到小往右上排"""
    for cx, cy, s in ((104, 21, 5.0), (116, 13, 3.5), (124, 6, 2.5)):
        left, right = cx - s, cx + s
        top, bot = cy - s, cy + s
        if in_rect(px, py, left, top - 0.9, right, top + 0.9):   # 顶横
            return True
        if in_rect(px, py, left, bot - 0.9, right, bot + 0.9):   # 底横
            return True
        if dist_seg(px, py, right, top, left, bot) <= 1.0:       # 斜线
            return True
    return False

def render_frame(eye_mode):
    W, H, S = 128, 64, 4
    frame = [[0] * W for _ in range(H)]
    for y in range(H):
        for x in range(W):
            hit = 0
            for sy in range(S):
                for sx in range(S):
                    px = x + (sx + 0.5) / S
                    py = y + (sy + 0.5) / S
                    if eye_mode == "sleep" and is_zzz(px, py):
                        hit += 1          # 打盹头顶的 Zzz 独立亮起
                        continue
                    if is_face(px, py):
                        # 眼睛/嘴巴是黑(挖空)
                        if not (is_eye(px, py, eye_mode, 50) or
                                is_eye(px, py, eye_mode, 78) or
                                is_mouth(px, py, eye_mode)):
                            hit += 1
            frame[y][x] = 1 if (hit / (S * S)) >= 0.5 else 0
    return frame

def encode_frame(frame):
    """列优先, 每字节 = 同一列的 8 个纵向像素, bit0=最顶行。"""
    pages = 8
    out = []
    for x in range(128):
        for p in range(pages):
            b = 0
            for r in range(8):
                y = p * 8 + r
                if frame[y][x]:
                    b |= (1 << r)
            out.append(b)
    return bytes(out)

# ============================================================
# 2) 8x8 ASCII 字库(横向扫描: 每行一个字节, bit7=最左)
# ============================================================
GLYPHS = {
    ' ': [24, "........", "........", "........", "........",
              "........", "........", "........", "........"],
    '0': [0, ".#####..", "##...##.", "##...##.", "##...##.",
              "##...##.", "##...##.", "##...##.", ".#####.."],
    '1': [0, "...##...", ".####...", "...##...", "...##...",
              "...##...", "...##...", "...##...", ".######."],
    '2': [0, ".#####..", "##...##.", ".....##.", "....##..",
              "...##...", "..##....", ".##.....", "#######."],
    '3': [0, ".#####..", "##...##.", ".....##.", ".####...",
              ".....##.", "##...##.", ".#####..", "........"],
    '4': [0, "....##..", "...###..", "..####..", ".##.##..",
              "##..##..", "#######.", "....##..", "....##.."],
    '5': [0, "#######.", "##......", "######..", ".....##.",
              ".....##.", "##...##.", ".#####..", "........"],
    '6': [0, ".#####..", "##......", "##......", "######..",
              "##...##.", "##...##.", "##...##.", ".#####.."],
    '7': [0, "#######.", ".....##.", "....##..", "...##...",
              "...##...", "..##....", "..##....", "..##...."],
    '8': [0, ".#####..", "##...##.", "##...##.", ".#####..",
              "##...##.", "##...##.", ".#####..", "........"],
    '9': [0, ".#####..", "##...##.", "##...##.", "##...##.",
              ".######.", ".....##.", ".....##.", ".#####.."],
    ':': [24, "........", "..##....", "..##....", "........",
              "........", "..##....", "..##....", "........"],
    '-': [24, "........", "........", "........", "..####..",
              "........", "........", "........", "........"],
    '.': [24, "........", "........", "........", "........",
              "........", "..##....", "..##....", "........"],
    'C': [0, ".#####..", "##...##.", "##......", "##......",
              "##......", "##...##.", ".#####..", "........"],
}

def encode_glyph(rows):
    out = []
    for r in rows:
        b = 0
        for i, ch in enumerate(r):
            if ch == '#':
                b |= (1 << (7 - i))
        out.append(b)
    return out

def build_font():
    table = [[0] * 8 for _ in range(128)]
    for ch, (_, *rows) in GLYPHS.items():
        table[ord(ch)] = encode_glyph(rows)
    return table

# ============================================================
# 3) 写文件
# ============================================================
def bytes_to_c(b, per_line=16):
    lines = []
    for i in range(0, len(b), per_line):
        chunk = b[i:i + per_line]
        lines.append("    " + ", ".join("0x%02X" % c for c in chunk) + ",")
    return "\n".join(lines)

frames = {
    "pet_frame_open":  render_frame("open"),
    "pet_frame_blink": render_frame("blink"),
    "pet_frame_happy": render_frame("happy"),
    "pet_frame_sleep": render_frame("sleep"),
    "pet_frame_sad":   render_frame("sad"),
}

# 预览(把 128x64 缩成 64 行宽的字符画, 目测脸形是否正确)
def preview(frame):
    for y in range(0, 64, 2):
        row = "".join("#" if (frame[y][x] or frame[y + 1][x]) else "."
                      for x in range(0, 128, 2))
        print(row)

print("== 预览 pet_frame_open ==")
preview(frames["pet_frame_open"])

with open(os.path.join(OUT_DIR, "pet_frames.h"), "w", encoding="utf-8-sig") as f:
    f.write("/* 自动生成的宠物动画帧 (128x64, 单色, 列优先纵向编码)\n")
    f.write(" * 每帧 1024 字节 = 128 列 x 8 页, 每字节 bit0 = 该页最顶行像素\n")
    f.write(" * 如需换形象, 改 gen_assets.py 重跑, 或用画图软件导出图后重新编码\n */\n")
    f.write("#ifndef PET_FRAMES_H\n#define PET_FRAMES_H\n\n")
    for name, fr in frames.items():
        data = encode_frame(fr)
        f.write("/* %s */\nstatic const unsigned char %s[1024] = {\n%s\n};\n\n"
                % (name, name, bytes_to_c(data)))
    f.write("#endif\n")

font = build_font()
with open(os.path.join(OUT_DIR, "font8x8.h"), "w", encoding="utf-8-sig") as f:
    f.write("/* 自动生成的 8x8 ASCII 字库 (每位一个字节, 横向扫描, bit7=最左)\n")
    f.write(" * 只有数字和少量符号, 汉字请用字模软件另外生成 16x16 字库\n */\n")
    f.write("#ifndef FONT8X8_H\n#define FONT8X8_H\n\n")
    f.write("static const unsigned char font8x8[128][8] = {\n")
    for i, g in enumerate(font):
        f.write("    { %s }, /* %3d (0x%02X) %s */\n" % (
            ", ".join("0x%02X" % b for b in g), i, i,
            chr(i) if 32 <= i < 127 else ""))
    f.write("};\n\n#endif\n")

print("\n生成完毕: pet_frames.h, font8x8.h")