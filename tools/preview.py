"""Export a static layout illustration, not a hardware screenshot or rendering test."""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont
from generate_font import FONT, main as generate_font

ROOT = Path(__file__).resolve().parents[1]
if not FONT.exists():
    generate_font()
font18 = ImageFont.truetype(str(FONT),18)
font14 = ImageFont.truetype(str(FONT),14)
font26 = ImageFont.truetype(str(FONT),26)
BG, CARD, WHITE, MUTED = '#080810', '#101820', '#eef0ed', '#8996a4'
GREEN, AMBER, RED = '#55e674', '#ffad4d', '#ffaaa0'
ACCENT = (GREEN, '#55bfff', AMBER)

def text(draw, xy, value, font, color):
    box = font.getbbox(value)
    draw.text((xy[0], xy[1]-box[1]),value,font=font,fill=color)

def screen(old=False, warm=False):
    im=Image.new('RGB',(320,240),BG)
    d=ImageDraw.Draw(im)
    text(d,(12,7),'土壤養分檢測',font18,WHITE)
    text(d,(226,8),'NPK / FIELD',font14,MUTED)
    d.line((12,30,308,30),fill=CARD)
    color=RED if old else AMBER if warm else GREEN
    d.ellipse((13,41,19,47),fill=color)
    text(d,(26,36),'無回應：檢查電源與接線' if old else '等待反應' if warm else '即時量測',font18,color)
    if warm: text(d,(110,36),'10 s',font14,AMBER)
    for i,(symbol,label,value) in enumerate(zip('NPK','氮磷鉀',('37','49','40'))):
        y=60+i*43
        d.rounded_rectangle((12,y,307,y+38),radius=5,fill=CARD)
        d.rounded_rectangle((12,y,15,y+38),radius=2,fill=ACCENT[i])
        text(d,(25,y+6),symbol,font26,ACCENT[i])
        text(d,(54,y+11),label,font18,WHITE)
        value='---' if warm else value
        w=d.textlength(value,font=font26)
        text(d,(238-w,y+6),value,font26,MUTED if old else WHITE)
        text(d,(250,y+15),'mg/kg',font14,MUTED)
    if warm: text(d,(12,190),'尚無有效資料',font18,MUTED)
    else:
        text(d,(12,190),'舊資料' if old else '更新於',font18,AMBER if old else MUTED)
        text(d,(75,190),'12' if old else '1',font14,MUTED)
        text(d,(158,190),'秒前',font18,MUTED)
    d.rounded_rectangle((12,211,153,238),radius=4,fill=CARD)
    d.rounded_rectangle((166,211,307,238),radius=4,fill='#204850')
    text(d,(65,216),'暫停',font18,WHITE)
    text(d,(201,216),'重新量測',font18,WHITE)
    return im

out=Image.new('RGB',(1060,360),'#edf1f3')
d=ImageDraw.Draw(out)
text(d,(24,18),'繁體中文介面設計示意｜320 × 240',font26,'#19313d')
for i,(label,im) in enumerate([('暖機等待',screen(warm=True)),('即時讀值',screen()),('通訊失敗與舊資料',screen(old=True))]):
    x=24+i*346
    text(d,(x,65),label,font18,'#19313d')
    out.paste(im,(x,94))
text(d,(24,340),'示意值取自手冊範例，非實機量測。字型細節以實機為準。',font14,'#536a76')
out.save(ROOT/'docs/ui-preview.png')
print('Created docs/ui-preview.png')
