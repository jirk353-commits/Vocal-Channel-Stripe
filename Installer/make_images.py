"""Draws Vocal Strip installer art: wizard images + icon (seven module colours)."""
import sys
from PIL import Image, ImageDraw, ImageFont, ImageFilter
fontpath = sys.argv[1]
COLS = [(79, 216, 255), (255, 179, 71), (255, 111, 168), (255, 122, 69), (169, 139, 255), (124, 240, 176), (91, 157, 255)]
HEIGHTS = [0.55, 0.85, 0.4, 0.7, 0.95, 0.6, 0.75]

def bg(w, h):
    img = Image.new('RGBA', (w, h)); d = ImageDraw.Draw(img)
    for y in range(h):
        t = y / h
        d.line([(0, y), (w, y)], fill=(int(29 - 18*t), int(31 - 19*t), int(36 - 21*t), 255))
    return img

def bars(img, x0, y0, w, h):
    glow = Image.new('RGBA', img.size, (0, 0, 0, 0)); g = ImageDraw.Draw(glow)
    n = len(COLS); bw = w / (n * 1.6)
    for i, (c, hh) in enumerate(zip(COLS, HEIGHTS)):
        x = x0 + i * bw * 1.6
        top = y0 + h * (1 - hh)
        g.rounded_rectangle([x, top, x + bw, y0 + h], radius=bw / 2.5, fill=c + (255,))
    img.alpha_composite(glow.filter(ImageFilter.GaussianBlur(w * 0.03)))
    img.alpha_composite(glow)

W, H = 328, 628
img = bg(W, H)
bars(img, 44, 90, 250, 230)
d = ImageDraw.Draw(img)
f1 = ImageFont.truetype(fontpath, 38); f2 = ImageFont.truetype(fontpath, 22); f3 = ImageFont.truetype(fontpath, 15)
def ctext(y, s, f, col):
    w = d.textlength(s, font=f); d.text(((W - w) / 2, y), s, font=f, fill=col)
ctext(400, "VOCAL STRIP", f1, (255, 214, 150))
ctext(458, "VOCAL CHANNEL STRIP", f3, (170, 176, 190))
d.line([(60, 525), (W - 60, 525)], fill=(110, 90, 60), width=2)
ctext(550, "ROHIT JI", f2, (255, 179, 71))
img.convert('RGB').save('wizard-large.bmp')
img.convert('RGB').resize((164, 314), Image.LANCZOS).save('wizard-large-100.bmp')

s = bg(110, 110); bars(s, 14, 14, 84, 82)
s.convert('RGB').save('wizard-small.bmp')
s.convert('RGB').resize((55, 55), Image.LANCZOS).save('wizard-small-100.bmp')

ic = Image.new('RGBA', (512, 512), (0, 0, 0, 0))
plate = bg(480, 480); m = Image.new('L', (480, 480), 0); ImageDraw.Draw(m).rounded_rectangle([0, 0, 479, 479], 100, fill=255)
ic.paste(plate, (16, 16), m)
bars(ic, 80, 90, 360, 330)
ic.save('icon.ico', sizes=[(16, 16), (24, 24), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)])
